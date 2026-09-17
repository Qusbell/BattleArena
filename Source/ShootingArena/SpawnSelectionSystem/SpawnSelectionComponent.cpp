#include "SpawnSelectionComponent.h"

#include "Components/SceneComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogSpawnSelection, Log, All);

USpawnSelectionComponent::USpawnSelectionComponent()
{
	// 스폰 시스템은 이벤트 기반으로만 동작하므로 Tick이 필요하지 않습니다.
	PrimaryComponentTick.bCanEverTick = false;
}

void USpawnSelectionComponent::InitializeSpawnPoints(
	const TArray<USceneComponent*>& InSpawnPoints)
{
	// 기존 BP 초기화가 자동 탐색 직후 동일한 목록을 다시 넘기는 경우,
	// BP에 타입 배열이 없다는 이유로 PlayerOnly/BotOnly 정보를 Both로 덮어쓰지 않습니다.
	if (bSpawnPointTypesConfigured && IsSameRegisteredSpawnPoints(InSpawnPoints))
	{
		UE_LOG(LogSpawnSelection, Log,
			TEXT("InitializeSpawnPoints skipped: retaining configured SpawnPoint types for %d points."),
			SpawnPoints.Num());
		return;
	}

	TArray<ESpawnPointType> DefaultSpawnPointTypes;
	DefaultSpawnPointTypes.Init(ESpawnPointType::Both, InSpawnPoints.Num());
	InitializeSpawnPointsWithTypes(InSpawnPoints, DefaultSpawnPointTypes);
}

void USpawnSelectionComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* OwnerActor = GetOwner();
	if (bAutoDiscoverLevelSpawnPoints && IsValid(OwnerActor) && OwnerActor->HasAuthority())
	{
		DiscoverLevelSpawnPoints();
	}
}

void USpawnSelectionComponent::DiscoverLevelSpawnPoints()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	TArray<USceneComponent*> DiscoveredSpawnPoints;
	TArray<ESpawnPointType> DiscoveredSpawnPointTypes;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* SpawnPointActor = *It;
		if (!IsValid(SpawnPointActor)
			|| !SpawnPointActor->GetClass()->GetName().StartsWith(TEXT("BP_PlayerSpawnPoint")))
		{
			continue;
		}

		TInlineComponentArray<USceneComponent*> SceneComponents(SpawnPointActor);
		USceneComponent* TransformComponent = nullptr;
		for (USceneComponent* SceneComponent : SceneComponents)
		{
			if (IsValid(SceneComponent)
				&& (SceneComponent->GetName().Contains(TEXT("SpawnPoint"), ESearchCase::IgnoreCase)
					|| SceneComponent->ComponentTags.Contains(TEXT("SpawnPoint"))))
			{
				TransformComponent = SceneComponent;
				break;
			}
		}

		// 명시적으로 이름/태그가 붙은 컴포넌트가 없으면 Actor Root를 Transform으로 사용합니다.
		if (!IsValid(TransformComponent))
		{
			TransformComponent = SpawnPointActor->GetRootComponent();
		}

		if (!IsValid(TransformComponent))
		{
			UE_LOG(LogSpawnSelection, Warning,
				TEXT("Auto discovery skipped %s: no SceneComponent was found."),
				*GetNameSafe(SpawnPointActor));
			continue;
		}

		DiscoveredSpawnPoints.Add(TransformComponent);
		const ESpawnPointType SpawnPointType = ReadSpawnPointType(SpawnPointActor);
		DiscoveredSpawnPointTypes.Add(SpawnPointType);
		UE_LOG(LogSpawnSelection, Log, TEXT("Auto discovery: %s -> %s"),
			*GetNameSafe(SpawnPointActor),
			*StaticEnum<ESpawnPointType>()->GetNameStringByValue(static_cast<int64>(SpawnPointType)));
	}

	InitializeSpawnPointsWithTypes(DiscoveredSpawnPoints, DiscoveredSpawnPointTypes);
}

