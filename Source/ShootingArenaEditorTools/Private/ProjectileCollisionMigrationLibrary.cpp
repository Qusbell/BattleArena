#include "ProjectileCollisionMigrationLibrary.h"

#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "K2Node_ComponentBoundEvent.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Weapon/ProjectileWorldCollisionComponent.h"
#include "Weapon/WeaponProjectileAimLibrary.h"
#include "UObject/UnrealType.h"

namespace
{
bool Supported(const UBlueprint* Blueprint)
{
	return IsValid(Blueprint) && IsValid(Blueprint->GeneratedClass) && IsValid(Blueprint->SimpleConstructionScript)
		&& (Blueprint->GetName() == TEXT("BP_PlasmaProjectile") || Blueprint->GetName() == TEXT("BP_RocketLauncherProjectile"));
}
}

bool UProjectileCollisionMigrationLibrary::AddWorldCollision(UBlueprint* Blueprint, float WorldRadius)
{
	if (!Supported(Blueprint) || !FMath::IsFinite(WorldRadius) || WorldRadius <= 0.f) return false;
	auto* Player = FindObject<USphereComponent>(Blueprint->GeneratedClass, TEXT("Collision Component_GEN_VARIABLE"));
	if (!IsValid(Player)) return false;
	USCS_Node* InheritedRoot = nullptr;
	for (UClass* Parent = Blueprint->GeneratedClass->GetSuperClass(); Parent && !InheritedRoot; Parent = Parent->GetSuperClass())
		if (const auto* ParentBP = Cast<UBlueprint>(Parent->ClassGeneratedBy); ParentBP && ParentBP->SimpleConstructionScript)
			InheritedRoot = ParentBP->SimpleConstructionScript->FindSCSNode(TEXT("Collision Component"));
	if (!InheritedRoot) return false;
	USimpleConstructionScript* SCS = Blueprint->SimpleConstructionScript;
	if (SCS->FindSCSNode(TEXT("WorldCollision"))) return false;
	Blueprint->Modify(); SCS->Modify(); Player->Modify();
	USCS_Node* Node = SCS->CreateNode(UProjectileWorldCollisionComponent::StaticClass(), TEXT("WorldCollision"));
	auto* World = CastChecked<UProjectileWorldCollisionComponent>(Node->ComponentTemplate);
	World->SetSphereRadius(WorldRadius);
	World->SetCollisionObjectType(Player->GetCollisionObjectType());
	World->SetCollisionResponseToChannels(Player->GetCollisionResponseToChannels());
	World->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	World->SetCollisionResponseToChannel(Player->GetCollisionObjectType(), ECR_Ignore);
	World->SetAbsolute(false, false, true);
	World->SetRelativeScale3D(FVector::OneVector);
	// Keep all existing mesh attachments, transforms, and root radius intact.
	Node->SetParent(InheritedRoot);
	SCS->AddNode(Node);
	Player->SetCollisionResponseToAllChannels(ECR_Ignore);
	// Do not let automatic root movement stop before the shared resolver can
	// reject a player reached through a nearby wall or choose an earlier wall.
	Player->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Player->SetGenerateOverlapEvents(false);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	return true;
}

