#include "Analyzer/ArenaBehaviorReporterComponent.h"
#include "Analyzer/ArenaMatchLogSubsystem.h"

#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

UArenaBehaviorReporterComponent::UArenaBehaviorReporterComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	SetIsReplicatedByDefault(true);
}

void UArenaBehaviorReporterComponent::ReportAttackInput(bool bPressed)
{
	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn || !Pawn->IsLocallyControlled() || bLastAttack == bPressed)
	{
		return; // only the owner's machine sees the input; repeats of the same state are not new input
	}
	bLastAttack = bPressed;
	ServerAttackInput(bPressed);
}

void UArenaBehaviorReporterComponent::ReportJumpInput()
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (Pawn && Pawn->IsLocallyControlled())
	{
		ServerJumpInput();
	}
}

void UArenaBehaviorReporterComponent::ServerAttackInput_Implementation(bool bPressed)
{
	if (UArenaMatchLogSubsystem* Log = UArenaMatchLogSubsystem::Get(this))
	{
		Log->RecordAttackInput(Cast<APawn>(GetOwner()), bPressed);
	}
}

void UArenaBehaviorReporterComponent::ServerJumpInput_Implementation()
{
	if (UArenaMatchLogSubsystem* Log = UArenaMatchLogSubsystem::Get(this))
	{
		Log->RecordJumpInput(Cast<APawn>(GetOwner()));
	}
}

void UArenaBehaviorReporterComponent::ServerExposure_Implementation(APawn* Target, bool bExposed, bool bOccluded)
{
	if (UArenaMatchLogSubsystem* Log = UArenaMatchLogSubsystem::Get(this))
	{
		Log->RecordExposureReport(Cast<APawn>(GetOwner()), Target, bExposed, bOccluded);
	}
}

void UArenaBehaviorReporterComponent::ClientSetInterval_Implementation(float Seconds)
{
	Interval = Seconds;
	Accumulated = 0.0f;
}

void UArenaBehaviorReporterComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn)
	{
		return;
	}
	if (Pawn->HasAuthority())
	{
		// The server owns the configured interval; push it to the owning machine when it appears or changes.
		// (An RPC rather than a replicated property: property changes made after the match started do not reach clients here.)
		if (const UArenaMatchLogSubsystem* Log = UArenaMatchLogSubsystem::Get(this))
		{
			const float Wanted = float(Log->GetBehaviorSampleInterval());
			if (Wanted != SentInterval)
			{
				SentInterval = Wanted;
				if (Pawn->IsLocallyControlled())
				{
					ClientSetInterval_Implementation(Wanted);
				}
				else if (Pawn->GetController())
				{
					ClientSetInterval(Wanted);
				}
			}
		}
	}
	if (Interval <= 0.0f || !Pawn->IsLocallyControlled())
	{
		return;
	}
	Accumulated += DeltaTime;
	if (Accumulated >= Interval)
	{
		Accumulated = 0.0f;
		PollExposure();
	}
}

void UArenaBehaviorReporterComponent::PollExposure()
{
	APawn* Pawn = Cast<APawn>(GetOwner());
	APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	UWorld* World = GetWorld();
	int32 ViewportX = 0, ViewportY = 0;
	if (PC && World)
	{
		PC->GetViewportSize(ViewportX, ViewportY);
	}
	if (ViewportX <= 0 || ViewportY <= 0)
	{
		return; // no screen to judge against: nothing is reported rather than guessing "not visible"
	}
	FVector Eye;
	FRotator EyeRotation;
	Pawn->GetActorEyesViewPoint(Eye, EyeRotation);

	for (TActorIterator<APawn> It(World); It; ++It)
	{
		APawn* Other = *It;
		if (Other == Pawn || !IsValid(Other))
		{
			continue;
		}
		const FVector TargetLocation = Other->GetActorLocation();
		FVector2D Screen;
		const bool bExposed = PC->ProjectWorldLocationToScreen(TargetLocation, Screen, false)
			&& Screen.X >= 0.0 && Screen.X <= ViewportX && Screen.Y >= 0.0 && Screen.Y <= ViewportY;
		bool bOccluded = false;
		if (bExposed)
		{
			FCollisionQueryParams Params(SCENE_QUERY_STAT(ArenaExposure), false);
			Params.AddIgnoredActor(Pawn);
			Params.AddIgnoredActor(Other);
			FHitResult Hit;
			bOccluded = World->LineTraceSingleByChannel(Hit, Eye, TargetLocation, ECC_Visibility, Params);
		}
		const uint8 Verdict = (bExposed ? 1 : 0) | (bOccluded ? 2 : 0);
		const uint8* Previous = Reported.Find(Other);
		// Exposed targets are re-reported every check so a target that respawned (a new life) is still picked up;
		// a target that is not on screen is reported once, when it leaves.
		if (bExposed || (Previous && *Previous != Verdict))
		{
			ServerExposure(Other, bExposed, bOccluded);
		}
		Reported.Add(Other, Verdict);
	}
}
