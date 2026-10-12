# BP_HoldableItem

에셋: `/Game/QuakeLike_1_0/DropItem/BP_HoldableItem`

부모: `/Script/Engine.BlueprintGeneratedClass'/Game/QuakeLike_1_0/DropItem/BP_DropItem.BP_DropItem_C'`

수집일: 2026-10-12. MCP에서 읽은 그래프·핀 연결의 자동 전사입니다. 상속된 그래프는 부모 문서를 함께 확인합니다.

정적 루트 참조 도달: 있음. 실행 여부 또는 삭제 가능 여부의 확정이 아닙니다.

컴포넌트 목록은 이 BP가 직접 정의한 SCS만 포함합니다. 부모·네이티브 생성 컴포넌트, 인스턴스 설정은 별도입니다.

## 그래프 목록

| 그래프 | 전체 노드 수 |
|---|---|
| ApplyItemEffect | 25 |
| EventGraph | 7 |
| Get Attribute Tags | 11 |
| GetPickSound | 6 |
| UserConstructionScript | 2 |

## 그래프별 실행 연결

실선은 출력 exec 핀의 실제 연결입니다. 여러 분기는 출력 핀 이름을 표시합니다. 함수 호출·delegate·타이머의 다음 그래프 실행은 이 그림에서 자동으로 이어 붙이지 않습니다.

순수 계산 노드 및 데이터 핀은 아래 목록과 원본 JSON을 확인합니다. 연결되지 않은 이벤트도 표시되므로, 노드가 있다는 이유만으로 실행된다고 판단하면 안 됩니다.

### ApplyItemEffect

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["ApplyItemEffect"]
  n1["반환 노드"]
  n2["Pickup<br/>Target is BPI Inventory"]
  n3["Is Valid"]
  n4["반환 노드"]
  n5["스폰 액터"]
  n6["Set Actor Hidden In Game<br/>타깃은 액터"]
  n7["데이터 테이블 행 가져오기"]
  n8["Set bSuppressEquipSoundForPickup"]
  n9["Set bSuppressEquipSoundForPickup"]
  n10["Is Valid"]
  n11["반환 노드"]
  n0 -->|"then"| n3
  n2 -->|"then"| n10
  n3 -->|"Is Valid"| n7
  n3 -->|"Is Not Valid"| n4
  n5 -->|"then"| n6
  n6 -->|"then"| n8
  n7 -->|"then"| n5
  n8 -->|"then"| n2
  n9 -->|"then"| n1
  n10 -->|"Is Valid"| n9
  n10 -->|"Is Not Valid"| n11
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | ApplyItemEffect |  |
| `K2Node_FunctionResult_2` | 반환 노드 | Succees=true |
| `K2Node_Message_0` | Pickup<br/>Target is BPI Inventory |  |
| `K2Node_CallFunction_0` | Get Component by Class<br/>타깃은 액터 | ComponentClass=/Game/QuakeLike_1_0/Inventory/BPC_Inventory.BPC_Inventory_C |
| `K2Node_MacroInstance_0` | Is Valid |  |
| `K2Node_FunctionResult_0` | 반환 노드 | Succees=false |
| `K2Node_Knot_0` | 경유 노드 |  |
| `K2Node_Knot_1` | 경유 노드 |  |
| `K2Node_SpawnActorFromClass_0` | 스폰 액터 | CollisionHandlingOverride=AlwaysSpawn; TransformScaleMethod=MultiplyWithRoot; bUseScope=False; bZoomIn=True; itemInfo_IsEquip_8_458BE63E4B8C61048CC8FE928CB6D8E3=False |
| `K2Node_CallFunction_1` | Get Actor Transform<br/>타깃은 액터 |  |
| `K2Node_Knot_2` | 경유 노드 |  |
| `K2Node_CallFunction_2` | Set Actor Hidden In Game<br/>타깃은 액터 | bNewHidden=true |
| `K2Node_Knot_3` | 경유 노드 |  |
| `K2Node_Knot_4` | 경유 노드 |  |
| `K2Node_BreakStruct_0` | Break Data Table Row Handle |  |
| `K2Node_GetDataTableRow_0` | 데이터 테이블 행 가져오기 |  |
| `K2Node_BreakStruct_1` | Break ST Held Item Data |  |
| `K2Node_Knot_5` | 경유 노드 |  |
| `K2Node_VariableGet_3` | Get DataTable Handle |  |
| `K2Node_VariableSet_0` | Set bSuppressEquipSoundForPickup | bSuppressEquipSoundForPickup=true; Output_Get=false |
| `K2Node_VariableSet_1` | Set bSuppressEquipSoundForPickup | bSuppressEquipSoundForPickup=false; Output_Get=false |
| `K2Node_MacroInstance_1` | Is Valid |  |
| `K2Node_FunctionResult_1` | 반환 노드 | Succees=true |
| `K2Node_Knot_6` | 경유 노드 |  |
| `K2Node_Knot_7` | 경유 노드 |  |

