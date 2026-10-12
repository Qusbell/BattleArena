# WB_TKC_Base

에셋: `/Game/CompShooterHUD/UI/Content/TeamKillCount/WB_TKC_Base`

부모: `/Script/CoreUObject.Class'/Script/UMG.UserWidget'`

수집일: 2026-10-12. MCP에서 읽은 그래프·핀 연결의 자동 전사입니다. 상속된 그래프는 부모 문서를 함께 확인합니다.

정적 루트 참조 도달: 있음. 실행 여부 또는 삭제 가능 여부의 확정이 아닙니다.

컴포넌트 목록은 이 BP가 직접 정의한 SCS만 포함합니다. 부모·네이티브 생성 컴포넌트, 인스턴스 설정은 별도입니다.

## 변수와 기본값

| 이름 | 타입 | CDO 기본값 |
|---|---|---|
| `WB_Team1` | `WB_Team1_C` | None |
| `TeamName` | `text` |  |
| `TeamTextColor` | `LinearColor` | &lt;Struct 'LinearColor' {r: 0.000000, g: 0.000000, b: 0.000000, a: 0.000000}&gt; |
| `ValueTextColor` | `LinearColor` | &lt;Struct 'LinearColor' {r: 0.000000, g: 0.000000, b: 0.000000, a: 0.000000}&gt; |
| `FrameColor` | `LinearColor` | &lt;Struct 'LinearColor' {r: 0.000000, g: 0.000000, b: 0.000000, a: 0.000000}&gt; |
| `ProgressColor` | `LinearColor` | &lt;Struct 'LinearColor' {r: 0.000000, g: 0.000000, b: 0.000000, a: 0.000000}&gt; |
| `BackgroundColor` | `LinearColor` | &lt;Struct 'LinearColor' {r: 0.000000, g: 0.000000, b: 0.000000, a: 0.000000}&gt; |
| `WB_Team2` | `WB_Team2_C` | None |

## 그래프 목록

| 그래프 | 전체 노드 수 |
|---|---|
| EventGraph | 44 |
| GetTeamKillCountMaxValue | 16 |
| GetTeamKillCountValue | 16 |
| SetData | 23 |

## 그래프별 실행 연결

실선은 출력 exec 핀의 실제 연결입니다. 여러 분기는 출력 핀 이름을 표시합니다. 함수 호출·delegate·타이머의 다음 그래프 실행은 이 그림에서 자동으로 이어 붙이지 않습니다.

순수 계산 노드 및 데이터 핀은 아래 목록과 원본 JSON을 확인합니다. 연결되지 않은 이벤트도 표시되므로, 노드가 있다는 이유만으로 실행된다고 판단하면 안 됩니다.

