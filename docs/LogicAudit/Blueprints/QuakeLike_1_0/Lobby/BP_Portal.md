# BP_Portal

에셋: `/Game/QuakeLike_1_0/Lobby/BP_Portal`

부모: `/Script/CoreUObject.Class'/Script/Engine.Actor'`

수집일: 2026-10-12. MCP에서 읽은 그래프·핀 연결의 자동 전사입니다. 상속된 그래프는 부모 문서를 함께 확인합니다.

정적 루트 참조 도달: 있음. 실행 여부 또는 삭제 가능 여부의 확정이 아닙니다.

컴포넌트 목록은 이 BP가 직접 정의한 SCS만 포함합니다. 부모·네이티브 생성 컴포넌트, 인스턴스 설정은 별도입니다.

## 직접 정의한 컴포넌트

| 이름 | 클래스 | 부모 컴포넌트 |
|---|---|---|
| `DefaultSceneRoot` | `SceneComponent` | `` |
| `PortalMesh` | `StaticMeshComponent` | `DefaultSceneRoot` |
| `Portal` | `SphereComponent` | `DefaultSceneRoot` |
| `Scene` | `SceneComponent` | `DefaultSceneRoot` |

## 변수와 기본값

| 이름 | 타입 | CDO 기본값 |
|---|---|---|
| `MapName` | `string` |  |

## 그래프 목록

| 그래프 | 전체 노드 수 |
|---|---|
| EventGraph | 16 |
| UserConstructionScript | 3 |

## 그래프별 실행 연결

실선은 출력 exec 핀의 실제 연결입니다. 여러 분기는 출력 핀 이름을 표시합니다. 함수 호출·delegate·타이머의 다음 그래프 실행은 이 그림에서 자동으로 이어 붙이지 않습니다.

순수 계산 노드 및 데이터 핀은 아래 목록과 원본 JSON을 확인합니다. 연결되지 않은 이벤트도 표시되므로, 노드가 있다는 이유만으로 실행된다고 판단하면 안 됩니다.

### EventGraph

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["On Component Begin Overlap (Portal)"]
  n1["Switch Has Authority"]
  n2["Cast To Pawn"]
  n3["Cast To PlayerController"]
  n4["Execute Console Command"]
  n5["Tick 이벤트"]
  n6["Draw Debug String"]
  n0 -->|"then"| n1
  n1 -->|"Authority"| n2
  n2 -->|"then"| n3
  n3 -->|"then"| n4
  n5 -->|"then"| n6
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_ComponentBoundEvent_1` | On Component Begin Overlap (Portal) | OtherBodyIndex=0; bFromSweep=false |
| `K2Node_MacroInstance_0` | Switch Has Authority |  |
| `K2Node_DynamicCast_0` | Cast To Pawn |  |
| `K2Node_Knot_2` | 경유 노드 |  |
| `K2Node_CallFunction_1` | Get Controller<br/>타깃은 폰 |  |
| `K2Node_DynamicCast_1` | Cast To PlayerController |  |
| `K2Node_CommutativeAssociativeBinaryOperator_0` | Append | A=ServerTravel  |
| `K2Node_CallFunction_10` | Execute Console Command |  |
| `K2Node_VariableGet_0` | Get MapName |  |
| `K2Node_Knot_0` | 경유 노드 |  |
| `K2Node_Knot_1` | 경유 노드 |  |
| `K2Node_Event_3` | Tick 이벤트 | DeltaSeconds=0.0 |
| `K2Node_CallFunction_0` | Draw Debug String | TextColor=(R=1.000000,G=1.000000,B=1.000000,A=1.000000); Duration=0.000000 |
| `K2Node_VariableGet_1` | Get MapName |  |
| `K2Node_VariableGet_2` | Get Scene |  |
| `K2Node_CallFunction_2` | Get World Location<br/>타깃은 씬 컴포넌트 |  |

</details>

### UserConstructionScript

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["Construction Script"]
  n1["Snap to Ground"]
  n0 -->|"then"| n1
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | Construction Script |  |
| `K2Node_CallFunction_0` | Snap to Ground |  |
| `K2Node_Self_0` | 셀프-레퍼런스 |  |

</details>

## 조회 범위의 한계

ObjectIterator로 발견했으나 MCP helper가 반환하지 않은 그래프입니다. delegate signature·컴파일된 중간 그래프·macro·애니메이션 내부 그래프가 섞일 수 있습니다.

- `BndEvt__BP_Portal_Portal_K2Node_ComponentBoundEvent_1_ComponentBeginOverlapSignature__DelegateSignature`: Graph 'BndEvt__BP_Portal_Portal_K2Node_ComponentBoundEvent_1_ComponentBeginOverlapSignature__DelegateSignature' not found in Blueprint.
- `EdGraph_0`: Graph 'EdGraph_0' not found in Blueprint.
- `ExecuteUbergraph_BP_Portal`: Graph 'ExecuteUbergraph_BP_Portal' not found in Blueprint.
- `ReceiveTick`: Graph 'ReceiveTick' not found in Blueprint.
- `UserConstructionScript_MERGED`: Graph 'UserConstructionScript_MERGED' not found in Blueprint.

[원본 MCP 스냅샷](../../../Evidence/blueprints-210.json)
