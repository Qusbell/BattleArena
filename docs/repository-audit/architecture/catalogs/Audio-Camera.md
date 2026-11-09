# Audio.Camera 객체 카탈로그

이 그룹은 경로 키워드에 따른 **분류 추론**입니다. 소유권·실행 여부를 증명하지 않습니다.
아래 역할 요약은 클래스 선언 주석, 상속 태그와 실제 그래프 이름을 근거로 하며, 이름만으로 동작을 확정하지 않습니다.

## C++ 선언

| 타입 | 역할 근거 / 헤더 함수 검색어 | 선언 위치 |
|---|---|---|
| `FBGMTrack` | BGM 한 곡과 곡별 재생 보정값입니다. / 헤더 검색어: IsConfigured, IsNull | [Source/ShootingArena/Public/Audio/BGMSettingsDataAsset.h:13](../../../../Source/ShootingArena/Public/Audio/BGMSettingsDataAsset.h) |
| `UBGMSettingsDataAsset` | 기획자가 공용 BGM, 맵별 BGM 및 Fade 값을 한 곳에서 교체하는 설정 에셋입니다. / 헤더 검색어: IsConfigured, IsNull | [Source/ShootingArena/Public/Audio/BGMSettingsDataAsset.h:40](../../../../Source/ShootingArena/Public/Audio/BGMSettingsDataAsset.h) |
| `EBGMState` | 선언 종류: enum class; 상속: uint8; 업무 역할 미서술 / 헤더 검색어: ApplyVolumeOverride, BeginLoadingBGM, CanPlayAudio, Deinitialize, EndLoadingBGM, GetBGMVolume, GetBGMVolumePercent, GetCurrentMapName | [Source/ShootingArena/Public/Audio/BGMSubsystem.h:15](../../../../Source/ShootingArena/Public/Audio/BGMSubsystem.h) |
| `UBGMSubsystem` | 레벨 전환을 넘어 유지되는 클라이언트 전용 BGM 재생/전환 관리자입니다. / 헤더 검색어: ApplyVolumeOverride, BeginLoadingBGM, CanPlayAudio, Deinitialize, EndLoadingBGM, GetBGMVolume, GetBGMVolumePercent, GetCurrentMapName | [Source/ShootingArena/Public/Audio/BGMSubsystem.h:27](../../../../Source/ShootingArena/Public/Audio/BGMSubsystem.h) |
| `ECharacterSpawnSoundPhase` | 선언 종류: enum class; 상속: uint8; 업무 역할 미서술 / 헤더 검색어: ClientPlaySpawnSounds, DispatchSpawnSounds, EndPlay, HasReadyInitialConnection, IsLocalInitialPlaybackReady, MulticastPlaySpawnSounds, PlayLocalEntry, PlayLocalSpawnSounds | [Source/ShootingArena/Public/Audio/CharacterSpawnSoundComponent.h:15](../../../../Source/ShootingArena/Public/Audio/CharacterSpawnSoundComponent.h) |
| `ECharacterSpawnSoundSpace` | 선언 종류: enum class; 상속: uint8; 업무 역할 미서술 / 헤더 검색어: ClientPlaySpawnSounds, DispatchSpawnSounds, EndPlay, HasReadyInitialConnection, IsLocalInitialPlaybackReady, MulticastPlaySpawnSounds, PlayLocalEntry, PlayLocalSpawnSounds | [Source/ShootingArena/Public/Audio/CharacterSpawnSoundComponent.h:22](../../../../Source/ShootingArena/Public/Audio/CharacterSpawnSoundComponent.h) |
| `FCharacterSpawnSoundEntry` | A single one-shot sound scheduled relative to a successful spawn. / 헤더 검색어: ClientPlaySpawnSounds, DispatchSpawnSounds, EndPlay, HasReadyInitialConnection, IsLocalInitialPlaybackReady, MulticastPlaySpawnSounds, PlayLocalEntry, PlayLocalSpawnSounds | [Source/ShootingArena/Public/Audio/CharacterSpawnSoundComponent.h:30](../../../../Source/ShootingArena/Public/Audio/CharacterSpawnSoundComponent.h) |
| `UCharacterSpawnSoundComponent` | Character-specific spawn sounds. Add to the base character Blueprint once. / 헤더 검색어: ClientPlaySpawnSounds, DispatchSpawnSounds, EndPlay, HasReadyInitialConnection, IsLocalInitialPlaybackReady, MulticastPlaySpawnSounds, PlayLocalEntry, PlayLocalSpawnSounds | [Source/ShootingArena/Public/Audio/CharacterSpawnSoundComponent.h:57](../../../../Source/ShootingArena/Public/Audio/CharacterSpawnSoundComponent.h) |
| `FKillStreakVoiceTier` | 킬 스트릭 기준 횟수와 해당 단계에서 재생할 사운드 목록입니다. / 헤더 검색어:  | [Source/ShootingArena/Public/Audio/KillStreakSettingsDataAsset.h:11](../../../../Source/ShootingArena/Public/Audio/KillStreakSettingsDataAsset.h) |
| `UKillStreakSettingsDataAsset` | 킬 이벤트 배치, 보이스 재생 및 최근 가해자 귀속 판정에 공용으로 사용하는 설정입니다. / 헤더 검색어:  | [Source/ShootingArena/Public/Audio/KillStreakSettingsDataAsset.h:26](../../../../Source/ShootingArena/Public/Audio/KillStreakSettingsDataAsset.h) |
| `UKillStreakVoiceComponent` | Batches credited kill events, tracks the current streak, and plays its voice for the owning player. / 헤더 검색어: BeginPlay, ClearPendingVoiceLocally, ClientClearPendingKillStreakVoice, ClientPlayKillStreakVoice, DispatchVoiceToOwner, EndPlay, FindTierForKillCount, GetCurrentStreakCount | [Source/ShootingArena/Public/Audio/KillStreakVoiceComponent.h:15](../../../../Source/ShootingArena/Public/Audio/KillStreakVoiceComponent.h) |
| `ADeathCamActor` | 실제 DeathCam 시점을 담당하는 CameraActor입니다. 사망 위치 주변의 제한된 범위에서 벽을 따라 이동하며 사망 위치와 Killer를 함께 담습니다. 네트워크 / ViewTarget / Spawn / Destroy는 DeathCamComponent가 담당합니다. / 헤더 검색어: AdvanceCameraSearch, ApplyKillerHighlight, ApplyKillerHighlightPostProcess, BeginCameraSearch, CacheSettings, CalculateBaseCameraLocation, CalculateFramingRotation, CalculateKillerDirection | [Source/ShootingArena/Public/Camera/DeathCamActor.h:20](../../../../Source/ShootingArena/Public/Camera/DeathCamActor.h) |
| `FHighlightComponentState` | Killer Highlight 종료 시 원래 CustomDepth 상태로 되돌리기 위한 백업입니다. / 헤더 검색어: AdvanceCameraSearch, ApplyKillerHighlight, ApplyKillerHighlightPostProcess, BeginCameraSearch, CacheSettings, CalculateBaseCameraLocation, CalculateFramingRotation, CalculateKillerDirection | [Source/ShootingArena/Public/Camera/DeathCamActor.h:140](../../../../Source/ShootingArena/Public/Camera/DeathCamActor.h) |
| `UDeathCamComponent` | 기존 PlayerController에 붙여 사용하는 DeathCam 전용 Component입니다. 네트워크 경계, DeathCam Actor 생명주기, ViewTarget 소유권만 담당합니다. 기존 Controller / Life / Respawn 시스템의 로직은 이 Component가 수정하지 않습니다. / 헤더 검색어: BeginPlay, ClientStartDeathCam, ClientStopDeathCam, DestroyDeathCamImmediately, EndPlay, FinishStopDeathCam, GetDeathCamActor, GetOwningPlayerController | [Source/ShootingArena/Public/Camera/DeathCamComponent.h:20](../../../../Source/ShootingArena/Public/Camera/DeathCamComponent.h) |
| `EDeathCamKillerHighlightType` | DeathCam에서 Killer를 강조하는 표현 방식입니다. Outline : 장애물에 가려진 Killer의 외곽선만 표시합니다. Fill : 장애물에 가려진 Killer 실루엣 전체를 표시합니다. / 헤더 검색어: GetKillerHighlightMaterial | [Source/ShootingArena/Public/Camera/DeathCamDataAsset.h:15](../../../../Source/ShootingArena/Public/Camera/DeathCamDataAsset.h) |
| `UDeathCamDataAsset` | DeathCam에서 디자이너가 조절할 값만 보관하는 DataAsset입니다. 게임 로직 / 네트워크 로직은 이 클래스에 넣지 않습니다. Killer Highlight는 실제 Material을 직접 교체하는 대신 killerHighlightType에서 Outline / Fill 중 하나를 선택해 사용합니다. / 헤더 검색어: GetKillerHighlightMaterial | [Source/ShootingArena/Public/Camera/DeathCamDataAsset.h:29](../../../../Source/ShootingArena/Public/Camera/DeathCamDataAsset.h) |
| `FDebugCameraOccludedMesh` | 선언 종류: struct; 상속: 없음; 업무 역할 미서술 / 헤더 검색어: EndPlay, FVector, RestoreOcclusion, UpdateOcclusion | [Source/ShootingArena/Public/Camera/DebugCameraOcclusionComponent.h:13](../../../../Source/ShootingArena/Public/Camera/DebugCameraOcclusionComponent.h) |
| `UDebugCameraOcclusionComponent` | Local-only visibility aid for BP_DebugFollowCamera. The camera drives this from its Tick. / 헤더 검색어: EndPlay, FVector, RestoreOcclusion, UpdateOcclusion | [Source/ShootingArena/Public/Camera/DebugCameraOcclusionComponent.h:26](../../../../Source/ShootingArena/Public/Camera/DebugCameraOcclusionComponent.h) |
| `UShooterRevengeComponent` | Presents BP_QuakePlayerState.RevengeTarget and transports it to the owning client. / 헤더 검색어: BeginPlay, ClearRevengeTarget, DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam, EndPlay, GetLifetimeReplicatedProps, GetRevengeTargetPlayerState, IsRevengeTarget, OnRep_TargetPlayerState | [Source/ShootingArena/Public/Camera/ShooterRevengeComponent.h:14](../../../../Source/ShootingArena/Public/Camera/ShooterRevengeComponent.h) |

