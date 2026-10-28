#include "Character/CharacterPoolLibrary.h"

#include "Character/CharacterPoolComponent.h"
#include "Character/CharacterPoolSubsystem.h"
#include "CharacterPoolReflection.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

bool UCharacterPoolLibrary::IsDedicatedPawn(APawn* Pawn)
{
	const auto* Life = IsValid(Pawn) ? Pawn->FindComponentByClass<UCharacterPoolComponent>() : nullptr;
	return Life && Life->IsManaged();
}

bool UCharacterPoolLibrary::ShouldApplyPossessionAppearance(APawn* Pawn)
{
	const auto* Life = IsValid(Pawn) ? Pawn->FindComponentByClass<UCharacterPoolComponent>() : nullptr;
	return !Life || !Life->IsManaged() || !Life->HasBeenPossessed();
}

bool UCharacterPoolLibrary::CanPlayMovementEffects(APawn* Pawn)
{
	return !UCharacterPoolComponent::IsInactive(Pawn);
}

bool UCharacterPoolLibrary::StartPooledRespawn(UActorComponent* Helper)
{
	using namespace CharacterPoolReflection;
	APawn* Pawn = Cast<APawn>(Object(Helper, TEXT("Now Pawn")));
	AController* Controller = Cast<AController>(Object(Helper, TEXT("Controller")));
	auto* Life = IsValid(Pawn) ? Pawn->FindComponentByClass<UCharacterPoolComponent>() : nullptr;
	if (!Life || !Life->IsManaged() || !Pawn->HasAuthority()) return false;
	Life->BeginDeath(Controller, Number(Helper, TEXT("Respawn Time")));
	return true;
}

void UCharacterPoolLibrary::ReserveWaitingPawn(APlayerController* Controller, float WaitingTime)
{
	if (IsValid(Controller) && Controller->HasAuthority())
		if (auto* Pool = Controller->GetWorld()->GetSubsystem<UCharacterPoolSubsystem>()) Pool->Reserve(Controller, WaitingTime);
}

APawn* UCharacterPoolLibrary::GetWaitingPawn(const UObject* WorldContextObject, const FString& UID)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	auto* Pool = World ? World->GetSubsystem<UCharacterPoolSubsystem>() : nullptr;
	return Pool ? Pool->Claim(UID) : nullptr;
}
