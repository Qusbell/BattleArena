# BP_Weapon_ShotGun

에셋: `/Game/QuakeLike_1_0/HeldItem/Gun/BP_Weapon_ShotGun`

부모: `/Script/Engine.BlueprintGeneratedClass'/Game/QuakeLike_1_0/HeldItem/Gun/BP_WeaponBase.BP_WeaponBase_C'`

수집일: 2026-10-12. MCP에서 읽은 그래프·핀 연결의 자동 전사입니다. 상속된 그래프는 부모 문서를 함께 확인합니다.

정적 루트 참조 도달: 있음. 실행 여부 또는 삭제 가능 여부의 확정이 아닙니다.

컴포넌트 목록은 이 BP가 직접 정의한 SCS만 포함합니다. 부모·네이티브 생성 컴포넌트, 인스턴스 설정은 별도입니다.

## 변수와 기본값

| 이름 | 타입 | CDO 기본값 |
|---|---|---|
| `baseVector` | `Vector` | &lt;Struct 'Vector' {x: 0.000000, y: 0.000000, z: 0.000000}&gt; |
| `baseDistance` | `real` | 0.0 |
| `baseDirection` | `Vector` | &lt;Struct 'Vector' {x: 0.000000, y: 0.000000, z: 0.000000}&gt; |
| `pelletDirection` | `Vector` | &lt;Struct 'Vector' {x: 0.000000, y: 0.000000, z: 0.000000}&gt; |

## 그래프 목록

| 그래프 | 전체 노드 수 |
|---|---|
| EventGraph | 0 |
| GetDamage | 15 |
| SetBase | 12 |
| SetPelletDirection | 6 |
| StartFire | 45 |
| UserConstructionScript | 2 |

## 그래프별 실행 연결

실선은 출력 exec 핀의 실제 연결입니다. 여러 분기는 출력 핀 이름을 표시합니다. 함수 호출·delegate·타이머의 다음 그래프 실행은 이 그림에서 자동으로 이어 붙이지 않습니다.

순수 계산 노드 및 데이터 핀은 아래 목록과 원본 JSON을 확인합니다. 연결되지 않은 이벤트도 표시되므로, 노드가 있다는 이유만으로 실행된다고 판단하면 안 됩니다.

### EventGraph

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|

</details>

### GetDamage

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["GetDamage"]
  n1["반환 노드"]
  n0 -->|"then"| n1
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | GetDamage |  |
| `K2Node_VariableGet_0` | Get weaponData |  |
| `K2Node_VariableGet_2` | Get damage |  |
| `K2Node_FunctionResult_0` | 반환 노드 |  |
| `K2Node_VariableGet_3` | Get minDamageMultiplier |  |
| `K2Node_VariableGet_4` | Get fallOffStart |  |
| `K2Node_VariableGet_5` | Get range |  |
| `K2Node_PromotableOperator_0` | float * float |  |
| `K2Node_CallFunction_0` | Lerp |  |
| `K2Node_Knot_0` | 경유 노드 |  |
| `K2Node_CallFunction_1` | Map Range Clamped | OutRangeA=0.0; OutRangeB=1.000000 |
| `K2Node_Knot_1` | 경유 노드 |  |
| `K2Node_Knot_2` | 경유 노드 |  |
| `K2Node_Knot_3` | 경유 노드 |  |
| `K2Node_Knot_4` | 경유 노드 |  |

</details>

### SetBase

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["SetBase"]
  n1["Set baseVector"]
  n2["Set baseDistance"]
  n3["Set baseDirection"]
  n0 -->|"then"| n1
  n1 -->|"then"| n2
  n2 -->|"then"| n3
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | SetBase |  |
| `K2Node_CallFunction_0` | Get Hit Scan Aim |  |
| `K2Node_PromotableOperator_0` | vector - vector |  |
| `K2Node_VariableSet_0` | Set baseVector |  |
| `K2Node_VariableSet_1` | Set baseDistance | Output_Get=0.0 |
| `K2Node_CallFunction_1` | Vector Length |  |
| `K2Node_Knot_0` | 경유 노드 |  |
| `K2Node_CallFunction_2` | Normalize | Tolerance=0.000100 |
| `K2Node_VariableSet_2` | Set baseDirection | Output_Get=0, 0, 0 |
| `K2Node_Knot_1` | 경유 노드 |  |
| `K2Node_Knot_2` | 경유 노드 |  |
| `K2Node_Knot_3` | 경유 노드 |  |

</details>

### SetPelletDirection

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["SetPelletDirection"]
  n1["Set pelletDirection"]
  n0 -->|"then"| n1
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | SetPelletDirection |  |
| `K2Node_VariableSet_0` | Set pelletDirection | Output_Get=0, 0, 0 |
| `K2Node_VariableGet_0` | Get weaponData |  |
| `K2Node_VariableGet_1` | Get spread |  |
| `K2Node_CallFunction_0` | Random Unit Vector in Cone in Degrees |  |
| `K2Node_VariableGet_2` | Get baseDirection |  |

</details>

