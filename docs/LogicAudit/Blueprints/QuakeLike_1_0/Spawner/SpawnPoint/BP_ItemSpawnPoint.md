# BP_ItemSpawnPoint

에셋: `/Game/QuakeLike_1_0/Spawner/SpawnPoint/BP_ItemSpawnPoint`

부모: `/Script/Engine.BlueprintGeneratedClass'/Game/QuakeLike_1_0/Spawner/SpawnPoint/BP_SpawnPointBase.BP_SpawnPointBase_C'`

수집일: 2026-10-12. MCP에서 읽은 그래프·핀 연결의 자동 전사입니다. 상속된 그래프는 부모 문서를 함께 확인합니다.

정적 루트 참조 도달: 있음. 실행 여부 또는 삭제 가능 여부의 확정이 아닙니다.

컴포넌트 목록은 이 BP가 직접 정의한 SCS만 포함합니다. 부모·네이티브 생성 컴포넌트, 인스턴스 설정은 별도입니다.

## 직접 정의한 컴포넌트

| 이름 | 클래스 | 부모 컴포넌트 |
|---|---|---|
| `BPC_LifeItemSpawner` | `BPC_LifeItemSpawner_C` | `` |
| `BPC_HeldItemSpawner` | `BPC_HeldItemSpawner_C` | `` |
| `BPC_AmmoItemSpawner` | `BPC_AmmoItemSpawner_C` | `` |
| `RespawnRing` | `StaticMeshComponent` | `` |
| `BPC_BuffSpawner` | `BPC_BuffSpawner_C` | `` |

## 변수와 기본값

| 이름 | 타입 | CDO 기본값 |
|---|---|---|
| `Having Item` | `BP_DropItem_C` | None |
| `ActiveSpawner` | `BPI_ItemSpawner_C` | None |
| `RespawnDelay` | `real` | 1.0 |
| `RespawnTargetTime` | `real` | 0.0 |
| `RingDynamicMat` | `MaterialInstanceDynamic` | None |
| `FirstSpawnCooltime` | `real` | 0.0 |
| `CurrentSpawnDelay` | `real` | 1.0 |
| `SpawnItemTable` | `DataTableRowHandle` | [] |
| `CurrentSpawnItem` | `DataTableRowHandle` | &lt;Struct 'DataTableRowHandle' {data_table: None, row_name: ""}&gt; |
| `CurrentSpawnIndex` | `int` | -1 |
| `SpawnDirection` | `int` | 1 |
| `spawn_type` | `E_ItemSpawnType` | &lt;E_ItemSpawnType.SEQUENTIAL: 0&gt; |
| `PreSpawnFeedbackLeadTime` | `real` | 1.0 |

## 그래프 목록

| 그래프 | 전체 노드 수 |
|---|---|
| EventGraph | 116 |
| FindSpawnerByTable | 21 |
| IsAvailable | 5 |
| UserConstructionScript | 2 |

## 그래프별 실행 연결

실선은 출력 exec 핀의 실제 연결입니다. 여러 분기는 출력 핀 이름을 표시합니다. 함수 호출·delegate·타이머의 다음 그래프 실행은 이 그림에서 자동으로 이어 붙이지 않습니다.

순수 계산 노드 및 데이터 핀은 아래 목록과 원본 JSON을 확인합니다. 연결되지 않은 이벤트도 표시되므로, 노드가 있다는 이유만으로 실행된다고 판단하면 안 됩니다.

