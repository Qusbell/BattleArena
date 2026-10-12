# WB_Crosshair_02

에셋: `/Game/CompShooterHUD/UI/Content/Crosshairs/Crosshairs/WB_Crosshair_02`

부모: `/Script/CoreUObject.Class'/Script/UMG.UserWidget'`

수집일: 2026-10-12. MCP에서 읽은 그래프·핀 연결의 자동 전사입니다. 상속된 그래프는 부모 문서를 함께 확인합니다.

정적 루트 참조 도달: 있음. 실행 여부 또는 삭제 가능 여부의 확정이 아닙니다.

컴포넌트 목록은 이 BP가 직접 정의한 SCS만 포함합니다. 부모·네이티브 생성 컴포넌트, 인스턴스 설정은 별도입니다.

## 변수와 기본값

| 이름 | 타입 | CDO 기본값 |
|---|---|---|
| `IsMoving` | `bool` | False |

## 그래프 목록

| 그래프 | 전체 노드 수 |
|---|---|
| EventGraph | 27 |
| Get_WB_CrosshairBase | 4 |

## 그래프별 실행 연결

실선은 출력 exec 핀의 실제 연결입니다. 여러 분기는 출력 핀 이름을 표시합니다. 함수 호출·delegate·타이머의 다음 그래프 실행은 이 그림에서 자동으로 이어 붙이지 않습니다.

순수 계산 노드 및 데이터 핀은 아래 목록과 원본 JSON을 확인합니다. 연결되지 않은 이벤트도 표시되므로, 노드가 있다는 이유만으로 실행된다고 판단하면 안 됩니다.

### EventGraph

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["RemoveCrosshair 이벤트<br/>I Crosshair Manager에서"]
  n1["Recoil 이벤트<br/>I Crosshair Manager에서"]
  n2["ActivateCrosshair 이벤트<br/>I Crosshair Manager에서"]
  n3["Recoil 1 Anim"]
  n4["Activate Anim"]
  n5["Activate Anim"]
  n6["Delay"]
  n7["Call On Crosshair Removed"]
  n8["Is Valid"]
  n9["CrosshairReaction 이벤트<br/>I Crosshair Manager에서"]
  n10["Set Scale"]
  n11["ZoomIn 이벤트<br/>I Crosshair Manager에서"]
  n12["ZoomOut 이벤트<br/>I Crosshair Manager에서"]
  n13["Set IsMoving"]
  n14["HandleMovementReaction<br/>Collapsed Graph"]
  n15["Do Once"]
  n16["Do Once"]
  n17["Zoom Anim"]
  n18["Zoom Anim"]
  n0 -->|"then"| n5
  n1 -->|"then"| n3
  n2 -->|"then"| n4
  n4 -->|"then"| n10
  n5 -->|"then"| n6
  n6 -->|"then"| n8
  n8 -->|"Is Valid"| n7
  n9 -->|"then"| n13
  n11 -->|"then"| n15
  n12 -->|"then"| n16
  n13 -->|"then"| n14
  n15 -->|"Completed"| n17
  n16 -->|"Completed"| n18
  n17 -->|"then"| n16
  n18 -->|"then"| n15
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_Event_4` | RemoveCrosshair 이벤트<br/>I Crosshair Manager에서 |  |
| `EdGraphNode_Comment_1` | Fade Out Anim Action |  |
| `EdGraphNode_Comment_12` | Event Recoil |  |
| `EdGraphNode_Comment_4` | Event Activate Crosshair |  |
| `EdGraphNode_Comment_5` | Event Remove Crosshair |  |
| `K2Node_Event_39` | Recoil 이벤트<br/>I Crosshair Manager에서 |  |
| `K2Node_Event_41` | ActivateCrosshair 이벤트<br/>I Crosshair Manager에서 |  |
| `K2Node_MacroInstance_962` | Recoil 1 Anim | Speed=1 |
| `K2Node_MacroInstance_20` | Activate Anim | FadeIn=true |
| `K2Node_MacroInstance_993` | Activate Anim |  |
| `K2Node_CallFunction_449` | Delay | Duration=0.125 |
| `K2Node_CallFunction_20` | Get WB Crosshair Base |  |
| `K2Node_CallDelegate_0` | Call On Crosshair Removed |  |
| `K2Node_MacroInstance_22` | Is Valid |  |
| `K2Node_Knot_53` | 경유 노드 |  |
| `K2Node_Event_62` | CrosshairReaction 이벤트<br/>I Crosshair Manager에서 |  |
| `K2Node_MacroInstance_1568` | Set Scale |  |
| `EdGraphNode_Comment_18` | Event Crosshair Reaction |  |
| `K2Node_Event_11` | ZoomIn 이벤트<br/>I Crosshair Manager에서 |  |
| `K2Node_Event_13` | ZoomOut 이벤트<br/>I Crosshair Manager에서 |  |
| `EdGraphNode_Comment_11` | Event Zoom In / Zoom Out |  |
| `K2Node_VariableSet_166` | Set IsMoving | Output_Get=false |
| `K2Node_Composite_88` | HandleMovementReaction<br/>Collapsed Graph |  |
| `K2Node_MacroInstance_76` | Do Once |  |
| `K2Node_MacroInstance_75` | Do Once |  |
| `K2Node_MacroInstance_125` | Zoom Anim | ZoomIn=true |
| `K2Node_MacroInstance_273` | Zoom Anim |  |

</details>

### Get_WB_CrosshairBase

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["Get_WB_CrosshairBase"]
  n1["반환 노드"]
  n2["Get All Widgets Of Class"]
  n0 -->|"then"| n2
  n2 -->|"then"| n1
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_15` | Get_WB_CrosshairBase |  |
| `K2Node_FunctionResult_0` | 반환 노드 |  |
| `K2Node_CallFunction_5` | Get All Widgets Of Class | WidgetClass=/Game/CompShooterHUD/UI/Content/Crosshairs/WB_CrosshairBase.WB_CrosshairBase_C; TopLevelOnly=false |
| `K2Node_GetArrayItem_0` | GET | Dimension 1=0 |

