#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/NetSerialization.h"
#include "DebugAINetworkComponent.generated.h"

/** Each pair in Lines is an independent segment; Points also includes isolated route points. */
USTRUCT()
struct FDebugAIRoutePositions
{
	GENERATED_BODY()
	UPROPERTY() TArray<FVector_NetQuantize10> Points;
	UPROPERTY() TArray<FVector_NetQuantize10> Lines;
};

/** Only world-space positions cross the wire for geometry. Field identity supplies its meaning. */
USTRUCT()
struct FDebugAIPositionFrame
{
	GENERATED_BODY()
	UPROPERTY() TArray<FVector_NetQuantize10> STG;
	UPROPERTY() TArray<FVector_NetQuantize10> LTG;
	UPROPERTY() TArray<FVector_NetQuantize10> Enemy;
	UPROPERTY() TArray<FVector_NetQuantize10> EnemyLast;
	UPROPERTY() TArray<FVector_NetQuantize10> Aim;
	UPROPERTY() FDebugAIRoutePositions Route;
	UPROPERTY() FDebugAIRoutePositions Links;
	UPROPERTY() FDebugAIRoutePositions ActiveRoute;
	UPROPERTY() TArray<FVector_NetQuantize10> Movement;
	UPROPERTY() TArray<FVector_NetQuantize10> StuckMovement;
	UPROPERTY() TArray<FVector_NetQuantize10> AimBoundary;
	UPROPERTY() TArray<FVector_NetQuantize10> StateLabel;
	UPROPERTY() TArray<FVector_NetQuantize10> GoalLabels;

	bool IsEmpty() const;
};

/** Values which exist only on the server; visual styling is never sent. */
USTRUCT()
struct FDebugAIValueFrame
{
	GENERATED_BODY()
	UPROPERTY() FString State;
	UPROPERTY() TArray<float> GoalWeights;
};

UENUM(BlueprintType)
enum class EDebugAIRouteStyle : uint8
{
	Route,
	Link,
	Active
};

/** One owner-only snapshot per debug update; the client renders its cached snapshot every frame. */
UCLASS()
class SHOOTINGARENA_API UDebugAINetworkComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UDebugAINetworkComponent();

	UFUNCTION(BlueprintCallable, Category="Debug|AI Network", meta=(DefaultToSelf="DebugComponent"))
	static void BeginDebugAIFrame(UActorComponent* DebugComponent);

	UFUNCTION(BlueprintCallable, Category="Debug|AI Network", meta=(DefaultToSelf="DebugComponent"))
	static void SendDebugAIFrame(UActorComponent* DebugComponent);

	// These are local server calls, not RPCs. The label/style chooses a fixed field in the snapshot.
	UFUNCTION(BlueprintCallable, Category="Debug|AI Network", meta=(DefaultToSelf="DebugComponent"))
	static void QueueDebugAITarget(UActorComponent* DebugComponent, FVector Origin, FVector TargetLocation, const FString& Text);

	UFUNCTION(BlueprintCallable, Category="Debug|AI Network", meta=(DefaultToSelf="DebugComponent"))
	static void QueueDebugAIRoute(UActorComponent* DebugComponent, const TArray<FVector>& Points, EDebugAIRouteStyle Style);

	UFUNCTION(BlueprintCallable, Category="Debug|AI Network", meta=(DefaultToSelf="DebugComponent"))
	static void QueueDebugAIMovement(UActorComponent* DebugComponent, FVector LineStart, FVector LineEnd, bool bStuck);

	UFUNCTION(BlueprintCallable, Category="Debug|AI Network", meta=(DefaultToSelf="DebugComponent"))
	static void QueueDebugAIAimBoundary(UActorComponent* DebugComponent, FVector Center, FVector Extent, FRotator Rotation);

	UFUNCTION(BlueprintCallable, Category="Debug|AI Network", meta=(DefaultToSelf="DebugComponent"))
	static void QueueDebugAIState(UActorComponent* DebugComponent, FVector TextLocation, const FString& Text);

	UFUNCTION(BlueprintCallable, Category="Debug|AI Network", meta=(DefaultToSelf="DebugComponent"))
	static void QueueDebugAIGoalWeight(UActorComponent* DebugComponent, FVector TextLocation, double Weight);

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	static UDebugAINetworkComponent* FindRelay(UActorComponent* DebugComponent, bool bCreate = false);

	UFUNCTION(Client, Unreliable)
	void ClientReceiveDebugAIFrame(const FDebugAIPositionFrame& Positions, const FDebugAIValueFrame& Values);

	UFUNCTION(Client, Reliable)
	void ClientClearDebugAIFrame();

	UPROPERTY(Transient) FDebugAIPositionFrame PendingPositions;
	UPROPERTY(Transient) FDebugAIValueFrame PendingValues;
	UPROPERTY(Transient) FDebugAIPositionFrame DisplayPositions;
	UPROPERTY(Transient) FDebugAIValueFrame DisplayValues;
	bool bSentActiveFrame = false;
};
