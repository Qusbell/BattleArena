# 2026-11-12 병합 후 게임 경로 안전성 점검

기준은 `PJH_20261111_Language`의 추가 main 병합 `d1b6473ff`입니다. 실제 게임 BP와 C++의 로딩·로비·HUD·영상 표시 경계를 점검했습니다. MCP 서버와 Python 도구 소스는 수정하지 않았습니다.

## 확인한 문제와 변경

- `ULoadingScreenSubsystem`: 이전 전환의 페이드아웃 중 새 전환이 시작되면 기존 오버레이를 재사용하면서도 `bFadingOut`이 남았습니다. 다음 Tick에서 새 전환까지 제거하고 입력 잠금·BGM 상태를 해제할 수 있었습니다. `ShowLoadingScreen`에서 이전 페이드아웃을 취소하고 기존 오버레이의 불투명도를 복구합니다.
- `ULoadingScreenSubsystem`: 프로세스 공용 `PostLoadMapWithWorld` 콜백에서 시작 메뉴의 오버레이 생략 플래그를 먼저 처리했습니다. 다른 GameInstance의 맵 완료가 현재 인스턴스의 플래그를 소비하고 BGM을 복구할 수 있었습니다. 콜백 진입 시 유효한 자기 GameInstance의 월드인지 먼저 검사합니다.
- `/Game/QuakeLike_1_0/GameMode/BP_QuakeGameInstance`와 `/Game/QuakeLike_1_0/Character/BP_ShooterBase`: 시작 로그에 `GetDataTableRowFromName_OutRow`의 `Unknown structure` 오류가 있었습니다. MCP로 원본을 별도 백업하고 BP를 재컴파일·저장했습니다. 게임 규칙이나 그래프 연결을 수동 변경하지 않았습니다. 재시작한 에디터의 전체 시작 로그에서 해당 오류가 0건임을 확인했습니다. GameInstance 조회 함수 2개, 49개 노드의 그래프 조회 결과도 재저장 전후 동일합니다.

로딩 WBP와 하위 위젯의 언어 패치 제외 정책은 유지합니다. `/Game/QuakeLike_1_0/GameFlow/UI/Loading/WBP_LoadingScreen_Ref` 자체는 변경하지 않았습니다.

## 조사·검증 범위

- MCP 그래프 조회: 로딩 WBP EventGraph, GameInstance의 맵 캐릭터 조회 함수 2개, `WBP_MultiConnect`·`WBP_MultiLobby` EventGraph입니다. BP 노드·핀·실행 연결과 C++ 호출을 대조했습니다.
- BP 컴파일: 위 5개 BP와 `WBP_DeathScreen`, `WBP_PlayerHUD_sdm`, 총 7개입니다. 모두 UpToDate입니다. 플레이 중 조회가 거부된 HUD 2개는 플레이 종료 후 다시 검사했습니다.
- 새 영상 소스 3개는 `./Movies/BioTech.mp4`, `./Movies/MiliTech.mp4`, `./Movies/Title.mp4`를 가리키며 실제 파일이 있습니다. 병합된 파일 40개의 내용·LFS 해시는 앞선 추가 병합에서 확인했습니다. 실제 디코딩·패키지 재생 검증과 구분합니다.
- `ShootingArena.Loading.TransitionSafety`는 월드 소유권, 시작 생략 플래그, 페이드아웃 중 재진입과 명시적 정리를 검사합니다. PIE·실제 맵 전환·BeginPlay를 실행하지 않습니다.
- Editor Development 빌드(81.83초), 게임 Shipping 빌드(103.54초)가 통과했습니다. 기존 JsonAssetSync의 DataValidation 의존성·include order·BGM 테스트 API 폐기 경고는 이번 범위에서 변경하지 않았습니다.
- `ShootingArena.Loading.TransitionSafety` 1개와 `ShootingArena.Localization` 5개가 모두 성공했습니다. 자동 테스트 대기를 위해 일시적으로 해제했던 백그라운드 CPU 절약 옵션은 원래 값으로 복구했고 설정 파일에는 저장하지 않았습니다.
- 원래 `TestMainMenu_Level`로 에디터를 재시작했습니다. 종료 시점 응답 정상, dirty content/map 패키지 0개입니다. [검증 기록](evidence/merge-runtime-safety-2026-11-12.json)을 참조합니다.

전체 저장소 재수집은 하지 않은 부분 점검입니다. 모든 BP의 런타임 안전성, 실제 멀티플레이·맵 전환·패키지 실행의 성공을 보장하지 않습니다. 플레이 테스트는 사용자가 진행합니다.