### EventGraph

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["AssignItem<br/>Custom Event"]
  n1["Set Having Item"]
  n2["Bind Event to On Destroyed"]
  n3["OnSpawnItemDestroyed<br/>Custom Event"]
  n4["Set Having Item"]
  n5["BeginPlay 이벤트"]
  n6["Set Spawn Target<br/>Target is BPI Item Spawner"]
  n7["Spawn<br/>Target is BPI Item Spawner"]
  n8["CheckAndSpawnRoutine<br/>Custom Event"]
  n9["Switch Has Authority"]
  n10["Branch"]
  n11["Assign Item<br/>타깃은 BP Item Spawn Point"]
  n12["Tick 이벤트"]
  n13["Branch"]
  n14["Create Dynamic Material Instance"]
  n15["Set Scalar Parameter Value<br/>타깃은 머티리얼 인스턴스 다이내믹"]
  n16["Set RingDynamicMat"]
  n17["Set Material<br/>타깃은 프리미티브 컴포넌트"]
  n18["Is Valid"]
  n19["Set Visibility<br/>타깃은 씬 컴포넌트"]
  n20["Set Visibility<br/>타깃은 씬 컴포넌트"]
  n21["Multicast_PlaySpawnFeedbackReplicated From Server<br/>Executes On All<br/>Custom Event"]
  n22["Play Sound at Location"]
  n23["Get Spawn Feedback<br/>Target is BPI Item Spawner"]
  n24["Multicast Play Spawn Feedback<br/>타깃은 BP Item Spawn Point<br/>모두에 리플리케이트됨 (서버인 경우)"]
  n25["Spawn System at Location"]
  n26["시퀀스"]
  n27["Set World Rotation<br/>타깃은 씬 컴포넌트"]
  n28["경유 노드"]
  n29["Bind Event to On Picked Up"]
  n30["OnSpawnItemPickedUp<br/>Custom Event"]
  n31["Unbind Event from On Destroyed"]
  n32["On Spawn Item Destroyed<br/>타깃은 BP Item Spawn Point"]
  n33["InitialBuffSpawn<br/>Custom Event"]
  n34["Check and Spawn Routine<br/>타깃은 BP Item Spawn Point"]
  n35["StartSpawnCycle<br/>Custom Event"]
  n36["PlayPendingSpawnFeedback<br/>Custom Event"]
  n37["Branch"]
  n38["Select Next Spawn Item"]
  n39["Set CurrentSpawnIndex"]
  n40["Set SpawnDirection"]
  n41["Set CurrentSpawnItem"]
  n42["Branch"]
  n43["Find Spawner by Table<br/>타깃은 BP Item Spawn Point"]
  n44["Branch"]
  n45["Set CurrentSpawnDelay"]
  n46["Set RespawnTargetTime"]
  n47["Branch"]
  n48["Set Timer by Function Name"]
  n49["Play Pending Spawn Feedback<br/>타깃은 BP Item Spawn Point"]
  n50["Branch"]
  n51["Set Timer by Function Name"]
  n52["Initial Buff Spawn<br/>타깃은 BP Item Spawn Point"]
  n53["Start Spawn Cycle<br/>타깃은 BP Item Spawn Point"]
  n54["Start Spawn Cycle<br/>타깃은 BP Item Spawn Point"]
  n55["Branch"]
  n56["Branch"]
  n57["Get Spawn Feedback<br/>Target is BPI Item Spawner"]
  n58["Multicast Play Spawn Feedback<br/>타깃은 BP Item Spawn Point<br/>모두에 리플리케이트됨 (서버인 경우)"]
  n0 -->|"then"| n55
  n1 -->|"then"| n2
  n2 -->|"then"| n29
  n3 -->|"then"| n4
  n4 -->|"then"| n54
  n5 -->|"then"| n14
  n6 -->|"then"| n7
  n7 -->|"then"| n11
  n8 -->|"then"| n9
  n9 -->|"Authority"| n10
  n10 -->|"then"| n6
  n12 -->|"then"| n13
  n13 -->|"else"| n26
  n14 -->|"then"| n16
  n15 -->|"then"| n18
  n16 -->|"then"| n17
  n17 -->|"then"| n53
  n18 -->|"Is Valid"| n19
  n18 -->|"Is Not Valid"| n20
  n21 -->|"then"| n22
  n22 -->|"then"| n25
  n23 -->|"then"| n24
  n26 -->|"then_0"| n28
  n26 -->|"then_1"| n15
  n28 -->|"OutputPin"| n27
  n29 -->|"then"| n56
  n30 -->|"then"| n31
  n31 -->|"then"| n32
  n33 -->|"then"| n34
  n35 -->|"then"| n37
  n36 -->|"then"| n23
  n37 -->|"then"| n38
  n38 -->|"then"| n39
  n39 -->|"then"| n40
  n40 -->|"then"| n41
  n41 -->|"then"| n42
  n42 -->|"then"| n43
  n43 -->|"then"| n44
  n44 -->|"then"| n45
  n45 -->|"then"| n46
  n46 -->|"then"| n47
  n47 -->|"then"| n48
  n47 -->|"else"| n49
  n48 -->|"then"| n50
  n49 -->|"then"| n50
  n50 -->|"then"| n51
  n50 -->|"else"| n52
  n55 -->|"then"| n1
  n56 -->|"then"| n57
  n57 -->|"then"| n58
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_CustomEvent_0` | AssignItem<br/>Custom Event |  |
| `K2Node_VariableSet_0` | Set Having Item |  |
| `K2Node_AddDelegate_0` | Bind Event to On Destroyed |  |
| `K2Node_CustomEvent_1` | OnSpawnItemDestroyed<br/>Custom Event |  |
| `K2Node_VariableSet_1` | Set Having Item |  |
| `K2Node_Event_0` | BeginPlay 이벤트 |  |
| `K2Node_Message_1` | Set Spawn Target<br/>Target is BPI Item Spawner |  |
| `K2Node_Message_2` | Spawn<br/>Target is BPI Item Spawner |  |
| `K2Node_VariableGet_11` | Get ActiveSpawner |  |
| `K2Node_CallFunction_23` | To Object (Interface) |  |
| `K2Node_CallFunction_14` | Get World Transform<br/>타깃은 씬 컴포넌트 |  |
| `K2Node_VariableGet_12` | Get SpawnPoint |  |
| `K2Node_CustomEvent_2` | CheckAndSpawnRoutine<br/>Custom Event |  |
| `K2Node_MacroInstance_1` | Switch Has Authority |  |
| `K2Node_CallFunction_2` | Is Available |  |
| `K2Node_IfThenElse_0` | Branch |  |
| `K2Node_Knot_4` | 경유 노드 |  |
| `K2Node_CallFunction_16` | Assign Item<br/>타깃은 BP Item Spawn Point |  |
| `K2Node_Event_1` | Tick 이벤트 | DeltaSeconds=0.0 |
| `K2Node_CallFunction_18` | Is Dedicated Server |  |
| `K2Node_IfThenElse_3` | Branch |  |
| `K2Node_CallMaterialParameterCollectionFunction_0` | Create Dynamic Material Instance | Parent=/Game/QuakeLike_1_0/Spawner/Materials/M_RespawnTimer.M_RespawnTimer; OptionalName=None; CreationFlags=None |
| `K2Node_CallFunction_3` | Clamp (Float) | Min=0.0; Max=1.0 |
| `K2Node_CallFunction_0` | Get Game State |  |
| `K2Node_CallFunction_1` | Get Server World Time Seconds<br/>타깃은 게임 스테이트 베이스 |  |
| `K2Node_VariableGet_15` | Get RespawnTargetTime |  |
| `K2Node_PromotableOperator_4` | float - float |  |
| `K2Node_PromotableOperator_5` | float / float |  |
| `K2Node_PromotableOperator_6` | float - float | A=1.000000 |
| `K2Node_VariableGet_17` | Get RingDynamicMat |  |
| `K2Node_CallFunction_20` | Set Scalar Parameter Value<br/>타깃은 머티리얼 인스턴스 다이내믹 | ParameterName=Percent |
| `K2Node_VariableSet_6` | Set RingDynamicMat |  |
| `K2Node_VariableGet_18` | Get RespawnRing |  |
| `K2Node_CallFunction_29` | Set Material<br/>타깃은 프리미티브 컴포넌트 | ElementIndex=0 |
| `K2Node_VariableGet_25` | Get Having Item |  |
| `K2Node_MacroInstance_0` | Is Valid |  |
| `K2Node_VariableGet_27` | Get RespawnRing |  |
| `K2Node_CallFunction_40` | Set Visibility<br/>타깃은 씬 컴포넌트 | bNewVisibility=false; bPropagateToChildren=false |
| `K2Node_CallFunction_39` | Set Visibility<br/>타깃은 씬 컴포넌트 | bNewVisibility=true; bPropagateToChildren=false |
| `K2Node_CustomEvent_3` | Multicast_PlaySpawnFeedbackReplicated From Server<br/>Executes On All<br/>Custom Event |  |
| `K2Node_CallFunction_4` | Play Sound at Location | Rotation=0, 0, 0; PitchMultiplier=1.000000; StartTime=0.000000; AttenuationSettings=/Game/QuakeLike_1_0/Spawner/SoundFX/SA_SpawnSound.SA_SpawnSound |
| `K2Node_VariableGet_0` | Get SpawnPoint |  |
| `K2Node_CallFunction_5` | Get World Location<br/>타깃은 씬 컴포넌트 |  |
| `K2Node_Message_0` | Get Spawn Feedback<br/>Target is BPI Item Spawner | Spawn Volume=0.0 |
| `K2Node_Knot_0` | 경유 노드 |  |
| `K2Node_Knot_1` | 경유 노드 |  |
| `K2Node_Knot_2` | 경유 노드 |  |
| `K2Node_CallFunction_8` | Multicast Play Spawn Feedback<br/>타깃은 BP Item Spawn Point<br/>모두에 리플리케이트됨 (서버인 경우) | SpawnVolume=0.0 |
| `K2Node_CallFunction_9` | Spawn System at Location | Rotation=0, 0, 0; Scale=1.000000,1.000000,1.000000; bAutoDestroy=true; bAutoActivate=true; PoolingMethod=None; bPreCullCheck=true |
| `K2Node_Knot_3` | 경유 노드 |  |
| `K2Node_Knot_5` | 경유 노드 |  |
| `K2Node_ExecutionSequence_0` | 시퀀스 |  |
| `K2Node_CallFunction_10` | Get Player Controller | PlayerIndex=0 |
| `K2Node_CallFunction_11` | Get Control Rotation<br/>타깃은 컨트롤러 | ReturnValue_Roll=0.0; ReturnValue_Pitch=0.0 |
| `K2Node_CallFunction_19` | Make Rotator | Roll=0.0; Pitch=90.000000 |
| `K2Node_CallFunction_21` | Set World Rotation<br/>타깃은 씬 컴포넌트 | bSweep=false; bTeleport=false |
| `K2Node_VariableGet_2` | Get RespawnRing |  |
| `K2Node_Knot_6` | 경유 노드 |  |
| `K2Node_AddDelegate_1` | Bind Event to On Picked Up |  |
| `K2Node_CustomEvent_4` | OnSpawnItemPickedUp<br/>Custom Event |  |
| `K2Node_RemoveDelegate_0` | Unbind Event from On Destroyed |  |
| `K2Node_CallFunction_22` | On Spawn Item Destroyed<br/>타깃은 BP Item Spawn Point |  |
| `K2Node_CustomEvent_5` | InitialBuffSpawn<br/>Custom Event |  |
| `K2Node_CallFunction_15` | Check and Spawn Routine<br/>타깃은 BP Item Spawn Point |  |
| `K2Node_VariableGet_7` | Get CurrentSpawnDelay |  |
| `K2Node_CustomEvent_6` | StartSpawnCycle<br/>Custom Event |  |
| `K2Node_CustomEvent_7` | PlayPendingSpawnFeedback<br/>Custom Event |  |
| `K2Node_CallFunction_30` | Has Authority<br/>타깃은 액터 |  |
| `K2Node_IfThenElse_4` | Branch |  |
| `K2Node_CallFunction_31` | Select Next Spawn Item |  |
| `K2Node_VariableGet_8` | Get SpawnItemTable |  |
| `K2Node_VariableGet_13` | Get spawn_type |  |
| `K2Node_VariableGet_16` | Get CurrentSpawnIndex |  |
| `K2Node_VariableGet_19` | Get SpawnDirection |  |
| `K2Node_VariableSet_7` | Set CurrentSpawnIndex | Output_Get=0 |
| `K2Node_VariableSet_8` | Set SpawnDirection | Output_Get=0 |
| `K2Node_VariableSet_9` | Set CurrentSpawnItem |  |
| `K2Node_IfThenElse_5` | Branch |  |
| `K2Node_CallFunction_32` | Find Spawner by Table<br/>타깃은 BP Item Spawn Point |  |
| `K2Node_VariableGet_20` | Get CurrentSpawnItem |  |
| `K2Node_IfThenElse_6` | Branch |  |
| `K2Node_VariableSet_10` | Set CurrentSpawnDelay | Output_Get=0.0 |
| `K2Node_CallFunction_33` | Get Game State |  |
| `K2Node_CallFunction_34` | Get Server World Time Seconds<br/>타깃은 게임 스테이트 베이스 |  |
| `K2Node_CallFunction_35` | float + float |  |
| `K2Node_VariableSet_11` | Set RespawnTargetTime | Output_Get=0.0 |
| `K2Node_CallFunction_36` | float - float |  |
| `K2Node_VariableGet_21` | Get PreSpawnFeedbackLeadTime |  |
| `K2Node_CallFunction_37` | float &gt; float | B=0.0 |
| `K2Node_IfThenElse_7` | Branch |  |
| `K2Node_CallFunction_38` | Set Timer by Function Name | FunctionName=PlayPendingSpawnFeedback; bLooping=false; bMaxOncePerFrame=false; InitialStartDelay=0.000000; InitialStartDelayVariance=0.000000 |
| `K2Node_CallFunction_41` | Play Pending Spawn Feedback<br/>타깃은 BP Item Spawn Point |  |
| `K2Node_CallFunction_42` | float &gt; float | B=0.0 |
| `K2Node_IfThenElse_8` | Branch |  |
| `K2Node_CallFunction_43` | Set Timer by Function Name | FunctionName=InitialBuffSpawn; bLooping=false; bMaxOncePerFrame=false; InitialStartDelay=0.000000; InitialStartDelayVariance=0.000000 |
| `K2Node_CallFunction_44` | Initial Buff Spawn<br/>타깃은 BP Item Spawn Point |  |
| `K2Node_CallFunction_46` | Start Spawn Cycle<br/>타깃은 BP Item Spawn Point |  |
| `K2Node_VariableGet_23` | Get FirstSpawnCooltime |  |
| `K2Node_CallFunction_47` | Start Spawn Cycle<br/>타깃은 BP Item Spawn Point |  |
| `K2Node_VariableGet_24` | Get RespawnDelay |  |
| `K2Node_CallFunction_13` | Get Safe Spawn Delay | ReturnValue=0.0 |
| `K2Node_VariableGet_26` | Get CurrentSpawnItem |  |
| `K2Node_VariableGet_28` | Get CurrentSpawnItem |  |
| `K2Node_VariableGet_29` | Get |  |
| `K2Node_CallFunction_17` | Get Safe Spawn Delay | ReturnValue=0.0 |
| `K2Node_CallFunction_24` | Get Safe Spawn Delay |  |
| `K2Node_Knot_7` | 경유 노드 |  |
| `K2Node_Knot_8` | 경유 노드 |  |
| `K2Node_Knot_9` | 경유 노드 |  |
| `K2Node_Knot_10` | 경유 노드 |  |
| `ItemSound_IsValidSpawn` | Is Valid |  |
| `ItemSound_ValidSpawn` | Branch |  |
| `K2Node_CallFunction_12` | Has Authority<br/>타깃은 액터 |  |
| `ItemSound_AfterAssign` | Branch |  |
| `ItemSound_GetSpawnFeedback` | Get Spawn Feedback<br/>Target is BPI Item Spawner |  |
| `ItemSound_PlaySpawnSound` | Multicast Play Spawn Feedback<br/>타깃은 BP Item Spawn Point<br/>모두에 리플리케이트됨 (서버인 경우) |  |

</details>

### FindSpawnerByTable

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["FindSpawnerByTable"]
  n1["Branch"]
  n2["Branch"]
  n3["Set ActiveSpawner"]
  n4["반환 노드"]
  n5["Set ActiveSpawner"]
  n6["Set ActiveSpawner"]
  n7["Branch"]
  n8["Print String"]
  n9["반환 노드"]
  n10["Branch"]
  n11["Set ActiveSpawner"]
  n0 -->|"then"| n1
  n1 -->|"then"| n3
  n1 -->|"else"| n2
  n2 -->|"then"| n5
  n2 -->|"else"| n7
  n3 -->|"then"| n4
  n5 -->|"then"| n4
  n6 -->|"then"| n4
  n7 -->|"then"| n6
  n7 -->|"else"| n10
  n8 -->|"then"| n9
  n10 -->|"then"| n11
  n10 -->|"else"| n8
  n11 -->|"then"| n4
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | FindSpawnerByTable |  |
| `K2Node_BreakStruct_0` | Break Data Table Row Handle |  |
| `K2Node_PromotableOperator_0` | Equal (Object) | B=/Game/QuakeLike_1_0/Data/Item/DropTable/DT_HeldItemTable.DT_HeldItemTable |
| `K2Node_IfThenElse_0` | Branch |  |
| `K2Node_IfThenElse_1` | Branch |  |
| `K2Node_PromotableOperator_3` | Equal (Object) | B=/Game/QuakeLike_1_0/Data/Item/DropTable/DT_LifeItemTable.DT_LifeItemTable |
| `K2Node_VariableSet_0` | Set ActiveSpawner |  |
| `K2Node_FunctionResult_4` | 반환 노드 | isFind=true |
| `K2Node_VariableGet_1` | Get BPC_HeldItemSpawner |  |
| `K2Node_VariableSet_1` | Set ActiveSpawner |  |
| `K2Node_VariableSet_2` | Set ActiveSpawner |  |
| `K2Node_VariableGet_2` | Get BPC_LifeItemSpawner |  |
| `K2Node_VariableGet_3` | Get BPC_AmmoItemSpawner |  |
| `K2Node_IfThenElse_3` | Branch |  |
| `K2Node_PromotableOperator_1` | Equal (Object) | B=/Game/QuakeLike_1_0/Data/Item/DropTable/DT_AmmoItemTable.DT_AmmoItemTable |
| `K2Node_CallFunction_0` | Print String | InString=경고! BP_ItemSpawnPoint에 설정된 데이터 테이블이 잘못되었습니다!; bPrintToScreen=true; bPrintToLog=true; TextColor=(R=1.000000,G=0.000000,B=0.041791,A=1.000000); Duration=5.000000; Key=None |
| `K2Node_FunctionResult_2` | 반환 노드 | isFind=false |
| `K2Node_PromotableOperator_2` | Equal (Object) | B=/Game/QuakeLike_1_0/Data/Item/DropTable/DT_BuffTable.DT_BuffTable |
| `K2Node_IfThenElse_2` | Branch |  |
| `K2Node_VariableSet_3` | Set ActiveSpawner |  |
| `K2Node_VariableGet_0` | Get BPC_BuffSpawner |  |

</details>

### IsAvailable

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["IsAvailable"]
  n1["반환 노드"]
  n0 -->|"then"| n1
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | IsAvailable |  |
| `K2Node_FunctionResult_1` | 반환 노드 |  |
| `K2Node_VariableGet_0` | Get Having Item |  |
| `K2Node_CallFunction_0` | Is Valid |  |
| `K2Node_CallFunction_1` | NOT Boolean |  |

</details>

### UserConstructionScript

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["Construction Script"]
  n1["부모: Construction Script"]
  n0 -->|"then"| n1
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | Construction Script |  |
| `K2Node_CallParentFunction_0` | 부모: Construction Script |  |

</details>

## 조회 범위의 한계

ObjectIterator로 발견했으나 MCP helper가 반환하지 않은 그래프입니다. delegate signature·컴파일된 중간 그래프·macro·애니메이션 내부 그래프가 섞일 수 있습니다.

- `AssignItem`: Graph 'AssignItem' not found in Blueprint.
- `CheckAndSpawnRoutine`: Graph 'CheckAndSpawnRoutine' not found in Blueprint.
- `InitialBuffSpawn`: Graph 'InitialBuffSpawn' not found in Blueprint.
- `Multicast_PlaySpawnFeedback`: Graph 'Multicast_PlaySpawnFeedback' not found in Blueprint.
- `OnSpawnItemDestroyed`: Graph 'OnSpawnItemDestroyed' not found in Blueprint.
- `OnSpawnItemPickedUp`: Graph 'OnSpawnItemPickedUp' not found in Blueprint.
- `PlayPendingSpawnFeedback`: Graph 'PlayPendingSpawnFeedback' not found in Blueprint.
- `ReceiveBeginPlay`: Graph 'ReceiveBeginPlay' not found in Blueprint.
- `ReceiveTick`: Graph 'ReceiveTick' not found in Blueprint.
- `StartSpawnCycle`: Graph 'StartSpawnCycle' not found in Blueprint.

[원본 MCP 스냅샷](../../../../Evidence/blueprints-105.json)
