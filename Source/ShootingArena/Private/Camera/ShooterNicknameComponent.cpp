#include "Camera/ShooterNicknameComponent.h"
#include "ShooterDisplayValidation.h"

#include "Camera/DeathCamActor.h"
#include "Camera/ShooterDisplayLibrary.h"
#include "Camera/ShooterNicknameWidget.h"
#include "Camera/ShooterRevengeComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "SceneView.h"

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
	MaxDistance = ShooterDisplayValidation::FiniteClamp(NewSettings->MaxDistance, 1.0f, MAX_flt, 20000.0f);
	SetNicknameSphereTraceEnabled(NewSettings->bUseSphereTraceFallback);
	SetNicknameSphereTraceRadius(NewSettings->SphereTraceRadius);
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

void UShooterNicknameComponent::SetNicknameSphereTraceEnabled(bool bEnabled)
{
	if (!bEndingPlay && !IsBeingDestroyed()) bUseSphereTraceFallback = bEnabled;
}

void UShooterNicknameComponent::SetNicknameSphereTraceRadius(float Radius)
{
	if (!bEndingPlay && !IsBeingDestroyed())
		SphereTraceRadius = ShooterDisplayValidation::FiniteClamp(Radius, 0.0f, 1000.0f, 0.0f);
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

APawn* UShooterNicknameComponent::FindAimTarget(const FVector& Origin, const FVector& Direction)
{
	const APlayerController* PC = Cast<APlayerController>(GetOwner());
	UWorld* World = GetWorld();
	if (bEndingPlay || IsBeingDestroyed() || !IsValid(PC) || PC->IsActorBeingDestroyed()
		|| !World || World->bIsTearingDown || Origin.ContainsNaN() || Direction.ContainsNaN()
		|| Direction.IsNearlyZero()) return nullptr;
	const FVector RayDirection = Direction.GetSafeNormal();
	if (RayDirection.ContainsNaN() || RayDirection.IsNearlyZero()) return nullptr;
	const float Distance = MaxDistance;
	FCollisionResponseParams Responses;
	// Nickname detection uses one hitscan-style query for every equipped weapon.
	const ECollisionChannel Channel = TraceChannel;
	if (Channel >= ECC_MAX || !FMath::IsFinite(Distance) || Distance <= 0.0f) return nullptr;
	const FVector End = Origin + RayDirection * Distance;
	if (End.ContainsNaN()
		|| bEndingPlay || IsBeingDestroyed() || !IsValid(PC) || PC->IsActorBeingDestroyed()
		|| GetWorld() != World || World->bIsTearingDown) return nullptr;
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
	auto IsTarget = [this, World](APawn* Shooter)
	{
		return IsValid(Shooter) && !Shooter->IsActorBeingDestroyed() && Shooter->GetWorld() == World && ShooterClass
			&& Shooter->IsA(ShooterClass) && !Shooter->IsHidden() && Shooter->CanBeDamaged()
			&& UShooterDisplayLibrary::IsShooterAlive(Shooter);
	};
	// Multi traces include the overlap capsules used by RailGun damage. The bool return
	// only reports blocking hits, so inspect the hit array even when that return is false.
	World->LineTraceMultiByChannel(Hits, Origin, End, Channel, Query, Responses);
	for (const FHitResult& Hit : Hits)
	{
		APawn* Shooter = Cast<APawn>(Hit.GetActor());
		if (IsTarget(Shooter)) return Shooter;
		// A gun actor is never promoted to its shooter owner. Its blocking collision, like
		// a wall, prevents a body behind it from being selected; NoCollision guns are ignored.
		if (Hit.bBlockingHit) break;
	}
	if (!bUseSphereTraceFallback || !FMath::IsFinite(SphereTraceRadius) || SphereTraceRadius <= 0.0f) return nullptr;
	// Preserve the original damage responses for visibility checks. The candidate
	// sweep collects overlaps so a wall expanded by the radius cannot stop the search
	// before a visible nearby shooter; it never grants visibility through that wall.
	FCollisionResponseParams CandidateResponses = Responses;
	for (int32 Index = 0; Index < ECC_MAX; ++Index)
	{
		const ECollisionChannel CandidateChannel = static_cast<ECollisionChannel>(Index);
		if (CandidateResponses.CollisionResponse.GetResponse(CandidateChannel) == ECR_Block)
			CandidateResponses.CollisionResponse.SetResponse(CandidateChannel, ECR_Overlap);
	}
	Hits.Reset();
	World->SweepMultiByChannel(Hits, Origin, End, FQuat::Identity, Channel,
		FCollisionShape::MakeSphere(SphereTraceRadius), Query, CandidateResponses);
	TArray<FHitResult> VisibilityHits;
	for (const FHitResult& Hit : Hits)
	{
		APawn* Shooter = Cast<APawn>(Hit.GetActor());
		if (!IsTarget(Shooter)) continue;
		FVector Contact = Hit.ImpactPoint;
		// Initial overlaps may not supply a surface point. Resolve it before testing
		// visibility, otherwise a nearby wall could be bypassed by a zero-length ray.
		if (Hit.bStartPenetrating)
		{
			UPrimitiveComponent* Collision = Hit.GetComponent();
			if (!IsValid(Collision) || Collision->IsBeingDestroyed() || !Collision->IsRegistered()
				|| !Collision->IsQueryCollisionEnabled()) continue;
			const float SurfaceDistance = Collision->GetClosestPointOnCollision(Origin, Contact);
			if (!FMath::IsFinite(SurfaceDistance) || SurfaceDistance < 0.0f) continue;
		}
		if (Contact.ContainsNaN()) continue;
		const FVector ToContact = Contact - Origin;
		if (ToContact.SizeSquared() > FMath::Square(static_cast<double>(Distance))
			|| FVector::DotProduct(ToContact, RayDirection) < 0.0) continue;
		const FVector VisibilityEnd = Contact + ToContact.GetSafeNormal() * 0.5;
		if (VisibilityEnd.ContainsNaN()) continue;
		VisibilityHits.Reset();
		World->LineTraceMultiByChannel(VisibilityHits, Origin, VisibilityEnd, Channel, Query, Responses);
		bool bVisible = true;
		for (const FHitResult& VisibilityHit : VisibilityHits)
		{
			if (VisibilityHit.GetActor() == Shooter) break;
			if (VisibilityHit.bBlockingHit || IsTarget(Cast<APawn>(VisibilityHit.GetActor())))
			{
				bVisible = false;
				break;
			}
		}
		if (bVisible) return Shooter;
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
	if (bEndingPlay || IsBeingDestroyed() || !Style.bEnabled || !ShooterClass
		|| !IsValid(PC) || PC->IsActorBeingDestroyed() || World->bIsTearingDown
		|| AimTarget.Get() != Target)
	{
		ClearDisplay();
		return;
	}
	if (IsValid(Target) && !Target->IsActorBeingDestroyed())
	{
		DisplayedTarget = Target;
		DisplayedName = ResolveNickname(Target);
		if (bEndingPlay || IsBeingDestroyed() || !Style.bEnabled || !ShooterClass
			|| !IsValid(PC) || PC->IsActorBeingDestroyed() || World->bIsTearingDown
			|| !IsValid(Target) || Target->IsActorBeingDestroyed() || AimTarget.Get() != Target)
		{
			ClearDisplay();
			return;
		}
		LostAimTime = -1.0;
	}
	else if (!DisplayedName.IsEmpty())
	{
		if (LostAimTime < 0.0) LostAimTime = World->GetTimeSeconds();
		if (!DisplayedTarget.IsValid() || !UShooterDisplayLibrary::IsShooterAlive(DisplayedTarget.Get())
			|| World->GetTimeSeconds() - LostAimTime >= Style.LingerTime)
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
