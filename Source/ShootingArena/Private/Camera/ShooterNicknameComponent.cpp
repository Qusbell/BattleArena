#include "Camera/ShooterNicknameComponent.h"
#include "ShooterDisplayValidation.h"

#include "Camera/DeathCamActor.h"
#include "Camera/ShooterDisplayLibrary.h"
#include "Camera/ShooterNicknameWidget.h"
#include "Camera/ShooterRevengeComponent.h"
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
	bEndingPlay = false;
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!IsValid(PC) || PC->IsActorBeingDestroyed() || !PC->IsLocalController() || GetNetMode() == NM_DedicatedServer)
	{
		SetComponentTickEnabled(false);
		return;
	}
	ApplyNicknameSettings(Settings.LoadSynchronous());
	if (!PC->GetLocalPlayer()) return;
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
	if (bEndingPlay || IsBeingDestroyed()) return;
	UpdateTarget(nullptr);
	if (bEndingPlay || IsBeingDestroyed()) return;
	ClearDisplay();
	if (!IsValid(NewSettings))
	{
		ShooterClass = nullptr;
		UE_LOG(LogTemp, Warning, TEXT("ShooterNickname: no settings asset assigned to %s."), *GetNameSafe(GetOwner()));
		return;
	}
	Settings = NewSettings;
	ShooterClass = NewSettings->ShooterClass.LoadSynchronous();
	TraceChannel = NewSettings->TraceChannel < ECC_MAX ? NewSettings->TraceChannel.GetValue() : ECC_GameTraceChannel2;
	bUseEquippedWeaponTraceChannel = NewSettings->bUseEquippedWeaponTraceChannel;
	MaxDistance = ShooterDisplayValidation::FiniteClamp(NewSettings->MaxDistance, 1.0f, MAX_flt, 20000.0f);
	SetCrosshairUV(NewSettings->CrosshairUV);
	SetNicknameStyle(NewSettings->Style);
}

void UShooterNicknameComponent::SetNicknameStyle(const FShooterNicknameStyle& NewStyle)
{
	if (bEndingPlay || IsBeingDestroyed()) return;
	Style = NewStyle;
	Style.LingerTime = ShooterDisplayValidation::FiniteClamp(Style.LingerTime, 0.0f, MAX_flt, 0.0f);
	Style.Font.Size = ShooterDisplayValidation::FiniteClamp(Style.Font.Size, 1.0f, 200.0f, 20.0f);
	Style.Color = ShooterDisplayValidation::FiniteColor(Style.Color, FLinearColor::White);
	Style.RevengeColor = ShooterDisplayValidation::FiniteColor(Style.RevengeColor, FLinearColor::Red);
	Style.Font.OutlineSettings.OutlineColor = ShooterDisplayValidation::FiniteColor(Style.Font.OutlineSettings.OutlineColor, FLinearColor::Black);
	if (Style.Offset.ContainsNaN()) Style.Offset = FVector2D(0, 28);
	Style.Font.OutlineSettings.OutlineSize = FMath::Clamp(Style.Font.OutlineSettings.OutlineSize, 0, 16);
	if (IsValid(Widget)) Widget->RefreshDisplay();
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
	if (UV.ContainsNaN()) UV = FVector2D(0.5, 0.5);
	CrosshairUV = FVector2D(FMath::Clamp(UV.X, 0.0, 1.0), FMath::Clamp(UV.Y, 0.0, 1.0));
}

void UShooterNicknameComponent::ResetNicknameStyle()
{
	if (const UShooterNicknameSettings* Defaults = Settings.Get()) SetNicknameStyle(Defaults->Style);
}

bool UShooterNicknameComponent::ResolveAimRay(FVector& Origin, FVector& Direction)
{
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	ULocalPlayer* Player = IsValid(PC) ? PC->GetLocalPlayer() : nullptr;
	if (!Player || !Player->ViewportClient || !Player->ViewportClient->Viewport) return false;
	FSceneViewProjectionData Projection;
	if (!Player->GetProjectionData(Player->ViewportClient->Viewport, Projection)) return false;
	const FIntRect Rect = Projection.GetConstrainedViewRect();
	if (Rect.Width() <= 0 || Rect.Height() <= 0) return false;
	CrosshairViewportPosition = FVector2D(Rect.Min) + FVector2D(Rect.Size()) * CrosshairUV;
	FSceneView::DeprojectScreenToWorld(CrosshairViewportPosition, Rect,
		Projection.ComputeViewProjectionMatrix().InverseFast(), Origin, Direction);
	return !Origin.ContainsNaN() && !Direction.ContainsNaN() && !Direction.IsNearlyZero();
}

