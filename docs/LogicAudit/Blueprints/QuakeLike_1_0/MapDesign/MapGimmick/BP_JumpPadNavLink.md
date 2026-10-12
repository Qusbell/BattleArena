# BP_JumpPadNavLink

에셋: `/Game/QuakeLike_1_0/MapDesign/MapGimmick/BP_JumpPadNavLink`

부모: `/Script/CoreUObject.Class'/Script/AIModule.NavLinkProxy'`

수집일: 2026-10-12. MCP에서 읽은 그래프·핀 연결의 자동 전사입니다. 상속된 그래프는 부모 문서를 함께 확인합니다.

정적 루트 참조 도달: 없음. 실행 여부 또는 삭제 가능 여부의 확정이 아닙니다.

컴포넌트 목록은 이 BP가 직접 정의한 SCS만 포함합니다. 부모·네이티브 생성 컴포넌트, 인스턴스 설정은 별도입니다.

## 직접 정의한 컴포넌트

| 이름 | 클래스 | 부모 컴포넌트 |
|---|---|---|
| `ChildActor` | `ChildActorComponent` | `` |

## 변수와 기본값

| 이름 | 타입 | CDO 기본값 |
|---|---|---|
| `TargetRight` | `Vector` | &lt;Struct 'Vector' {x: 0.000000, y: 0.000000, z: 0.000000}&gt; |

## 그래프 목록

| 그래프 | 전체 노드 수 |
|---|---|
| EventGraph | 11 |
| UserConstructionScript | 22 |

## 그래프별 실행 연결

실선은 출력 exec 핀의 실제 연결입니다. 여러 분기는 출력 핀 이름을 표시합니다. 함수 호출·delegate·타이머의 다음 그래프 실행은 이 그림에서 자동으로 이어 붙이지 않습니다.

순수 계산 노드 및 데이터 핀은 아래 목록과 원본 JSON을 확인합니다. 연결되지 않은 이벤트도 표시되므로, 노드가 있다는 이유만으로 실행된다고 판단하면 안 됩니다.

