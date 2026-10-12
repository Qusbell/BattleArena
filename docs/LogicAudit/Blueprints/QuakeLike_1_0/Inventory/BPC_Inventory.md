# BPC_Inventory

에셋: `/Game/QuakeLike_1_0/Inventory/BPC_Inventory`

부모: `/Script/CoreUObject.Class'/Script/Engine.ActorComponent'`

수집일: 2026-10-12. MCP에서 읽은 그래프·핀 연결의 자동 전사입니다. 상속된 그래프는 부모 문서를 함께 확인합니다.

정적 루트 참조 도달: 있음. 실행 여부 또는 삭제 가능 여부의 확정이 아닙니다.

컴포넌트 목록은 이 BP가 직접 정의한 SCS만 포함합니다. 부모·네이티브 생성 컴포넌트, 인스턴스 설정은 별도입니다.

## 변수와 기본값

| 이름 | 타입 | CDO 기본값 |
|---|---|---|
| `All_Items` | `BP_HeldItem_C` | [] |
| `OnPickupItem` | `mcdelegate` | &lt;Delegate 'DelegateBase' &lt;Unbound&gt;&gt; |
| `OnSwapItem` | `mcdelegate` | &lt;Delegate 'DelegateBase' &lt;Unbound&gt;&gt; |
| `IndexInfo` | `ST_InventoryInfo` | &lt;Struct 'ST_InventoryInfo' {}&gt; |

## 그래프 목록

| 그래프 | 전체 노드 수 |
|---|---|
| AITrySwap | 12 |
| EventGraph | 52 |
| Get Next Valid Index | 42 |
| Get Shifted Index | 8 |
| GetBeforeItem | 10 |
| IsValidIndex | 12 |
| OnRep_All_Items | 6 |
| OnRep_IndexInfo | 11 |
| SetIndex | 7 |
| TrySwap | 17 |

## 그래프별 실행 연결

실선은 출력 exec 핀의 실제 연결입니다. 여러 분기는 출력 핀 이름을 표시합니다. 함수 호출·delegate·타이머의 다음 그래프 실행은 이 그림에서 자동으로 이어 붙이지 않습니다.

순수 계산 노드 및 데이터 핀은 아래 목록과 원본 JSON을 확인합니다. 연결되지 않은 이벤트도 표시되므로, 노드가 있다는 이유만으로 실행된다고 판단하면 안 됩니다.