</details>

## 조회 범위의 한계

ObjectIterator로 발견했으나 MCP helper가 반환하지 않은 그래프입니다. delegate signature·컴파일된 중간 그래프·macro·애니메이션 내부 그래프가 섞일 수 있습니다.

- `ActivateCrosshair`: Graph 'ActivateCrosshair' not found in Blueprint.
- `Activate_Anim`: Graph 'Activate_Anim' not found in Blueprint.
- `CrosshairReaction`: Graph 'CrosshairReaction' not found in Blueprint.
- `HitIndicator`: Graph 'HitIndicator' not found in Blueprint.
- `OnMovementReaction`: Graph 'OnMovementReaction' not found in Blueprint.
- `Reaction_Anim`: Graph 'Reaction_Anim' not found in Blueprint.
- `Recoil`: Graph 'Recoil' not found in Blueprint.
- `Recoil1_Anim`: Graph 'Recoil1_Anim' not found in Blueprint.
- `RemoveCrosshair`: Graph 'RemoveCrosshair' not found in Blueprint.
- `RemoveProgressBar`: Graph 'RemoveProgressBar' not found in Blueprint.
- `SetScale`: Graph 'SetScale' not found in Blueprint.
- `UpdateProgressBar`: Graph 'UpdateProgressBar' not found in Blueprint.
- `ZoomIn`: Graph 'ZoomIn' not found in Blueprint.
- `ZoomOut`: Graph 'ZoomOut' not found in Blueprint.
- `Zoom_Anim`: Graph 'Zoom_Anim' not found in Blueprint.

[원본 MCP 스냅샷](../../../../../../Evidence/blueprints-316.json)
