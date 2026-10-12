# WBP_IPInput

에셋: `/Game/QuakeLike_1_0/GameFlow/UI/WBP_IPInput`

부모: `/Script/CoreUObject.Class'/Script/UMG.UserWidget'`

수집일: 2026-10-12. MCP에서 읽은 그래프·핀 연결의 자동 전사입니다. 상속된 그래프는 부모 문서를 함께 확인합니다.

정적 루트 참조 도달: 있음. 실행 여부 또는 삭제 가능 여부의 확정이 아닙니다.

컴포넌트 목록은 이 BP가 직접 정의한 SCS만 포함합니다. 부모·네이티브 생성 컴포넌트, 인스턴스 설정은 별도입니다.

## 변수와 기본값

| 이름 | 타입 | CDO 기본값 |
|---|---|---|
| `UserIP` | `string` |  |

## 그래프 목록

| 그래프 | 전체 노드 수 |
|---|---|
| EventGraph | 52 |

## 그래프별 실행 연결

실선은 출력 exec 핀의 실제 연결입니다. 여러 분기는 출력 핀 이름을 표시합니다. 함수 호출·delegate·타이머의 다음 그래프 실행은 이 그림에서 자동으로 이어 붙이지 않습니다.

순수 계산 노드 및 데이터 핀은 아래 목록과 원본 JSON을 확인합니다. 연결되지 않은 이벤트도 표시되므로, 노드가 있다는 이유만으로 실행된다고 판단하면 안 됩니다.

