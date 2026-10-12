# WB_HB_Base

에셋: `/Game/CompShooterHUD/UI/Content/HealthBar/WB_HB_Base`

부모: `/Script/CoreUObject.Class'/Script/UMG.UserWidget'`

수집일: 2026-10-12. MCP에서 읽은 그래프·핀 연결의 자동 전사입니다. 상속된 그래프는 부모 문서를 함께 확인합니다.

정적 루트 참조 도달: 있음. 실행 여부 또는 삭제 가능 여부의 확정이 아닙니다.

컴포넌트 목록은 이 BP가 직접 정의한 SCS만 포함합니다. 부모·네이티브 생성 컴포넌트, 인스턴스 설정은 별도입니다.

## 변수와 기본값

| 이름 | 타입 | CDO 기본값 |
|---|---|---|
| `OnHealthBarFadedOut` | `mcdelegate` | &lt;Delegate 'DelegateBase' &lt;Unbound&gt;&gt; |
| `WB_ValueBar` | `WB_HealthBar_C` | None |
| `WB_HealthAnim` | `WB_HealthAnim_C` | None |
| `WB_DamageAnim` | `WB_DamageAnim_C` | None |
| `DamageColor` | `LinearColor` | &lt;Struct 'LinearColor' {r: 0.000000, g: 0.000000, b: 0.000000, a: 0.000000}&gt; |

## 그래프 목록

| 그래프 | 전체 노드 수 |
|---|---|
| EventGraph | 79 |
| GetCurrentHealth | 6 |
| GetCurrentMaxHealth | 6 |
| UpdateColors | 6 |
| UpdateMaxHealthSensitive | 7 |

## 그래프별 실행 연결

실선은 출력 exec 핀의 실제 연결입니다. 여러 분기는 출력 핀 이름을 표시합니다. 함수 호출·delegate·타이머의 다음 그래프 실행은 이 그림에서 자동으로 이어 붙이지 않습니다.

순수 계산 노드 및 데이터 핀은 아래 목록과 원본 JSON을 확인합니다. 연결되지 않은 이벤트도 표시되므로, 노드가 있다는 이유만으로 실행된다고 판단하면 안 됩니다.