### AITrySwap

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["AITrySwap"]
  n1["For Each Loop"]
  n2["Is Valid"]
  n3["Check Item ID<br/>타깃은 BPI Held Item"]
  n4["Branch"]
  n5["Swap Item<br/>타깃은 BPC Inventory"]
  n6["반환 노드"]
  n7["반환 노드"]
  n0 -->|"then"| n1
  n1 -->|"LoopBody"| n2
  n1 -->|"Completed"| n7
  n2 -->|"Is Valid"| n3
  n3 -->|"then"| n4
  n4 -->|"then"| n5
  n5 -->|"then"| n6
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_1` | AITrySwap |  |
| `K2Node_MacroInstance_1` | For Each Loop |  |
| `K2Node_VariableGet_0` | Get All_Items |  |
| `K2Node_MacroInstance_2` | Is Valid |  |
| `K2Node_Knot_0` | 경유 노드 |  |
| `K2Node_CallFunction_2` | Check Item ID<br/>타깃은 BPI Held Item |  |
| `K2Node_Knot_1` | 경유 노드 |  |
| `K2Node_Knot_2` | 경유 노드 |  |
| `K2Node_IfThenElse_2` | Branch |  |
| `K2Node_CallFunction_3` | Swap Item<br/>타깃은 BPC Inventory |  |
| `K2Node_FunctionResult_1` | 반환 노드 | Success=true |
| `K2Node_FunctionResult_0` | 반환 노드 | Success=false |

</details>

### EventGraph

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["Branch"]
  n1["Pickup 이벤트<br/>BPI Inventory에서"]
  n2["SwapItem 이벤트<br/>BPI Inventory에서"]
  n3["Is Valid"]
  n4["Recharge 이벤트<br/>BPI Inventory에서"]
  n5["Has Item<br/>타깃은 BPC Inventory"]
  n6["Has Item<br/>타깃은 BPC Inventory"]
  n7["Branch"]
  n8["SwapNext 이벤트<br/>BPI Inventory에서"]
  n9["SwapPrev 이벤트<br/>BPI Inventory에서"]
  n10["Clear 이벤트<br/>BPI Inventory에서"]
  n11["For Each Loop"]
  n12["Destroy Actor<br/>타깃은 액터"]
  n13["Set Index<br/>타깃은 BPC Inventory"]
  n14["Begin Play 이벤트"]
  n15["Bind Event to On Swap Item"]
  n16["Swap<br/>Custom Event"]
  n17["Get Before Item<br/>타깃은 BPC Inventory"]
  n18["Get Selected Item<br/>타깃은 BPC Inventory"]
  n19["시퀀스"]
  n20["Switch Has Authority"]
  n21["Swap Item<br/>타깃은 BPC Inventory"]
  n22["Swap Item<br/>타깃은 BPC Inventory"]
  n23["Try Swap<br/>타깃은 BPC Inventory"]
  n24["Equip<br/>Target is BPI Held Item"]
  n25["Unequip<br/>Target is BPI Held Item"]
  n26["Get Item ID<br/>Target is BPI Held Item"]
  n27["Recover<br/>Target is BPI Usable"]
  n28["Recover<br/>Target is BPI Usable"]
  n29["Destroy Actor<br/>타깃은 액터"]
  n30["Set Array Elem"]
  n31["Is Valid"]
  n32["Get Slot Number<br/>타깃은 BPI Held Item"]
  n0 -->|"then"| n28
  n0 -->|"else"| n32
  n1 -->|"then"| n20
  n2 -->|"then"| n23
  n3 -->|"Is Valid"| n26
  n4 -->|"then"| n5
  n5 -->|"then"| n7
  n6 -->|"then"| n0
  n7 -->|"then"| n27
  n8 -->|"then"| n21
  n9 -->|"then"| n22
  n10 -->|"then"| n11
  n11 -->|"LoopBody"| n31
  n14 -->|"then"| n15
  n16 -->|"then"| n19
  n17 -->|"then"| n25
  n18 -->|"then"| n24
  n19 -->|"then_0"| n17
  n19 -->|"then_1"| n18
  n20 -->|"Authority"| n3
  n26 -->|"then"| n6
  n28 -->|"then"| n29
  n30 -->|"then"| n13
  n31 -->|"Is Valid"| n12
  n32 -->|"then"| n30
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_IfThenElse_2` | Branch |  |
| `K2Node_Event_9` | Pickup 이벤트<br/>BPI Inventory에서 |  |
| `K2Node_Event_6` | SwapItem 이벤트<br/>BPI Inventory에서 |  |
| `K2Node_Knot_46` | 경유 노드 |  |
| `K2Node_Knot_47` | 경유 노드 |  |
| `K2Node_MacroInstance_2` | Is Valid |  |
| `EdGraphNode_Comment_0` | RPC설정 문제로 동기화가 잘 안되어있을 수 있음. |  |
| `K2Node_Event_0` | Recharge 이벤트<br/>BPI Inventory에서 |  |
| `K2Node_CallFunction_3` | Has Item<br/>타깃은 BPC Inventory |  |
| `K2Node_CallFunction_28` | Has Item<br/>타깃은 BPC Inventory |  |
| `K2Node_Knot_0` | 경유 노드 |  |
| `K2Node_IfThenElse_3` | Branch |  |
| `K2Node_Knot_7` | 경유 노드 |  |
| `K2Node_Knot_8` | 경유 노드 |  |
| `K2Node_Knot_10` | 경유 노드 |  |
| `K2Node_Knot_3` | 경유 노드 |  |
| `K2Node_Knot_6` | 경유 노드 |  |
| `K2Node_Event_1` | SwapNext 이벤트<br/>BPI Inventory에서 |  |
| `K2Node_Event_2` | SwapPrev 이벤트<br/>BPI Inventory에서 |  |
| `K2Node_Event_3` | Clear 이벤트<br/>BPI Inventory에서 |  |
| `K2Node_MacroInstance_1` | For Each Loop |  |
| `K2Node_VariableGet_3` | Get All_Items |  |
| `K2Node_CallFunction_12` | Destroy Actor<br/>타깃은 액터 |  |
| `K2Node_CallFunction_10` | Set Index<br/>타깃은 BPC Inventory | isPickup=true |
| `K2Node_Event_4` | Begin Play 이벤트 |  |
| `K2Node_AddDelegate_0` | Bind Event to On Swap Item |  |
| `K2Node_CustomEvent_0` | Swap<br/>Custom Event |  |
| `K2Node_CallFunction_15` | Get Before Item<br/>타깃은 BPC Inventory |  |
| `K2Node_CallFunction_2` | Get Selected Item<br/>타깃은 BPC Inventory |  |
| `K2Node_ExecutionSequence_0` | 시퀀스 |  |
| `K2Node_MacroInstance_0` | Switch Has Authority |  |
| `K2Node_CallFunction_8` | Swap Item<br/>타깃은 BPC Inventory |  |
| `K2Node_CallFunction_4` | Swap Item<br/>타깃은 BPC Inventory |  |
| `K2Node_CallFunction_16` | Try Swap<br/>타깃은 BPC Inventory |  |
| `K2Node_Message_1` | Equip<br/>Target is BPI Held Item |  |
| `K2Node_Message_3` | Unequip<br/>Target is BPI Held Item |  |
| `K2Node_Message_7` | Get Item ID<br/>Target is BPI Held Item |  |
| `K2Node_Message_8` | Recover<br/>Target is BPI Usable |  |
| `K2Node_Message_9` | Recover<br/>Target is BPI Usable | recoverCount=30 |
| `K2Node_Knot_4` | 경유 노드 |  |
| `K2Node_CallFunction_1` | Destroy Actor<br/>타깃은 액터 |  |
| `K2Node_Knot_5` | 경유 노드 |  |
| `K2Node_CallArrayFunction_2` | Set Array Elem | bSizeToFit=true |
| `K2Node_CallFunction_7` | Get Next Valid Index | Is Next=true |
| `K2Node_CallFunction_5` | Get Next Valid Index | Is Next=false |
| `K2Node_Knot_1` | 경유 노드 |  |
| `K2Node_MacroInstance_3` | Is Valid |  |
| `K2Node_CallFunction_0` | Get Slot Number<br/>타깃은 BPI Held Item |  |
| `K2Node_VariableGet_1` | Get All_Items |  |
| `K2Node_Knot_2` | 경유 노드 |  |
| `K2Node_Knot_9` | 경유 노드 |  |
| `K2Node_Knot_11` | 경유 노드 |  |

