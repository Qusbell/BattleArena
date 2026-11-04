#include "Debug/DebugAINetworkComponent.h"

#include "DrawDebugHelpers.h"
#include "GameFramework/PlayerController.h"

namespace
{
	using FPositions = TArray<FVector_NetQuantize10>;

	void AddPosition(FPositions& Positions, const FVector& Position)
	{
		Positions.Add(FVector_NetQuantize10(Position));
	}

	void AddLine(FPositions& Positions, const FVector& Start, const FVector& End)
	{
		AddPosition(Positions, Start);
		AddPosition(Positions, End);
	}

	void DrawLines(UWorld* World, const FPositions& Positions, const FColor& Color)
	{
		for (int32 Index = 0; Index + 1 < Positions.Num(); Index += 2)
		{
			DrawDebugLine(World, Positions[Index], Positions[Index + 1], Color, false, 0.f, 0, 3.f);
		}
	}

	void DrawTarget(UWorld* World, const FPositions& Positions, const FColor& Color, const TCHAR* Label, float Radius = 100.f)
	{
		DrawLines(World, Positions, Color);
		for (int32 Index = 1; Index < Positions.Num(); Index += 2)
		{
			DrawDebugSphere(World, Positions[Index], Radius, 12, Color, false, 0.f, 0, 3.f);
			DrawDebugString(World, Positions[Index], Label, nullptr, Color, 0.f);
		}
	}

	void DrawRoute(UWorld* World, const FDebugAIRoutePositions& Route, const FColor& Color)
	{
		DrawLines(World, Route.Lines, Color);
		for (const auto& Point : Route.Points)
		{
			DrawDebugSphere(World, Point, 37.5f, 12, Color, false, 0.f);
		}
	}

	void DrawMovement(UWorld* World, const FPositions& Positions, const FColor& Color)
	{
		for (int32 Index = 0; Index + 1 < Positions.Num(); Index += 2)
		{
			DrawDebugDirectionalArrow(World, Positions[Index], Positions[Index + 1], 200.f, Color, false, 0.f, 0, 3.f);
		}
	}
}

bool FDebugAIPositionFrame::IsEmpty() const
{
	return STG.IsEmpty() && LTG.IsEmpty() && Enemy.IsEmpty() && EnemyLast.IsEmpty() && Aim.IsEmpty()
		&& Route.Points.IsEmpty() && Links.Points.IsEmpty() && ActiveRoute.Points.IsEmpty()
		&& Movement.IsEmpty() && StuckMovement.IsEmpty() && AimBoundary.IsEmpty()
		&& StateLabel.IsEmpty() && GoalLabels.IsEmpty();
}

UDebugAINetworkComponent::UDebugAINetworkComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

UDebugAINetworkComponent* UDebugAINetworkComponent::FindRelay(UActorComponent* DebugComponent, bool bCreate)
{
#if UE_BUILD_SHIPPING
	return nullptr;
#else
	auto* Controller = IsValid(DebugComponent) ? Cast<APlayerController>(DebugComponent->GetOwner()) : nullptr;
	if (!IsValid(Controller) || !Controller->HasAuthority()) return nullptr;
	auto* Relay = Controller->FindComponentByClass<UDebugAINetworkComponent>();
	if (!Relay && bCreate)
	{
		Relay = NewObject<UDebugAINetworkComponent>(Controller);
		Relay->RegisterComponent();
	}
	return Relay;
#endif
}

void UDebugAINetworkComponent::BeginDebugAIFrame(UActorComponent* DebugComponent)
{
	if (auto* Relay = FindRelay(DebugComponent, true))
	{
		Relay->PendingPositions = {};
		Relay->PendingValues = {};
	}
}

void UDebugAINetworkComponent::SendDebugAIFrame(UActorComponent* DebugComponent)
{
	if (auto* Relay = FindRelay(DebugComponent))
	{
		if (!Relay->PendingPositions.IsEmpty())
		{
			Relay->ClientReceiveDebugAIFrame(Relay->PendingPositions, Relay->PendingValues);
			Relay->bSentActiveFrame = true;
		}
		else if (Relay->bSentActiveFrame)
		{
			// A lost final unreliable snapshot must not leave stale geometry on screen.
			Relay->ClientClearDebugAIFrame();
			Relay->bSentActiveFrame = false;
		}
	}
}

void UDebugAINetworkComponent::QueueDebugAITarget(UActorComponent* DebugComponent, FVector Origin, FVector TargetLocation, const FString& Text)
{
	if (auto* Relay = FindRelay(DebugComponent))
	{
		FPositions* Positions = nullptr;
		if (Text == TEXT("STG Location")) Positions = &Relay->PendingPositions.STG;
		else if (Text == TEXT("LTG Location")) Positions = &Relay->PendingPositions.LTG;
		else if (Text == TEXT("Enemy")) Positions = &Relay->PendingPositions.Enemy;
		else if (Text == TEXT("EnemyLastLocation")) Positions = &Relay->PendingPositions.EnemyLast;
		else if (Text == TEXT("Current Aim")) Positions = &Relay->PendingPositions.Aim;
		if (Positions) AddLine(*Positions, Origin, TargetLocation);
	}
}

void UDebugAINetworkComponent::QueueDebugAIRoute(UActorComponent* DebugComponent, const TArray<FVector>& Points, EDebugAIRouteStyle Style)
{
	if (auto* Relay = FindRelay(DebugComponent))
	{
		auto& Route = Style == EDebugAIRouteStyle::Active ? Relay->PendingPositions.ActiveRoute
			: Style == EDebugAIRouteStyle::Link ? Relay->PendingPositions.Links : Relay->PendingPositions.Route;
		for (int32 Index = 0; Index < Points.Num(); ++Index)
		{
			AddPosition(Route.Points, Points[Index]);
			if (Index > 0) AddLine(Route.Lines, Points[Index - 1], Points[Index]);
		}
	}
}

