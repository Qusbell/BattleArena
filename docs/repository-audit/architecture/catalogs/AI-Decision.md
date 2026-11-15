# AI.Decision 객체 카탈로그

이 그룹은 경로 키워드에 따른 **분류 추론**입니다. 소유권·실행 여부를 증명하지 않습니다.
아래 역할 요약은 클래스 선언 주석, 상속 태그와 실제 그래프 이름을 근거로 하며, 이름만으로 동작을 확정하지 않습니다.

## C++ 선언

| 타입 | 역할 근거 / 헤더 함수 검색어 | 선언 위치 |
|---|---|---|
| `FBlackboardUsageScanner` | 선언 종류: class; 상속: 없음; 업무 역할 미서술 / 헤더 검색어: Run | [Plugins/BlackboardUsageScanner/Source/BlackboardUsageScanner/Public/BlackboardUsageScanner.h:5](../../../../Plugins/BlackboardUsageScanner/Source/BlackboardUsageScanner/Public/BlackboardUsageScanner.h) |
| `AMyAIController` | 선언 종류: class; 상속: public AAIController; 업무 역할 미서술 / 헤더 검색어: ActivateOrbitalLook, ClearOrbitalLook, ConfigureOrbitalLook, GetOrbitalLookDebug, OnPossess, OnUnPossess, RequestDamageLook, ResetDamageLook | [Source/ShootingArena/MyAIController.h:8](../../../../Source/ShootingArena/MyAIController.h) |
| `FQuakeAIFireCycleState` | 선언 종류: struct; 상속: 없음; 업무 역할 미서술 / 헤더 검색어: BeginNext, IsExpired | [Source/ShootingArena/Private/AI/QuakeAIFireCycleState.h:6](../../../../Source/ShootingArena/Private/AI/QuakeAIFireCycleState.h) |
| `UBTService_SelectTarget_R` | 선언 종류: class; 상속: public UBTService; 업무 역할 미서술 / 헤더 검색어: TickNode | [Source/ShootingArena/Public/AI/BTService_SelectTarget_R.h:8](../../../../Source/ShootingArena/Public/AI/BTService_SelectTarget_R.h) |
| `UQuakeAIFireCycleComponent` | Owns one fire/rest cycle per combat, rather than per BT service activation. / 헤더 검색어: ActivateFireGateForController, BeginNextPhase, BeginPlay, Deactivate, EndPlay, GetPhaseEndTime, InitializeForPawn, IsCombatCycleActive | [Source/ShootingArena/Public/AI/QuakeAIFireCycleComponent.h:13](../../../../Source/ShootingArena/Public/AI/QuakeAIFireCycleComponent.h) |
| `AQuakeTargetSelectionAIController` | 선언 종류: class; 상속: public AMyAIController; 업무 역할 미서술 / 헤더 검색어: ApplySelectedTarget, GetAcceptedTargetClass, GetCharacterTargets, GetCharacterTargets_Implementation, HandlePossessedPawnDamaged, HandleTargetPerceptionUpdated, IsCharacterDead, OnPossess | [Source/ShootingArena/Public/AI/QuakeTargetSelectionAIController.h:12](../../../../Source/ShootingArena/Public/AI/QuakeTargetSelectionAIController.h) |

헤더 검색어는 같은 헤더에 선언된 다른 타입의 함수도 포함할 수 있는 탐색 보조 자료입니다.

## Blueprint 정의

`BP_QuakeAIController2`의 `Set LTG Timer`는 0.2초 반복 주기를 사용합니다. 타이머를 생성할 때 명시적인 첫 호출 오프셋 ±0.1초로 평가가 같은 프레임에 몰리는 빈도를 낮춥니다. `Get Goal Weight`는 기존 양수 점수 조건을 경로 거리 조회 전에 검사합니다. 무효 후보의 반환값은 -1이며, 양수 후보는 기존 PathLink 거리로 감쇠합니다.

