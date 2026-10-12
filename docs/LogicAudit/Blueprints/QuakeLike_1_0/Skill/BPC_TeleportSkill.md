# BPC_TeleportSkill

에셋: `/Game/QuakeLike_1_0/Skill/BPC_TeleportSkill`

부모: `/Script/CoreUObject.Class'/Script/Engine.ActorComponent'`

수집일: 2026-10-12. MCP에서 읽은 그래프·핀 연결의 자동 전사입니다. 상속된 그래프는 부모 문서를 함께 확인합니다.

정적 루트 참조 도달: 있음. 실행 여부 또는 삭제 가능 여부의 확정이 아닙니다.

컴포넌트 목록은 이 BP가 직접 정의한 SCS만 포함합니다. 부모·네이티브 생성 컴포넌트, 인스턴스 설정은 별도입니다.

## 변수와 기본값

| 이름 | 타입 | CDO 기본값 |
|---|---|---|
| `skillCooldownEndTime` | `real` | 0.0 |
| `activeSkillProjectile` | `BP_ProjectileBase_C` | None |
| `skillData` | `PDA_TeleportSkill_C` | None |
| `ownerCharacter` | `BP_ShooterBase_C` | None |
| `projectileFireComponent` | `BPC_ProjectileFire_C` | None |
| `travelLoopAudio` | `AudioComponent` | None |

## 그래프 목록

| 그래프 | 전체 노드 수 |
|---|---|
| AddSkillGaugeByCooldownReduction | 11 |
| EventGraph | 122 |
| GetCooldownPercent | 18 |
| IsSkillReady | 5 |
| NotifyTeleportProjectileSpawned | 25 |
| OnRep_skillCooldownEndTime | 1 |
| ResetTeleportSkill | 9 |
| Server_FIreTeleportProjectile | 37 |
| Server_OnTeleportProjectileExpired | 8 |
| Server_OnTeleportProjectileImpact | 8 |
| Server_TeleportToProjectile | 30 |
| Server_UseTeleportSkillLogic | 15 |
| StopTravelLoopSound | 5 |
| TryUseTeleportSkill | 6 |

## 그래프별 실행 연결

실선은 출력 exec 핀의 실제 연결입니다. 여러 분기는 출력 핀 이름을 표시합니다. 함수 호출·delegate·타이머의 다음 그래프 실행은 이 그림에서 자동으로 이어 붙이지 않습니다.

순수 계산 노드 및 데이터 핀은 아래 목록과 원본 JSON을 확인합니다. 연결되지 않은 이벤트도 표시되므로, 노드가 있다는 이유만으로 실행된다고 판단하면 안 됩니다.

### AddSkillGaugeByCooldownReduction

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["AddSkillGaugeByCooldownReduction"]
  n1["Branch"]
  n2["반환 노드"]
  n3["Set with Notify skillCooldownEndTime"]
  n0 -->|"then"| n1
  n1 -->|"then"| n3
  n1 -->|"else"| n2
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | AddSkillGaugeByCooldownReduction |  |
| `K2Node_CallFunction_0` | Get Owner<br/>타깃은 액터 컴포넌트 |  |
| `K2Node_CallFunction_1` | Has Authority<br/>타깃은 액터 |  |
| `K2Node_IfThenElse_0` | Branch |  |
| `K2Node_FunctionResult_0` | 반환 노드 |  |
| `K2Node_VariableGet_0` | Get skillCooldownEndTime |  |
| `K2Node_PromotableOperator_0` | float - float |  |
| `K2Node_CallFunction_2` | Get Game Time in Seconds |  |
| `K2Node_CommutativeAssociativeBinaryOperator_0` | Max (Float) |  |
| `K2Node_VariableSet_0` | Set with Notify skillCooldownEndTime | Output_Get=0.0 |
| `K2Node_Knot_0` | 경유 노드 |  |

</details>

