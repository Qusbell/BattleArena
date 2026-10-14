# 버프 종료 예고음

`BPC_Buff`의 기존 `Set Buff Presentation` / `Clear Buff Presentation` 호출을 사용한다.
`ApplyBuff`, `ApplyDroppedBuff`, `OnBuffTimerFinished`에 별도 예고음 노드를 추가하지 않는다.

서버는 현재 Buff DT 행의 예고음 설정을 기존 `EndServerTime`과 함께 복제한다.
각 클라이언트는 컴포넌트의 기존 0.1초 Tick에서 공유 Remaining Time을 읽는다.
남은 `(4, 5]`, `(3, 4]`, `(2, 3]`, `(1, 2]`, `(0, 1]` 구간에서 각각 한 번 재생한다.
0초에는 재생하지 않는다. 프레임 저하나 늦은 복제 시 지나간 구간은 건너뛴다.
시간 보정으로 남은 시간이 증가해도 소비한 구간은 다시 재생하지 않는다.

DT의 `WarningSound`, `WarningLocalEnabled`, `WarningWorldEnabled`,
`WarningLocalVolume`, `WarningWorldVolume`, `WarningAttenuation`을 사용한다.
볼륨 0/음수/비정상 값 또는 Sound None이면 해당 출력만 생략한다.
Local은 현재 보유 폰을 조종하는 로컬 플레이어의 2D 음이다.
World는 각 클라이언트에서 현재 보유 캐릭터의 Mesh(없으면 Root)에 부착한다.
Both는 보유자에게 2D+3D가 의도적으로 겹친다. 다른 플레이어는 3D만 듣는다.
WarningAttenuation None이면 반경 300cm 및 추가 감쇠 거리 1200cm의 기본 3D 감쇠를 사용한다.

만료 시각/행/태그가 달라지면 재생 세대를 갱신하고 이전 오디오를 정리한다.
동일한 설정 호출은 이미 소비한 구간을 초기화하지 않는다.
드롭 전달이나 지속시간 단축 시 적용 전에 이미 지난 예고 시점은 재생하지 않는다.
예를 들어 남은 4.5초를 넘겨받으면 4/3/2/1초 예고만 이어간다.
같은 초 구간에서 즉시 갱신한 경우 같은 예고음의 연속 중복을 방지한다.
사망/해제/만료/EndPlay에서 두 오디오 핸들을 정리한다.
매 예고음마다 새 RPC를 보내지 않으며, 실제 버프 종료 타이머나 효과 수치는 변경하지 않는다.

## 테스트 모드

PIE 및 콘솔을 사용할 수 있는 Development/Test 빌드에서 다음 명령을 사용한다.
멀티플레이의 별도 프로세스는 각 클라이언트에서 설정한다.

| 콘솔 명령 | 출력 |
| --- | --- |
| `Buff.WarningSound.Mode -1` | DT 체크박스 사용 (기본) |
| `Buff.WarningSound.Mode 0` | None |
| `Buff.WarningSound.Mode 1` | Local Only |
| `Buff.WarningSound.Mode 2` | World Only |
| `Buff.WarningSound.Mode 3` | Both |

모드 변경으로 같은 초를 다시 재생하지 않는다. 다음 구간부터 새 모드를 적용한다.
모드를 끄면 해당 경로의 재생 중 오디오도 정리한다. 볼륨은 DT 값을 그대로 사용한다.
획득음의 별도 `Buff.PickupSound.Mode`에는 영향을 주지 않는다.

`Buff.WarningSound.Debug 1`은 각 구간의 보유자, 남은 시간, 실제 생성된 Local/World 오디오를 로그에 출력한다.
확인 후 `Buff.WarningSound.Debug 0`으로 끈다 (기본 0).