`Update Goal Valid`는 LTG와 STG에 공통으로 적용합니다. 현재 목표가 획득된 `BP_BuffItem`이면 `MyOwner`가 자신의 Pawn일 때 곧바로 블랙보드의 목표 액터와 위치를 비웁니다. 다른 캐릭터가 획득한 버프는 저장한 위치를 향한 시야가 트일 때 해제합니다. `Get Goal Weight`의 Buff 분기는 이미 획득된 `BP_BuffItem`을 -1로 평가합니다. 다른 아이템에는 기존 시야, 액터 유효성 및 가중치 규칙을 적용합니다. `Lvl_PortalMap`의 `Play As Client`에서 실제 Overlap 획득과 목표 갱신 타이머, 시야별 LTG/STG 판정, 새 후보 제외를 검증했습니다.

| 객체 경로 | 상속 / 정의 역할 | 확인한 그래프 |
|---|---|---|
| `/Game/QuakeLike_1_0/AI/AIC/BP_QuakeAIController2.BP_QuakeAIController2` | Blueprint; /Script/CoreUObject.Class'/Script/ShootingArena.QuakeTargetSelectionAIController' | Can Arrive In Time, Clear Target Lost Timer, Debug, Debug_Chase Points, Debug_LTG, Debug_Perceived, Debug_STGtLine, EventGraph, Filtering STG, Forget Blackboard Info, Get Aggression Goal Weight, Get Aggression Score, Get Attacker Pawn, Get Best Goal, Get Goal Weight, Get Height Score, Get SelfPreservation Goal Weight, Get Survive Score, Get Weapon Score, Is Shorter Then LTG, Pop Chase Point, Push Chase Point, Set Current Target, Set LTG, Set LTG Timer, Set STG, SetupCharacteristic, Start Target Lost Timer, Target Lost, Update Goal Valid, UserConstructionScript |
| `/Game/QuakeLike_1_0/AI/BP/BPC_AIDataCache.BPC_AIDataCache` | Blueprint; /Script/CoreUObject.Class'/Script/Engine.ActorComponent' | EventGraph, GetCharacteristicRow, GetItemWeightRow, GetWeaponChangeRow |
| `/Game/QuakeLike_1_0/AI/BP/BPC_AIWeaponSelection.BPC_AIWeaponSelection` | Blueprint; /Script/CoreUObject.Class'/Script/Engine.ActorComponent' | EventGraph, GetBestWeaponScore, InitializeWeaponSelection, SetBestWeaponScoreCached |
| `/Game/QuakeLike_1_0/AI/BP/BPC_AIWeaponSelection1.BPC_AIWeaponSelection1` | Blueprint; /Script/CoreUObject.Class'/Script/Engine.ActorComponent' | EventGraph, GetBestWeaponScore, InitializeWeaponSelection |
| `/Game/QuakeLike_1_0/AI/BP/BPC_GoalWeightCalculator.BPC_GoalWeightCalculator` | Blueprint; /Script/CoreUObject.Class'/Script/Engine.ActorComponent' | CalcFuzzyWeight, EventGraph |
| `/Game/QuakeLike_1_0/AI/BP/BPC_PointsTourist.BPC_PointsTourist` | Blueprint; /Script/CoreUObject.Class'/Script/Engine.ActorComponent' | EventGraph, GetCloser, SelectNextPoint |
| `/Game/QuakeLike_1_0/AI/BP/BPC_VariousScore.BPC_VariousScore` | Blueprint; /Script/CoreUObject.Class'/Script/Engine.ActorComponent' | EventGraph, GetHeightScore, GetSurviveScore, GetWeaponScore |
| `/Game/QuakeLike_1_0/AI/BP/BP_DroneTarget.BP_DroneTarget` | Blueprint; /Script/CoreUObject.Class'/Script/Engine.Actor' | EventGraph, UserConstructionScript |
| `/Game/QuakeLike_1_0/AI/BP/BP_OrbitalTargetDrone.BP_OrbitalTargetDrone` | Blueprint; /Script/CoreUObject.Class'/Script/Engine.Actor' | EventGraph, Get Owner View Location, Get Rotation To Target, Get Selected Target, Is Rotation In Threshold, UserConstructionScript |
| `/Game/QuakeLike_1_0/AI/BP/BP_TourPoint.BP_TourPoint` | Blueprint; /Script/CoreUObject.Class'/Script/Engine.Actor' | EventGraph, UserConstructionScript |
| `/Game/QuakeLike_1_0/AI/BTD/BTD_FirstTime.BTD_FirstTime` | Blueprint; /Script/CoreUObject.Class'/Script/AIModule.BTDecorator_BlueprintBase' | EventGraph, PerformConditionCheck |
| `/Game/QuakeLike_1_0/AI/BTD/BTD_IsAlive.BTD_IsAlive` | Blueprint; /Script/CoreUObject.Class'/Script/AIModule.BTDecorator_BlueprintBase' | EventGraph, PerformConditionCheckAI |
| `/Game/QuakeLike_1_0/AI/BTS/BTS_Test_UpdateDroneLocation.BTS_Test_UpdateDroneLocation` | Blueprint; /Script/CoreUObject.Class'/Script/AIModule.BTService_BlueprintBase' | Clear Offset, EventGraph, Get Enemy, Get Next Drone Location, Select Offset, Update Offset |
| `/Game/QuakeLike_1_0/AI/BTS/BTS_UpdateDroneLocation.BTS_UpdateDroneLocation` | Blueprint; /Script/CoreUObject.Class'/Script/AIModule.BTService_BlueprintBase' | EventGraph |
| `/Game/QuakeLike_1_0/AI/BTT/BTTask_CompleteTarget.BTTask_CompleteTarget` | Blueprint; /Script/CoreUObject.Class'/Script/AIModule.BTTask_BlueprintBase' | EventGraph |
| `/Game/QuakeLike_1_0/AI/BTT/BTTask_EvaluateRetreatExit.BTTask_EvaluateRetreatExit` | Blueprint; /Script/CoreUObject.Class'/Script/AIModule.BTTask_BlueprintBase' | EventGraph |
| `/Game/QuakeLike_1_0/AI/BTT/BTTask_MoveToTarget_HBC.BTTask_MoveToTarget_HBC` | Blueprint; /Script/CoreUObject.Class'/Script/AIModule.BTTask_BlueprintBase' | EventGraph |
| `/Game/QuakeLike_1_0/AI/BTT/BTTask_SetAIState.BTTask_SetAIState` | Blueprint; /Script/CoreUObject.Class'/Script/AIModule.BTTask_BlueprintBase' | EventGraph |
| `/Game/QuakeLike_1_0/AI/BTT/BTTask_TryUse.BTTask_TryUse` | Blueprint; /Script/CoreUObject.Class'/Script/AIModule.BTTask_BlueprintBase' | EventGraph |
| `/Game/QuakeLike_1_0/AI/BTT/BTTask_Unstuck.BTTask_Unstuck` | Blueprint; /Script/CoreUObject.Class'/Script/AIModule.BTTask_BlueprintBase' | EventGraph |
| `/Game/QuakeLike_1_0/AI/EQS/EQC_Enemy.EQC_Enemy` | Blueprint; /Script/CoreUObject.Class'/Script/AIModule.EnvQueryContext_BlueprintBase' | EventGraph, ProvideSingleActor |
| `/Game/QuakeLike_1_0/AI/EQS/EQC_PlayerForwardAxis.EQC_PlayerForwardAxis` | Blueprint; /Script/CoreUObject.Class'/Script/AIModule.EnvQueryContext_BlueprintBase' | EventGraph, ProvideLocationsSet |
| `/Game/QuakeLike_1_0/AI/EQS/EQC_RetreatAnchor.EQC_RetreatAnchor` | Blueprint; /Script/CoreUObject.Class'/Script/AIModule.EnvQueryContext_BlueprintBase' | EventGraph, ProvideSingleLocation |
| `/Game/QuakeLike_1_0/AI/EQS/EQS_TestingPawn.EQS_TestingPawn` | Blueprint; /Script/CoreUObject.Class'/Script/AIModule.EQSTestingPawn' | EventGraph, UserConstructionScript |
| `/Game/QuakeLike_1_0/AI/Hiding/EQC_Target.EQC_Target` | Blueprint; /Script/CoreUObject.Class'/Script/AIModule.EnvQueryContext_BlueprintBase' | EventGraph, ProvideSingleActor |
| `/Game/QuakeLike_1_0/AI/Interface/BPI_AIStateControl.BPI_AIStateControl` | Blueprint; /Script/CoreUObject.Class'/Script/CoreUObject.Interface' | Request Set AI State |
| `/Game/QuakeLike_1_0/AI/Interface/BPI_AITargetable.BPI_AITargetable` | Blueprint; /Script/CoreUObject.Class'/Script/CoreUObject.Interface' | GetData |
| `/Game/QuakeLike_1_0/AI/Interface/BPI_AI_Setting.BPI_AI_Setting` | Blueprint; /Script/CoreUObject.Class'/Script/CoreUObject.Interface' | Get Characteristic, GetAISetting, Set Proper Combat Distance, SetAISetting, Try Get Aggression Score |
| `/Game/QuakeLike_1_0/AI/Interface/BPI_ChaserInfo.BPI_ChaserInfo` | Blueprint; /Script/CoreUObject.Class'/Script/CoreUObject.Interface' | CompleteTarget, GetCharacterTargets, GetChaserInventory, GetChaserLife, GetChaserLocation, GetChaserSpeed, GetCurrentTargetInfo |
| `/Game/QuakeLike_1_0/AI/Interface/BPI_GetTargetDrone.BPI_GetTargetDrone` | Blueprint; /Script/CoreUObject.Class'/Script/CoreUObject.Interface' | GetTargetDrone |
| `/Game/QuakeLike_1_0/AI/Interface/BPI_TourPoint.BPI_TourPoint` | Blueprint; /Script/CoreUObject.Class'/Script/CoreUObject.Interface' | GetVisitCount, Spotting |
| `/Game/QuakeLike_1_0/AI/Link/BP_LinkTargetPoint.BP_LinkTargetPoint` | Blueprint; /Script/CoreUObject.Class'/Script/Engine.Actor' | Attatch To Ground, EventGraph, UserConstructionScript |
| `/Game/QuakeLike_1_0/AI/Refactoring_BT/BTD/BTD_CheckAttackSkill_R.BTD_CheckAttackSkill_R` | Blueprint; /Script/CoreUObject.Class'/Script/AIModule.BTDecorator_BlueprintBase' | EventGraph, PerformConditionCheckAI |
| `/Game/QuakeLike_1_0/AI/Refactoring_BT/BTD/BTD_CheckGoalValid.BTD_CheckGoalValid` | Blueprint; /Script/CoreUObject.Class'/Script/AIModule.BTDecorator_BlueprintBase' | EventGraph, PerformConditionCheckAI |
| `/Game/QuakeLike_1_0/AI/Refactoring_BT/BTD/BTD_CheckRange_R.BTD_CheckRange_R` | Blueprint; /Script/CoreUObject.Class'/Script/AIModule.BTDecorator_BlueprintBase' | EventGraph, PerformConditionCheckAI |
| `/Game/QuakeLike_1_0/AI/Refactoring_BT/BTD/BTD_HasClearLine_R.BTD_HasClearLine_R` | Blueprint; /Script/CoreUObject.Class'/Script/AIModule.BTDecorator_BlueprintBase' | EventGraph, Get Target As Actor, PerformConditionCheckAI |
| `/Game/QuakeLike_1_0/AI/Refactoring_BT/BTD/BTD_InProperDistance.BTD_InProperDistance` | Blueprint; /Script/CoreUObject.Class'/Script/AIModule.BTDecorator_BlueprintBase' | EventGraph, PerformConditionCheckAI |
| `/Game/QuakeLike_1_0/AI/Refactoring_BT/BTD/BTD_IsLowAggressionScore_R.BTD_IsLowAggressionScore_R` | Blueprint; /Script/CoreUObject.Class'/Script/AIModule.BTDecorator_BlueprintBase' | EventGraph, PerformConditionCheckAI |
| `/Game/QuakeLike_1_0/AI/Refactoring_BT/BTD/BTD_IsReacting.BTD_IsReacting` | Blueprint; /Script/CoreUObject.Class'/Script/AIModule.BTDecorator_BlueprintBase' | EventGraph, PerformConditionCheckAI |
| `/Game/QuakeLike_1_0/AI/Refactoring_BT/BTD/BTD_IsTargetInRetreatDirection.BTD_IsTargetInRetreatDirection` | Blueprint; /Script/CoreUObject.Class'/Script/AIModule.BTDecorator_BlueprintBase' | EventGraph, PerformConditionCheckAI |
| `/Game/QuakeLike_1_0/AI/Refactoring_BT/BTD/BTD_IsWithinFireAngle_R.BTD_IsWithinFireAngle_R` | Blueprint; /Script/CoreUObject.Class'/Script/AIModule.BTDecorator_BlueprintBase' | EventGraph, PerformConditionCheckAI |
| `/Game/QuakeLike_1_0/AI/Refactoring_BT/BTD/BTD_StuckCheck_R.BTD_StuckCheck_R` | Blueprint; /Script/CoreUObject.Class'/Script/AIModule.BTDecorator_BlueprintBase' | EventGraph, PerformConditionCheckAI |
| `/Game/QuakeLike_1_0/AI/Refactoring_BT/BTS/BTS_AccumulateReactionTime.BTS_AccumulateReactionTime` | Blueprint; /Script/CoreUObject.Class'/Script/AIModule.BTService_BlueprintBase' | EventGraph |
| `/Game/QuakeLike_1_0/AI/Refactoring_BT/BTS/BTS_OrbitalFocus_R.BTS_OrbitalFocus_R` | Blueprint; /Script/CoreUObject.Class'/Script/AIModule.BTService_BlueprintBase' | EventGraph |
| `/Game/QuakeLike_1_0/AI/Refactoring_BT/BTS/BTS_Print_Debug.BTS_Print_Debug` | Blueprint; /Script/CoreUObject.Class'/Script/AIModule.BTService_BlueprintBase' | EventGraph |
| `/Game/QuakeLike_1_0/AI/Refactoring_BT/BTS/BTS_RandomJump_R.BTS_RandomJump_R` | Blueprint; /Script/CoreUObject.Class'/Script/AIModule.BTService_BlueprintBase' | EventGraph, Jump |
| `/Game/QuakeLike_1_0/AI/Refactoring_BT/BTS/BTS_TryUseWeapon_R.BTS_TryUseWeapon_R` | Blueprint; /Script/CoreUObject.Class'/Script/AIModule.BTService_BlueprintBase' | EventGraph |
| `/Game/QuakeLike_1_0/AI/Refactoring_BT/BTT/BTT_ClearKey_R.BTT_ClearKey_R` | Blueprint; /Script/CoreUObject.Class'/Script/AIModule.BTTask_BlueprintBase' | EventGraph |
| `/Game/QuakeLike_1_0/AI/Refactoring_BT/BTT/BTT_DebugState_R.BTT_DebugState_R` | Blueprint; /Script/CoreUObject.Class'/Script/AIModule.BTTask_BlueprintBase' | EventGraph |
| `/Game/QuakeLike_1_0/AI/Refactoring_BT/BTT/BTT_ForgetEnemy_R.BTT_ForgetEnemy_R` | Blueprint; /Script/CoreUObject.Class'/Script/AIModule.BTTask_BlueprintBase' | EventGraph |
| `/Game/QuakeLike_1_0/AI/Refactoring_BT/BTT/BTT_ReactionTime_R.BTT_ReactionTime_R` | Blueprint; /Script/CoreUObject.Class'/Script/AIModule.BTTask_BlueprintBase' | EventGraph |
| `/Game/QuakeLike_1_0/AI/Refactoring_BT/BTT/BTT_ResetReactionTime.BTT_ResetReactionTime` | Blueprint; /Script/CoreUObject.Class'/Script/AIModule.BTTask_BlueprintBase' | EventGraph |
| `/Game/QuakeLike_1_0/AI/Refactoring_BT/BTT/BTT_UpdateLocationByActor.BTT_UpdateLocationByActor` | Blueprint; /Script/CoreUObject.Class'/Script/AIModule.BTTask_BlueprintBase' | EventGraph |
| `/Game/QuakeLike_1_0/Controller/BP_QuakeAIController.BP_QuakeAIController` | Blueprint; /Script/CoreUObject.Class'/Script/ShootingArena.MyAIController' | AddTarget, Debug, Debug_TargetLine, EventGraph, RemoveTarget, UserConstructionScript |
| `/Game/QuakeLike_1_0/Test/YesItWillBeNotEasy/BP_AIController_Test_NonNavMesh.BP_AIController_Test_NonNavMesh` | Blueprint; /Script/CoreUObject.Class'/Script/ShootingArena.MyAIController' | EventGraph, UserConstructionScript |
| `/Game/QuakeLike_1_0/Test/YesItWillBeNotEasy/BP_AIController_Test_PantyRun.BP_AIController_Test_PantyRun` | Blueprint; /Script/CoreUObject.Class'/Script/ShootingArena.MyAIController' | EventGraph, UserConstructionScript |
| `/Game/QuakeLike_Base/HBC/Controller/BP_ShooterAIController_HBC.BP_ShooterAIController_HBC` | Blueprint; /Script/CoreUObject.Class'/Script/AIModule.AIController' | Debug Compare Weight, Debug Log, Debug Sensing Targets Weight, Debug Target Stack Log, EventGraph, Filter Path Targets, Get All Items of Pawn, Get Debug Key, Get Health of Pawn, Get Huge Weight Actor, Get Long Term Target, Get Now Item Usable Count of Pawn, Get Plate of Pawn, Get Safe Rate, Get Target Tag, Get Target Weight, Get Weight by Distance, Init Pawn Info, Is Combat as Target Tag, Is Path Efficient, Peek Target Stack, Pop Target Stack, Select New Long Term Target, Set BlackBoard Bool, TryClearEvent, TryRegisterEvent, TryUnRegisterEvent, Update BlackBoard Target, UpdateHealth, UserConstructionScript |
| `/Game/Variant_Shooter/Blueprints/AI/BP_ShooterAIController.BP_ShooterAIController` | Blueprint; /Script/CoreUObject.Class'/Script/AIModule.AIController' | Construct Bullet Transform, EventGraph, UserConstructionScript |
| `/Game/Variant_Shooter/Blueprints/AI/BP_ShooterNPC.BP_ShooterNPC` | Blueprint; /Script/Engine.BlueprintGeneratedClass'/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter.BP_FirstPersonCharacter_C' | EventGraph, Has Weapon, UserConstructionScript |
| `/Game/Variant_Shooter/Blueprints/AI/EQS/EnvQueryContext_Target.EnvQueryContext_Target` | Blueprint; /Script/CoreUObject.Class'/Script/AIModule.EnvQueryContext_BlueprintBase' | EventGraph, ProvideSingleActor |
| `/Game/Variant_Shooter/Blueprints/AI/StateTree/StateTreeCondition_HasLineOfSightToTarget.StateTreeCondition_HasLineOfSightToTarget` | Blueprint; /Script/CoreUObject.Class'/Script/StateTreeModule.StateTreeConditionBlueprintBase' | EventGraph, ReceiveGetDescription, ReceiveTestCondition |
| `/Game/Variant_Shooter/Blueprints/AI/StateTree/StateTreeTask_FaceActor.StateTreeTask_FaceActor` | Blueprint; /Script/CoreUObject.Class'/Script/StateTreeModule.StateTreeTaskBlueprintBase' | EventGraph |
| `/Game/Variant_Shooter/Blueprints/AI/StateTree/StateTreeTask_FaceLocation.StateTreeTask_FaceLocation` | Blueprint; /Script/CoreUObject.Class'/Script/StateTreeModule.StateTreeTaskBlueprintBase' | EventGraph |
| `/Game/Variant_Shooter/Blueprints/AI/StateTree/StateTreeTask_SenseEnemies.StateTreeTask_SenseEnemies` | Blueprint; /Script/CoreUObject.Class'/Script/StateTreeModule.StateTreeTaskBlueprintBase' | EventGraph, Has Direct Line of Sight to Stimulus |
| `/Game/Variant_Shooter/Blueprints/AI/StateTree/StateTreeTask_SetRandomFloat.StateTreeTask_SetRandomFloat` | Blueprint; /Script/CoreUObject.Class'/Script/StateTreeModule.StateTreeTaskBlueprintBase' | EventGraph, ReceiveGetDescription |
| `/Game/Variant_Shooter/Blueprints/AI/StateTree/StateTreeTask_ShootAtTarget.StateTreeTask_ShootAtTarget` | Blueprint; /Script/CoreUObject.Class'/Script/StateTreeModule.StateTreeTaskBlueprintBase' | EventGraph, ReceiveGetDescription |

호출 노드·변수·컴포넌트 상세는 로컬 실행 폴더의 `blueprint-catalog.json`과 `graphs/`에 있습니다.
JSON 조회기 외 그래프는 `graph-text/`의 Unreal 텍스트 내보내기로 보충했으며, 컴파일 파생 그래프는 별도 제외 목록에 기록합니다.
