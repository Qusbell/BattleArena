# Character 객체 카탈로그

이 그룹은 경로 키워드에 따른 **분류 추론**입니다. 소유권·실행 여부를 증명하지 않습니다.
아래 역할 요약은 클래스 선언 주석, 상속 태그와 실제 그래프 이름을 근거로 하며, 이름만으로 동작을 확정하지 않습니다.

## C++ 선언

| 타입 | 역할 근거 / 헤더 함수 검색어 | 선언 위치 |
|---|---|---|
| `UShooterDisplayLibrary` | Read-only adapters for the existing Blueprint life and display-name data. / 헤더 검색어: GetPlayerStateDisplayName, GetShooterDisplayName | [Source/ShootingArena/Public/Camera/ShooterDisplayLibrary.h:12](../../../../Source/ShootingArena/Public/Camera/ShooterDisplayLibrary.h) |
| `UShooterNicknameComponent` | 선언 종류: class; 상속: public UActorComponent; 업무 역할 미서술 / 헤더 검색어: ApplyNicknameSettings, BeginPlay, ClearDisplay, ConfigureSearchSchedule, DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam, EndPlay, FVector2D, FindAimTarget | [Source/ShootingArena/Public/Camera/ShooterNicknameComponent.h:14](../../../../Source/ShootingArena/Public/Camera/ShooterNicknameComponent.h) |
| `FShooterNicknameStyle` | 선언 종류: struct; 상속: 없음; 업무 역할 미서술 / 헤더 검색어: FVector2D | [Source/ShootingArena/Public/Camera/ShooterNicknameSettings.h:12](../../../../Source/ShootingArena/Public/Camera/ShooterNicknameSettings.h) |
| `UShooterNicknameSettings` | Seconds to retain the last name after aim leaves it; zero hides immediately. / 헤더 검색어: FVector2D | [Source/ShootingArena/Public/Camera/ShooterNicknameSettings.h:37](../../../../Source/ShootingArena/Public/Camera/ShooterNicknameSettings.h) |
| `UShooterNicknameWidget` | Independent native overlay; existing crosshair and HUD Blueprint assets are untouched. / 헤더 검색어: NativePaint, RefreshDisplay | [Source/ShootingArena/Public/Camera/ShooterNicknameWidget.h:11](../../../../Source/ShootingArena/Public/Camera/ShooterNicknameWidget.h) |
| `FShooterOutlineProxy` | 선언 종류: struct; 상속: 없음; 업무 역할 미서술 / 헤더 검색어: ApplyOutlineSettings, ApplyViewVisibility, BeginPlay, ClearProxies, EndPlay, GetActiveMaterial, GetOutlineStyle, GetShooterStencil | [Source/ShootingArena/Public/Camera/ShooterOutlineComponent.h:15](../../../../Source/ShootingArena/Public/Camera/ShooterOutlineComponent.h) |
| `UShooterOutlineComponent` | Local PlayerController presentation, automatically inherited by BP_QuakePlayerController. / 헤더 검색어: ApplyOutlineSettings, ApplyViewVisibility, BeginPlay, ClearProxies, EndPlay, GetActiveMaterial, GetOutlineStyle, GetShooterStencil | [Source/ShootingArena/Public/Camera/ShooterOutlineComponent.h:28](../../../../Source/ShootingArena/Public/Camera/ShooterOutlineComponent.h) |
| `FShooterOutlineStyle` | 선언 종류: struct; 상속: 없음; 업무 역할 미서술 / 헤더 검색어:  | [Source/ShootingArena/Public/Camera/ShooterOutlineSettings.h:11](../../../../Source/ShootingArena/Public/Camera/ShooterOutlineSettings.h) |
| `UShooterOutlineSettings` | Designer defaults; runtime preferences are copied, never written back to this asset. / 헤더 검색어:  | [Source/ShootingArena/Public/Camera/ShooterOutlineSettings.h:30](../../../../Source/ShootingArena/Public/Camera/ShooterOutlineSettings.h) |
| `FCharacterDeathMeshPart` | 다중 파츠 사망 캐릭터에서 컴포넌트별로 교체할 Skeletal Mesh입니다. / 헤더 검색어: ApplyConfiguredAppearance, ApplyDeathAppearance, ApplyDeathMesh, ApplyDeathPartMaterials, ApplyLiveMesh, ApplyModelingIdInternal, BeginPlay, FindModelingRow | [Source/ShootingArena/Public/Character/CharacterAppearanceComponent.h:16](../../../../Source/ShootingArena/Public/Character/CharacterAppearanceComponent.h) |
| `FCharacterAppearanceRow` | 캐릭터 DataTable의 Row입니다. 캐릭터는 모델 자체를 직접 보관하지 않고 modelingId로 모델링 DataTable을 참조합니다. / 헤더 검색어: ApplyConfiguredAppearance, ApplyDeathAppearance, ApplyDeathMesh, ApplyDeathPartMaterials, ApplyLiveMesh, ApplyModelingIdInternal, BeginPlay, FindModelingRow | [Source/ShootingArena/Public/Character/CharacterAppearanceComponent.h:33](../../../../Source/ShootingArena/Public/Character/CharacterAppearanceComponent.h) |
| `FCharacterModelingRow` | 모델링 DataTable의 Row입니다. 일반 메시와 사망 메시를 한 레코드에 묶어, 사망 시에도 현재 선택 캐릭터의 외형을 사용합니다. / 헤더 검색어: ApplyConfiguredAppearance, ApplyDeathAppearance, ApplyDeathMesh, ApplyDeathPartMaterials, ApplyLiveMesh, ApplyModelingIdInternal, BeginPlay, FindModelingRow | [Source/ShootingArena/Public/Character/CharacterAppearanceComponent.h:46](../../../../Source/ShootingArena/Public/Character/CharacterAppearanceComponent.h) |
| `UCharacterAppearanceComponent` | 캐릭터 외형과 사망 외형을 DataTable 기반으로 동기화합니다. BP에는 이 컴포넌트를 추가하고 DataTable/메시 컴포넌트만 지정하면 BeginPlay에 자동 적용됩니다. 기존 사망 그래프는 Death Mesh Component를 표시하고 Ragdoll을 적용하는 흐름을 그대로 사용합니다. / 헤더 검색어: ApplyConfiguredAppearance, ApplyDeathAppearance, ApplyDeathMesh, ApplyDeathPartMaterials, ApplyLiveMesh, ApplyModelingIdInternal, BeginPlay, FindModelingRow | [Source/ShootingArena/Public/Character/CharacterAppearanceComponent.h:80](../../../../Source/ShootingArena/Public/Character/CharacterAppearanceComponent.h) |
| `FCharacterPoolLife` | 선언 종류: struct; 상속: 없음; 업무 역할 미서술 / 헤더 검색어: BeginDeath, BeginPlay, CapturePresentation, CompleteReconnect, DetachControllerLife, EndPlay, GetLifetimeReplicatedProps, HasBeenPossessed | [Source/ShootingArena/Public/Character/CharacterPoolComponent.h:13](../../../../Source/ShootingArena/Public/Character/CharacterPoolComponent.h) |
| `UCharacterPoolComponent` | Reusable life state. The pawn is never lent to a different participant. / 헤더 검색어: BeginDeath, BeginPlay, CapturePresentation, CompleteReconnect, DetachControllerLife, EndPlay, GetLifetimeReplicatedProps, HasBeenPossessed | [Source/ShootingArena/Public/Character/CharacterPoolComponent.h:24](../../../../Source/ShootingArena/Public/Character/CharacterPoolComponent.h) |
| `FPresentation` | 선언 종류: struct; 상속: 없음; 업무 역할 미서술 / 헤더 검색어: BeginDeath, BeginPlay, CapturePresentation, CompleteReconnect, DetachControllerLife, EndPlay, GetLifetimeReplicatedProps, HasBeenPossessed | [Source/ShootingArena/Public/Character/CharacterPoolComponent.h:49](../../../../Source/ShootingArena/Public/Character/CharacterPoolComponent.h) |
| `UCharacterPoolLibrary` | Narrow bridge preserving the existing Blueprint lifecycle entry points. / 헤더 검색어: CanPlayMovementEffects, GetWaitingPawn, ReserveWaitingPawn, ShouldApplyPossessionAppearance, StartPooledRespawn | [Source/ShootingArena/Public/Character/CharacterPoolLibrary.h:13](../../../../Source/ShootingArena/Public/Character/CharacterPoolLibrary.h) |
| `UCharacterPoolSettings` | Each participant owns one pawn; the population determines the allocation count. / 헤더 검색어:  | [Source/ShootingArena/Public/Character/CharacterPoolSettings.h:9](../../../../Source/ShootingArena/Public/Character/CharacterPoolSettings.h) |
| `FOwnedCharacterPawn` | 선언 종류: struct; 상속: 없음; 업무 역할 미서술 / 헤더 검색어: Acquire, BindController, Claim, Deinitialize, DestroyOwnedPawn, DoesSupportWorldType, FindOwnedPawn, ForgetPawn | [Source/ShootingArena/Public/Character/CharacterPoolSubsystem.h:12](../../../../Source/ShootingArena/Public/Character/CharacterPoolSubsystem.h) |
| `FCharacterPoolStats` | 선언 종류: struct; 상속: 없음; 업무 역할 미서술 / 헤더 검색어: Acquire, BindController, Claim, Deinitialize, DestroyOwnedPawn, DoesSupportWorldType, FindOwnedPawn, ForgetPawn | [Source/ShootingArena/Public/Character/CharacterPoolSubsystem.h:23](../../../../Source/ShootingArena/Public/Character/CharacterPoolSubsystem.h) |
| `UCharacterPoolSubsystem` | Server registry of participant-owned pawns and bounded reconnection reservations. / 헤더 검색어: Acquire, BindController, Claim, Deinitialize, DestroyOwnedPawn, DoesSupportWorldType, FindOwnedPawn, ForgetPawn | [Source/ShootingArena/Public/Character/CharacterPoolSubsystem.h:35](../../../../Source/ShootingArena/Public/Character/CharacterPoolSubsystem.h) |

