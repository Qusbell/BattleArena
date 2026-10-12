# C++ 조사 색인

실제 프로젝트 루트의 Source만 포함합니다. Assignment/Source는 배포 복사본이므로 별도 구현으로 중복 집계하지 않았습니다. Runtime 구현 / 테스트 / Editor 도구를 구분합니다.

## JumpPadCommonSettings.cpp

분류: Runtime. [소스](../../Source/ShootingArena/JumpPadCommonSettings.cpp)

- 9: `void UJumpPadCommonSettings::PlayJumpPadSound(const UObject* WorldContextObject, FVector Location)`

## JumpPadCommonSettings.h

분류: Runtime. [소스](../../Source/ShootingArena/JumpPadCommonSettings.h)

- 9: `class USoundBase;`
- 10: `class USoundAttenuation;`
- 23: `class SHOOTINGARENA_API UJumpPadCommonSettings : public UDataAsset`

## JumpPadFlightComponent.cpp

분류: Runtime. [소스](../../Source/ShootingArena/JumpPadFlightComponent.cpp)

- 8: `UJumpPadFlightComponent::UJumpPadFlightComponent()`
- 14: `void UJumpPadFlightComponent::BeginPlay()`
- 26: `void UJumpPadFlightComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)`
- 36: `void UJumpPadFlightComponent::BeginFlight(float RiseGravityScale, float FallGravityScale)`
- 59: `void UJumpPadFlightComponent::EndFlight()`
- 70: `void UJumpPadFlightComponent::HandleLanded(const FHitResult& Hit)`
- 75: `void UJumpPadFlightComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)`

## JumpPadFlightComponent.h

분류: Runtime. [소스](../../Source/ShootingArena/JumpPadFlightComponent.h)

- 9: `class ACharacter;`
- 10: `class UCharacterMovementComponent;`
- 24: `class SHOOTINGARENA_API UJumpPadFlightComponent : public UActorComponent`

## MyAIController.cpp

분류: Runtime. [소스](../../Source/ShootingArena/MyAIController.cpp)

- 4: `void AMyAIController::UpdateControlRotation(float DeltaTime, bool bUpdatePawn)`

## MyAIController.h

분류: Runtime. [소스](../../Source/ShootingArena/MyAIController.h)

- 8: `class SHOOTINGARENA_API AMyAIController : public AAIController`

## AIAreaBase.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/AI/Area/AIAreaBase.cpp)

- 20: `AAIAreaBase::AAIAreaBase()`
- 77: `void AAIAreaBase::OnConstruction(const FTransform& Transform)`
- 92: `void AAIAreaBase::PostActorCreated()`
- 103: `void AAIAreaBase::PostLoad()`
- 114: `void AAIAreaBase::BeginDestroy()`
- 123: `bool AAIAreaBase::ContainsPosition(const FVector& WorldPosition) const`
- 139: `FVector AAIAreaBase::GetAreaCenter() const`
- 144: `FVector AAIAreaBase::GetAreaExtent() const`
- 149: `FVector AAIAreaBase::GetClosestPointInArea(const FVector& WorldPosition, const float Inset) const`
- 174: `FVector AAIAreaBase::GetPointInsideTowards(const FVector& WorldTarget, const float Inset) const`
- 179: `FBox AAIAreaBase::GetAreaBounds() const`
- 190: `void AAIAreaBase::FitAreaToSurroundings()`
- 338: `void AAIAreaBase::UpdateAreaDebugPreview()`
- 447: `void AAIAreaBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)`
- 457: `void AAIAreaBase::PostEditImport()`
- 467: `void AAIAreaBase::PostEditUndo()`
- 476: `void AAIAreaBase::BindActorLabelChangedDelegate()`
- 489: `void AAIAreaBase::UnbindActorLabelChangedDelegate()`
- 500: `void AAIAreaBase::HandleActorLabelChanged(AActor* ChangedActor)`
- 509: `void AAIAreaBase::SyncAreaIdFromActorLabel()`

## AreaLinkBase.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/AI/Area/AreaLinkBase.cpp)

- 9: `AAreaLinkBase::AAreaLinkBase()`
- 31: `void AAreaLinkBase::OnConstruction(const FTransform& Transform)`
- 45: `bool AAreaLinkBase::IsValidLink() const`
- 84: `bool AAreaLinkBase::ResolveEndpointLocations(`
- 103: `void AAreaLinkBase::RefreshEndpointPreview()`
- 108: `bool AAreaLinkBase::TryResolveEndpointLocations(`
- 174: `FVector AAreaLinkBase::GetEntryLocationAtoB() const`
- 188: `FVector AAreaLinkBase::GetExitLocationAtoB() const`
- 202: `EAreaTraversalType AAreaLinkBase::GetTraversalTypeBtoA() const`
- 218: `FVector AAreaLinkBase::GetEntryLocationBtoA() const`
- 229: `FVector AAreaLinkBase::GetExitLocationBtoA() const`
- 234: `AActor* AAreaLinkBase::GetEffectiveEntryActor() const`
- 244: `bool AAreaLinkBase::TryResolveActorReferenceEndpoints(`
- 267: `bool AAreaLinkBase::TryResolveAutomaticEndpoints(`
- 379: `bool AAreaLinkBase::TryTraceAndProjectLandingPoint(`
- 428: `bool AAreaLinkBase::TryProjectPointToNavigation(`
- 457: `void AAreaLinkBase::UpdateEndpointPreview()`

## AreaManagerActorBase.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/AI/Area/AreaManagerActorBase.cpp)

- 28: `AAreaManagerActorBase::AAreaManagerActorBase()`
- 33: `void AAreaManagerActorBase::RebuildAreaGraph()`
- 104: `void AAreaManagerActorBase::ValidateAreaGraph()`
- 146: `void AAreaManagerActorBase::ClearBakedAreaGraph()`
- 159: `void AAreaManagerActorBase::DrawAreaGraphDebug()`

## AreaManagerSubsystem.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/AI/Area/AreaManagerSubsystem.cpp)

- 50: `UAreaManagerSubsystem::UAreaManagerSubsystem() = default;`
- 58: `void UAreaManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)`
- 74: `void UAreaManagerSubsystem::Deinitialize()`
- 97: `void UAreaManagerSubsystem::OnWorldBeginPlay(UWorld& InWorld)`
- 118: `bool UAreaManagerSubsystem::RebuildRuntimeGraph()`
- 200: `AAIAreaBase* UAreaManagerSubsystem::GetAreaByPosition(`
- 208: `float UAreaManagerSubsystem::GetAreaRiskScore(`
- 217: `FAreaRiskScoreResult UAreaManagerSubsystem::GetAreaRiskScoreDetails(`
- 226: `bool UAreaManagerSubsystem::SetAreaRecognized(`
- 242: `bool UAreaManagerSubsystem::SetAreaRecognizedByPosition(`
- 256: `bool UAreaManagerSubsystem::RecognizeUnit(`
- 272: `bool UAreaManagerSubsystem::ForgetRecognizedUnit(`
- 286: `bool UAreaManagerSubsystem::MarkCombatDetected(`
- 300: `bool UAreaManagerSubsystem::ClearCombatDetected(`
- 314: `bool UAreaManagerSubsystem::ClearCombatDetectedByPosition(`
- 327: `void UAreaManagerSubsystem::RemoveObserverRiskMemory(`
- 338: `void UAreaManagerSubsystem::CleanupInvalidRiskMemory()`
- 346: `bool UAreaManagerSubsystem::FindSafestAreaRoute(`
- 361: `bool UAreaManagerSubsystem::FindSafestAreaRouteToActor(`
- 385: `bool UAreaManagerSubsystem::FindAreaRoute(`
- 401: `bool UAreaManagerSubsystem::FindAreaRouteToActor(`
- 423: `TArray<AAIAreaBase*> UAreaManagerSubsystem::GetRegisteredAreas() const`
- 443: `bool UAreaManagerSubsystem::GetBestBakedRetreatPoint(`
- 453: `void UAreaManagerSubsystem::StoreRetreatRouteCache(`
- 472: `bool UAreaManagerSubsystem::TryGetRetreatRouteCache(`
- 513: `void UAreaManagerSubsystem::ClearRetreatRouteCache(`
- 522: `void UAreaManagerSubsystem::HandleAreaRiskDebugTimer()`
- 569: `void UAreaManagerSubsystem::DumpAreaRiskDebug(`
- 778: `bool UAreaManagerSubsystem::PassesAreaRiskDebugFilter(`
- 803: `FString UAreaManagerSubsystem::GetAreaRiskDebugObserverName(`
- 811: `FString UAreaManagerSubsystem::GetAreaRiskDebugAreaName(`
- 825: `void UAreaManagerSubsystem::ResetAreaRiskDebugState()`
- 832: `bool UAreaManagerSubsystem::DoesSupportWorldType(`
- 840: `AAreaManagerActorBase* UAreaManagerSubsystem::FindManagerActor() const`
- 876: `bool UAreaManagerSubsystem::CanWriteAuthoritativeRisk() const`
- 882: `void UAreaManagerSubsystem::DestroyServices()`

## AreaRiskConfigDataAsset.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/AI/Area/AreaRiskConfigDataAsset.cpp)

- 선언·구현 항목이 없거나 매크로로 정의됩니다.

## BTS_UpdateRetreatDecision.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/AI/Area/BTS_UpdateRetreatDecision.cpp)

- 12: `UBTS_UpdateRetreatDecision::UBTS_UpdateRetreatDecision()`
- 44: `void UBTS_UpdateRetreatDecision::OnBecomeRelevant(`
- 52: `void UBTS_UpdateRetreatDecision::TickNode(`
- 61: `void UBTS_UpdateRetreatDecision::UpdateRetreatDecision(`

## BTT_SelectRetreatAreaStep.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/AI/Area/BTT_SelectRetreatAreaStep.cpp)

- 14: `UBTT_SelectRetreatAreaStep::UBTT_SelectRetreatAreaStep()`
- 73: `EBTNodeResult::Type UBTT_SelectRetreatAreaStep::ExecuteTask(`
- 152: `void UBTT_SelectRetreatAreaStep::ResetBlackboardOutputs(`
- 171: `bool UBTT_SelectRetreatAreaStep::WriteFirstRouteStepToBlackboard(`
- 267: `AActor* UBTT_SelectRetreatAreaStep::GetOrCreateMoveTargetActor(`

## AreaGraphService.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/AI/Area/Internal/AreaGraphService.cpp)

- 76: `bool FAreaGraphService::BuildFromWorldActors(UWorld* World, const AAreaManagerActorBase* ManagerActor)`
- 107: `bool FAreaGraphService::LoadBakedConnections(UWorld* World, const AAreaManagerActorBase* ManagerActor)`
- 168: `void FAreaGraphService::ExportBakedConnections(TArray<FAreaBakedConnection>& OutConnections) const`
- 196: `void FAreaGraphService::BuildEditorNavigationCaches(`
- 311: `bool FAreaGraphService::TryGetBakedTransitionDistance(`
- 332: `bool FAreaGraphService::GetBestBakedRetreatPoint(`
- 362: `uint64 FAreaGraphService::MakeTransitionCacheKey(`
- 370: `void FAreaGraphService::Reset()`
- 384: `AAIAreaBase* FAreaGraphService::FindAreaByPosition(const FVector& WorldPosition) const`
- 398: `AAIAreaBase* FAreaGraphService::FindContainingAreaByPosition(const FVector& WorldPosition) const`
- 420: `AAIAreaBase* FAreaGraphService::FindAreaByPositionOrNearest(`
- 481: `bool FAreaGraphService::IsAreaPreferredOnTie(`
- 538: `const TArray<int32>& FAreaGraphService::GetOutgoingConnectionIndices(const AAIAreaBase* Area) const`
- 548: `void FAreaGraphService::Validate(TArray<FString>& OutIssues) const`
- 590: `void FAreaGraphService::CollectAreas(UWorld* World)`
- 602: `void FAreaGraphService::AddManualLinks(UWorld* World, TSet<uint32>& OutManualPairHashes)`
- 729: `void FAreaGraphService::AddAutomaticNormalLinks(`
- 842: `void FAreaGraphService::AddDirectedConnection(const FAreaDirectedConnection& Connection)`
- 850: `void FAreaGraphService::RebuildAdjacency()`
- 866: `uint32 FAreaGraphService::MakeDirectedPairHash(const AAIAreaBase* FromArea, const AAIAreaBase* ToArea)`
- 871: `void FAreaGraphService::BuildBoundaryCandidates(`
- 970: `bool FAreaGraphService::TryProjectCandidateToNavigation(`
- 1027: `bool FAreaGraphService::TryFindBestAutomaticNormalEndpoints(`
- 1113: `struct FBoundaryCandidatePair`
- 1322: `float FAreaGraphService::ComputeBoundsGapXY(const FBox& A, const FBox& B)`
- 1329: `bool FAreaGraphService::TryGetNavigationPathLength(`

## AreaGraphService.h

분류: Runtime. [소스](../../Source/ShootingArena/Private/AI/Area/Internal/AreaGraphService.h)

- 6: `class AAIAreaBase;`
- 7: `class AAreaLinkBase;`
- 8: `class AAreaManagerActorBase;`
- 9: `class UWorld;`
- 12: `struct FAreaDirectedConnection`
- 32: `class FAreaGraphService`