### StartFire

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["StartFire"]
  n1["시퀀스"]
  n2["Set Hit Scan<br/>타깃은 BP Weapon Base"]
  n3["Set Base<br/>타깃은 BP Weapon Shot Gun"]
  n4["For Loop"]
  n5["시퀀스"]
  n6["Set Pellet Direction<br/>타깃은 BP Weapon Shot Gun"]
  n7["Line Trace By Channel"]
  n8["Add"]
  n9["Is Valid"]
  n10["Add"]
  n11["Set hitSoundCount"]
  n12["경유 노드"]
  n13["[ C]Ch Hit Sound<br/>타깃은 BP Weapon Base<br/>소유 클라이언트에 리플리케이트됨 (서버인 경우)"]
  n14["[ M]Hit Effect<br/>타깃은 BP Weapon Base<br/>모두에 리플리케이트됨 (서버인 경우)"]
  n15["Cast To Character"]
  n16["Branch"]
  n0 -->|"then"| n1
  n1 -->|"then_0"| n2
  n1 -->|"then_1"| n12
  n1 -->|"then_2"| n13
  n2 -->|"then"| n3
  n4 -->|"LoopBody"| n5
  n5 -->|"then_0"| n6
  n5 -->|"then_1"| n7
  n7 -->|"then"| n16
  n8 -->|"then"| n9
  n9 -->|"Is Valid"| n10
  n10 -->|"then"| n15
  n12 -->|"OutputPin"| n4
  n13 -->|"then"| n14
  n15 -->|"then"| n11
  n16 -->|"then"| n8
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | StartFire |  |
| `K2Node_ExecutionSequence_0` | 시퀀스 |  |
| `K2Node_CallFunction_0` | Set Hit Scan<br/>타깃은 BP Weapon Base |  |
| `K2Node_CallFunction_1` | Set Base<br/>타깃은 BP Weapon Shot Gun |  |
| `K2Node_MacroInstance_0` | For Loop |  |
| `K2Node_VariableGet_0` | Get weaponData |  |
| `K2Node_VariableGet_1` | Get pellet |  |
| `K2Node_PromotableOperator_0` | int - int | B=1 |
| `K2Node_ExecutionSequence_1` | 시퀀스 |  |
| `K2Node_CallFunction_3` | Set Pellet Direction<br/>타깃은 BP Weapon Shot Gun |  |
| `K2Node_CallFunction_4` | Line Trace By Channel | bTraceComplex=false; DrawDebugType=None; bIgnoreSelf=true; TraceColor=(R=1.000000,G=0.000000,B=0.000000,A=1.000000); TraceHitColor=(R=0.000000,G=1.000000,B=0.000000,A=1.000000); DrawTime=0.000000 |
| `K2Node_VariableGet_2` | Get pelletDirection |  |
| `K2Node_VariableGet_3` | Get baseDistance |  |
| `K2Node_PromotableOperator_1` | vector * vector |  |
| `K2Node_PromotableOperator_2` | vector + vector |  |
| `K2Node_CallFunction_5` | Get Hit Scan Aim | aimEnd=0, 0, 0 |
| `K2Node_Knot_0` | 경유 노드 |  |
| `K2Node_VariableGet_4` | Get weaponData |  |
| `K2Node_VariableGet_5` | Get traceChannel |  |
| `K2Node_CallFunction_6` | Get Owner<br/>타깃은 액터 |  |
| `K2Node_MakeArray_0` | 배열 만들기 |  |
| `K2Node_CallFunction_7` | Break Hit Result | bBlockingHit=false; bInitialOverlap=false; Time=0.0; Location=0, 0, 0; Normal=0, 0, 0; HitBoneName=None; BoneName=None; HitItem=0; ElementIndex=0; FaceIndex=0; TraceStart=0, 0, 0; TraceEnd=0, 0, 0 |
| `K2Node_VariableGet_6` | Get effectVector |  |
| `K2Node_CallArrayFunction_0` | Add | ReturnValue=0 |
| `K2Node_MakeStruct_0` | Make ST Hit Effect Vector |  |
| `K2Node_MacroInstance_1` | Is Valid |  |
| `K2Node_VariableGet_7` | Get hitInfo |  |
| `K2Node_CallArrayFunction_1` | Add | ReturnValue=0 |
| `K2Node_Knot_4` | 경유 노드 |  |
| `K2Node_CallFunction_8` | Get Damage |  |
| `K2Node_Knot_5` | 경유 노드 |  |
| `K2Node_Knot_6` | 경유 노드 |  |
| `K2Node_Knot_7` | 경유 노드 |  |
| `K2Node_Knot_8` | 경유 노드 |  |
| `K2Node_VariableGet_8` | Get hitSoundCount |  |
| `K2Node_VariableSet_0` | Set hitSoundCount | Output_Get=0 |
| `K2Node_PromotableOperator_3` | int + int | B=1 |
| `K2Node_Knot_9` | 경유 노드 |  |
| `K2Node_CallFunction_9` | [ C]Ch Hit Sound<br/>타깃은 BP Weapon Base<br/>소유 클라이언트에 리플리케이트됨 (서버인 경우) |  |
| `K2Node_CallFunction_11` | [ M]Hit Effect<br/>타깃은 BP Weapon Base<br/>모두에 리플리케이트됨 (서버인 경우) |  |
| `K2Node_VariableGet_9` | Get effectVector |  |
| `K2Node_VariableGet_10` | Get hitSoundCount |  |
| `K2Node_DynamicCast_0` | Cast To Character |  |
| `K2Node_Knot_1` | 경유 노드 |  |
| `K2Node_IfThenElse_0` | Branch |  |

</details>

### UserConstructionScript

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["Construction Script"]
  n1["부모: Construction Script"]
  n0 -->|"then"| n1
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | Construction Script |  |
| `K2Node_CallParentFunction_0` | 부모: Construction Script |  |

</details>


[원본 MCP 스냅샷](../../../../Evidence/blueprints-105.json)
