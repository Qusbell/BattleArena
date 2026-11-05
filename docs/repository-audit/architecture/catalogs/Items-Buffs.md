# Items.Buffs 객체 카탈로그

이 그룹은 경로 키워드에 따른 **분류 추론**입니다. 소유권·실행 여부를 증명하지 않습니다.
아래 역할 요약은 클래스 선언 주석, 상속 태그와 실제 그래프 이름을 근거로 하며, 이름만으로 동작을 확정하지 않습니다.

## C++ 선언

| 타입 | 역할 근거 / 헤더 함수 검색어 | 선언 위치 |
|---|---|---|
| `FBuffVisualEffects` | Attachment settings only. Niagara and Cascade assets come from the acquired Buff DT row. / 헤더 검색어: BeginPlay, ClearBuffPresentation, ClientPlayBuffPickupSound, ConsumeWarningSecond, EndPlay, GetEffects, GetLifetimeReplicatedProps, GetPresentationRemainingTime | [Source/ShootingArena/Public/Buff/BuffPresentationComponent.h:26](../../../../Source/ShootingArena/Public/Buff/BuffPresentationComponent.h) |
| `FBuffPresentationState` | 선언 종류: struct; 상속: 없음; 업무 역할 미서술 / 헤더 검색어: BeginPlay, ClearBuffPresentation, ClientPlayBuffPickupSound, ConsumeWarningSecond, EndPlay, GetEffects, GetLifetimeReplicatedProps, GetPresentationRemainingTime | [Source/ShootingArena/Public/Buff/BuffPresentationComponent.h:45](../../../../Source/ShootingArena/Public/Buff/BuffPresentationComponent.h) |
| `UBuffPresentationComponent` | Presentation-only parent of BPC_Buff; gameplay duration/damage/drop remain in Blueprint. / 헤더 검색어: BeginPlay, ClearBuffPresentation, ClientPlayBuffPickupSound, ConsumeWarningSecond, EndPlay, GetEffects, GetLifetimeReplicatedProps, GetPresentationRemainingTime | [Source/ShootingArena/Public/Buff/BuffPresentationComponent.h:91](../../../../Source/ShootingArena/Public/Buff/BuffPresentationComponent.h) |
| `UBuffSpawnCycleSubsystem` | Keeps buff item spawn points alternating between their ATK and DEF rows. BP_ItemSpawnPoint owns the actual spawn/respawn flow. This subsystem only changes its SpawnItemTable row after the current item has been removed, so the existing timer, VFX, sound, and replication behavior remain untouched. / 헤더 검색어: Deinitialize, GetNextBuffRow, GetStatId, IsTickable, RegisterActor, RegisterLevelActors, Tick, UnregisterLevelActors | [Source/ShootingArena/Public/Buff/BuffSpawnCycleSubsystem.h:19](../../../../Source/ShootingArena/Public/Buff/BuffSpawnCycleSubsystem.h) |
| `FSpawnPointState` | 선언 종류: struct; 상속: 없음; 업무 역할 미서술 / 헤더 검색어: Deinitialize, GetNextBuffRow, GetStatId, IsTickable, RegisterActor, RegisterLevelActors, Tick, UnregisterLevelActors | [Source/ShootingArena/Public/Buff/BuffSpawnCycleSubsystem.h:32](../../../../Source/ShootingArena/Public/Buff/BuffSpawnCycleSubsystem.h) |
| `FBuffGlowLook` | One overlay look (gun or body). Values feed M_BuffGlowOverlay-style material parameters. / 헤더 검색어: FindByPredicate, FindProfile, FromSRGBColor, IsValid | [Source/ShootingArena/Public/Buff/BuffWeaponGlow.h:12](../../../../Source/ShootingArena/Public/Buff/BuffWeaponGlow.h) |
| `FBuffGlowProfile` | Gun and body looks for the buffs matching BuffTag. / 헤더 검색어: FindByPredicate, FindProfile, FromSRGBColor, IsValid | [Source/ShootingArena/Public/Buff/BuffWeaponGlow.h:79](../../../../Source/ShootingArena/Public/Buff/BuffWeaponGlow.h) |
| `UBuffWeaponGlowSettings` | Gun and third-person body overlay glow per buff. / 헤더 검색어: FindByPredicate, FindProfile, FromSRGBColor, IsValid | [Source/ShootingArena/Public/Buff/BuffWeaponGlow.h:97](../../../../Source/ShootingArena/Public/Buff/BuffWeaponGlow.h) |
| `ADeathDropCarrier` | Replicated physical body for a death pickup. Map pickups never use this actor. / 헤더 검색어: BeginPlay, ConfigurePickup, EndPlay, FinishPickup, GetLifetimeReplicatedProps, GetServerTimeSeconds, Initialize, Launch | [Source/ShootingArena/Public/Items/DeathDropCarrier.h:13](../../../../Source/ShootingArena/Public/Items/DeathDropCarrier.h) |
| `FDeathDropSettings` | 캐릭터가 한 번 사망했을 때 사용할 드롭 설정. 맵에 배치된 아이템 스포너에는 적용하지 않는다. / 헤더 검색어:  | [Source/ShootingArena/Public/Items/DeathDropSettings.h:9](../../../../Source/ShootingArena/Public/Items/DeathDropSettings.h) |
| `UDeathDropSubsystem` | Collects death-state candidates before LifeInfo notifies listeners. / 헤더 검색어: Remove, ResetForNextLife | [Source/ShootingArena/Public/Items/DeathDropSubsystem.h:10](../../../../Source/ShootingArena/Public/Items/DeathDropSubsystem.h) |
| `E_ItemSpawnType` | 선언 종류: enum class; 상속: uint8; 업무 역할 미서술 / 헤더 검색어: GetSafeSpawnDelay, Max | [Source/ShootingArena/Public/Spawner/ItemSpawnSelectionLibrary.h:9](../../../../Source/ShootingArena/Public/Spawner/ItemSpawnSelectionLibrary.h) |
| `UItemSpawnSelectionLibrary` | 선언 종류: class; 상속: public UBlueprintFunctionLibrary; 업무 역할 미서술 / 헤더 검색어: GetSafeSpawnDelay, Max | [Source/ShootingArena/Public/Spawner/ItemSpawnSelectionLibrary.h:19](../../../../Source/ShootingArena/Public/Spawner/ItemSpawnSelectionLibrary.h) |

