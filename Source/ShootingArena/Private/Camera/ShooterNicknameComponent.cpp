#include "Camera/ShooterNicknameComponent.h"

#include "Camera/DeathCamActor.h"
#include "Camera/ShooterDisplayLibrary.h"
#include "Camera/ShooterNicknameWidget.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "SceneView.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"

UShooterNicknameComponent::UShooterNicknameComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
	Settings = TSoftObjectPtr<UShooterNicknameSettings>(FSoftObjectPath(TEXT("/Game/QuakeLike_1_0/Data/Player/DA_ShooterNickname_Default.DA_ShooterNickname_Default")));
}

void UShooterNicknameComponent::BeginPlay()
{
	Super::BeginPlay();
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC || !PC->IsLocalController() || GetNetMode() == NM_DedicatedServer)
	{
		SetComponentTickEnabled(false);
		return;
	}
	ApplyNicknameSettings(Settings.LoadSynchronous());
	Widget = CreateWidget<UShooterNicknameWidget>(PC, UShooterNicknameWidget::StaticClass());
	if (Widget)
	{
		Widget->SetDisplayComponent(this);
		Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
		Widget->AddToPlayerScreen(20);
	}
}

void UShooterNicknameComponent::ApplyNicknameSettings(UShooterNicknameSettings* NewSettings)
{
	UpdateTarget(nullptr);
	ClearDisplay();
	if (!IsValid(NewSettings))
	{
		ShooterClass = nullptr;
		UE_LOG(LogTemp, Warning, TEXT("ShooterNickname: no settings asset assigned to %s."), *GetNameSafe(GetOwner()));
		return;
	}
	Settings = NewSettings;
	ShooterClass = NewSettings->ShooterClass.LoadSynchronous();
	TraceChannel = NewSettings->TraceChannel;
	bUseEquippedWeaponTraceChannel = NewSettings->bUseEquippedWeaponTraceChannel;
	MaxDistance = FMath::Max(1.0f, NewSettings->MaxDistance);
	SetCrosshairUV(NewSettings->CrosshairUV);
	SetNicknameStyle(NewSettings->Style);
}

void UShooterNicknameComponent::SetNicknameStyle(const FShooterNicknameStyle& NewStyle)
{
	Style = NewStyle;
	Style.LingerTime = FMath::Max(0.0f, Style.LingerTime);
	Style.Font.Size = FMath::Clamp(Style.Font.Size, 1.0f, 200.0f);
	Style.Font.OutlineSettings.OutlineSize = FMath::Clamp(Style.Font.OutlineSettings.OutlineSize, 0, 16);
	if (Widget) Widget->RefreshDisplay();
	if (!Style.bEnabled)
	{
		UpdateTarget(nullptr);
		ClearDisplay();
	}
	else if (!AimTarget.IsValid() && LostAimTime >= 0.0 && GetWorld()
		&& GetWorld()->GetTimeSeconds() - LostAimTime >= Style.LingerTime)
	{
		ClearDisplay();
	}
}

void UShooterNicknameComponent::SetNicknameEnabled(bool bEnabled)
{
	FShooterNicknameStyle NewStyle = Style;
	NewStyle.bEnabled = bEnabled;
	SetNicknameStyle(NewStyle);
}

void UShooterNicknameComponent::SetNicknameLingerTime(float Seconds)
{
	FShooterNicknameStyle NewStyle = Style;
	NewStyle.LingerTime = Seconds;
	SetNicknameStyle(NewStyle);
}

void UShooterNicknameComponent::SetCrosshairUV(FVector2D UV)
{
	CrosshairUV = FVector2D(FMath::Clamp(UV.X, 0.0, 1.0), FMath::Clamp(UV.Y, 0.0, 1.0));
}

void UShooterNicknameComponent::ResetNicknameStyle()
{
	if (const UShooterNicknameSettings* Defaults = Settings.Get()) SetNicknameStyle(Defaults->Style);
}

bool UShooterNicknameComponent::ResolveAimRay(FVector& Origin, FVector& Direction)
{
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	ULocalPlayer* Player = PC ? PC->GetLocalPlayer() : nullptr;
	if (!Player || !Player->ViewportClient || !Player->ViewportClient->Viewport) return false;
	FSceneViewProjectionData Projection;
	if (!Player->GetProjectionData(Player->ViewportClient->Viewport, Projection)) return false;
	const FIntRect Rect = Projection.GetConstrainedViewRect();
	if (Rect.Width() <= 0 || Rect.Height() <= 0) return false;
	CrosshairViewportPosition = FVector2D(Rect.Min) + FVector2D(Rect.Size()) * CrosshairUV;
	FSceneView::DeprojectScreenToWorld(CrosshairViewportPosition, Rect,
		Projection.ComputeViewProjectionMatrix().InverseFast(), Origin, Direction);
	return true;
}