## AreaRetreatPlanner.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/AI/Area/Internal/AreaRetreatPlanner.cpp)

- 9: `bool FAreaRetreatPlanner::FindBestRetreatPlan(`
- 124: `bool FAreaRetreatPlanner::IsRouteExecutable(`
- 168: `bool FAreaRetreatPlanner::IsCandidateRouteBetter(`

## AreaRetreatPlanner.h

분류: Runtime. [소스](../../Source/ShootingArena/Private/AI/Area/Internal/AreaRetreatPlanner.h)

- 6: `class AAIController;`
- 7: `class AAIAreaBase;`
- 8: `class APawn;`
- 9: `class UAreaManagerSubsystem;`
- 10: `class UWorld;`
- 13: `struct FAreaRetreatPlannerSettings`
- 23: `struct FAreaRetreatPlan`
- 36: `class FAreaRetreatPlanner`

## AreaRiskService.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/AI/Area/Internal/AreaRiskService.cpp)

- 10: `FAreaRiskService::FAreaRiskService(FAreaGraphService& InGraphService)`
- 15: `void FAreaRiskService::SetRiskConfig(UAreaRiskConfigDataAsset* InRiskConfig)`
- 20: `bool FAreaRiskService::SetAreaRecognized(`
- 61: `bool FAreaRiskService::RecognizeUnit(`
- 104: `bool FAreaRiskService::ForgetRecognizedUnit(`
- 127: `bool FAreaRiskService::MarkCombatDetected(`
- 167: `bool FAreaRiskService::ClearCombatDetected(`
- 190: `FAreaRiskScoreResult FAreaRiskService::GetAreaRiskScoreDetails(`
- 269: `float FAreaRiskService::GetAreaRiskScore(`
- 277: `void FAreaRiskService::RemoveObserverMemory(const AAIController* ObserverController)`
- 285: `void FAreaRiskService::CleanupInvalidMemory()`
- 315: `void FAreaRiskService::Reset()`
- 321: `FAreaObserverRiskMemory* FAreaRiskService::FindObserverMemory(`
- 335: `const FAreaObserverRiskMemory* FAreaRiskService::FindObserverMemory(`
- 349: `FAreaObserverRiskMemory& FAreaRiskService::FindOrAddObserverMemory(`
- 362: `int32 FAreaRiskService::CountConnectedAreas(const AAIAreaBase* Area) const`
- 396: `double FAreaRiskService::GetCurrentWorldTime(`
- 419: `bool FAreaRiskService::IsUnitMemoryActive(`
- 442: `bool FAreaRiskService::IsCombatMemoryActive(`
- 465: `bool FAreaRiskService::ContainsArea(`
- 480: `void FAreaRiskService::AddAreaUnique(`
- 492: `void FAreaRiskService::RemoveArea(`

## AreaRiskService.h

분류: Runtime. [소스](../../Source/ShootingArena/Private/AI/Area/Internal/AreaRiskService.h)

- 6: `class AAIAreaBase;`
- 7: `class AAIController;`
- 8: `class AActor;`
- 9: `class UAreaRiskConfigDataAsset;`
- 10: `class FAreaGraphService;`
- 13: `struct FAreaRecognizedUnitRuntime`
- 23: `struct FAreaCombatMemoryRuntime`
- 37: `struct FAreaObserverRiskMemory`
- 57: `class FAreaRiskService`

## AreaRouteFinder.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/AI/Area/Internal/AreaRouteFinder.cpp)

- 11: `struct FRouteScore`
- 17: `struct FSearchState`
- 150: `FAreaRouteFinder::FAreaRouteFinder(`
- 160: `bool FAreaRouteFinder::FindSafestRoute(`
- 167: `bool FAreaRouteFinder::FindRoute(`
- 174: `bool FAreaRouteFinder::FindRouteInternal(`

## AreaRouteFinder.h

분류: Runtime. [소스](../../Source/ShootingArena/Private/AI/Area/Internal/AreaRouteFinder.h)

- 6: `class UWorld;`
- 7: `class FAreaGraphService;`
- 8: `class FAreaRiskService;`
- 11: `class FAreaRouteFinder`

## BTService_SelectTarget_R.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/AI/BTService_SelectTarget_R.cpp)

- 6: `UBTService_SelectTarget_R::UBTService_SelectTarget_R()`
- 14: `void UBTService_SelectTarget_R::TickNode(`

## PathLinkRouteFinder.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/AI/PathLink/Internal/PathLinkRouteFinder.cpp)

- 18: `enum class EDynamicRouteEdgeType : uint8`
- 25: `struct FDynamicRouteEdge`
- 171: `bool FPathLinkRouteFinder::BuildStaticGraph(`
- 318: `bool FPathLinkRouteFinder::FindShortestRoute(`
- 369: `struct FEndpointCandidate`
- 378: `struct FEndpointState`

## PathLinkRouteFinder.h

분류: Runtime. [소스](../../Source/ShootingArena/Private/AI/PathLink/Internal/PathLinkRouteFinder.h)

- 6: `class AActor;`
- 7: `class APathLink;`
- 8: `class UWorld;`
- 19: `class FPathLinkRouteFinder`

## PathLink.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/AI/PathLink/PathLink.cpp)

- 70: `APathLink::APathLink()`
- 106: `void APathLink::BeginPlay()`
- 123: `void APathLink::EndPlay(const EEndPlayReason::Type EndPlayReason)`
- 136: `void APathLink::Tick(const float DeltaSeconds)`
- 149: `void APathLink::AttachToGround()`
- 194: `bool APathLink::IsValidLink() const`
- 201: `bool APathLink::IsValidLinkForRouteCache() const`
- 215: `bool APathLink::ValidateLink(FText& OutFailureReason) const`
- 230: `bool APathLink::ValidateAndLog() const`
- 294: `void APathLink::ValidateInEditor()`
- 299: `bool APathLink::LogValidationErrors() const`
- 330: `bool APathLink::HasDuplicatePlacementError(FString& OutDetails) const`
- 361: `void APathLink::CollectValidationErrors(TArray<FString>& OutErrors, const bool bIncludeDuplicatePlacement) const`
- 499: `void APathLink::CollectDuplicatePlacementErrors(`
- 555: `bool APathLink::TraceGround(FHitResult& OutHit) const`
- 582: `void APathLink::CollectNavigationErrors(TArray<FString>& OutErrors) const`
- 638: `bool APathLink::CanUseForNavigation() const`
- 656: `bool APathLink::CanProjectToNavigation(const FVector& WorldLocation) const`
- 681: `FVector APathLink::GetEntryLocation() const`
- 687: `FVector APathLink::GetExitLocation() const`
- 694: `bool APathLink::ResolveTravelLocations(`
- 714: `bool APathLink::TryResolveTravelLocations(`
- 766: `double APathLink::GetTravelDistance(const bool Reverse) const`
- 786: `FVector APathLink::ResolveExitPoint(AActor* Actor, const FVector& LocalOffset) const`
- 799: `FColor APathLink::GetVisualColor() const`
- 821: `void APathLink::DrawEditorVisual() const`
- 872: `void APathLink::DrawArrowHead(`

## PathLinkSettings.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/AI/PathLink/PathLinkSettings.cpp)

- 선언·구현 항목이 없거나 매크로로 정의됩니다.

## PathLinkSubsystem.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/AI/PathLink/PathLinkSubsystem.cpp)

- 18: `void UPathLinkSubsystem::Initialize(FSubsystemCollectionBase& Collection)`
- 25: `void UPathLinkSubsystem::Deinitialize()`
- 39: `void UPathLinkSubsystem::OnWorldBeginPlay(UWorld& InWorld)`
- 83: `void UPathLinkSubsystem::RefreshLinks()`
- 150: `TArray<APathLink*> UPathLinkSubsystem::GetAllLinks() const`
- 166: `TArray<APathLink*> UPathLinkSubsystem::GetEnabledLinks() const`
- 183: `TArray<APathLink*> UPathLinkSubsystem::GetInvalidLinks() const`
- 199: `bool UPathLinkSubsystem::ValidateAllLinks(`
- 234: `TArray<APathLink*> UPathLinkSubsystem::GetLinksByType(`
- 261: `int32 UPathLinkSubsystem::GetLinkCount() const`
- 275: `APathLink* UPathLinkSubsystem::GetNearestLink(`
- 314: `bool UPathLinkSubsystem::ProjectToNavigation(`
- 349: `bool UPathLinkSubsystem::GetNavPathDistance(`
- 379: `void UPathLinkSubsystem::DrawDebugRoute(`
- 465: `bool UPathLinkSubsystem::FindShortestRoute(`
- 514: `bool UPathLinkSubsystem::FindShortestRouteToActor(`
- 533: `bool UPathLinkSubsystem::GetRouteDistance(`
- 551: `bool UPathLinkSubsystem::CanReach(`
- 560: `void UPathLinkSubsystem::InvalidateRouteGraphCache()`
- 572: `void UPathLinkSubsystem::RebuildCommonRouteCache() const`
- 614: `TArray<APathLink*> UPathLinkSubsystem::GetCachedRouteLinks() const`
- 630: `bool UPathLinkSubsystem::EnsureRouteGraphCache(`
- 700: `bool UPathLinkSubsystem::HasBlockingDuplicateLinks(`
- 747: `void UPathLinkSubsystem::BlockEditorPlayForDuplicates(const FString& Summary) const`
- 766: `void UPathLinkSubsystem::RegisterLink(APathLink* Link)`
- 799: `void UPathLinkSubsystem::UnregisterLink(APathLink* Link)`

## QuakeTargetSelectionAIController.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/AI/QuakeTargetSelectionAIController.cpp)

- 13: `struct FCandidate`
- 22: `AQuakeTargetSelectionAIController::AQuakeTargetSelectionAIController()`
- 26: `void AQuakeTargetSelectionAIController::OnPossess(APawn* InPawn)`
- 45: `void AQuakeTargetSelectionAIController::OnUnPossess()`
- 57: `void AQuakeTargetSelectionAIController::GetCharacterTargets_Implementation(`
- 71: `void AQuakeTargetSelectionAIController::HandleTargetPerceptionUpdated(`
- 110: `void AQuakeTargetSelectionAIController::HandlePossessedPawnDamaged(`
- 125: `ACharacter* AQuakeTargetSelectionAIController::ResolveAttacker(`
- 159: `bool AQuakeTargetSelectionAIController::IsCharacterDead(const ACharacter* Character)`
- 216: `bool AQuakeTargetSelectionAIController::TryGetAggression(`
- 311: `void AQuakeTargetSelectionAIController::SelectBestTarget()`
- 404: `UClass* AQuakeTargetSelectionAIController::GetAcceptedTargetClass() const`
- 440: `void AQuakeTargetSelectionAIController::ApplySelectedTarget(ACharacter* Target)`

## ArenaAnalyzeLibrary.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/Analyzer/ArenaAnalyzeLibrary.cpp)

- 11: `bool UArenaAnalyzeLibrary::LoadAnalyzeFile(const FString& FilePath, FArenaAnalyzeSession& OutSession)`
- 27: `TArray<FString> UArenaAnalyzeLibrary::GetAnalyzeFiles()`
- 50: `bool UArenaAnalyzeLibrary::LoadAnalyzeFileByName(`
- 62: `TArray<FArenaLineSegment> UArenaAnalyzeLibrary::BuildMovementLineSegments(`
- 99: `TArray<FArenaMovementSample> UArenaAnalyzeLibrary::FilterMovementSamples(`
- 117: `TArray<FArenaDamageSample> UArenaAnalyzeLibrary::FilterDamageSamples(`

## ArenaAnalyzeSubsystem.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/Analyzer/ArenaAnalyzeSubsystem.cpp)

- 12: `bool UArenaAnalyzeSubsystem::ShouldCreateSubsystem(UObject* Outer) const`
- 22: `void UArenaAnalyzeSubsystem::Initialize(FSubsystemCollectionBase& Collection)`
- 37: `void UArenaAnalyzeSubsystem::Deinitialize()`
- 63: `void UArenaAnalyzeSubsystem::RegisterController(AController* Controller)`
- 83: `void UArenaAnalyzeSubsystem::AnalyzeControllers()`
- 127: `void UArenaAnalyzeSubsystem::SaveSamplesToJson()`
- 166: `UArenaAnalyzeSubsystem::FindStateForController(const AController* Controller)`
- 180: `UArenaAnalyzeSubsystem::FindStateForPawn(const APawn* Pawn)`
- 193: `void UArenaAnalyzeSubsystem::SetTrackedPawn(`
- 218: `void UArenaAnalyzeSubsystem::OnPossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)`
- 235: `void UArenaAnalyzeSubsystem::OnPawnTakeAnyDamage(`

## BGMPlayback.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/Audio/BGMPlayback.cpp)

- 9: `USoundBase* BGMPlayback::CreateLoopingSound(UObject* Outer, USoundBase* Source)`

## BGMPlayback.h

분류: Runtime. [소스](../../Source/ShootingArena/Private/Audio/BGMPlayback.h)

- 3: `class UObject;`
- 4: `class USoundBase;`

## BGMSettingsDataAsset.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/Audio/BGMSettingsDataAsset.cpp)