ESpawnPointType USpawnSelectionComponent::ReadSpawnPointType(const AActor* SpawnPointActor) const
{
	if (!IsValid(SpawnPointActor))
	{
		return ESpawnPointType::Both;
	}

	// Blueprint 변수는 프로젝트마다 Spawn_Type 또는 spawnType으로 명명되어 있어 둘 다 지원합니다.
	const FProperty* Property = FindFProperty<FProperty>(SpawnPointActor->GetClass(), TEXT("Spawn_Type"));
	if (Property == nullptr)
	{
		Property = FindFProperty<FProperty>(SpawnPointActor->GetClass(), TEXT("spawnType"));
	}
	if (Property == nullptr)
	{
		Property = FindFProperty<FProperty>(SpawnPointActor->GetClass(), TEXT("SpawnType"));
	}

	if (Property == nullptr)
	{
		UE_LOG(LogSpawnSelection, Warning,
			TEXT("Auto discovery: %s has no Spawn_Type/spawnType property; using Both."),
			*GetNameSafe(SpawnPointActor));
		return ESpawnPointType::Both;
	}

	const void* ValueAddress = Property ? Property->ContainerPtrToValuePtr<void>(SpawnPointActor) : nullptr;
	int64 RawValue = static_cast<int64>(ESpawnPointType::Both);

	if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
	{
		RawValue = EnumProperty->GetUnderlyingProperty()->GetSignedIntPropertyValue(ValueAddress);
	}
	else if (const FByteProperty* ByteProperty = CastField<FByteProperty>(Property))
	{
		RawValue = ByteProperty->GetPropertyValue(ValueAddress);
	}

	return RawValue >= static_cast<int64>(ESpawnPointType::Both)
		&& RawValue <= static_cast<int64>(ESpawnPointType::BotOnly)
		? static_cast<ESpawnPointType>(RawValue)
		: ESpawnPointType::Both;
}

bool USpawnSelectionComponent::IsSameRegisteredSpawnPoints(
	const TArray<USceneComponent*>& InSpawnPoints) const
{
	TArray<USceneComponent*> ValidIncomingSpawnPoints;
	ValidIncomingSpawnPoints.Reserve(InSpawnPoints.Num());

	for (USceneComponent* SpawnPoint : InSpawnPoints)
	{
		if (IsValid(SpawnPoint))
		{
			ValidIncomingSpawnPoints.Add(SpawnPoint);
		}
	}

	if (ValidIncomingSpawnPoints.Num() != SpawnPoints.Num())
	{
		return false;
	}

	for (int32 Index = 0; Index < SpawnPoints.Num(); ++Index)
	{
		if (SpawnPoints[Index] != ValidIncomingSpawnPoints[Index])
		{
			return false;
		}
	}

	return true;
}

void USpawnSelectionComponent::InitializeSpawnPointsWithTypes(
	const TArray<USceneComponent*>& InSpawnPoints,
	const TArray<ESpawnPointType>& InSpawnPointTypes)
{
	SpawnPoints.Reset();
	SpawnPointTypes.Reset();
	bSpawnPointTypesConfigured = true;

	for (int32 Index = 0; Index < InSpawnPoints.Num(); ++Index)
	{
		USceneComponent* SpawnPoint = InSpawnPoints[Index];
		if (IsValid(SpawnPoint))
		{
			SpawnPoints.Add(SpawnPoint);
			SpawnPointTypes.Add(InSpawnPointTypes.IsValidIndex(Index)
				? InSpawnPointTypes[Index]
				: ESpawnPointType::Both);
		}
	}

	// SpawnPoint 목록이 바뀌면 기존 최초 스폰 배정 상태는 사용할 수 없습니다.
	InitialSpawnCounts.Reset();
	InitialTotalCharacterCount = 0;
	InitialMaxPerPoint = 0;
	InitialAssignmentsRemaining = 0;
	bInitialSpawnPrepared = false;

	UE_LOG(LogSpawnSelection, Log,
		TEXT("InitializeSpawnPoints: %d valid spawn points registered."),
		SpawnPoints.Num());
}