### EventGraph

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["AddOrUpdateTeamKillCount<br/>Custom Event"]
  n1["Set Data<br/>타깃은 WB TKC Base"]
  n2["RemoveTeamKillCount<br/>Custom Event"]
  n3["Switch on E_TeamCount"]
  n4["경유 노드"]
  n5["시퀀스"]
  n6["경유 노드"]
  n7["RemoveTeam2<br/>Collapsed Graph"]
  n8["RemoveTeam1<br/>Collapsed Graph"]
  n9["HandleTeamCounts<br/>Collapsed Graph"]
  n10["Get"]
  n11["Get"]
  n12["Update Max Value<br/>타깃은 WB Team 1"]
  n13["Remove Max Value<br/>타깃은 WB Team 1"]
  n14["Update Max Value<br/>타깃은 WB Team 2"]
  n15["Remove Max Value<br/>타깃은 WB Team 2"]
  n16["Get"]
  n17["Get"]
  n18["Get"]
  n19["Get"]
  n20["Update Value<br/>타깃은 WB Team 1"]
  n21["Update Value<br/>타깃은 WB Team 2"]
  n22["Get"]
  n23["Get"]
  n24["Remove Value<br/>타깃은 WB Team 1"]
  n25["Remove Value<br/>타깃은 WB Team 2"]
  n26["UpdateMaxValue<br/>Custom Event"]
  n27["Switch on E_TeamCount"]
  n28["시퀀스"]
  n29["RemoveMaxValue<br/>Custom Event"]
  n30["Switch on E_TeamCount"]
  n31["시퀀스"]
  n32["UpdateValue<br/>Custom Event"]
  n33["Switch on E_TeamCount"]
  n34["시퀀스"]
  n35["RemoveValue<br/>Custom Event"]
  n36["Switch on E_TeamCount"]
  n37["시퀀스"]
  n0 -->|"then"| n1
  n1 -->|"then"| n9
  n2 -->|"then"| n3
  n3 -->|"NewEnumerator0"| n4
  n3 -->|"NewEnumerator1"| n6
  n3 -->|"NewEnumerator2"| n5
  n4 -->|"OutputPin"| n8
  n5 -->|"then_0"| n4
  n5 -->|"then_1"| n6
  n6 -->|"OutputPin"| n7
  n10 -->|"then"| n12
  n11 -->|"then"| n14
  n16 -->|"then"| n13
  n17 -->|"then"| n15
  n18 -->|"then"| n20
  n19 -->|"then"| n21
  n22 -->|"then"| n24
  n23 -->|"then"| n25
  n26 -->|"then"| n27
  n27 -->|"NewEnumerator0"| n10
  n27 -->|"NewEnumerator1"| n11
  n27 -->|"NewEnumerator2"| n28
  n28 -->|"then_0"| n10
  n28 -->|"then_1"| n11
  n29 -->|"then"| n30
  n30 -->|"NewEnumerator0"| n16
  n30 -->|"NewEnumerator1"| n17
  n30 -->|"NewEnumerator2"| n31
  n31 -->|"then_0"| n16
  n31 -->|"then_1"| n17
  n32 -->|"then"| n33
  n33 -->|"NewEnumerator0"| n18
  n33 -->|"NewEnumerator1"| n19
  n33 -->|"NewEnumerator2"| n34
  n34 -->|"then_0"| n18
  n34 -->|"then_1"| n19
  n35 -->|"then"| n36
  n36 -->|"NewEnumerator0"| n22
  n36 -->|"NewEnumerator1"| n23
  n36 -->|"NewEnumerator2"| n37
  n37 -->|"then_0"| n22
  n37 -->|"then_1"| n23
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_CustomEvent_0` | AddOrUpdateTeamKillCount<br/>Custom Event |  |
| `K2Node_CallFunction_339` | Set Data<br/>타깃은 WB TKC Base |  |
| `K2Node_CustomEvent_131` | RemoveTeamKillCount<br/>Custom Event |  |
| `K2Node_SwitchEnum_132` | Switch on E_TeamCount |  |
| `K2Node_Knot_554` | 경유 노드 |  |
| `K2Node_ExecutionSequence_154` | 시퀀스 |  |
| `K2Node_Knot_1071` | 경유 노드 |  |
| `EdGraphNode_Comment_303` | Event Remove Team Kill Count |  |
| `K2Node_Composite_147` | RemoveTeam2<br/>Collapsed Graph |  |
| `K2Node_Composite_148` | RemoveTeam1<br/>Collapsed Graph |  |
| `K2Node_Composite_157` | HandleTeamCounts<br/>Collapsed Graph |  |
| `EdGraphNode_Comment_360` | Event Add Or Update Team Kill Count |  |
| `K2Node_VariableGet_4485` | Get |  |
| `K2Node_VariableGet_4486` | Get |  |
| `K2Node_CallFunction_5117` | Update Max Value<br/>타깃은 WB Team 1 |  |
| `K2Node_CallFunction_5118` | Remove Max Value<br/>타깃은 WB Team 1 |  |
| `K2Node_CallFunction_5119` | Update Max Value<br/>타깃은 WB Team 2 |  |
| `K2Node_CallFunction_5120` | Remove Max Value<br/>타깃은 WB Team 2 |  |
| `K2Node_VariableGet_4487` | Get |  |
| `K2Node_VariableGet_4488` | Get |  |
| `K2Node_VariableGet_4489` | Get |  |
| `K2Node_VariableGet_4490` | Get |  |
| `K2Node_CallFunction_5121` | Update Value<br/>타깃은 WB Team 1 |  |
| `K2Node_CallFunction_5122` | Update Value<br/>타깃은 WB Team 2 |  |
| `K2Node_VariableGet_4484` | Get |  |
| `K2Node_VariableGet_996` | Get |  |
| `K2Node_CallFunction_5123` | Remove Value<br/>타깃은 WB Team 1 |  |
| `K2Node_CallFunction_5124` | Remove Value<br/>타깃은 WB Team 2 |  |
| `K2Node_CustomEvent_431` | UpdateMaxValue<br/>Custom Event |  |
| `K2Node_SwitchEnum_415` | Switch on E_TeamCount |  |
| `K2Node_ExecutionSequence_722` | 시퀀스 |  |
| `K2Node_CustomEvent_499` | RemoveMaxValue<br/>Custom Event |  |
| `K2Node_SwitchEnum_480` | Switch on E_TeamCount |  |
| `K2Node_ExecutionSequence_829` | 시퀀스 |  |
| `K2Node_CustomEvent_492` | UpdateValue<br/>Custom Event |  |
| `K2Node_SwitchEnum_474` | Switch on E_TeamCount |  |
| `K2Node_ExecutionSequence_822` | 시퀀스 |  |
| `K2Node_CustomEvent_465` | RemoveValue<br/>Custom Event |  |
| `K2Node_SwitchEnum_448` | Switch on E_TeamCount |  |
| `K2Node_ExecutionSequence_784` | 시퀀스 |  |
| `EdGraphNode_Comment_392` | Event Update Max Value |  |
| `EdGraphNode_Comment_393` | Event Remove Max Value |  |
| `EdGraphNode_Comment_394` | Event Update Value |  |
| `EdGraphNode_Comment_395` | Event Remove Value |  |

</details>

### GetTeamKillCountMaxValue

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["GetTeamKillCountMaxValue"]
  n1["반환 노드"]
  n2["Get"]
  n3["Get"]
  n4["Switch on E_TeamCount"]
  n5["반환 노드"]
  n6["반환 노드"]
  n7["경유 노드"]
  n8["Get"]
  n9["Get"]
  n10["반환 노드"]
  n0 -->|"then"| n4
  n2 -->|"then"| n6
  n2 -->|"else"| n10
  n3 -->|"then"| n1
  n3 -->|"else"| n10
  n4 -->|"NewEnumerator0"| n2
  n4 -->|"NewEnumerator1"| n3
  n4 -->|"NewEnumerator2"| n7
  n7 -->|"OutputPin"| n8
  n8 -->|"then"| n9
  n8 -->|"else"| n10
  n9 -->|"then"| n5
  n9 -->|"else"| n10
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_91` | GetTeamKillCountMaxValue |  |
| `K2Node_FunctionResult_3` | 반환 노드 |  |
| `K2Node_VariableGet_670` | Get |  |
| `K2Node_VariableGet_671` | Get |  |
| `K2Node_SwitchEnum_79` | Switch on E_TeamCount |  |
| `K2Node_FunctionResult_0` | 반환 노드 |  |
| `K2Node_FunctionResult_1` | 반환 노드 |  |
| `K2Node_Knot_332` | 경유 노드 |  |
| `K2Node_VariableGet_299` | Get |  |
| `K2Node_VariableGet_300` | Get |  |
| `K2Node_CommutativeAssociativeBinaryOperator_0` | int + int |  |
| `K2Node_FunctionResult_2` | 반환 노드 | TeamKillCountMaxValue=0 |
| `K2Node_VariableGet_883` | Get MaxValue |  |
| `K2Node_VariableGet_884` | Get MaxValue |  |
| `K2Node_VariableGet_881` | Get MaxValue |  |
| `K2Node_VariableGet_882` | Get MaxValue |  |

