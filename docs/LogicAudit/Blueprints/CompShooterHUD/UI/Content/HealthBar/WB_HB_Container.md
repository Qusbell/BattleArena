# WB_HB_Container

에셋: `/Game/CompShooterHUD/UI/Content/HealthBar/WB_HB_Container`

부모: `/Script/CoreUObject.Class'/Script/UMG.UserWidget'`

수집일: 2026-10-12. MCP에서 읽은 그래프·핀 연결의 자동 전사입니다. 상속된 그래프는 부모 문서를 함께 확인합니다.

정적 루트 참조 도달: 있음. 실행 여부 또는 삭제 가능 여부의 확정이 아닙니다.

컴포넌트 목록은 이 BP가 직접 정의한 SCS만 포함합니다. 부모·네이티브 생성 컴포넌트, 인스턴스 설정은 별도입니다.

## 변수와 기본값

| 이름 | 타입 | CDO 기본값 |
|---|---|---|
| `HB_Base` | `WB_HB_Base_C` | None |
| `MaxHealth` | `int` | 0 |
| `Health` | `int` | 0 |
| `Damage` | `int` | 0 |

## 그래프 목록

| 그래프 | 전체 노드 수 |
|---|---|
| EventGraph | 57 |
| GetCurrentHealth | 6 |
| GetMaxHealth | 6 |
| PopulateHealthBar | 6 |

## 그래프별 실행 연결

실선은 출력 exec 핀의 실제 연결입니다. 여러 분기는 출력 핀 이름을 표시합니다. 함수 호출·delegate·타이머의 다음 그래프 실행은 이 그림에서 자동으로 이어 붙이지 않습니다.

순수 계산 노드 및 데이터 핀은 아래 목록과 원본 JSON을 확인합니다. 연결되지 않은 이벤트도 표시되므로, 노드가 있다는 이유만으로 실행된다고 판단하면 안 됩니다.

