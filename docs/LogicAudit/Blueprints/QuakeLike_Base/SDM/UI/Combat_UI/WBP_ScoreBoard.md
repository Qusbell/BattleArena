# WBP_ScoreBoard

에셋: `/Game/QuakeLike_Base/SDM/UI/Combat_UI/WBP_ScoreBoard`

부모: `/Script/CoreUObject.Class'/Script/UMG.UserWidget'`

수집일: 2026-10-12. MCP에서 읽은 그래프·핀 연결의 자동 전사입니다. 상속된 그래프는 부모 문서를 함께 확인합니다.

정적 루트 참조 도달: 있음. 실행 여부 또는 삭제 가능 여부의 확정이 아닙니다.

컴포넌트 목록은 이 BP가 직접 정의한 SCS만 포함합니다. 부모·네이티브 생성 컴포넌트, 인스턴스 설정은 별도입니다.

## 변수와 기본값

| 이름 | 타입 | CDO 기본값 |
|---|---|---|
| `WeaponName` | `name` | None |
| `CurrentWeapon` | `BP_HeldItem_C` | None |
| `WeaponSlotArray` | `WBP_WeaponSlot_C` | [] |
| `PlayerInventory` | `BPC_Inventory_C` | None |
| `CastedPlayers` | `BP_QuakePlayerState_C` | [] |
| `SortedResult` | `BP_QuakePlayerState_C` | [] |

## 그래프 목록

| 그래프 | 전체 노드 수 |
|---|---|
| EventGraph | 37 |
| RefreshAllWeaponSlot | 25 |
| RefreshAmmoGauge | 14 |
| ResetAllSlot | 10 |
| SortPlayersByKill | 24 |

## 그래프별 실행 연결

실선은 출력 exec 핀의 실제 연결입니다. 여러 분기는 출력 핀 이름을 표시합니다. 함수 호출·delegate·타이머의 다음 그래프 실행은 이 그림에서 자동으로 이어 붙이지 않습니다.

순수 계산 노드 및 데이터 핀은 아래 목록과 원본 JSON을 확인합니다. 연결되지 않은 이벤트도 표시되므로, 노드가 있다는 이유만으로 실행된다고 판단하면 안 됩니다.