</details>

### GetTeamKillCountValue

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["GetTeamKillCountValue"]
  n1["반환 노드"]
  n2["Get"]
  n3["Get"]
  n4["Switch on E_TeamCount"]
  n5["반환 노드"]
  n6["반환 노드"]
  n7["경유 노드"]
  n8["Get"]
  n9["Get"]
  n10["반환 노드"]
  n0 -->|"then"| n4
  n2 -->|"then"| n6
  n2 -->|"else"| n10
  n3 -->|"then"| n1
  n3 -->|"else"| n10
  n4 -->|"NewEnumerator0"| n2
  n4 -->|"NewEnumerator1"| n3
  n4 -->|"NewEnumerator2"| n7
  n7 -->|"OutputPin"| n8
  n8 -->|"then"| n9
  n8 -->|"else"| n10
  n9 -->|"then"| n5
  n9 -->|"else"| n10
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_91` | GetTeamKillCountValue |  |
| `K2Node_FunctionResult_3` | 반환 노드 |  |
| `K2Node_VariableGet_670` | Get |  |
| `K2Node_VariableGet_671` | Get |  |
| `K2Node_VariableGet_672` | Get Value |  |
| `K2Node_SwitchEnum_79` | Switch on E_TeamCount |  |
| `K2Node_FunctionResult_0` | 반환 노드 |  |
| `K2Node_FunctionResult_1` | 반환 노드 |  |
| `K2Node_VariableGet_673` | Get Value |  |
| `K2Node_Knot_332` | 경유 노드 |  |
| `K2Node_VariableGet_299` | Get |  |
| `K2Node_VariableGet_300` | Get |  |
| `K2Node_VariableGet_521` | Get Value |  |
| `K2Node_VariableGet_582` | Get Value |  |
| `K2Node_CommutativeAssociativeBinaryOperator_0` | int + int |  |
| `K2Node_FunctionResult_2` | 반환 노드 | TeamKillCountValue=0 |

</details>

### SetData

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["SetData"]
  n1["Set TeamName"]
  n2["Set TeamTextColor"]
  n3["Set ValueTextColor"]
  n4["Set FrameColor"]
  n5["Set ProgressColor"]
  n6["반환 노드"]
  n7["Set BackgroundColor"]
  n8["Set Content Color and Opacity<br/>타깃은 보더"]
  n9["Switch on E_TeamCount"]
  n10["Set Content Color and Opacity<br/>타깃은 보더"]
  n11["Set Content Color and Opacity<br/>타깃은 보더"]
  n0 -->|"then"| n1
  n1 -->|"then"| n2
  n2 -->|"then"| n3
  n3 -->|"then"| n4
  n4 -->|"then"| n5
  n5 -->|"then"| n7
  n7 -->|"then"| n9
  n8 -->|"then"| n6
  n9 -->|"NewEnumerator0"| n8
  n9 -->|"NewEnumerator1"| n10
  n9 -->|"NewEnumerator2"| n11
  n10 -->|"then"| n6
  n11 -->|"then"| n6
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_102` | SetData |  |
| `K2Node_VariableSet_298` | Set TeamName |  |
| `K2Node_VariableSet_299` | Set TeamTextColor |  |
| `K2Node_VariableSet_300` | Set ValueTextColor |  |
| `K2Node_VariableSet_301` | Set FrameColor |  |
| `K2Node_VariableSet_302` | Set ProgressColor |  |
| `K2Node_Knot_783` | 경유 노드 |  |
| `K2Node_Knot_784` | 경유 노드 |  |
| `K2Node_Knot_785` | 경유 노드 |  |
| `K2Node_Knot_786` | 경유 노드 |  |
| `K2Node_FunctionResult_0` | 반환 노드 |  |
| `K2Node_VariableSet_477` | Set BackgroundColor |  |
| `K2Node_Knot_1006` | 경유 노드 |  |
| `K2Node_VariableGet_2952` | Get b_Anim1Color |  |
| `K2Node_VariableGet_742` | Get ProgressColor |  |
| `K2Node_CallFunction_3340` | Set Content Color and Opacity<br/>타깃은 보더 |  |
| `K2Node_VariableGet_2953` | Get b_Anim2Color |  |
| `K2Node_VariableGet_728` | Get b_Anim1Color |  |
| `K2Node_VariableGet_881` | Get b_Anim2Color |  |
| `K2Node_Knot_1156` | 경유 노드 |  |
| `K2Node_SwitchEnum_269` | Switch on E_TeamCount |  |
| `K2Node_CallFunction_3341` | Set Content Color and Opacity<br/>타깃은 보더 |  |
| `K2Node_CallFunction_2302` | Set Content Color and Opacity<br/>타깃은 보더 |  |