</details>

### Get Next Valid Index

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["Get Next Valid Index"]
  n1["Set StartIndex"]
  n2["For Loop with Break"]
  n3["반환 노드"]
  n4["Set next"]
  n5["Branch"]
  n6["Set ResultIndex"]
  n7["경유 노드"]
  n8["Set ResultIndex"]
  n9["Branch"]
  n10["경유 노드"]
  n11["경유 노드"]
  n12["경유 노드"]
  n13["경유 노드"]
  n14["Get Usage Count<br/>Target is BPI Usable"]
  n15["Branch"]
  n0 -->|"then"| n4
  n1 -->|"then"| n2
  n2 -->|"LoopBody"| n5
  n2 -->|"Completed"| n3
  n4 -->|"then"| n1
  n5 -->|"then"| n7
  n5 -->|"else"| n8
  n6 -->|"then"| n11
  n7 -->|"OutputPin"| n6
  n8 -->|"then"| n10
  n9 -->|"then"| n14
  n10 -->|"OutputPin"| n9
  n11 -->|"OutputPin"| n9
  n12 -->|"OutputPin"| n13
  n13 -->|"OutputPin"| n2
  n14 -->|"then"| n15
  n15 -->|"then"| n12
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | Get Next Valid Index |  |
| `K2Node_VariableGet_0` | Get |  |
| `K2Node_VariableSet_1` | Set StartIndex | Output_Get=0 |
| `K2Node_MacroInstance_2` | For Loop with Break | FirstIndex=1 |
| `K2Node_FunctionResult_1` | 반환 노드 |  |
| `K2Node_VariableGet_3` | Get All_Items |  |
| `K2Node_CallArrayFunction_0` | Length |  |
| `K2Node_VariableGet_17` | Get StartIndex |  |
| `K2Node_VariableSet_5` | Set next | Output_Get=false |
| `K2Node_VariableGet_16` | Get next |  |
| `K2Node_IfThenElse_1` | Branch |  |
| `K2Node_PromotableOperator_5` | int + int |  |
| `K2Node_Knot_5` | 경유 노드 |  |
| `K2Node_VariableGet_12` | Get StartIndex |  |
| `K2Node_Knot_4` | 경유 노드 |  |
| `K2Node_PromotableOperator_6` | int - int |  |
| `K2Node_PromotableOperator_7` | int + int |  |
| `K2Node_VariableGet_19` | Get All_Items |  |
| `K2Node_CallArrayFunction_3` | Length |  |
| `K2Node_Knot_6` | 경유 노드 |  |
| `K2Node_Knot_7` | 경유 노드 |  |
| `K2Node_CallFunction_5` | % (Integer) |  |
| `K2Node_VariableGet_14` | Get All_Items |  |
| `K2Node_CallArrayFunction_1` | Length |  |
| `K2Node_CallFunction_4` | % (Integer) |  |
| `K2Node_VariableSet_6` | Set ResultIndex | Output_Get=0 |
| `K2Node_Knot_8` | 경유 노드 |  |
| `K2Node_VariableSet_7` | Set ResultIndex | Output_Get=0 |
| `K2Node_Knot_9` | 경유 노드 |  |
| `K2Node_CallFunction_7` | Is Valid Index |  |
| `K2Node_VariableGet_20` | Get ResultIndex |  |
| `K2Node_IfThenElse_3` | Branch |  |
| `K2Node_Knot_10` | 경유 노드 |  |
| `K2Node_Knot_11` | 경유 노드 |  |
| `K2Node_Knot_12` | 경유 노드 |  |
| `K2Node_Knot_13` | 경유 노드 |  |
| `K2Node_Knot_14` | 경유 노드 |  |
| `K2Node_VariableGet_21` | Get ResultIndex |  |
| `K2Node_Message_0` | Get Usage Count<br/>Target is BPI Usable | maxCount=0 |
| `K2Node_PromotableOperator_1` | integer &gt; integer |  |
| `K2Node_IfThenElse_2` | Branch |  |
| `K2Node_Knot_0` | 경유 노드 |  |

