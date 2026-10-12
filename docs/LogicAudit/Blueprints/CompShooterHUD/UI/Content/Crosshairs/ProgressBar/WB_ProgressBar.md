# WB_ProgressBar

에셋: `/Game/CompShooterHUD/UI/Content/Crosshairs/ProgressBar/WB_ProgressBar`

부모: `/Script/CoreUObject.Class'/Script/UMG.UserWidget'`

수집일: 2026-10-12. MCP에서 읽은 그래프·핀 연결의 자동 전사입니다. 상속된 그래프는 부모 문서를 함께 확인합니다.

정적 루트 참조 도달: 있음. 실행 여부 또는 삭제 가능 여부의 확정이 아닙니다.

컴포넌트 목록은 이 BP가 직접 정의한 SCS만 포함합니다. 부모·네이티브 생성 컴포넌트, 인스턴스 설정은 별도입니다.

## 변수와 기본값

| 이름 | 타입 | CDO 기본값 |
|---|---|---|
| `ProgressTime` | `real` | 0.0 |
| `Timesteps_Time` | `real` | 0.0010000000474974513 |
| `LiveProgress` | `real` | 0.0 |
| `Opacity` | `real` | 1.0 |
| `FillColor` | `LinearColor` | &lt;Struct 'LinearColor' {r: 0.000000, g: 0.167567, b: 1.000000, a: 1.000000}&gt; |
| `EmptyColor` | `LinearColor` | &lt;Struct 'LinearColor' {r: 1.000000, g: 1.000000, b: 1.000000, a: 0.250000}&gt; |
| `CurrentValue` | `real` | 0.0 |

## 그래프 목록

| 그래프 | 전체 노드 수 |
|---|---|
| EventGraph | 63 |
| GetOwnerHUD | 5 |
| UpdateAllInfos | 21 |
| UpdateShine01 | 4 |
| UpdateShine02 | 4 |
| UpdateShineColors | 6 |

## 그래프별 실행 연결

실선은 출력 exec 핀의 실제 연결입니다. 여러 분기는 출력 핀 이름을 표시합니다. 함수 호출·delegate·타이머의 다음 그래프 실행은 이 그림에서 자동으로 이어 붙이지 않습니다.

순수 계산 노드 및 데이터 핀은 아래 목록과 원본 JSON을 확인합니다. 연결되지 않은 이벤트도 표시되므로, 노드가 있다는 이유만으로 실행된다고 판단하면 안 됩니다.