- 3: `UBGMSettingsDataAsset::UBGMSettingsDataAsset()`

## BGMSubsystem.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/Audio/BGMSubsystem.cpp)

- 14: `void UBGMSubsystem::Initialize(FSubsystemCollectionBase& Collection)`
- 32: `void UBGMSubsystem::Deinitialize()`
- 51: `void UBGMSubsystem::SetSettings(UBGMSettingsDataAsset* NewSettings)`
- 58: `void UBGMSubsystem::PlayMainMenuBGM()`
- 63: `void UBGMSubsystem::PlayLobbyBGM()`
- 68: `bool UBGMSubsystem::PlayGameplayBGM(FName MapID)`
- 82: `void UBGMSubsystem::PlayResultBGM()`
- 87: `bool UBGMSubsystem::IsGameplayMap(const FString& MapNameOrPath) const`
- 92: `void UBGMSubsystem::StopBGM(float FadeOutTime)`
- 115: `void UBGMSubsystem::SetBGMVolume(float NormalizedVolume, float FadeTime)`
- 121: `void UBGMSubsystem::SetBGMVolumePercent(float VolumePercent, float FadeTime)`
- 126: `void UBGMSubsystem::PlayPreviewBGM(USoundBase* TestSound, bool bLoop, float FadeInTime, float FadeOutTime)`
- 141: `void UBGMSubsystem::HandlePreLoadMap(const FString& MapName)`
- 152: `void UBGMSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)`
- 167: `void UBGMSubsystem::PlayConfiguredTrack(const FBGMTrack& Track, EBGMState NewState, const TCHAR* DebugName)`
- 193: `void UBGMSubsystem::PlayLoadedTrack(USoundBase* Sound, const FBGMTrack& Track, EBGMState NewState, FString DebugName, uint32 RequestId)`
- 244: `void UBGMSubsystem::HandleAudioFinished(UAudioComponent* FinishedComponent)`
- 255: `void UBGMSubsystem::ApplyVolumeOverride(float FadeTime)`
- 267: `FName UBGMSubsystem::GetCurrentMapName() const`
- 276: `FName UBGMSubsystem::NormalizeMapName(const FString& MapNameOrPath)`
- 305: `float UBGMSubsystem::ResolveFadeIn(const FBGMTrack& Track) const`
- 310: `float UBGMSubsystem::ResolveFadeOut(const FBGMTrack& Track) const`
- 315: `bool UBGMSubsystem::CanPlayAudio() const`

## CharacterSpawnSoundComponent.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/Audio/CharacterSpawnSoundComponent.cpp)

- 12: `UCharacterSpawnSoundComponent::UCharacterSpawnSoundComponent()`
- 18: `void UCharacterSpawnSoundComponent::PlaySpawnSounds(ECharacterSpawnSoundPhase Phase)`
- 59: `void UCharacterSpawnSoundComponent::MulticastPlaySpawnSounds_Implementation(ECharacterSpawnSoundPhase Phase, FVector SpawnLocation)`
- 64: `void UCharacterSpawnSoundComponent::ClientPlaySpawnSounds_Implementation(ECharacterSpawnSoundPhase Phase, FVector SpawnLocation)`
- 69: `void UCharacterSpawnSoundComponent::PlayLocalSpawnSounds(ECharacterSpawnSoundPhase Phase, const FVector& SpawnLocation, bool bAudibleToOthers)`
- 113: `void UCharacterSpawnSoundComponent::PlayLocalEntry(const FCharacterSpawnSoundEntry& Entry, const FVector& SpawnLocation)`

## KillStreakVoiceComponent.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/Audio/KillStreakVoiceComponent.cpp)

- 14: `UKillStreakVoiceComponent::UKillStreakVoiceComponent()`
- 20: `void UKillStreakVoiceComponent::BeginPlay()`
- 32: `void UKillStreakVoiceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)`
- 50: `void UKillStreakVoiceComponent::QueueKillEvent(bool bIsRevenge)`
- 90: `void UKillStreakVoiceComponent::PlayFirstBloodVoice()`
- 112: `void UKillStreakVoiceComponent::NotifyOwnerDeath()`
- 142: `void UKillStreakVoiceComponent::ProcessKillBatch()`
- 201: `USoundBase* UKillStreakVoiceComponent::SelectVoiceForCurrentStreak() const`
- 266: `void UKillStreakVoiceComponent::DispatchVoiceToOwner(USoundBase* Sound)`
- 298: `void UKillStreakVoiceComponent::ClientPlayKillStreakVoice_Implementation(USoundBase* Sound)`
- 303: `void UKillStreakVoiceComponent::ClientClearPendingKillStreakVoice_Implementation()`
- 308: `void UKillStreakVoiceComponent::PlayVoiceLocally(USoundBase* Sound)`
- 337: `void UKillStreakVoiceComponent::ClearPendingVoiceLocally()`
- 342: `void UKillStreakVoiceComponent::HandleActiveVoiceFinished()`

## BuffPresentationComponent.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/Buff/BuffPresentationComponent.cpp)

- 27: `UBuffPresentationComponent::UBuffPresentationComponent()`
- 34: `void UBuffPresentationComponent::BeginPlay()`
- 53: `void UBuffPresentationComponent::OnOwnerDeath(AController* InstigatedBy, AActor* DamageCauser)`
- 63: `void UBuffPresentationComponent::SetBuffPresentation(FGameplayTag BuffTag, double EndServerTime)`
- 94: `void UBuffPresentationComponent::ClearBuffPresentation()`
- 99: `double UBuffPresentationComponent::GetPresentationRemainingTime() const`
- 108: `const FBuffVisualEffects* UBuffPresentationComponent::GetEffects() const`
- 115: `void UBuffPresentationComponent::OnRep_Presentation()`
- 149: `void UBuffPresentationComponent::StopEffects()`
- 158: `void UBuffPresentationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)`
- 165: `void UBuffPresentationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)`
- 180: `void UBuffPresentationComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const`
- 186: `void UBuffPresentationComponent::UpdateBuffRemainingTimeText(AActor* BuffOwner, UTextBlock* TimeText)`

## BuffSpawnCycleSubsystem.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/Buff/BuffSpawnCycleSubsystem.cpp)

- 36: `void UBuffSpawnCycleSubsystem::Tick(const float DeltaTime)`
- 89: `TStatId UBuffSpawnCycleSubsystem::GetStatId() const`
- 94: `bool UBuffSpawnCycleSubsystem::IsTickable() const`
- 100: `void UBuffSpawnCycleSubsystem::UpdateReplicatedBuffItem(AActor* BuffItem)`
- 141: `void UBuffSpawnCycleSubsystem::UpdateSpawnPoint(AActor* SpawnPoint)`
- 199: `FName UBuffSpawnCycleSubsystem::GetNextBuffRow(const FName CurrentRow)`

## DeathCamActor.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/Camera/DeathCamActor.cpp)

- 12: `ADeathCamActor::ADeathCamActor()`
- 21: `void ADeathCamActor::Tick(float deltaSeconds)`
- 33: `void ADeathCamActor::EndPlay(const EEndPlayReason::Type endPlayReason)`
- 42: `bool ADeathCamActor::InitializeDeathCam(`
- 103: `void ADeathCamActor::StopDeathCam()`
- 115: `void ADeathCamActor::CacheSettings(UDeathCamDataAsset* inDeathCamData)`
- 141: `void ADeathCamActor::UpdateDeathCam(float deltaSeconds)`
- 179: `FVector ADeathCamActor::CalculateKillerDirection() const`
- 204: `FVector ADeathCamActor::CalculateBaseCameraLocation(const FVector& killerDirection) const`
- 211: `FVector ADeathCamActor::ResolveInitialCameraLocation(const FVector& desiredCameraLocation) const`
- 263: `FVector ADeathCamActor::MoveCameraWithCollision(`
- 348: `FVector ADeathCamActor::ClampCameraLocation(`
- 372: `FRotator ADeathCamActor::CalculateLookAtCenterRotation(const FVector& cameraLocation) const`
- 384: `void ADeathCamActor::ApplyKillerHighlightPostProcess()`
- 409: `void ADeathCamActor::ApplyKillerHighlight()`
- 516: `void ADeathCamActor::RemoveKillerHighlight()`

## DeathCamComponent.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/Camera/DeathCamComponent.cpp)

- 13: `UDeathCamComponent::UDeathCamComponent()`
- 24: `void UDeathCamComponent::BeginPlay()`
- 36: `void UDeathCamComponent::EndPlay(const EEndPlayReason::Type endPlayReason)`
- 49: `void UDeathCamComponent::TickComponent(`
- 83: `void UDeathCamComponent::StartDeathCam(APawn* deadPawn, AController* instigatedBy, AActor* damageCauser)`
- 111: `void UDeathCamComponent::StopDeathCam(APawn* newPawn)`
- 134: `void UDeathCamComponent::ClientStartDeathCam_Implementation(`
- 142: `void UDeathCamComponent::ClientStopDeathCam_Implementation(APawn* newPawn)`
- 147: `APlayerController* UDeathCamComponent::GetOwningPlayerController()`
- 157: `AActor* UDeathCamComponent::ResolveOtherActor(`
- 214: `void UDeathCamComponent::StartDeathCamLocal(`
- 301: `void UDeathCamComponent::RequestStopDeathCamLocal(APawn* newPawn)`
- 321: `void UDeathCamComponent::TryCompleteStopDeathCam()`
- 384: `void UDeathCamComponent::FinishStopDeathCam()`
- 404: `void UDeathCamComponent::DestroyDeathCamImmediately()`
- 416: `float UDeathCamComponent::GetViewTargetBlendTime() const`

## DeathCamDataAsset.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/Camera/DeathCamDataAsset.cpp)

- 3: `UMaterialInterface* UDeathCamDataAsset::GetKillerHighlightMaterial() const`

## DebugCameraOcclusionComponent.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/Camera/DebugCameraOcclusionComponent.cpp)

- 10: `UDebugCameraOcclusionComponent::UDebugCameraOcclusionComponent()`
- 21: `void UDebugCameraOcclusionComponent::UpdateOcclusion(UCameraComponent* Camera, APawn* TargetPawn)`
- 109: `void UDebugCameraOcclusionComponent::RestoreOcclusion()`
- 125: `void UDebugCameraOcclusionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)`

## ShooterDisplayLibrary.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/Camera/ShooterDisplayLibrary.cpp)

- 8: `bool UShooterDisplayLibrary::IsShooterAlive(APawn* Shooter)`
- 38: `FText UShooterDisplayLibrary::GetShooterDisplayName(APawn* Shooter)`

## ShooterDisplayValidation.h

분류: Runtime. [소스](../../Source/ShootingArena/Private/Camera/ShooterDisplayValidation.h)

- 선언·구현 항목이 없거나 매크로로 정의됩니다.

## ShooterNicknameComponent.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/Camera/ShooterNicknameComponent.cpp)

- 19: `UShooterNicknameComponent::UShooterNicknameComponent()`
- 26: `void UShooterNicknameComponent::BeginPlay()`
- 47: `void UShooterNicknameComponent::ApplyNicknameSettings(UShooterNicknameSettings* NewSettings)`
- 68: `void UShooterNicknameComponent::SetNicknameStyle(const FShooterNicknameStyle& NewStyle)`
- 92: `void UShooterNicknameComponent::SetNicknameEnabled(bool bEnabled)`
- 99: `void UShooterNicknameComponent::SetNicknameLingerTime(float Seconds)`
- 106: `void UShooterNicknameComponent::SetCrosshairUV(FVector2D UV)`
- 112: `void UShooterNicknameComponent::ResetNicknameStyle()`
- 117: `bool UShooterNicknameComponent::ResolveAimRay(FVector& Origin, FVector& Direction)`
- 132: `ECollisionChannel UShooterNicknameComponent::ResolveWeaponDamageTraceChannel(UObject* Data, float& Distance,`
- 161: `ECollisionChannel UShooterNicknameComponent::ResolveDamageTraceChannel(float& Distance, FCollisionResponseParams& Responses) const`
- 193: `APawn* UShooterNicknameComponent::FindAimTarget(const FVector& Origin, const FVector& Direction)`
- 232: `void UShooterNicknameComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction)`
- 284: `void UShooterNicknameComponent::UpdateTarget(APawn* Target)`
- 293: `void UShooterNicknameComponent::ClearDisplay()`
- 301: `FText UShooterNicknameComponent::ResolveNickname_Implementation(APawn* Shooter) const`
- 306: `FLinearColor UShooterNicknameComponent::GetDisplayedNicknameColor() const`
- 313: `void UShooterNicknameComponent::EndPlay(const EEndPlayReason::Type Reason)`

## ShooterNicknameSettings.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/Camera/ShooterNicknameSettings.cpp)

- 4: `FShooterNicknameStyle::FShooterNicknameStyle()`

## ShooterNicknameWidget.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/Camera/ShooterNicknameWidget.cpp)

- 10: `void UShooterNicknameWidget::SetDisplayComponent(UShooterNicknameComponent* Component)`
- 15: `void UShooterNicknameWidget::RefreshDisplay()`
- 20: `int32 UShooterNicknameWidget::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry,`

## ShooterOutlineComponent.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/Camera/ShooterOutlineComponent.cpp)