</details>

### EventGraph

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["데이터 테이블 행 가져오기"]
  n1["Set Static Mesh<br/>타깃은 스태틱 메시 컴포넌트"]
  n2["Setup Mesh 이벤트"]
  n0 -->|"then"| n1
  n2 -->|"then"| n0
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_BreakStruct_0` | Break Data Table Row Handle |  |
| `K2Node_BreakStruct_1` | Break ST Held Item Data |  |
| `K2Node_GetDataTableRow_0` | 데이터 테이블 행 가져오기 |  |
| `K2Node_CallFunction_0` | Set Static Mesh<br/>타깃은 스태틱 메시 컴포넌트 | ReturnValue=false |
| `K2Node_VariableGet_0` | Get StaticMesh |  |
| `K2Node_VariableGet_2` | Get DataTable Handle |  |
| `K2Node_Event_1` | Setup Mesh 이벤트 |  |

</details>

### Get Attribute Tags

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["Get Attribute Tags"]
  n1["반환 노드"]
  n2["Is Valid"]
  n3["데이터 테이블 행 가져오기"]
  n0 -->|"then"| n2
  n2 -->|"Is Valid"| n3
  n3 -->|"then"| n1
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | Get Attribute Tags |  |
| `K2Node_FunctionResult_0` | 반환 노드 |  |
| `K2Node_MacroInstance_0` | Is Valid |  |
| `K2Node_BreakStruct_0` | Break Data Table Row Handle |  |
| `K2Node_VariableGet_3` | Get DataTable Handle |  |
| `K2Node_Knot_0` | 경유 노드 |  |
| `K2Node_Knot_1` | 경유 노드 |  |
| `K2Node_Knot_2` | 경유 노드 |  |
| `K2Node_GetDataTableRow_1` | 데이터 테이블 행 가져오기 |  |
| `K2Node_BreakStruct_2` | Break ST Held Item Data |  |
| `K2Node_VariableGet_1` | Get attributes |  |

</details>

### GetPickSound

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["GetPickSound"]
  n1["반환 노드"]
  n2["데이터 테이블 행 가져오기"]
  n0 -->|"then"| n2
  n2 -->|"then"| n1
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | GetPickSound |  |
| `K2Node_FunctionResult_0` | 반환 노드 |  |
| `K2Node_VariableGet_0` | Get DataTable Handle |  |
| `K2Node_BreakStruct_0` | Break Data Table Row Handle |  |
| `K2Node_GetDataTableRow_0` | 데이터 테이블 행 가져오기 |  |
| `K2Node_BreakStruct_1` | Break ST Held Item Data |  |

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

- `Setup Mesh`: Graph 'Setup Mesh' not found in Blueprint.

[원본 MCP 스냅샷](../../../Evidence/blueprints-070.json)