### EventGraph

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["RemoveHealthBar<br/>Custom Event"]
  n1["Call On Health Bar Faded Out"]
  n2["On Animation Finished 이벤트"]
  n3["Branch"]
  n4["AddOrUpdateHealthBar<br/>Custom Event"]
  n5["Is Valid"]
  n6["Update Health<br/>타깃은 WB Health Bar"]
  n7["Is Valid"]
  n8["Update Max Health<br/>타깃은 WB Health Bar"]
  n9["UpdateHealth<br/>Custom Event"]
  n10["UpdateMaxHealth<br/>Custom Event"]
  n11["Fade Health Bar Anim"]
  n12["Update OV Health Bar Visibility"]
  n13["Fade Health Bar Anim"]
  n14["Update Colors<br/>타깃은 WB HB Base"]
  n15["HandleColors<br/>Custom Event"]
  n16["Handle Colors<br/>타깃은 WB HB Base"]
  n17["HandleChilds<br/>Collapsed Graph"]
  n18["Delay"]
  n19["Play Health Fade Anim"]
  n20["Play Health Fade Anim"]
  n21["AddIcon<br/>Custom Event"]
  n22["RemoveIcon<br/>Custom Event"]
  n23["Is Valid"]
  n24["Add Value Icon<br/>타깃은 WB Health Bar"]
  n25["Is Valid"]
  n26["Remove Value Icon<br/>타깃은 WB Health Bar"]
  n27["Play Health Fade Anim"]
  n28["Add Icon<br/>타깃은 WB HB Base"]
  n29["Update Max Health Sensitive<br/>타깃은 WB HB Base"]
  n30["RemoveHealth<br/>Custom Event"]
  n31["Play Damage Fade Anim"]
  n32["UpdateDamageAnimColor<br/>Collapsed Graph"]
  n33["Is Valid"]
  n34["Update Health<br/>타깃은 WB Health Bar"]
  n0 -->|"then"| n18
  n2 -->|"then"| n3
  n3 -->|"then"| n1
  n4 -->|"then"| n17
  n5 -->|"Is Valid"| n6
  n7 -->|"Is Valid"| n8
  n9 -->|"then"| n19
  n10 -->|"then"| n20
  n12 -->|"then"| n28
  n13 -->|"then"| n12
  n15 -->|"then"| n14
  n16 -->|"then"| n27
  n17 -->|"then"| n16
  n18 -->|"then"| n11
  n19 -->|"then"| n5
  n20 -->|"then"| n7
  n21 -->|"then"| n23
  n22 -->|"then"| n25
  n23 -->|"Is Valid"| n24
  n25 -->|"Is Valid"| n26
  n27 -->|"then"| n13
  n28 -->|"then"| n29
  n30 -->|"then"| n32
  n31 -->|"then"| n33
  n32 -->|"then"| n31
  n33 -->|"Is Valid"| n34
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_CustomEvent_0` | RemoveHealthBar<br/>Custom Event |  |
| `K2Node_CallDelegate_44` | Call On Health Bar Faded Out |  |
| `EdGraphNode_Comment_9` | Event Remove Health Bar |  |
| `K2Node_Event_3` | On Animation Finished 이벤트 |  |
| `K2Node_CallFunction_15` | Equal (Object) |  |
| `K2Node_IfThenElse_0` | Branch |  |
| `K2Node_CustomEvent_14` | AddOrUpdateHealthBar<br/>Custom Event |  |
| `EdGraphNode_Comment_64` | Event Add Or Update Health Bar |  |
| `K2Node_VariableGet_1189` | Get WB_ValueBar |  |
| `K2Node_MacroInstance_1570` | Is Valid |  |
| `K2Node_CallFunction_3856` | Update Health<br/>타깃은 WB Health Bar | bIsAddingHealth=true |
| `K2Node_VariableGet_478` | Get WB_ValueBar |  |
| `K2Node_MacroInstance_44` | Is Valid |  |
| `K2Node_CallFunction_359` | Update Max Health<br/>타깃은 WB Health Bar |  |
| `K2Node_CustomEvent_103` | UpdateHealth<br/>Custom Event |  |
| `K2Node_CustomEvent_112` | UpdateMaxHealth<br/>Custom Event |  |
| `K2Node_Knot_285` | 경유 노드 |  |
| `K2Node_Knot_1850` | 경유 노드 |  |
| `K2Node_Knot_287` | 경유 노드 |  |
| `K2Node_Knot_288` | 경유 노드 |  |
| `K2Node_Knot_1851` | 경유 노드 |  |
| `K2Node_Knot_290` | 경유 노드 |  |
| `EdGraphNode_Comment_22` | Event Add Health |  |
| `EdGraphNode_Comment_92` | Event Update Max Health |  |
| `K2Node_VariableGet_324` | Get FadeInHealthBar |  |
| `K2Node_MacroInstance_1509` | Fade Health Bar Anim |  |
| `K2Node_MacroInstance_1545` | Update OV Health Bar Visibility | bVisible=true |
| `K2Node_MacroInstance_1592` | Fade Health Bar Anim | bVisible=true |
| `K2Node_CallFunction_1340` | Update Colors<br/>타깃은 WB HB Base |  |
| `K2Node_CallFunction_1945` | To Color (LinearColor) | InUseSRGB=true |
| `K2Node_CallFunction_1946` | To LinearColor (Color) |  |
| `K2Node_CallFunction_1893` | To Color (LinearColor) | InUseSRGB=true |
| `K2Node_CallFunction_4128` | To LinearColor (Color) |  |
| `K2Node_CallFunction_1988` | To Color (LinearColor) | InUseSRGB=true |
| `K2Node_CallFunction_1989` | To LinearColor (Color) |  |
| `K2Node_CallFunction_1990` | To Color (LinearColor) | InUseSRGB=true |
| `K2Node_CallFunction_1991` | To LinearColor (Color) |  |
| `K2Node_CustomEvent_516` | HandleColors<br/>Custom Event |  |
| `EdGraphNode_Comment_96` | Event Handle Colors |  |
| `K2Node_CallFunction_2384` | Handle Colors<br/>타깃은 WB HB Base |  |
| `K2Node_Composite_63` | HandleChilds<br/>Collapsed Graph |  |
| `K2Node_CallFunction_97` | Delay | Duration=0.01 |
| `K2Node_MacroInstance_221` | Play Health Fade Anim |  |
| `K2Node_MacroInstance_845` | Play Health Fade Anim |  |
| `K2Node_CustomEvent_102` | AddIcon<br/>Custom Event |  |
| `K2Node_CustomEvent_110` | RemoveIcon<br/>Custom Event |  |
| `K2Node_VariableGet_295` | Get WB_ValueBar |  |
| `K2Node_MacroInstance_373` | Is Valid |  |
| `K2Node_CallFunction_1078` | Add Value Icon<br/>타깃은 WB Health Bar |  |
| `K2Node_Knot_300` | 경유 노드 |  |
| `K2Node_Knot_301` | 경유 노드 |  |
| `K2Node_Knot_367` | 경유 노드 |  |
| `K2Node_VariableGet_323` | Get WB_ValueBar |  |
| `K2Node_MacroInstance_301` | Is Valid |  |
| `K2Node_Knot_302` | 경유 노드 |  |
| `K2Node_CallFunction_1088` | Remove Value Icon<br/>타깃은 WB Health Bar |  |
| `EdGraphNode_Comment_85` | Event Add Icon |  |
| `EdGraphNode_Comment_86` | Event Remove Icon |  |
| `K2Node_MacroInstance_1040` | Play Health Fade Anim |  |
| `K2Node_CallFunction_1443` | Add Icon<br/>타깃은 WB HB Base |  |
| `K2Node_Knot_303` | 경유 노드 |  |
| `K2Node_Knot_304` | 경유 노드 |  |
| `K2Node_CallFunction_2881` | Update Max Health Sensitive<br/>타깃은 WB HB Base |  |
| `K2Node_Knot_1325` | 경유 노드 |  |
| `K2Node_Knot_1326` | 경유 노드 |  |
| `EdGraphNode_Comment_91` | Event Remove Health |  |
| `K2Node_CustomEvent_23` | RemoveHealth<br/>Custom Event |  |
| `K2Node_MacroInstance_2155` | Play Damage Fade Anim |  |
| `K2Node_Composite_335` | UpdateDamageAnimColor<br/>Collapsed Graph |  |
| `K2Node_VariableGet_56` | Get WB_ValueBar |  |
| `K2Node_MacroInstance_79` | Is Valid |  |
| `K2Node_CallFunction_352` | Update Health<br/>타깃은 WB Health Bar | bIsAddingHealth=false |
| `K2Node_Knot_286` | 경유 노드 |  |
| `K2Node_Knot_289` | 경유 노드 |  |
| `K2Node_VariableGet_1223` | Get HealthValue |  |
| `K2Node_CallFunction_3965` | int - int |  |
| `K2Node_Knot_855` | 경유 노드 |  |
| `K2Node_Knot_2090` | 경유 노드 |  |
| `K2Node_VariableGet_1022` | Get DamageColor |  |

</details>

### GetCurrentHealth

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["GetCurrentHealth"]
  n1["반환 노드"]
  n2["Is Valid"]
  n3["반환 노드"]
  n0 -->|"then"| n2
  n2 -->|"Is Valid"| n1
  n2 -->|"Is Not Valid"| n3
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_663` | GetCurrentHealth |  |
| `K2Node_FunctionResult_1` | 반환 노드 |  |
| `K2Node_VariableGet_478` | Get WB_ValueBar |  |
| `K2Node_MacroInstance_44` | Is Valid |  |
| `K2Node_VariableGet_643` | Get HealthValue |  |
| `K2Node_FunctionResult_0` | 반환 노드 | CurrentHealth=0 |

