# BP_Weapon_MachineGun

에셋: `/Game/QuakeLike_1_0/HeldItem/Gun/BP_Weapon_MachineGun`

부모: `/Script/Engine.BlueprintGeneratedClass'/Game/QuakeLike_1_0/HeldItem/Gun/BP_WeaponBase.BP_WeaponBase_C'`

수집일: 2026-10-12. MCP에서 읽은 그래프·핀 연결의 자동 전사입니다. 상속된 그래프는 부모 문서를 함께 확인합니다.

정적 루트 참조 도달: 있음. 실행 여부 또는 삭제 가능 여부의 확정이 아닙니다.

컴포넌트 목록은 이 BP가 직접 정의한 SCS만 포함합니다. 부모·네이티브 생성 컴포넌트, 인스턴스 설정은 별도입니다.

## 그래프 목록

| 그래프 | 전체 노드 수 |
|---|---|
| EventGraph | 0 |
| StartFire | 36 |
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

### StartFire

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["StartFire"]
  n1["Set Hit Scan<br/>타깃은 BP Weapon Base"]
  n2["Line Trace By Channel"]
  n3["시퀀스"]
  n4["Add"]
  n5["Is Valid"]
  n6["Add"]
  n7["Set hitSoundCount"]
  n8["경유 노드"]
  n9["[ M]Hit Scan Beam Effect<br/>타깃은 BP Weapon Base<br/>모두에 리플리케이트됨 (서버인 경우)"]
  n10["경유 노드"]
  n11["[ M]Hit Effect<br/>타깃은 BP Weapon Base<br/>모두에 리플리케이트됨 (서버인 경우)"]
  n12["[ C]Ch Hit Sound<br/>타깃은 BP Weapon Base<br/>소유 클라이언트에 리플리케이트됨 (서버인 경우)"]
  n13["Cast To Character"]
  n14["Branch"]
  n0 -->|"then"| n3
  n2 -->|"then"| n14
  n3 -->|"then_0"| n1
  n3 -->|"then_1"| n8
  n3 -->|"then_2"| n10
  n4 -->|"then"| n5
  n5 -->|"Is Valid"| n6
  n6 -->|"then"| n13
  n8 -->|"OutputPin"| n2
  n9 -->|"then"| n11
  n10 -->|"OutputPin"| n12
  n12 -->|"then"| n9
  n13 -->|"then"| n7
  n14 -->|"then"| n4
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | StartFire |  |
| `K2Node_CallFunction_0` | Set Hit Scan<br/>타깃은 BP Weapon Base |  |
| `K2Node_CallFunction_1` | Get Hit Scan Aim |  |
| `K2Node_VariableGet_0` | Get weaponData |  |
| `K2Node_CallFunction_3` | Line Trace By Channel | bTraceComplex=false; DrawDebugType=None; bIgnoreSelf=true; TraceColor=(R=1.000000,G=0.000000,B=0.000000,A=1.000000); TraceHitColor=(R=0.000000,G=1.000000,B=0.000000,A=1.000000); DrawTime=0.000000 |
| `K2Node_VariableGet_1` | Get traceChannel |  |
| `K2Node_CallFunction_7` | Get Owner<br/>타깃은 액터 |  |
| `K2Node_MakeArray_0` | 배열 만들기 |  |
| `K2Node_ExecutionSequence_0` | 시퀀스 |  |
| `K2Node_CallFunction_5` | Break Hit Result | bBlockingHit=false; bInitialOverlap=false; Time=0.0; Distance=0.0; Location=0, 0, 0; Normal=0, 0, 0; HitBoneName=None; BoneName=None; HitItem=0; ElementIndex=0; FaceIndex=0; TraceStart=0, 0, 0; TraceEnd=0, 0, 0 |
| `K2Node_VariableGet_2` | Get effectVector |  |
| `K2Node_CallArrayFunction_0` | Add | ReturnValue=0 |
| `K2Node_MakeStruct_0` | Make ST Hit Effect Vector |  |
| `K2Node_Knot_0` | 경유 노드 |  |
| `K2Node_Knot_1` | 경유 노드 |  |
| `K2Node_MacroInstance_0` | Is Valid |  |
| `K2Node_Knot_2` | 경유 노드 |  |
| `K2Node_Knot_3` | 경유 노드 |  |
| `K2Node_Knot_4` | 경유 노드 |  |
| `K2Node_VariableGet_3` | Get hitInfo |  |
| `K2Node_CallArrayFunction_1` | Add | ReturnValue=0 |
| `K2Node_Knot_5` | 경유 노드 |  |
| `K2Node_VariableGet_4` | Get weaponData |  |
| `K2Node_VariableGet_8` | Get damage |  |
| `K2Node_VariableSet_0` | Set hitSoundCount | hitSoundCount=1; Output_Get=0 |
| `K2Node_Knot_6` | 경유 노드 |  |
| `K2Node_CallFunction_6` | [ M]Hit Scan Beam Effect<br/>타깃은 BP Weapon Base<br/>모두에 리플리케이트됨 (서버인 경우) | beamPlayback=MachineGun |
| `K2Node_Knot_7` | 경유 노드 |  |
| `K2Node_CallFunction_8` | [ M]Hit Effect<br/>타깃은 BP Weapon Base<br/>모두에 리플리케이트됨 (서버인 경우) |  |
| `K2Node_CallFunction_9` | [ C]Ch Hit Sound<br/>타깃은 BP Weapon Base<br/>소유 클라이언트에 리플리케이트됨 (서버인 경우) |  |
| `K2Node_VariableGet_7` | Get effectVector |  |
| `K2Node_VariableGet_6` | Get hitSoundCount |  |
| `K2Node_DynamicCast_0` | Cast To Character |  |
| `K2Node_Knot_8` | 경유 노드 |  |
| `K2Node_IfThenElse_0` | Branch |  |
| `K2Node_CallFunction_2` | Select Vector |  |

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
