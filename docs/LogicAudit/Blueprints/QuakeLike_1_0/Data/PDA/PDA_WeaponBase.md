# PDA_WeaponBase

에셋: `/Game/QuakeLike_1_0/Data/PDA/PDA_WeaponBase`

부모: `/Script/Engine.BlueprintGeneratedClass'/Game/QuakeLike_1_0/Data/PDA/PDA_HeldItem.PDA_HeldItem_C'`

수집일: 2026-10-12. MCP에서 읽은 그래프·핀 연결의 자동 전사입니다. 상속된 그래프는 부모 문서를 함께 확인합니다.

정적 루트 참조 도달: 있음. 실행 여부 또는 삭제 가능 여부의 확정이 아닙니다.

컴포넌트 목록은 이 BP가 직접 정의한 SCS만 포함합니다. 부모·네이티브 생성 컴포넌트, 인스턴스 설정은 별도입니다.

## 변수와 기본값

| 이름 | 타입 | CDO 기본값 |
|---|---|---|
| `weaponType` | `E_WeaponType` | &lt;E_WeaponType.HIT_SCAN: 0&gt; |
| `damage` | `real` | 0.0 |
| `range` | `real` | 0.0 |
| `fireRate` | `real` | 0.0 |
| `spread` | `real` | 0.0 |
| `projectileClass` | `BP_ProjectileBase_C` | None |
| `projectileSpeed` | `real` | 0.0 |
| `damageJudgmentOffset` | `real` | 0.0 |
| `explosionRadius` | `real` | 0.0 |
| `explosionDamage` | `real` | 0.0 |
| `selfDamageMultiplier` | `real` | 0.0 |
| `selfKnockbackMultiplier` | `real` | 0.0 |
| `otherKnockbackMultiplier` | `real` | 0.0 |
| `pellet` | `int` | 0 |
| `NSFireFX` | `NiagaraSystem` | None |
| `fireFX` | `ParticleSystem` | None |
| `fireSound` | `SoundBase` | None |
| `pierceMultiplier` | `real` | 0.0 |
| `zoomFov` | `real` | 0.0 |
| `interpSpeed` | `real` | 0.0 |
| `pierceMax` | `int` | 0 |
| `traceChannel` | `ETraceTypeQuery` | &lt;TraceTypeQuery.TRACE_TYPE_QUERY1: 0&gt; |
| `projectileSound` | `SoundBase` | None |
| `NSProjectileDestroyEffect` | `NiagaraSystem` | None |
| `projectileDestroyEffect` | `ParticleSystem` | None |
| `emptyAmmoSound` | `SoundBase` | None |
| `fallOffStart` | `real` | 0.0 |
| `minDamageMultiplier` | `real` | 0.0 |
| `hitscanBeamEffect` | `NiagaraSystem` | None |
| `NSHitscanHitEffect` | `NiagaraSystem` | None |
| `hitscanHitEffect` | `ParticleSystem` | None |
| `reloadTime` | `real` | 0.0 |
| `explosionEffectScale` | `real` | 0.0 |
| `railGunSonicBombEffect` | `NiagaraSystem` | None |
| `railGunTrailEffect` | `NiagaraSystem` | None |
| `chHitSound` | `SoundBase` | None |
| `fireVolumeMultiplier` | `real` | 1.0 |
| `emptyAmmoVolumeMultiplier` | `real` | 1.0 |
| `chHitVolumeMultiplier` | `real` | 1.0 |
| `projectileVisualForwardOffset` | `real` | 100.0 |
| `projectileDestroyVolumeMultiplier` | `real` | 1.0 |
| `selfDamage` | `real` | 40.0 |
| `lockTimeMultiplier` | `real` | 1.0 |
| `rocketJumpZOffset` | `real` | 0.0 |
| `explosionPhysicsForce` | `real` | 0.0 |
| `NSAfterSmokeEffect` | `NiagaraSystem` | None |
| `afterSmokeEffect` | `ParticleSystem` | None |
| `smokeEffectSocketName` | `name` | None |
| `crosshairSetting` | `ST_CrosshairSetting` | &lt;Struct 'ST_CrosshairSetting' {}&gt; |
| `zoomAimSpeedMultiplier` | `real` | 0.7 |
| `name` | `string` |  |
| `Icon` | `Texture2D` | None |
| `WeaponHUDColor` | `LinearColor` | &lt;Struct 'LinearColor' {r: 0.000000, g: 0.000000, b: 0.000000, a: 0.000000}&gt; |
| `MachineGunBeamLifeTime` | `real` | 12000.0 |
| `RailGunBeamLifeTime` | `real` | 0.2 |

## 그래프 목록

| 그래프 | 전체 노드 수 |
|---|---|
| EventGraph | 0 |

## 그래프별 실행 연결

실선은 출력 exec 핀의 실제 연결입니다. 여러 분기는 출력 핀 이름을 표시합니다. 함수 호출·delegate·타이머의 다음 그래프 실행은 이 그림에서 자동으로 이어 붙이지 않습니다.

순수 계산 노드 및 데이터 핀은 아래 목록과 원본 JSON을 확인합니다. 연결되지 않은 이벤트도 표시되므로, 노드가 있다는 이유만으로 실행된다고 판단하면 안 됩니다.

### EventGraph

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|

</details>


[원본 MCP 스냅샷](../../../../Evidence/blueprints-035.json)
