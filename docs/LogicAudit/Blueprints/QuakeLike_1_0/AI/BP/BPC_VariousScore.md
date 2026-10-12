# BPC_VariousScore

에셋: `/Game/QuakeLike_1_0/AI/BP/BPC_VariousScore`

부모: `/Script/CoreUObject.Class'/Script/Engine.ActorComponent'`

수집일: 2026-10-12. MCP에서 읽은 그래프·핀 연결의 자동 전사입니다. 상속된 그래프는 부모 문서를 함께 확인합니다.

정적 루트 참조 도달: 있음. 실행 여부 또는 삭제 가능 여부의 확정이 아닙니다.

컴포넌트 목록은 이 BP가 직접 정의한 SCS만 포함합니다. 부모·네이티브 생성 컴포넌트, 인스턴스 설정은 별도입니다.

## 변수와 기본값

| 이름 | 타입 | CDO 기본값 |
|---|---|---|
| `DA_Aggression` | `AggressionDataAssetBase` | /Game/QuakeLike_1_0/Data/AI/Aggression/DA_DefaultAggression.DA_DefaultAggression |

## 그래프 목록

| 그래프 | 전체 노드 수 |
|---|---|
| EventGraph | 0 |
| GetHeightScore | 9 |
| GetSurviveScore | 14 |
| GetWeaponScore | 14 |

## 그래프별 실행 연결

실선은 출력 exec 핀의 실제 연결입니다. 여러 분기는 출력 핀 이름을 표시합니다. 함수 호출·delegate·타이머의 다음 그래프 실행은 이 그림에서 자동으로 이어 붙이지 않습니다.

순수 계산 노드 및 데이터 핀은 아래 목록과 원본 JSON을 확인합니다. 연결되지 않은 이벤트도 표시되므로, 노드가 있다는 이유만으로 실행된다고 판단하면 안 됩니다.

### EventGraph

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|

</details>

### GetHeightScore

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["GetHeightScore"]
  n1["반환 노드"]
  n0 -->|"then"| n1
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | GetHeightScore |  |
| `K2Node_FunctionResult_0` | 반환 노드 |  |
| `K2Node_CallFunction_0` | Safe Divide |  |
| `K2Node_PromotableOperator_0` | float - float | A=1.000000 |
| `K2Node_CallFunction_3` | Break Vector | X=0.0; Y=0.0 |
| `K2Node_CallFunction_6` | Break Vector | X=0.0; Y=0.0 |
| `K2Node_PromotableOperator_1` | float - float |  |
| `K2Node_VariableGet_0` | Get DA_Aggression |  |
| `K2Node_VariableGet_1` | Get HeightStandard |  |

</details>

### GetSurviveScore

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["GetSurviveScore"]
  n1["반환 노드"]
  n2["Get Life<br/>타깃은 BPI Life"]
  n3["Get Life<br/>타깃은 BPI Life"]
  n0 -->|"then"| n2
  n2 -->|"then"| n3
  n3 -->|"then"| n1
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | GetSurviveScore |  |
| `K2Node_FunctionResult_0` | 반환 노드 |  |
| `K2Node_CallFunction_0` | Get Life<br/>타깃은 BPI Life | type=NewEnumerator0 |
| `K2Node_PromotableOperator_4` | float / float |  |
| `K2Node_CallFunction_1` | Get Life<br/>타깃은 BPI Life | type=NewEnumerator1 |
| `K2Node_PromotableOperator_3` | float / float |  |
| `K2Node_Knot_0` | 경유 노드 |  |
| `K2Node_PromotableOperator_6` | float * float |  |
| `K2Node_PromotableOperator_5` | float * float |  |
| `K2Node_PromotableOperator_7` | float + float |  |
| `K2Node_VariableGet_0` | Get DA_Aggression |  |
| `K2Node_VariableGet_1` | Get ArmorWeight |  |
| `K2Node_VariableGet_2` | Get DA_Aggression |  |
| `K2Node_VariableGet_3` | Get HealthWeight |  |

</details>

### GetWeaponScore

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["GetWeaponScore"]
  n1["반환 노드"]
  n2["Has Item<br/>Target is BPI Inventory"]
  n3["Branch"]
  n4["Get Usage Count<br/>Target is BPI Usable"]
  n0 -->|"then"| n2
  n2 -->|"then"| n3
  n3 -->|"then"| n4
  n4 -->|"then"| n1
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | GetWeaponScore |  |
| `K2Node_FunctionResult_1` | 반환 노드 |  |
| `K2Node_Message_0` | Has Item<br/>Target is BPI Inventory |  |
| `K2Node_CallFunction_19` | To Object (Interface) |  |
| `K2Node_IfThenElse_0` | Branch |  |
| `K2Node_Message_1` | Get Usage Count<br/>Target is BPI Usable |  |
| `K2Node_CallFunction_2` | Safe Divide |  |
| `K2Node_CallFunction_20` | To Float (Integer) |  |
| `K2Node_CallFunction_21` | To Float (Integer) |  |
| `K2Node_CallFunction_3` | Safe Divide |  |
| `K2Node_CallFunction_22` | To Float (Integer) |  |
| `K2Node_PromotableOperator_1` | float * float |  |
| `K2Node_VariableGet_0` | Get DA_Aggression |  |
| `K2Node_VariableGet_2` | Get WeaponBaseMax |  |

</details>


[원본 MCP 스냅샷](../../../../Evidence/blueprints-070.json)