bool USpawnSelectionComponent::SetSpawnPointType(
	USceneComponent* SpawnPoint,
	const ESpawnPointType SpawnPointType)
{
	const int32 SpawnPointIndex = SpawnPoints.IndexOfByKey(SpawnPoint);
	if (!SpawnPointTypes.IsValidIndex(SpawnPointIndex))
	{
		return false;
	}

	SpawnPointTypes[SpawnPointIndex] = SpawnPointType;
	return true;
}

ESpawnPointType USpawnSelectionComponent::GetSpawnPointType(USceneComponent* SpawnPoint) const
{
	const int32 SpawnPointIndex = SpawnPoints.IndexOfByKey(SpawnPoint);
	return SpawnPointTypes.IsValidIndex(SpawnPointIndex)
		? SpawnPointTypes[SpawnPointIndex]
		: ESpawnPointType::Both;
}

int32 USpawnSelectionComponent::GetSpawnPointCount() const
{
	int32 ValidCount = 0;

	for (const TObjectPtr<USceneComponent>& SpawnPoint : SpawnPoints)
	{
		if (IsValid(SpawnPoint))
		{
			++ValidCount;
		}
	}

	return ValidCount;
}

bool USpawnSelectionComponent::PrepareInitialSpawn(
	const int32 TotalCharacterCount)
{
	bInitialSpawnPrepared = false;
	InitialSpawnCounts.Reset();
	InitialTotalCharacterCount = 0;
	InitialMaxPerPoint = 0;
	InitialAssignmentsRemaining = 0;

	if (TotalCharacterCount <= 0)
	{
		UE_LOG(LogSpawnSelection, Warning,
			TEXT("PrepareInitialSpawn failed: TotalCharacterCount must be greater than 0."));
		return false;
	}

	if (GetSpawnPointCount() <= 0)
	{
		UE_LOG(LogSpawnSelection, Warning,
			TEXT("PrepareInitialSpawn failed: no valid SpawnPoint is registered."));
		return false;
	}

	InitialSpawnCounts.Init(0, SpawnPoints.Num());
	InitialTotalCharacterCount = TotalCharacterCount;
	InitialAssignmentsRemaining = TotalCharacterCount;

	if (!RecalculateInitialMaxPerPoint())
	{
		InitialSpawnCounts.Reset();
		InitialTotalCharacterCount = 0;
		InitialAssignmentsRemaining = 0;
		return false;
	}

	bInitialSpawnPrepared = true;

	UE_LOG(LogSpawnSelection, Log,
		TEXT("PrepareInitialSpawn: Total=%d, SpawnPoints=%d, MaxPerPoint=%d"),
		InitialTotalCharacterCount,
		GetSpawnPointCount(),
		InitialMaxPerPoint);

	return true;
}

bool USpawnSelectionComponent::ExtendInitialSpawn(
	const int32 AdditionalCharacterCount)
{
	if (AdditionalCharacterCount <= 0)
	{
		return false;
	}

	// BeginPlay 이후 첫 신규 참가자가 들어오는 경우에도 안전하게 시작할 수 있게 합니다.
	if (!bInitialSpawnPrepared)
	{
		return PrepareInitialSpawn(AdditionalCharacterCount);
	}

	InitialTotalCharacterCount += AdditionalCharacterCount;
	InitialAssignmentsRemaining += AdditionalCharacterCount;

	if (!RecalculateInitialMaxPerPoint())
	{
		InitialTotalCharacterCount -= AdditionalCharacterCount;
		InitialAssignmentsRemaining -= AdditionalCharacterCount;
		return false;
	}

	UE_LOG(LogSpawnSelection, Log,
		TEXT("ExtendInitialSpawn: Added=%d, NewTotal=%d, Remaining=%d, MaxPerPoint=%d"),
		AdditionalCharacterCount,
		InitialTotalCharacterCount,
		InitialAssignmentsRemaining,
		InitialMaxPerPoint);

	return true;
}

