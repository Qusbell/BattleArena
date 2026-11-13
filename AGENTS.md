# BP Asset
- 거의 모든 사용 중인 Blueprint asset은 `/Game/QuakeLike_1_0`에 위치해 있습니다.
- `/Game/QuakeLike_Base`의 asset은 대부분 폐기처리 되어 있으며, 일부분(레벨 등)만이 사용되고 있습니다.
- 그 이외에도 레거시와 실제 사용 중인 Blueprint asset이 혼재되어 있습니다.

# Rule

## 저장소 조사 문서
- 조사 문서는 [`docs/repository-audit/README.md`](docs/repository-audit/README.md)를 진입점으로 사용합니다.
- 일반 작업은 관련 시스템 문서만 읽습니다. 저장소 전반의 구조 파악이 필요한 경우 README → `audit/progress.json` → `architecture/SystemMap.md` 순서로 읽고, DataAuthority·Findings·카탈로그는 필요한 부분만 확인합니다.
- 역할·의존 관계·설정 원본을 실질적으로 변경하면 관련 설명과 영향 범위를 갱신합니다. 조사 색인 갱신은 `audit/README.md`의 절차를 따르며, 기본은 부분 갱신입니다. 전체 재수집은 명시적으로 요청했거나 광범위한 구조 변경이 있을 때 수행합니다.
- 대형 JSON·CSV·그래프 원본은 전체를 문맥에 넣지 말고 필요한 항목을 검색·추출합니다. 수집 원본은 보존하고 사실·추론·미확인 및 실제 검증 범위를 구분합니다.
- 과거·폐기·미검증 설계 문서는 [`docs/history/`](docs/history/README.md)에 보관하고 `index.json`에 이유를 남깁니다. 현행 판단에는 최신 근거를 사용하며, 정적 미도달·역참조 0건만으로 삭제하지 않습니다.

## 말투
- 정중하고 전문적인 `-합니다`체를 유지합니다.

## MCP
- plan 모드 사용 시, MCP가 사용 가능한 상태인지 확인하세요.
- Blueprint 조작은 MCP로만 접근하고, 만약 MCP에 접근할 수 없다면 우회하지 말고 사용자에게 실패 및 그 이유를 보고하세요.

## PIE 실행 모드
- 이 프로젝트의 게임을 PIE에서 실행하거나 검증할 때는 `Net Mode`를 반드시 `Play As Client`로 설정합니다. 레벨을 직접 열어 시작하는 경우에도 같은 설정을 사용합니다.
- `Play Standalone`과 `Play As Listen Server`에서는 게임이 정상적으로 동작하지 않으므로 게임 동작 검증에 사용하지 않습니다.

## Live Coding
- Editor를 켠 상태로 Live Coding 사용 시 Editor가 영구 정지하는 문제가 반복적으로 관찰되고 있습니다.
- Live Coding을 사용했다면 모든 작업을 마친 이후 Editor 상태를 확인하고, `응답 없음` 상태가 지속된다면 임의로 Editor를 종료해도 좋습니다. 사용자의 작업물은 Git으로 보존되며 현 세션에서의 변경점은 없습니다.
