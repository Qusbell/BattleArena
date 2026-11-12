<!-- repository-audit-20261105:start -->
> 2026-11-05 저장소 조사: 현재 소스·에셋과 연결을 정적 대조한 참고 문서입니다. 아래 과거 빌드·자동 테스트·실행 결과를 이번 검증 결과로 해석하지 않습니다.
> 현행 연결과 미확인 사항은 [시스템 지도](repository-audit/architecture/SystemMap.md), [데이터 원본 정책](repository-audit/architecture/DataAuthority.md)를 확인합니다.
<!-- repository-audit-20261105:end -->

# 클라이언트 로딩 완료 보고

2026-11-12 언어 기능 통합: 로딩 WBP는 언어 패치 대상에서 제외합니다. 언어 subsystem 초기화 의존성·StatusText 번역 어댑터·전역 글꼴 교체를 로딩 경로에서 사용하지 않습니다. WBP 생성·전환별 배경·페이드·입력 잠금·BGM·준비 보고는 이 문서의 경로를 유지합니다.

## 로딩 화면 표시 — 2026-11-12 변경

`ULoadingScreenSubsystem::ShowLoadingScreen`은 전환마다
`/Game/QuakeLike_1_0/GameFlow/UI/Loading/WBP_LoadingScreen_Ref`를 GameInstance 소유로 만들어 오버레이 안에 표시합니다.
위젯은 Construct에서 GameInstance의 `Selected_Map_ID`로 `DT_MapData.Map_Image`를 배경에 넣습니다.
클래스를 불러오지 못하면 경고를 남기고 기존 상태 문구 오버레이를 씁니다. 위젯 경로는 C++ 문자열이므로
`DefaultGame.ini`의 `DirectoriesToAlwaysCook`에 `GameFlow/UI/Loading` 폴더를 등록해 쿡에 포함합니다.
전용 서버에서는 표시하지 않습니다. 이 변경은 Game 타겟 컴파일만 확인했고 실행·패키징 검증은 하지 않았습니다.

## BGM과의 연결 — 2026-11-05 조사 보충

`ULoadingScreenSubsystem`은 BGMSubsystem을 초기화 의존성으로 선언합니다.
로딩 시작 시 BGM 감쇠를 요청하고, `RemoveLoadingScreen(bRestoreBGM=true)`에서
`EndLoadingBGM()`으로 볼륨을 복구합니다. BGM 완료 콜백을 플레이 준비 보고 조건으로 사용하지는 않습니다.
설정·곡 선택·옵션 볼륨의 별도 경로는 [BGM 시스템 조사](repository-audit/systems/BGM.md)에 정리했습니다.
아래의 준비 보고 절차는 이번 조사에서 전체 재검증하지 않았습니다.

엄밀한 전체 초기화 보장이 아닌, 로딩 화면 종료와 실제 Pawn 도착을 기준으로 한 간단한 시작 신호입니다.

## 자동 보고 경로

1. `ULoadingScreenSubsystem`이 현재 월드의 Coordinator `ReadyToPlay`를 확인합니다.
2. 페이드아웃 후 `RemoveLoadingScreen()`에서 오버레이 제거와 `ReleaseInputLock()`을 완료합니다.
3. 로컬 `AMyPlayerController`가 자신의 Pawn과 Controller 연결을 확인합니다. 아직 Pawn이 없으면 0.1초 후 재확인합니다.
4. Reliable Server RPC `ServerReportGameplayReady`로 Pawn을 보고합니다.
5. 서버는 보고한 Pawn이 해당 컨트롤러의 현재 Pawn인지 확인한 뒤 GameMode에 알립니다.

실패/타임아웃에 의한 오버레이 해제, 수동 `EndExternalLoading`, 메뉴 도착, 서브시스템 종료는 준비 보고를 만들지 않습니다. 새 로딩이 시작되면 대기 중 보고를 취소합니다. Coordinator 없이 직접 시작하는 PIE에서는 자동 보고하지 않습니다.

## BP 연결 위치

에디터를 종료한 상태에서 C++ 빌드한 뒤 다시 열어 `BP_MultiplayerAIGameMode`에서 이벤트를 추가합니다. `AMyReconnectionGameMode`를 상속한 다른 GameMode에서도 사용 가능합니다.

- **Event On First Client Gameplay Ready**: 해당 GameMode에서 첫 클라이언트 보고를 받았을 때 한 번만 실행합니다. AI 전체 시작과 경기 타이머 시작을 연결할 위치입니다. 전원 준비를 기다리지 않습니다.
- **Event On Client Gameplay Ready**: 컨트롤러별 첫 보고마다 실행합니다. `Player` 핀으로 보고한 플레이어를 받습니다.
- **First Client Gameplay Ready**: 서버에서 첫 보고 수신 여부를 읽는 bool입니다.

두 이벤트 모두 서버에서 실행합니다. 첫 보고에서는 First 이벤트, 개별 Client 이벤트 순서입니다. 동일 컨트롤러의 중복 보고와 같은 월드의 리스폰은 이벤트를 다시 실행하지 않습니다. 새 GameMode가 생성되면 상태를 새로 시작합니다.

현재 `BP_QuakeGameMode`의 First 이벤트에는 경기 시계 시작과 기존 AI 해제 체인이 연결되어 있습니다.

```text
On First Client Gameplay Ready
→ 맵 이름 확인 (기존 메뉴 제외 조건)
→ bMatchStarted = true
→ ElapsedMatchTime = 0
→ MarkMatchTime (GameState.StartTime 기록)
→ 기존 AI 블랙보드 해제 체인
```

기존 BeginPlay의 시계 시작 연결은 제거했습니다. C++ `AMatchRulesInfo`의 HUD 시계 동기화도 Coordinator 상태 대신 `bMatchStarted`를 기준으로 시작합니다. 동기화 검사는 0.5초 간격입니다.

## 확인 방법

`TestMainMenu_Level`부터 게임에 접속하고 로그를 확인합니다.

```text
[Loading] Client gameplay ready: ...
[Loading] Server received gameplay ready: ... (first=1)
```

멀티플레이의 두 번째 참가자는 서버 로그에 `first=0`으로 표시됩니다. BP 이벤트에 Print String을 연결하면 이벤트 호출도 확인할 수 있습니다.
