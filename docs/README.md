# ShootingArena 문서 안내

다음 AI는 프로젝트 루트 `AGENTS.md`와 [저장소 조사 문서의 읽는 순서](repository-audit/README.md)를 먼저 읽습니다.

| 자료 | 용도와 범위 |
|---|---|
| [조사 기준선](repository-audit/audit/BASELINE.md) | 1단계 파일·용량·에셋 목록. 수집 시점의 스냅샷입니다. |
| [조사 진행 상태](repository-audit/audit/progress.json) | 현재 완료 범위와 다음 작업 |
| [전체 객체 카탈로그](repository-audit/architecture/README.md) | 3단계: 소스 타입·Blueprint·에셋 역할 색인 |
| [시스템 지도](repository-audit/architecture/SystemMap.md) | C++·Blueprint·외부 데이터의 주요 연결 |
| [데이터 원본 정책](repository-audit/architecture/DataAuthority.md) | JSON과 DT/DA 편집 기준 |
| [발견 사항](repository-audit/architecture/Findings.md) | 오래된 경로·테스트 전제·정리 전 보류 항목 |
| [로직 삭제 후보](repository-audit/cleanup/README.md) | TestMainMenu_Level 기준 3분류, 정확한 경로·참조자·간접 영향 |
| [과거·보류 문서](history/README.md) | 레거시 사용 안내, 과거 실험/수정 결과와 재검증 전 설계 자료 |
| [BGM 시스템](repository-audit/systems/BGM.md) | 2단계 시험 조사: 역할·설정·Blueprint/C++ 연결·미확인 사항 |
| [클라이언트 로딩 완료](../Docs/ClientGameplayReady.md) | 로딩 후 클라이언트 준비 보고 |
| [게임플레이 기록 DB](../GamePlayDB/README.md) | 경기 기록 SQLite와 검증 |
| [아이템 생성음·획득음](../Docs/ItemSoundSettings.md) | 기존 기능 문서. 이번에 최신성 전체 검증은 하지 않았습니다. |

과거 실험·수정 기록과 폐기된 안내는 `docs/history/`로 분리했습니다.
현행 기능 문서도 상단의 검증 범위와 작성 시점을 확인하며, 실제 실행 검증 여부를 구분합니다.
