# BPA_YJS_MovingPlatform

에셋: `/Game/QuakeLike_1_0/MapDesign/MapGimmick/BPA_YJS_MovingPlatform`

부모: `/Script/CoreUObject.Class'/Script/Engine.Actor'`

수집일: 2026-10-12. MCP에서 읽은 그래프·핀 연결의 자동 전사입니다. 상속된 그래프는 부모 문서를 함께 확인합니다.

정적 루트 참조 도달: 없음. 실행 여부 또는 삭제 가능 여부의 확정이 아닙니다.

컴포넌트 목록은 이 BP가 직접 정의한 SCS만 포함합니다. 부모·네이티브 생성 컴포넌트, 인스턴스 설정은 별도입니다.

## 직접 정의한 컴포넌트

| 이름 | 클래스 | 부모 컴포넌트 |
|---|---|---|
| `DefaultSceneRoot` | `SceneComponent` | `` |
| `Platform` | `StaticMeshComponent` | `DefaultSceneRoot` |
| `PlatformTrigger` | `BoxComponent` | `Platform` |

## 변수와 기본값

| 이름 | 타입 | CDO 기본값 |
|---|---|---|
| `MoveDistance` | `real` | 1500.0 |
| `MoveTime` | `real` | 3.0 |

## 그래프 목록

| 그래프 | 전체 노드 수 |
|---|---|
| EventGraph | 15 |
| UserConstructionScript | 1 |

## 그래프별 실행 연결

실선은 출력 exec 핀의 실제 연결입니다. 여러 분기는 출력 핀 이름을 표시합니다. 함수 호출·delegate·타이머의 다음 그래프 실행은 이 그림에서 자동으로 이어 붙이지 않습니다.

순수 계산 노드 및 데이터 핀은 아래 목록과 원본 JSON을 확인합니다. 연결되지 않은 이벤트도 표시되므로, 노드가 있다는 이유만으로 실행된다고 판단하면 안 됩니다.

### EventGraph

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["On Component Begin Overlap (PlatformTrigger)"]
  n1["On Component End Overlap (PlatformTrigger)"]
  n2["MovePlatform"]
  n3["Cast To Character"]
  n4["Set Relative Location<br/>타깃은 씬 컴포넌트"]
  n5["BeginPlay 이벤트"]
  n6["Set Play Rate<br/>타깃은 타임라인 컴포넌트"]
  n0 -->|"then"| n3
  n1 -->|"then"| n2
  n2 -->|"Update"| n4
  n3 -->|"then"| n2
  n5 -->|"then"| n6
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_ComponentBoundEvent_0` | On Component Begin Overlap (PlatformTrigger) | OtherBodyIndex=0; bFromSweep=false |
| `K2Node_ComponentBoundEvent_1` | On Component End Overlap (PlatformTrigger) | OtherBodyIndex=0 |
| `K2Node_Timeline_0` | MovePlatform | NewTime=0.0 |
| `K2Node_DynamicCast_0` | Cast To Character |  |
| `K2Node_CallFunction_1` | Lerp (Vector) | A=0, 0, 0; B_X=0.0; B_Y=0.0 |
| `K2Node_VariableGet_1` | Get MoveDistance |  |
| `K2Node_Knot_0` | 경유 노드 |  |
| `K2Node_CallFunction_3` | Set Relative Location<br/>타깃은 씬 컴포넌트 | bSweep=false; bTeleport=false |
| `K2Node_VariableGet_2` | Get Platform |  |
| `K2Node_Event_3` | BeginPlay 이벤트 |  |
| `K2Node_VariableGet_3` | Get MovePlatform |  |
| `K2Node_CallFunction_4` | Set Play Rate<br/>타깃은 타임라인 컴포넌트 |  |
| `K2Node_VariableGet_4` | Get MoveTime |  |
| `K2Node_PromotableOperator_0` | float / float | A=1.000000 |
| `EdGraphNode_Comment_0` | 타임라인 시간 조절 방법<br/>EX: Play Rate가 1 / 3 (0.33...)이면 0.33배속 (3배 느리게 재생)<br/>1초 짜리를 3배 느리게 하니 3초가 걸림 |  |

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

- `BndEvt__BPA_YJS_MovingPlatform_PlatformTrigger_K2Node_ComponentBoundEvent_0_ComponentBeginOverlapSignature__DelegateSignature`: Graph 'BndEvt__BPA_YJS_MovingPlatform_PlatformTrigger_K2Node_ComponentBoundEvent_0_ComponentBeginOverlapSignature__DelegateSignature' not found in Blueprint.
- `BndEvt__BPA_YJS_MovingPlatform_PlatformTrigger_K2Node_ComponentBoundEvent_1_ComponentEndOverlapSignature__DelegateSignature`: Graph 'BndEvt__BPA_YJS_MovingPlatform_PlatformTrigger_K2Node_ComponentBoundEvent_1_ComponentEndOverlapSignature__DelegateSignature' not found in Blueprint.
- `ExecuteUbergraph_BPA_YJS_MovingPlatform`: Graph 'ExecuteUbergraph_BPA_YJS_MovingPlatform' not found in Blueprint.
- `MovePlatform__FinishedFunc`: Graph 'MovePlatform__FinishedFunc' not found in Blueprint.
- `MovePlatform__UpdateFunc`: Graph 'MovePlatform__UpdateFunc' not found in Blueprint.
- `ReceiveBeginPlay`: Graph 'ReceiveBeginPlay' not found in Blueprint.
- `UserConstructionScript_MERGED`: Graph 'UserConstructionScript_MERGED' not found in Blueprint.

[원본 MCP 스냅샷](../../../../Evidence/blueprints-210.json)
