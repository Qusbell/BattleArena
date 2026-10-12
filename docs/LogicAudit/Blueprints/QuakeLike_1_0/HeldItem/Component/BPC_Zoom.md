# BPC_Zoom

에셋: `/Game/QuakeLike_1_0/HeldItem/Component/BPC_Zoom`

부모: `/Script/CoreUObject.Class'/Script/Engine.ActorComponent'`

수집일: 2026-10-12. MCP에서 읽은 그래프·핀 연결의 자동 전사입니다. 상속된 그래프는 부모 문서를 함께 확인합니다.

정적 루트 참조 도달: 있음. 실행 여부 또는 삭제 가능 여부의 확정이 아닙니다.

컴포넌트 목록은 이 BP가 직접 정의한 SCS만 포함합니다. 부모·네이티브 생성 컴포넌트, 인스턴스 설정은 별도입니다.

## 변수와 기본값

| 이름 | 타입 | CDO 기본값 |
|---|---|---|
| `defaultFov` | `real` | 0.0 |
| `currentTargetFov` | `real` | 0.0 |
| `isZoom` | `bool` | False |
| `cameraRef` | `CameraComponent` | None |
| `wConfig` | `ST_WeaponConfig` | &lt;Struct 'ST_WeaponConfig' {}&gt; |
| `interpSpeed` | `real` | 0.0 |

## 그래프 목록

| 그래프 | 전체 노드 수 |
|---|---|
| EventGraph | 11 |
| InitializeZoom | 7 |
| SetDefaultFromCurrent | 5 |
| StartZoom | 5 |
| StopZoom | 4 |
| UpdateFOV | 6 |

## 그래프별 실행 연결

실선은 출력 exec 핀의 실제 연결입니다. 여러 분기는 출력 핀 이름을 표시합니다. 함수 호출·delegate·타이머의 다음 그래프 실행은 이 그림에서 자동으로 이어 붙이지 않습니다.

순수 계산 노드 및 데이터 핀은 아래 목록과 원본 JSON을 확인합니다. 연결되지 않은 이벤트도 표시되므로, 노드가 있다는 이유만으로 실행된다고 판단하면 안 됩니다.

### EventGraph

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["Tick 이벤트"]
  n1["Is Valid"]
  n2["Set Field Of View<br/>타깃은 카메라 컴포넌트"]
  n1 -->|"Is Valid"| n2
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_Event_1` | Tick 이벤트 |  |
| `K2Node_MacroInstance_0` | Is Valid |  |
| `K2Node_VariableGet_1` | Get FieldOfView |  |
| `K2Node_CallFunction_0` | FInterp To |  |
| `K2Node_Knot_2` | 경유 노드 |  |
| `K2Node_CallFunction_1` | Set Field Of View<br/>타깃은 카메라 컴포넌트 |  |
| `K2Node_VariableGet_5` | Get cameraRef |  |
| `K2Node_VariableGet_9` | Get currentTargetFov |  |
| `K2Node_Knot_3` | 경유 노드 |  |
| `K2Node_Knot_4` | 경유 노드 |  |
| `K2Node_VariableGet_0` | Get interpSpeed |  |

</details>

### InitializeZoom

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["InitializeZoom"]
  n1["Is Valid"]
  n2["Set cameraRef"]
  n3["Set defaultFov"]
  n4["Set currentTargetFov"]
  n0 -->|"then"| n2
  n1 -->|"Is Valid"| n3
  n2 -->|"then"| n1
  n3 -->|"then"| n4
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | InitializeZoom |  |
| `K2Node_MacroInstance_0` | Is Valid |  |
| `K2Node_VariableGet_1` | Get FieldOfView |  |
| `K2Node_VariableSet_3` | Set cameraRef |  |
| `K2Node_VariableGet_2` | Get cameraRef |  |
| `K2Node_VariableSet_4` | Set defaultFov |  |
| `K2Node_VariableSet_5` | Set currentTargetFov | Output_Get=0.0 |

</details>

### SetDefaultFromCurrent

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["SetDefaultFromCurrent"]
  n1["Is Valid"]
  n2["Set defaultFov"]
  n0 -->|"then"| n1
  n1 -->|"Is Valid"| n2
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | SetDefaultFromCurrent |  |
| `K2Node_VariableGet_0` | Get cameraRef |  |
| `K2Node_MacroInstance_0` | Is Valid |  |
| `K2Node_VariableSet_0` | Set defaultFov | Output_Get=0.0 |
| `K2Node_VariableGet_1` | Get FieldOfView |  |

</details>

### StartZoom

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["StartZoom"]
  n1["Set isZoom"]
  n2["Set currentTargetFov"]
  n3["Set interpSpeed"]
  n0 -->|"then"| n1
  n1 -->|"then"| n2
  n2 -->|"then"| n3
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | StartZoom |  |
| `K2Node_VariableSet_3` | Set isZoom | isZoom=true; Output_Get=false |
| `K2Node_VariableSet_4` | Set currentTargetFov | Output_Get=0.0 |
| `K2Node_VariableSet_0` | Set interpSpeed | Output_Get=0.0 |
| `K2Node_Knot_0` | 경유 노드 |  |

</details>

### StopZoom

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["StopZoom"]
  n1["Set isZoom"]
  n2["Set currentTargetFov"]
  n0 -->|"then"| n1
  n1 -->|"then"| n2
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | StopZoom |  |
| `K2Node_VariableSet_0` | Set isZoom | isZoom=false; Output_Get=false |
| `K2Node_VariableSet_1` | Set currentTargetFov | Output_Get=0.0 |
| `K2Node_VariableGet_0` | Get defaultFov |  |

</details>

### UpdateFOV

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["UpdateFOV"]
  n1["Is Valid"]
  n2["Set defaultFov"]
  n3["Set currentTargetFov"]
  n0 -->|"then"| n1
  n1 -->|"Is Valid"| n2
  n2 -->|"then"| n3
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | UpdateFOV |  |
| `K2Node_MacroInstance_0` | Is Valid |  |
| `K2Node_VariableGet_1` | Get FieldOfView |  |
| `K2Node_VariableSet_4` | Set defaultFov |  |
| `K2Node_VariableSet_5` | Set currentTargetFov | Output_Get=0.0 |
| `K2Node_VariableGet_2` | Get cameraRef |  |

</details>

## 조회 범위의 한계

ObjectIterator로 발견했으나 MCP helper가 반환하지 않은 그래프입니다. delegate signature·컴파일된 중간 그래프·macro·애니메이션 내부 그래프가 섞일 수 있습니다.

- `ReceiveTick`: Graph 'ReceiveTick' not found in Blueprint.

[원본 MCP 스냅샷](../../../../Evidence/blueprints-070.json)
