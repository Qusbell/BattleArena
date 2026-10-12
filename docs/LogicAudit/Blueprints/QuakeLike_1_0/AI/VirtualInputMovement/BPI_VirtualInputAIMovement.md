# BPI_VirtualInputAIMovement

에셋: `/Game/QuakeLike_1_0/AI/VirtualInputMovement/BPI_VirtualInputAIMovement`

부모: `/Script/CoreUObject.Class'/Script/CoreUObject.Interface'`

수집일: 2026-10-12. MCP에서 읽은 그래프·핀 연결의 자동 전사입니다. 상속된 그래프는 부모 문서를 함께 확인합니다.

정적 루트 참조 도달: 있음. 실행 여부 또는 삭제 가능 여부의 확정이 아닙니다.

컴포넌트 목록은 이 BP가 직접 정의한 SCS만 포함합니다. 부모·네이티브 생성 컴포넌트, 인스턴스 설정은 별도입니다.

## 그래프 목록

| 그래프 | 전체 노드 수 |
|---|---|
| Can Random Jump | 2 |
| Check Completed | 2 |
| Clear NavPath | 1 |
| Complete Current Route | 1 |
| Request Build NavPath To Actor | 1 |
| Request Build NavPath To Location | 1 |
| Set MoveCharacter | 1 |
| Touch Jump Pad | 2 |

## 그래프별 실행 연결

실선은 출력 exec 핀의 실제 연결입니다. 여러 분기는 출력 핀 이름을 표시합니다. 함수 호출·delegate·타이머의 다음 그래프 실행은 이 그림에서 자동으로 이어 붙이지 않습니다.

순수 계산 노드 및 데이터 핀은 아래 목록과 원본 JSON을 확인합니다. 연결되지 않은 이벤트도 표시되므로, 노드가 있다는 이유만으로 실행된다고 판단하면 안 됩니다.

### Can Random Jump

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["Can Random Jump"]
  n1["반환 노드"]
  n0 -->|"then"| n1
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | Can Random Jump |  |
| `K2Node_FunctionResult_0` | 반환 노드 | Can=false |

</details>

### Check Completed

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["Check Completed"]
  n1["반환 노드"]
  n0 -->|"then"| n1
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | Check Completed |  |
| `K2Node_FunctionResult_0` | 반환 노드 | Is Valid=false; Is Completed=false |

</details>

### Clear NavPath

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["Clear NavPath"]
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | Clear NavPath |  |

</details>

### Complete Current Route

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["Complete Current Route"]
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | Complete Current Route |  |

</details>

### Request Build NavPath To Actor

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["Request Build NavPath To Actor"]
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | Request Build NavPath To Actor |  |

</details>

### Request Build NavPath To Location

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["Request Build NavPath To Location"]
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | Request Build NavPath To Location |  |

</details>

### Set MoveCharacter

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["Set MoveCharacter"]
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | Set MoveCharacter |  |

</details>

### Touch Jump Pad

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["Touch Jump Pad"]
  n1["반환 노드"]
  n0 -->|"then"| n1
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | Touch Jump Pad |  |
| `K2Node_FunctionResult_0` | 반환 노드 |  |

</details>


[원본 MCP 스냅샷](../../../../Evidence/blueprints-000.json)