</details>

### GetCurrentMaxHealth

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["GetCurrentMaxHealth"]
  n1["Is Valid"]
  n2["반환 노드"]
  n3["반환 노드"]
  n0 -->|"then"| n1
  n1 -->|"Is Valid"| n3
  n1 -->|"Is Not Valid"| n2
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_817` | GetCurrentMaxHealth |  |
| `K2Node_MacroInstance_44` | Is Valid |  |
| `K2Node_VariableGet_478` | Get WB_ValueBar |  |
| `K2Node_FunctionResult_2` | 반환 노드 | CurrentMaxHealth=0 |
| `K2Node_FunctionResult_0` | 반환 노드 |  |
| `K2Node_VariableGet_730` | Get MaxHealthValue |  |

</details>

### UpdateColors

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["UpdateColors"]
  n1["Update Color<br/>타깃은 WB Health Anim"]
  n2["Update Colors<br/>타깃은 WB Health Bar"]
  n3["Set DamageColor"]
  n0 -->|"then"| n3
  n1 -->|"then"| n2
  n3 -->|"then"| n1
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_827` | UpdateColors |  |
| `K2Node_VariableGet_303` | Get WB_ValueBar |  |
| `K2Node_VariableGet_384` | Get WB_HealthAnim |  |
| `K2Node_CallFunction_1392` | Update Color<br/>타깃은 WB Health Anim |  |
| `K2Node_CallFunction_1393` | Update Colors<br/>타깃은 WB Health Bar |  |
| `K2Node_VariableSet_716` | Set DamageColor |  |