</details>

## 조회 범위의 한계

ObjectIterator로 발견했으나 MCP helper가 반환하지 않은 그래프입니다. delegate signature·컴파일된 중간 그래프·macro·애니메이션 내부 그래프가 섞일 수 있습니다.

- `AddOrUpdateTeamKillCount`: Graph 'AddOrUpdateTeamKillCount' not found in Blueprint.
- `CreateTeam1`: Graph 'CreateTeam1' not found in Blueprint.
- `CreateTeam2`: Graph 'CreateTeam2' not found in Blueprint.
- `PlayFadeAnim1`: Graph 'PlayFadeAnim1' not found in Blueprint.
- `PlayFadeAnim2`: Graph 'PlayFadeAnim2' not found in Blueprint.
- `RemoveMaxValue`: Graph 'RemoveMaxValue' not found in Blueprint.
- `RemoveTeamKillCount`: Graph 'RemoveTeamKillCount' not found in Blueprint.
- `RemoveValue`: Graph 'RemoveValue' not found in Blueprint.
- `Team1Visibility`: Graph 'Team1Visibility' not found in Blueprint.
- `Team2Visibility`: Graph 'Team2Visibility' not found in Blueprint.
- `TimedFadeAnim1`: Graph 'TimedFadeAnim1' not found in Blueprint.
- `TimedFadeAnim2`: Graph 'TimedFadeAnim2' not found in Blueprint.
- `UpdateMaxValue`: Graph 'UpdateMaxValue' not found in Blueprint.
- `UpdateValue`: Graph 'UpdateValue' not found in Blueprint.

[원본 MCP 스냅샷](../../../../../Evidence/blueprints-316.json)
