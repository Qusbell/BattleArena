# BP_TeleportSkillProjectile

에셋: `/Game/QuakeLike_1_0/Skill/BP_TeleportSkillProjectile`

부모: `/Script/Engine.BlueprintGeneratedClass'/Game/QuakeLike_1_0/HeldItem/Projectile/BP_ProjectileBase.BP_ProjectileBase_C'`

수집일: 2026-10-12. MCP에서 읽은 그래프·핀 연결의 자동 전사입니다. 상속된 그래프는 부모 문서를 함께 확인합니다.

정적 루트 참조 도달: 있음. 실행 여부 또는 삭제 가능 여부의 확정이 아닙니다.

컴포넌트 목록은 이 BP가 직접 정의한 SCS만 포함합니다. 부모·네이티브 생성 컴포넌트, 인스턴스 설정은 별도입니다.

## 변수와 기본값

| 이름 | 타입 | CDO 기본값 |
|---|---|---|
| `canTeleport` | `bool` | False |
| `skillData` | `PDA_TeleportSkill_C` | None |

## 그래프 목록

| 그래프 | 전체 노드 수 |
|---|---|
| EventGraph | 40 |
| UserConstructionScript | 1 |

## 그래프별 실행 연결

실선은 출력 exec 핀의 실제 연결입니다. 여러 분기는 출력 핀 이름을 표시합니다. 함수 호출·delegate·타이머의 다음 그래프 실행은 이 그림에서 자동으로 이어 붙이지 않습니다.

순수 계산 노드 및 데이터 핀은 아래 목록과 원본 JSON을 확인합니다. 연결되지 않은 이벤트도 표시되므로, 노드가 있다는 이유만으로 실행된다고 판단하면 안 됩니다.