</details>

### UpdateMaxHealthSensitive

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["UpdateMaxHealthSensitive"]
  n1["Is Valid"]
  n2["Set Max Health Sensitive<br/>타깃은 WB Health Bar"]
  n0 -->|"then"| n1
  n1 -->|"Is Valid"| n2
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_890` | UpdateMaxHealthSensitive |  |
| `K2Node_VariableGet_478` | Get WB_ValueBar |  |
| `K2Node_MacroInstance_44` | Is Valid |  |
| `K2Node_CallFunction_2785` | Set Max Health Sensitive<br/>타깃은 WB Health Bar |  |
| `K2Node_Knot_1322` | 경유 노드 |  |
| `K2Node_Knot_1323` | 경유 노드 |  |
| `K2Node_Knot_1324` | 경유 노드 |  |

</details>

## 조회 범위의 한계

ObjectIterator로 발견했으나 MCP helper가 반환하지 않은 그래프입니다. delegate signature·컴파일된 중간 그래프·macro·애니메이션 내부 그래프가 섞일 수 있습니다.

- `AddDamageAnimToOverlay`: Graph 'AddDamageAnimToOverlay' not found in Blueprint.
- `AddHealthAnimToOverlay`: Graph 'AddHealthAnimToOverlay' not found in Blueprint.
- `AddHealthBarToOverlay`: Graph 'AddHealthBarToOverlay' not found in Blueprint.
- `AddIcon`: Graph 'AddIcon' not found in Blueprint.
- `AddOrUpdateHealthBar`: Graph 'AddOrUpdateHealthBar' not found in Blueprint.
- `FadeHealthBarAnim`: Graph 'FadeHealthBarAnim' not found in Blueprint.
- `HandleColors`: Graph 'HandleColors' not found in Blueprint.
- `OV_DamageAnim_HasNoChildren?`: Graph 'OV_DamageAnim_HasNoChildren?' not found in Blueprint.
- `OV_HealthAnim_HasNoChildren?`: Graph 'OV_HealthAnim_HasNoChildren?' not found in Blueprint.
- `OV_HealthBar_HasNoChildren?`: Graph 'OV_HealthBar_HasNoChildren?' not found in Blueprint.
- `OnAnimationFinished`: Graph 'OnAnimationFinished' not found in Blueprint.
- `OnHealthBarFadedOut`: Graph 'OnHealthBarFadedOut' not found in Blueprint.
- `PlayDamageFadeAnim`: Graph 'PlayDamageFadeAnim' not found in Blueprint.
- `PlayHealthFadeAnim`: Graph 'PlayHealthFadeAnim' not found in Blueprint.
- `RemoveHealth`: Graph 'RemoveHealth' not found in Blueprint.
- `RemoveHealthBar`: Graph 'RemoveHealthBar' not found in Blueprint.
- `RemoveIcon`: Graph 'RemoveIcon' not found in Blueprint.
- `UpdateHealth`: Graph 'UpdateHealth' not found in Blueprint.
- `UpdateMaxHealth`: Graph 'UpdateMaxHealth' not found in Blueprint.
- `UpdateOV_HealthBar_Visibility`: Graph 'UpdateOV_HealthBar_Visibility' not found in Blueprint.

[원본 MCP 스냅샷](../../../../../Evidence/blueprints-316.json)
