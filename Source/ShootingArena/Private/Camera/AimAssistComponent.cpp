#include "Camera/AimAssistComponent.h"
#include "AimAssistMath.h"
#include "Camera/AimAssistSettings.h"
#include "Camera/ShooterDisplayLibrary.h"
#include "Camera/CameraComponent.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/SpectatorPawn.h"
#include "InputCoreTypes.h"
#include "SceneView.h"
#include "TimerManager.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
bool Heart(APawn* Pawn, FVector& Out)
{
	UFunction* Function = Pawn->FindFunction(TEXT("GetHeart"));
	if (!Function) return false;
	FStructOnScope Params(Function);
	Pawn->ProcessEvent(Function, Params.GetStructMemory());
	if (!IsValid(Pawn) || Pawn->IsActorBeingDestroyed()) return false;
	const FStructProperty* Result = FindFProperty<FStructProperty>(Function, TEXT("heartLocation"));
	if (!Result || !Result->HasAnyPropertyFlags(CPF_OutParm) || Result->Struct != TBaseStructure<FVector>::Get()) return false;
	Out = *Result->ContainerPtrToValuePtr<FVector>(Params.GetStructMemory());
	return !Out.ContainsNaN();
}

UCameraComponent* Camera(APawn* Pawn)
{
	const FObjectPropertyBase* Property = Pawn ? FindFProperty<FObjectPropertyBase>(Pawn->GetClass(), TEXT("FP_Camera")) : nullptr;
	return Property ? Cast<UCameraComponent>(Property->GetObjectPropertyValue_InContainer(Pawn)) : nullptr;
}

bool Focused(const APlayerController* PC)
{
	const ULocalPlayer* Player = PC ? PC->GetLocalPlayer() : nullptr;
	const FViewport* Viewport = Player && Player->ViewportClient ? Player->ViewportClient->Viewport : nullptr;
	return Viewport && Viewport->HasFocus() && Viewport->IsForegroundWindow();
}
}

UAimAssistComponent::UAimAssistComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAimAssistComponent::ProcessToggle()
{
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC || !PC->IsLocalController() || !Focused(PC)) return;
	if (PC->WasInputKeyJustPressed(EKeys::F10)
		&& (PC->IsInputKeyDown(EKeys::LeftControl) || PC->IsInputKeyDown(EKeys::RightControl))
		&& (PC->IsInputKeyDown(EKeys::LeftShift) || PC->IsInputKeyDown(EKeys::RightShift))
		&& (PC->IsInputKeyDown(EKeys::LeftAlt) || PC->IsInputKeyDown(EKeys::RightAlt)))
	{
		bEnabled = !bEnabled;
		ShowStatus();
	}
}

bool UAimAssistComponent::CanAssist() const
{
	const APlayerController* PC = Cast<APlayerController>(GetOwner());
	const UWorld* World = GetWorld();
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	return bEnabled && World && !World->bIsTearingDown && !World->IsPaused()
		&& PC && PC->IsLocalController() && PC->IsInState(NAME_Playing) && Focused(PC)
		&& !PC->bShowMouseCursor && !PC->IsLookInputIgnored() && PC->GetViewTarget() == Pawn
		&& IsValid(Pawn) && !Pawn->IsHidden() && Camera(Pawn) && UShooterDisplayLibrary::IsShooterAlive(Pawn)
		&& PC->IsInputKeyDown(EKeys::LeftMouseButton);
}

APawn* UAimAssistComponent::SelectTarget(FVector& OutHeart) const
{
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	ULocalPlayer* Player = PC ? PC->GetLocalPlayer() : nullptr;
	FSceneViewProjectionData Projection;
	if (!Player || !Player->ViewportClient || !Player->ViewportClient->Viewport
		|| !Player->GetProjectionData(Player->ViewportClient->Viewport, Projection)) return nullptr;
	const FIntRect Rect = Projection.GetConstrainedViewRect();
	const FMatrix Matrix = Projection.ComputeViewProjectionMatrix();
	APawn* Self = PC->GetPawn();
	UCameraComponent* Eye = Camera(Self);
	if (!Eye) return nullptr;
	const FVector Origin = Eye->GetComponentLocation();
	return SelectTargetInView(Self, Origin, Rect, Matrix, OutHeart);
}