임시 확인용 사운드: `SoundPack_SDM/ETC/hit_Pitch_Changer__1_` (약 0.216초).
Buff DT 네 행에 WarningSound를 지정하고 Local ON / World OFF / 두 볼륨 1.0으로 설정했다.
최종 사운드는 기획에서 DT만 교체하면 된다.

자동 검사: `Automation RunTests ShootingArena.Buff.WarningSound`.
구간별 1회, 0초, 프레임 점프, 시간 역행, 4개 출력 조합, DT 읽기,
중복 적용/갱신/사망/해제/재획득을 검사한다. 실제 청취 검사는 별도로 진행한다.

## 2026-10-14 확인 결과

### 사망 검증 결과 정정

사망 테스트 중 Blueprint 런타임 오류가 발생했다. 따라서 아래 오디오 정리 관찰만으로
사망·UI를 포함한 전체 회귀 검증이 통과했다고 판단하면 안 된다.

- 테스트 스크립트 `Saved/BuffWarning/verify_lifecycle_pie.py:20`에서
  `apply_damage(..., 100000.0, None, None, ...)`으로 공격자와 피해 원인 없이 사망을 유발했다.
- `Saved/Logs/ShootingArena.log`의 2026-10-14 10:29:48 및 10:41:39 UTC에
  BPC_Life의 `InstigatedBy → Get Controlled Pawn → Get Component by Class → GetSelectedItem`
  경로에서 None 참조 오류가 기록됐다.
- 같은 시각 서버 월드 `UEDPIE_0`의 BP_QuakePlayerController에서
  `LifeAndWeapon` / `deathScreen` None 참조 오류가 기록됐다.
  현재 그래프는 해당 Toggle 함수에서 로컬 컨트롤러 확인 없이 위젯 생성을 시도하고,
  생성 반환값을 확인하지 않고 Add to Viewport / Set Visibility로 진행한다.
  ClientShowDeathScreen에서도 deathScreen을 확인하지 않고 InitDeathUI를 호출한다.
- 버프 사망 시 오디오 정리 자체는 관찰됐지만, 위 런타임 오류가 남아 있으므로
  사망 통합 검증은 미완료다. 유효한 공격자를 사용하는 재검사와 서버/UI 경로 검증이 필요하다.
- 2026-10-14 오류 조사에서는 게임 소스와 Blueprint를 수정하지 않았다.
  조사 시 PIE는 종료 상태였고 dirty content package는 없었다.

### 후속 수정 및 재검사 — 2026-10-14

사용자 요청으로 아래 두 Blueprint에 참조 검사를 추가하고 저장했다. 기존 노드 위치는 유지했다.

- `BPC_Life.EventGraph`: InstigatedBy → Controlled Pawn → Inventory를 순서대로 Is Valid 검사한다.
  하나라도 없으면 기존 DamageWeaponData=None 경로로 합류한다. 피해 적용과 사망 처리는 계속한다.
- `BP_QuakePlayerController.Toggle Player HUD`, `Toggle Death UI`: 로컬 플레이어 컨트롤러에서만
  실행하고, 생성한 위젯이 유효할 때만 Add to Viewport 및 후속 표시 처리를 수행한다.
- `ClientShowDeathScreen`: 로컬 플레이어 확인 후 처리하며 유효한 deathScreen에만 InitDeathUI를 호출한다.
- 재검사 중 드러난 `Client_AddKillLog`의 동일한 LifeAndWeapon None 문제에도 로컬 플레이어 확인과
  위젯 Is Valid 검사를 추가했다. 서버에서는 UI 접근을 생략한다.
- 일반 사망 검증 스크립트는 유효한 다른 플레이어 컨트롤러/캐릭터를 공격자와 피해 원인으로 넘기도록 수정했다.

전용 서버 + 2클라이언트 Portal PIE의 최종 재검사:

