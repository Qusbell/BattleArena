# BTS_OrbitalFocus_R

에셋: `/Game/QuakeLike_1_0/AI/Refactoring_BT/BTS/BTS_OrbitalFocus_R`

부모: `/Script/CoreUObject.Class'/Script/AIModule.BTService_BlueprintBase'`

수집일: 2026-10-12. MCP에서 읽은 그래프·핀 연결의 자동 전사입니다. 상속된 그래프는 부모 문서를 함께 확인합니다.

정적 루트 참조 도달: 있음. 실행 여부 또는 삭제 가능 여부의 확정이 아닙니다.

컴포넌트 목록은 이 BP가 직접 정의한 SCS만 포함합니다. 부모·네이티브 생성 컴포넌트, 인스턴스 설정은 별도입니다.

## 그래프 목록

| 그래프 | 전체 노드 수 |
|---|---|
| EventGraph | 42 |

## 그래프별 실행 연결

실선은 출력 exec 핀의 실제 연결입니다. 여러 분기는 출력 핀 이름을 표시합니다. 함수 호출·delegate·타이머의 다음 그래프 실행은 이 그림에서 자동으로 이어 붙이지 않습니다.

순수 계산 노드 및 데이터 핀은 아래 목록과 원본 JSON을 확인합니다. 연결되지 않은 이벤트도 표시되므로, 노드가 있다는 이유만으로 실행된다고 판단하면 안 됩니다.

### EventGraph

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["Get Target Drone<br/>Target is BPI Get Target Drone"]
  n1["Receive Activation AI 이벤트"]
  n2["SetFocus<br/>타깃은 AI 컨트롤러"]
  n3["Receive Tick AI 이벤트"]
  n4["Look At<br/>Target is BPI Look"]
  n5["Branch"]
  n6["Receive Deactivation AI 이벤트"]
  n7["Clear Look<br/>Target is BPI Look"]
  n8["Look At<br/>Target is BPI Look"]
  n9["경유 노드"]
  n10["Get Aim<br/>Target is BPI Aim"]
  n0 -->|"then"| n2
  n1 -->|"then"| n0
  n3 -->|"then"| n5
  n5 -->|"then"| n4
  n5 -->|"else"| n9
  n6 -->|"then"| n7
  n9 -->|"OutputPin"| n10
  n10 -->|"then"| n8
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_Message_2` | Get Target Drone<br/>Target is BPI Get Target Drone |  |
| `K2Node_Event_1` | Receive Activation AI 이벤트 |  |
| `K2Node_CallFunction_10` | SetFocus<br/>타깃은 AI 컨트롤러 |  |
| `K2Node_Event_0` | Receive Tick AI 이벤트 | DeltaSeconds=0.0 |
| `K2Node_CallFunction_0` | Get Velocity<br/>타깃은 액터 |  |
| `K2Node_Knot_1` | 경유 노드 |  |
| `K2Node_CallFunction_7` | Get Actor Eyes View Point<br/>타깃은 액터 | OutRotation=0, 0, 0 |
| `K2Node_PromotableOperator_0` | vector + vector |  |
| `K2Node_Knot_0` | 경유 노드 |  |
| `K2Node_Message_5` | Look At<br/>Target is BPI Look |  |
| `K2Node_Knot_2` | 경유 노드 |  |
| `K2Node_Knot_3` | 경유 노드 |  |
| `K2Node_CallFunction_8` | Vector Length Squared |  |
| `K2Node_PromotableOperator_4` | float &gt; float | B=0.100000 |
| `K2Node_IfThenElse_0` | Branch |  |
| `K2Node_Knot_7` | 경유 노드 |  |
| `K2Node_Knot_8` | 경유 노드 |  |
| `K2Node_Event_3` | Receive Deactivation AI 이벤트 |  |
| `K2Node_Message_3` | Clear Look<br/>Target is BPI Look |  |
| `K2Node_Message_0` | Look At<br/>Target is BPI Look |  |
| `K2Node_Knot_9` | 경유 노드 |  |
| `K2Node_PromotableOperator_1` | vector + vector | B_Z=0.0 |
| `K2Node_CallFunction_3` | Get Forward Vector |  |
| `K2Node_PromotableOperator_2` | vector * vector | B=500.000000; ReturnValue_Z=0.0 |
| `K2Node_Message_6` | Get Aim<br/>Target is BPI Aim |  |
| `K2Node_Knot_10` | 경유 노드 |  |
| `K2Node_Knot_11` | 경유 노드 |  |
| `EdGraphNode_Comment_1` | 바라보고 있던 방향 바라봄 |  |
| `K2Node_CallFunction_17` | Make Vector | Z=0.0 |
| `K2Node_CallFunction_18` | Break Vector | Z=0.0 |
| `K2Node_CallFunction_19` | Normalize | Tolerance=0.000100 |
| `K2Node_PromotableOperator_7` | vector * vector | B=500.000000 |
| `K2Node_CallFunction_4` | Get Actor Location<br/>타깃은 액터 |  |
| `K2Node_CallFunction_5` | vector - vector |  |
| `K2Node_CallFunction_6` | vector - vector |  |
| `K2Node_Knot_4` | 경유 노드 |  |
| `K2Node_Knot_5` | 경유 노드 |  |
| `K2Node_Knot_6` | 경유 노드 |  |
| `K2Node_Knot_12` | 경유 노드 |  |
| `K2Node_Knot_13` | 경유 노드 |  |
| `K2Node_Knot_14` | 경유 노드 |  |
| `K2Node_Knot_15` | 경유 노드 |  |

</details>

## 조회 범위의 한계

ObjectIterator로 발견했으나 MCP helper가 반환하지 않은 그래프입니다. delegate signature·컴파일된 중간 그래프·macro·애니메이션 내부 그래프가 섞일 수 있습니다.

- `ReceiveActivationAI`: Graph 'ReceiveActivationAI' not found in Blueprint.
- `ReceiveDeactivationAI`: Graph 'ReceiveDeactivationAI' not found in Blueprint.
- `ReceiveTickAI`: Graph 'ReceiveTickAI' not found in Blueprint.

[원본 MCP 스냅샷](../../../../../Evidence/blueprints-035.json)