### EventGraph

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["On Component Hit (Collision Component)"]
  n1["Branch"]
  n2["Server on Teleport Projectile Impact<br/>타깃은 BPC Teleport Skill"]
  n3["Destroyed 이벤트"]
  n4["부모: Destroyed"]
  n5["Branch"]
  n6["Server on Teleport Projectile Expired<br/>타깃은 BPC Teleport Skill"]
  n7["BeginPlay 이벤트"]
  n8["Set ProjectileGravityScale"]
  n9["Set Velocity in Local Space<br/>타깃은 발사체 무브먼트 컴포넌트"]
  n10["Set Life Span<br/>타깃은 액터"]
  n11["Ignore<br/>타깃은 BP Projectile Base"]
  n12["Is Valid"]
  n13["Is Valid"]
  n0 -->|"then"| n1
  n1 -->|"then"| n12
  n3 -->|"then"| n4
  n4 -->|"then"| n5
  n5 -->|"then"| n13
  n7 -->|"then"| n8
  n8 -->|"then"| n9
  n9 -->|"then"| n11
  n11 -->|"then"| n10
  n12 -->|"Is Valid"| n2
  n13 -->|"Is Valid"| n6
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_ComponentBoundEvent_0` | On Component Hit (Collision Component) | NormalImpulse=0, 0, 0 |
| `K2Node_VariableGet_0` | Get canTeleport |  |
| `K2Node_IfThenElse_0` | Branch |  |
| `K2Node_CallFunction_9` | Server on Teleport Projectile Impact<br/>타깃은 BPC Teleport Skill |  |
| `K2Node_Self_0` | 셀프-레퍼런스 |  |
| `K2Node_CallFunction_1` | Break Hit Result | bBlockingHit=false; bInitialOverlap=false; Time=0.0; Distance=0.0; Location=0, 0, 0; Normal=0, 0, 0; HitBoneName=None; BoneName=None; HitItem=0; ElementIndex=0; FaceIndex=0; TraceStart=0, 0, 0; TraceEnd=0, 0, 0 |
| `K2Node_Knot_0` | 경유 노드 |  |
| `K2Node_Knot_1` | 경유 노드 |  |
| `K2Node_Event_0` | Destroyed 이벤트 |  |
| `K2Node_CallParentFunction_0` | 부모: Destroyed |  |
| `K2Node_IfThenElse_1` | Branch |  |
| `K2Node_VariableGet_2` | Get canTeleport |  |
| `K2Node_CallFunction_5` | Get Owner<br/>타깃은 액터 |  |
| `K2Node_CallFunction_6` | Get Component by Class<br/>타깃은 액터 | ComponentClass=/Game/QuakeLike_1_0/Skill/BPC_TeleportSkill.BPC_TeleportSkill_C |
| `K2Node_CallFunction_11` | Get Owner<br/>타깃은 액터 |  |
| `K2Node_CallFunction_4` | Get Component by Class<br/>타깃은 액터 | ComponentClass=/Game/QuakeLike_1_0/Skill/BPC_TeleportSkill.BPC_TeleportSkill_C |
| `K2Node_CallFunction_7` | Server on Teleport Projectile Expired<br/>타깃은 BPC Teleport Skill |  |
| `K2Node_Self_1` | 셀프-레퍼런스 |  |
| `K2Node_CallFunction_8` | Get Actor Location<br/>타깃은 액터 |  |
| `K2Node_Event_1` | BeginPlay 이벤트 |  |
| `K2Node_VariableSet_0` | Set ProjectileGravityScale | ProjectileGravityScale=0.0; Output_Get=0.0 |
| `K2Node_CallFunction_0` | Set Velocity in Local Space<br/>타깃은 발사체 무브먼트 컴포넌트 | NewVelocity_Y=0.0; NewVelocity_Z=0.0 |
| `K2Node_CallFunction_2` | Set Life Span<br/>타깃은 액터 |  |
| `K2Node_PromotableOperator_0` | float / float |  |
| `K2Node_CallFunction_3` | Ignore<br/>타깃은 BP Projectile Base |  |
| `K2Node_VariableGet_4` | Get ProjectileMovement |  |
| `K2Node_Knot_2` | 경유 노드 |  |
| `K2Node_VariableGet_5` | Get skillData |  |
| `K2Node_VariableGet_7` | Get projectileSpeed |  |
| `K2Node_Knot_3` | 경유 노드 |  |
| `K2Node_Knot_4` | 경유 노드 |  |
| `K2Node_VariableGet_8` | Get maxDistance |  |
| `K2Node_Knot_5` | 경유 노드 |  |
| `K2Node_Knot_6` | 경유 노드 |  |
| `K2Node_Knot_7` | 경유 노드 |  |
| `K2Node_Knot_8` | 경유 노드 |  |
| `K2Node_Knot_9` | 경유 노드 |  |
| `K2Node_Knot_10` | 경유 노드 |  |
| `K2Node_MacroInstance_0` | Is Valid |  |
| `K2Node_MacroInstance_1` | Is Valid |  |

</details>

### UserConstructionScript

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["Construction Script"]
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | Construction Script |  |

</details>

## 조회 범위의 한계

ObjectIterator로 발견했으나 MCP helper가 반환하지 않은 그래프입니다. delegate signature·컴파일된 중간 그래프·macro·애니메이션 내부 그래프가 섞일 수 있습니다.

- `BndEvt__BP_TeleportSkillProjectile_Collision Component_K2Node_ComponentBoundEvent_0_ComponentHitSignature__DelegateSignature`: Graph 'BndEvt__BP_TeleportSkillProjectile_Collision Component_K2Node_ComponentBoundEvent_0_ComponentHitSignature__DelegateSignature' not found in Blueprint.
- `ReceiveBeginPlay`: Graph 'ReceiveBeginPlay' not found in Blueprint.
- `ReceiveDestroyed`: Graph 'ReceiveDestroyed' not found in Blueprint.

[원본 MCP 스냅샷](../../../Evidence/blueprints-070.json)