### EventGraph

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["UpdateProgressBar 이벤트<br/>I Crosshair Manager에서"]
  n1["Set Timer by Event"]
  n2["UpdateTimedProgress<br/>Custom Event"]
  n3["Delay"]
  n4["Unpause Timer by Handle"]
  n5["Pause Timer by Handle"]
  n6["UpdateProgress<br/>Collapsed Graph"]
  n7["Set Opacity"]
  n8["Update Content Opacity"]
  n9["Set Timer by Event"]
  n10["FadeOut<br/>Custom Event"]
  n11["Set Opacity"]
  n12["Update Content Opacity"]
  n13["Unpause Timer by Handle"]
  n14["Pause Timer by Handle"]
  n15["Update Visibility"]
  n16["Update Visibility"]
  n17["Do Once"]
  n18["경유 노드"]
  n19["경유 노드"]
  n20["Call On Progress Bar Completed"]
  n21["SetData<br/>Collapsed Graph"]
  n22["Set Opacity"]
  n23["Update Content Opacity"]
  n24["Update All Infos<br/>타깃은 WB Progress Bar"]
  n25["Update Visibility"]
  n26["RemoveProgressBar 이벤트<br/>I Crosshair Manager에서"]
  n27["Update Visibility"]
  n28["Fade Anim"]
  n29["Delay"]
  n30["Fade Anim"]
  n31["FadeOutContent<br/>Collapsed Graph"]
  n32["Do Once"]
  n33["Fade Anim"]
  n34["Delay"]
  n35["Update Visibility"]
  n36["Branch"]
  n37["Set CurrentValue"]
  n38["경유 노드"]
  n39["HandleSizeSteps<br/>Collapsed Graph"]
  n40["경유 노드"]
  n41["Set LiveProgress"]
  n0 -->|"then"| n39
  n1 -->|"then"| n4
  n2 -->|"then"| n7
  n3 -->|"then"| n9
  n4 -->|"then"| n3
  n5 -->|"then"| n15
  n7 -->|"then"| n8
  n8 -->|"then"| n6
  n9 -->|"then"| n13
  n10 -->|"then"| n11
  n11 -->|"then"| n12
  n13 -->|"then"| n28
  n14 -->|"then"| n5
  n15 -->|"then"| n20
  n16 -->|"then"| n41
  n17 -->|"Completed"| n16
  n18 -->|"OutputPin"| n17
  n19 -->|"OutputPin"| n18
  n20 -->|"then"| n19
  n21 -->|"TimedProgress"| n17
  n21 -->|"UpdateProgress"| n40
  n22 -->|"then"| n23
  n23 -->|"then"| n24
  n24 -->|"then"| n32
  n25 -->|"then"| n22
  n26 -->|"then"| n30
  n28 -->|"then"| n29
  n29 -->|"then"| n14
  n30 -->|"then"| n31
  n31 -->|"then"| n27
  n32 -->|"Completed"| n33
  n33 -->|"then"| n34
  n34 -->|"then"| n35
  n35 -->|"then"| n37
  n36 -->|"then"| n38
  n36 -->|"else"| n25
  n38 -->|"OutputPin"| n32
  n39 -->|"then"| n21
  n40 -->|"OutputPin"| n36
  n41 -->|"then"| n1
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_Event_24` | UpdateProgressBar 이벤트<br/>I Crosshair Manager에서 |  |
| `K2Node_CallFunction_4859` | Set Timer by Event | bLooping=true; bMaxOncePerFrame=false; InitialStartDelay=0.000000; InitialStartDelayVariance=0.000000 |
| `K2Node_CustomEvent_0` | UpdateTimedProgress<br/>Custom Event |  |
| `K2Node_CallFunction_5030` | Delay |  |
| `K2Node_VariableGet_98` | Get ProgressTime |  |
| `K2Node_CallFunction_804` | Unpause Timer by Handle |  |
| `K2Node_VariableGet_1042` | Get Timesteps_Time |  |
| `K2Node_CallFunction_5031` | Pause Timer by Handle |  |
| `K2Node_MacroInstance_588` | Interp by Time | Target=1; InterpTime=0.15 |
| `K2Node_Composite_77` | UpdateProgress<br/>Collapsed Graph |  |
| `K2Node_VariableSet_835` | Set Opacity |  |
| `K2Node_VariableGet_1802` | Get Opacity |  |
| `K2Node_MacroInstance_589` | Update Content Opacity |  |
| `K2Node_CallFunction_644` | Set Timer by Event | bLooping=true; bMaxOncePerFrame=false; InitialStartDelay=0.000000; InitialStartDelayVariance=0.000000 |
| `K2Node_VariableGet_172` | Get Timesteps_Time |  |
| `K2Node_CustomEvent_153` | FadeOut<br/>Custom Event |  |
| `K2Node_MacroInstance_869` | Interp by Time | InterpTime=0.2 |
| `K2Node_VariableGet_2358` | Get Opacity |  |
| `K2Node_VariableSet_1040` | Set Opacity |  |
| `K2Node_MacroInstance_870` | Update Content Opacity |  |
| `K2Node_CallFunction_4909` | Unpause Timer by Handle |  |
| `K2Node_CallFunction_918` | Pause Timer by Handle |  |
| `K2Node_MacroInstance_1083` | Update Visibility |  |
| `K2Node_MacroInstance_626` | Update Visibility | bVisible=true |
| `K2Node_MacroInstance_332` | Do Once |  |
| `K2Node_Knot_193` | 경유 노드 |  |
| `K2Node_Knot_194` | 경유 노드 |  |
| `K2Node_CallDelegate_0` | Call On Progress Bar Completed |  |
| `K2Node_CallFunction_5695` | Get Owner HUD |  |
| `K2Node_Composite_92` | SetData<br/>Collapsed Graph |  |
| `K2Node_VariableGet_2740` | Get CurrentValue |  |
| `K2Node_VariableGet_1053` | Get Opacity |  |
| `K2Node_VariableSet_495` | Set Opacity |  |
| `K2Node_MacroInstance_217` | Update Content Opacity |  |
| `K2Node_CallFunction_2509` | float / float | A=1; B=0.15 |
| `K2Node_CallFunction_1732` | FInterp to Constant | Target=1 |
| `K2Node_CallFunction_6333` | Get World Delta Seconds |  |
| `K2Node_CallFunction_2829` | Update All Infos<br/>타깃은 WB Progress Bar |  |
| `K2Node_VariableGet_850` | Get FillColor |  |
| `K2Node_VariableGet_863` | Get EmptyColor |  |
| `K2Node_MacroInstance_674` | Update Visibility | bVisible=true |
| `K2Node_Event_3043` | RemoveProgressBar 이벤트<br/>I Crosshair Manager에서 |  |
| `K2Node_MacroInstance_3563` | Update Visibility |  |
| `K2Node_MacroInstance_1082` | Fade Anim |  |
| `K2Node_CallFunction_7795` | Delay | Duration=0.2 |
| `K2Node_MacroInstance_4304` | Fade Anim |  |
| `K2Node_Composite_99` | FadeOutContent<br/>Collapsed Graph |  |
| `EdGraphNode_Comment_97` | Event Remove Progress Bar |  |
| `K2Node_MacroInstance_1081` | Do Once |  |
| `K2Node_MacroInstance_842` | Fade Anim |  |
| `K2Node_CallFunction_7124` | Delay | Duration=0.2 |
| `K2Node_MacroInstance_324` | Update Visibility |  |
| `K2Node_VariableGet_1145` | Get CurrentValue |  |
| `K2Node_CallFunction_7782` | float &gt;= float | B=1 |
| `K2Node_IfThenElse_43` | Branch |  |
| `K2Node_VariableSet_1375` | Set CurrentValue | CurrentValue=0.0; Output_Get=0.0 |
| `K2Node_Knot_266` | 경유 노드 |  |
| `K2Node_Composite_132` | HandleSizeSteps<br/>Collapsed Graph |  |
| `EdGraphNode_Comment_105` | Timed Progress |  |
| `EdGraphNode_Comment_106` | Updating Progress |  |
| `K2Node_Knot_276` | 경유 노드 |  |
| `EdGraphNode_Comment_107` | Event Update Progress Bar |  |
| `K2Node_VariableSet_504` | Set LiveProgress | LiveProgress=0.0; Output_Get=0.0 |

</details>

### GetOwnerHUD

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["GetOwnerHUD"]
  n1["반환 노드"]
  n2["Cast To HUD_CompShooter"]
  n0 -->|"then"| n2
  n2 -->|"then"| n1
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_3575` | GetOwnerHUD |  |
| `K2Node_FunctionResult_4` | 반환 노드 |  |
| `K2Node_CallFunction_5636` | Get Owning Player<br/>타깃은 위젯 |  |
| `K2Node_CallFunction_5637` | Get HUD<br/>타깃은 플레이어 컨트롤러 |  |
| `K2Node_DynamicCast_0` | Cast To HUD_CompShooter |  |