bool USpawnSelectionComponent::RecalculateInitialMaxPerPoint()
{
	const int32 ValidSpawnPointCount = GetSpawnPointCount();

	if (InitialTotalCharacterCount <= 0 || ValidSpawnPointCount <= 0)
	{
		InitialMaxPerPoint = 0;
		return false;
	}

	/*
	 * 260824 PPT 공식:
	 * 최대 배치 수 = Ceil(전체 캐릭터 수 / 전체 SpawnPoint 수)
	 */
	InitialMaxPerPoint = FMath::CeilToInt(
		static_cast<double>(InitialTotalCharacterCount)
		/ static_cast<double>(ValidSpawnPointCount));

	return InitialMaxPerPoint > 0;
}

bool USpawnSelectionComponent::CanControllerUseSpawnPoint(
	const int32 SpawnPointIndex,
	const AController* Controller) const
{
	if (!SpawnPointTypes.IsValidIndex(SpawnPointIndex))
	{
		return false;
	}

	// 기존 위치 선택 노드는 Controller를 넘기지 않으므로 하위 호환을 위해 필터링하지 않습니다.
	if (!IsValid(Controller))
	{
		return true;
	}

	const ESpawnPointType SpawnPointType = SpawnPointTypes[SpawnPointIndex];
	if (SpawnPointType == ESpawnPointType::Both)
	{
		return true;
	}

	const bool bIsPlayer = Controller->IsPlayerController();
	return bIsPlayer
		? SpawnPointType == ESpawnPointType::PlayerOnly
		: SpawnPointType == ESpawnPointType::BotOnly;
}

bool USpawnSelectionComponent::SelectInitialSpawnTransform(
	FTransform& OutSpawnTransform,
	int32& OutSpawnPointIndex)
{
	return SelectInitialSpawnTransformForController(nullptr, OutSpawnTransform, OutSpawnPointIndex);
}

bool USpawnSelectionComponent::SelectInitialSpawnTransformForController(
	AController* Controller,
	FTransform& OutSpawnTransform,
	int32& OutSpawnPointIndex)
{
	OutSpawnTransform = FTransform::Identity;
	OutSpawnPointIndex = INDEX_NONE;

	if (!bInitialSpawnPrepared || InitialAssignmentsRemaining <= 0)
	{
		return false;
	}

	if (SpawnPoints.IsEmpty() || InitialSpawnCounts.Num() != SpawnPoints.Num())
	{
		return false;
	}

	// 전용 포인트가 있으면 Both보다 먼저 사용합니다. 반대 전용 타입은 CanControllerUseSpawnPoint에서 제외됩니다.
	const bool bHasControllerType = IsValid(Controller);
	const ESpawnPointType PreferredSpawnPointType = bHasControllerType && Controller->IsPlayerController()
		? ESpawnPointType::PlayerOnly
		: ESpawnPointType::BotOnly;

	TArray<int32> PreferredCandidates;
	TArray<int32> BothCandidates;
	TArray<int32> PreferredAllowedCandidates;
	TArray<int32> BothAllowedCandidates;
	PreferredCandidates.Reserve(SpawnPoints.Num());
	BothCandidates.Reserve(SpawnPoints.Num());
	PreferredAllowedCandidates.Reserve(SpawnPoints.Num());
	BothAllowedCandidates.Reserve(SpawnPoints.Num());

	for (int32 Index = 0; Index < SpawnPoints.Num(); ++Index)
	{
		if (!IsValid(SpawnPoints[Index]) || !CanControllerUseSpawnPoint(Index, Controller))
		{
			continue;
		}

		const bool bIsPreferred = bHasControllerType && SpawnPointTypes[Index] == PreferredSpawnPointType;
		if (InitialSpawnCounts[Index] < InitialMaxPerPoint)
		{
			(bIsPreferred ? PreferredCandidates : BothCandidates).Add(Index);
		}
		else
		{
			(bIsPreferred ? PreferredAllowedCandidates : BothAllowedCandidates).Add(Index);
		}
	}

	TArray<int32>* SelectedCandidateList = nullptr;
	if (bHasControllerType)
	{
		// 생성 제한을 만족하는 전용 → Both 순서. 전용만 존재하는 경우에는 제한을 넘겨도 전용을 유지합니다.
		if (!PreferredCandidates.IsEmpty())
		{
			SelectedCandidateList = &PreferredCandidates;
		}
		else if (!BothCandidates.IsEmpty())
		{
			SelectedCandidateList = &BothCandidates;
		}
		else if (!PreferredAllowedCandidates.IsEmpty())
		{
			SelectedCandidateList = &PreferredAllowedCandidates;
		}
		else if (!BothAllowedCandidates.IsEmpty())
		{
			SelectedCandidateList = &BothAllowedCandidates;
		}
	}
	else
	{
		// Controller를 받지 않는 기존 Blueprint 호출은 종전과 같은 전체 후보 동작을 유지합니다.
		if (!BothCandidates.IsEmpty())
		{
			SelectedCandidateList = &BothCandidates;
		}
	}

	if (SelectedCandidateList == nullptr || SelectedCandidateList->IsEmpty())
	{
		UE_LOG(LogSpawnSelection, Warning,
			TEXT("SelectInitialSpawnTransform failed: no candidate remains. Remaining=%d"),
			InitialAssignmentsRemaining);
		return false;
	}

	// 등록 순서가 결과를 결정하지 않도록 후보 전체에서 Random 선택합니다.
	const int32 RandomCandidateIndex = FMath::RandRange(0, SelectedCandidateList->Num() - 1);
	const int32 SelectedSpawnIndex = (*SelectedCandidateList)[RandomCandidateIndex];

	USceneComponent* SelectedSpawnPoint = SpawnPoints[SelectedSpawnIndex];
	if (!IsValid(SelectedSpawnPoint))
	{
		return false;
	}

	// 같은 프레임의 다음 Player/AI도 갱신된 배정 상태를 보도록 선택 즉시 반영합니다.
	InitialSpawnCounts[SelectedSpawnIndex]++;
	InitialAssignmentsRemaining--;

	OutSpawnTransform = SelectedSpawnPoint->GetComponentTransform();
	OutSpawnPointIndex = SelectedSpawnIndex;

	return true;
}

