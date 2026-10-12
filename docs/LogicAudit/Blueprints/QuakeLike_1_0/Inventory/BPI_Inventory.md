# BPI_Inventory

에셋: `/Game/QuakeLike_1_0/Inventory/BPI_Inventory`

부모: `/Script/CoreUObject.Class'/Script/CoreUObject.Interface'`

수집일: 2026-10-12. MCP에서 읽은 그래프·핀 연결의 자동 전사입니다. 상속된 그래프는 부모 문서를 함께 확인합니다.

정적 루트 참조 도달: 있음. 실행 여부 또는 삭제 가능 여부의 확정이 아닙니다.

컴포넌트 목록은 이 BP가 직접 정의한 SCS만 포함합니다. 부모·네이티브 생성 컴포넌트, 인스턴스 설정은 별도입니다.

## 그래프 목록

| 그래프 | 전체 노드 수 |
|---|---|
| Clear | 1 |
| GetSelectedItem | 2 |
| HasItem | 2 |
| Pickup | 1 |
| Recharge | 1 |
| SwapItem | 1 |
| SwapItemByTag | 2 |
| SwapNext | 1 |
| SwapPrev | 1 |

## 그래프별 실행 연결

실선은 출력 exec 핀의 실제 연결입니다. 여러 분기는 출력 핀 이름을 표시합니다. 함수 호출·delegate·타이머의 다음 그래프 실행은 이 그림에서 자동으로 이어 붙이지 않습니다.

순수 계산 노드 및 데이터 핀은 아래 목록과 원본 JSON을 확인합니다. 연결되지 않은 이벤트도 표시되므로, 노드가 있다는 이유만으로 실행된다고 판단하면 안 됩니다.

### Clear

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["Clear"]
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | Clear |  |

</details>

### GetSelectedItem

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["GetSelectedItem"]
  n1["반환 노드"]
  n0 -->|"then"| n1
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | GetSelectedItem |  |
| `K2Node_FunctionResult_0` | 반환 노드 |  |

</details>

### HasItem

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["HasItem"]
  n1["반환 노드"]
  n0 -->|"then"| n1
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | HasItem |  |
| `K2Node_FunctionResult_0` | 반환 노드 | has=false |

</details>

### Pickup

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["Pickup"]
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | Pickup |  |

</details>

### Recharge

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["Recharge"]
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | Recharge |  |

</details>

### SwapItem

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["SwapItem"]
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | SwapItem |  |

</details>

### SwapItemByTag

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["SwapItemByTag"]
  n1["반환 노드"]
  n0 -->|"then"| n1
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | SwapItemByTag |  |
| `K2Node_FunctionResult_0` | 반환 노드 | Success=false |

</details>

### SwapNext

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["SwapNext"]
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | SwapNext |  |

</details>

### SwapPrev

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["SwapPrev"]
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | SwapPrev |  |

</details>


[원본 MCP 스냅샷](../../../Evidence/blueprints-070.json)