### EventGraph

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["AddHealthBar<br/>Custom Event"]
  n1["RemoveHealthBar<br/>Custom Event"]
  n2["Clear Overlay Children"]
  n3["Clear Overlay Children"]
  n4["OnHealthBarFadedOut<br/>Custom Event"]
  n5["Is Valid"]
  n6["Bind Event to On Health Bar Faded Out"]
  n7["Unbind Event from On Health Bar Faded Out"]
  n8["Add or Update Health Bar<br/>타깃은 WB HB Base"]
  n9["Populate Health Bar<br/>타깃은 WB HB Container"]
  n10["Is Valid"]
  n11["Is Valid"]
  n12["Is Valid"]
  n13["Update Max Health<br/>타깃은 WB HB Base"]
  n14["UpdateMaxHealth<br/>Custom Event"]
  n15["UpdateHealth<br/>Custom Event"]
  n16["Remove Health Bar<br/>타깃은 WB HB Base"]
  n17["Set MaxHealth"]
  n18["Set Health"]
  n19["Update Max Health<br/>타깃은 WB HB Container"]
  n20["Update Health<br/>타깃은 WB HB Container"]
  n21["RemoveHealth<br/>Custom Event"]
  n22["Set Damage"]
  n23["Is Valid"]
  n24["Remove Health<br/>타깃은 WB HB Base"]
  n25["Update Health<br/>타깃은 WB HB Base"]
  n26["Branch"]
  n27["경유 노드"]
  n28["경유 노드"]
  n29["경유 노드"]
  n0 -->|"then"| n26
  n1 -->|"then"| n5
  n2 -->|"then"| n9
  n3 -->|"then"| n7
  n4 -->|"then"| n3
  n5 -->|"Is Valid"| n16
  n8 -->|"then"| n19
  n9 -->|"then"| n27
  n10 -->|"Is Valid"| n8
  n11 -->|"Is Valid"| n25
  n12 -->|"Is Valid"| n13
  n14 -->|"then"| n17
  n15 -->|"then"| n18
  n16 -->|"then"| n6
  n17 -->|"then"| n12
  n18 -->|"then"| n11
  n19 -->|"then"| n20
  n21 -->|"then"| n22
  n22 -->|"then"| n23
  n23 -->|"Is Valid"| n24
  n26 -->|"then"| n2
  n26 -->|"else"| n28
  n27 -->|"OutputPin"| n10
  n28 -->|"OutputPin"| n29
  n29 -->|"OutputPin"| n27
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_CustomEvent_0` | AddHealthBar<br/>Custom Event |  |
| `K2Node_CustomEvent_2` | RemoveHealthBar<br/>Custom Event |  |
| `K2Node_MacroInstance_417` | Clear Overlay Children |  |
| `K2Node_MacroInstance_438` | Clear Overlay Children |  |
| `EdGraphNode_Comment_48` | Event Remove HealthBar |  |
| `K2Node_CustomEvent_42` | OnHealthBarFadedOut<br/>Custom Event |  |
| `K2Node_VariableGet_43` | Get HB_Base |  |
| `K2Node_MacroInstance_76` | Is Valid |  |
| `K2Node_AddDelegate_0` | Bind Event to On Health Bar Faded Out |  |
| `K2Node_Knot_221` | 경유 노드 |  |
| `K2Node_RemoveDelegate_0` | Unbind Event from On Health Bar Faded Out |  |
| `K2Node_Knot_222` | 경유 노드 |  |
| `K2Node_Knot_223` | 경유 노드 |  |
| `K2Node_CallFunction_240` | Add or Update Health Bar<br/>타깃은 WB HB Base |  |
| `K2Node_CallFunction_268` | Populate Health Bar<br/>타깃은 WB HB Container |  |
| `K2Node_MacroInstance_180` | Is Valid |  |
| `EdGraphNode_Comment_95` | Event Add Health Bar |  |
| `K2Node_VariableGet_148` | Get HB_Base |  |
| `K2Node_MacroInstance_146` | Is Valid |  |
| `K2Node_VariableGet_460` | Get HB_Base |  |
| `K2Node_MacroInstance_135` | Is Valid |  |
| `K2Node_CallFunction_698` | Update Max Health<br/>타깃은 WB HB Base |  |
| `K2Node_CustomEvent_213` | UpdateMaxHealth<br/>Custom Event |  |
| `K2Node_CustomEvent_221` | UpdateHealth<br/>Custom Event |  |
| `K2Node_Knot_294` | 경유 노드 |  |
| `K2Node_Knot_295` | 경유 노드 |  |
| `K2Node_Knot_296` | 경유 노드 |  |
| `K2Node_Knot_243` | 경유 노드 |  |
| `K2Node_Knot_298` | 경유 노드 |  |
| `K2Node_Knot_299` | 경유 노드 |  |
| `EdGraphNode_Comment_61` | Event Add Health |  |
| `EdGraphNode_Comment_98` | Event Update Max Health |  |
| `K2Node_CallFunction_278` | Remove Health Bar<br/>타깃은 WB HB Base |  |
| `K2Node_Knot_419` | 경유 노드 |  |
| `K2Node_VariableSet_103` | Set MaxHealth |  |
| `K2Node_VariableSet_107` | Set Health |  |
| `K2Node_CallFunction_97` | Update Max Health<br/>타깃은 WB HB Container |  |
| `K2Node_CallFunction_98` | Update Health<br/>타깃은 WB HB Container |  |
| `K2Node_VariableGet_245` | Get Health |  |
| `K2Node_VariableGet_246` | Get MaxHealth |  |
| `K2Node_CustomEvent_53` | RemoveHealth<br/>Custom Event |  |
| `EdGraphNode_Comment_97` | Event Remove Health |  |
| `K2Node_VariableSet_359` | Set Damage |  |
| `K2Node_VariableGet_185` | Get HB_Base |  |
| `K2Node_MacroInstance_181` | Is Valid |  |
| `K2Node_Knot_297` | 경유 노드 |  |
| `K2Node_CallFunction_497` | Remove Health<br/>타깃은 WB HB Base |  |
| `K2Node_Knot_646` | 경유 노드 |  |
| `K2Node_Knot_647` | 경유 노드 |  |
| `K2Node_CallFunction_707` | Update Health<br/>타깃은 WB HB Base |  |
| `K2Node_IfThenElse_9` | Branch |  |
| `K2Node_Knot_234` | 경유 노드 |  |
| `K2Node_Knot_235` | 경유 노드 |  |
| `K2Node_Knot_236` | 경유 노드 |  |
| `K2Node_VariableGet_93` | Get HB_Base |  |
| `K2Node_Knot_237` | 경유 노드 |  |
| `K2Node_Composite_52` | OV_Has_No_Children?<br/>Collapsed Graph |  |

</details>

### GetCurrentHealth

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["GetCurrentHealth"]
  n1["반환 노드"]
  n2["Is Valid"]
  n3["Get Current Health<br/>타깃은 WB HB Base"]
  n4["반환 노드"]
  n0 -->|"then"| n2
  n2 -->|"Is Valid"| n3
  n2 -->|"Is Not Valid"| n4
  n3 -->|"then"| n1
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_263` | GetCurrentHealth |  |
| `K2Node_FunctionResult_1` | 반환 노드 |  |
| `K2Node_VariableGet_185` | Get HB_Base |  |
| `K2Node_MacroInstance_181` | Is Valid |  |
| `K2Node_CallFunction_589` | Get Current Health<br/>타깃은 WB HB Base |  |
| `K2Node_FunctionResult_0` | 반환 노드 | CurrentHealth=0 |