</details>

### Get Shifted Index

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["Get Shifted Index"]
  n1["반환 노드"]
  n0 -->|"then"| n1
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | Get Shifted Index |  |
| `K2Node_VariableGet_0` | Get All_Items |  |
| `K2Node_CallArrayFunction_1` | Length |  |
| `K2Node_PromotableOperator_1` | int + int |  |
| `K2Node_CallFunction_1` | % (Integer) |  |
| `K2Node_FunctionResult_1` | 반환 노드 |  |
| `K2Node_Knot_0` | 경유 노드 |  |
| `K2Node_VariableGet_1` | Get |  |

</details>

### GetBeforeItem

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["GetBeforeItem"]
  n1["반환 노드"]
  n2["Branch"]
  n0 -->|"then"| n2
  n2 -->|"then"| n1
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | GetBeforeItem |  |
| `K2Node_FunctionResult_0` | 반환 노드 |  |
| `K2Node_VariableGet_0` | Get |  |
| `K2Node_VariableGet_1` | Get All_Items |  |
| `K2Node_CallArrayFunction_0` | Is Valid Index |  |
| `K2Node_Knot_1` | 경유 노드 |  |
| `K2Node_Knot_2` | 경유 노드 |  |
| `K2Node_GetArrayItem_1` | GET |  |
| `K2Node_Knot_3` | 경유 노드 |  |
| `K2Node_IfThenElse_0` | Branch |  |