헤더 검색어는 같은 헤더에 선언된 다른 타입의 함수도 포함할 수 있는 탐색 보조 자료입니다.

## Blueprint 정의

| 객체 경로 | 상속 / 정의 역할 | 확인한 그래프 |
|---|---|---|
| `/Game/QuakeLike_1_0/Character/BPI_Aim.BPI_Aim` | Blueprint; /Script/CoreUObject.Class'/Script/CoreUObject.Interface' | GetAim |
| `/Game/QuakeLike_1_0/Character/BPI_BodyInfo.BPI_BodyInfo` | Blueprint; /Script/CoreUObject.Class'/Script/CoreUObject.Interface' | GetBounds, GetHeart, GetMeshes, Set Input Stop Timer |
| `/Game/QuakeLike_1_0/Character/BPI_Camera.BPI_Camera` | Blueprint; /Script/CoreUObject.Class'/Script/CoreUObject.Interface' | GetCurrentFOV |
| `/Game/QuakeLike_1_0/Character/BPI_ItemHolder.BPI_ItemHolder` | Blueprint; /Script/CoreUObject.Class'/Script/CoreUObject.Interface' | GetInventory, UsingItem |
| `/Game/QuakeLike_1_0/Character/BPI_Look.BPI_Look` | Blueprint; /Script/CoreUObject.Class'/Script/CoreUObject.Interface' | Clear Look, LookAt |
| `/Game/QuakeLike_1_0/Character/BP_AIShooter.BP_AIShooter` | Blueprint; /Script/Engine.BlueprintGeneratedClass'/Game/QuakeLike_1_0/Character/BP_ShooterBase.BP_ShooterBase_C' | EventGraph, Get Movement Option, GetAim, UserConstructionScript |
| `/Game/QuakeLike_1_0/Character/BP_PlayerShooter.BP_PlayerShooter` | Blueprint; /Script/Engine.BlueprintGeneratedClass'/Game/QuakeLike_1_0/Character/BP_ShooterBase.BP_ShooterBase_C' | Aim, ApplyZoom, Block Movement Input For Duration, ClearMovementInputBlock, EventGraph, GetAim, GetMeshes, Move, UpdateBrightness, UpdateCameraFOV, UpdateHeldItemGrip, UserConstructionScript |
| `/Game/QuakeLike_1_0/Character/BP_ShooterBase.BP_ShooterBase` | Blueprint; /Script/CoreUObject.Class'/Script/Engine.Character' | Accel Character, Callbacks, Debug_Life, Debug_UsableCount, EventGraph, Get Movement Option, GetYaw, Ignore Input End, RagdollOnDeath, SetItemOwner, UserConstructionScript |
| `/Game/QuakeLike_1_0/Player/BPC_InputOption.BPC_InputOption` | Blueprint; /Script/CoreUObject.Class'/Script/Engine.ActorComponent' | Activate Input Option UI, Create Input Option UI, EventGraph, Get Aim Speed As Pure |
| `/Game/QuakeLike_1_0/Player/BPI_InputOption.BPI_InputOption` | Blueprint; /Script/CoreUObject.Class'/Script/CoreUObject.Interface' | GetAimSpeed, SaveAimSpeed |
| `/Game/QuakeLike_1_0/Player/BP_SaveInputOption.BP_SaveInputOption` | Blueprint; /Script/CoreUObject.Class'/Script/Engine.SaveGame' | EventGraph |
| `/Game/QuakeLike_1_0/Player/WBP_PlayerOption.WBP_PlayerOption` | WidgetBlueprint; /Script/CoreUObject.Class'/Script/UMG.UserWidget' | EventGraph, Get AimSpeed, Get Commit Result, Get Valid InputBox Value, OnKeyDown, Set AimSpeed, Set InputBox, Set X AimSpeed, Set Y AimSpeed, Value Initialize |

호출 노드·변수·컴포넌트 상세는 로컬 실행 폴더의 `blueprint-catalog.json`과 `graphs/`에 있습니다.
JSON 조회기 외 그래프는 `graph-text/`의 Unreal 텍스트 내보내기로 보충했으며, 컴파일 파생 그래프는 별도 제외 목록에 기록합니다.
