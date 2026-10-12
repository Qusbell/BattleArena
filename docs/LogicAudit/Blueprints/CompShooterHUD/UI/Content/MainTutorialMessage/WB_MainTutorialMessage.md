# WB_MainTutorialMessage

에셋: `/Game/CompShooterHUD/UI/Content/MainTutorialMessage/WB_MainTutorialMessage`

부모: `/Script/CoreUObject.Class'/Script/UMG.UserWidget'`

수집일: 2026-10-12. MCP에서 읽은 그래프·핀 연결의 자동 전사입니다. 상속된 그래프는 부모 문서를 함께 확인합니다.

정적 루트 참조 도달: 있음. 실행 여부 또는 삭제 가능 여부의 확정이 아닙니다.

컴포넌트 목록은 이 BP가 직접 정의한 SCS만 포함합니다. 부모·네이티브 생성 컴포넌트, 인스턴스 설정은 별도입니다.

## 변수와 기본값

| 이름 | 타입 | CDO 기본값 |
|---|---|---|
| `RemoveMainTutorialMessage` | `mcdelegate` | &lt;Delegate 'DelegateBase' &lt;Unbound&gt;&gt; |
| `MainTutorialWindow` | `WB_MainTutorialWindow_C` | None |
| `Blur` | `bool` | False |

## 그래프 목록

| 그래프 | 전체 노드 수 |
|---|---|
| EventGraph | 47 |
| RemoveTutorialVideo | 3 |
| UpdateTutorialVideo | 4 |

## 그래프별 실행 연결

실선은 출력 exec 핀의 실제 연결입니다. 여러 분기는 출력 핀 이름을 표시합니다. 함수 호출·delegate·타이머의 다음 그래프 실행은 이 그림에서 자동으로 이어 붙이지 않습니다.

순수 계산 노드 및 데이터 핀은 아래 목록과 원본 JSON을 확인합니다. 연결되지 않은 이벤트도 표시되므로, 노드가 있다는 이유만으로 실행된다고 판단하면 안 됩니다.