void UDebugAINetworkComponent::QueueDebugAIMovement(UActorComponent* DebugComponent, FVector LineStart, FVector LineEnd, bool bStuck)
{
	if (auto* Relay = FindRelay(DebugComponent))
	{
		AddLine(bStuck ? Relay->PendingPositions.StuckMovement : Relay->PendingPositions.Movement, LineStart, LineEnd);
	}
}

void UDebugAINetworkComponent::QueueDebugAIAimBoundary(UActorComponent* DebugComponent, FVector Center, FVector Extent, FRotator Rotation)
{
	if (auto* Relay = FindRelay(DebugComponent))
	{
		FVector Corners[8];
		for (int32 Index = 0; Index < 8; ++Index)
		{
			const FVector Local((Index & 1) ? Extent.X : -Extent.X, (Index & 2) ? Extent.Y : -Extent.Y, (Index & 4) ? Extent.Z : -Extent.Z);
			Corners[Index] = Center + Rotation.RotateVector(Local);
		}
		for (const FVector& Corner : Corners) AddPosition(Relay->PendingPositions.AimBoundary, Corner);
	}
}

void UDebugAINetworkComponent::QueueDebugAIState(UActorComponent* DebugComponent, FVector TextLocation, const FString& Text)
{
	if (auto* Relay = FindRelay(DebugComponent))
	{
		Relay->PendingPositions.StateLabel.Reset();
		AddPosition(Relay->PendingPositions.StateLabel, TextLocation);
		Relay->PendingValues.State = Text;
	}
}

void UDebugAINetworkComponent::QueueDebugAIGoalWeight(UActorComponent* DebugComponent, FVector TextLocation, double Weight)
{
	if (auto* Relay = FindRelay(DebugComponent))
	{
		AddPosition(Relay->PendingPositions.GoalLabels, TextLocation);
		Relay->PendingValues.GoalWeights.Add(static_cast<float>(Weight));
	}
}

void UDebugAINetworkComponent::ClientReceiveDebugAIFrame_Implementation(const FDebugAIPositionFrame& Positions, const FDebugAIValueFrame& Values)
{
	DisplayPositions = Positions;
	DisplayValues = Values;
	SetComponentTickEnabled(true);
}

void UDebugAINetworkComponent::ClientClearDebugAIFrame_Implementation()
{
	DisplayPositions = {};
	DisplayValues = {};
	SetComponentTickEnabled(false);
}

void UDebugAINetworkComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
#if !UE_BUILD_SHIPPING
	const auto* Controller = Cast<APlayerController>(GetOwner());
	if (!IsValid(Controller) || !Controller->IsLocalController()) return;
	UWorld* World = GetWorld();
	// All presentation constants live here, on the receiving client.
	DrawTarget(World, DisplayPositions.STG, FLinearColor(0.915734f, 1.f, 0.f).ToFColor(true), TEXT("STG Location"));
	DrawTarget(World, DisplayPositions.LTG, FLinearColor(1.f, 0.f, 0.772613f).ToFColor(true), TEXT("LTG Location"));
	DrawTarget(World, DisplayPositions.Enemy, FColor::Black, TEXT("Enemy"));
	DrawTarget(World, DisplayPositions.EnemyLast, FLinearColor(1.f, 0.549098f, 0.f).ToFColor(true), TEXT("EnemyLastLocation"));
	DrawTarget(World, DisplayPositions.Aim, FLinearColor(1.f, 0.687355f, 0.f).ToFColor(true), TEXT("Current Aim"), 5.f);
	DrawRoute(World, DisplayPositions.Route, FLinearColor(0.052278f, 0.f, 1.f).ToFColor(true));
	DrawRoute(World, DisplayPositions.Links, FLinearColor(1.f, 0.017960f, 0.f).ToFColor(true));
	DrawRoute(World, DisplayPositions.ActiveRoute, FColor::White);
	DrawMovement(World, DisplayPositions.Movement, FLinearColor(1.f, 0.962494f, 0.f).ToFColor(true));
	DrawMovement(World, DisplayPositions.StuckMovement, FLinearColor(1.f, 0.f, 0.012693f).ToFColor(true));
	if (DisplayPositions.AimBoundary.Num() == 8)
	{
		for (int32 Index = 0; Index < 8; ++Index)
		{
			for (int32 Axis = 1; Axis <= 4; Axis <<= 1)
			{
				if ((Index & Axis) == 0) DrawDebugLine(World, DisplayPositions.AimBoundary[Index], DisplayPositions.AimBoundary[Index | Axis], FColor::Black, false, 0.f);
			}
		}
	}
	if (!DisplayPositions.StateLabel.IsEmpty())
	{
		DrawDebugString(World, DisplayPositions.StateLabel[0], DisplayValues.State, nullptr, FColor::White, 0.f);
	}
	FNumberFormattingOptions Format;
	Format.SetMaximumFractionalDigits(2);
	const int32 Count = FMath::Min(DisplayPositions.GoalLabels.Num(), DisplayValues.GoalWeights.Num());
	for (int32 Index = 0; Index < Count; ++Index)
	{
		DrawDebugString(World, DisplayPositions.GoalLabels[Index], FText::AsNumber(DisplayValues.GoalWeights[Index], &Format).ToString(), nullptr, FColor::White, 0.f);
	}
#endif
}