</details>

### UpdateAllInfos

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["UpdateAllInfos"]
  n1["Set Scalar Parameter Value<br/>타깃은 머티리얼 인스턴스 다이내믹"]
  n2["Set Vector Parameter Value<br/>타깃은 머티리얼 인스턴스 다이내믹"]
  n3["Set Vector Parameter Value<br/>타깃은 머티리얼 인스턴스 다이내믹"]
  n4["Is Valid"]
  n5["Update Shine 02<br/>타깃은 WB Progress Bar"]
  n6["Update Shine 01<br/>타깃은 WB Progress Bar"]
  n7["Get Dynamic Material<br/>타깃은 이미지"]
  n8["Update Shine Colors<br/>타깃은 WB Progress Bar"]
  n9["Set Scalar Parameter Value<br/>타깃은 머티리얼 인스턴스 다이내믹"]
  n10["Set Scalar Parameter Value<br/>타깃은 머티리얼 인스턴스 다이내믹"]
  n0 -->|"then"| n6
  n2 -->|"then"| n3
  n3 -->|"then"| n1
  n4 -->|"Is Valid"| n2
  n5 -->|"then"| n8
  n6 -->|"then"| n5
  n7 -->|"then"| n4
  n8 -->|"then"| n7
  n9 -->|"then"| n10
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_1774` | UpdateAllInfos |  |
| `K2Node_CallFunction_4` | Set Scalar Parameter Value<br/>타깃은 머티리얼 인스턴스 다이내믹 | ParameterName=Percent |
| `K2Node_CallFunction_3` | Set Vector Parameter Value<br/>타깃은 머티리얼 인스턴스 다이내믹 | ParameterName=EmptyColor |
| `K2Node_CallFunction_2` | Set Vector Parameter Value<br/>타깃은 머티리얼 인스턴스 다이내믹 | ParameterName=FillColor |
| `K2Node_MacroInstance_0` | Is Valid |  |
| `K2Node_CallFunction_1139` | Update Shine 02<br/>타깃은 WB Progress Bar |  |
| `K2Node_CallFunction_1280` | Update Shine 01<br/>타깃은 WB Progress Bar |  |
| `K2Node_VariableGet_728` | Get CircleImg |  |
| `K2Node_CallFunction_3173` | Get Dynamic Material<br/>타깃은 이미지 |  |
| `K2Node_Knot_171` | 경유 노드 |  |
| `K2Node_Knot_172` | 경유 노드 |  |
| `K2Node_Knot_173` | 경유 노드 |  |
| `K2Node_Knot_174` | 경유 노드 |  |
| `K2Node_Knot_175` | 경유 노드 |  |
| `K2Node_Knot_176` | 경유 노드 |  |
| `K2Node_Knot_177` | 경유 노드 |  |
| `K2Node_Knot_178` | 경유 노드 |  |
| `K2Node_CallFunction_3598` | Update Shine Colors<br/>타깃은 WB Progress Bar |  |
| `K2Node_Knot_179` | 경유 노드 |  |
| `K2Node_CallFunction_5` | Set Scalar Parameter Value<br/>타깃은 머티리얼 인스턴스 다이내믹 | ParameterName=Size; Value=0.0 |
| `K2Node_CallFunction_1` | Set Scalar Parameter Value<br/>타깃은 머티리얼 인스턴스 다이내믹 | ParameterName=Steps; Value=0.0 |

</details>

### UpdateShine01

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["UpdateShine01"]
  n1["Set Render Transform Angle<br/>타깃은 위젯"]
  n0 -->|"then"| n1
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_846` | UpdateShine01 |  |
| `K2Node_CallFunction_620` | Map Range Clamped | InRangeA=0.1; InRangeB=1; OutRangeA=-180; OutRangeB=150 |
| `K2Node_VariableGet_533` | Get B_Shine1_Rotation |  |
| `K2Node_CallFunction_1020` | Set Render Transform Angle<br/>타깃은 위젯 |  |