</details>

### IsValidIndex

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["IsValidIndex"]
  n1["반환 노드"]
  n2["Branch"]
  n3["반환 노드"]
  n0 -->|"then"| n2
  n2 -->|"then"| n1
  n2 -->|"else"| n3
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | IsValidIndex |  |
| `K2Node_VariableGet_0` | Get All_Items |  |
| `K2Node_CallArrayFunction_0` | Is Valid Index |  |
| `K2Node_Knot_0` | 경유 노드 |  |
| `K2Node_Knot_1` | 경유 노드 |  |
| `K2Node_GetArrayItem_0` | GET |  |
| `K2Node_CallFunction_0` | Is Valid |  |
| `K2Node_FunctionResult_0` | 반환 노드 |  |
| `K2Node_Knot_3` | 경유 노드 |  |
| `K2Node_Knot_4` | 경유 노드 |  |
| `K2Node_IfThenElse_0` | Branch |  |
| `K2Node_FunctionResult_1` | 반환 노드 | valid=false |

</details>

### OnRep_All_Items

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["OnRep_All_Items"]
  n1["Call On Pickup Item"]
  n2["Is Valid"]
  n0 -->|"then"| n2
  n2 -->|"Is Valid"| n1
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | OnRep_All_Items |  |
| `K2Node_CallDelegate_1` | Call On Pickup Item |  |
| `K2Node_VariableGet_0` | Get All_Items |  |
| `K2Node_GetArrayItem_0` | GET |  |
| `K2Node_CallArrayFunction_0` | Last Index |  |
| `K2Node_MacroInstance_0` | Is Valid |  |

</details>

### OnRep_IndexInfo

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["OnRep_IndexInfo"]
  n1["Branch"]
  n2["Call On Pickup Item"]
  n3["Call On Swap Item"]
  n4["Get Selected Item<br/>타깃은 BPC Inventory"]
  n5["Is Valid"]
  n6["Call On Swap Item"]
  n0 -->|"then"| n4
  n1 -->|"then"| n2
  n1 -->|"else"| n3
  n2 -->|"then"| n6
  n4 -->|"then"| n5
  n5 -->|"Is Valid"| n1
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | OnRep_IndexInfo |  |
| `K2Node_VariableGet_0` | Get |  |
| `K2Node_IfThenElse_0` | Branch |  |
| `K2Node_Knot_5` | 경유 노드 |  |
| `K2Node_CallDelegate_0` | Call On Pickup Item |  |
| `K2Node_CallDelegate_2` | Call On Swap Item |  |
| `K2Node_CallFunction_0` | Get Selected Item<br/>타깃은 BPC Inventory |  |
| `K2Node_MacroInstance_1` | Is Valid |  |
| `K2Node_Knot_6` | 경유 노드 |  |
| `K2Node_CallDelegate_1` | Call On Swap Item |  |
| `K2Node_Knot_7` | 경유 노드 |  |

