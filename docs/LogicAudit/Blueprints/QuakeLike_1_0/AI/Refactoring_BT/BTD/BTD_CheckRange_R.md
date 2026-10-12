# BTD_CheckRange_R

에셋: `/Game/QuakeLike_1_0/AI/Refactoring_BT/BTD/BTD_CheckRange_R`

부모: `/Script/CoreUObject.Class'/Script/AIModule.BTDecorator_BlueprintBase'`

수집일: 2026-10-12. MCP에서 읽은 그래프·핀 연결의 자동 전사입니다. 상속된 그래프는 부모 문서를 함께 확인합니다.

정적 루트 참조 도달: 있음. 실행 여부 또는 삭제 가능 여부의 확정이 아닙니다.

컴포넌트 목록은 이 BP가 직접 정의한 SCS만 포함합니다. 부모·네이티브 생성 컴포넌트, 인스턴스 설정은 별도입니다.

## 변수와 기본값

| 이름 | 타입 | CDO 기본값 |
|---|---|---|
| `Target` | `BlackboardKeySelector` | &lt;Struct 'BlackboardKeySelector' {allowed_types: , selected_key_name: "", selected_key_type: None, selected_key_id: 65535, none_is_allowed_value: False}&gt; |

## 그래프 목록

| 그래프 | 전체 노드 수 |
|---|---|
| EventGraph | 0 |
| PerformConditionCheckAI | 51 |

## 그래프별 실행 연결

실선은 출력 exec 핀의 실제 연결입니다. 여러 분기는 출력 핀 이름을 표시합니다. 함수 호출·delegate·타이머의 다음 그래프 실행은 이 그림에서 자동으로 이어 붙이지 않습니다.

순수 계산 노드 및 데이터 핀은 아래 목록과 원본 JSON을 확인합니다. 연결되지 않은 이벤트도 표시되므로, 노드가 있다는 이유만으로 실행된다고 판단하면 안 됩니다.

### EventGraph

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|

</details>

### PerformConditionCheckAI

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["Perform Condition Check AI"]
  n1["반환 노드"]
  n2["Get Inventory<br/>Target is BPI Item Holder"]
  n3["Branch"]
  n4["반환 노드"]
  n5["Get Selected Item<br/>타깃은 BPI Inventory"]
  n6["Get Weapon Data<br/>Target is BPI Weapon"]
  n7["Is Valid"]
  n8["Branch"]
  n9["반환 노드"]
  n10["반환 노드"]
  n11["Get Aim<br/>Target is BPI Aim"]
  n12["Is Valid"]
  n13["반환 노드"]
  n14["반환 노드"]
  n15["Is Valid"]
  n16["반환 노드"]
  n17["Set SelfPawn"]
  n18["Get"]
  n19["Branch"]
  n20["반환 노드"]
  n0 -->|"then"| n17
  n2 -->|"then"| n3
  n3 -->|"then"| n5
  n3 -->|"else"| n4
  n5 -->|"then"| n15
  n6 -->|"then"| n19
  n7 -->|"Is Valid"| n8
  n7 -->|"Is Not Valid"| n10
  n8 -->|"then"| n11
  n8 -->|"else"| n9
  n11 -->|"then"| n1
  n12 -->|"Is Valid"| n18
  n12 -->|"Is Not Valid"| n13
  n15 -->|"Is Valid"| n6
  n15 -->|"Is Not Valid"| n16
  n17 -->|"then"| n12
  n18 -->|"then"| n2
  n18 -->|"else"| n14
  n19 -->|"then"| n7
  n19 -->|"else"| n20
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | Perform Condition Check AI |  |
| `K2Node_FunctionResult_3` | 반환 노드 |  |
| `K2Node_Message_0` | Get Inventory<br/>Target is BPI Item Holder |  |
| `K2Node_IfThenElse_0` | Branch |  |
| `K2Node_FunctionResult_6` | 반환 노드 | ReturnValue=false |
| `K2Node_CallFunction_0` | Get Selected Item<br/>타깃은 BPI Inventory |  |
| `K2Node_Knot_0` | 경유 노드 |  |
| `K2Node_Message_1` | Get Weapon Data<br/>Target is BPI Weapon |  |
| `K2Node_MacroInstance_0` | Is Valid |  |
| `K2Node_Knot_1` | 경유 노드 |  |
| `K2Node_VariableGet_0` | Get attributes |  |
| `K2Node_CallFunction_1` | Has Tag | Tag=(TagName="Item.Weapon.explosion"); bExactMatch=false |
| `K2Node_IfThenElse_1` | Branch |  |
| `K2Node_FunctionResult_1` | 반환 노드 | ReturnValue=true |
| `K2Node_FunctionResult_7` | 반환 노드 | ReturnValue=false |
| `K2Node_Message_4` | Get Aim<br/>Target is BPI Aim | aimRotation=0, 0, 0 |
| `K2Node_VariableGet_3` | Get Target |  |
| `K2Node_CallFunction_6` | Get Blackboard Value as Actor |  |
| `K2Node_CallFunction_4` | Get Actor Location<br/>타깃은 액터 |  |
| `K2Node_VariableGet_2` | Get explosionRadius |  |
| `K2Node_Knot_4` | 경유 노드 |  |
| `K2Node_CallFunction_5` | Distance Squared (Vector) |  |
| `K2Node_Knot_5` | 경유 노드 |  |
| `K2Node_PromotableOperator_0` | float * float |  |
| `K2Node_PromotableOperator_1` | float &gt; float |  |
| `K2Node_VariableGet_5` | Get Target |  |
| `K2Node_CallFunction_2` | Get Blackboard Value as Actor |  |
| `K2Node_MacroInstance_2` | Is Valid |  |
| `K2Node_FunctionResult_5` | 반환 노드 | ReturnValue=false |
| `K2Node_FunctionResult_0` | 반환 노드 | ReturnValue=false |
| `K2Node_Knot_7` | 경유 노드 |  |
| `K2Node_MacroInstance_3` | Is Valid |  |
| `K2Node_Knot_8` | 경유 노드 |  |
| `K2Node_FunctionResult_4` | 반환 노드 | ReturnValue=false |
| `K2Node_VariableGet_4` | Get range |  |
| `K2Node_VariableGet_1` | Get Target |  |
| `K2Node_CallFunction_3` | Get Blackboard Value as Actor |  |
| `K2Node_CallFunction_7` | Get Actor Location<br/>타깃은 액터 |  |
| `K2Node_VariableSet_0` | Set SelfPawn |  |
| `K2Node_VariableGet_6` | Get |  |
| `K2Node_VariableGet_7` | Get SelfPawn |  |
| `K2Node_VariableGet_8` | Get SelfPawn |  |
| `K2Node_CallFunction_8` | Get Actor Location<br/>타깃은 액터 |  |
| `K2Node_CallFunction_9` | Distance Squared (Vector) |  |
| `K2Node_PromotableOperator_2` | float * float |  |
| `K2Node_Knot_9` | 경유 노드 |  |
| `K2Node_Knot_10` | 경유 노드 |  |
| `K2Node_Knot_12` | 경유 노드 |  |
| `K2Node_PromotableOperator_3` | float &lt; float |  |
| `K2Node_IfThenElse_2` | Branch |  |
| `K2Node_FunctionResult_2` | 반환 노드 | ReturnValue=false |

</details>


[원본 MCP 스냅샷](../../../../../Evidence/blueprints-105.json)