bool UProjectileCollisionMigrationLibrary::ConnectResolvedImpact(UBlueprint* Blueprint)
{
	if (!Supported(Blueprint)) return false;
	const auto* ComponentProperty = FindFProperty<FObjectProperty>(Blueprint->GeneratedClass, TEXT("WorldCollision"));
	const auto* DelegateProperty = FindFProperty<FMulticastDelegateProperty>(UProjectileWorldCollisionComponent::StaticClass(), TEXT("OnResolvedImpact"));
	if (!ComponentProperty || !DelegateProperty) return false;
	for (UEdGraph* Graph : Blueprint->UbergraphPages)
	{
		UK2Node_ComponentBoundEvent* Original = nullptr;
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			auto* Event = Cast<UK2Node_ComponentBoundEvent>(Node);
			if (!Event) continue;
			if (Event->GetComponentPropertyName() == TEXT("WorldCollision")) return false;
			if (Event->GetComponentPropertyName() == TEXT("Collision Component") && Event->DelegatePropertyName == TEXT("OnComponentHit")) Original = Event;
		}
		if (!Original) continue;
		Blueprint->Modify(); Graph->Modify(); Original->Modify();
		auto* Event = NewObject<UK2Node_ComponentBoundEvent>(Graph);
		Graph->AddNode(Event, false, false);
		Event->CreateNewGuid();
		Event->InitializeComponentBoundEventParams(ComponentProperty, DelegateProperty);
		Event->PostPlacedNewNode(); Event->AllocateDefaultPins();
		Event->NodePosX = Original->NodePosX; Event->NodePosY = Original->NodePosY - 350;
		const auto* Schema = GetDefault<UEdGraphSchema_K2>();
		UEdGraphPin* OldHit = Original->FindPin(TEXT("Hit"));
		UEdGraphPin* NewHit = Event->FindPin(TEXT("Hit"));
		if (OldHit && NewHit && !OldHit->SubPins.IsEmpty()) Schema->SplitPin(NewHit, false);
		for (UEdGraphPin* Pin : Original->Pins)
			if (!Pin->LinkedTo.IsEmpty() && !Event->FindPin(Pin->PinName)) return false;
		for (UEdGraphPin* Pin : Original->Pins)
			if (UEdGraphPin* NewPin = Event->FindPin(Pin->PinName)) Schema->MovePinLinks(*Pin, *NewPin, true);
		Original->NodeComment = TEXT("Legacy single-collision entry retained; links now use WorldCollision.OnResolvedImpact.");
		Event->NodeComment = TEXT("Player root + small world sphere: earliest valid impact, existing damage/explosion body unchanged.");
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		return true;
	}
	return false;
}

FString UProjectileCollisionMigrationLibrary::InspectCollisionInstance(UBlueprint* Blueprint)
{
	TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
	Report->SetBoolField(TEXT("success"), false);
	if (Supported(Blueprint))
	{
		const auto Values = UWorld::InitializationValues().AllowAudioPlayback(false).RequiresHitProxies(false)
			.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
		if (UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values))
		{
			// Exercise normal movement-component registration without BeginPlay
			// or ticking a game; EditorPreview otherwise skips this registration.
			World->bForceUseMovementComponentInNonGameWorld = true;
			FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AActor* Actor = World->SpawnActor<AActor>(Blueprint->GeneratedClass, FTransform::Identity, Params);
			const bool bLiveActor = IsValid(Actor) && !Actor->IsActorBeingDestroyed();
			const auto* Player = bLiveActor ? Cast<USphereComponent>(Actor->GetRootComponent()) : nullptr;
			const auto* Environment = bLiveActor ? Actor->FindComponentByClass<UProjectileWorldCollisionComponent>() : nullptr;
			const auto* Movement = bLiveActor ? Actor->FindComponentByClass<UProjectileMovementComponent>() : nullptr;
			if (IsValid(Player) && IsValid(Environment) && IsValid(Movement)
				&& !Player->IsBeingDestroyed() && !Environment->IsBeingDestroyed() && !Movement->IsBeingDestroyed())
			{
				Report->SetNumberField(TEXT("player_radius_cm"), Player->GetScaledSphereRadius());
				Report->SetNumberField(TEXT("world_radius_cm"), Environment->GetScaledSphereRadius());
				Report->SetBoolField(TEXT("world_attached_to_existing_root"), Environment->GetAttachParent() == Player);
				Report->SetBoolField(TEXT("world_absolute_scale"), Environment->IsUsingAbsoluteScale());
				Report->SetBoolField(TEXT("impact_bound"), Environment->OnResolvedImpact.IsBound());
				Report->SetBoolField(TEXT("movement_uses_existing_root"), Movement->UpdatedComponent == Player);
				Report->SetBoolField(TEXT("has_begun_play"), Actor->HasActorBegunPlay());
				Report->SetBoolField(TEXT("success"), Environment->GetAttachParent() == Player && Environment->IsUsingAbsoluteScale()
					&& Environment->OnResolvedImpact.IsBound() && Movement->UpdatedComponent == Player && !Actor->HasActorBegunPlay());
			}
			World->DestroyWorld(false);
		}
	}
	FString Result; FJsonSerializer::Serialize(Report, TJsonWriterFactory<>::Create(&Result));
	return Result;
}