헤더 검색어는 같은 헤더에 선언된 다른 타입의 함수도 포함할 수 있는 탐색 보조 자료입니다.

## Blueprint 정의

| 객체 경로 | 상속 / 정의 역할 | 확인한 그래프 |
|---|---|---|
| `/Game/QuakeLike_1_0/Buff/BPC_Buff.BPC_Buff` | Blueprint; /Script/CoreUObject.Class'/Script/ShootingArena.BuffPresentationComponent' | AddBuffTag, ApplyBuff, ApplyDroppedBuff, CalcAttackBuff, CalcDefenceBuff, EventGraph, GetRemainBuffTime, RemoveBuffTag |
| `/Game/QuakeLike_1_0/Buff/BPC_Buff_Manager.BPC_Buff_Manager` | Blueprint; /Script/CoreUObject.Class'/Script/Engine.ActorComponent' | BuffTimeSet, EventGraph |
| `/Game/QuakeLike_1_0/Buff/BPI_Buff.BPI_Buff` | Blueprint; /Script/CoreUObject.Class'/Script/CoreUObject.Interface' | MultiplyApplyDamage, ReduceTakenDamage |
| `/Game/QuakeLike_1_0/DropItem/BPI_AmmoTarget.BPI_AmmoTarget` | Blueprint; /Script/CoreUObject.Class'/Script/CoreUObject.Interface' | Get Ammo Target Item ID |
| `/Game/QuakeLike_1_0/DropItem/BPI_DropItem.BPI_DropItem` | Blueprint; /Script/CoreUObject.Class'/Script/CoreUObject.Interface' | GetItemID |
| `/Game/QuakeLike_1_0/DropItem/BP_AmmoItem.BP_AmmoItem` | Blueprint; /Script/Engine.BlueprintGeneratedClass'/Game/QuakeLike_1_0/DropItem/BP_DropItem.BP_DropItem_C' | ApplyItemEffect, EventGraph, Get Ammo Item Data, Get Attribute Tags, GetPickSound, UserConstructionScript |
| `/Game/QuakeLike_1_0/DropItem/BP_BuffItem.BP_BuffItem` | Blueprint; /Script/Engine.BlueprintGeneratedClass'/Game/QuakeLike_1_0/DropItem/BP_DropItem.BP_DropItem_C' | ApplyItemEffect, EventGraph, Get Attribute Tags, GetPickSound, UserConstructionScript |
| `/Game/QuakeLike_1_0/DropItem/BP_DropItem.BP_DropItem` | Blueprint; /Script/CoreUObject.Class'/Script/Engine.Actor' | ApplyItemEffect, EventGraph, GetPickSound, OnRep_IsPickedUp, Spin, UserConstructionScript |
| `/Game/QuakeLike_1_0/DropItem/BP_HoldableItem.BP_HoldableItem` | Blueprint; /Script/Engine.BlueprintGeneratedClass'/Game/QuakeLike_1_0/DropItem/BP_DropItem.BP_DropItem_C' | ApplyItemEffect, EventGraph, Get Attribute Tags, GetPickSound, UserConstructionScript |
| `/Game/QuakeLike_1_0/DropItem/BP_LifeItem.BP_LifeItem` | Blueprint; /Script/Engine.BlueprintGeneratedClass'/Game/QuakeLike_1_0/DropItem/BP_DropItem.BP_DropItem_C' | ApplyItemEffect, EventGraph, Get Attribute Tags, GetData, GetPickSound, UserConstructionScript |
| `/Game/QuakeLike_1_0/Inventory/BPC_Inventory.BPC_Inventory` | Blueprint; /Script/CoreUObject.Class'/Script/Engine.ActorComponent' | AITrySwap, EventGraph, Get Next Valid Index, Get Shifted Index, GetBeforeItem, HasItemCached, IsValidIndex, OnRep_All_Items, OnRep_IndexInfo, RebuildItemCache, SetIndex, TrySwap |
| `/Game/QuakeLike_1_0/Inventory/BPI_Inventory.BPI_Inventory` | Blueprint; /Script/CoreUObject.Class'/Script/CoreUObject.Interface' | Clear, GetSelectedItem, HasItem, Pickup, Recharge, SwapItem, SwapItemByTag, SwapNext, SwapPrev |
| `/Game/QuakeLike_1_0/Life/BPC_Life.BPC_Life` | Blueprint; /Script/CoreUObject.Class'/Script/Engine.ActorComponent' | Apply Heal, Calc Dmg Distribution, DropBuffOnDeath, EventGraph, Get Max Health, Get Max Plate, Get Now Health, Get Now Plate, Get Over Max Health, Get Over Max Plate, Get Owning Controller, GetBuffTakeDamage, HandleOverLife, Init Info, Is Death, IsFatalDamage, OnRep_LifeInfo |
| `/Game/QuakeLike_1_0/Life/BPI_Life.BPI_Life` | Blueprint; /Script/CoreUObject.Class'/Script/CoreUObject.Interface' | ApplyHeal, GetLife, OwnerTakeDamage |
| `/Game/QuakeLike_1_0/Spawner/BPI_ItemSpawner.BPI_ItemSpawner` | Blueprint; /Script/CoreUObject.Class'/Script/CoreUObject.Interface' | GetSpawnFeedback, SetSpawnTarget, Spawn |
| `/Game/QuakeLike_1_0/Spawner/Component/BPC_AmmoItemSpawner.BPC_AmmoItemSpawner` | Blueprint; /Script/CoreUObject.Class'/Script/Engine.ActorComponent' | EventGraph |
| `/Game/QuakeLike_1_0/Spawner/Component/BPC_BuffSpawner.BPC_BuffSpawner` | Blueprint; /Script/CoreUObject.Class'/Script/Engine.ActorComponent' | EventGraph |
| `/Game/QuakeLike_1_0/Spawner/Component/BPC_LifeItemSpawner.BPC_LifeItemSpawner` | Blueprint; /Script/CoreUObject.Class'/Script/Engine.ActorComponent' | EventGraph |
| `/Game/QuakeLike_1_0/Spawner/Component/BPC_RespawnHelper.BPC_RespawnHelper` | Blueprint; /Script/CoreUObject.Class'/Script/Engine.ActorComponent' | Debug_Owner, Destroy Shooter, EventGraph, Set Pawn |
| `/Game/QuakeLike_1_0/Spawner/Component/BPC_SpawnerManager.BPC_SpawnerManager` | Blueprint; /Script/CoreUObject.Class'/Script/Engine.ActorComponent' | EventGraph, FindSafeSpawnPoint, Is Spawner Initialized |
| `/Game/QuakeLike_1_0/Spawner/SpawnPoint/BP_FixedItemSpawnPoint.BP_FixedItemSpawnPoint` | Blueprint; /Script/Engine.BlueprintGeneratedClass'/Game/QuakeLike_1_0/Spawner/SpawnPoint/BP_SpawnPointBase.BP_SpawnPointBase_C' | EventGraph, FindSpawnerByTable, IsAvailable, UserConstructionScript |
| `/Game/QuakeLike_1_0/Spawner/SpawnPoint/BP_ItemSpawnPoint.BP_ItemSpawnPoint` | Blueprint; /Script/Engine.BlueprintGeneratedClass'/Game/QuakeLike_1_0/Spawner/SpawnPoint/BP_SpawnPointBase.BP_SpawnPointBase_C' | EventGraph, FindSpawnerByTable, IsAvailable, UserConstructionScript |
| `/Game/QuakeLike_1_0/Spawner/SpawnPoint/BP_PlayerSpawnPoint.BP_PlayerSpawnPoint` | Blueprint; /Script/Engine.BlueprintGeneratedClass'/Game/QuakeLike_1_0/Spawner/SpawnPoint/BP_SpawnPointBase.BP_SpawnPointBase_C' | EventGraph, UserConstructionScript |
| `/Game/QuakeLike_1_0/Spawner/SpawnPoint/BP_SpawnPointBase.BP_SpawnPointBase` | Blueprint; /Script/CoreUObject.Class'/Script/Engine.Actor' | EventGraph, Get Spawn Transform, Toggle Base Plate, UserConstructionScript |
| `/Game/QuakeLike_Base/LJH/Item/DropItem/BP_LJH_DropItemBase.BP_LJH_DropItemBase` | Blueprint; /Script/CoreUObject.Class'/Script/Engine.Actor' | ApplyItemEffect, EventGraph, Spin , UserConstructionScript |
| `/Game/QuakeLike_Base/LJH/Item/DropItem/BP_LJH_HealPack.BP_LJH_HealPack` | Blueprint; /Script/Engine.BlueprintGeneratedClass'/Game/QuakeLike_Base/LJH/Item/DropItem/BP_LJH_DropItemBase.BP_LJH_DropItemBase_C' | ApplyItemEffect, EventGraph, UserConstructionScript |
| `/Game/QuakeLike_Base/LJH/Item/DropItem/BP_LJH_PlateHeal.BP_LJH_PlateHeal` | Blueprint; /Script/Engine.BlueprintGeneratedClass'/Game/QuakeLike_Base/LJH/Item/DropItem/BP_LJH_DropItemBase.BP_LJH_DropItemBase_C' | ApplyItemEffect, EventGraph, UserConstructionScript |
| `/Game/QuakeLike_Base/LJH/Item/DropItem/BP_LJH_WeaponAmmoItem.BP_LJH_WeaponAmmoItem` | Blueprint; /Script/Engine.BlueprintGeneratedClass'/Game/QuakeLike_Base/LJH/Item/DropItem/BP_LJH_DropItemBase.BP_LJH_DropItemBase_C' | ApplyItemEffect, EventGraph, UserConstructionScript |
| `/Game/QuakeLike_Base/LJH/Item/DropItem/BP_LJH_WeaponItem.BP_LJH_WeaponItem` | Blueprint; /Script/Engine.BlueprintGeneratedClass'/Game/QuakeLike_Base/LJH/Item/DropItem/BP_LJH_DropItemBase.BP_LJH_DropItemBase_C' | ApplyItemEffect, EventGraph, UserConstructionScript |
| `/Game/QuakeLike_Base/LJH/Item/Spawner/BP_LJH_ItemSpawnerBase.BP_LJH_ItemSpawnerBase` | Blueprint; /Script/CoreUObject.Class'/Script/Engine.Actor' | EventGraph, UserConstructionScript |
| `/Game/QuakeLike_Base/LJH/Item/Spawner/BP_LJH_PlayerSpawner.BP_LJH_PlayerSpawner` | Blueprint; /Script/CoreUObject.Class'/Script/Engine.Actor' | EventGraph, UserConstructionScript |
| `/Game/QuakeLike_Base/LJH/Life/BPC_BGH_Life.BPC_BGH_Life` | Blueprint; /Script/CoreUObject.Class'/Script/Engine.ActorComponent' | Apply Damage, ApplyHeal, EventGraph, Get Event Container, HandleDeath, HandleOverCharge, Setup State |
| `/Game/QuakeLike_Base/LJH/Life/BPI_BGH_Life.BPI_BGH_Life` | Blueprint; /Script/CoreUObject.Class'/Script/CoreUObject.Interface' | Apply Health Heal, Apply Plate Heal, Get Health, Get Plate, Take Damage |
| `/Game/QuakeLike_Base/LJH/Life/BP_DamageTypeBase.BP_DamageTypeBase` | Blueprint; /Script/CoreUObject.Class'/Script/Engine.DamageType' | 최상위 그래프 없음; 상속/데이터 중심 정의 가능 |
| `/Game/QuakeLike_Base/LJH/Life/BP_HealTypeBase.BP_HealTypeBase` | Blueprint; /Script/CoreUObject.Class'/Script/CoreUObject.Object' | EventGraph |

호출 노드·변수·컴포넌트 상세는 로컬 실행 폴더의 `blueprint-catalog.json`과 `graphs/`에 있습니다.
JSON 조회기 외 그래프는 `graph-text/`의 Unreal 텍스트 내보내기로 보충했으며, 컴파일 파생 그래프는 별도 제외 목록에 기록합니다.