APawn* UAimAssistComponent::SelectTargetInView(APawn* Self, FVector Origin, const FIntRect& Rect, const FMatrix& Matrix, FVector& OutHeart) const
{
	const FVector2D Center(Rect.Min.X + Rect.Width() * 0.5, Rect.Min.Y + Rect.Height() * 0.5);
	APawn* Best = nullptr;
	double BestDistance = TNumericLimits<double>::Max();
	for (TActorIterator<APawn> It(GetWorld()); It; ++It)
	{
		APawn* Candidate = *It;
		if (Candidate == Self || Candidate->IsHidden() || Candidate->IsActorBeingDestroyed()
			|| Candidate->IsA<ASpectatorPawn>()
			|| !UShooterDisplayLibrary::IsShooterAlive(Candidate)) continue;
		const APlayerState* State = Candidate->GetPlayerState();
		if (State && (State->IsOnlyASpectator() || State->IsSpectator())) continue;
		FVector Position;
		FVector2D Screen;
		if (!Heart(Candidate, Position) || (Position - Origin).IsNearlyZero()
			|| !FSceneView::ProjectWorldToScreen(Position, Rect, Matrix, Screen)
			|| !AimAssistMath::ScreenCandidate(Screen, Rect)) continue;
		const double Distance = FVector2D::DistSquared(Screen, Center);
		if (Distance > BestDistance || (Distance == BestDistance && Best && Candidate->GetUniqueID() > Best->GetUniqueID())) continue;
		FCollisionQueryParams Query(SCENE_QUERY_STAT(AimAssistVisibility), false, Self);
		Query.AddIgnoredActor(Candidate);
		// Check only the existing world geometry channel; the endpoint is inside the target's chest.
		if (GetWorld()->LineTraceTestByChannel(Origin, Position, ECC_Visibility, Query)) continue;
		Best = Candidate;
		BestDistance = Distance;
		OutHeart = Position;
	}
	return Best;
}

bool UAimAssistComponent::TryGetRotation(float DeltaTime, FRotator& OutRotation)
{
	if (!CanAssist() || !FMath::IsFinite(DeltaTime) || DeltaTime <= 0.f) return false;
	FVector Position;
	if (!SelectTarget(Position)) return false;
	APlayerController* PC = CastChecked<APlayerController>(GetOwner());
	const FVector Origin = Camera(PC->GetPawn())->GetComponentLocation();
	OutRotation = AimAssistMath::Interpolate(PC->GetControlRotation(), (Position - Origin).Rotation(),
		DeltaTime, GetDefault<UAimAssistSettings>()->ConvergenceTime);
	return true;
}

void UAimAssistComponent::ShowStatus()
{
	ClearStatus();
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	ULocalPlayer* Player = PC ? PC->GetLocalPlayer() : nullptr;
	if (!Player || !Player->ViewportClient) return;
	StatusWidget = SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(FMargin(0, 64, 0, 0))
	[
		SNew(SBorder).Padding(12)
		[
			SNew(STextBlock).Text(bEnabled
				? NSLOCTEXT("AimAssist", "On", "Aim Assist: ON")
				: NSLOCTEXT("AimAssist", "Off", "Aim Assist: OFF"))
		]
	];
	StatusWidget->SetVisibility(EVisibility::HitTestInvisible);
	Player->ViewportClient->AddViewportWidgetForPlayer(Player, StatusWidget.ToSharedRef(), 100);
	GetWorld()->GetTimerManager().SetTimer(StatusTimer, this, &UAimAssistComponent::ClearStatus, 2.f, false);
}

void UAimAssistComponent::ClearStatus()
{
	if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(StatusTimer);
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	ULocalPlayer* Player = PC ? PC->GetLocalPlayer() : nullptr;
	if (StatusWidget && Player && Player->ViewportClient)
		Player->ViewportClient->RemoveViewportWidgetForPlayer(Player, StatusWidget.ToSharedRef());
	StatusWidget.Reset();
}

void UAimAssistComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	ClearStatus();
	bEnabled = false;
	Super::EndPlay(Reason);
}