- 공격자 없는 ApplyDamage, 정상 공격자의 ApplyDamage에서 피해가 적용됐다.
- Pawn 없는 컨트롤러는 OwnerTakeDamage 이벤트 직접 호출로 참조 검사와 피해 적용을 확인했다.
- 유효한 공격자의 사망도 OwnerTakeDamage 이벤트 직접 호출로 확인했다. 무기 데이터 참조가 유지됐고,
  사망자 클라이언트에서 HUD 숨김/사망 UI 표시를 관찰했다.
- 공격자 없는 치명타는 ApplyDamage로 확인했다. 두 사망 모두 버프 Remaining Time이 즉시 0이 됐다.
- 서버에서는 HUD/사망 위젯을 만들지 않았고, 양쪽 클라이언트의 표시/숨김 및 위젯 재사용이 정상 작동했다.
- 최종 로그 구간(11:11:32.922 UTC의 FINAL regression BEGIN 표식부터)에서 PIE Blueprint 런타임 오류 0건.
  정확한 구간은 로그 표식을 기준으로 판정한다. 기존 오류 기록은 지우지 않았다.
- 실제 무기 입력/투사체에 의한 사망 및 패키징 빌드는 이번 수정에서 확인하지 않았다.

백업, 변경 전후 그래프, 재현 스크립트와 결과:
`Saved/BuffWarning/RuntimeErrorFix_20261014/` (Git 제외).
최종 결과 파일은 `final_runtime_report.json`이다. 전체 게임 무오류나 버프 획득 렉 해결을 뜻하지 않는다.
PIE 종료 후 원래 MainMenu_Level, 클라이언트 수 1, 백그라운드 CPU 설정을 복원했다. Dirty content package는 없었다.

- C++ Live Coding 및 BPC_Buff 컴파일 성공.
- `ShootingArena.Buff.WarningSound`, 기존 `ShootingArena.Buff.Presentation` 자동 검사 성공.
- Portal 맵의 전용 서버 + 2개 클라이언트 PIE에서 실제 생성된 오디오 컴포넌트를 수집해 확인했다.

| 모드 | 보유 클라이언트 Local | 보유 클라이언트 World | 주변 클라이언트 World |
| --- | --- | --- | --- |
| None | 0 | 0 | 0 |
| Local Only | 5 | 0 | 0 |
| World Only | 0 | 5 | 5 |
| Both | 5 | 5 | 5 |

모든 재생은 남은 5~1초 안에서 발생했고 0초 추가 재생은 없었다.
World 사운드의 부모는 보유 캐릭터 Mesh였고, PIE에서 보유자를 이동시켰을 때 부모와 위치 차이는 0이었다.
별도 실행에서는 거리 밖 클라이언트의 World 생성이 감쇠 범위에 따라 생략되는 것도 확인했다.

실제 피해로 보유자 사망을 발생시킨 뒤 서버 표시 Remaining Time이 즉시 0이 됐다.
동일한 만료 시각을 다른 플레이어의 ApplyDroppedBuff 경로에 전달했을 때 남은 약 4.5초를 이어받고
World 출력이 새 보유자 Mesh로 이동했다. 최종 보완 후 2인 PIE 재검사에서 지나간 5초 예고를 반복하지 않고
4초 시점부터 이어 재생했다. 이후 OnBuffTimerFinished를 호출한 조기 종료 후 추가 출력은 없었다.
이 검사는 물리 드롭 액터와의 충돌 획득을 대신해 기존 드롭 적용 API를 직접 호출했다.
PIE 종료 후 재시작에서도 모든 새 폰의 Remaining Time은 0이었고, 이전 예고음 오디오가 남지 않았다.
테스트 후 PIE를 종료하고 플레이어 수/오디오 장치/백그라운드 CPU 설정과 원래 에디터 맵을 복원했다.

상세 PIE 기록과 변경 전 DT 백업은 Git에서 제외되는 `Saved/BuffWarning`에 있다.
출력 컴포넌트와 로그를 확인한 결과이며, 최종 사운드 청취 및 패키징 빌드 검사는 별도로 남는다.
