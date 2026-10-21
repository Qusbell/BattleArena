# Filtering STG의 추정 거리 LTG 비교 적용

2026-10-21. 대상: /Game/QuakeLike_1_0/AI/AIC/BP_QuakeAIController2 의 Filtering STG.

사용자가 만든 Bot→STG 추정 거리 및 도달 시간 계산은 유지했습니다. 기존 Is Shorter Then LTG 호출을 제거하고, 그 비교 로직을 Filtering STG 안에 풀어 배치했습니다. 원본 함수 정의는 다른 호출부와의 호환성을 위해 남겼습니다.

도달 시간 조건 통과 후 MyCharacter와 LTG의 유효성을 확인합니다. 추가한 추정 조회는 Bot→LTG, STG→LTG 두 개이며 앞에서 계산한 Bot→STG 거리를 재사용합니다. 기존과 같은 기본 PathfindingContext를 사용합니다.

최종 통과 조건은 다음과 같습니다.

```text
Bot→LTG 추정 조회 성공
AND STG→LTG 추정 조회 성공
AND Bot→STG 거리 < Bot→LTG 거리
AND STG→LTG 거리 < Bot→LTG 거리
```

조회 실패값을 거리 0으로 사용해 후보를 통과시키지 않습니다. 추정 실패는 도달 불가의 정밀 증명은 아니므로, 현재는 해당 평가에서 후보를 제외하는 정책입니다. 기존 두 거리 비교를 합산 우회 비용 비교로 변경하지 않았습니다.

추정 조회 두 노드는 const API의 기본 Pure 형태로 만들었습니다. 출력은 하나의 최종 비교 Branch에만 연결했으며, 조회 성공 여부와 거리 비교를 그 Branch의 데이터 수집 단계에서 함께 사용합니다. 후보별 Bot→STG를 다시 조회하지 않습니다. Bot→LTG는 현재 시간 조건을 통과한 후보마다 조회하며, 추후 필요하면 루프 바깥으로 이동해 공통 값으로 재사용할 수 있습니다.

검증 결과:

- Blueprint 컴파일 UpToDate, 저장 완료.
- 실제 Filtering STG 함수를 임시 Controller·Character·목표 Actor로 호출해 60개 위치 조합 검사: 예상과 실제 결과 불일치 0개, 통과 14개, 탈락 46개.
- 빈 후보 목록, LTG 없음, 추정 조회 실패 처리 검사 통과.
- 동작 검사에서는 시간 제한을 임시 Actor에만 크게 설정해 LTG 비교를 분리했습니다. 원본 CDO의 TravelTimeLimit=5는 유지했습니다.
- 임시 Actor 모두 제거. 실제 PIE 이동·낙하 물리 검사는 이번 검증에 포함하지 않았습니다.

모든 Blueprint 접근 및 변경은 Unreal MCP로 수행했습니다. 구현 스크립트는 Scripts/InlineSTGLTGEstimate.py, 동작 검사 스크립트는 Scripts/VerifySTGInlineLTG.py입니다. 이전 노드 구성 및 검사 원본은 Saved/PathLinkBench/FilteringSTG_BeforeInlineLTG.json, FilteringSTGInlineLTGVerification.json에 보관했습니다.
