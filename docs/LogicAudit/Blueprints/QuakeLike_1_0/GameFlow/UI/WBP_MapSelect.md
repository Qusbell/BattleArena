# WBP_MapSelect

에셋: `/Game/QuakeLike_1_0/GameFlow/UI/WBP_MapSelect`

부모: `/Script/CoreUObject.Class'/Script/UMG.UserWidget'`

수집일: 2026-10-12. MCP에서 읽은 그래프·핀 연결의 자동 전사입니다. 상속된 그래프는 부모 문서를 함께 확인합니다.

정적 루트 참조 도달: 있음. 실행 여부 또는 삭제 가능 여부의 확정이 아닙니다.

컴포넌트 목록은 이 BP가 직접 정의한 SCS만 포함합니다. 부모·네이티브 생성 컴포넌트, 인스턴스 설정은 별도입니다.

## 변수와 기본값

| 이름 | 타입 | CDO 기본값 |
|---|---|---|
| `Selected_Map_ID_Local` | `name` | None |
| `MapRowNames` | `name` | [] |

## 그래프 목록

| 그래프 | 전체 노드 수 |
|---|---|
| EventGraph | 80 |

## 그래프별 실행 연결

실선은 출력 exec 핀의 실제 연결입니다. 여러 분기는 출력 핀 이름을 표시합니다. 함수 호출·delegate·타이머의 다음 그래프 실행은 이 그림에서 자동으로 이어 붙이지 않습니다.

순수 계산 노드 및 데이터 핀은 아래 목록과 원본 JSON을 확인합니다. 연결되지 않은 이벤트도 표시되므로, 노드가 있다는 이유만으로 실행된다고 판단하면 안 됩니다.