FString UProjectileCollisionMigrationLibrary::VerifyVisualRetirement(UBlueprint* Blueprint)
{
	TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
	Report->SetBoolField(TEXT("success"), false);
	if (Supported(Blueprint))
	{
		const auto* Id = FindFProperty<FIntProperty>(Blueprint->GeneratedClass, TEXT("projectileId"));
		const auto* Damage = FindFProperty<FBoolProperty>(Blueprint->GeneratedClass, TEXT("canDealDamage"));
		const auto Values = UWorld::InitializationValues().AllowAudioPlayback(false).RequiresHitProxies(false)
			.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
		UWorld* World = Id && Damage ? UWorld::CreateWorld(EWorldType::EditorPreview, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values) : nullptr;
		if (World)
		{
			if (GEngine) GEngine->CreateNewWorldContext(EWorldType::EditorPreview).SetCurrentWorld(World);
			AActor* Weapon = World->SpawnActor<AActor>();
			AActor* OtherWeapon = World->SpawnActor<AActor>();
			TArray<AActor*> Actors;
			for (int32 Index = 0; Index < 5; ++Index)
			{
				FActorSpawnParameters Params;
				Params.Owner = Index == 3 ? OtherWeapon : Weapon;
				Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				AActor* Actor = World->SpawnActor<AActor>(Blueprint->GeneratedClass, FTransform::Identity, Params);
				Actors.Add(Actor);
				if (IsValid(Actor))
				{
					Id->SetPropertyValue_InContainer(Actor, Index == 2 ? 42 : 41);
					Damage->SetPropertyValue_InContainer(Actor, Index == 4);
				}
			}
			bool bValid = IsValid(Weapon) && IsValid(OtherWeapon);
			for (AActor* Actor : Actors) bValid &= IsValid(Actor) && !Actor->HasActorBegunPlay();
			if (bValid)
			{
				Report->SetBoolField(TEXT("null_weapon_safe"), UWeaponProjectileAimLibrary::DestroyProjectileVisualsById(nullptr, 41) == 0);
				const int32 Count = UWeaponProjectileAimLibrary::DestroyProjectileVisualsById(Weapon, 41);
				const bool bMatched = Actors[0]->IsActorBeingDestroyed() && Actors[1]->IsActorBeingDestroyed();
				const bool bOthers = !Actors[2]->IsActorBeingDestroyed() && !Actors[3]->IsActorBeingDestroyed() && !Actors[4]->IsActorBeingDestroyed();
				const bool bRepeatSafe = UWeaponProjectileAimLibrary::DestroyProjectileVisualsById(Weapon, 41) == 0;
				Report->SetNumberField(TEXT("removed_visual_copies"), Count);
				Report->SetBoolField(TEXT("matching_copies_removed"), bMatched);
				Report->SetBoolField(TEXT("other_shot_other_weapon_authority_untouched"), bOthers);
				Report->SetBoolField(TEXT("repeat_safe"), bRepeatSafe);
				Report->SetBoolField(TEXT("success"), Count == 2 && bMatched && bOthers && bRepeatSafe);
			}
			if (GEngine) GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}
	}
	FString Result; FJsonSerializer::Serialize(Report, TJsonWriterFactory<>::Create(&Result));
	return Result;
}
