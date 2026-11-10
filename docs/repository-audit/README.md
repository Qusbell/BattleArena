# 저장소 조사 문서

이번 저장소 조사 문서와 근거 자료를 모은 폴더입니다. 현행 기능 문서는 원래 위치에 유지하며,
레거시·과거 실험·재검증 전 설계 문서는 [docs/history](../history/README.md)에 보관합니다.

## 읽는 순서

1. [현재 진행 상태](audit/progress.json): 완료 단계·미확인 사항·다음 작업
2. [시스템 지도](architecture/SystemMap.md): 객체의 책임과 시스템 연결
3. [데이터 원본 정책](architecture/DataAuthority.md): JSON·DT/DA 편집 기준
4. [발견 사항](architecture/Findings.md): 불일치·테스트 전제·정리 전 보류 항목
5. [객체 카탈로그](architecture/README.md): 필요한 시스템의 소스·Blueprint 색인

재수집·갱신 방법은 [조사 작업 안내](audit/README.md), 용량과 최초 수집 이력은
[기준선](audit/BASELINE.md), 상세 시험 조사 예시는 [BGM](systems/BGM.md)에 있습니다.
과거 수집 시점의 상태와 현재 `audit/progress.json`을 구분합니다.

삭제 후보 검토는 [TestMainMenu_Level 기준 명세](cleanup/README.md)를 확인합니다.
역참조 0건·얇은 의존 관계·레거시/프로토타입 묶음으로 구분하며, 전체 CSV/JSON은 필요한 항목만 검색합니다.

## 폴더 구성

- [보고된 버프·결과창·부활 문제의 실행 조사 및 수정 검증](systems/ReportedIssues-2026-11-10.md)
- [촬영용 UI 표시 설정](systems/UIRecording.md) — UI별 DA 체크박스, 58개 Blueprint 연결과 native 표시 경계.

| 경로 | 내용 |
|---|---|
| `audit/` | 조사 절차·현재 진행 상태·최초 기준선 |
| `architecture/` | 시스템 지도·역할 카탈로그·데이터 연결·발견 사항 |
| `systems/` | 개별 시스템 상세 조사와 증거 요약 |
| `audit/generated/` | 원본 그래프·CSV·체크포인트 등 로컬 수집 자료. Git 제외 |

## 갱신 방법

- 역할·호출 경로·설정 원본이 바뀌면 관련 설명 문서와 진행 상태를 함께 갱신합니다.
- 전체 수집이 필요하면 `audit/README.md`의 MCP 수집 절차를 따르고 새 실행 폴더를 사용합니다.
- 셸에서 분석기·카탈로그 작성기·검증기를 실행합니다. 사람이 검토한 시스템 설명은 자동으로 최신화되지 않습니다.
- 정적 분석 결과, 실제 실행 검증, 삭제 여부를 구분합니다. 미도달 에셋을 미사용으로 단정하지 않습니다.

## 폴더 이동 전 기록

2026-11-05 문서를 이 폴더로 통합했습니다. 기존 수집 JSON·CSV·해시에는 `docs/audit`,
`docs/architecture`, `docs/systems`가 남아 있을 수 있습니다. 당시 근거를 보존하기 위해 원본 수집 자료를
덮어쓰지 않았으며, `relocation.json`의 경로 대응과 `Scripts/AuditPaths.py`로 새 위치를 해석합니다.
검증기는 원래 해시 또는 이동 과정에서 기록한 정확한 변경 전후 해시만 허용합니다.
이후 내용 변경을 무조건 허용하는 규칙은 아닙니다.