### EventGraph

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["On Clicked (Btn_Back)"]
  n1["Create WBP Main Menu Widget"]
  n2["Add to Viewport<br/>타깃은 사용자 위젯"]
  n3["Remove from Parent<br/>타깃은 위젯"]
  n4["Construct 이벤트"]
  n5["Clear"]
  n6["Get Data Table Row Names"]
  n7["For Each Loop"]
  n8["Branch"]
  n9["Add"]
  n10["Refresh Map Page<br/>타깃은 WBP Map Select"]
  n11["SelectMapByIndex<br/>Custom Event"]
  n12["Branch"]
  n13["Set Selected_Map_ID_Local"]
  n14["Cast To BP_QuakeGameInstance"]
  n15["Set Selected_Map_ID"]
  n16["On Clicked (LevelFrame)"]
  n17["RefreshMapPage<br/>Custom Event"]
  n18["Get Data Table Row DT_MapData"]
  n19["On Clicked (LevelFrame_2)"]
  n20["For Each Loop"]
  n21["Branch"]
  n22["Set Visibility<br/>타깃은 위젯"]
  n23["Get Data Table Row DT_MapData"]
  n24["SetText (Text)<br/>타깃은 텍스트"]
  n25["Set Image Brush"]
  n26["Start Single Server<br/>Target is BPI Login Info"]
  n27["Stop Single Server<br/>Target is BPI Login Info"]
  n28["Select Map by Index<br/>타깃은 WBP Map Select"]
  n29["Select Map by Index<br/>타깃은 WBP Map Select"]
  n30["Create WBP Difficulty Select Widget"]
  n31["Add to Viewport<br/>타깃은 사용자 위젯"]
  n32["Remove from Parent<br/>타깃은 위젯"]
  n33["Branch"]
  n34["Refresh Map Page<br/>타깃은 WBP Map Select"]
  n0 -->|"then"| n27
  n1 -->|"then"| n2
  n2 -->|"then"| n3
  n4 -->|"then"| n5
  n5 -->|"then"| n6
  n6 -->|"then"| n7
  n7 -->|"LoopBody"| n18
  n7 -->|"Completed"| n33
  n8 -->|"then"| n9
  n11 -->|"then"| n12
  n12 -->|"then"| n13
  n13 -->|"then"| n14
  n14 -->|"then"| n15
  n15 -->|"then"| n30
  n16 -->|"then"| n28
  n17 -->|"then"| n20
  n18 -->|"then"| n8
  n19 -->|"then"| n29
  n20 -->|"LoopBody"| n21
  n21 -->|"then"| n22
  n22 -->|"then"| n23
  n23 -->|"then"| n24
  n24 -->|"then"| n25
  n26 -->|"then"| n10
  n27 -->|"then"| n1
  n30 -->|"then"| n31
  n31 -->|"then"| n32
  n33 -->|"then"| n26
  n33 -->|"else"| n34
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_Keep_K2Node_ComponentBoundEvent_0` | On Clicked (Btn_Back) |  |
| `K2Node_Keep_K2Node_CreateWidget_1` | Create WBP Main Menu Widget | Class=/Game/QuakeLike_1_0/GameFlow/UI/WBP_MainMenu.WBP_MainMenu_C |
| `K2Node_Keep_K2Node_CallFunction_3` | Add to Viewport<br/>타깃은 사용자 위젯 | ZOrder=0 |
| `K2Node_Keep_K2Node_CallFunction_5` | Get Owning Player<br/>타깃은 위젯 |  |
| `K2Node_Keep_K2Node_CallFunction_6` | Remove from Parent<br/>타깃은 위젯 |  |
| `K2Node_Construct` | Construct 이벤트 |  |
| `K2Node_ClearMapRows` | Clear |  |
| `K2Node_Get_MapRowNames_15` | Get MapRowNames |  |
| `K2Node_ReadMapTable` | Get Data Table Row Names | Table=/Game/QuakeLike_1_0/GameFlow/Table/DT/DT_MapData.DT_MapData |
| `K2Node_ForEachMapRow` | For Each Loop |  |
| `K2Node_FilterPlayableMaps` | Branch |  |
| `K2Node_AddMapRow` | Add | ReturnValue=0 |
| `K2Node_CallFunction_30` | Refresh Map Page<br/>타깃은 WBP Map Select |  |
| `K2Node_SelectMapByIndex` | SelectMapByIndex<br/>Custom Event |  |
| `K2Node_Get_MapRowNames_26` | Get MapRowNames |  |
| `K2Node_ValidateSelectedIndex` | Is Valid Index |  |
| `K2Node_HasSelectedMap` | Branch |  |
| `K2Node_GetSelectedRow` | Get |  |
| `K2Node_StoreSelectedRow` | Set Selected_Map_ID_Local |  |
| `K2Node_SelectionGameInstance` | Get Game Instance<br/>타깃은 위젯 |  |
| `K2Node_SelectionCast` | Cast To BP_QuakeGameInstance |  |
| `K2Node_StoreGameInstanceMap` | Set Selected_Map_ID | Output_Get=None |
| `K2Node_Lvl_Image_1` | On Clicked (LevelFrame) |  |
| `K2Node_RefreshMapPage` | RefreshMapPage<br/>Custom Event |  |
| `K2Node_ReadCandidateMap` | Get Data Table Row DT_MapData | DataTable=/Game/QuakeLike_1_0/GameFlow/Table/DT/DT_MapData.DT_MapData |
| `K2Node_HasMapImage` | Is Valid |  |
| `K2Node_Knot_11` | 경유 노드 |  |
| `K2Node_Knot_12` | 경유 노드 |  |
| `K2Node_Knot_6` | 경유 노드 |  |
| `K2Node_Knot_10` | 경유 노드 |  |
| `K2Node_ComponentBoundEvent_6` | On Clicked (LevelFrame_2) |  |
| `K2Node_VariableGet_11` | Get Lvl_Image |  |
| `K2Node_VariableGet_16` | Get Lvl_Image_2 |  |
| `K2Node_MakeArray_1` | 배열 만들기 |  |
| `K2Node_MacroInstance_1` | For Each Loop |  |
| `K2Node_Get_MapRowNames_80` | Get MapRowNames |  |
| `K2Node_Get_Selected_Map_ID_Local_81` | Get Selected_Map_ID_Local |  |
| `K2Node_SelectedIndex_82` | Find Item |  |
| `K2Node_Page_83` | int / int | B=4 |
| `K2Node_PageStart_84` | int * int | B=4 |
| `K2Node_DisplayedRowIndex` | int + int | B=1 |
| `K2Node_ValidVisibleRow` | Is Valid Index |  |
| `K2Node_Get_MapRowNames_102` | Get MapRowNames |  |
| `K2Node_SlotHasMap` | Branch |  |
| `K2Node_ShowMapImage` | Set Visibility<br/>타깃은 위젯 | InVisibility=Visible |
| `K2Node_ReadDisplayedMap` | Get Data Table Row DT_MapData | DataTable=/Game/QuakeLike_1_0/GameFlow/Table/DT/DT_MapData.DT_MapData |
| `K2Node_MapDisplayName` | To Text (String) |  |
| `K2Node_SetMapName` | SetText (Text)<br/>타깃은 텍스트 |  |
| `K2Node_Get_Selected_Map_ID_Local_118` | Get Selected_Map_ID_Local |  |
| `K2Node_ThisMapIsSelected` | Equal (Name) | ReturnValue=false |
| `K2Node_Knot_9` | 경유 노드 |  |
| `K2Node_DisplayedRowName` | Get |  |
| `K2Node_Knot_20` | 경유 노드 |  |
| `K2Node_Knot_21` | 경유 노드 |  |
| `K2Node_Array_MapTT_` | 배열 만들기 |  |
| `K2Node_Get_MapTT_1_91` | Get MapTT_1 |  |
| `K2Node_Get_MapTT_2_92` | Get MapTT_2 |  |
| `K2Node_SlotText` | Get |  |
| `K2Node_Knot_22` | 경유 노드 |  |
| `K2Node_Knot_24` | 경유 노드 |  |
| `K2Node_Knot_25` | 경유 노드 |  |
| `K2Node_Knot_26` | 경유 노드 |  |
| `K2Node_Knot_27` | 경유 노드 |  |
| `K2Node_Knot_35` | 경유 노드 |  |
| `K2Node_MacroInstance_2` | Set Image Brush |  |
| `K2Node_Knot_7` | 경유 노드 |  |
| `K2Node_CallFunction_16` | Get Game Instance<br/>타깃은 위젯 |  |
| `K2Node_Message_3` | Start Single Server<br/>Target is BPI Login Info | Initial Map Name=MainMenu_Level; Port=7779 |
| `K2Node_CallFunction_18` | Get Game Instance<br/>타깃은 위젯 |  |
| `K2Node_Message_4` | Stop Single Server<br/>Target is BPI Login Info |  |
| `K2Node_CallFunction_19` | Select Map by Index<br/>타깃은 WBP Map Select | Index=0 |
| `K2Node_CallFunction_20` | Select Map by Index<br/>타깃은 WBP Map Select | Index=1 |
| `K2Node_CreateWidget_0` | Create WBP Difficulty Select Widget | Class=/Game/QuakeLike_1_0/GameFlow/UI/WBP_DifficultySelect.WBP_DifficultySelect_C |
| `K2Node_CallFunction_21` | Get Owning Player<br/>타깃은 위젯 |  |
| `K2Node_CallFunction_25` | Add to Viewport<br/>타깃은 사용자 위젯 | ZOrder=0 |
| `K2Node_CallFunction_26` | Remove from Parent<br/>타깃은 위젯 |  |
| `K2Node_IfThenElse_8` | Branch |  |
| `K2Node_CallFunction_28` | Is Local Dedicated Server Running |  |
| `K2Node_CallFunction_29` | NOT Boolean |  |
| `K2Node_RefreshMapPage_23` | Refresh Map Page<br/>타깃은 WBP Map Select |  |

</details>

## 조회 범위의 한계

ObjectIterator로 발견했으나 MCP helper가 반환하지 않은 그래프입니다. delegate signature·컴파일된 중간 그래프·macro·애니메이션 내부 그래프가 섞일 수 있습니다.

- `BndEvt__WBP_MapSelect_Btn_Back_K2Node_ComponentBoundEvent_0_OnButtonClickedEvent__DelegateSignature`: Graph 'BndEvt__WBP_MapSelect_Btn_Back_K2Node_ComponentBoundEvent_0_OnButtonClickedEvent__DelegateSignature' not found in Blueprint.
- `BndEvt__WBP_MapSelect_LevelFrame_2_K2Node_ComponentBoundEvent_0_OnButtonClickedEvent__DelegateSignature`: Graph 'BndEvt__WBP_MapSelect_LevelFrame_2_K2Node_ComponentBoundEvent_0_OnButtonClickedEvent__DelegateSignature' not found in Blueprint.
- `BndEvt__WBP_MapSelect_Lvl_Image_1_K2Node_ComponentBoundEvent_2_OnButtonClickedEvent__DelegateSignature`: Graph 'BndEvt__WBP_MapSelect_Lvl_Image_1_K2Node_ComponentBoundEvent_2_OnButtonClickedEvent__DelegateSignature' not found in Blueprint.
- `Construct`: Graph 'Construct' not found in Blueprint.
- `RefreshMapPage`: Graph 'RefreshMapPage' not found in Blueprint.
- `SelectMapByIndex`: Graph 'SelectMapByIndex' not found in Blueprint.

[원본 MCP 스냅샷](../../../../Evidence/blueprints-105.json)