- 23: `class FShooterOutlineViewExtension final : public FSceneViewExtensionBase`
- 67: `void UShooterOutlineComponent::ApplyViewVisibility(FSceneView& View) const`
- 92: `UShooterOutlineComponent::UShooterOutlineComponent()`
- 99: `void UShooterOutlineComponent::BeginPlay()`
- 113: `void UShooterOutlineComponent::ApplyOutlineSettings(UShooterOutlineSettings* NewSettings)`
- 135: `void UShooterOutlineComponent::PrepareMaterial(UMaterialInterface* SourceMaterial)`
- 152: `bool UShooterOutlineComponent::IsMaterialReady() const`
- 159: `void UShooterOutlineComponent::SetOutlineStyle(const FShooterOutlineStyle& NewStyle)`
- 177: `void UShooterOutlineComponent::SetOutlineEnabled(bool bEnabled)`
- 184: `void UShooterOutlineComponent::ResetOutlineStyle()`
- 189: `void UShooterOutlineComponent::UpdateMaterial()`
- 201: `int32 UShooterOutlineComponent::GetShooterStencil(const APawn* Shooter) const`
- 208: `void UShooterOutlineComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction)`
- 234: `void UShooterOutlineComponent::RefreshShooters()`
- 315: `void UShooterOutlineComponent::UpdateProxies()`
- 368: `void UShooterOutlineComponent::ClearProxies()`
- 375: `void UShooterOutlineComponent::EndPlay(const EEndPlayReason::Type Reason)`

## ShooterRevengeComponent.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/Camera/ShooterRevengeComponent.cpp)

- 21: `UShooterRevengeComponent::UShooterRevengeComponent()`
- 29: `void UShooterRevengeComponent::BeginPlay()`
- 38: `void UShooterRevengeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)`
- 44: `void UShooterRevengeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)`
- 52: `APlayerState* UShooterRevengeComponent::ReadPlayerStateRevengeTarget() const`
- 66: `void UShooterRevengeComponent::RefreshRevengeTargetFromPlayerState()`
- 81: `void UShooterRevengeComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const`
- 87: `void UShooterRevengeComponent::SetRevengeTargetPlayerState(APlayerState* Target)`
- 105: `void UShooterRevengeComponent::SetRevengeTarget(APawn* Target)`
- 110: `void UShooterRevengeComponent::ClearRevengeTarget()`
- 115: `APlayerState* UShooterRevengeComponent::GetRevengeTargetPlayerState() const`
- 127: `bool UShooterRevengeComponent::IsRevengeTarget(const APawn* Shooter) const`
- 134: `void UShooterRevengeComponent::OnRep_TargetPlayerState()`

## CharacterAppearanceComponent.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/Character/CharacterAppearanceComponent.cpp)

- 14: `UCharacterAppearanceComponent::UCharacterAppearanceComponent()`
- 20: `void UCharacterAppearanceComponent::BeginPlay()`
- 34: `void UCharacterAppearanceComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const`
- 40: `bool UCharacterAppearanceComponent::ApplyConfiguredAppearance()`
- 63: `bool UCharacterAppearanceComponent::SetModelingId(const FString& newModelingId)`
- 81: `bool UCharacterAppearanceComponent::SetModelingRowName(const FName newModelingRowName)`
- 99: `bool UCharacterAppearanceComponent::SetModelingRowHandle(const FDataTableRowHandle& newModelingRowHandle)`
- 140: `bool UCharacterAppearanceComponent::ApplyDeathAppearance()`
- 152: `bool UCharacterAppearanceComponent::GetActiveModelingRow(FCharacterModelingRow& outModelingRow) const`
- 167: `void UCharacterAppearanceComponent::OnRep_ActiveModelingId()`
- 172: `bool UCharacterAppearanceComponent::ApplyModelingIdInternal(const FString& modelingId)`
- 185: `bool UCharacterAppearanceComponent::FindModelingRow(const FString& modelingId, FCharacterModelingRow& outModelingRow) const`
- 224: `USkeletalMeshComponent* UCharacterAppearanceComponent::ResolveCharacterMeshComponent() const`
- 244: `TArray<USkeletalMeshComponent*> UCharacterAppearanceComponent::ResolveDeathMeshComponents() const`
- 322: `void UCharacterAppearanceComponent::ApplyLiveMesh(const FCharacterModelingRow& modelingRow) const`
- 345: `void UCharacterAppearanceComponent::ApplyDeathMesh(const FCharacterModelingRow& modelingRow) const`
- 410: `void UCharacterAppearanceComponent::ApplyDeathPartMaterials(`

## RecentAttackerComponent.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/Combat/RecentAttackerComponent.cpp)

- 11: `URecentAttackerComponent::URecentAttackerComponent()`
- 17: `void URecentAttackerComponent::BeginPlay()`
- 40: `void URecentAttackerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)`
- 50: `void URecentAttackerComponent::HandleOwnerTakeAnyDamage(`
- 81: `FKillCreditResolution URecentAttackerComponent::ResolveDeathCredit(`
- 138: `APlayerState* URecentAttackerComponent::ResolvePlayerState(`
- 191: `bool URecentAttackerComponent::IsSelfCaused(AController* InstigatedBy, AActor* DamageCauser) const`
- 227: `APlayerState* URecentAttackerComponent::GetVictimPlayerState() const`
- 234: `float URecentAttackerComponent::GetAttackerMemoryTime() const`

## CustomWrapperLibrary.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/CustomWrapperLibrary.cpp)

- 19: `ENavPathTestResult UCustomWrapperLibrary::TestPathExistsSync(`
- 86: `bool UCustomWrapperLibrary::GetActiveBehaviorTreeNode(`
- 145: `bool UCustomWrapperLibrary::GetBehaviorTreeDebugInfo(`
- 202: `bool UCustomWrapperLibrary::GetActiveBehaviorTreePath(`
- 258: `bool UCustomWrapperLibrary::GetPeakKeyFromRuntimeFloatCurve(`
- 384: `bool UCustomWrapperLibrary::SuggestJumpPadVelocity(`
- 440: `bool UCustomWrapperLibrary::SuggestJumpPadVelocityByApexTime(`
- 489: `FGameplayTag UCustomWrapperLibrary::GetDirectParentGameplayTag(const FGameplayTag& Tag)`
- 495: `FText UCustomWrapperLibrary::WrapTextByCharCount(const FString& InString, int32 MaxCharsPerLine, bool bBreakAtWordBoundary)`

## AggressionDataAssetBase.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/DA/AggressionDataAssetBase.cpp)

- 38: `void UAggressionDataAssetBase::PostEditChangeProperty(`
- 235: `void UAggressionDataAssetBase::ShowValidationNotification(`
- 288: `void UAggressionDataAssetBase::DismissValidationNotification(`
- 305: `EDataValidationResult UAggressionDataAssetBase::IsDataValid(`

## DeathDropCarrier.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/Items/DeathDropCarrier.cpp)

- 39: `ADeathDropCarrier::ADeathDropCarrier()`
- 63: `void ADeathDropCarrier::Initialize(const FDeathDropSettings& Settings, UClass* PickupClass,`
- 73: `void ADeathDropCarrier::BeginPlay()`
- 80: `bool ADeathDropCarrier::ConfigurePickup(AActor* Item, bool bUpdateMesh)`
- 103: `void ADeathDropCarrier::SpawnPickup()`
- 127: `void ADeathDropCarrier::Launch(const FVector& Impulse)`
- 139: `void ADeathDropCarrier::OnRep_Pickup()`
- 145: `void ADeathDropCarrier::OnRep_Settled()`
- 153: `void ADeathDropCarrier::OnBodyHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,`
- 170: `void ADeathDropCarrier::FinishPickup()`
- 178: `void ADeathDropCarrier::Tick(float DeltaSeconds)`
- 200: `void ADeathDropCarrier::EndPlay(const EEndPlayReason::Type EndPlayReason)`
- 207: `void ADeathDropCarrier::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const`

## DeathDropSubsystem.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/Items/DeathDropSubsystem.cpp)

- 17: `struct FCandidate`
- 107: `void UDeathDropSubsystem::HandleFatalDamage(AActor* Character, double NewHealth, FDataTableRowHandle Settings)`

## LoadingBlueprintLibrary.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/Loading/LoadingBlueprintLibrary.cpp)

- 16: `void ULoadingBlueprintLibrary::BeginLoadingScreen(const UObject* WorldContextObject, const FString& StatusText)`
- 26: `void ULoadingBlueprintLibrary::EndLoadingScreen(const UObject* WorldContextObject)`
- 39: `void ULoadingBlueprintLibrary::BeginLoadingAndOpenLevel(const UObject* WorldContextObject, const FString& LevelName, const FString& StatusText)`
- 59: `void ULoadingBlueprintLibrary::BeginLoadingAndReturnToMainMenu(const UObject* WorldContextObject, const FString& StatusText)`
- 100: `void ULoadingBlueprintLibrary::BeginLoadingAndLeaveMatchToMainMenu(const UObject* WorldContextObject, const FString& StatusText)`
- 136: `void ULoadingBlueprintLibrary::MarkServerWorldReady(const UObject* WorldContextObject)`
- 147: `int32 ULoadingBlueprintLibrary::GetConnectedHumanPlayerCount(const UObject* WorldContextObject)`

## LoadingCoordinator.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/Loading/LoadingCoordinator.cpp)

- 11: `ALoadingCoordinator::ALoadingCoordinator()`
- 19: `void ALoadingCoordinator::BeginPlay()`
- 40: `void ALoadingCoordinator::Tick(float DeltaSeconds)`
- 52: `void ALoadingCoordinator::MarkServerWorldReady()`
- 60: `void ALoadingCoordinator::RefreshServerState()`
- 76: `int32 ALoadingCoordinator::CountConnectedHumanPlayers() const`
- 88: `void ALoadingCoordinator::ApplyServerReadyFallback()`
- 96: `void ALoadingCoordinator::OnRep_Phase()`
- 101: `void ALoadingCoordinator::HandlePhaseChanged()`
- 122: `void ALoadingCoordinator::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const`

## LoadingCoordinatorWorldSubsystem.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/Loading/LoadingCoordinatorWorldSubsystem.cpp)

- 6: `void ULoadingCoordinatorWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)`

## LoadingScreenSubsystem.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/Loading/LoadingScreenSubsystem.cpp)

- 16: `void ULoadingScreenSubsystem::Initialize(FSubsystemCollectionBase& Collection)`
- 30: `void ULoadingScreenSubsystem::Deinitialize()`
- 39: `void ULoadingScreenSubsystem::HandlePreLoadMap(const FString& MapName)`
- 50: `void ULoadingScreenSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)`
- 59: `void ULoadingScreenSubsystem::BeginExternalLoading(const FString& StatusText)`
- 72: `bool ULoadingScreenSubsystem::Tick(float DeltaSeconds)`
- 101: `void ULoadingScreenSubsystem::UpdateLoadingState()`
- 153: `void ULoadingScreenSubsystem::ShowLoadingScreen(const FString& StatusText)`
- 182: `void ULoadingScreenSubsystem::HideLoadingScreen()`
- 197: `void ULoadingScreenSubsystem::RemoveLoadingScreen()`
- 218: `void ULoadingScreenSubsystem::ApplyInputLock()`
- 242: `void ULoadingScreenSubsystem::ReleaseInputLock()`
- 257: `FText ULoadingScreenSubsystem::GetStatusText() const`

## LobbyGameMode.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/LobbyGameMode.cpp)

- 9: `ALobbyGameState* ALobbyGameMode::GetLobbyGameState() const`
- 14: `void ALobbyGameMode::BeginPlay()`
- 23: `void ALobbyGameMode::AssignHost(APlayerController* NewHost)`
- 43: `void ALobbyGameMode::EnsureHostAssignedFallback()`
- 63: `void ALobbyGameMode::InitializeLobby(const FString& DefaultMapID, int32 MaxPlayerCount)`
- 71: `void ALobbyGameMode::MarkMatchLaunched()`
- 78: `void ALobbyGameMode::TravelToMatch(const FString& MatchTravelURL)`
- 100: `void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)`
- 150: `void ALobbyGameMode::Logout(AController* Exiting)`

## LobbyGameState.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/LobbyGameState.cpp)

- 7: `ALobbyGameState::ALobbyGameState()`
- 11: `void ALobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const`
- 21: `void ALobbyGameState::RebuildSlots(const FString& NewMapID, int32 NewMaxPlayerCount)`
- 92: `int32 ALobbyGameState::AssignPlayerToOpenSlot(APlayerState* PlayerState)`
- 119: `void ALobbyGameState::ReleasePlayerSlot(APlayerState* PlayerState)`
- 157: `void ALobbyGameState::ResetNonPlayerSlots()`
- 199: `void ALobbyGameState::SaveAISlotsToGameInstance() const`
- 230: `void ALobbyGameState::RestoreAISlotsFromGameInstance()`
- 273: `int32 ALobbyGameState::FindSlotIndexForPlayer(APlayerState* PlayerState) const`
- 291: `bool ALobbyGameState::AreAllNonHostPlayersReady() const`
- 306: `int32 ALobbyGameState::GetFilledSlotCount() const`
- 319: `void ALobbyGameState::CountAISlotsByDifficulty(int32& OutEasy, int32& OutNormal, int32& OutHard) const`
- 345: `void ALobbyGameState::OnRep_Slots()`
- 350: `void ALobbyGameState::OnRep_SelectedMapID()`
- 355: `void ALobbyGameState::OnRep_HostPlayerState()`
- 360: `void ALobbyGameState::OnRep_MatchServerAddress()`