void USpawnSelectionComponent::ReleaseInitialSpawnSelection(
	const int32 SpawnPointIndex)
{
	if (!bInitialSpawnPrepared || !InitialSpawnCounts.IsValidIndex(SpawnPointIndex))
	{
		return;
	}

	if (InitialSpawnCounts[SpawnPointIndex] > 0)
	{
		InitialSpawnCounts[SpawnPointIndex]--;
		InitialAssignmentsRemaining++;
	}
}

bool USpawnSelectionComponent::IsInitialSpawnPrepared() const
{
	return bInitialSpawnPrepared;
}

int32 USpawnSelectionComponent::GetInitialAssignmentsRemaining() const
{
	return InitialAssignmentsRemaining;
}

int32 USpawnSelectionComponent::GetInitialTotalCharacterCount() const
{
	return InitialTotalCharacterCount;
}

bool USpawnSelectionComponent::SelectRespawnTransform(
	const FVector& DeathLocation,
	FTransform& OutSpawnTransform,
	int32& OutSpawnPointIndex)
{
	return SelectRespawnTransformForController(nullptr, DeathLocation, OutSpawnTransform, OutSpawnPointIndex);
}

bool USpawnSelectionComponent::SelectRespawnTransformForController(
	AController* Controller,
	const FVector& DeathLocation,
	FTransform& OutSpawnTransform,
	int32& OutSpawnPointIndex)
{
	OutSpawnTransform = FTransform::Identity;
	OutSpawnPointIndex = INDEX_NONE;

	struct FRespawnDistanceEntry
	{
		int32 SpawnPointIndex = INDEX_NONE;
		double DistanceSquared = 0.0;
		double RandomTieBreaker = 0.0;
	};

	const bool bHasControllerType = IsValid(Controller);
	const ESpawnPointType PreferredSpawnPointType = bHasControllerType && Controller->IsPlayerController()
		? ESpawnPointType::PlayerOnly
		: ESpawnPointType::BotOnly;

	TArray<FRespawnDistanceEntry> PreferredEntries;
	TArray<FRespawnDistanceEntry> BothEntries;
	TArray<FRespawnDistanceEntry> LegacyEntries;
	PreferredEntries.Reserve(SpawnPoints.Num());
	BothEntries.Reserve(SpawnPoints.Num());
	LegacyEntries.Reserve(SpawnPoints.Num());

	for (int32 Index = 0; Index < SpawnPoints.Num(); ++Index)
	{
		if (!IsValid(SpawnPoints[Index]) || !CanControllerUseSpawnPoint(Index, Controller))
		{
			continue;
		}

		TArray<FRespawnDistanceEntry>& TargetEntries = bHasControllerType
			? (SpawnPointTypes[Index] == PreferredSpawnPointType ? PreferredEntries : BothEntries)
			: LegacyEntries;
		FRespawnDistanceEntry& Entry = TargetEntries.AddDefaulted_GetRef();
		Entry.SpawnPointIndex = Index;
		Entry.DistanceSquared = FVector::DistSquared(
			DeathLocation,
			SpawnPoints[Index]->GetComponentLocation());

		// 거리가 완전히 같은 포인트에서 등록 순서가 후보 경계를 결정하지 않게 합니다.
		Entry.RandomTieBreaker = FMath::FRand();
	}

	TArray<FRespawnDistanceEntry>* CandidateEntries = nullptr;
	if (bHasControllerType)
	{
		// 전용 타입 전체를 먼저 확인하고, 전용 포인트가 하나라도 있으면 Both는 후보에서 제외합니다.
		CandidateEntries = !PreferredEntries.IsEmpty() ? &PreferredEntries : &BothEntries;
	}
	else
	{
		CandidateEntries = &LegacyEntries;
	}

	if (CandidateEntries->IsEmpty())
	{
		return false;
	}

	CandidateEntries->Sort(
		[](const FRespawnDistanceEntry& A, const FRespawnDistanceEntry& B)
		{
			if (A.DistanceSquared == B.DistanceSquared)
			{
				return A.RandomTieBreaker > B.RandomTieBreaker;
			}

			return A.DistanceSquared > B.DistanceSquared;
		});

	/*
	 * 260824 PPT 공식:
	 * 리스폰 후보 수 = Max(1, Floor(선택된 타입 SpawnPoint 수 / 2))
	 * int32 / 2가 내림 처리를 수행합니다.
	 */
	const int32 CandidateCount = FMath::Max(1, CandidateEntries->Num() / 2);
	const int32 SelectedSpawnIndex = (*CandidateEntries)[FMath::RandRange(0, CandidateCount - 1)].SpawnPointIndex;

	if (!SpawnPoints.IsValidIndex(SelectedSpawnIndex)
		|| !IsValid(SpawnPoints[SelectedSpawnIndex]))
	{
		return false;
	}

	OutSpawnTransform = SpawnPoints[SelectedSpawnIndex]->GetComponentTransform();
	OutSpawnPointIndex = SelectedSpawnIndex;

	return true;
}

