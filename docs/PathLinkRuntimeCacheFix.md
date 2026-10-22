# 포탈맵 추정 거리 False 문제

## 확인된 원인

### 1. 플레이 전 준비 순서

맵을 연 직후 첫 플레이에서 에디터 NavMesh가 아직 읽을 수 있는 Polygon을 제공하지 않는 경우를 재현했다.
`PrepareRoutingCacheForEditor()`는 이 상태를 캐시 미스로 처리하고 기존 `DataVersion`을 0으로 만들었다.
이어서 실행한 Bake도 `NavMesh has no traversable ground polygons`로 실패했다.
빈 캐시가 PIE로 복제되고, 런타임은 `No complete baked routing cache`로 모든 추정 조회를 거부했다.

`EnsureBuildCompletion()`은 이미 제출된 작업을 기다릴 뿐, 초기 등록·대기 중인 dirty 작업을 반드시
제출하지는 않는다. 이후 에디터 초기화가 끝나면 같은 맵에서도 Bake가 성공하므로 재시작/테스트 시점에
따라 정상으로 보일 수 있었다. 런타임에서 별도 거리 조회를 호출하는 행위가 Bake를 재생성하는 것은 아니다.

더 빠른 시작 검사에서는 PreBeginPIE가 엔진의 FlushAsyncLoading보다 먼저 실행되어, 일부 Actor/에셋의
초기화가 끝나기 전에 541개 Polygon/6개 Traversal로 Bake되는 경우도 확인했다. 준비 훅을 BeginPIE로 옮기고,
비동기 로딩·에셋 컴파일·지연 Actor Construction·Level Streaming 완료 후 Navigation을 검사하도록 수정했다.

수정 후에는 NavMesh 읽기 가능 여부부터 검사한다. 대기 중이거나 비어 있으면 에디터에서 Navigation Build를
완료하고 다시 검사한다. 단순히 NavMesh 준비가 덜 됐다는 이유로 완성된 Bake를 먼저 지우지 않는다.
게임 실행 중 All-Pairs Bake는 수행하지 않는다.

### 2. 런타임 Polygon 참조 ID 변경

포탈맵 RecastNavMesh의 Runtime Generation은 Dynamic이다. 지형의 모양이 같더라도 Tile 교체 시 Detour의
salt가 바뀌어 NavNodeRef가 달라진다. 기존 Polygon 참조→Bake 인덱스 맵은 초기 ID를 계속 사용했다.
전체 생성 완료 알림/OnNavMeshUpdate만으로는 실행 중인 개별 Tile 교체를 빠짐없이 감지할 수 없었다.

다음 좌표를 실제 `BPC_DebugAI_AttachToPlayerController.ReceiveTick`의 AI Pawn 입력으로 사용해
`StartPolygonMissing`과 Hello2를 재현했다. 위치는 테스트 직후 복원했다.

- 시작: `(1185.449396, -4228.476338, 2085.926174)`
- 목표 예시: `(676.674360, -3020.739873, 2281.150757)`
- NavMesh를 다시 읽으면 시작 Polygon 인덱스는 159, 목표 인덱스는 288이었다.
- 실행 중 시작 NavNodeRef는 `10269266804740`, `36657545871364` 등으로 바뀌었지만 지형 서명은 같았다.

수정 후에는 추정/상세 경로에서 캐시를 사용할 때 프레임당 한 번 Tile 참조 목록을 확인한다.
목록이 달라지면 현재 지형·연결 서명을 검증하고, 동일할 때만 참조 맵을 갱신한다.
프레임 중간의 Tile 교체로 Polygon 조회가 실패하는 경우에도 참조를 재확인한다.
지형·연결이 실제로 바뀌면 기존 Bake와 정적 Route 그래프를 무효화한다.
dirty 작업이 계속 대기한다는 이유만으로 런타임을 영구 차단하지 않고 현재 커밋된 NavMesh를 검증한다.

## 검증

- Editor/Game Development 빌드 통과.
- `ShootingArena.PathLink.BakedRouting`, `ShootingArena.PathLink.LazySearch` 통과.
- 이전 실패 좌표 4개를 실제 PIE에서 20초간 반복 조회: 첫 검사 240회, 두 번째 검사 232회,
  최종 첫 프레임 자동 플레이 이후 검사 240회 모두 성공.
- 최종 검사 평균 0.01766ms, P95 0.04470ms, 최대 0.25890ms. 진단이 켜진 개발 에디터의 제한된 회귀 표본이다.
- 동기 NavMesh 경로 탐색 0회. 런타임 캐시 약 0.5153MiB (Tile 참조 목록 512개 포함).
- 실제 디버그 Blueprint의 Hello3/거리 출력 확인.
- 맵 재로드와 같은 에디터 Tick에서 플레이를 요청해 자동 Bake/런타임 검증 성공 확인.
- 최종 빌드에서 `-ExecCmds`로 에디터 첫 프레임에 자동 플레이를 요청한 검사도 통과:
  Polygon 910개, Island 210개, Traversal 12개로 생성한 뒤 PIE에서 서명 검증 성공.
  수정 전 같은 조건에서는 빈 캐시 또는 Polygon 541개/Traversal 6개 Bake로 실패했다.
- PIE에 임시 충돌 Cube를 생성해 실제 NavMesh 지형을 변경하면 조회가 False 및
  `NavMesh signature mismatch; rebake the level.`로 차단됨을 확인했다. 테스트 Cube는 제거했고 에디터 맵에는 생성하지 않았다.

회귀 스크립트: `Scripts/VerifyPathLinkRuntimeReferences.py`.
결과: `Saved/PathLinkBench/RuntimeReferenceRegression.json`.

## 진단 API

`GetEstimatedRouteDistance` 직후 같은 실행 경로에서 `GetLastEstimateStatus`를 읽는다.

- `Success`: 조회 성공
- `CacheUnavailable`: Bake 부재/불일치 등. `GetRoutingCacheStatus`도 확인
- `StartProjectionFailed` / `TargetProjectionFailed`: 해당 좌표를 NavMesh에 투영하지 못함
- `StartPolygonMissing` / `TargetPolygonMissing`: 투영된 Polygon이 유효한 참조 맵에 없음
- `ClientWorld`: 서버용 조회를 클라이언트 World에서 요청
- `NavigationDataMismatch` / `UnsupportedFilter`: Bake와 다른 NavData/필터
- `Unreachable`: 테이블과 Path Link 그래프에서 경로를 찾지 못함

`GetRoutingCacheStatus`가 정상이더라도 개별 좌표 투영은 실패할 수 있다.
낙하 중 좌표, NavMesh 밖 좌표 등은 여전히 정상적으로 False를 반환한다.
임시 로그: `PathLink.EstimateDebug 1` 또는 `2`, 종료 후 `PathLink.EstimateDebug 0`.