## LobbyPlayerController.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/LobbyPlayerController.cpp)

- 8: `ALobbyGameState* ALobbyPlayerController::GetLobbyGameState() const`
- 13: `bool ALobbyPlayerController::IsHost() const`
- 19: `void ALobbyPlayerController::Server_SetSlotType_Implementation(int32 SlotIndex, ELobbySlotType NewType)`
- 53: `void ALobbyPlayerController::Server_SetAIConfig_Implementation(int32 SlotIndex, const FString& AIName, ELobbyDifficulty Difficulty)`
- 84: `void ALobbyPlayerController::Server_SetReady_Implementation(bool bReady)`
- 102: `void ALobbyPlayerController::Server_ToggleReady_Implementation()`
- 120: `void ALobbyPlayerController::Server_SetPlayerName_Implementation(const FString& NewName)`
- 157: `void ALobbyPlayerController::Server_ChangeMap_Implementation(const FString& NewMapID, int32 NewMaxPlayerCount)`
- 179: `void ALobbyPlayerController::Server_StartGame_Implementation()`
- 202: `void ALobbyPlayerController::Server_CleanupMatchServer_Implementation()`

## LocalDedicatedServerLibrary.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/LocalDedicatedServerLibrary.cpp)

- 339: `bool ULocalDedicatedServerLibrary::StartLocalDedicatedServer(`
- 569: `void ULocalDedicatedServerLibrary::StopLocalDedicatedServer()`
- 608: `void ULocalDedicatedServerLibrary::InitServerCommandWatcher()`
- 613: `bool ULocalDedicatedServerLibrary::SetLocalDedicatedServerMap(const FString& MapName)`
- 668: `bool ULocalDedicatedServerLibrary::IsLocalDedicatedServerRunning()`
- 675: `bool ULocalDedicatedServerLibrary::IsLocalDedicatedServerReady()`
- 694: `bool ULocalDedicatedServerLibrary::MarkLocalDedicatedServerReady(`
- 841: `bool ULocalDedicatedServerLibrary::StartMatchServer(const FString& MapName, int32 Port)`
- 988: `void ULocalDedicatedServerLibrary::StopMatchServer()`
- 1016: `bool ULocalDedicatedServerLibrary::IsMatchServerRunning()`
- 1021: `bool ULocalDedicatedServerLibrary::IsMatchServerReady()`
- 1038: `bool ULocalDedicatedServerLibrary::IsLocalIPAddress(const FString& IPAddress)`
- 1079: `bool ULocalDedicatedServerLibrary::IsValidIPAddress(const FString& IPAddress)`

## MyPlayerController.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/MyPlayerController.cpp)

- 10: `AMyPlayerController::AMyPlayerController()`
- 17: `void AMyPlayerController::PawnLeavingGame()`
- 26: `void AMyPlayerController::ServerSetGamePaused_Implementation(bool bShouldPause)`
- 51: `void AMyPlayerController::SetGamePausedByGameMode(bool bShouldPause)`
- 61: `void AMyPlayerController::ServerReturnToLobby_Implementation()`
- 83: `bool AMyPlayerController::IsStandaloneGame() const`
- 92: `void AMyPlayerController::ServerShutdownMatchServer_Implementation()`

## MyPlayerState.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/MyPlayerState.cpp)

- 11: `void AMyPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const`
- 18: `void AMyPlayerState::SetOwner(AActor* NewOwner)`
- 32: `void AMyPlayerState::Server_SetWantsReturnToLobby_Implementation(bool bWants)`
- 54: `void AMyPlayerState::Multicast_UpdateReturnToLobbyVoteCounts_Implementation(int32 InVotedCount, int32 InEligibleCount)`
- 61: `void AMyPlayerState::Server_RequestReturnToLobbyVotesRefresh_Implementation()`
- 76: `void AMyPlayerState::Multicast_PlayResultBGM_Implementation()`
- 94: `void AMyPlayerState::OnRep_WantsReturnToLobby()`
- 99: `void AMyPlayerState::ComputeReturnToLobbyVotes(const UWorld* World, int32& OutVotedCount, int32& OutEligibleCount)`
- 125: `void AMyPlayerState::GetReturnToLobbyVoteCounts(int32& OutVotedCount, int32& OutEligibleCount) const`
- 133: `FText AMyPlayerState::GetReturnToLobbyVoteCountsText() const`

## MyReconnectionGameMode.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/MyReconnectionGameMode.cpp)

- 11: `FString AMyReconnectionGameMode::InitNewPlayer(APlayerController* NewPlayerController,`
- 54: `void AMyReconnectionGameMode::PostLogin(APlayerController* NewPlayer)`
- 88: `void AMyReconnectionGameMode::Logout(AController* Exiting)`
- 98: `void AMyReconnectionGameMode::CheckReturnToLobbyVotes()`

## ShootingArenaGameInstance.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/ShootingArenaGameInstance.cpp)

- 10: `void UShootingArenaGameInstance::Init()`
- 29: `void UShootingArenaGameInstance::Shutdown()`
- 40: `void UShootingArenaGameInstance::HandlePreLoadMap(const FString& MapName)`
- 50: `FString UShootingArenaGameInstance::MakeDefaultPlayerName(bool bCampaign) const`
- 87: `void UShootingArenaGameInstance::SetSavedNickname(const FString& NetworkAddress, const FString& Nickname)`
- 97: `FString UShootingArenaGameInstance::GetSavedNickname(const FString& NetworkAddress) const`

## ItemSpawnSelectionLibrary.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/Spawner/ItemSpawnSelectionLibrary.cpp)

- 7: `bool UItemSpawnSelectionLibrary::SelectNextSpawnItem(`
- 90: `bool FItemSpawnSelectionTest::RunTest(const FString& Parameters)`

## BGMPlaybackTests.cpp

분류: 테스트. [소스](../../Source/ShootingArena/Private/Tests/BGMPlaybackTests.cpp)

- 35: `bool FBGMNativeWaveLoopTest::RunTest(const FString& Parameters)`
- 61: `bool FBGMNativeCueLoopTest::RunTest(const FString& Parameters)`
- 98: `bool FBGMWholeGraphLoopTest::RunTest(const FString& Parameters)`
- 138: `struct FBGMPauseTestContext`
- 158: `bool FCheckBGMDuringPause::Update()`
- 198: `bool FBeginBGMPauseTest::Update()`
- 240: `bool FRestoreBGMPauseTestSettings::Update()`
- 251: `bool FBGMPausedPIETest::RunTest(const FString& Parameters)`

## BuffPresentationTests.cpp

분류: 테스트. [소스](../../Source/ShootingArena/Private/Tests/BuffPresentationTests.cpp)

- 24: `bool FBuffPresentationTest::RunTest(const FString& Parameters)`

## CharacterDisplayTests.cpp

분류: 테스트. [소스](../../Source/ShootingArena/Private/Tests/CharacterDisplayTests.cpp)

- 40: `bool FCharacterDisplayRegressionTest::RunTest(const FString& Parameters)`

## ProjectileConvergenceTests.cpp

분류: 테스트. [소스](../../Source/ShootingArena/Private/Tests/ProjectileConvergenceTests.cpp)

- 18: `bool FProjectileConvergenceTest::RunTest(const FString& Parameters)`

## RevengePlayerStateBridgeTests.cpp

분류: 테스트. [소스](../../Source/ShootingArena/Private/Tests/RevengePlayerStateBridgeTests.cpp)

- 15: `bool FRevengePlayerStateBridgeTest::RunTest(const FString& Parameters)`

## WeaponProjectileAimTests.cpp

분류: 테스트. [소스](../../Source/ShootingArena/Private/Tests/WeaponProjectileAimTests.cpp)

- 18: `bool FWeaponProjectileAimTest::RunTest(const FString& Parameters)`

## AnimNode_WeaponAimAlignment.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/Weapon/AnimNode_WeaponAimAlignment.cpp)

- 54: `FAnimNode_WeaponAimAlignment::FAnimNode_WeaponAimAlignment()`
- 61: `void FAnimNode_WeaponAimAlignment::PreUpdate(const UAnimInstance* AnimInstance)`
- 136: `void FAnimNode_WeaponAimAlignment::Initialize_AnyThread(const FAnimationInitializeContext& Context)`
- 145: `void FAnimNode_WeaponAimAlignment::UpdateInternal(const FAnimationUpdateContext& Context)`
- 153: `void FAnimNode_WeaponAimAlignment::InitializeBoneReferences(const FBoneContainer& Bones)`
- 170: `bool FAnimNode_WeaponAimAlignment::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& Bones)`
- 176: `void FAnimNode_WeaponAimAlignment::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& Result)`
- 276: `void FAnimNode_WeaponAimAlignment::GatherDebugData(FNodeDebugData& DebugData)`

## InitialWeaponLibrary.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/Weapon/InitialWeaponLibrary.cpp)

- 4: `bool UInitialWeaponLibrary::IsDefaultWeapon(const AActor* Weapon)`

## InitialWeaponLoadoutComponent.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/Weapon/InitialWeaponLoadoutComponent.cpp)

- 196: `UInitialWeaponLoadoutComponent::UInitialWeaponLoadoutComponent()`
- 201: `void UInitialWeaponLoadoutComponent::GrantLoadout()`
- 253: `bool UInitialWeaponLoadoutComponent::IsDefaultWeapon(const AActor* Weapon) const`

## InitialWeaponLoadoutDataAsset.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/Weapon/InitialWeaponLoadoutDataAsset.cpp)

- 3: `UInitialWeaponLoadoutDataAsset::UInitialWeaponLoadoutDataAsset()`

## ProjectileConvergenceComponent.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/Weapon/ProjectileConvergenceComponent.cpp)

- 5: `UProjectileConvergenceComponent::UProjectileConvergenceComponent()`
- 11: `bool UProjectileConvergenceComponent::Initialize(UProjectileMovementComponent* InMovement,`
- 37: `void UProjectileConvergenceComponent::TickComponent(float DeltaTime, ELevelTick TickType,`

## ProjectileFirstPersonRenderingComponent.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/Weapon/ProjectileFirstPersonRenderingComponent.cpp)

- 30: `UProjectileFirstPersonRenderingComponent::UProjectileFirstPersonRenderingComponent()`
- 36: `void UProjectileFirstPersonRenderingComponent::Track(UPrimitiveComponent* Primitive)`
- 44: `void UProjectileFirstPersonRenderingComponent::RestoreView()`
- 56: `void UProjectileFirstPersonRenderingComponent::Restore()`
- 64: `void UProjectileFirstPersonRenderingComponent::PrepareWeapons(USkeletalMeshComponent* Arms)`
- 112: `void UProjectileFirstPersonRenderingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)`
- 154: `void UProjectileFirstPersonRenderingComponent::ApplyFirstPersonMuzzleEffect(AActor* Weapon, UPrimitiveComponent* Effect)`
- 165: `void UProjectileFirstPersonRenderingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)`

## ProjectileMuzzleFlashLibrary.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/Weapon/ProjectileMuzzleFlashLibrary.cpp)

- 41: `bool UProjectileMuzzleFlashLibrary::ShouldUseProjectileMuzzleFlash(AActor* Weapon)`
- 48: `FVector UProjectileMuzzleFlashLibrary::ResolveFlashDirection(FVector Velocity, FVector ShotStart, FVector ShotTarget)`
- 55: `bool UProjectileMuzzleFlashLibrary::SpawnProjectileMuzzleFlash(AActor* Weapon,`

## WeaponAimAlignmentComponent.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/Weapon/WeaponAimAlignmentComponent.cpp)

- 11: `UWeaponAimAlignmentComponent::UWeaponAimAlignmentComponent()`
- 18: `APawn* UWeaponAimAlignmentComponent::GetShooter() const`
- 30: `USkeletalMeshComponent* UWeaponAimAlignmentComponent::GetThirdPersonWeaponMesh() const`
- 38: `bool UWeaponAimAlignmentComponent::RefreshLocalTarget()`
- 68: `bool UWeaponAimAlignmentComponent::GetAlignmentTarget(FVector& OutTarget) const`
- 74: `bool UWeaponAimAlignmentComponent::GetAlignmentAim(FVector& OutTarget, FVector& OutViewForward) const`
- 85: `FVector UWeaponAimAlignmentComponent::ResolvePoseAimTarget(FVector MuzzleLocation, FVector FlightTarget,`
- 104: `float UWeaponAimAlignmentComponent::ClampSpineAimAngle(UAnimInstance* AnimInstance, float Angle)`
- 132: `float UWeaponAimAlignmentComponent::GetSpineAimLimit(UAnimInstance* AnimInstance)`
- 145: `float UWeaponAimAlignmentComponent::GetFreeHandPoseWeight(UAnimInstance* AnimInstance)`
- 158: `void UWeaponAimAlignmentComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)`
- 205: `void UWeaponAimAlignmentComponent::RestorePoseTick()`
- 216: `void UWeaponAimAlignmentComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)`
- 222: `void UWeaponAimAlignmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const`

