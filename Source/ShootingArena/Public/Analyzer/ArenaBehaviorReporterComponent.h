#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ArenaBehaviorReporterComponent.generated.h"

class APawn;

/**
 * Carries what only the player's own machine knows to the server's match log: fire/jump input and which other
 * pawns are inside the player's screen (and whether a wall hides them). Add one to the player pawn Blueprint
 * (BP_PlayerShooter) and call ReportAttackInput / ReportJumpInput from the input actions.
 *
 * Everything is a no-op unless the server has a behaviour observation interval configured
 * (DA_ArenaLogging "Behavior Sample Interval Seconds" or -ArenaBehaviorSampleInterval=<s>); the server then tells
 * the owning client how often to check exposure. The server derives distances and heights itself; the client
 * only reports the screen/occlusion verdicts.
 */
UCLASS(ClassGroup = (Arena), meta = (BlueprintSpawnableComponent))
class SHOOTINGARENA_API UArenaBehaviorReporterComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UArenaBehaviorReporterComponent();

    /** Fire input changed: true on press, false on release. Safe to call every frame while held. */
    UFUNCTION(BlueprintCallable, Category = "Arena Match Log|Behavior")
    void ReportAttackInput(bool bPressed);

    /** Jump input pressed. Call before the actual Jump node so the "could jump" state is the one at input time. */
    UFUNCTION(BlueprintCallable, Category = "Arena Match Log|Behavior")
    void ReportJumpInput();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    UFUNCTION(Server, Reliable)
    void ServerAttackInput(bool bPressed);

    UFUNCTION(Server, Reliable)
    void ServerJumpInput();

    UFUNCTION(Server, Reliable)
    void ServerExposure(APawn* Target, bool bExposed, bool bOccluded);

    UFUNCTION(Client, Reliable)
    void ClientSetInterval(float Seconds);

    void PollExposure();

    float Interval = 0.0f;       // owning client: seconds between exposure checks, 0 = off
    float SentInterval = -1.0f;  // server: last value pushed to the client
    float Accumulated = 0.0f;
    bool bLastAttack = false;
    TMap<TWeakObjectPtr<APawn>, uint8> Reported; // pawn -> last verdict (bit0 exposed, bit1 occluded)
};
