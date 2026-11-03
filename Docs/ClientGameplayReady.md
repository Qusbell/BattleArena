# 클라이언트 로딩 완료 보고

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
