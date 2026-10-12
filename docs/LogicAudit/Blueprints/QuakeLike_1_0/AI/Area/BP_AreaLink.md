# BP_AreaLink

에셋: `/Game/QuakeLike_1_0/AI/Area/BP_AreaLink`

부모: `/Script/CoreUObject.Class'/Script/Engine.Actor'`

수집일: 2026-10-12. MCP에서 읽은 그래프·핀 연결의 자동 전사입니다. 상속된 그래프는 부모 문서를 함께 확인합니다.

정적 루트 참조 도달: 있음. 실행 여부 또는 삭제 가능 여부의 확정이 아닙니다.

컴포넌트 목록은 이 BP가 직접 정의한 SCS만 포함합니다. 부모·네이티브 생성 컴포넌트, 인스턴스 설정은 별도입니다.

## 직접 정의한 컴포넌트

| 이름 | 클래스 | 부모 컴포넌트 |
|---|---|---|
| `DefaultSceneRoot` | `SceneComponent` | `` |

## 변수와 기본값

| 이름 | 타입 | CDO 기본값 |
|---|---|---|
| `areaA` | `BP_AIArea_C` | None |
| `areaB` | `BP_AIArea_C` | None |
| `isTwoWay` | `bool` | True |
| `linkType` | `E_AreaLinkType` | &lt;E_AreaLinkType.NORMAL: 0&gt; |
| `linkCost` | `real` | 1.0 |
| `isEnabled` | `bool` | True |

## 그래프 목록

| 그래프 | 전체 노드 수 |
|---|---|
| EventGraph | 3 |
| IsValidLink | 16 |
| UserConstructionScript | 6 |

## 그래프별 실행 연결

실선은 출력 exec 핀의 실제 연결입니다. 여러 분기는 출력 핀 이름을 표시합니다. 함수 호출·delegate·타이머의 다음 그래프 실행은 이 그림에서 자동으로 이어 붙이지 않습니다.

순수 계산 노드 및 데이터 핀은 아래 목록과 원본 JSON을 확인합니다. 연결되지 않은 이벤트도 표시되므로, 노드가 있다는 이유만으로 실행된다고 판단하면 안 됩니다.

### EventGraph

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["BeginPlay 이벤트"]
  n1["ActorBeginOverlap 이벤트"]
  n2["Tick 이벤트"]
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_Event_0` | BeginPlay 이벤트 |  |
| `K2Node_Event_1` | ActorBeginOverlap 이벤트 |  |
| `K2Node_Event_2` | Tick 이벤트 | DeltaSeconds=0.0 |

</details>

### IsValidLink

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["IsValidLink"]
  n1["반환 노드"]
  n2["Branch"]
  n3["Is Valid"]
  n4["Is Valid"]
  n5["Branch"]
  n6["반환 노드"]
  n7["경유 노드"]
  n8["경유 노드"]
  n9["경유 노드"]
  n0 -->|"then"| n2
  n2 -->|"then"| n3
  n2 -->|"else"| n9
  n3 -->|"Is Valid"| n4
  n3 -->|"Is Not Valid"| n8
  n4 -->|"Is Valid"| n5
  n4 -->|"Is Not Valid"| n7
  n5 -->|"then"| n6
  n5 -->|"else"| n1
  n7 -->|"OutputPin"| n1
  n8 -->|"OutputPin"| n7
  n9 -->|"OutputPin"| n8
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | IsValidLink |  |
| `K2Node_FunctionResult_0` | 반환 노드 | isValid=false |
| `K2Node_VariableGet_0` | Get isEnabled |  |
| `K2Node_IfThenElse_0` | Branch |  |
| `K2Node_VariableGet_1` | Get areaA |  |
| `K2Node_VariableGet_2` | Get areaB |  |
| `K2Node_MacroInstance_0` | Is Valid |  |
| `K2Node_MacroInstance_1` | Is Valid |  |
| `K2Node_PromotableOperator_0` | Not Equal (Object) |  |
| `K2Node_VariableGet_3` | Get areaA |  |
| `K2Node_VariableGet_4` | Get areaB |  |
| `K2Node_IfThenElse_1` | Branch |  |
| `K2Node_FunctionResult_1` | 반환 노드 | isValid=true |
| `K2Node_Knot_0` | 경유 노드 |  |
| `K2Node_Knot_1` | 경유 노드 |  |
| `K2Node_Knot_2` | 경유 노드 |  |

</details>

### UserConstructionScript

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["Construction Script"]
  n1["Draw Debug Line"]
  n0 -->|"then"| n1
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | Construction Script |  |
| `K2Node_CallFunction_0` | Get Actor Location<br/>타깃은 액터 |  |
| `K2Node_CallFunction_1` | Draw Debug Line | LineColor=(R=1.000000,G=0.000000,B=0.053988,A=1.000000); Duration=0.000000; Thickness=0.000000 |
| `K2Node_VariableGet_0` | Get areaA |  |
| `K2Node_VariableGet_1` | Get areaB |  |
| `K2Node_CallFunction_2` | Get Actor Location<br/>타깃은 액터 |  |

</details>

## 조회 범위의 한계

ObjectIterator로 발견했으나 MCP helper가 반환하지 않은 그래프입니다. delegate signature·컴파일된 중간 그래프·macro·애니메이션 내부 그래프가 섞일 수 있습니다.

- `EdGraph_0`: Graph 'EdGraph_0' not found in Blueprint.
- `EdGraph_1`: Graph 'EdGraph_1' not found in Blueprint.
- `ExecuteUbergraph_BP_AreaLink`: Graph 'ExecuteUbergraph_BP_AreaLink' not found in Blueprint.
- `IsValidLink_MERGED`: Graph 'IsValidLink_MERGED' not found in Blueprint.
- `UserConstructionScript_MERGED`: Graph 'UserConstructionScript_MERGED' not found in Blueprint.

[원본 MCP 스냅샷](../../../../Evidence/blueprints-140.json)