### EventGraph

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["Branch"]
  n1["Set Visibility<br/>타깃은 위젯"]
  n2["Delay"]
  n3["Set Visibility<br/>타깃은 위젯"]
  n4["On Clicked (Btn_Connect)"]
  n5["Cast To BP_QuakeGameInstance"]
  n6["Set Target IP"]
  n7["Set Target Port"]
  n8["Execute Console Command"]
  n9["Start Match Server"]
  n10["Branch"]
  n11["Delay"]
  n12["경유 노드"]
  n13["Set UserIP"]
  n14["Branch"]
  n15["Set Target IP"]
  n16["Set Target Port"]
  n17["Execute Console Command"]
  n18["Set UserIP"]
  n19["Branch"]
  n20["경유 노드"]
  n21["Begin Loading Screen"]
  n22["경유 노드"]
  n23["On Clicked (Btn_Back)"]
  n24["Remove from Parent<br/>타깃은 위젯"]
  n0 -->|"then"| n22
  n0 -->|"else"| n1
  n1 -->|"then"| n2
  n2 -->|"then"| n3
  n4 -->|"then"| n0
  n5 -->|"then"| n19
  n6 -->|"then"| n7
  n7 -->|"then"| n8
  n9 -->|"then"| n10
  n10 -->|"then"| n12
  n10 -->|"else"| n11
  n11 -->|"then"| n10
  n12 -->|"OutputPin"| n14
  n13 -->|"then"| n6
  n14 -->|"then"| n13
  n14 -->|"else"| n18
  n15 -->|"then"| n16
  n16 -->|"then"| n17
  n18 -->|"then"| n15
  n19 -->|"then"| n21
  n19 -->|"else"| n20
  n20 -->|"OutputPin"| n18
  n21 -->|"then"| n9
  n22 -->|"OutputPin"| n5
  n23 -->|"then"| n24
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_VariableGet_4` | Get EditableText_IP |  |
| `K2Node_CallFunction_3` | Is Valid IPAddress |  |
| `K2Node_IfThenElse_3` | Branch |  |
| `K2Node_VariableGet_5` | Get Text_Warning |  |
| `K2Node_CallFunction_17` | Set Visibility<br/>타깃은 위젯 | InVisibility=Visible |
| `K2Node_CallFunction_18` | Delay | Duration=2.000000 |
| `K2Node_VariableGet_1` | Get Text_Warning |  |
| `K2Node_CallFunction_12` | Set Visibility<br/>타깃은 위젯 | InVisibility=Collapsed |
| `K2Node_ComponentBoundEvent_0` | On Clicked (Btn_Connect) |  |
| `K2Node_CallFunction_4` | Get Game Instance<br/>타깃은 위젯 |  |
| `K2Node_DynamicCast_0` | Cast To BP_QuakeGameInstance |  |
| `K2Node_VariableSet_2` | Set Target IP |  |
| `K2Node_VariableSet_4` | Set Target Port | Target Port=7777 |
| `K2Node_CallFunction_13` | Get Login Command |  |
| `K2Node_CallFunction_14` | Execute Console Command |  |
| `K2Node_CallFunction_7` | Start Match Server | MapName=Lobby_Level; Port=7777; ReturnValue=false |
| `K2Node_IfThenElse_0` | Branch |  |
| `K2Node_CallFunction_8` | Is Match Server Ready |  |
| `K2Node_Knot_4` | 경유 노드 |  |
| `K2Node_CallFunction_9` | Delay | Duration=0.2 |
| `K2Node_Knot_1` | 경유 노드 |  |
| `K2Node_Knot_2` | 경유 노드 |  |
| `K2Node_Knot_7` | 경유 노드 |  |
| `K2Node_Knot_0` | 경유 노드 |  |
| `K2Node_VariableSet_5` | Set UserIP |  |
| `K2Node_CallFunction_15` | To String (Text) |  |
| `K2Node_VariableGet_3` | Get Text |  |
| `K2Node_IfThenElse_1` | Branch |  |
| `K2Node_VariableSet_0` | Set Target IP |  |
| `K2Node_VariableSet_1` | Set Target Port | Target Port=7777 |
| `K2Node_CallFunction_5` | Get Login Command |  |
| `K2Node_CallFunction_10` | Execute Console Command |  |
| `K2Node_Knot_5` | 경유 노드 |  |
| `K2Node_VariableSet_3` | Set UserIP |  |
| `K2Node_IfThenElse_2` | Branch |  |
| `K2Node_CallFunction_32` | To String (Text) |  |
| `K2Node_VariableGet_8` | Get Text |  |
| `K2Node_Knot_8` | 경유 노드 |  |
| `K2Node_CallFunction_6` | Is Local IPAddress |  |
| `K2Node_Knot_3` | 경유 노드 |  |
| `K2Node_Knot_6` | 경유 노드 |  |
| `K2Node_Knot_9` | 경유 노드 |  |
| `K2Node_Knot_10` | 경유 노드 |  |
| `K2Node_CallFunction_16` | Is Empty |  |
| `K2Node_CallFunction_11` | Begin Loading Screen | StatusText=멀티 플레이로 이동하는 중... |
| `K2Node_Knot_11` | 경유 노드 |  |
| `K2Node_VariableGet_6` | Get EditableText_IP |  |
| `K2Node_VariableGet_7` | Get EditableText_IP |  |
| `K2Node_ComponentBoundEvent_1` | On Clicked (Btn_Back) |  |
| `K2Node_CallFunction_19` | Remove from Parent<br/>타깃은 위젯 |  |
| `K2Node_VariableGet_2` | Get Text |  |
| `K2Node_CallFunction_21` | To String (Text) |  |

</details>

## 조회 범위의 한계

ObjectIterator로 발견했으나 MCP helper가 반환하지 않은 그래프입니다. delegate signature·컴파일된 중간 그래프·macro·애니메이션 내부 그래프가 섞일 수 있습니다.

- `BndEvt__WBP_IPInput_Btn_Back_K2Node_ComponentBoundEvent_1_OnButtonClickedEvent__DelegateSignature`: Graph 'BndEvt__WBP_IPInput_Btn_Back_K2Node_ComponentBoundEvent_1_OnButtonClickedEvent__DelegateSignature' not found in Blueprint.
- `BndEvt__WBP_IPInput_Btn_Connect_K2Node_ComponentBoundEvent_0_OnButtonClickedEvent__DelegateSignature`: Graph 'BndEvt__WBP_IPInput_Btn_Connect_K2Node_ComponentBoundEvent_0_OnButtonClickedEvent__DelegateSignature' not found in Blueprint.

[원본 MCP 스냅샷](../../../../Evidence/blueprints-000.json)