</details>

### GetMaxHealth

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["GetMaxHealth"]
  n1["반환 노드"]
  n2["Is Valid"]
  n3["반환 노드"]
  n4["Get Current Max Health<br/>타깃은 WB HB Base"]
  n0 -->|"then"| n2
  n2 -->|"Is Valid"| n4
  n2 -->|"Is Not Valid"| n3
  n4 -->|"then"| n1
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_263` | GetMaxHealth |  |
| `K2Node_FunctionResult_1` | 반환 노드 |  |
| `K2Node_VariableGet_185` | Get HB_Base |  |
| `K2Node_MacroInstance_181` | Is Valid |  |
| `K2Node_FunctionResult_0` | 반환 노드 | CurrentMaxHealth=0 |
| `K2Node_CallFunction_700` | Get Current Max Health<br/>타깃은 WB HB Base |  |

</details>

### PopulateHealthBar

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["PopulateHealthBar"]
  n1["Create WB HB Base Widget"]
  n2["Add Child to Overlay"]
  n3["Set HB_Base"]
  n4["반환 노드"]
  n0 -->|"then"| n1
  n1 -->|"then"| n3
  n2 -->|"then"| n4
  n3 -->|"then"| n2
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_193` | PopulateHealthBar |  |
| `K2Node_CallFunction_0` | Get Owning Player<br/>타깃은 위젯 |  |
| `K2Node_CreateWidget_0` | Create WB HB Base Widget | Class=/Game/CompShooterHUD/UI/Content/HealthBar/WB_HB_Base.WB_HB_Base_C |
| `K2Node_MacroInstance_348` | Add Child to Overlay |  |
| `K2Node_VariableSet_30` | Set HB_Base |  |
| `K2Node_FunctionResult_1` | 반환 노드 |  |

</details>

## 조회 범위의 한계

ObjectIterator로 발견했으나 MCP helper가 반환하지 않은 그래프입니다. delegate signature·컴파일된 중간 그래프·macro·애니메이션 내부 그래프가 섞일 수 있습니다.

- `AddChildToOverlay`: Graph 'AddChildToOverlay' not found in Blueprint.
- `AddHealthBar`: Graph 'AddHealthBar' not found in Blueprint.
- `ClearOverlayChildren`: Graph 'ClearOverlayChildren' not found in Blueprint.
- `OnHealthBarFadedOut`: Graph 'OnHealthBarFadedOut' not found in Blueprint.
- `RemoveHealth`: Graph 'RemoveHealth' not found in Blueprint.
- `RemoveHealthBar`: Graph 'RemoveHealthBar' not found in Blueprint.
- `UpdateHealth`: Graph 'UpdateHealth' not found in Blueprint.
- `UpdateMaxHealth`: Graph 'UpdateMaxHealth' not found in Blueprint.

[원본 MCP 스냅샷](../../../../../Evidence/blueprints-316.json)
