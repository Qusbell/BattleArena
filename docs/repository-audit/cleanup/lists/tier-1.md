# 분류 1 전체 목록

판정 기준과 실행 검증 한계는 [삭제 후보 명세](../README.md)를 먼저 확인합니다. 이 목록은 삭제 실행 기록이 아닙니다.

| 패키지 경로 | 역할 근거 | 직접 역참조 | 간접 로직 영향 | 참조자/소스 |
|---|---|---:|---:|---|
| `/Game/Blueprints/Data/ST_MASComp` | 사용자 데이터 구조 정의 | 0 | 0 | 없음 |
| `/Game/CompShooterHUD/Blueprints/CS_HUD_Examples` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/CompShooterHUD/Blueprints/Structs/S_CrosshairInfo` | 사용자 데이터 구조 정의 | 0 | 0 | 없음 |
| `/Game/CompShooterHUD/DemoContent/DemoActors/A_AbilityButtonArea` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/CompShooterHUD/DemoContent/DemoRoom/BluePrint/BP_Audio_Trigger` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/CompShooterHUD/DemoContent/DemoRoom/BluePrint/BP_LightScultpureDynamic` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/CompShooterHUD/DemoContent/Maps/DemoMap` | 현재 진입점 밖의 World 정의 | 0 | 0 | 없음 |
| `/Game/CompShooterHUD/UI/Content/ArmorBar/Content/WB_ArmorProgress` | UI 위젯 로직 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/AI/Area/BPC_AreaRiskComponent` | Area Manager에 유닛 인식·전투 위험을 전달하는 미연결 컴포넌트 | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/AI/Area/BTS_SelectRetreatMovePosition` | Area 위험도를 이용해 후퇴 위치를 정하는 BT Service | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/AI/Area/BTT_ClearRetreatState` | 후퇴 Blackboard 상태를 초기화하는 BT Task | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/AI/Area/BTT_SelectRetreatMovePosition` | 후퇴 이동 위치를 정하는 BT Task | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/AI/BP/BPC_AIWeaponSelection1` | 무기 점수 계산·선택을 가진 별도 AI 컴포넌트 | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/AI/BP/BP_OrbitalTargetDrone` | Blueprint/설정 정의; 그래프: Get Owner View Location, Get Rotation To Target, Get Selected Target, Is Rotation In Threshold | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/AI/BTD/BTD_FirstTime` | Blueprint/설정 정의; 그래프: PerformConditionCheck | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/AI/BTT/BTTask_EvaluateRetreatExit` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/AI/BTT/BTTask_SetAIState` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/AI/BTT/BTTask_Unstuck` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/AI/EQS/EQC_PlayerForwardAxis` | Blueprint/설정 정의; 그래프: ProvideLocationsSet | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/AI/EQS/EQS_CombatPosition` | EQS 공간·상황 질의 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/AI/EQS/EQS_RetreatPosition` | EQS 공간·상황 질의 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/AI/EQS/EQS_TestingPawn` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/AI/Hiding/EQS_GetHidingLocation` | EQS 공간·상황 질의 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/AI/Interface/BPI_GetTargetDrone` | Blueprint/설정 정의; 그래프: GetTargetDrone | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/AI/Refactoring_BT/BTD/BTD_CheckGoalValid` | Blueprint/설정 정의; 그래프: PerformConditionCheckAI | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/AI/Refactoring_BT/BTD/BTD_StuckCheck_R` | Blueprint/설정 정의; 그래프: PerformConditionCheckAI | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/AI/Refactoring_BT/BTS/BTS_Print_Debug` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/AI/Refactoring_BT/BTT/BTT_ReactionTime_R` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/AI/Refactoring_BT/EQS/EQS_NearbyRandomLocation` | EQS 공간·상황 질의 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/AITest_YJS/AI_Char_YJS` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/AITest_YJS/BTS/BTS_UpdateCombatMoveActor_YJS` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/AITest_YJS/BTTask/BTT_SetChaseTarget_YJS` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/AITest_YJS/Untitled` | 현재 진입점 밖의 World 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/Animation/Steel/ABP/Steel_ABP_FP_BGH_MachineGun` | 애니메이션 상태·포즈 제어 Blueprint; 그래프: AnimGraph | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/Animation/Steel/ABP/Steel_ABP_FP_BGH_Pistol1` | 애니메이션 상태·포즈 제어 Blueprint; 그래프: AnimGraph | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/Animation/Steel/ABP/Steel_ABP_FP_BGH_Plasma` | 애니메이션 상태·포즈 제어 Blueprint; 그래프: AnimGraph | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/Animation/Steel/ABP/Steel_ABP_FP_BGH_RocketLauncher` | 애니메이션 상태·포즈 제어 Blueprint; 그래프: AnimGraph | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/Animation/Steel/ABP/Steel_ABP_FP_BGH_ShotGun` | 애니메이션 상태·포즈 제어 Blueprint; 그래프: AnimGraph | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/Animation/Steel/ABP/Steel_ABP_TP_BGH_MachineGun` | 애니메이션 상태·포즈 제어 Blueprint; 그래프: AnimGraph | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/Animation/Steel/ABP/Steel_ABP_TP_BGH_Pistol1` | 애니메이션 상태·포즈 제어 Blueprint; 그래프: AnimGraph | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/Animation/Steel/ABP/Steel_ABP_TP_BGH_Plasma` | 애니메이션 상태·포즈 제어 Blueprint; 그래프: AnimGraph | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/Animation/Steel/ABP/Steel_ABP_TP_BGH_RocketLauncher` | 애니메이션 상태·포즈 제어 Blueprint; 그래프: AnimGraph | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/Animation/Steel/ABP/Steel_ABP_TP_BGH_ShotGun` | 애니메이션 상태·포즈 제어 Blueprint; 그래프: AnimGraph | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/Assets/Biohazard/Blueprints/BP_AllAssetsPlacer` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/Assets/StarterBundle/ModularSci_Int/Blueprints/BP_SkySphere_Original` | Blueprint/설정 정의; 그래프: UpdateSunDirection | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/Assets/StarterBundle/ModularScifiHallways/Blueprints/BP_SkySphere_Original` | Blueprint/설정 정의; 그래프: UpdateSunDirection | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/Camera/BP_CM_FOVTest` | Blueprint/설정 정의; 그래프: BlueprintModifyCamera | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/Controller/BP_QuakeAIController` | 현행 AI/AIC/BP_QuakeAIController2와 별개인 Controller 구현 | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/Data/Item/DA_TestWeapon` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/Data/PDA/PDA_AIGameMode` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/GameFlow/Lobby/WBP_MultiConnect` | UI 위젯 로직 정의; 그래프: DoConnect, InitFromGameInstance, OnIpChanged, OnPortChanged | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/GameFlow/Table/DT/DT_FLobbySlot` | 구조화된 행 데이터 | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/GameFlow/Table/DT/DT_Lobby` | 구조화된 행 데이터 | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/GameFlow/Table/DT/DT_MultiLobby` | 구조화된 행 데이터 | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/MapDesign/MapGimmick/BPA_YJS_CrumblingPlatform` | Blueprint/설정 정의; 그래프: Hidden Platform, Show Platform | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/MapDesign/MapGimmick/BPA_YJS_Normal_JumpPad` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/MapDesign/MapGimmick/BPA_YJS_PressurePlate` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/MapDesign/MapGimmick/JumpPad/PDA_JumpPadSettings` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/Player/BPC_InputOption` | Blueprint/설정 정의; 그래프: Activate Input Option UI, Create Input Option UI, Get Aim Speed As Pure | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/Spawner/E_SpawnerType` | 사용자 열거형 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/Spawner/SpawnPoint/BP_FixedItemSpawnPoint` | Blueprint/설정 정의; 그래프: FindSpawnerByTable, IsAvailable | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/Test/BP_StackTester` | Blueprint/설정 정의; 그래프: SpawnActorDummy, SpawnActorDummys | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/Test/BTS_RandomJump_Test` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/Test/CollisionTest` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/Test/IamFucked` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/Test/LinkTester/LinkTesterManager` | Blueprint/설정 정의; 그래프: Debug_Route | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/Test/TagTester` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/Test/TargetDrone_Test` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/Test/TestActor` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/Test/Test_AITargetStack` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/Test/Test_Char` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/Test/Test_Inven` | Blueprint/설정 정의; 그래프: Pickup | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/Test/YesItWillBeNotEasy/BPC_WayPointLinkManager` | Blueprint/설정 정의; 그래프: Get Closest Area, Get Location Start And End, Get WayPoint | 0 | 0 | 없음 |
| `/Game/QuakeLike_1_0/Test/YesItWillBeNotEasy/E_WayPointType` | 사용자 열거형 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_Base/BGH/AIWeight/E_BGH_AITargetType` | 사용자 열거형 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_Base/BGH/AssetTest/Anim/ABP_FP_BGH_Pistol` | 애니메이션 상태·포즈 제어 Blueprint; 그래프: AnimGraph | 0 | 0 | 없음 |
| `/Game/QuakeLike_Base/BGH/AssetTest/Anim/ABP_TP_BGH_Pistol` | 애니메이션 상태·포즈 제어 Blueprint; 그래프: AnimGraph | 0 | 0 | 없음 |
| `/Game/QuakeLike_Base/BGH/BGH_JumpLink` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_Base/BGH/BGH_TargetalbeDummy` | Blueprint/설정 정의; 그래프: Set Active | 0 | 0 | 없음 |
| `/Game/QuakeLike_Base/BGH/EventTestActor` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_Base/BP_BGH_TempGameMode` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_Base/HBC/BTS/BTS_AimUpdate` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_Base/HBC/BTS/BTS_BaseAim` | Blueprint/설정 정의; 그래프: ClosePenalty, GetAimBox | 0 | 0 | 없음 |
| `/Game/QuakeLike_Base/HBC/BTS/BTS_SetFocusCustom` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_Base/HBC/BTTask/BTTask_AI_ChasePlayer_HBC` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_Base/HBC/BTTask/BTTask_AI_Shooting_HBC` | Blueprint/설정 정의; 그래프: GetItemHolder | 0 | 0 | 없음 |
| `/Game/QuakeLike_Base/HBC/BTTask/BTTask_AI_ZigZag_HBC` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_Base/HBC/BTTask/BTTask_BGH_MoveToTarget` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_Base/HBC/BTTask/BTTask_BTT_ShotPrintLog` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_Base/HBC/BTTask/BTTask_Combat` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_Base/HBC/BTTask/BTTask_Debuging` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_Base/HBC/BTTask/BTTask_ItemTour_HBC` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_Base/HBC/BTTask/BTTask_StopShooting_HBC` | Blueprint/설정 정의; 그래프: GetItemHolder | 0 | 0 | 없음 |
| `/Game/QuakeLike_Base/HBC/Controller/BP_ShooterPlayerController_HBC` | Blueprint/설정 정의; 그래프: TryClearEvent, TryRegisterEvent | 0 | 0 | 없음 |
| `/Game/QuakeLike_Base/LJH/Components/BP_AutoDoor` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_Base/LJH/Components/BP_LJH_TestSHOOT` | Blueprint/설정 정의; 그래프: Is Weapon Already Owned | 0 | 0 | 없음 |
| `/Game/QuakeLike_Base/LJH/Interfaces/BPI_ModifyHealth` | Blueprint/설정 정의; 그래프: Get CurrentHealth | 0 | 0 | 없음 |
| `/Game/QuakeLike_Base/LJH/Interfaces/BPI_ModifyPlate` | Blueprint/설정 정의; 그래프: Get CurrentPlate | 0 | 0 | 없음 |
| `/Game/QuakeLike_Base/LJH/Item/DropItem/DA_DropItem_Default` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_Base/LJH/Life/DA_Life_Default` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_Base/PJH/DataAsset/DA_PJH_WeaponBaseAsset` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_Base/PJH/ObjectPool/BP_PJH_Weapon_PlasmaGun_OP` | Blueprint/설정 정의; 그래프: Calculate Bullet Spawn Position, Spawn Bullet, Start Firing | 0 | 0 | 없음 |
| `/Game/QuakeLike_Base/PJH/Projectile/BP_PJH_Projectile_BoomBoom` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_Base/PJH/Weapon/BP_PJH_Weapon_Boom` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_Base/PJH/Weapon/BP_PJH_Weapon_ShotGun_Scan` | Blueprint/설정 정의; 그래프: ConeScan, Damage Func, Distance Damage, Firing HitScan | 0 | 0 | 없음 |
| `/Game/QuakeLike_Base/SDM/Anim/ABP_Shotgun` | 애니메이션 상태·포즈 제어 Blueprint; 그래프: AnimGraph | 0 | 0 | 없음 |
| `/Game/QuakeLike_Base/SDM/UI/Combat_UI/WBP_MaxSpeed` | UI 위젯 로직 정의; 그래프: RefreshAllWeaponSlot, RefreshAmmoGauge, ResetAllSlot | 0 | 0 | 없음 |
| `/Game/QuakeLike_Base/SDM/UI/Combat_UI/WBP_Ranking` | UI 위젯 로직 정의; 그래프: RefreshAllWeaponSlot, RefreshAmmoGauge, ResetAllSlot | 0 | 0 | 없음 |
| `/Game/QuakeLike_Base/SDM/UI/Option_UI/WBP_ESCMenu` | UI 위젯 로직 정의; 그래프: RefreshAllWeaponSlot, RefreshAmmoGauge, ResetAllSlot | 0 | 0 | 없음 |
| `/Game/QuakeLike_Base/SDM/UI/Option_UI/WBP_QuitConfirm` | UI 위젯 로직 정의; 그래프: RefreshAllWeaponSlot, RefreshAmmoGauge, ResetAllSlot | 0 | 0 | 없음 |
| `/Game/QuakeLike_Base/SDM/UI/Option_UI/WBP_Setting` | UI 위젯 로직 정의; 그래프: RefreshAllWeaponSlot, RefreshAmmoGauge, ResetAllSlot | 0 | 0 | 없음 |
| `/Game/QuakeLike_Base/SDM/UI/Option_UI/WBP_Setting_Audio` | UI 위젯 로직 정의; 그래프: RefreshAllWeaponSlot, RefreshAmmoGauge, ResetAllSlot | 0 | 0 | 없음 |
| `/Game/QuakeLike_Base/SDM/UI/Option_UI/WBP_Setting_GamePlay` | UI 위젯 로직 정의; 그래프: RefreshAllWeaponSlot, RefreshAmmoGauge, ResetAllSlot | 0 | 0 | 없음 |
| `/Game/QuakeLike_Base/SDM/UI/Option_UI/WBP_Setting_GamePlay_CrossHair` | UI 위젯 로직 정의; 그래프: RefreshAllWeaponSlot, RefreshAmmoGauge, ResetAllSlot | 0 | 0 | 없음 |
| `/Game/QuakeLike_Base/SDM/UI/Option_UI/WBP_Setting_GamePlay_KeySetting` | UI 위젯 로직 정의; 그래프: RefreshAllWeaponSlot, RefreshAmmoGauge, ResetAllSlot | 0 | 0 | 없음 |
| `/Game/QuakeLike_Base/SDM/UI/Option_UI/WBP_Setting_Graphics` | UI 위젯 로직 정의; 그래프: RefreshAllWeaponSlot, RefreshAmmoGauge, ResetAllSlot | 0 | 0 | 없음 |
| `/Game/QuakeLike_Base/YJH_에셋제작` | 현재 진입점 밖의 World 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_Base/YJS/Data/Data_YJS_SpecialMovement` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_Base/YJS/Server/BP_YJS_GameInstance` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/QuakeLike_Base/YJS/Server/LobbyLevel` | 현재 진입점 밖의 World 정의 | 0 | 0 | 없음 |
| `/Game/Sci-Fi_Weapon_Starter_Pack/Sci-Fi_Assault_Rifle_1/Demo_Scene/Map/Demo_Scene` | 현재 진입점 밖의 World 정의 | 0 | 0 | 없음 |
| `/Game/Sci-Fi_Weapon_Starter_Pack/Sci-Fi_MachineGun/Demo_Scene/Map/Demo_Scene` | 현재 진입점 밖의 World 정의 | 0 | 0 | 없음 |
| `/Game/Sci-Fi_Weapon_Starter_Pack/Sci-Fi_Shotgun/Demo_Scene/Map/Demo_Scene` | 현재 진입점 밖의 World 정의 | 0 | 0 | 없음 |
| `/Game/Sci-Fi_Weapon_Starter_Pack/Sci-Fi_Sniper_Rifle/Demo_Scene/Map/Demo_Scene` | 현재 진입점 밖의 World 정의 | 0 | 0 | 없음 |
| `/Game/SciFiRocketLauncher_AK9/Blueprints/BP_AmmoPickup` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/SciFiRocketLauncher_AK9/Blueprints/BP_RocketLauncher_AK9_GameMode` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/SciFiRocketLauncher_AK9/Blueprints/BP_ThirdPerson_Examples` | Blueprint/설정 정의 | 0 | 0 | 없음 |
| `/Game/SciFiRocketLauncher_AK9/Blueprints/WBP_Instructions` | UI 위젯 로직 정의 | 0 | 0 | 없음 |
| `/Game/Variant_Shooter/Blueprints/BP_TestShooter` | Blueprint/설정 정의; 그래프: Is Weapon Already Owned | 0 | 0 | 없음 |
