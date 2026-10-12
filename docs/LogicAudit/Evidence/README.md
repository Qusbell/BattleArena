# 조사 증거와 재현 범위

2026-10-12 현재 Unreal Editor를 MCP로 조회한 스냅샷과 로컬 Source 정적 색인입니다. `.uasset`를 shell에서 분석하거나 수정하지 않았습니다. Blueprint를 compile/save하지 않았고 Live Coding/PIE를 실행하지 않았습니다.

| 파일 | 내용 |
|---|---|
| asset-inventory.json | 로직 관련 에셋 목록, 부모, hard/soft package 루트 도달, 참조자 |
| asset-reachability.json | 루트·도달 package·대표 predecessor·의존 연결 |
| blueprint-targets.json | 상세 조사한 Blueprint 선정 목록 |
| blueprints-*.json | 그래프 전체 노드/핀/default/links, 직접 변수·SCS·CDO 값 |
| behavior-tree.json | BT_QuakeTree runtime node 순서·Decorator·Service |
| blackboard.json | BB_QuakeBoard 현재 key 이름·타입 |
| data-tables.json | QuakeLike_1_0 DataTable 15개의 전체 행 |
| data-assets.json | 주요 DataAsset의 현재 editor 값 |
| json-apply-registry.json | 실제 JsonAssetSync 적용 binding |
| component-defaults.json | 로드된 주요 SCS/native component template 기본값 |
| map-settings.json | DT의 맵 및 서버/로그인 맵 WorldSettings GameMode |
| map-actor-classes.json | 로드된 12개 맵의 메모리 Actor 클래스별 수량 |
| level-scripts.json | 발견된 LevelScriptBlueprint의 조회 가능한 그래프 |
| coverage.json | 실제 집계·실패·미지원 그래프 수 |
| validation.json | 문서 링크·Mermaid 구조 검사 결과 |
| source-hashes.json | 조사 시점 Source·Config 파일 SHA-256 |

## 수집 방식

`collect_unreal.py`는 Unreal Python을 사용하는 **읽기 전용 수집 함수**입니다. MCP `util.execute_python` 내부에서 실행해야 합니다. inventory()는 registry를 읽고, collect_batch(start, count)는 Asset Registry에서 선정한 BP를 작은 배치로 로드하여 MCPythonHelper로 그래프·변수·SCS를 읽습니다. 기존 증거를 덮어쓰므로 재조사할 때 새 출력 경로를 정하거나 먼저 보관해야 합니다.

`build_atlas.py`는 이미 저장한 JSON만 읽어 Blueprint 상세 Markdown·색인·Reflection 목록을 생성합니다. `.uasset`에 접근하지 않습니다. 시스템 요약/설계 제안은 수동 대조한 문서이며 자동 생성 대상이 아닙니다.

루트 선정은 게임/에디터 시작 맵, GI, 서버 기본/로그인/전환 맵, 현재 패키징의 경기 맵, UI 로비, 문자열로 선택하는 multiplayer GameMode, 전역 fallback GameMode입니다. 없는 cook 맵 01/03도 설정의 사실로 루트 목록에 남겼습니다. /Game 의존은 hard/soft package만 포함하며 native Script와 management/searchable-name은 제외했습니다. C++ 문자열 로드/Reflection 의존은 ReflectionContracts에서 별도로 기록합니다.

## 한계·오류 기록

- `get_blueprint_graph_info`의 helper는 UbergraphPages/FunctionGraphs만 지원합니다. ObjectIterator가 발견한 macro/delegate/compiler intermediate/state graph 중 조회 실패한 이름은 각 BP의 unsupported_graphs에 남습니다. 이 수치는 누락된 gameplay 함수 수가 아닙니다.
- 함수/이벤트 존재와 실제 실행은 다릅니다. 상세 도표는 unconnected 이벤트도 보존하고, 데이터 핀/default·pure 계산은 원본 JSON과 전체 노드 목록을 제공합니다.
- 맵 package load 배치 한 번에서 MCP의 30초 응답 제한을 넘겼습니다. 작업이 Editor에서 완료된 후 파일 생성과 JSON 파싱, 후속 MCP 응답을 확인했습니다. 수집 실패로 판단해 우회하지 않았습니다.
- Python deprecation warning 때문에 일부 wrapper 출력이 JSON 판정에 실패했지만, 해당 파일은 Editor에서 생성되었으며 구조를 다시 검증했습니다.
- 최초 전체 Git status는 Git LFS 임시 디렉터리 쓰기 권한으로 실패했습니다. 문서/Source 범위 확인은 에셋 필터를 건드리지 않는 scoped 확인을 사용했습니다. 이 작업에서 사용자의 기존 에셋 변경은 되돌리지 않았습니다.
- current Editor의 메모리 값은 JsonAssetSync MemoryOnly 적용 결과일 수 있습니다. 디스크 저장본과 동일하다고 보장하지 않습니다.
- WorldPartition/streaming 인스턴스 전체, 원격 네트워크 실행, cook·build·자동 테스트는 이번 조사에서 검증하지 않았습니다.
