#include "UI/UIBackInputSubsystem.h"

#include "UI/UIBackHandler.h"
#include "UI/UIBackInputSettings.h"

#include "Algo/StableSort.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "CommonActivatableWidget.h"
#include "CommonButtonBase.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

class FUIBackInputProcessor : public IInputProcessor
{
public:
	explicit FUIBackInputProcessor(UUIBackInputSubsystem* InOwner) : Owner(InOwner) {}

	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override {}

	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override
	{
		if (InKeyEvent.IsRepeat())
		{
			return false;
		}
		return Owner.IsValid() && Owner->HandleKeyPressed(InKeyEvent.GetKey());
	}

	virtual const TCHAR* GetDebugName() const override { return TEXT("UIBackInput"); }

private:
	TWeakObjectPtr<UUIBackInputSubsystem> Owner;
};

bool UUIBackInputSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	// 데디케이트 서버에는 UI/Slate가 없습니다.
	return !IsRunningDedicatedServer() && Super::ShouldCreateSubsystem(Outer);
}

void UUIBackInputSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (FSlateApplication::IsInitialized())
	{
		InputProcessor = MakeShared<FUIBackInputProcessor>(this);
		FSlateApplication::Get().RegisterInputPreProcessor(InputProcessor);
	}
}

void UUIBackInputSubsystem::Deinitialize()
{
	if (InputProcessor.IsValid() && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(InputProcessor);
	}
	InputProcessor.Reset();

	Super::Deinitialize();
}

bool UUIBackInputSubsystem::HandleKeyPressed(const FKey& Key)
{
	const UUIBackInputSettings* Settings = GetDefault<UUIBackInputSettings>();
	if (!Settings->BackKeys.Contains(Key))
	{
		return false;
	}

	// 에디터에서 한 프로세스에 PIE 창이 여러 개면 프리프로세서가 전부 호출되므로, 활성 창의 인스턴스만 처리합니다.
	if (UGameViewportClient* ViewportClient = GetGameInstance() ? GetGameInstance()->GetGameViewportClient() : nullptr)
	{
		const TSharedPtr<SWindow> Window = ViewportClient->GetWindow();
		if (Window.IsValid() && FSlateApplication::Get().GetActiveTopLevelWindow() != Window)
		{
			return false;
		}
	}

	return HandleBackAction();
}

bool UUIBackInputSubsystem::HandleBackAction()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	TArray<UUserWidget*> TopLevelWidgets;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, TopLevelWidgets, UUserWidget::StaticClass(), true);

	TArray<UCommonActivatableWidgetContainerBase*> Stacks;

	// 나중에 만들어진 위젯이 보통 위에 있으므로 뒤에서부터 봅니다.
	for (int32 i = TopLevelWidgets.Num() - 1; i >= 0; --i)
	{
		UUserWidget* Widget = TopLevelWidgets[i];
		if (!Widget || !Widget->IsVisible() || !Widget->WidgetTree)
		{
			continue;
		}

		const int32 StackCountBefore = Stacks.Num();
		Widget->WidgetTree->ForEachWidgetAndDescendants([&Stacks](UWidget* Child)
		{
			if (UCommonActivatableWidgetContainerBase* Stack = Cast<UCommonActivatableWidgetContainerBase>(Child))
			{
				Stacks.Add(Stack);
			}
		});

		// 스택을 가진 위젯(RootLayout)은 아래에서 따로 처리합니다. 나머지는 팝업으로 취급합니다.
		if (Stacks.Num() == StackCountBefore && TryHandleWidget(Widget))
		{
			return true;
		}
	}

	// 모달 스택이 먼저, 그 외 스택은 그 다음.
	Algo::StableSort(Stacks, [](const UCommonActivatableWidgetContainerBase* A, const UCommonActivatableWidgetContainerBase* B)
	{
		const bool bAModal = A->GetName().Contains(TEXT("Modal"));
		const bool bBModal = B->GetName().Contains(TEXT("Modal"));
		return bAModal && !bBModal;
	});

	for (UCommonActivatableWidgetContainerBase* Stack : Stacks)
	{
		UCommonActivatableWidget* Active = Stack->GetActiveWidget();
		if (!Active)
		{
			continue;
		}

		// 일반 스택의 바닥 화면(메인 메뉴)은 닫지 않습니다.
		const bool bModal = Stack->GetName().Contains(TEXT("Modal"));
		if (!bModal && Stack->GetNumWidgets() <= 1)
		{
			continue;
		}

		if (TryHandleWidget(Active))
		{
			return true;
		}

		Active->DeactivateWidget();
		return true;
	}

	return false;
}

bool UUIBackInputSubsystem::TryHandleWidget(UUserWidget* Widget) const
{
	if (Widget->Implements<UUIBackHandler>() && IUIBackHandler::Execute_HandleBackAction(Widget))
	{
		return true;
	}
	return TryClickBackButton(Widget);
}

bool UUIBackInputSubsystem::TryClickBackButton(UUserWidget* Widget) const
{
	for (const FName& ButtonName : GetDefault<UUIBackInputSettings>()->BackButtonNames)
	{
		UWidget* Found = Widget->GetWidgetFromName(ButtonName);
		if (!Found || !Found->IsVisible() || !Found->GetIsEnabled())
		{
			continue;
		}

		bool bAncestorsAvailable = true;
		for (UWidget* Current = Found; Current;)
		{
			if (!Current->IsVisible() || !Current->GetIsEnabled())
			{
				bAncestorsAvailable = false;
				break;
			}
			if (Current == Widget) break;
			// Named-slot content can cross a UserWidget's widget tree.
			Current = Current->GetParent() ? static_cast<UWidget*>(Current->GetParent()) : Current->GetTypedOuter<UUserWidget>();
		}
		if (!bAncestorsAvailable) continue;

		if (UCommonButtonBase* CommonButton = Cast<UCommonButtonBase>(Found))
		{
			if (!CommonButton->IsInteractionEnabled())
			{
				continue;
			}
			// 실제 클릭과 같은 경로(HandleButtonClicked → OnButtonBaseClicked 브로드캐스트)를 탑니다. protected UFUNCTION이라 리플렉션으로 호출합니다.
			if (UFunction* ClickFunction = CommonButton->FindFunction(TEXT("HandleButtonClicked")))
			{
				CommonButton->ProcessEvent(ClickFunction, nullptr);
				return true;
			}
		}
		else if (UButton* Button = Cast<UButton>(Found))
		{
			Button->OnClicked.Broadcast();
			return true;
		}
	}
	return false;
}