헤더 검색어는 같은 헤더에 선언된 다른 타입의 함수도 포함할 수 있는 탐색 보조 자료입니다.

## Blueprint 정의

| 객체 경로 | 상속 / 정의 역할 | 확인한 그래프 |
|---|---|---|
| `/Game/CompShooterHUD/DemoContent/DemoRoom/Audio/Blueprints/BPI_Interface.BPI_Interface` | Blueprint; /Script/CoreUObject.Class'/Script/CoreUObject.Interface' | 5.1 test, Alt Fire, Altdestroyed, Blueprint Spawn, Camera_Text, Exit Room, Reset Button, ShootBullet, ShootProjectile, Start_Shooting, Stop audio components, Stop_Shooting, Surface Room |
| `/Game/QuakeLike_1_0/Camera/BP_CM_FOVTest.BP_CM_FOVTest` | Blueprint; /Script/CoreUObject.Class'/Script/Engine.CameraModifier' | BlueprintModifyCamera, EventGraph |
| `/Game/QuakeLike_1_0/Camera/WBP_FOVTest.WBP_FOVTest` | WidgetBlueprint; /Script/CoreUObject.Class'/Script/UMG.UserWidget' | ApplyFOV, EventGraph |

호출 노드·변수·컴포넌트 상세는 로컬 실행 폴더의 `blueprint-catalog.json`과 `graphs/`에 있습니다.
JSON 조회기 외 그래프는 `graph-text/`의 Unreal 텍스트 내보내기로 보충했으며, 컴파일 파생 그래프는 별도 제외 목록에 기록합니다.
