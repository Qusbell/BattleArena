# WB_AB_Container

에셋: `/Game/CompShooterHUD/UI/Content/ArmorBar/WB_AB_Container`

부모: `/Script/CoreUObject.Class'/Script/UMG.UserWidget'`

수집일: 2026-10-12. MCP에서 읽은 그래프·핀 연결의 자동 전사입니다. 상속된 그래프는 부모 문서를 함께 확인합니다.

정적 루트 참조 도달: 있음. 실행 여부 또는 삭제 가능 여부의 확정이 아닙니다.

컴포넌트 목록은 이 BP가 직접 정의한 SCS만 포함합니다. 부모·네이티브 생성 컴포넌트, 인스턴스 설정은 별도입니다.

## 변수와 기본값

| 이름 | 타입 | CDO 기본값 |
|---|---|---|
| `AB_Base` | `WB_AB_Base_C` | None |
| `MaxArmor` | `int` | 0 |
| `Armor` | `int` | 0 |
| `Damage` | `int` | 0 |

## 그래프 목록

| 그래프 | 전체 노드 수 |
|---|---|
| EventGraph | 57 |
| GetCurrentArmor | 7 |
| GetCurrentMaxArmor | 7 |
| PopulateArmorBar | 6 |

## 그래프별 실행 연결

실선은 출력 exec 핀의 실제 연결입니다. 여러 분기는 출력 핀 이름을 표시합니다. 함수 호출·delegate·타이머의 다음 그래프 실행은 이 그림에서 자동으로 이어 붙이지 않습니다.

순수 계산 노드 및 데이터 핀은 아래 목록과 원본 JSON을 확인합니다. 연결되지 않은 이벤트도 표시되므로, 노드가 있다는 이유만으로 실행된다고 판단하면 안 됩니다.