## WeaponBeamEffectLibrary.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/Weapon/WeaponBeamEffectLibrary.cpp)

- 114: `bool UWeaponBeamEffectLibrary::PlayHitScanBeam(`

## WeaponPresentationLibrary.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/Weapon/WeaponPresentationLibrary.cpp)

- 30: `void UWeaponPresentationLibrary::ReconcileStandaloneEquip(UActorComponent* Inventory)`

## WeaponProjectileAimLibrary.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Private/Weapon/WeaponProjectileAimLibrary.cpp)

- 49: `FVector UWeaponProjectileAimLibrary::ResolveProjectileViewStart(AActor* Shooter,`
- 75: `bool UWeaponProjectileAimLibrary::TryGetProjectileBoreTransform(USceneComponent* Mesh,`
- 88: `const USphereComponent* UWeaponProjectileAimLibrary::GetProjectileCollisionTemplate(AActor* Weapon)`
- 98: `bool UWeaponProjectileAimLibrary::IsProjectileStartBlocked(UWorld* World, const USphereComponent* Collision,`
- 109: `bool UWeaponProjectileAimLibrary::IsProjectileMuzzleObstructed(UWorld* World, const USphereComponent* Collision,`
- 123: `bool UWeaponProjectileAimLibrary::TryResolveCameraProjectileStart(UWorld* World, const USphereComponent* Collision,`
- 165: `FVector UWeaponProjectileAimLibrary::ResolveSafeProjectileStart(AActor* Weapon, AActor* Shooter,`
- 190: `void UWeaponProjectileAimLibrary::ResolveMuzzleProjectileAim(const UObject* WorldContextObject,`
- 231: `bool UWeaponProjectileAimLibrary::FindMuzzleRayJoin(FVector CameraStart, FVector CameraForward,`
- 253: `void UWeaponProjectileAimLibrary::ResolveProjectileConvergence(UProjectileConvergenceSettings* Settings,`
- 289: `bool UWeaponProjectileAimLibrary::ConfigureProjectileConvergence(AActor* Projectile,`

## AIAreaBase.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/AI/Area/AIAreaBase.h)

- 7: `class USceneComponent;`
- 8: `class UBoxComponent;`
- 9: `class UStaticMeshComponent;`
- 10: `class UMaterialInterface;`
- 11: `class UMaterialInstanceDynamic;`
- 19: `class SHOOTINGARENA_API AAIAreaBase : public AActor`

## AreaLinkBase.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/AI/Area/AreaLinkBase.h)

- 8: `class USceneComponent;`
- 9: `class UArrowComponent;`
- 10: `class AAIAreaBase;`
- 11: `class UWorld;`
- 18: `class SHOOTINGARENA_API AAreaLinkBase : public AActor`

## AreaManagerActorBase.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/AI/Area/AreaManagerActorBase.h)

- 8: `class UAreaRiskConfigDataAsset;`
- 15: `class SHOOTINGARENA_API AAreaManagerActorBase : public AActor`

## AreaManagerSubsystem.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/AI/Area/AreaManagerSubsystem.h)

- 9: `class AAIAreaBase;`
- 10: `class AAreaManagerActorBase;`
- 11: `class AAIController;`
- 12: `class AActor;`
- 13: `class UAreaRiskConfigDataAsset;`
- 14: `class FAreaGraphService;`
- 15: `class FAreaRiskService;`
- 16: `class FAreaRouteFinder;`
- 22: `struct FAreaRiskDebugSnapshot`
- 69: `struct FCachedAreaRetreatRoute`
- 82: `class SHOOTINGARENA_API UAreaManagerSubsystem : public UWorldSubsystem`

## AreaRiskConfigDataAsset.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/AI/Area/AreaRiskConfigDataAsset.h)

- 20: `class SHOOTINGARENA_API UAreaRiskConfigDataAsset : public UPrimaryDataAsset`

## AreaTypes.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/AI/Area/AreaTypes.h)

- 6: `class AAIAreaBase;`
- 7: `class AAreaLinkBase;`
- 8: `class AAIController;`
- 9: `class AActor;`
- 16: `enum class EAreaTraversalType : uint8`
- 31: `enum class EAreaLinkEndpointMode : uint8`
- 48: `struct SHOOTINGARENA_API FAreaRiskScoreResult`
- 101: `struct SHOOTINGARENA_API FAreaTraversalCapabilities`
- 144: `struct SHOOTINGARENA_API FAreaBakedConnection`
- 194: `struct SHOOTINGARENA_API FAreaBakedTransitionDistance`
- 217: `struct SHOOTINGARENA_API FAreaBakedRetreatPoint`
- 230: `struct SHOOTINGARENA_API FAreaRouteStep`
- 265: `struct SHOOTINGARENA_API FAreaRouteRequest`
- 297: `struct SHOOTINGARENA_API FAreaRouteResult`

## BTS_UpdateRetreatDecision.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/AI/Area/BTS_UpdateRetreatDecision.h)

- 17: `class SHOOTINGARENA_API UBTS_UpdateRetreatDecision : public UBTService`

## BTT_SelectRetreatAreaStep.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/AI/Area/BTT_SelectRetreatAreaStep.h)

- 9: `class AAIController;`
- 10: `class AActor;`
- 11: `class UBlackboardComponent;`
- 12: `class UWorld;`
- 21: `class SHOOTINGARENA_API UBTT_SelectRetreatAreaStep : public UBTTaskNode`

## BTService_SelectTarget_R.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/AI/BTService_SelectTarget_R.h)

- 8: `class SHOOTINGARENA_API UBTService_SelectTarget_R : public UBTService`

## PathLink.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/AI/PathLink/PathLink.h)

- 8: `class USceneComponent;`
- 9: `class UBillboardComponent;`
- 16: `class SHOOTINGARENA_API APathLink : public AActor`

## PathLinkSettings.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/AI/PathLink/PathLinkSettings.h)

- 13: `class SHOOTINGARENA_API UPathLinkSettings : public UDeveloperSettings`

## PathLinkSubsystem.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/AI/PathLink/PathLinkSubsystem.h)

- 8: `class AActor;`
- 9: `class APathLink;`
- 16: `class SHOOTINGARENA_API UPathLinkSubsystem : public UWorldSubsystem`

## PathLinkTypes.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/AI/PathLink/PathLinkTypes.h)

- 6: `class APathLink;`
- 13: `enum class EPathLinkType : uint8`
- 23: `enum class EPathLinkSegmentType : uint8`
- 35: `struct SHOOTINGARENA_API FPathLinkRouteSegment`
- 73: `struct SHOOTINGARENA_API FPathLinkRouteResult`
- 106: `struct SHOOTINGARENA_API FPathLinkTraversalNode`
- 125: `struct SHOOTINGARENA_API FPathLinkCachedTransition`
- 135: `struct SHOOTINGARENA_API FPathLinkStaticGraph`

## QuakeTargetSelectionAIController.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/AI/QuakeTargetSelectionAIController.h)

- 8: `class ACharacter;`
- 11: `class SHOOTINGARENA_API AQuakeTargetSelectionAIController : public AMyAIController`

## ArenaAnalyzeLibrary.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/Analyzer/ArenaAnalyzeLibrary.h)

- 11: `class SHOOTINGARENA_API UArenaAnalyzeLibrary : public UBlueprintFunctionLibrary`

## ArenaAnalyzeSubsystem.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/Analyzer/ArenaAnalyzeSubsystem.h)

- 9: `class AController;`
- 10: `class APawn;`
- 11: `class UDamageType;`
- 14: `struct FArenaMovementSample`
- 29: `struct FArenaDamageSample`
- 52: `struct FArenaLineSegment`
- 67: `struct FArenaAnalyzeSession`
- 84: `class SHOOTINGARENA_API UArenaAnalyzeSubsystem : public UWorldSubsystem`
- 89: `struct FControllerAnalyzeState`

## BGMSettingsDataAsset.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/Audio/BGMSettingsDataAsset.h)

- 8: `class USoundClass;`
- 9: `class USoundMix;`
- 13: `struct SHOOTINGARENA_API FBGMTrack`
- 40: `class SHOOTINGARENA_API UBGMSettingsDataAsset : public UDataAsset`

## BGMSubsystem.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/Audio/BGMSubsystem.h)

- 8: `class UAudioComponent;`
- 9: `class USoundBase;`
- 10: `class UWorld;`
- 13: `enum class EBGMState : uint8`
- 25: `class SHOOTINGARENA_API UBGMSubsystem : public UGameInstanceSubsystem`

## CharacterSpawnSoundComponent.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/Audio/CharacterSpawnSoundComponent.h)

- 7: `class USoundAttenuation;`
- 8: `class USoundBase;`
- 11: `enum class ECharacterSpawnSoundPhase : uint8`
- 18: `enum class ECharacterSpawnSoundSpace : uint8`
- 26: `struct SHOOTINGARENA_API FCharacterSpawnSoundEntry`
- 53: `class SHOOTINGARENA_API UCharacterSpawnSoundComponent : public UActorComponent`

## KillStreakSettingsDataAsset.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/Audio/KillStreakSettingsDataAsset.h)

- 7: `class USoundBase;`
- 11: `struct SHOOTINGARENA_API FKillStreakVoiceTier`
- 26: `class SHOOTINGARENA_API UKillStreakSettingsDataAsset : public UDataAsset`

## KillStreakVoiceComponent.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/Audio/KillStreakVoiceComponent.h)

- 7: `class UAudioComponent;`
- 8: `class UKillStreakSettingsDataAsset;`
- 9: `class USoundBase;`
- 13: `class SHOOTINGARENA_API UKillStreakVoiceComponent : public UActorComponent`

## BuffPresentationComponent.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/Buff/BuffPresentationComponent.h)

- 8: `class UNiagaraSystem;`
- 9: `class UNiagaraComponent;`
- 10: `class UParticleSystem;`
- 11: `class UParticleSystemComponent;`
- 12: `class UTextBlock;`
- 16: `struct FBuffVisualEffects`
- 35: `struct FBuffPresentationState`
- 54: `class SHOOTINGARENA_API UBuffPresentationComponent : public UActorComponent`

## BuffSpawnCycleSubsystem.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/Buff/BuffSpawnCycleSubsystem.h)

- 8: `class AActor;`
- 18: `class SHOOTINGARENA_API UBuffSpawnCycleSubsystem final`
- 29: `struct FSpawnPointState`

## DeathCamActor.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/Camera/DeathCamActor.h)

- 7: `class APlayerController;`
- 8: `class UDeathCamDataAsset;`
- 9: `class UPrimitiveComponent;`
- 10: `class UMaterialInterface;`
- 26: `class SHOOTINGARENA_API ADeathCamActor : public ACameraActor`
- 118: `struct FHighlightComponentState`

## DeathCamComponent.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/Camera/DeathCamComponent.h)

- 7: `class AController;`
- 8: `class ADeathCamActor;`
- 9: `class APawn;`
- 10: `class APlayerController;`
- 11: `class UDeathCamDataAsset;`
- 20: `class SHOOTINGARENA_API UDeathCamComponent : public UActorComponent`

## DeathCamDataAsset.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/Camera/DeathCamDataAsset.h)

- 15: `enum class EDeathCamKillerHighlightType : uint8`
- 29: `class SHOOTINGARENA_API UDeathCamDataAsset : public UDataAsset`

## DebugCameraOcclusionComponent.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/Camera/DebugCameraOcclusionComponent.h)

- 7: `class APawn;`
- 8: `class UCameraComponent;`
- 9: `class UMaterialInterface;`
- 10: `class UPrimitiveComponent;`
- 13: `struct FDebugCameraOccludedMesh`
- 26: `class SHOOTINGARENA_API UDebugCameraOcclusionComponent : public UActorComponent`

## ShooterDisplayLibrary.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/Camera/ShooterDisplayLibrary.h)

- 7: `class APawn;`
- 11: `class SHOOTINGARENA_API UShooterDisplayLibrary : public UBlueprintFunctionLibrary`

## ShooterNicknameComponent.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/Camera/ShooterNicknameComponent.h)

- 8: `class UShooterNicknameWidget;`
- 9: `struct FCollisionResponseParams;`
- 14: `class SHOOTINGARENA_API UShooterNicknameComponent : public UActorComponent`

## ShooterNicknameSettings.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/Camera/ShooterNicknameSettings.h)

- 9: `class APawn;`
- 12: `struct SHOOTINGARENA_API FShooterNicknameStyle`
- 34: `class SHOOTINGARENA_API UShooterNicknameSettings : public UDataAsset`

## ShooterNicknameWidget.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/Camera/ShooterNicknameWidget.h)

- 7: `class UShooterNicknameComponent;`
- 11: `class SHOOTINGARENA_API UShooterNicknameWidget : public UUserWidget`

## ShooterOutlineComponent.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/Camera/ShooterOutlineComponent.h)

- 8: `class UMaterialInstanceDynamic;`
- 9: `class UMaterialInterface;`
- 10: `class FSceneViewExtensionBase;`
- 11: `class UMeshComponent;`
- 12: `class FSceneView;`
- 15: `struct FShooterOutlineProxy`
- 28: `class SHOOTINGARENA_API UShooterOutlineComponent : public UActorComponent`