ECollisionChannel UShooterNicknameComponent::ResolveDamageTraceChannel(float& Distance) const
{
	if (!bUseEquippedWeaponTraceChannel) return TraceChannel;
	const APlayerController* PC = Cast<APlayerController>(GetOwner());
	APawn* Pawn = IsValid(PC) ? PC->GetPawn() : nullptr;
	if (!IsValid(Pawn) || Pawn->IsActorBeingDestroyed()) return TraceChannel;
	TInlineComponentArray<UActorComponent*> Components(Pawn);
	for (UActorComponent* Component : Components)
	{
		if (!IsValid(Component) || Component->IsBeingDestroyed()) continue;
		bool bInventory = false;
		for (UClass* Class = Component->GetClass(); Class; Class = Class->GetSuperClass())
			bInventory |= Class->GetName() == TEXT("BPC_Inventory_C");
		UFunction* GetSelected = bInventory ? Component->FindFunction(TEXT("GetSelectedItem")) : nullptr;
		if (!GetSelected) continue;
		FStructOnScope Parameters(GetSelected);
		Component->ProcessEvent(GetSelected, Parameters.GetStructMemory());
		if (!IsValid(Component) || Component->IsBeingDestroyed() || !IsValid(Pawn) || Pawn->IsActorBeingDestroyed()) return TraceChannel;
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
			const uint8 QueryChannel = ChannelProperty->GetPropertyValue_InContainer(Data);
			if (QueryChannel >= TraceTypeQuery_MAX) continue;
			if (const FNumericProperty* RangeProperty = FindFProperty<FNumericProperty>(Data->GetClass(), TEXT("range")))
				if (RangeProperty->IsFloatingPoint())
				{
					const double Range = RangeProperty->GetFloatingPointPropertyValue(RangeProperty->ContainerPtrToValuePtr<void>(Data));
					if (FMath::IsFinite(Range)) Distance = FMath::Min(Distance, static_cast<float>(FMath::Clamp(Range, 0.0, static_cast<double>(MAX_flt))));
				}
			const ECollisionChannel WeaponChannel = UEngineTypes::ConvertToCollisionChannel(static_cast<ETraceTypeQuery>(QueryChannel));
			if (WeaponChannel < ECC_MAX) return WeaponChannel;
		}
	}
	return TraceChannel;
}

APawn* UShooterNicknameComponent::FindAimTarget(const FVector& Origin, const FVector& Direction)
{
	const APlayerController* PC = Cast<APlayerController>(GetOwner());
	UWorld* World = GetWorld();
	if (bEndingPlay || IsBeingDestroyed() || !IsValid(PC) || PC->IsActorBeingDestroyed()
		|| !World || World->bIsTearingDown || Origin.ContainsNaN() || Direction.ContainsNaN()
		|| Direction.IsNearlyZero()) return nullptr;
	float Distance = MaxDistance;
	const ECollisionChannel Channel = ResolveDamageTraceChannel(Distance);
	if (Channel >= ECC_MAX || !FMath::IsFinite(Distance) || Distance <= 0.0f) return nullptr;
	const FVector End = Origin + Direction.GetSafeNormal() * Distance;
	if (End.ContainsNaN()
		|| IsBeingDestroyed() || !IsValid(PC) || PC->IsActorBeingDestroyed() || World->bIsTearingDown) return nullptr;
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
	World->LineTraceMultiByChannel(Hits, Origin, End, Channel, Query);
	for (const FHitResult& Hit : Hits)
	{
		APawn* Shooter = Cast<APawn>(Hit.GetActor());
		if (IsValid(Shooter) && ShooterClass && Shooter->IsA(ShooterClass) && !Shooter->IsHidden()
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
	UWorld* World = GetWorld();
	if (bEndingPlay || IsBeingDestroyed() || !IsValid(PC) || PC->IsActorBeingDestroyed() || !World
		|| World->bIsTearingDown || !Style.bEnabled || !ShooterClass || Cast<ADeathCamActor>(PC->GetViewTarget()))
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
	// Blueprint target-change listeners can disable or destroy this component/pawn.
	if (bEndingPlay || IsBeingDestroyed() || !Style.bEnabled || World->bIsTearingDown)
	{
		ClearDisplay();
		return;
	}
	if (IsValid(Target) && !Target->IsActorBeingDestroyed())
	{
		DisplayedTarget = Target;
		DisplayedName = ResolveNickname(Target);
		if (bEndingPlay || IsBeingDestroyed() || !Style.bEnabled || !IsValid(Target) || Target->IsActorBeingDestroyed())
		{
			ClearDisplay();
			return;
		}
		LostAimTime = -1.0;
	}
	else if (!DisplayedName.IsEmpty())
	{
		if (LostAimTime < 0.0) LostAimTime = GetWorld()->GetTimeSeconds();
		if (!DisplayedTarget.IsValid() || !UShooterDisplayLibrary::IsShooterAlive(DisplayedTarget.Get())
			|| GetWorld()->GetTimeSeconds() - LostAimTime >= Style.LingerTime)
			ClearDisplay();
	}
	if (IsValid(Widget)) Widget->RefreshDisplay();
}

void UShooterNicknameComponent::UpdateTarget(APawn* Target)
{
	if (bEndingPlay || IsBeingDestroyed()) return;
	if (!IsValid(Target) || Target->IsActorBeingDestroyed()) Target = nullptr;
	if (AimTarget.Get() == Target) return;
	AimTarget = Target;
	OnAimTargetChanged.Broadcast(Target);
}

void UShooterNicknameComponent::ClearDisplay()
{
	DisplayedTarget.Reset();
	DisplayedName = FText::GetEmpty();
	LostAimTime = -1.0;
	if (IsValid(Widget)) Widget->RefreshDisplay();
}

FText UShooterNicknameComponent::ResolveNickname_Implementation(APawn* Shooter) const
{
	return UShooterDisplayLibrary::GetShooterDisplayName(Shooter);
}

FLinearColor UShooterNicknameComponent::GetDisplayedNicknameColor() const
{
	const AActor* Owner = GetOwner();
	const UShooterRevengeComponent* Revenge = IsValid(Owner) ? Owner->FindComponentByClass<UShooterRevengeComponent>() : nullptr;
	return IsValid(Revenge) && Revenge->IsRevengeTarget(DisplayedTarget.Get()) ? Style.RevengeColor : Style.Color;
}

void UShooterNicknameComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	bEndingPlay = true;
	AimTarget.Reset();
	ClearDisplay();
	if (IsValid(Widget)) Widget->RemoveFromParent();
	Widget = nullptr;
	Super::EndPlay(Reason);
}
