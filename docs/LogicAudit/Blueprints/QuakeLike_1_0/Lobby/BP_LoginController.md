# BP_LoginController

에셋: `/Game/QuakeLike_1_0/Lobby/BP_LoginController`

부모: `/Script/CoreUObject.Class'/Script/Engine.PlayerController'`

수집일: 2026-10-12. MCP에서 읽은 그래프·핀 연결의 자동 전사입니다. 상속된 그래프는 부모 문서를 함께 확인합니다.

정적 루트 참조 도달: 있음. 실행 여부 또는 삭제 가능 여부의 확정이 아닙니다.

컴포넌트 목록은 이 BP가 직접 정의한 SCS만 포함합니다. 부모·네이티브 생성 컴포넌트, 인스턴스 설정은 별도입니다.

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
  n0["BeginPlay 이벤트"]
  n1["Create WBP Server Connect UI Widget"]
  n2["Add to Viewport<br/>타깃은 사용자 위젯"]
  n3["Set bShowMouseCursor"]
  n4["Branch"]
  n5["Try Login<br/>Custom Event"]
  n6["Get Login Target<br/>Target is BPI Login Info"]
  n7["Execute Console Command"]
  n8["Bind Event to On Server Start"]
  n0 -->|"then"| n4
  n1 -->|"then"| n8
  n2 -->|"then"| n3
  n4 -->|"then"| n1
  n5 -->|"then"| n6
  n6 -->|"then"| n7
  n8 -->|"then"| n2
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_Event_3` | BeginPlay 이벤트 |  |
| `K2Node_CreateWidget_1` | Create WBP Server Connect UI Widget | Class=/Game/QuakeLike_1_0/UI/ServerUI/WBP_ServerConnectUI.WBP_ServerConnectUI_C |
| `K2Node_CallFunction_0` | Add to Viewport<br/>타깃은 사용자 위젯 | ZOrder=0 |
| `K2Node_Self_1` | 셀프-레퍼런스 |  |
| `K2Node_VariableSet_2` | Set bShowMouseCursor | bShowMouseCursor=true; Output_Get=false |
| `K2Node_CallFunction_4` | Is Local Player Controller<br/>타깃은 컨트롤러 |  |
| `K2Node_IfThenElse_0` | Branch |  |
| `K2Node_CustomEvent_0` | Try Login<br/>Custom Event |  |
| `K2Node_CallFunction_3` | Get Game Instance |  |
| `K2Node_Message_0` | Get Login Target<br/>Target is BPI Login Info |  |
| `K2Node_CallFunction_5` | Get Login Command |  |
| `K2Node_CallFunction_6` | Execute Console Command |  |
| `K2Node_Self_0` | 셀프-레퍼런스 |  |
| `K2Node_Knot_0` | 경유 노드 |  |
| `K2Node_AddDelegate_0` | Bind Event to On Server Start |  |

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

- `ExecuteUbergraph_BP_LoginController`: Graph 'ExecuteUbergraph_BP_LoginController' not found in Blueprint.
- `ReceiveBeginPlay`: Graph 'ReceiveBeginPlay' not found in Blueprint.
- `Try Login`: Graph 'Try Login' not found in Blueprint.
- `UserConstructionScript_MERGED`: Graph 'UserConstructionScript_MERGED' not found in Blueprint.

[원본 MCP 스냅샷](../../../Evidence/blueprints-210.json)
