# 로컬 에임 어시스트

2026-11-10 추가. 합의는 [결정 문서](../../../Decision/002-aim-assist.md)에 있습니다.

## 사용과 설정

Ctrl+Shift+Alt+F10으로 전환합니다. 기본 OFF이며 화면에 상태를 2초 표시합니다. ON 상태에서 좌클릭을 누르면 살아 있는 적의 가슴으로 카메라와 조준 방향을 보간합니다. 탄약과 발사 가능 상태는 검사하지 않습니다. Shipping에서도 동일한 경로입니다.

`Project Settings > Game > Aim Assist > Convergence Time`은 고정 목표에 대한 초기 각도 오차의 90%가 줄어드는 시간(초)이며 기본 0.3입니다. 설정 원본은 native `UAimAssistSettings` 기본값 및 `DefaultGame.ini`의 `[/Script/ShootingArena.AimAssistSettings] ConvergenceTime`입니다. JsonAssetSync/DT/DA와 연결하지 않습니다. 토글 상태는 저장하지 않습니다.

## 역할·의존 관계

- `AMyPlayerController`의 native 기본 서브오브젝트 `UAimAssistComponent`를 통해 `BP_QuakePlayerController`가 상속합니다. Blueprint 에셋 수정 없이 연결합니다.
- 입력 처리 후 F10의 최초 눌림과 Ctrl/Shift/Alt 상태를 검사합니다. 좌우 보조키 모두 허용합니다.
- 회전 갱신 시 매 프레임 후보를 재선택합니다. 가슴을 카메라의 constrained view에 투영하고 화면 중심과 픽셀 거리로 정렬합니다. 별도 거리·각도 제한이 없습니다. 같은 거리이면 객체 ID로 동률을 해소합니다.
- `BP_ShooterBase.GetHeart`의 `heartLocation`을 읽습니다. 사용자가 PlayerState.Team 미사용과 자신 이외의 모든 Pawn이 적임을 확인했습니다. 팀 데이터는 읽지 않습니다. PlayerState가 없는 AI도 후보이며 관전자 Pawn 또는 관전자 PlayerState가 있으면 제외합니다.
- 생존은 기존 `UShooterDisplayLibrary::IsShooterAlive`를 통해 LifeInfo와 CharacterPool의 비활성 상태를 확인합니다. 관전자 PlayerState와 숨겨진 Pawn은 제외합니다. 가슴까지 Visibility trace가 막히면 후보에서 제외합니다.
- 로컬 플레이어이며 Playing 상태, 자신의 Pawn을 관측 중, FP_Camera 존재, 생존, 커서 숨김, 시점 입력 허용, 게임 viewport 포커스가 모두 필요합니다. 일시정지·메뉴·사망·관전·포커스 상실 시 중지하고 정상 복귀 후 현재 좌클릭 상태에 따라 재개합니다.
- 보간이 적용되는 프레임에만 `RotationInput`을 제거합니다. ignore-look-input 스택을 수정하지 않으므로 다음 프레임의 대상 상실·버튼 해제 시 입력이 자연스럽게 복귀합니다. 이동·발사 입력은 유지합니다.
- `UpdateRotation`에서 ControlRotation을 변경한 뒤 기본 카메라 제한/FaceRotation을 적용합니다. 기존 `BP_PlayerShooter.GetAim → GetBaseAimRotation`과 사격 복제 경로를 사용합니다. 선행 조준과 명중 판정 변경은 없습니다.
- 토글은 컨트롤러 수명 동안만 유지합니다. 리스폰/Seamless travel에서 같은 컨트롤러를 유지하면 ON도 유지하며, 새 컨트롤러 생성 시 OFF입니다. 일반 Slate 위젯을 사용하여 Shipping에서 debug-message 제한의 영향을 받지 않습니다.

## 검증 범위

설치형 UE 5.6.1 (`5.6.1-44394996`, `D:/UE_5.6`)에서 최종 Editor Development 빌드와 게임 Win64 Shipping 컴파일·링크가 성공했습니다. Shipping 실행 파일 빌드는 Cook·패키징이나 서버 빌드 검증이 아닙니다. 사용자가 확인한 Git 소스 빌드 엔진은 이 컴퓨터에 없으므로 서버/Cook/패키징 실행은 별도 환경에서 확인해야 합니다.

MCP로 실행한 `ShootingArena.AimAssist.ConvergenceAndEligibility`와 `ShootingArena.AimAssist.BlueprintBridge`가 모두 성공했습니다(2026-11-10 20:23 KST). 수학 자동화는 수렴 시간, 30/60/144 FPS, yaw 경계, 정지 프레임, 잘못된 설정과 화면 경계를 확인했습니다. Blueprint bridge는 실제 파생 컨트롤러의 컴포넌트 상속과 OFF 기본값, GetHeart, 기존 GetAim의 ControlRotation 사용, 화면 거리 우선 선택과 즉시 재선택, 엄폐와 즉시 재획득, 사망·관전자 제외, PlayerState 부재, 100m 후보 선택을 격리된 실제 Blueprint 월드에서 확인했습니다.

초기 테스트의 격리 월드에서 Blueprint 실행 허용이 없어 getter가 생략되는 문제와, 실행 허용을 possession 전부터 켜 기존 UI 생성까지 실행된 문제를 발견했습니다. 테스트에서 possession 후 getter 실행만 허용하도록 수정한 최종 실행은 오류 없이 통과했습니다. 일반 플레이의 BeginPlay나 Blueprint 에셋은 변경하지 않았습니다.

실제 키보드 단축키·ON/OFF 표시·좌클릭 중 마우스 차단·메뉴/포커스 전환의 수동 플레이, 모든 무기의 네트워크 사격과 보간 감각은 아직 실행 검증하지 않았습니다. 이 경로들은 소스 검토와 컴파일 범위이며 자동화 성공으로 대체하지 않습니다. Live Coding을 사용하지 않았으며 완료 시 에디터 응답 상태를 확인했습니다. [검증 기록](evidence/aim-assist.json)을 참조합니다.

전체 저장소 수집은 수행하지 않으며 기존 기준선 원본을 보존합니다.