bool USpawnSelectionComponent::SpawnInitialPawn(
	AController* Controller,
	TSubclassOf<APawn> PawnClass,
	APawn*& OutSpawnedPawn,
	FTransform& OutSpawnTransform,
	int32& OutSpawnPointIndex)
{
	OutSpawnedPawn = nullptr;
	OutSpawnTransform = FTransform::Identity;
	OutSpawnPointIndex = INDEX_NONE;

	if (!SelectInitialSpawnTransformForController(Controller, OutSpawnTransform, OutSpawnPointIndex))
	{
		UE_LOG(LogSpawnSelection, Warning,
			TEXT("SpawnInitialPawn failed: could not select an initial spawn point."));
		return false;
	}

	if (!SpawnPawnAtTransform(
		Controller,
		PawnClass,
		OutSpawnTransform,
		OutSpawnedPawn))
	{
		// SelectInitialSpawnTransform에서 선점한 배정 횟수를 자동 복구합니다.
		ReleaseInitialSpawnSelection(OutSpawnPointIndex);
		OutSpawnTransform = FTransform::Identity;
		OutSpawnPointIndex = INDEX_NONE;
		return false;
	}

	return true;
}

bool USpawnSelectionComponent::RespawnPawn(
	AController* Controller,
	TSubclassOf<APawn> PawnClass,
	const FVector& DeathLocation,
	APawn*& OutSpawnedPawn,
	FTransform& OutSpawnTransform,
	int32& OutSpawnPointIndex)
{
	OutSpawnedPawn = nullptr;
	OutSpawnTransform = FTransform::Identity;
	OutSpawnPointIndex = INDEX_NONE;

	if (!SelectRespawnTransformForController(
		Controller,
		DeathLocation,
		OutSpawnTransform,
		OutSpawnPointIndex))
	{
		UE_LOG(LogSpawnSelection, Warning,
			TEXT("RespawnPawn failed: could not select a respawn point."));
		return false;
	}

	return SpawnPawnAtTransform(
		Controller,
		PawnClass,
		OutSpawnTransform,
		OutSpawnedPawn);
}