## ShooterOutlineSettings.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/Camera/ShooterOutlineSettings.h)

- 7: `class UMaterialInterface;`
- 8: `class APawn;`
- 11: `struct SHOOTINGARENA_API FShooterOutlineStyle`
- 30: `class SHOOTINGARENA_API UShooterOutlineSettings : public UDataAsset`

## ShooterRevengeComponent.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/Camera/ShooterRevengeComponent.h)

- 7: `class APawn;`
- 8: `class APlayerState;`
- 14: `class SHOOTINGARENA_API UShooterRevengeComponent : public UActorComponent`

## CharacterAppearanceComponent.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/Character/CharacterAppearanceComponent.h)

- 10: `class UAnimInstance;`
- 11: `class USkeletalMesh;`
- 12: `class USkeletalMeshComponent;`
- 16: `struct FCharacterDeathMeshPart`
- 33: `struct FCharacterAppearanceRow : public FTableRowBase`
- 46: `struct FCharacterModelingRow : public FTableRowBase`
- 80: `class SHOOTINGARENA_API UCharacterAppearanceComponent : public UActorComponent`

## RecentAttackerComponent.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/Combat/RecentAttackerComponent.h)

- 7: `class AController;`
- 8: `class APlayerState;`
- 9: `class UKillStreakSettingsDataAsset;`
- 10: `class UDamageType;`
- 13: `enum class EKillDeathCause : uint8`
- 21: `enum class EKillCreditSource : uint8`
- 29: `struct SHOOTINGARENA_API FKillCreditResolution`
- 53: `class SHOOTINGARENA_API URecentAttackerComponent : public UActorComponent`

## CustomWrapperLibrary.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/CustomWrapperLibrary.h)

- 16: `enum class ENavPathTestResult : uint8`
- 30: `class SHOOTINGARENA_API UCustomWrapperLibrary : public UBlueprintFunctionLibrary`

## AggressionDataAssetBase.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/DA/AggressionDataAssetBase.h)

- 14: `class SHOOTINGARENA_API UAggressionDataAssetBase : public UPrimaryDataAsset`

## DeathDropCarrier.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/Items/DeathDropCarrier.h)

- 8: `class USphereComponent;`
- 12: `class SHOOTINGARENA_API ADeathDropCarrier : public AActor`

## DeathDropSettings.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/Items/DeathDropSettings.h)

- 9: `struct SHOOTINGARENA_API FDeathDropSettings : public FTableRowBase`

## DeathDropSubsystem.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/Items/DeathDropSubsystem.h)

- 10: `class SHOOTINGARENA_API UDeathDropSubsystem : public UWorldSubsystem`

## LoadingBlueprintLibrary.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/Loading/LoadingBlueprintLibrary.h)

- 9: `class SHOOTINGARENA_API ULoadingBlueprintLibrary : public UBlueprintFunctionLibrary`

## LoadingCoordinator.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/Loading/LoadingCoordinator.h)

- 10: `enum class ELoadingCoordinatorPhase : uint8`
- 26: `class SHOOTINGARENA_API ALoadingCoordinator : public AInfo`

## LoadingCoordinatorWorldSubsystem.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/Loading/LoadingCoordinatorWorldSubsystem.h)

- 9: `class SHOOTINGARENA_API ULoadingCoordinatorWorldSubsystem : public UWorldSubsystem`

## LoadingScreenSubsystem.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/Loading/LoadingScreenSubsystem.h)

- 8: `class SWidget;`
- 9: `class APlayerController;`
- 10: `class UWorld;`
- 14: `class SHOOTINGARENA_API ULoadingScreenSubsystem : public UGameInstanceSubsystem`

## LobbyGameMode.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/LobbyGameMode.h)

- 7: `class ALobbyGameState;`
- 18: `class SHOOTINGARENA_API ALobbyGameMode : public AMyReconnectionGameMode`

## LobbyGameState.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/LobbyGameState.h)

- 13: `class SHOOTINGARENA_API ALobbyGameState : public AGameStateBase`

## LobbyPlayerController.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/LobbyPlayerController.h)

- 8: `class ALobbyGameState;`
- 15: `class SHOOTINGARENA_API ALobbyPlayerController : public APlayerController`

## LobbyTypes.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/LobbyTypes.h)

- 6: `class APlayerState;`
- 10: `enum class ELobbySlotType : uint8`
- 20: `enum class ELobbyDifficulty : uint8`
- 30: `struct FLobbySavedAISlot`
- 46: `struct FLobbySlot`

## LocalDedicatedServerLibrary.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/LocalDedicatedServerLibrary.h)

- 14: `class SHOOTINGARENA_API ULocalDedicatedServerLibrary : public UBlueprintFunctionLibrary`

## MyPlayerController.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/MyPlayerController.h)

- 12: `class SHOOTINGARENA_API AMyPlayerController : public APlayerController`

## MyPlayerState.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/MyPlayerState.h)

- 12: `class SHOOTINGARENA_API AMyPlayerState : public APlayerState`

## MyReconnectionGameMode.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/MyReconnectionGameMode.h)

- 9: `class UReconnectionInterface : public UInterface { GENERATED_BODY() };`
- 11: `class IReconnectionInterface`
- 29: `class SHOOTINGARENA_API AMyReconnectionGameMode : public AGameModeBase`

## PlayerDefaultNameSettings.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/PlayerDefaultNameSettings.h)

- 9: `class SHOOTINGARENA_API UPlayerDefaultNameSettings : public UDataAsset`

## ShootingArenaGameInstance.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/ShootingArenaGameInstance.h)

- 8: `class UBGMSettingsDataAsset;`
- 9: `class UPlayerDefaultNameSettings;`
- 17: `class SHOOTINGARENA_API UShootingArenaGameInstance : public UGameInstance`

## ItemSpawnSelectionLibrary.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/Spawner/ItemSpawnSelectionLibrary.h)

- 9: `enum class E_ItemSpawnType : uint8`
- 19: `class SHOOTINGARENA_API UItemSpawnSelectionLibrary final : public UBlueprintFunctionLibrary`

## AnimNode_WeaponAimAlignment.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/Weapon/AnimNode_WeaponAimAlignment.h)

- 7: `class UWeaponAimAlignmentComponent;`
- 11: `struct SHOOTINGARENA_API FAnimNode_WeaponAimAlignment : public FAnimNode_SkeletalControlBase`

## InitialWeaponLibrary.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/Weapon/InitialWeaponLibrary.h)

- 10: `class SHOOTINGARENA_API UInitialWeaponLibrary : public UBlueprintFunctionLibrary`

## InitialWeaponLoadoutComponent.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/Weapon/InitialWeaponLoadoutComponent.h)

- 7: `class UInitialWeaponLoadoutDataAsset;`
- 11: `class SHOOTINGARENA_API UInitialWeaponLoadoutComponent : public UActorComponent`

## InitialWeaponLoadoutDataAsset.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/Weapon/InitialWeaponLoadoutDataAsset.h)

- 8: `struct FInitialWeaponEntry`
- 21: `class SHOOTINGARENA_API UInitialWeaponLoadoutDataAsset : public UPrimaryDataAsset`

## ProjectileConvergenceComponent.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/Weapon/ProjectileConvergenceComponent.h)

- 7: `class UProjectileMovementComponent;`
- 10: `enum class EProjectileConvergenceMode : uint8`
- 18: `class SHOOTINGARENA_API UProjectileConvergenceSettings : public UActorComponent`
- 53: `class SHOOTINGARENA_API UProjectileConvergenceComponent : public UActorComponent`

## ProjectileFirstPersonRenderingComponent.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/Weapon/ProjectileFirstPersonRenderingComponent.h)

- 8: `class UCameraComponent;`
- 9: `class USkeletalMesh;`
- 10: `class USkeletalMeshComponent;`
- 14: `class SHOOTINGARENA_API UProjectileFirstPersonRenderingComponent : public UActorComponent`

## ProjectileMuzzleFlashLibrary.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/Weapon/ProjectileMuzzleFlashLibrary.h)

- 7: `class AActor;`
- 8: `class USceneComponent;`
- 12: `class SHOOTINGARENA_API UProjectileMuzzleFlashLibrary : public UBlueprintFunctionLibrary`

## WeaponAimAlignmentComponent.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/Weapon/WeaponAimAlignmentComponent.h)

- 9: `class APawn;`
- 10: `class USkeletalMeshComponent;`
- 11: `class UAnimMontage;`
- 12: `class UAnimInstance;`
- 16: `class SHOOTINGARENA_API UWeaponAimAlignmentDataAsset : public UDataAsset`
- 79: `struct FWeaponAimAlignmentTarget`
- 92: `class SHOOTINGARENA_API UWeaponAimAlignmentComponent : public UActorComponent`

## WeaponAimAlignmentMath.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/Weapon/WeaponAimAlignmentMath.h)

- 선언·구현 항목이 없거나 매크로로 정의됩니다.

## WeaponBeamEffectLibrary.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/Weapon/WeaponBeamEffectLibrary.h)

- 8: `class UNiagaraSystem;`
- 9: `class UParticleSystem;`
- 13: `enum class EWeaponBeamPlayback : uint8`
- 20: `class SHOOTINGARENA_API UWeaponBeamEffectLibrary : public UBlueprintFunctionLibrary`

## WeaponPresentationLibrary.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/Weapon/WeaponPresentationLibrary.h)

- 7: `class UActorComponent;`
- 11: `class SHOOTINGARENA_API UWeaponPresentationLibrary : public UBlueprintFunctionLibrary`

## WeaponProjectileAimLibrary.h

분류: Runtime. [소스](../../Source/ShootingArena/Public/Weapon/WeaponProjectileAimLibrary.h)

- 7: `class AActor;`
- 8: `class USceneComponent;`
- 9: `class UProjectileConvergenceSettings;`
- 10: `class USphereComponent;`
- 11: `class UWorld;`
- 14: `class SHOOTINGARENA_API UWeaponProjectileAimLibrary : public UBlueprintFunctionLibrary`

## ShootingArena.cpp

분류: Runtime. [소스](../../Source/ShootingArena/ShootingArena.cpp)

- 19: `class FShootingArenaGameModule final : public FDefaultGameModuleImpl`

## ShootingArena.h

분류: Runtime. [소스](../../Source/ShootingArena/ShootingArena.h)

- 선언·구현 항목이 없거나 매크로로 정의됩니다.

## SpawnSelectionComponent.cpp

분류: Runtime. [소스](../../Source/ShootingArena/SpawnSelectionSystem/SpawnSelectionComponent.cpp)

- 15: `USpawnSelectionComponent::USpawnSelectionComponent()`
- 21: `void USpawnSelectionComponent::InitializeSpawnPoints(`
- 39: `void USpawnSelectionComponent::BeginPlay()`
- 50: `void USpawnSelectionComponent::DiscoverLevelSpawnPoints()`
- 108: `ESpawnPointType USpawnSelectionComponent::ReadSpawnPointType(const AActor* SpawnPointActor) const`
- 152: `bool USpawnSelectionComponent::IsSameRegisteredSpawnPoints(`
- 182: `void USpawnSelectionComponent::InitializeSpawnPointsWithTypes(`
- 214: `bool USpawnSelectionComponent::SetSpawnPointType(`
- 228: `ESpawnPointType USpawnSelectionComponent::GetSpawnPointType(USceneComponent* SpawnPoint) const`
- 236: `int32 USpawnSelectionComponent::GetSpawnPointCount() const`
- 251: `bool USpawnSelectionComponent::PrepareInitialSpawn(`
- 297: `bool USpawnSelectionComponent::ExtendInitialSpawn(`
- 331: `bool USpawnSelectionComponent::RecalculateInitialMaxPerPoint()`
- 352: `bool USpawnSelectionComponent::CanControllerUseSpawnPoint(`
- 379: `bool USpawnSelectionComponent::SelectInitialSpawnTransform(`
- 386: `bool USpawnSelectionComponent::SelectInitialSpawnTransformForController(`
- 495: `void USpawnSelectionComponent::ReleaseInitialSpawnSelection(`
- 510: `bool USpawnSelectionComponent::IsInitialSpawnPrepared() const`
- 515: `int32 USpawnSelectionComponent::GetInitialAssignmentsRemaining() const`
- 520: `int32 USpawnSelectionComponent::GetInitialTotalCharacterCount() const`
- 525: `bool USpawnSelectionComponent::SelectRespawnTransform(`
- 533: `bool USpawnSelectionComponent::SelectRespawnTransformForController(`
- 542: `struct FRespawnDistanceEntry`
- 628: `bool USpawnSelectionComponent::SpawnInitialPawn(`
- 663: `bool USpawnSelectionComponent::RespawnPawn(`
- 694: `bool USpawnSelectionComponent::SpawnPawnAtTransform(`
- 774: `void USpawnSelectionComponent::ApplySpawnRotation(`

## SpawnSelectionComponent.h

분류: Runtime. [소스](../../Source/ShootingArena/SpawnSelectionSystem/SpawnSelectionComponent.h)

- 7: `class AController;`
- 8: `class APawn;`
- 9: `class USceneComponent;`
- 10: `enum class ECharacterSpawnSoundPhase : uint8;`
- 14: `enum class ESpawnPointType : uint8`
- 40: `class USpawnSelectionComponent : public UActorComponent`