### EventGraph

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["AddArmorBar<br/>Custom Event"]
  n1["RemoveArmorBar<br/>Custom Event"]
  n2["Clear Overlay Children"]
  n3["Clear Overlay Children"]
  n4["OnArmorBarFadedOut<br/>Custom Event"]
  n5["Is Valid"]
  n6["Populate Armor Bar<br/>타깃은 WB AB Container"]
  n7["Is Valid"]
  n8["Is Valid"]
  n9["Is Valid"]
  n10["UpdateMaxArmor<br/>Custom Event"]
  n11["UpdateArmor<br/>Custom Event"]
  n12["Set MaxArmor"]
  n13["Set Armor"]
  n14["Update Max Armor<br/>타깃은 WB AB Container"]
  n15["Update Armor<br/>타깃은 WB AB Container"]
  n16["RemoveArmor<br/>Custom Event"]
  n17["Set Damage"]
  n18["Is Valid"]
  n19["Update Armor<br/>타깃은 WB AB Base"]
  n20["Remove Armor<br/>타깃은 WB AB Base"]
  n21["Remove Armor Bar<br/>타깃은 WB AB Base"]
  n22["Bind Event to On Armor Bar Faded Out"]
  n23["Unbind Event from On Armor Bar Faded Out"]
  n24["Update Max Health<br/>타깃은 WB AB Base"]
  n25["Add or Update Armor Bar<br/>타깃은 WB AB Base"]
  n26["Branch"]
  n27["경유 노드"]
  n28["경유 노드"]
  n29["경유 노드"]
  n0 -->|"then"| n26
  n1 -->|"then"| n5
  n2 -->|"then"| n6
  n3 -->|"then"| n23
  n4 -->|"then"| n3
  n5 -->|"Is Valid"| n21
  n6 -->|"then"| n27
  n7 -->|"Is Valid"| n25
  n8 -->|"Is Valid"| n19
  n9 -->|"Is Valid"| n24
  n10 -->|"then"| n12
  n11 -->|"then"| n13
  n12 -->|"then"| n9
  n13 -->|"then"| n8
  n14 -->|"then"| n15
  n16 -->|"then"| n17
  n17 -->|"then"| n18
  n18 -->|"Is Valid"| n20
  n21 -->|"then"| n22
  n25 -->|"then"| n14
  n26 -->|"then"| n2
  n26 -->|"else"| n28
  n27 -->|"OutputPin"| n7
  n28 -->|"OutputPin"| n29
  n29 -->|"OutputPin"| n27
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_CustomEvent_0` | AddArmorBar<br/>Custom Event |  |
| `K2Node_CustomEvent_2` | RemoveArmorBar<br/>Custom Event |  |
| `K2Node_MacroInstance_417` | Clear Overlay Children |  |
| `K2Node_MacroInstance_438` | Clear Overlay Children |  |
| `EdGraphNode_Comment_48` | Event Remove ArmorBar |  |
| `K2Node_CustomEvent_42` | OnArmorBarFadedOut<br/>Custom Event |  |
| `K2Node_VariableGet_43` | Get AB_Base |  |
| `K2Node_MacroInstance_76` | Is Valid |  |
| `K2Node_Knot_223` | 경유 노드 |  |
| `K2Node_CallFunction_268` | Populate Armor Bar<br/>타깃은 WB AB Container |  |
| `K2Node_MacroInstance_180` | Is Valid |  |
| `EdGraphNode_Comment_95` | Event Add Armor Bar |  |
| `K2Node_VariableGet_148` | Get AB_Base |  |
| `K2Node_MacroInstance_146` | Is Valid |  |
| `K2Node_VariableGet_460` | Get AB_Base |  |
| `K2Node_MacroInstance_135` | Is Valid |  |
| `K2Node_CustomEvent_213` | UpdateMaxArmor<br/>Custom Event |  |
| `K2Node_CustomEvent_221` | UpdateArmor<br/>Custom Event |  |
| `K2Node_Knot_295` | 경유 노드 |  |
| `K2Node_Knot_296` | 경유 노드 |  |
| `K2Node_Knot_298` | 경유 노드 |  |
| `K2Node_Knot_299` | 경유 노드 |  |
| `EdGraphNode_Comment_61` | Event Add Armor |  |
| `EdGraphNode_Comment_98` | Event Update Max Armor |  |
| `K2Node_Knot_419` | 경유 노드 |  |
| `K2Node_VariableSet_103` | Set MaxArmor |  |
| `K2Node_VariableSet_107` | Set Armor |  |
| `K2Node_CallFunction_97` | Update Max Armor<br/>타깃은 WB AB Container |  |
| `K2Node_CallFunction_98` | Update Armor<br/>타깃은 WB AB Container |  |
| `K2Node_VariableGet_245` | Get Armor |  |
| `K2Node_VariableGet_246` | Get MaxArmor |  |
| `K2Node_CustomEvent_53` | RemoveArmor<br/>Custom Event |  |
| `EdGraphNode_Comment_97` | Event Remove Armor |  |
| `K2Node_VariableSet_359` | Set Damage |  |
| `K2Node_VariableGet_185` | Get AB_Base |  |
| `K2Node_MacroInstance_181` | Is Valid |  |
| `K2Node_Knot_646` | 경유 노드 |  |
| `K2Node_Knot_647` | 경유 노드 |  |
| `K2Node_CallFunction_413` | Update Armor<br/>타깃은 WB AB Base |  |
| `K2Node_Knot_1145` | 경유 노드 |  |
| `K2Node_CallFunction_414` | Remove Armor<br/>타깃은 WB AB Base |  |
| `K2Node_Knot_1146` | 경유 노드 |  |
| `K2Node_CallFunction_455` | Remove Armor Bar<br/>타깃은 WB AB Base |  |
| `K2Node_Knot_1147` | 경유 노드 |  |
| `K2Node_AddDelegate_25` | Bind Event to On Armor Bar Faded Out |  |
| `K2Node_Knot_1148` | 경유 노드 |  |
| `K2Node_RemoveDelegate_25` | Unbind Event from On Armor Bar Faded Out |  |
| `K2Node_CallFunction_547` | Update Max Health<br/>타깃은 WB AB Base |  |
| `K2Node_Knot_1187` | 경유 노드 |  |
| `K2Node_CallFunction_588` | Add or Update Armor Bar<br/>타깃은 WB AB Base |  |
| `K2Node_VariableGet_94` | Get AB_Base |  |
| `K2Node_Knot_238` | 경유 노드 |  |
| `K2Node_IfThenElse_9` | Branch |  |
| `K2Node_Knot_239` | 경유 노드 |  |
| `K2Node_Knot_240` | 경유 노드 |  |
| `K2Node_Knot_241` | 경유 노드 |  |
| `K2Node_Composite_53` | OV_Has_No_children?<br/>Collapsed Graph |  |

</details>

### GetCurrentArmor

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["GetCurrentArmor"]
  n1["반환 노드"]
  n2["Is Valid"]
  n3["반환 노드"]
  n4["Get Current Health<br/>타깃은 WB AB Base"]
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
| `K2Node_FunctionEntry_263` | GetCurrentArmor |  |
| `K2Node_FunctionResult_1` | 반환 노드 |  |
| `K2Node_VariableGet_185` | Get AB_Base |  |
| `K2Node_MacroInstance_181` | Is Valid |  |
| `K2Node_FunctionResult_0` | 반환 노드 | CurrentArmor=0 |
| `K2Node_CallFunction_669` | Get Current Health<br/>타깃은 WB AB Base |  |
| `K2Node_Knot_1226` | 경유 노드 |  |

</details>

### GetCurrentMaxArmor

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["GetCurrentMaxArmor"]
  n1["반환 노드"]
  n2["Is Valid"]
  n3["반환 노드"]
  n4["Get Current Max Health<br/>타깃은 WB AB Base"]
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
| `K2Node_FunctionEntry_263` | GetCurrentMaxArmor |  |
| `K2Node_FunctionResult_1` | 반환 노드 |  |
| `K2Node_VariableGet_185` | Get AB_Base |  |
| `K2Node_MacroInstance_181` | Is Valid |  |
| `K2Node_FunctionResult_0` | 반환 노드 | CurrentMaxArmor=0 |
| `K2Node_CallFunction_750` | Get Current Max Health<br/>타깃은 WB AB Base |  |
| `K2Node_Knot_1265` | 경유 노드 |  |

</details>

### PopulateArmorBar

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["PopulateArmorBar"]
  n1["Create WB AB Base Widget"]
  n2["Add Child to Overlay"]
  n3["Set AB_Base"]
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
| `K2Node_FunctionEntry_193` | PopulateArmorBar |  |
| `K2Node_CallFunction_0` | Get Owning Player<br/>타깃은 위젯 |  |
| `K2Node_CreateWidget_0` | Create WB AB Base Widget | Class=/Game/CompShooterHUD/UI/Content/ArmorBar/WB_AB_Base.WB_AB_Base_C |
| `K2Node_MacroInstance_348` | Add Child to Overlay |  |
| `K2Node_VariableSet_30` | Set AB_Base |  |
| `K2Node_FunctionResult_1` | 반환 노드 |  |

</details>

## 조회 범위의 한계

ObjectIterator로 발견했으나 MCP helper가 반환하지 않은 그래프입니다. delegate signature·컴파일된 중간 그래프·macro·애니메이션 내부 그래프가 섞일 수 있습니다.

- `AddArmorBar`: Graph 'AddArmorBar' not found in Blueprint.
- `AddChildToOverlay`: Graph 'AddChildToOverlay' not found in Blueprint.
- `ClearOverlayChildren`: Graph 'ClearOverlayChildren' not found in Blueprint.
- `OnArmorBarFadedOut`: Graph 'OnArmorBarFadedOut' not found in Blueprint.
- `RemoveArmor`: Graph 'RemoveArmor' not found in Blueprint.
- `RemoveArmorBar`: Graph 'RemoveArmorBar' not found in Blueprint.
- `UpdateArmor`: Graph 'UpdateArmor' not found in Blueprint.
- `UpdateMaxArmor`: Graph 'UpdateMaxArmor' not found in Blueprint.

[원본 MCP 스냅샷](../../../../../Evidence/blueprints-316.json)