bool USpawnSelectionComponent::SpawnPawnAtTransform(
	AController* Controller,
	TSubclassOf<APawn> PawnClass,
	const FTransform& SpawnTransform,
	APawn*& OutSpawnedPawn)
{
	OutSpawnedPawn = nullptr;

	if (!IsValid(Controller) || !PawnClass)
	{
		UE_LOG(LogSpawnSelection, Warning,
			TEXT("SpawnPawnAtTransform failed: Controller or PawnClass is invalid."));
		return false;
	}

	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor) || !OwnerActor->HasAuthority())
	{
		UE_LOG(LogSpawnSelection, Warning,
			TEXT("SpawnPawnAtTransform rejected: spawn must be executed on Authority."));
		return false;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParameters;
	// 현재 BP_QuakeGameMode Request Respawn의 동작과 동일하게 맞춥니다.
	SpawnParameters.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	APawn* SpawnedPawn = World->SpawnActor<APawn>(
		PawnClass,
		SpawnTransform.GetLocation(),
		SpawnTransform.Rotator(),
		SpawnParameters);

	if (!IsValid(SpawnedPawn))
	{
		UE_LOG(LogSpawnSelection, Warning,
			TEXT("SpawnPawnAtTransform failed: SpawnActor returned null."));
		return false;
	}

	Controller->Possess(SpawnedPawn);

	if (Controller->GetPawn() != SpawnedPawn)
	{
		UE_LOG(LogSpawnSelection, Warning,
			TEXT("SpawnPawnAtTransform failed: Controller could not possess the spawned Pawn."));

		SpawnedPawn->Destroy();
		return false;
	}

	/*
	 * 스폰 시스템은 캐릭터의 게임플레이 상태를 초기화하지 않습니다.
	 * HP/Armor/Inventory/Buff, 이동 Velocity/Force 등의 초기화는
	 * Controller/Pawn 쪽 기존 시스템의 책임으로 남겨둡니다.
	 *
	 * 여기서는 SpawnPoint가 지정한 방향만 적용합니다.
	 */
	SpawnedPawn->SetActorRotation(SpawnTransform.Rotator());
	ApplySpawnRotation(Controller, SpawnTransform.Rotator());

	OutSpawnedPawn = SpawnedPawn;
	return true;
}

void USpawnSelectionComponent::ApplySpawnRotation(
	AController* Controller,
	const FRotator& SpawnRotation)
{
	if (!IsValid(Controller))
	{
		return;
	}

	// 서버 Controller / AIController의 ControlRotation을 먼저 초기화합니다.
	Controller->SetControlRotation(SpawnRotation);

	/*
	 * PlayerController는 실제 화면을 소유한 Client에도 회전을 적용해야 합니다.
	 * ClientSetRotation은 엔진에 이미 있는 Reliable Client RPC이므로
	 * BP_PlayerController에 별도 커스텀 RPC를 만들 필요가 없습니다.
	 */
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		PlayerController->ClientSetRotation(SpawnRotation, true);
	}
}