</details>

### SetIndex

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["SetIndex"]
  n1["노티파이로 설정"]
  n2["Reconcile Standalone Equip"]
  n0 -->|"then"| n1
  n1 -->|"then"| n2
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | SetIndex |  |
| `K2Node_VariableGet_1` | Get |  |
| `K2Node_Knot_0` | 경유 노드 |  |
| `K2Node_Knot_1` | 경유 노드 |  |
| `K2Node_VariableSet_0` | 노티파이로 설정 |  |
| `K2Node_Knot_2` | 경유 노드 |  |
| `K2Node_CallFunction_0` | Reconcile Standalone Equip |  |

</details>

### TrySwap

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["TrySwap"]
  n1["Branch"]
  n2["Get Selected Item<br/>타깃은 BPC Inventory"]
  n3["Branch"]
  n4["Set Index<br/>타깃은 BPC Inventory"]
  n5["Switch Has Authority"]
  n6["Get Usage Count<br/>Target is BPI Usable"]
  n7["Branch"]
  n0 -->|"then"| n5
  n1 -->|"then"| n2
  n2 -->|"then"| n3
  n3 -->|"then"| n6
  n5 -->|"Authority"| n1
  n6 -->|"then"| n7
  n7 -->|"then"| n4
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | TrySwap |  |
| `K2Node_CallFunction_0` | Is Valid Index |  |
| `K2Node_IfThenElse_0` | Branch |  |
| `K2Node_CallFunction_1` | Get Selected Item<br/>타깃은 BPC Inventory |  |
| `K2Node_PromotableOperator_0` | Not Equal (Object) |  |
| `K2Node_IfThenElse_1` | Branch |  |
| `K2Node_Knot_4` | 경유 노드 |  |
| `K2Node_Knot_5` | 경유 노드 |  |
| `K2Node_CallFunction_4` | Set Index<br/>타깃은 BPC Inventory | isPickup=false |
| `K2Node_Knot_9` | 경유 노드 |  |
| `K2Node_MacroInstance_0` | Switch Has Authority |  |
| `K2Node_Message_0` | Get Usage Count<br/>Target is BPI Usable | maxCount=0 |
| `K2Node_PromotableOperator_1` | integer &gt; integer |  |
| `K2Node_IfThenElse_2` | Branch |  |
| `K2Node_Knot_1` | 경유 노드 |  |
| `K2Node_Knot_2` | 경유 노드 |  |
| `K2Node_Knot_3` | 경유 노드 |  |

</details>

## 조회 범위의 한계

ObjectIterator로 발견했으나 MCP helper가 반환하지 않은 그래프입니다. delegate signature·컴파일된 중간 그래프·macro·애니메이션 내부 그래프가 섞일 수 있습니다.

- `Clear`: Graph 'Clear' not found in Blueprint.
- `GetSelectedItem`: Graph 'GetSelectedItem' not found in Blueprint.
- `HasItem`: Graph 'HasItem' not found in Blueprint.
- `OnPickupItem`: Graph 'OnPickupItem' not found in Blueprint.
- `OnSwapItem`: Graph 'OnSwapItem' not found in Blueprint.
- `Pickup`: Graph 'Pickup' not found in Blueprint.
- `ReceiveBeginPlay`: Graph 'ReceiveBeginPlay' not found in Blueprint.
- `Recharge`: Graph 'Recharge' not found in Blueprint.
- `Swap`: Graph 'Swap' not found in Blueprint.
- `SwapItem`: Graph 'SwapItem' not found in Blueprint.
- `SwapItemByTag`: Graph 'SwapItemByTag' not found in Blueprint.
- `SwapNext`: Graph 'SwapNext' not found in Blueprint.
- `SwapPrev`: Graph 'SwapPrev' not found in Blueprint.

[원본 MCP 스냅샷](../../../Evidence/blueprints-070.json)