### EventGraph

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["UpdateScoreboard<br/>Custom Event"]
  n1["Clear Children<br/>타깃은 패널 위젯"]
  n2["Cast To BP_QuakeGameState"]
  n3["For Each Loop"]
  n4["Cast To BP_QuakePlayerState"]
  n5["Create WBP Scoreboard Row Widget"]
  n6["Set Row Data<br/>타깃은 WBP Scoreboard Row"]
  n7["Add Child<br/>타깃은 패널 위젯"]
  n8["Add"]
  n9["Sort Players by Kill<br/>타깃은 WBP Score Board"]
  n10["경유 노드"]
  n11["Set SortedResult"]
  n12["For Each Loop"]
  n13["경유 노드"]
  n14["Clear"]
  n15["Construct 이벤트"]
  n16["Cast To BP_QuakeGameState"]
  n17["OnAnyScoreChanged<br/>Custom Event"]
  n18["Destruct 이벤트"]
  n19["Cast To BP_QuakeGameState"]
  n20["Unbind Event from On Any Player Score Changed"]
  n21["Update Scoreboard<br/>타깃은 WBP Score Board"]
  n22["Bind Event to On Any Player Score Changed"]
  n0 -->|"then"| n1
  n1 -->|"then"| n14
  n2 -->|"then"| n3
  n3 -->|"LoopBody"| n4
  n3 -->|"Completed"| n10
  n4 -->|"then"| n8
  n5 -->|"then"| n6
  n6 -->|"then"| n7
  n9 -->|"then"| n11
  n10 -->|"OutputPin"| n13
  n11 -->|"then"| n12
  n12 -->|"LoopBody"| n5
  n13 -->|"OutputPin"| n9
  n14 -->|"then"| n2
  n15 -->|"then"| n16
  n16 -->|"then"| n22
  n17 -->|"then"| n21
  n18 -->|"then"| n19
  n19 -->|"then"| n20
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_CustomEvent_0` | UpdateScoreboard<br/>Custom Event |  |
| `K2Node_CallFunction_1` | Clear Children<br/>타깃은 패널 위젯 |  |
| `K2Node_CallFunction_2` | Get Game State |  |
| `K2Node_DynamicCast_0` | Cast To BP_QuakeGameState |  |
| `K2Node_VariableGet_2` | Get PlayerArray |  |
| `K2Node_MacroInstance_0` | For Each Loop |  |
| `K2Node_DynamicCast_1` | Cast To BP_QuakePlayerState |  |
| `K2Node_CreateWidget_1` | Create WBP Scoreboard Row Widget | Class=/Game/QuakeLike_Base/SDM/UI/Combat_UI/WBP_ScoreboardRow.WBP_ScoreboardRow_C |
| `K2Node_CallFunction_5` | Set Row Data<br/>타깃은 WBP Scoreboard Row |  |
| `K2Node_VariableGet_5` | Get VBox_List |  |
| `K2Node_CallFunction_6` | Add Child<br/>타깃은 패널 위젯 |  |
| `K2Node_VariableGet_4` | Get VBox_List |  |
| `EdGraphNode_Comment_9` | 스코어보드에 row(한 줄)씩 추가함 |  |
| `K2Node_Knot_4` | 경유 노드 |  |
| `K2Node_VariableGet_0` | Get CastedPlayers |  |
| `K2Node_CallArrayFunction_0` | Add | ReturnValue=0 |
| `K2Node_CallFunction_3` | Sort Players by Kill<br/>타깃은 WBP Score Board |  |
| `K2Node_Knot_5` | 경유 노드 |  |
| `K2Node_VariableSet_0` | Set SortedResult |  |
| `K2Node_MacroInstance_1` | For Each Loop |  |
| `K2Node_PromotableOperator_1` | int + int | B=1 |
| `K2Node_Knot_6` | 경유 노드 |  |
| `K2Node_Knot_8` | 경유 노드 |  |
| `K2Node_VariableGet_1` | Get CastedPlayers |  |
| `K2Node_CallArrayFunction_1` | Clear |  |
| `K2Node_CallFunction_8` | Get Game State |  |
| `K2Node_Event_1` | Construct 이벤트 |  |
| `K2Node_DynamicCast_5` | Cast To BP_QuakeGameState |  |
| `K2Node_CustomEvent_2` | OnAnyScoreChanged<br/>Custom Event |  |
| `K2Node_Event_0` | Destruct 이벤트 |  |
| `K2Node_CallFunction_0` | Get Game State |  |
| `K2Node_DynamicCast_3` | Cast To BP_QuakeGameState |  |
| `K2Node_RemoveDelegate_0` | Unbind Event from On Any Player Score Changed |  |
| `K2Node_CallFunction_9` | Update Scoreboard<br/>타깃은 WBP Score Board |  |
| `K2Node_AddDelegate_0` | Bind Event to On Any Player Score Changed |  |
| `K2Node_VariableGet_3` | Get CampaignName |  |
| `K2Node_CallFunction_4` | Is Empty | ReturnValue=false |

</details>

### RefreshAllWeaponSlot

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["RefreshAllWeaponSlot"]
  n1["반환 노드"]
  n2["For Each Loop"]
  n3["Get Item ID<br/>Target is BPI Held Item"]
  n4["Has Item<br/>Target is BPI Inventory"]
  n5["Branch"]
  n6["Update Ammo Gauge<br/>타깃은 WBP Weapon Slot"]
  n7["Get Usage Count<br/>Target is BPI Usable"]
  n8["Set Slot Active<br/>타깃은 WBP Weapon Slot"]
  n9["Update Ammo Gauge<br/>타깃은 WBP Weapon Slot"]
  n10["Branch"]
  n11["Set Slot Active<br/>타깃은 WBP Weapon Slot"]
  n12["Set Slot Active<br/>타깃은 WBP Weapon Slot"]
  n0 -->|"then"| n3
  n2 -->|"LoopBody"| n4
  n2 -->|"Completed"| n1
  n3 -->|"then"| n2
  n4 -->|"then"| n5
  n5 -->|"then"| n10
  n5 -->|"else"| n8
  n7 -->|"then"| n6
  n8 -->|"then"| n9
  n10 -->|"then"| n11
  n10 -->|"else"| n12
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | RefreshAllWeaponSlot |  |
| `K2Node_FunctionResult_0` | 반환 노드 |  |
| `K2Node_VariableGet_11` | Get WeaponSlotArray |  |
| `K2Node_MacroInstance_5` | For Each Loop |  |
| `K2Node_Message_2` | Get Item ID<br/>Target is BPI Held Item |  |
| `K2Node_VariableGet_12` | Get SlotWeaponID |  |
| `K2Node_PromotableOperator_1` | Equal (GameplayTag) |  |
| `K2Node_Knot_7` | 경유 노드 |  |
| `K2Node_Message_1` | Has Item<br/>Target is BPI Inventory |  |
| `K2Node_IfThenElse_0` | Branch |  |
| `K2Node_VariableGet_1` | Get PlayerInventory |  |
| `K2Node_Knot_3` | 경유 노드 |  |
| `K2Node_CreateDelegate_0` | 이벤트 생성 |  |
| `K2Node_CallFunction_5` | Update Ammo Gauge<br/>타깃은 WBP Weapon Slot |  |
| `K2Node_Message_3` | Get Usage Count<br/>Target is BPI Usable |  |
| `K2Node_Knot_4` | 경유 노드 |  |
| `K2Node_CallFunction_7` | Set Slot Active<br/>타깃은 WBP Weapon Slot | IsActive=false; IsOwned=false |
| `K2Node_Knot_6` | 경유 노드 |  |
| `K2Node_Knot_8` | 경유 노드 |  |
| `K2Node_CallFunction_0` | Update Ammo Gauge<br/>타깃은 WBP Weapon Slot | CurrentAmmo=0; MaxAmmo=0 |
| `K2Node_Knot_0` | 경유 노드 |  |
| `K2Node_IfThenElse_1` | Branch |  |
| `K2Node_CallFunction_3` | Set Slot Active<br/>타깃은 WBP Weapon Slot | IsOwned=true |
| `K2Node_CallFunction_4` | Set Slot Active<br/>타깃은 WBP Weapon Slot | IsActive=false; IsOwned=true |
| `K2Node_Knot_1` | 경유 노드 |  |

</details>

### RefreshAmmoGauge

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["RefreshAmmoGauge"]
  n1["For Each Loop"]
  n2["Update Ammo Gauge<br/>타깃은 WBP Weapon Slot"]
  n3["Branch"]
  n4["반환 노드"]
  n0 -->|"then"| n1
  n1 -->|"LoopBody"| n3
  n1 -->|"Completed"| n4
  n3 -->|"then"| n2
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | RefreshAmmoGauge |  |
| `K2Node_VariableGet_6` | Get WeaponSlotArray |  |
| `K2Node_MacroInstance_1` | For Each Loop |  |
| `K2Node_VariableGet_7` | Get SlotWeaponID |  |
| `K2Node_CallFunction_4` | Update Ammo Gauge<br/>타깃은 WBP Weapon Slot |  |
| `K2Node_IfThenElse_0` | Branch |  |
| `K2Node_Knot_11` | 경유 노드 |  |
| `K2Node_Knot_16` | 경유 노드 |  |
| `K2Node_Knot_0` | 경유 노드 |  |
| `K2Node_Knot_1` | 경유 노드 |  |
| `K2Node_FunctionResult_0` | 반환 노드 |  |
| `K2Node_CallFunction_0` | Has Tag | bExactMatch=true |
| `K2Node_Knot_3` | 경유 노드 |  |
| `K2Node_Knot_4` | 경유 노드 |  |

</details>

### ResetAllSlot

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["ResetAllSlot"]
  n1["For Each Loop"]
  n2["Update Ammo Gauge<br/>타깃은 WBP Weapon Slot"]
  n3["Set Slot Active<br/>타깃은 WBP Weapon Slot"]
  n4["Get Usage Count<br/>Target is BPI Usable"]
  n5["Branch"]
  n6["Has Item<br/>Target is BPI Inventory"]
  n0 -->|"then"| n1
  n1 -->|"LoopBody"| n6
  n2 -->|"then"| n3
  n4 -->|"then"| n2
  n5 -->|"then"| n4
  n6 -->|"then"| n5
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | ResetAllSlot |  |
| `K2Node_VariableGet_0` | Get WeaponSlotArray |  |
| `K2Node_MacroInstance_0` | For Each Loop |  |
| `K2Node_CallFunction_0` | Update Ammo Gauge<br/>타깃은 WBP Weapon Slot |  |
| `K2Node_CallFunction_1` | Set Slot Active<br/>타깃은 WBP Weapon Slot | IsActive=false; IsOwned=false |
| `K2Node_VariableGet_2` | Get SlotWeaponID |  |
| `K2Node_Message_2` | Get Usage Count<br/>Target is BPI Usable |  |
| `K2Node_IfThenElse_0` | Branch |  |
| `K2Node_Knot_1` | 경유 노드 |  |
| `K2Node_Message_1` | Has Item<br/>Target is BPI Inventory |  |

</details>

### SortPlayersByKill

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["SortPlayersByKill"]
  n1["반환 노드"]
  n2["Set SortedPlayers"]
  n3["For Loop"]
  n4["For Loop"]
  n5["Branch"]
  n6["Swap Array Elements"]
  n0 -->|"then"| n2
  n2 -->|"then"| n3
  n3 -->|"LoopBody"| n4
  n3 -->|"Completed"| n1
  n4 -->|"LoopBody"| n5
  n5 -->|"then"| n6
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | SortPlayersByKill |  |
| `K2Node_FunctionResult_0` | 반환 노드 |  |
| `K2Node_VariableSet_0` | Set SortedPlayers |  |
| `K2Node_MacroInstance_1` | For Loop |  |
| `K2Node_VariableGet_0` | Get SortedPlayers |  |
| `K2Node_CallArrayFunction_0` | Length |  |
| `K2Node_PromotableOperator_0` | int - int | B=1 |
| `K2Node_MacroInstance_2` | For Loop |  |
| `K2Node_VariableGet_1` | Get SortedPlayers |  |
| `K2Node_CallArrayFunction_1` | Length |  |
| `K2Node_PromotableOperator_1` | int - int | B=2 |
| `K2Node_VariableGet_2` | Get SortedPlayers |  |
| `K2Node_GetArrayItem_0` | GET |  |
| `K2Node_PromotableOperator_2` | int + int | B=1 |
| `K2Node_VariableGet_8` | Get SortedPlayers |  |
| `K2Node_GetArrayItem_4` | GET |  |
| `K2Node_VariableGet_9` | Get KillCount |  |
| `K2Node_VariableGet_10` | Get KillCount |  |
| `K2Node_PromotableOperator_6` | integer &lt; integer |  |
| `K2Node_IfThenElse_0` | Branch |  |
| `K2Node_CallArrayFunction_5` | Swap Array Elements |  |
| `K2Node_VariableGet_11` | Get SortedPlayers |  |
| `K2Node_Knot_2` | 경유 노드 |  |
| `K2Node_VariableGet_12` | Get SortedPlayers |  |

</details>

## 조회 범위의 한계

ObjectIterator로 발견했으나 MCP helper가 반환하지 않은 그래프입니다. delegate signature·컴파일된 중간 그래프·macro·애니메이션 내부 그래프가 섞일 수 있습니다.

- `Construct`: Graph 'Construct' not found in Blueprint.
- `Destruct`: Graph 'Destruct' not found in Blueprint.
- `OnAnyScoreChanged`: Graph 'OnAnyScoreChanged' not found in Blueprint.
- `UpdateScoreboard`: Graph 'UpdateScoreboard' not found in Blueprint.

[원본 MCP 스냅샷](../../../../../Evidence/blueprints-035.json)
