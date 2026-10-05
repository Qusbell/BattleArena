# 아이템 생성음 / 획득음 설정

설정 위치: 콘텐츠 브라우저 `QuakeLike_1_0/Data/Item/DropTable`의 아래 네 DT.

- `DT_HeldItemTable`
- `DT_BuffTable`
- `DT_LifeItemTable`
- `DT_AmmoItemTable`

| 필드 | 용도 |
| --- | --- |
| `SpawnSound` | 아이템 생성 성공 시 재생할 사운드. 최초 생성과 재생성에 적용. |
| `SpawnVolume` | 생성음 볼륨 배율. 기본 1, 0이면 음소거. |
| `PickupSound` | 획득 성공 시 먹은 플레이어에게만 한 번 재생할 사운드. |
| `PickupVolume` | 획득음 볼륨 배율. 1은 원래 음량, 0이면 음소거. |

Life/Ammo의 기존 `pickupSound`, `pickupVolume`은 같은 역할이며 기존 표기와 값을 유지한다.
사운드 타입은 `SoundBase`이므로 Sound Cue와 Sound Wave를 지정할 수 있다.
`None`이면 해당 사운드만 생략하며 아이템 생성/획득은 정상 처리한다.

## 타이밍과 네트워크

생성음은 실제 `Spawn`이 유효한 아이템을 반환하고 `AssignItem` 등록/바인딩이 끝난 직후 재생한다.
최초 대기가 0인 경우에도 같은 경로를 사용하며, 생성 실패 시 생성음을 재생하지 않는다.
기존 `PreSpawnFeedbackLeadTime`은 사전 FX에만 적용한다. FX를 생성음과 함께 중복 출력하지 않는다.
스폰 대기시간, Respawn Ring과 카메라 회전은 변경하지 않는다.
재생 위치와 `SA_SpawnSound` 감쇠 설정을 유지하며 서버에서 선택한 사운드와 볼륨을 기존 RPC로 멀티캐스트한다.

획득음은 서버의 기존 `ApplyItemEffect` 성공 경로에서
`BP_QuakePlayerController.Client_PlayPickupSound`로 전달한다. 획득 실패에는 재생하지 않는다.
무기 맵 획득의 자동 장착음만 생략해 중복 재생을 방지하고, 초기 지급/이후 무기 교체의 DA 장착음은 유지한다.

## 기존 데이터 이전 결과

- Buff/Ammo: 기존 공용 Sound를 생성음과 획득음에 각각 복사. 이후 서로 다르게 변경 가능.
- HeldItem: 기존 생성 Sound 유지. 획득음 초기값은 각 무기 DA의 기존 장착음/볼륨에서 복사.
- Life: 기존 획득음 유지. 생성음은 기존 설정이 없어 `None`; 사용할 생성음을 직접 지정해야 한다.
- HeldItem 생성음도 기존대로 `None`이므로 생성음을 듣고 싶으면 지정해야 한다.
- Buff의 기존 획득 볼륨 0은 초기 이전 시 보존했으나, 1차 검수 수정에서 네 행 모두 `PickupVolume=1.0`으로 설정했다.
- 메시, 버프 수치, VFX, 지속시간, 사망 드롭 규칙과 맵 배치는 변경하지 않는다.

## 검증

에디터 빌드 및 아래 자동 테스트 통과:

- `ShootingArena.Items.SpawnAndPickupSound`: 네 DT의 전체 25개 행, 서로 다른 생성/획득음 및 볼륨, None 조합, 음소거, 없는 행, RPC 플래그.
- `ShootingArena.Items.SpawnSoundTiming`: 저장된 Blueprint의 사전 FX/실제 생성음 분리, 유효 아이템 확인, 서버 권한, 현재 행 공유 연결 회귀 검사.
- `ShootingArena.Buff.Presentation`: 기존 버프 남은 시간/VFX/드롭 시간 회귀 검사.
- `ShootingArena.Spawner.SelectionModes`: 기존 순환 선택 방식 회귀 검사.

자동 테스트는 소리를 실제로 듣는 검사가 아니다. PIE 2인에서 가까이/멀리의 생성음과
먹은 사람에게만 들리는 획득음을 청취하고, 무기 자동 장착의 중복음이 없는지 확인한다.
원본 백업/실행 로그/자동 테스트 보고서는 Git에서 제외되는 `Saved/ItemSoundMigration`에 있다.

## 적용 도구

`Tools/Items/migrate_item_sounds.py`는 이번 에셋 이전에 사용한 명시적 일회성 도구다.
게임이나 에디터 시작 시 자동 실행되지 않으며, 적용된 DT에서는 재실행하지 않는다.
`ItemSoundMigrationLibrary`는 기존 EditorTools 모듈의 에디터 전용 도구이고 런타임 의존성은 없다.