</details>

### UpdateShine02

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["UpdateShine02"]
  n1["Set Render Transform Angle<br/>타깃은 위젯"]
  n0 -->|"then"| n1
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_778` | UpdateShine02 |  |
| `K2Node_CallFunction_459` | Map Range Clamped | InRangeA=0.0; InRangeB=1; OutRangeA=-180; OutRangeB=180 |
| `K2Node_VariableGet_517` | Get B_Shine2_Rotation |  |
| `K2Node_CallFunction_978` | Set Render Transform Angle<br/>타깃은 위젯 |  |

</details>

### UpdateShineColors

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["UpdateShineColors"]
  n1["Set Content Color and Opacity<br/>타깃은 보더"]
  n2["반환 노드"]
  n0 -->|"then"| n1
  n1 -->|"then"| n2
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_2159` | UpdateShineColors |  |
| `K2Node_VariableGet_755` | Get B_Shine1_Rotation |  |
| `K2Node_VariableGet_756` | Get B_Shine2_Rotation |  |
| `K2Node_CallFunction_3459` | Set Content Color and Opacity<br/>타깃은 보더 |  |
| `K2Node_FunctionResult_3` | 반환 노드 |  |
| `K2Node_VariableGet_1206` | Get b_StaticCircle |  |

</details>

## 조회 범위의 한계

ObjectIterator로 발견했으나 MCP helper가 반환하지 않은 그래프입니다. delegate signature·컴파일된 중간 그래프·macro·애니메이션 내부 그래프가 섞일 수 있습니다.

- `ActivateCrosshair`: Graph 'ActivateCrosshair' not found in Blueprint.
- `CrosshairReaction`: Graph 'CrosshairReaction' not found in Blueprint.
- `FadeAnim`: Graph 'FadeAnim' not found in Blueprint.
- `FadeOut`: Graph 'FadeOut' not found in Blueprint.
- `HitIndicator`: Graph 'HitIndicator' not found in Blueprint.
- `InterpByTime`: Graph 'InterpByTime' not found in Blueprint.
- `Recoil`: Graph 'Recoil' not found in Blueprint.
- `RemoveCrosshair`: Graph 'RemoveCrosshair' not found in Blueprint.
- `RemoveFadeOut`: Graph 'RemoveFadeOut' not found in Blueprint.
- `RemoveProgressBar`: Graph 'RemoveProgressBar' not found in Blueprint.
- `UpdateContentOpacity`: Graph 'UpdateContentOpacity' not found in Blueprint.
- `UpdateProgressBar`: Graph 'UpdateProgressBar' not found in Blueprint.
- `UpdateTimedProgress`: Graph 'UpdateTimedProgress' not found in Blueprint.
- `UpdateVisibility`: Graph 'UpdateVisibility' not found in Blueprint.
- `ZoomIn`: Graph 'ZoomIn' not found in Blueprint.
- `ZoomOut`: Graph 'ZoomOut' not found in Blueprint.

[원본 MCP 스냅샷](../../../../../../Evidence/blueprints-351.json)