### EventGraph

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["Begin Play 이벤트"]
  n1["Cast To BP_ShooterBase"]
  n2["Set ownerCharacter"]
  n3["Is Valid"]
  n4["Set projectileFireComponent"]
  n5["Set with Notify skillCooldownEndTime"]
  n6["[S]TryUseTeleportSkillRELIABLE Replicated From Client<br/>Executes On Server<br/>Custom Event"]
  n7["Branch"]
  n8["Server Use Teleport Skill Logic<br/>타깃은 BPC Teleport Skill"]
  n9["[M]OnTeleportProjectileFiredReplicated From Server<br/>Executes On All<br/>Custom Event"]
  n10["Is Valid"]
  n11["Spawn Emitter at Location"]
  n12["시퀀스"]
  n13["Is Valid"]
  n14["Branch"]
  n15["Is Valid"]
  n16["Play Sound 2D"]
  n17["[M]OnTeleportSuccessRELIABLE Replicated From Server<br/>Executes On All<br/>Custom Event"]
  n18["Is Valid"]
  n19["Destroy Actor<br/>타깃은 액터"]
  n20["Set activeSkillProjectile"]
  n21["시퀀스"]
  n22["Is Valid"]
  n23["경유 노드"]
  n24["Spawn Emitter at Location"]
  n25["Spawn Emitter at Location"]
  n26["Is Valid"]
  n27["Spawn Sound at Location"]
  n28["[M]OnTeleportProjectileImpactRELIABLE Replicated From Server<br/>Executes On All<br/>Custom Event"]
  n29["Stop Travel Loop Sound<br/>타깃은 BPC Teleport Skill"]
  n30["Stop Travel Loop Sound<br/>타깃은 BPC Teleport Skill"]
  n31["Is Valid"]
  n32["Set Actor Location<br/>타깃은 액터"]
  n33["Destroy Actor<br/>타깃은 액터"]
  n34["Set activeSkillProjectile"]
  n35["시퀀스"]
  n36["Is Valid"]
  n37["Spawn Emitter at Location"]
  n38["경유 노드"]
  n39["경유 노드"]
  n40["Is Valid"]
  n41["Branch"]
  n42["Is Valid"]
  n43["Spawn Sound at Location"]
  n44["[M]OnTeleportProjectileExpiredRELIABLE Replicated From Server<br/>Executes On All<br/>Custom Event"]
  n45["Stop Travel Loop Sound<br/>타깃은 BPC Teleport Skill"]
  n46["Is Valid"]
  n47["Set Actor Location<br/>타깃은 액터"]
  n48["Destroy Actor<br/>타깃은 액터"]
  n49["Set activeSkillProjectile"]
  n50["InitializeTeleportSkillData 이벤트<br/>BPI Skill에서"]
  n51["Set skillData"]
  n52["Is Valid"]
  n0 -->|"then"| n1
  n1 -->|"then"| n2
  n2 -->|"then"| n3
  n3 -->|"Is Valid"| n4
  n4 -->|"then"| n7
  n6 -->|"then"| n8
  n7 -->|"then"| n52
  n9 -->|"then"| n12
  n10 -->|"Is Valid"| n11
  n12 -->|"then_0"| n10
  n12 -->|"then_1"| n13
  n13 -->|"Is Valid"| n14
  n14 -->|"then"| n15
  n15 -->|"Is Valid"| n16
  n17 -->|"then"| n21
  n18 -->|"Is Valid"| n19
  n19 -->|"then"| n20
  n21 -->|"then_0"| n29
  n21 -->|"then_1"| n23
  n21 -->|"then_2"| n26
  n22 -->|"Is Valid"| n24
  n23 -->|"OutputPin"| n22
  n24 -->|"then"| n25
  n26 -->|"Is Valid"| n27
  n28 -->|"then"| n35
  n29 -->|"then"| n18
  n30 -->|"then"| n31
  n31 -->|"Is Valid"| n32
  n32 -->|"then"| n33
  n33 -->|"then"| n34
  n35 -->|"then_0"| n30
  n35 -->|"then_1"| n38
  n35 -->|"then_2"| n40
  n36 -->|"Is Valid"| n37
  n38 -->|"OutputPin"| n39
  n39 -->|"OutputPin"| n36
  n40 -->|"Is Valid"| n41
  n41 -->|"then"| n42
  n42 -->|"Is Valid"| n43
  n44 -->|"then"| n45
  n45 -->|"then"| n46
  n46 -->|"Is Valid"| n47
  n47 -->|"then"| n48
  n48 -->|"then"| n49
  n50 -->|"then"| n51
  n52 -->|"Is Valid"| n5
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_Event_0` | Begin Play 이벤트 |  |
| `K2Node_CallFunction_2` | Get Owner<br/>타깃은 액터 컴포넌트 |  |
| `K2Node_DynamicCast_1` | Cast To BP_ShooterBase |  |
| `K2Node_VariableSet_3` | Set ownerCharacter |  |
| `K2Node_CallFunction_5` | Get Component by Class<br/>타깃은 액터 | ComponentClass=/Game/QuakeLike_1_0/HeldItem/Component/BPC_ProjectileFire.BPC_ProjectileFire_C |
| `K2Node_MacroInstance_0` | Is Valid |  |
| `K2Node_VariableSet_5` | Set projectileFireComponent |  |
| `K2Node_Knot_0` | 경유 노드 |  |
| `K2Node_Knot_1` | 경유 노드 |  |
| `K2Node_CallFunction_6` | Get Game Time in Seconds |  |
| `K2Node_VariableGet_1` | Get skillData |  |
| `K2Node_VariableGet_17` | Get skillCooldown |  |
| `K2Node_PromotableOperator_1` | float + float |  |
| `K2Node_VariableSet_6` | Set with Notify skillCooldownEndTime | Output_Get=0.0 |
| `K2Node_CustomEvent_2` | [S]TryUseTeleportSkillRELIABLE Replicated From Client<br/>Executes On Server<br/>Custom Event |  |
| `K2Node_CallFunction_7` | Get Owner<br/>타깃은 액터 컴포넌트 |  |
| `K2Node_CallFunction_8` | Has Authority<br/>타깃은 액터 |  |
| `K2Node_IfThenElse_0` | Branch |  |
| `K2Node_CallFunction_9` | Server Use Teleport Skill Logic<br/>타깃은 BPC Teleport Skill |  |
| `K2Node_CustomEvent_10` | [M]OnTeleportProjectileFiredReplicated From Server<br/>Executes On All<br/>Custom Event |  |
| `K2Node_MacroInstance_4` | Is Valid |  |
| `K2Node_VariableGet_19` | Get skillData |  |
| `K2Node_VariableGet_20` | Get fireEffect |  |
| `K2Node_Knot_2` | 경유 노드 |  |
| `K2Node_CallFunction_11` | Spawn Emitter at Location | Scale=1.000000,1.000000,1.000000; bAutoDestroy=true; PoolingMethod=None; bAutoActivateSystem=true |
| `K2Node_Knot_3` | 경유 노드 |  |
| `K2Node_Knot_4` | 경유 노드 |  |
| `K2Node_Knot_5` | 경유 노드 |  |
| `K2Node_ExecutionSequence_0` | 시퀀스 |  |
| `K2Node_MacroInstance_5` | Is Valid |  |
| `K2Node_VariableGet_21` | Get ownerCharacter |  |
| `K2Node_Knot_6` | 경유 노드 |  |
| `K2Node_CallFunction_12` | Is Locally Controlled<br/>타깃은 폰 |  |
| `K2Node_IfThenElse_1` | Branch |  |
| `K2Node_VariableGet_22` | Get skillData |  |
| `K2Node_VariableGet_23` | Get fireSound |  |
| `K2Node_MacroInstance_6` | Is Valid |  |
| `K2Node_Knot_18` | 경유 노드 |  |
| `K2Node_CallFunction_13` | Play Sound 2D | PitchMultiplier=1.000000; StartTime=0.000000; bIsUISound=true |
| `K2Node_Knot_19` | 경유 노드 |  |
| `K2Node_VariableGet_24` | Get fireSoundMultiplier |  |
| `K2Node_Knot_20` | 경유 노드 |  |
| `K2Node_CustomEvent_11` | [M]OnTeleportSuccessRELIABLE Replicated From Server<br/>Executes On All<br/>Custom Event |  |
| `K2Node_VariableGet_26` | Get activeSkillProjectile |  |
| `K2Node_MacroInstance_8` | Is Valid |  |
| `K2Node_CallFunction_15` | Destroy Actor<br/>타깃은 액터 |  |
| `K2Node_VariableSet_8` | Set activeSkillProjectile |  |
| `K2Node_ExecutionSequence_1` | 시퀀스 |  |
| `K2Node_VariableGet_27` | Get skillData |  |
| `K2Node_VariableGet_28` | Get teleportEffect |  |
| `K2Node_MacroInstance_9` | Is Valid |  |
| `K2Node_Knot_23` | 경유 노드 |  |
| `K2Node_CallFunction_17` | Spawn Emitter at Location | Rotation=0, 0, 0; Scale=1.000000,1.000000,1.000000; bAutoDestroy=true; PoolingMethod=None; bAutoActivateSystem=true |
| `K2Node_CallFunction_16` | Spawn Emitter at Location | Rotation=0, 0, 0; Scale=1.000000,1.000000,1.000000; bAutoDestroy=true; PoolingMethod=None; bAutoActivateSystem=true |
| `K2Node_Knot_25` | 경유 노드 |  |
| `K2Node_Knot_26` | 경유 노드 |  |
| `K2Node_MacroInstance_10` | Is Valid |  |
| `K2Node_VariableGet_29` | Get skillData |  |
| `K2Node_VariableGet_30` | Get teleportSound |  |
| `K2Node_Knot_27` | 경유 노드 |  |
| `K2Node_CallFunction_18` | Spawn Sound at Location | Rotation=0, 0, 0; PitchMultiplier=1.000000; StartTime=0.000000; bAutoDestroy=true |
| `K2Node_Knot_28` | 경유 노드 |  |
| `K2Node_Knot_29` | 경유 노드 |  |
| `K2Node_Knot_30` | 경유 노드 |  |
| `K2Node_Knot_31` | 경유 노드 |  |
| `K2Node_VariableGet_31` | Get teleportSoundMultiplier |  |
| `K2Node_Knot_32` | 경유 노드 |  |
| `K2Node_CustomEvent_12` | [M]OnTeleportProjectileImpactRELIABLE Replicated From Server<br/>Executes On All<br/>Custom Event |  |
| `K2Node_CallFunction_19` | Stop Travel Loop Sound<br/>타깃은 BPC Teleport Skill |  |
| `K2Node_CallFunction_20` | Stop Travel Loop Sound<br/>타깃은 BPC Teleport Skill |  |
| `K2Node_VariableGet_32` | Get activeSkillProjectile |  |
| `K2Node_MacroInstance_11` | Is Valid |  |
| `K2Node_Knot_33` | 경유 노드 |  |
| `K2Node_CallFunction_21` | Set Actor Location<br/>타깃은 액터 | bSweep=false; bTeleport=true; ReturnValue=false |
| `K2Node_Knot_34` | 경유 노드 |  |
| `K2Node_VariableGet_33` | Get activeSkillProjectile |  |
| `K2Node_CallFunction_22` | Destroy Actor<br/>타깃은 액터 |  |
| `K2Node_VariableSet_9` | Set activeSkillProjectile |  |
| `K2Node_ExecutionSequence_2` | 시퀀스 |  |
| `K2Node_VariableGet_34` | Get skillData |  |
| `K2Node_VariableGet_35` | Get impactEffect |  |
| `K2Node_MacroInstance_12` | Is Valid |  |
| `K2Node_PromotableOperator_2` | vector * vector | B=3.000000 |
| `K2Node_PromotableOperator_3` | vector + vector |  |
| `K2Node_CallFunction_23` | Make Rot from Z |  |
| `K2Node_Knot_35` | 경유 노드 |  |
| `K2Node_CallFunction_25` | Spawn Emitter at Location | Scale=1.000000,1.000000,1.000000; bAutoDestroy=true; PoolingMethod=None; bAutoActivateSystem=true |
| `K2Node_Knot_36` | 경유 노드 |  |
| `K2Node_Knot_37` | 경유 노드 |  |
| `K2Node_Knot_38` | 경유 노드 |  |
| `K2Node_Knot_39` | 경유 노드 |  |
| `K2Node_Knot_40` | 경유 노드 |  |
| `K2Node_Knot_41` | 경유 노드 |  |
| `K2Node_VariableGet_36` | Get ownerCharacter |  |
| `K2Node_MacroInstance_13` | Is Valid |  |
| `K2Node_Knot_42` | 경유 노드 |  |
| `K2Node_CallFunction_26` | Is Locally Controlled<br/>타깃은 폰 |  |
| `K2Node_IfThenElse_2` | Branch |  |
| `K2Node_VariableGet_37` | Get skillData |  |
| `K2Node_VariableGet_38` | Get impactSound |  |
| `K2Node_MacroInstance_14` | Is Valid |  |
| `K2Node_Knot_43` | 경유 노드 |  |
| `K2Node_CallFunction_27` | Spawn Sound at Location | Rotation=0, 0, 0; PitchMultiplier=1.000000; StartTime=0.000000; bAutoDestroy=true |
| `K2Node_Knot_44` | 경유 노드 |  |
| `K2Node_Knot_45` | 경유 노드 |  |
| `K2Node_Knot_46` | 경유 노드 |  |
| `K2Node_VariableGet_39` | Get impactSoundMultiplier |  |
| `K2Node_Knot_47` | 경유 노드 |  |
| `K2Node_Knot_48` | 경유 노드 |  |
| `K2Node_CustomEvent_13` | [M]OnTeleportProjectileExpiredRELIABLE Replicated From Server<br/>Executes On All<br/>Custom Event |  |
| `K2Node_CallFunction_28` | Stop Travel Loop Sound<br/>타깃은 BPC Teleport Skill |  |
| `K2Node_VariableGet_40` | Get activeSkillProjectile |  |
| `K2Node_MacroInstance_15` | Is Valid |  |
| `K2Node_VariableGet_41` | Get activeSkillProjectile |  |
| `K2Node_CallFunction_29` | Set Actor Location<br/>타깃은 액터 | bSweep=false; bTeleport=true; ReturnValue=false |
| `K2Node_Knot_49` | 경유 노드 |  |
| `K2Node_VariableGet_42` | Get activeSkillProjectile |  |
| `K2Node_CallFunction_30` | Destroy Actor<br/>타깃은 액터 |  |
| `K2Node_VariableSet_10` | Set activeSkillProjectile |  |
| `K2Node_Event_1` | InitializeTeleportSkillData 이벤트<br/>BPI Skill에서 |  |
| `K2Node_VariableSet_11` | Set skillData |  |
| `K2Node_MacroInstance_1` | Is Valid |  |

</details>

### GetCooldownPercent

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["GetCooldownPercent"]
  n1["반환 노드"]
  n2["Is Valid"]
  n3["Branch"]
  n4["경유 노드"]
  n5["경유 노드"]
  n6["반환 노드"]
  n0 -->|"then"| n2
  n2 -->|"Is Valid"| n3
  n2 -->|"Is Not Valid"| n5
  n3 -->|"then"| n6
  n3 -->|"else"| n4
  n4 -->|"OutputPin"| n1
  n5 -->|"OutputPin"| n4
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | GetCooldownPercent |  |
| `K2Node_FunctionResult_0` | 반환 노드 | cooldownPercent=1.000000 |
| `K2Node_VariableGet_0` | Get skillData |  |
| `K2Node_MacroInstance_0` | Is Valid |  |
| `K2Node_Knot_0` | 경유 노드 |  |
| `K2Node_VariableGet_1` | Get skillCooldown |  |
| `K2Node_IfThenElse_0` | Branch |  |
| `K2Node_PromotableOperator_1` | float &gt; float |  |
| `K2Node_CallFunction_0` | Get Game Time in Seconds |  |
| `K2Node_Knot_1` | 경유 노드 |  |
| `K2Node_Knot_2` | 경유 노드 |  |
| `K2Node_PromotableOperator_2` | float - float |  |
| `K2Node_VariableGet_2` | Get skillCooldownEndTime |  |
| `K2Node_PromotableOperator_3` | float / float |  |
| `K2Node_Knot_3` | 경유 노드 |  |
| `K2Node_CallFunction_1` | Clamp (Float) | Min=0.0; Max=1.0 |
| `K2Node_PromotableOperator_4` | float - float | A=1.000000 |
| `K2Node_FunctionResult_1` | 반환 노드 |  |

</details>

### IsSkillReady

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["IsSkillReady"]
  n1["반환 노드"]
  n0 -->|"then"| n1
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | IsSkillReady |  |
| `K2Node_FunctionResult_0` | 반환 노드 |  |
| `K2Node_CallFunction_0` | Get Game Time in Seconds |  |
| `K2Node_PromotableOperator_0` | float &gt;= float |  |
| `K2Node_VariableGet_0` | Get skillCooldownEndTime |  |

</details>

### NotifyTeleportProjectileSpawned

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["NotifyTeleportProjectileSpawned"]
  n1["Set activeSkillProjectile"]
  n2["Is Valid"]
  n3["Is Valid"]
  n4["Branch"]
  n5["반환 노드"]
  n6["Is Valid"]
  n7["Spawn Sound Attached"]
  n8["Set travelLoopAudio"]
  n9["경유 노드"]
  n10["경유 노드"]
  n11["경유 노드"]
  n0 -->|"then"| n2
  n1 -->|"then"| n3
  n2 -->|"Is Valid"| n1
  n2 -->|"Is Not Valid"| n11
  n3 -->|"Is Valid"| n4
  n3 -->|"Is Not Valid"| n10
  n4 -->|"then"| n6
  n4 -->|"else"| n9
  n6 -->|"Is Valid"| n7
  n7 -->|"then"| n8
  n9 -->|"OutputPin"| n5
  n10 -->|"OutputPin"| n9
  n11 -->|"OutputPin"| n10
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | NotifyTeleportProjectileSpawned |  |
| `K2Node_VariableSet_0` | Set activeSkillProjectile |  |
| `K2Node_MacroInstance_0` | Is Valid |  |
| `K2Node_Knot_0` | 경유 노드 |  |
| `K2Node_Knot_1` | 경유 노드 |  |
| `K2Node_Knot_2` | 경유 노드 |  |
| `K2Node_Knot_3` | 경유 노드 |  |
| `K2Node_VariableGet_0` | Get ownerCharacter |  |
| `K2Node_MacroInstance_1` | Is Valid |  |
| `K2Node_Knot_4` | 경유 노드 |  |
| `K2Node_CallFunction_0` | Is Locally Controlled<br/>타깃은 폰 |  |
| `K2Node_IfThenElse_0` | Branch |  |
| `K2Node_FunctionResult_0` | 반환 노드 |  |
| `K2Node_VariableGet_1` | Get skillData |  |
| `K2Node_VariableGet_2` | Get travelLoopSound |  |
| `K2Node_MacroInstance_2` | Is Valid |  |
| `K2Node_CallFunction_1` | Spawn Sound Attached | AttachPointName=None; Location=0, 0, 0; Rotation=0, 0, 0; LocationType=KeepRelativeOffset; bStopWhenAttachedToDestroyed=false; VolumeMultiplier=1.000000; PitchMultiplier=1.000000; StartTime=0.000000; bAutoDestroy=true |
| `K2Node_VariableGet_3` | Get activeSkillProjectile |  |
| `K2Node_VariableGet_4` | Get RootComponent |  |
| `K2Node_Knot_5` | 경유 노드 |  |
| `K2Node_Knot_6` | 경유 노드 |  |
| `K2Node_VariableSet_1` | Set travelLoopAudio |  |
| `K2Node_Knot_7` | 경유 노드 |  |
| `K2Node_Knot_8` | 경유 노드 |  |
| `K2Node_Knot_9` | 경유 노드 |  |

</details>

### OnRep_skillCooldownEndTime

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["OnRep_skillCooldownEndTime"]
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | OnRep_skillCooldownEndTime |  |

</details>

### ResetTeleportSkill

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["ResetTeleportSkill"]
  n1["Branch"]
  n2["[ M]On Teleport Projectile Expired<br/>타깃은 BPC Teleport Skill<br/>RELIABLE 모두에 리플리케이트됨 (서버인 경우)"]
  n3["Is Valid"]
  n4["Set activeSkillProjectile"]
  n0 -->|"then"| n1
  n1 -->|"then"| n3
  n2 -->|"then"| n4
  n3 -->|"Is Valid"| n2
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | ResetTeleportSkill |  |
| `K2Node_CallFunction_0` | Get Owner<br/>타깃은 액터 컴포넌트 |  |
| `K2Node_CallFunction_1` | Has Authority<br/>타깃은 액터 |  |
| `K2Node_IfThenElse_0` | Branch |  |
| `K2Node_CallFunction_2` | [ M]On Teleport Projectile Expired<br/>타깃은 BPC Teleport Skill<br/>RELIABLE 모두에 리플리케이트됨 (서버인 경우) |  |
| `K2Node_VariableGet_0` | Get ownerCharacter |  |
| `K2Node_MacroInstance_0` | Is Valid |  |
| `K2Node_CallFunction_3` | Get Actor Location<br/>타깃은 액터 |  |
| `K2Node_VariableSet_0` | Set activeSkillProjectile |  |

</details>

### Server_FIreTeleportProjectile

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["Server_FIreTeleportProjectile"]
  n1["Is Valid"]
  n2["반환 노드"]
  n3["Is Valid"]
  n4["Is Valid"]
  n5["Get Aim<br/>Target is BPI Aim"]
  n6["경유 노드"]
  n7["경유 노드"]
  n8["경유 노드"]
  n9["[ M]Spawn Teleport Projectile<br/>타깃은 BPC Projectile Fire<br/>RELIABLE 모두에 리플리케이트됨 (서버인 경우)"]
  n10["Set with Notify skillCooldownEndTime"]
  n11["Switch Has Authority"]
  n12["[ M]On Teleport Projectile Fired<br/>타깃은 BPC Teleport Skill<br/>모두에 리플리케이트됨 (서버인 경우)"]
  n13["Get Aim<br/>Target is BPI Aim"]
  n0 -->|"then"| n1
  n1 -->|"Is Valid"| n3
  n1 -->|"Is Not Valid"| n8
  n3 -->|"Is Valid"| n4
  n3 -->|"Is Not Valid"| n7
  n4 -->|"Is Valid"| n5
  n4 -->|"Is Not Valid"| n6
  n5 -->|"then"| n9
  n6 -->|"OutputPin"| n2
  n7 -->|"OutputPin"| n6
  n8 -->|"OutputPin"| n7
  n9 -->|"then"| n13
  n11 -->|"Authority"| n10
  n12 -->|"then"| n11
  n13 -->|"then"| n12
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | Server_FIreTeleportProjectile |  |
| `K2Node_VariableGet_0` | Get skillData |  |
| `K2Node_MacroInstance_0` | Is Valid |  |
| `K2Node_FunctionResult_0` | 반환 노드 |  |
| `K2Node_VariableGet_1` | Get projectileFireComponent |  |
| `K2Node_MacroInstance_1` | Is Valid |  |
| `K2Node_VariableGet_10` | Get ownerCharacter |  |
| `K2Node_MacroInstance_2` | Is Valid |  |
| `K2Node_Message_0` | Get Aim<br/>Target is BPI Aim |  |
| `K2Node_Knot_0` | 경유 노드 |  |
| `K2Node_Knot_1` | 경유 노드 |  |
| `K2Node_Knot_2` | 경유 노드 |  |
| `K2Node_Knot_3` | 경유 노드 |  |
| `K2Node_Knot_4` | 경유 노드 |  |
| `K2Node_Knot_5` | 경유 노드 |  |
| `K2Node_CallFunction_0` | Get Forward Vector |  |
| `K2Node_VariableGet_3` | Get skillData |  |
| `K2Node_VariableGet_4` | Get maxDistance |  |
| `K2Node_PromotableOperator_0` | vector * vector |  |
| `K2Node_PromotableOperator_1` | vector + vector |  |
| `K2Node_VariableGet_9` | Get projectileFireComponent |  |
| `K2Node_CallFunction_1` | [ M]Spawn Teleport Projectile<br/>타깃은 BPC Projectile Fire<br/>RELIABLE 모두에 리플리케이트됨 (서버인 경우) |  |
| `K2Node_Knot_6` | 경유 노드 |  |
| `K2Node_Knot_7` | 경유 노드 |  |
| `K2Node_Knot_8` | 경유 노드 |  |
| `K2Node_Knot_9` | 경유 노드 |  |
| `K2Node_Knot_10` | 경유 노드 |  |
| `K2Node_VariableGet_6` | Get skillData |  |
| `K2Node_CallFunction_2` | Get Game Time in Seconds |  |
| `K2Node_VariableGet_7` | Get skillData |  |
| `K2Node_VariableGet_8` | Get skillCooldown |  |
| `K2Node_VariableSet_0` | Set with Notify skillCooldownEndTime | Output_Get=0.0 |
| `K2Node_PromotableOperator_2` | float + float |  |
| `K2Node_MacroInstance_3` | Switch Has Authority |  |
| `K2Node_CallFunction_3` | [ M]On Teleport Projectile Fired<br/>타깃은 BPC Teleport Skill<br/>모두에 리플리케이트됨 (서버인 경우) |  |
| `K2Node_Message_1` | Get Aim<br/>Target is BPI Aim |  |
| `K2Node_VariableGet_2` | Get ownerCharacter |  |

</details>

### Server_OnTeleportProjectileExpired

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["Server_OnTeleportProjectileExpired"]
  n1["Branch"]
  n2["반환 노드"]
  n3["[ M]On Teleport Projectile Expired<br/>타깃은 BPC Teleport Skill<br/>RELIABLE 모두에 리플리케이트됨 (서버인 경우)"]
  n4["Set activeSkillProjectile"]
  n0 -->|"then"| n1
  n1 -->|"then"| n3
  n1 -->|"else"| n2
  n3 -->|"then"| n4
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | Server_OnTeleportProjectileExpired |  |
| `K2Node_VariableGet_0` | Get activeSkillProjectile |  |
| `K2Node_PromotableOperator_0` | Equal (Object) |  |
| `K2Node_IfThenElse_0` | Branch |  |
| `K2Node_Knot_0` | 경유 노드 |  |
| `K2Node_FunctionResult_0` | 반환 노드 |  |
| `K2Node_CallFunction_0` | [ M]On Teleport Projectile Expired<br/>타깃은 BPC Teleport Skill<br/>RELIABLE 모두에 리플리케이트됨 (서버인 경우) |  |
| `K2Node_VariableSet_0` | Set activeSkillProjectile |  |

</details>

### Server_OnTeleportProjectileImpact

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["Server_OnTeleportProjectileImpact"]
  n1["Branch"]
  n2["반환 노드"]
  n3["[ M]On Teleport Projectile Impact<br/>타깃은 BPC Teleport Skill<br/>RELIABLE 모두에 리플리케이트됨 (서버인 경우)"]
  n4["Set activeSkillProjectile"]
  n0 -->|"then"| n1
  n1 -->|"then"| n3
  n1 -->|"else"| n2
  n3 -->|"then"| n4
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | Server_OnTeleportProjectileImpact |  |
| `K2Node_VariableGet_0` | Get activeSkillProjectile |  |
| `K2Node_PromotableOperator_0` | Equal (Object) |  |
| `K2Node_IfThenElse_0` | Branch |  |
| `K2Node_Knot_0` | 경유 노드 |  |
| `K2Node_FunctionResult_0` | 반환 노드 |  |
| `K2Node_CallFunction_0` | [ M]On Teleport Projectile Impact<br/>타깃은 BPC Teleport Skill<br/>RELIABLE 모두에 리플리케이트됨 (서버인 경우) |  |
| `K2Node_VariableSet_0` | Set activeSkillProjectile |  |

</details>

### Server_TeleportToProjectile

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["Server_TeleportToProjectile"]
  n1["Is Valid"]
  n2["반환 노드"]
  n3["Is Valid"]
  n4["Set Actor Location<br/>타깃은 액터"]
  n5["경유 노드"]
  n6["경유 노드"]
  n7["Set newLocation"]
  n8["Set oldLocation"]
  n9["[ M]On Teleport Success<br/>타깃은 BPC Teleport Skill<br/>RELIABLE 모두에 리플리케이트됨 (서버인 경우)"]
  n10["Set activeSkillProjectile"]
  n11["Clear Accumulated Forces<br/>타깃은 캐릭터 무브먼트 컴포넌트"]
  n12["시퀀스"]
  n13["Stop Movement Immediately<br/>타깃은 무브먼트 컴포넌트"]
  n0 -->|"then"| n1
  n1 -->|"Is Valid"| n7
  n1 -->|"Is Not Valid"| n6
  n3 -->|"Is Valid"| n8
  n3 -->|"Is Not Valid"| n5
  n4 -->|"then"| n9
  n5 -->|"OutputPin"| n2
  n6 -->|"OutputPin"| n5
  n7 -->|"then"| n3
  n8 -->|"then"| n12
  n9 -->|"then"| n10
  n11 -->|"then"| n13
  n12 -->|"then_0"| n11
  n12 -->|"then_1"| n4
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | Server_TeleportToProjectile |  |
| `K2Node_VariableGet_0` | Get activeSkillProjectile |  |
| `K2Node_MacroInstance_0` | Is Valid |  |
| `K2Node_FunctionResult_0` | 반환 노드 |  |
| `K2Node_Knot_0` | 경유 노드 |  |
| `K2Node_CallFunction_0` | Get Actor Location<br/>타깃은 액터 |  |
| `K2Node_VariableGet_1` | Get ownerCharacter |  |
| `K2Node_MacroInstance_1` | Is Valid |  |
| `K2Node_Knot_1` | 경유 노드 |  |
| `K2Node_CallFunction_2` | Set Actor Location<br/>타깃은 액터 | bSweep=false; bTeleport=true; ReturnValue=false |
| `K2Node_Knot_2` | 경유 노드 |  |
| `K2Node_Knot_3` | 경유 노드 |  |
| `K2Node_CallFunction_3` | Get Actor Location<br/>타깃은 액터 |  |
| `K2Node_VariableSet_0` | Set newLocation | Output_Get=0, 0, 0 |
| `K2Node_Knot_5` | 경유 노드 |  |
| `K2Node_VariableSet_1` | Set oldLocation | Output_Get=0, 0, 0 |
| `K2Node_VariableGet_2` | Get newLocation |  |
| `K2Node_CallFunction_4` | [ M]On Teleport Success<br/>타깃은 BPC Teleport Skill<br/>RELIABLE 모두에 리플리케이트됨 (서버인 경우) |  |
| `K2Node_VariableGet_3` | Get newLocation |  |
| `K2Node_VariableGet_4` | Get oldLocation |  |
| `K2Node_VariableSet_2` | Set activeSkillProjectile |  |
| `K2Node_VariableGet_5` | Get ownerCharacter |  |
| `K2Node_VariableGet_6` | Get CharacterMovement |  |
| `K2Node_CallFunction_1` | Clear Accumulated Forces<br/>타깃은 캐릭터 무브먼트 컴포넌트 |  |
| `K2Node_ExecutionSequence_0` | 시퀀스 |  |
| `K2Node_Knot_4` | 경유 노드 |  |
| `K2Node_Knot_6` | 경유 노드 |  |
| `K2Node_Knot_7` | 경유 노드 |  |
| `K2Node_CallFunction_5` | Stop Movement Immediately<br/>타깃은 무브먼트 컴포넌트 |  |
| `K2Node_Knot_8` | 경유 노드 |  |

</details>

### Server_UseTeleportSkillLogic

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["Server_UseTeleportSkillLogic"]
  n1["Is Valid"]
  n2["반환 노드"]
  n3["Is Valid"]
  n4["Server Teleport to Projectile<br/>타깃은 BPC Teleport Skill"]
  n5["Branch"]
  n6["Server FIre Teleport Projectile<br/>타깃은 BPC Teleport Skill"]
  n7["경유 노드"]
  n8["경유 노드"]
  n9["경유 노드"]
  n10["Is Valid"]
  n0 -->|"then"| n1
  n1 -->|"Is Valid"| n3
  n1 -->|"Is Not Valid"| n9
  n3 -->|"Is Valid"| n10
  n3 -->|"Is Not Valid"| n8
  n5 -->|"then"| n6
  n5 -->|"else"| n7
  n7 -->|"OutputPin"| n2
  n8 -->|"OutputPin"| n7
  n9 -->|"OutputPin"| n8
  n10 -->|"Is Valid"| n4
  n10 -->|"Is Not Valid"| n5
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | Server_UseTeleportSkillLogic |  |
| `K2Node_VariableGet_0` | Get skillData |  |
| `K2Node_MacroInstance_0` | Is Valid |  |
| `K2Node_FunctionResult_0` | 반환 노드 |  |
| `K2Node_VariableGet_1` | Get projectileFireComponent |  |
| `K2Node_MacroInstance_1` | Is Valid |  |
| `K2Node_CallFunction_1` | Server Teleport to Projectile<br/>타깃은 BPC Teleport Skill |  |
| `K2Node_IfThenElse_2` | Branch |  |
| `K2Node_CallFunction_2` | Is Skill Ready |  |
| `K2Node_CallFunction_3` | Server FIre Teleport Projectile<br/>타깃은 BPC Teleport Skill |  |
| `K2Node_Knot_0` | 경유 노드 |  |
| `K2Node_Knot_1` | 경유 노드 |  |
| `K2Node_Knot_2` | 경유 노드 |  |
| `K2Node_VariableGet_4` | Get activeSkillProjectile |  |
| `K2Node_MacroInstance_3` | Is Valid |  |

</details>

### StopTravelLoopSound

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["StopTravelLoopSound"]
  n1["Is Valid"]
  n2["Stop<br/>타깃은 오디오 컴포넌트"]
  n3["Set travelLoopAudio"]
  n0 -->|"then"| n1
  n1 -->|"Is Valid"| n2
  n2 -->|"then"| n3
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | StopTravelLoopSound |  |
| `K2Node_VariableGet_25` | Get travelLoopAudio |  |
| `K2Node_MacroInstance_7` | Is Valid |  |
| `K2Node_CallFunction_14` | Stop<br/>타깃은 오디오 컴포넌트 |  |
| `K2Node_VariableSet_7` | Set travelLoopAudio |  |

</details>

### TryUseTeleportSkill

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["TryUseTeleportSkill"]
  n1["Branch"]
  n2["[ S]Try Use Teleport Skill<br/>타깃은 BPC Teleport Skill<br/>RELIABLE 서버에 리플리케이트됨 (소유 클라이언트인 경우)"]
  n3["Server Use Teleport Skill Logic<br/>타깃은 BPC Teleport Skill"]
  n0 -->|"then"| n1
  n1 -->|"then"| n3
  n1 -->|"else"| n2
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | TryUseTeleportSkill |  |
| `K2Node_CallFunction_0` | Get Owner<br/>타깃은 액터 컴포넌트 |  |
| `K2Node_CallFunction_1` | Has Authority<br/>타깃은 액터 |  |
| `K2Node_IfThenElse_0` | Branch |  |
| `K2Node_CallFunction_2` | [ S]Try Use Teleport Skill<br/>타깃은 BPC Teleport Skill<br/>RELIABLE 서버에 리플리케이트됨 (소유 클라이언트인 경우) |  |
| `K2Node_CallFunction_3` | Server Use Teleport Skill Logic<br/>타깃은 BPC Teleport Skill |  |

</details>

## 조회 범위의 한계

ObjectIterator로 발견했으나 MCP helper가 반환하지 않은 그래프입니다. delegate signature·컴파일된 중간 그래프·macro·애니메이션 내부 그래프가 섞일 수 있습니다.

- `InitializeTeleportSkillData`: Graph 'InitializeTeleportSkillData' not found in Blueprint.
- `ReceiveBeginPlay`: Graph 'ReceiveBeginPlay' not found in Blueprint.
- `[M]OnTeleportProjectileExpired`: Graph '[M]OnTeleportProjectileExpired' not found in Blueprint.
- `[M]OnTeleportProjectileFired`: Graph '[M]OnTeleportProjectileFired' not found in Blueprint.
- `[M]OnTeleportProjectileImpact`: Graph '[M]OnTeleportProjectileImpact' not found in Blueprint.
- `[M]OnTeleportSuccess`: Graph '[M]OnTeleportSuccess' not found in Blueprint.
- `[S]TryUseTeleportSkill`: Graph '[S]TryUseTeleportSkill' not found in Blueprint.

[원본 MCP 스냅샷](../../../Evidence/blueprints-070.json)
