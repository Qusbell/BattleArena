# AI 디버그 네트워크 표시

## 전송 및 표시

- `BPC_DebugAI_AttachToPlayerController`가 기존 서버 AI 조회와 입력/RPC를 유지합니다.
- 디버그 수집은 0.1초 간격입니다. 한 번의 수집 결과를 소유 클라이언트에 하나의 Unreliable RPC로 전달합니다.
- `FDebugAIPositionFrame`의 기하 정보는 월드 좌표 벡터만 포함합니다. `FVector_NetQuantize10`으로 전송합니다.
- 색상, 반지름, 선 두께, 화살표 크기, 고정 라벨과 도형 연결 규칙은 `DebugAINetworkComponent.cpp`의 클라이언트 렌더링 코드에서 결정합니다.
- 서버 전용 동적 값은 `FDebugAIValueFrame`으로 분리합니다. AI 상태/BT/특성/Aggression 문자열과 숫자 GoalWeight 값을 유지합니다.
- 클라이언트는 최신 스냅샷을 보관하고 매 프레임 그립니다. 수신 빈도에 맞추어 도형 유지 시간을 늘리지 않습니다.
- 관전/표시 종료 시 Reliable clear 한 번으로 캐시와 렌더링을 끕니다. Shipping에서는 수집 컴포넌트를 생성하지 않습니다.
- 도형별 `ClientDrawDebugLine/Sphere/Arrow/Box/String` RPC는 제거합니다.

## 반영 상태

2026-11-04: C++ 일반 빌드 성공 후 에디터를 재실행하고, MCP로 Blueprint 연결을 교체했습니다. BPC_DebugAI_AttachToPlayerController와 BP_QuakePlayerController 모두 컴파일에 성공했습니다. Live Coding은 사용하지 않았습니다. 실행 테스트는 수행하지 않았습니다.

## 사용자 테스트

`TestMainMenu_Level` → 기존 메뉴 경로 → 데디케이티드 서버 접속으로 확인합니다. 리슨 서버는 테스트 범위에서 제외합니다.

1. Y로 AI를 선택하고 STG/LTG/Enemy/EnemyLastLocation/현재 조준점과 경로 표시를 확인합니다.
2. H로 항목을 개별로 켜고 끕니다. AI 상태·BT·Aggression·아이템 GoalWeight도 확인합니다.
3. 이동 방향/막힘 및 조준 경계 표시를 확인합니다.
4. Y 관전 종료 후 본인 Pawn과 카메라 복구 및 표시 해제를 확인합니다.
5. 다른 클라이언트의 표시가 함께 변경되지 않는지, U 명령이 기존처럼 서버 AI에 적용되는지 확인합니다.

실행 테스트는 사용자가 담당합니다. 에이전트는 Blueprint 컴파일과 C++ 빌드 및 그래프 연결을 확인합니다.