### EventGraph

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["AddMainTutorialMessage<br/>Custom Event"]
  n1["Anim Fade Background Blur"]
  n2["Branch"]
  n3["Call Remove Main Tutorial Message"]
  n4["RemoveMainTutorial<br/>Custom Event"]
  n5["Delay"]
  n6["Anim Fade Background Blur"]
  n7["경유 노드"]
  n8["Add Main Tutorial Window"]
  n9["Set Blur"]
  n10["Branch"]
  n11["Remove Video"]
  n12["Update Tutorial Video<br/>타깃은 WB Main Tutorial Message"]
  n13["Update Headline Icon Info<br/>타깃은 WB Main Tutorial Window"]
  n14["Update Tutorial Message Info<br/>타깃은 WB Main Tutorial Window"]
  n15["Update Background Color<br/>타깃은 WB Main Tutorial Window"]
  n16["Is Valid"]
  n17["Add Custom Text Widget<br/>타깃은 WB Main Tutorial Window"]
  n18["AddCustomTextWidget<br/>Custom Event"]
  n19["Is Valid"]
  n20["Update Button Info<br/>타깃은 WB Main Tutorial Window"]
  n21["UpdateButtonInfo<br/>Custom Event"]
  n0 -->|"then"| n9
  n1 -->|"then"| n7
  n2 -->|"then"| n1
  n2 -->|"else"| n7
  n4 -->|"then"| n11
  n5 -->|"then"| n3
  n6 -->|"then"| n5
  n7 -->|"OutputPin"| n8
  n8 -->|"then"| n12
  n9 -->|"then"| n2
  n10 -->|"then"| n6
  n10 -->|"else"| n5
  n11 -->|"then"| n10
  n12 -->|"then"| n13
  n13 -->|"then"| n14
  n14 -->|"then"| n15
  n16 -->|"Is Valid"| n17
  n18 -->|"then"| n16
  n19 -->|"Is Valid"| n20
  n21 -->|"then"| n19
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_CustomEvent_0` | AddMainTutorialMessage<br/>Custom Event |  |
| `K2Node_MacroInstance_25` | Anim Fade Background Blur | bVisible=true |
| `K2Node_IfThenElse_0` | Branch |  |
| `K2Node_CallDelegate_3` | Call Remove Main Tutorial Message |  |
| `K2Node_CustomEvent_27` | RemoveMainTutorial<br/>Custom Event |  |
| `K2Node_CallFunction_46` | Delay | Duration=0.3 |
| `EdGraphNode_Comment_18` | Event Remove Main Tutorial |  |
| `K2Node_MacroInstance_65` | Anim Fade Background Blur |  |
| `EdGraphNode_Comment_6` | Event AddMainTutorialMessage |  |
| `K2Node_Knot_0` | 경유 노드 |  |
| `K2Node_MacroInstance_332` | Add Main Tutorial Window |  |
| `K2Node_VariableSet_207` | Set Blur |  |
| `K2Node_VariableGet_286` | Get Blur |  |
| `K2Node_IfThenElse_64` | Branch |  |
| `K2Node_MacroInstance_452` | Remove Video |  |
| `K2Node_CallFunction_331` | Update Tutorial Video<br/>타깃은 WB Main Tutorial Message | bLooping=true |
| `K2Node_BreakStruct_36` | Break S Tutorial Video Info |  |
| `K2Node_Knot_31` | 경유 노드 |  |
| `K2Node_VariableGet_71` | Get MainTutorialWindow |  |
| `K2Node_CallFunction_143` | Update Headline Icon Info<br/>타깃은 WB Main Tutorial Window |  |
| `K2Node_Knot_7` | 경유 노드 |  |
| `K2Node_Knot_8` | 경유 노드 |  |
| `K2Node_Knot_9` | 경유 노드 |  |
| `K2Node_Knot_10` | 경유 노드 |  |
| `K2Node_Knot_24` | 경유 노드 |  |
| `K2Node_VariableGet_84` | Get MainTutorialWindow |  |
| `K2Node_CallFunction_148` | Update Tutorial Message Info<br/>타깃은 WB Main Tutorial Window |  |
| `K2Node_Knot_5` | 경유 노드 |  |
| `K2Node_Knot_6` | 경유 노드 |  |
| `K2Node_VariableGet_659` | Get MainTutorialWindow |  |
| `K2Node_CallFunction_170` | Update Background Color<br/>타깃은 WB Main Tutorial Window |  |
| `K2Node_Knot_158` | 경유 노드 |  |
| `K2Node_Knot_159` | 경유 노드 |  |
| `K2Node_VariableGet_193` | Get MainTutorialWindow |  |
| `K2Node_MacroInstance_63` | Is Valid |  |
| `K2Node_CallFunction_45` | Add Custom Text Widget<br/>타깃은 WB Main Tutorial Window |  |
| `K2Node_CustomEvent_5` | AddCustomTextWidget<br/>Custom Event |  |
| `K2Node_Knot_186` | 경유 노드 |  |
| `K2Node_Knot_185` | 경유 노드 |  |
| `EdGraphNode_Comment_2` | Event Add Custom Text Widget |  |
| `K2Node_VariableGet_104` | Get MainTutorialWindow |  |
| `K2Node_Knot_4` | 경유 노드 |  |
| `K2Node_MacroInstance_10` | Is Valid |  |
| `K2Node_Knot_3` | 경유 노드 |  |
| `K2Node_CallFunction_350` | Update Button Info<br/>타깃은 WB Main Tutorial Window |  |
| `K2Node_CustomEvent_54` | UpdateButtonInfo<br/>Custom Event |  |
| `EdGraphNode_Comment_5` | Event Update Button Info |  |

</details>

### RemoveTutorialVideo

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["RemoveTutorialVideo"]
  n1["Remove Video<br/>타깃은 WB Main Tutorial Window"]
  n0 -->|"then"| n1
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_36` | RemoveTutorialVideo |  |
| `K2Node_VariableGet_187` | Get MainTutorialWindow |  |
| `K2Node_CallFunction_68` | Remove Video<br/>타깃은 WB Main Tutorial Window |  |

</details>

### UpdateTutorialVideo

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["UpdateTutorialVideo"]
  n1["Update Tutorial Video<br/>타깃은 WB Main Tutorial Window"]
  n2["반환 노드"]
  n0 -->|"then"| n1
  n1 -->|"then"| n2
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_36` | UpdateTutorialVideo |  |
| `K2Node_CallFunction_40` | Update Tutorial Video<br/>타깃은 WB Main Tutorial Window |  |
| `K2Node_VariableGet_263` | Get MainTutorialWindow |  |
| `K2Node_FunctionResult_0` | 반환 노드 |  |

</details>

## 조회 범위의 한계

ObjectIterator로 발견했으나 MCP helper가 반환하지 않은 그래프입니다. delegate signature·컴파일된 중간 그래프·macro·애니메이션 내부 그래프가 섞일 수 있습니다.

- `AddCustomTextWidget`: Graph 'AddCustomTextWidget' not found in Blueprint.
- `AddMainTutorialMessage`: Graph 'AddMainTutorialMessage' not found in Blueprint.
- `AddMainTutorialWindow`: Graph 'AddMainTutorialWindow' not found in Blueprint.
- `AnimFadeBackgroundBlur`: Graph 'AnimFadeBackgroundBlur' not found in Blueprint.
- `RemoveMainTutorial`: Graph 'RemoveMainTutorial' not found in Blueprint.
- `RemoveMainTutorialMessage`: Graph 'RemoveMainTutorialMessage' not found in Blueprint.
- `RemoveVideo`: Graph 'RemoveVideo' not found in Blueprint.
- `UpdateButtonInfo`: Graph 'UpdateButtonInfo' not found in Blueprint.

[원본 MCP 스냅샷](../../../../../Evidence/blueprints-316.json)
