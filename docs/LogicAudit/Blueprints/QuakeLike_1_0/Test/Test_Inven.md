# Test_Inven

에셋: `/Game/QuakeLike_1_0/Test/Test_Inven`

부모: `/Script/CoreUObject.Class'/Script/Engine.Actor'`

수집일: 2026-10-12. MCP에서 읽은 그래프·핀 연결의 자동 전사입니다. 상속된 그래프는 부모 문서를 함께 확인합니다.

정적 루트 참조 도달: 없음. 실행 여부 또는 삭제 가능 여부의 확정이 아닙니다.

컴포넌트 목록은 이 BP가 직접 정의한 SCS만 포함합니다. 부모·네이티브 생성 컴포넌트, 인스턴스 설정은 별도입니다.

## 직접 정의한 컴포넌트

| 이름 | 클래스 | 부모 컴포넌트 |
|---|---|---|
| `DefaultSceneRoot` | `SceneComponent` | `` |
| `BPC_Inventory` | `BPC_Inventory_C` | `` |

## 그래프 목록

| 그래프 | 전체 노드 수 |
|---|---|
| EventGraph | 22 |
| Pickup | 5 |
| UserConstructionScript | 1 |

## 그래프별 실행 연결

실선은 출력 exec 핀의 실제 연결입니다. 여러 분기는 출력 핀 이름을 표시합니다. 함수 호출·delegate·타이머의 다음 그래프 실행은 이 그림에서 자동으로 이어 붙이지 않습니다.

순수 계산 노드 및 데이터 핀은 아래 목록과 원본 JSON을 확인합니다. 연결되지 않은 이벤트도 표시되므로, 노드가 있다는 이유만으로 실행된다고 판단하면 안 됩니다.

### EventGraph

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["BeginPlay 이벤트"]
  n1["시퀀스"]
  n2["Has Item<br/>Target is BPI Inventory"]
  n3["Branch"]
  n4["Print String"]
  n5["Print String"]
  n6["Pickup<br/>타깃은 Test Inven"]
  n7["Pickup<br/>타깃은 Test Inven"]
  n8["Pickup<br/>타깃은 Test Inven"]
  n9["Delay"]
  n10["Swap Item<br/>타깃은 BPC Inventory"]
  n11["Print String"]
  n12["Swap Item<br/>타깃은 BPC Inventory"]
  n13["Print String"]
  n14["Swap Item<br/>타깃은 BPC Inventory"]
  n15["Print String"]
  n0 -->|"then"| n1
  n1 -->|"then_0"| n6
  n1 -->|"then_1"| n9
  n1 -->|"then_2"| n10
  n1 -->|"then_3"| n12
  n1 -->|"then_4"| n14
  n2 -->|"then"| n3
  n3 -->|"then"| n4
  n3 -->|"else"| n5
  n6 -->|"then"| n7
  n7 -->|"then"| n8
  n9 -->|"then"| n2
  n10 -->|"then"| n11
  n12 -->|"then"| n13
  n14 -->|"then"| n15
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_Event_0` | BeginPlay 이벤트 |  |
| `K2Node_ExecutionSequence_0` | 시퀀스 |  |
| `K2Node_VariableGet_2` | Get BPC_Inventory |  |
| `K2Node_Message_2` | Has Item<br/>Target is BPI Inventory | itemID=(TagName="Event.HeldItem") |
| `K2Node_IfThenElse_1` | Branch |  |
| `K2Node_CallFunction_6` | Print String | InString=Hello; bPrintToScreen=true; bPrintToLog=true; TextColor=(R=0.000000,G=0.660000,B=1.000000,A=1.000000); Duration=2.000000; Key=None |
| `K2Node_CallFunction_7` | Print String | bPrintToScreen=true; bPrintToLog=true; TextColor=(R=0.000000,G=0.660000,B=1.000000,A=1.000000); Duration=2.000000; Key=None |
| `K2Node_CallFunction_4` | Pickup<br/>타깃은 Test Inven | NewParam=(TagName="Event.ItemHolder") |
| `K2Node_CallFunction_5` | Pickup<br/>타깃은 Test Inven | NewParam=(TagName="Event.HeldItem") |
| `K2Node_CallFunction_3` | Pickup<br/>타깃은 Test Inven | NewParam=(TagName="Event.Life") |
| `K2Node_CallFunction_8` | Delay | Duration=0.2 |
| `K2Node_CallFunction_21` | Get Display Name |  |
| `K2Node_CommutativeAssociativeBinaryOperator_0` | Append | A=대상 없음 :  |
| `K2Node_CallFunction_11` | Swap Item<br/>타깃은 BPC Inventory | index=0 |
| `K2Node_VariableGet_4` | Get BPC_Inventory |  |
| `K2Node_CallFunction_12` | Print String | InString=swap0; bPrintToScreen=true; bPrintToLog=true; TextColor=(R=0.000000,G=0.660000,B=1.000000,A=1.000000); Duration=2.000000; Key=None |
| `K2Node_CallFunction_13` | Swap Item<br/>타깃은 BPC Inventory | index=1 |
| `K2Node_VariableGet_5` | Get BPC_Inventory |  |
| `K2Node_CallFunction_14` | Print String | InString=swap1; bPrintToScreen=true; bPrintToLog=true; TextColor=(R=0.000000,G=0.660000,B=1.000000,A=1.000000); Duration=2.000000; Key=None |
| `K2Node_CallFunction_9` | Swap Item<br/>타깃은 BPC Inventory | index=2 |
| `K2Node_VariableGet_3` | Get BPC_Inventory |  |
| `K2Node_CallFunction_10` | Print String | InString=swap2; bPrintToScreen=true; bPrintToLog=true; TextColor=(R=0.000000,G=0.660000,B=1.000000,A=1.000000); Duration=2.000000; Key=None |

</details>

### Pickup

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["Pickup"]
  n1["SpawnActor T H"]
  n2["Pickup<br/>Target is BPI Inventory"]
  n0 -->|"then"| n1
  n1 -->|"then"| n2
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | Pickup |  |
| `K2Node_SpawnActorFromClass_0` | SpawnActor T H | Class=/Game/QuakeLike_1_0/Test/T_H.T_H_C; CollisionHandlingOverride=AlwaysSpawn; TransformScaleMethod=MultiplyWithRoot |
| `K2Node_CallFunction_0` | Get Actor Transform<br/>타깃은 액터 |  |
| `K2Node_VariableGet_0` | Get BPC_Inventory |  |
| `K2Node_Message_0` | Pickup<br/>Target is BPI Inventory |  |

</details>

### UserConstructionScript

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["Construction Script"]
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | Construction Script |  |

</details>

## 조회 범위의 한계

ObjectIterator로 발견했으나 MCP helper가 반환하지 않은 그래프입니다. delegate signature·컴파일된 중간 그래프·macro·애니메이션 내부 그래프가 섞일 수 있습니다.

- `ExecuteUbergraph_Test_Inven`: Graph 'ExecuteUbergraph_Test_Inven' not found in Blueprint.
- `Pickup_MERGED`: Graph 'Pickup_MERGED' not found in Blueprint.
- `ReceiveBeginPlay`: Graph 'ReceiveBeginPlay' not found in Blueprint.
- `UserConstructionScript_MERGED`: Graph 'UserConstructionScript_MERGED' not found in Blueprint.

[원본 MCP 스냅샷](../../../Evidence/blueprints-245.json)