ECollisionChannel UShooterNicknameComponent::ResolveDamageTraceChannel(float& Distance) const
{
	if (!bUseEquippedWeaponTraceChannel) return TraceChannel;
	const APlayerController* PC = Cast<APlayerController>(GetOwner());
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!Pawn) return TraceChannel;
	TInlineComponentArray<UActorComponent*> Components(Pawn);
	for (UActorComponent* Component : Components)
	{
		bool bInventory = false;
		for (UClass* Class = Component->GetClass(); Class; Class = Class->GetSuperClass())
			bInventory |= Class->GetName() == TEXT("BPC_Inventory_C");
		UFunction* GetSelected = bInventory ? Component->FindFunction(TEXT("GetSelectedItem")) : nullptr;
		if (!GetSelected) continue;
		FStructOnScope Parameters(GetSelected);
		Component->ProcessEvent(GetSelected, Parameters.GetStructMemory());
		for (TFieldIterator<FProperty> It(GetSelected); It; ++It)
		{
			if (!It->HasAnyPropertyFlags(CPF_OutParm)) continue;
			const FObjectPropertyBase* ItemProperty = CastField<FObjectPropertyBase>(*It);
			UObject* Weapon = ItemProperty ? ItemProperty->GetObjectPropertyValue_InContainer(Parameters.GetStructMemory()) : nullptr;
			const FObjectPropertyBase* DataProperty = IsValid(Weapon) ? FindFProperty<FObjectPropertyBase>(Weapon->GetClass(), TEXT("weaponData")) : nullptr;
			UObject* Data = DataProperty ? DataProperty->GetObjectPropertyValue_InContainer(Weapon) : nullptr;
			if (!IsValid(Data)) continue;
			const FByteProperty* ChannelProperty = FindFProperty<FByteProperty>(Data->GetClass(), TEXT("traceChannel"));
			if (!ChannelProperty) continue;
			if (const FNumericProperty* RangeProperty = FindFProperty<FNumericProperty>(Data->GetClass(), TEXT("range")))
				if (RangeProperty->IsFloatingPoint())
					Distance = FMath::Min(Distance, FMath::Max(0.0f, static_cast<float>(RangeProperty->GetFloatingPointPropertyValue(RangeProperty->ContainerPtrToValuePtr<void>(Data)))));
			return UEngineTypes::ConvertToCollisionChannel(static_cast<ETraceTypeQuery>(ChannelProperty->GetPropertyValue_InContainer(Data)));
		}
	}
	return TraceChannel;
}

APawn* UShooterNicknameComponent::FindAimTarget(const FVector& Origin, const FVector& Direction)
{
	const APlayerController* PC = CastChecked<APlayerController>(GetOwner());
	float Distance = MaxDistance;
	const ECollisionChannel Channel = ResolveDamageTraceChannel(Distance);
	FCollisionQueryParams Query(SCENE_QUERY_STAT(ShooterNicknameDamage), false);
	Query.AddIgnoredActor(PC->GetPawn());
	Query.AddIgnoredActor(PC->GetViewTarget());
	if (APawn* LocalPawn = PC->GetPawn())
	{
		TArray<AActor*> Attached;
		LocalPawn->GetAttachedActors(Attached, true, true);
		Query.AddIgnoredActors(Attached);
	}
	TArray<FHitResult> Hits;
	// Multi traces include the overlap capsules used by RailGun damage. The bool return
	// only reports blocking hits, so inspect the hit array even when that return is false.
	GetWorld()->LineTraceMultiByChannel(Hits, Origin, Origin + Direction * Distance, Channel, Query);
	for (const FHitResult& Hit : Hits)
	{
		APawn* Shooter = Cast<APawn>(Hit.GetActor());
		if (Shooter && ShooterClass && Shooter->IsA(ShooterClass) && !Shooter->IsHidden()
			&& Shooter->CanBeDamaged() && UShooterDisplayLibrary::IsShooterAlive(Shooter)) return Shooter;
		// A gun actor is never promoted to its shooter owner. Its blocking collision, like
		// a wall, prevents a body behind it from being selected; NoCollision guns are ignored.
		if (Hit.bBlockingHit) break;
	}
	return nullptr;
}

void UShooterNicknameComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
	Super::TickComponent(DeltaTime, TickType, TickFunction);
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC || !Style.bEnabled || !ShooterClass || Cast<ADeathCamActor>(PC->GetViewTarget()))
	{
		UpdateTarget(nullptr);
		ClearDisplay();
		return;
	}
	FVector Origin, Direction;
	if (!ResolveAimRay(Origin, Direction))
	{
		UpdateTarget(nullptr);
		ClearDisplay();
		return;
	}
	APawn* Target = FindAimTarget(Origin, Direction);
	UpdateTarget(Target);
	if (Target)
	{
		DisplayedTarget = Target;
		DisplayedName = ResolveNickname(Target);
		LostAimTime = -1.0;
	}
	else if (!DisplayedName.IsEmpty())
	{
		if (LostAimTime < 0.0) LostAimTime = GetWorld()->GetTimeSeconds();
		if (!DisplayedTarget.IsValid() || !UShooterDisplayLibrary::IsShooterAlive(DisplayedTarget.Get())
			|| GetWorld()->GetTimeSeconds() - LostAimTime >= Style.LingerTime)
			ClearDisplay();
	}
	if (Widget) Widget->RefreshDisplay();
}

void UShooterNicknameComponent::UpdateTarget(APawn* Target)
{
	if (AimTarget.Get() == Target) return;
	AimTarget = Target;
	OnAimTargetChanged.Broadcast(Target);
}

void UShooterNicknameComponent::ClearDisplay()
{
	DisplayedTarget.Reset();
	DisplayedName = FText::GetEmpty();
	LostAimTime = -1.0;
	if (Widget) Widget->RefreshDisplay();
}

FText UShooterNicknameComponent::ResolveNickname_Implementation(APawn* Shooter) const
{
	return UShooterDisplayLibrary::GetShooterDisplayName(Shooter);
}

void UShooterNicknameComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	ClearDisplay();
	if (Widget) Widget->RemoveFromParent();
	Widget = nullptr;
	Super::EndPlay(Reason);
}