### EventGraph

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["Receive Smart Link Reached 이벤트"]
  n1["Cast To Character"]
  n2["Suggest Projectile Velocity Custom Arc"]
  n3["Launch Character<br/>타깃은 캐릭터"]
  n4["BeginPlay 이벤트"]
  n5["SpawnActor BPA YJS Normal Jump Pad Nav"]
  n6["Set Actor Location<br/>타깃은 액터"]
  n0 -->|"then"| n1
  n1 -->|"then"| n2
  n2 -->|"then"| n3
  n4 -->|"then"| n5
  n5 -->|"then"| n6
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_Event_3` | Receive Smart Link Reached 이벤트 |  |
| `K2Node_DynamicCast_0` | Cast To Character |  |
| `K2Node_CallFunction_0` | Suggest Projectile Velocity Custom Arc | OverrideGravityZ=0.000000; ArcParam=0.500000; ReturnValue=false |
| `K2Node_CallFunction_1` | Get Actor Location<br/>타깃은 액터 |  |
| `K2Node_CallFunction_2` | Launch Character<br/>타깃은 캐릭터 | bXYOverride=true; bZOverride=true |
| `K2Node_PromotableOperator_0` | vector + vector | B=0.000000,0.000000,300.000000 |
| `K2Node_Event_2` | BeginPlay 이벤트 |  |
| `K2Node_SpawnActorFromClass_2` | SpawnActor BPA YJS Normal Jump Pad Nav | Class=/Game/QuakeLike_1_0/MapDesign/MapGimmick/BPA_YJS_Normal_JumpPad_Nav.BPA_YJS_Normal_JumpPad_Nav_C; CollisionHandlingOverride=AlwaysSpawn; TransformScaleMethod=MultiplyWithRoot |
| `K2Node_CallFunction_7` | Set Actor Location<br/>타깃은 액터 | bSweep=false; bTeleport=false; ReturnValue=false |
| `K2Node_CallFunction_8` | Get Actor Location<br/>타깃은 액터 |  |
| `K2Node_CallFunction_9` | Get Actor Transform<br/>타깃은 액터 |  |

</details>

### UserConstructionScript

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["Construction Script"]
  n1["Line Trace By Channel"]
  n2["Branch"]
  n3["Set members in Navigation Link"]
  n4["Set Smart Link Enabled<br/>타깃은 내비 링크 프록시"]
  n5["Set TargetRight"]
  n0 -->|"then"| n1
  n1 -->|"then"| n2
  n2 -->|"then"| n3
  n3 -->|"then"| n5
  n5 -->|"then"| n4
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_FunctionEntry_0` | Construction Script |  |
| `K2Node_CallFunction_0` | Transform Location |  |
| `K2Node_CallFunction_1` | Get Actor Transform<br/>타깃은 액터 |  |
| `K2Node_CallFunction_2` | Line Trace By Channel | TraceChannel=TraceTypeQuery1; bTraceComplex=false; DrawDebugType=ForDuration; OutHit_bBlockingHit=false; OutHit_bInitialOverlap=false; OutHit_Time=0.0; OutHit_Distance=0.0; OutHit_ImpactPoint=0, 0, 0; OutHit_Normal=0, 0, 0; OutHit_ImpactNormal=0, 0, 0; OutHit_HitBoneName=None; OutHit_BoneName=None; OutHit_HitItem=0; OutHit_ElementIndex=0; OutHit_FaceIndex=0; OutHit_TraceStart=0, 0, 0; OutHit_TraceEnd=0, 0, 0; bIgnoreSelf=true; TraceColor=(R=1.000000,G=0.000000,B=0.000000,A=1.000000); TraceHitColor=(R=0.000000,G=1.000000,B=0.000000,A=1.000000); DrawTime=5.000000 |
| `K2Node_PromotableOperator_0` | vector - vector | B=0.000000,0.000000,2000.000000 |
| `K2Node_IfThenElse_0` | Branch |  |
| `K2Node_Knot_0` | 경유 노드 |  |
| `K2Node_CallFunction_5` | Inverse Transform Location |  |
| `K2Node_Knot_1` | 경유 노드 |  |
| `K2Node_VariableGet_2` | Get PointLinks |  |
| `K2Node_GetArrayItem_0` | GET | Dimension 1=0 |
| `K2Node_SetFieldsInStruct_0` | Set members in Navigation Link | Direction=BothWays |
| `K2Node_Knot_3` | 경유 노드 |  |
| `K2Node_CallFunction_8` | Set Smart Link Enabled<br/>타깃은 내비 링크 프록시 | bEnabled=true |
| `K2Node_CallFunction_9` | Break Vector |  |
| `K2Node_CallFunction_10` | Make Vector |  |
| `K2Node_PromotableOperator_3` | float + float | B=15.000000 |
| `K2Node_CallFunction_16` | Make Vector | X=500.000000; Y=0.0; Z=1000.000000 |
| `K2Node_Knot_4` | 경유 노드 |  |
| `K2Node_Knot_5` | 경유 노드 |  |
| `K2Node_Knot_6` | 경유 노드 |  |
| `K2Node_VariableSet_2` | Set TargetRight | Output_Get=0, 0, 0 |

</details>

## 조회 범위의 한계

ObjectIterator로 발견했으나 MCP helper가 반환하지 않은 그래프입니다. delegate signature·컴파일된 중간 그래프·macro·애니메이션 내부 그래프가 섞일 수 있습니다.

- `ExecuteUbergraph_BP_JumpPadNavLink`: Graph 'ExecuteUbergraph_BP_JumpPadNavLink' not found in Blueprint.
- `ReceiveBeginPlay`: Graph 'ReceiveBeginPlay' not found in Blueprint.
- `ReceiveSmartLinkReached`: Graph 'ReceiveSmartLinkReached' not found in Blueprint.
- `UserConstructionScript_MERGED`: Graph 'UserConstructionScript_MERGED' not found in Blueprint.

[원본 MCP 스냅샷](../../../../Evidence/blueprints-245.json)