## OneWayTeleportActor.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Teleport/OneWayTeleportActor.cpp)

- 149: `AOneWayTeleportActor::AOneWayTeleportActor()`
- 253: `void AOneWayTeleportActor::OnConstruction(const FTransform& Transform)`
- 261: `void AOneWayTeleportActor::BeginPlay()`
- 280: `void AOneWayTeleportActor::UpdateTeleportActivation()`
- 298: `void AOneWayTeleportActor::UpdatePortalVisual()`
- 423: `bool AOneWayTeleportActor::CanDisplayPortalView(`
- 495: `FTransform AOneWayTeleportActor::GetPortalViewCameraTransform(`
- 527: `float AOneWayTeleportActor::GetPortalViewOpacity(float distance) const`
- 545: `void AOneWayTeleportActor::ApplyPortalView(`
- 585: `void AOneWayTeleportActor::UpdatePortalVFX()`
- 770: `void AOneWayTeleportActor::ClearPortalView()`
- 778: `FVector AOneWayTeleportActor::GetExitLaunchForward() const`
- 785: `void AOneWayTeleportActor::OnEntryBeginOverlap(`
- 852: `void AOneWayTeleportActor::TeleportCharacter(ACharacter* character)`
- 917: `FRotator AOneWayTeleportActor::GetExitFacingRotation() const`
- 930: `FVector AOneWayTeleportActor::GetLaunchVelocity() const`
- 945: `void AOneWayTeleportActor::ApplyMoveLock_Implementation(`

## OneWayTeleportActor.h

분류: Runtime. [소스](../../Source/ShootingArena/Teleport/OneWayTeleportActor.h)

- 7: `class UBoxComponent;`
- 8: `class USceneComponent;`
- 9: `class UStaticMeshComponent;`
- 10: `class UPrimitiveComponent;`
- 11: `class UMaterialInterface;`
- 12: `class UMaterialInstanceDynamic;`
- 13: `class UTeleportDataAsset;`
- 14: `class UTextureRenderTarget2D;`
- 15: `class UNiagaraComponent;`
- 16: `class UNiagaraSystem;`
- 17: `class UStaticMesh;`
- 18: `class ACharacter;`
- 19: `class APlayerController;`
- 34: `class SHOOTINGARENA_API AOneWayTeleportActor : public AActor`

## PortalViewSubsystem.cpp

분류: Runtime. [소스](../../Source/ShootingArena/Teleport/PortalViewSubsystem.cpp)

- 15: `struct FPortalCandidate`
- 23: `struct FReadyPortalCapture`
- 32: `void UPortalViewSubsystem::Initialize(FSubsystemCollectionBase& collection)`
- 45: `void UPortalViewSubsystem::Deinitialize()`
- 51: `void UPortalViewSubsystem::Tick(float deltaTime)`
- 286: `TStatId UPortalViewSubsystem::GetStatId() const`
- 291: `FPortalViewInstance& UPortalViewSubsystem::FindOrCreateView(AOneWayTeleportActor* portal)`
- 328: `int32 UPortalViewSubsystem::ResolvePortalRenderTargetSize(`
- 376: `float UPortalViewSubsystem::ResolvePortalUpdateRate(`
- 424: `bool UPortalViewSubsystem::EnsureRenderTarget(`
- 477: `void UPortalViewSubsystem::ClearPortalView(AOneWayTeleportActor* portal, FPortalViewInstance& view)`
- 496: `void UPortalViewSubsystem::ClearAllPortalViews()`

## PortalViewSubsystem.h

분류: Runtime. [소스](../../Source/ShootingArena/Teleport/PortalViewSubsystem.h)

- 7: `class AOneWayTeleportActor;`
- 8: `class USceneCaptureComponent2D;`
- 9: `class UTextureRenderTarget2D;`
- 12: `struct FPortalViewInstance`
- 40: `class SHOOTINGARENA_API UPortalViewSubsystem : public UTickableWorldSubsystem`

## TeleportDataAsset.h

분류: Runtime. [소스](../../Source/ShootingArena/Teleport/TeleportDataAsset.h)

- 7: `class USoundBase;`
- 14: `class SHOOTINGARENA_API UTeleportDataAsset : public UDataAsset`

## AnimGraphNode_WeaponAimAlignment.cpp

분류: Editor 도구. [소스](../../Source/ShootingArenaEditorTools/Private/AnimGraphNode_WeaponAimAlignment.cpp)

- 3: `FText UAnimGraphNode_WeaponAimAlignment::GetControllerDescription() const`
- 8: `FText UAnimGraphNode_WeaponAimAlignment::GetNodeTitle(ENodeTitleType::Type TitleType) const`
- 13: `FText UAnimGraphNode_WeaponAimAlignment::GetTooltipText() const`

## AnimGraphNode_WeaponAimAlignment.h

분류: Editor 도구. [소스](../../Source/ShootingArenaEditorTools/Private/AnimGraphNode_WeaponAimAlignment.h)

- 9: `class SHOOTINGARENAEDITORTOOLS_API UAnimGraphNode_WeaponAimAlignment : public UAnimGraphNode_SkeletalControlBase`

## DressSwapTool.cpp

분류: Editor 도구. [소스](../../Source/ShootingArenaEditorTools/Private/DressSwapTool.cpp)

- 26: `class SDressSwapPanel final : public SCompoundWidget`
- 185: `struct FReplacementTarget`
- 254: `struct FMaterialTarget`
- 321: `void FDressSwapTool::Register()`
- 354: `void FDressSwapTool::Unregister()`
- 367: `TSharedRef<SDockTab> FDressSwapTool::CreateTab(const FSpawnTabArgs& Args)`

## DressSwapTool.h

분류: Editor 도구. [소스](../../Source/ShootingArenaEditorTools/Private/DressSwapTool.h)

- 5: `class FDressSwapTool`

## ItemSoundMigrationLibrary.cpp

분류: Editor 도구. [소스](../../Source/ShootingArenaEditorTools/Private/ItemSoundMigrationLibrary.cpp)

- 121: `bool UItemSoundMigrationLibrary::ConfigureSoundFields(UUserDefinedStruct* S, const FString& Kind)`
- 149: `bool UItemSoundMigrationLibrary::ExtendSpawnFeedback(UBlueprint* BP)`
- 161: `bool UItemSoundMigrationLibrary::WireSpawnerSound(UBlueprint* BP, UDataTable* Table)`
- 203: `bool UItemSoundMigrationLibrary::WirePickupSound(UBlueprint* BP, UDataTable* Table)`
- 269: `bool UItemSoundMigrationLibrary::WireSpawnVolume(UBlueprint* BP)`
- 287: `bool UItemSoundMigrationLibrary::MoveSpawnSoundToSpawn(UBlueprint* BP)`
- 344: `bool UItemSoundMigrationLibrary::SeparateWeaponPickupSound(UBlueprint* Held, UBlueprint* Drop)`
- 393: `bool UItemSoundMigrationLibrary::RefreshAndCompile(UBlueprint* BP)`

## ItemSoundMigrationLibrary.h

분류: Editor 도구. [소스](../../Source/ShootingArenaEditorTools/Private/ItemSoundMigrationLibrary.h)

- 7: `class UBlueprint;`
- 8: `class UDataTable;`
- 9: `class UUserDefinedStruct;`
- 13: `class UItemSoundMigrationLibrary : public UBlueprintFunctionLibrary`

## PlasmaExplosionMigrationLibrary.cpp

분류: Editor 도구. [소스](../../Source/ShootingArenaEditorTools/Private/PlasmaExplosionMigrationLibrary.cpp)

- 18: `bool UPlasmaExplosionMigrationLibrary::CopyRocketExplosionFunctions(UBlueprint* Rocket, UBlueprint* Plasma)`
- 94: `bool UPlasmaExplosionMigrationLibrary::RestoreRocketExplosionDefaults(UBlueprint* Rocket, UBlueprint* Plasma)`
- 145: `bool UPlasmaExplosionMigrationLibrary::PrepareExplosionValidationActor(AActor* Projectile, AActor* Weapon, UObject* Config)`
- 169: `FVector UPlasmaExplosionMigrationLibrary::GetExplosionValidationLaunchVelocity(ACharacter* Character)`

## PlasmaExplosionMigrationLibrary.h

분류: Editor 도구. [소스](../../Source/ShootingArenaEditorTools/Private/PlasmaExplosionMigrationLibrary.h)

- 7: `class UBlueprint;`
- 8: `class AActor;`
- 9: `class ACharacter;`
- 13: `class UPlasmaExplosionMigrationLibrary : public UBlueprintFunctionLibrary`

## ProjectileBlueprintMigrationLibrary.cpp

분류: Editor 도구. [소스](../../Source/ShootingArenaEditorTools/Private/ProjectileBlueprintMigrationLibrary.cpp)

- 9: `bool UProjectileBlueprintMigrationLibrary::AddMuzzleAimSwitch(UBlueprint* Blueprint)`
- 22: `bool UProjectileBlueprintMigrationLibrary::ConfigureProjectileMulticast(UBlueprint* Blueprint,`
- 54: `bool UProjectileBlueprintMigrationLibrary::ExtendProjectileAimOutputs(UBlueprint* Blueprint)`
- 79: `void UProjectileBlueprintMigrationLibrary::RefreshProjectileNodes(UBlueprint* Blueprint)`

## ProjectileBlueprintMigrationLibrary.h

분류: Editor 도구. [소스](../../Source/ShootingArenaEditorTools/Private/ProjectileBlueprintMigrationLibrary.h)

- 7: `class UBlueprint;`
- 11: `class UProjectileBlueprintMigrationLibrary : public UBlueprintFunctionLibrary`

## ProjectileMuzzleFlashMigrationLibrary.cpp

분류: Editor 도구. [소스](../../Source/ShootingArenaEditorTools/Private/ProjectileMuzzleFlashMigrationLibrary.cpp)

- 53: `bool UProjectileMuzzleFlashMigrationLibrary::AddRocketProjectileMuzzleFlash(UBlueprint* Base, UBlueprint* Rocket)`
- 114: `bool UProjectileMuzzleFlashMigrationLibrary::VerifyMuzzleFlashDirections()`

## ProjectileMuzzleFlashMigrationLibrary.h

분류: Editor 도구. [소스](../../Source/ShootingArenaEditorTools/Private/ProjectileMuzzleFlashMigrationLibrary.h)

- 7: `class UBlueprint;`
- 10: `class UProjectileMuzzleFlashMigrationLibrary : public UBlueprintFunctionLibrary`

## ShootingArenaEditorToolsModule.cpp

분류: Editor 도구. [소스](../../Source/ShootingArenaEditorTools/Private/ShootingArenaEditorToolsModule.cpp)

- 27: `class FShootingArenaEditorToolsModule : public IModuleInterface`

## ItemSoundTests.cpp

분류: Editor 도구. [소스](../../Source/ShootingArenaEditorTools/Private/Tests/ItemSoundTests.cpp)

- 21: `bool FItemSoundTimingTest::RunTest(const FString& Parameters)`
- 71: `bool FItemSoundTest::RunTest(const FString& Parameters)`

## WeaponAimAlignmentMigrationLibrary.cpp

분류: Editor 도구. [소스](../../Source/ShootingArenaEditorTools/Private/WeaponAimAlignmentMigrationLibrary.cpp)

- 40: `bool UWeaponAimAlignmentMigrationLibrary::AddAlignmentToDuplicate(UAnimBlueprint* Blueprint)`
- 77: `FString UWeaponAimAlignmentMigrationLibrary::AddAnimationSelfNode(UAnimBlueprint* Blueprint, int32 X, int32 Y)`
- 94: `bool UWeaponAimAlignmentMigrationLibrary::AddPlasmaFreeHandLayer(UAnimBlueprint* Blueprint)`
- 161: `bool UWeaponAimAlignmentMigrationLibrary::RetargetPlasmaFreeHandStance(UAnimBlueprint* Blueprint)`
- 214: `FString UWeaponAimAlignmentMigrationLibrary::InspectPitchPose(UAnimBlueprint* Blueprint)`
- 244: `FString UWeaponAimAlignmentMigrationLibrary::VerifyPitchPoseMath()`
- 292: `bool UWeaponAimAlignmentMigrationLibrary::AddWeaponAlignmentComponent(UBlueprint* Blueprint, UWeaponAimAlignmentDataAsset* Settings)`
- 309: `bool UWeaponAimAlignmentMigrationLibrary::SetThirdPersonAnimation(UObject* WeaponData, UAnimBlueprint* Blueprint)`
- 321: `FString UWeaponAimAlignmentMigrationLibrary::InspectAlignment(UAnimBlueprint* Blueprint, UBlueprint* WeaponBlueprint, UObject* WeaponData)`

## WeaponAimAlignmentMigrationLibrary.h

분류: Editor 도구. [소스](../../Source/ShootingArenaEditorTools/Private/WeaponAimAlignmentMigrationLibrary.h)

- 7: `class UAnimBlueprint;`
- 8: `class UBlueprint;`
- 9: `class UWeaponAimAlignmentDataAsset;`
- 12: `class UWeaponAimAlignmentMigrationLibrary : public UBlueprintFunctionLibrary`
