# 전수조사 발견 사항과 정리 전 판단

2026-11-05 정적 조사 결과입니다. 아래 항목은 삭제 승인이 아니며 파일·에셋은 삭제하지 않았습니다.

## 확인한 불일치와 문서 보정

1. `Config/DefaultGame.ini:108`, `:109`의 Cook 맵 경로 `Lvl_QuakeMap_01`, `Lvl_QuakeMap_03`은 현재
   Asset Registry와 `does_asset_exist`에서 모두 발견되지 않았습니다. 설정 문자열 존재를 실사용 에셋 존재로
   해석하면 안 됩니다. 설정 수정은 별도 변경/패키징 검증으로 진행해야 합니다.
2. `docs/history/legacy/Life관련 사용 설명서.txt`는 `QuakeLike_Base/LJH` 기준 레거시 안내입니다. 현재 Life 원본은
   `Content/DataAssets/DA_DefaultLife.json`, 대상은 `/Game/QuakeLike_1_0/Data/Chracter/DA_DefaultLife`입니다.
   원문은 이력으로 보존하고 현행 경로를 상단에 보충했습니다.
3. 아이템·버프 DT 편집 안내에는 JsonAssetSync 원본 정책을 보충했습니다. 대상만 직접 편집한 값은
   시작 시 JSON 적용으로 바뀔 수 있습니다. 실제 Registry/Manifest 29쌍은 일치합니다.
4. 과거 PathLink 실험 문서에는 현재 WorldSettings 캐시 저장 구조와 과거 결과의 구분을 보충했습니다.
   과거 성능 수치를 새 측정 결과로 바꾸지 않았습니다.
5. Git 경로 `Docs/`와 `docs/`가 혼재합니다. Windows에서는 같은 디렉터리로 보이지만 다른 환경에서는
   링크/자동화 차이가 생길 수 있습니다. 이번 새 링크는 Git의 대소문자를 대조합니다. 폴더 통일은 아직 하지 않았습니다.
6. 맵 미리보기 테스트 입력 `Content/Movies/MainMenuPreview.mp4`는 현재 없습니다.
   `MapPreviewMediaTests.cpp:143`은 파일이 없으면 Warning 후 true를 반환하므로 이 조건의 성공은 재생 검증이 아닙니다.
   기존 미리보기 문서에 테스트 전제를 명시했습니다. 영상 파일을 새로 만들거나 테스트를 실행하지 않았습니다.

## 에셋 도달 관계의 의미

- 패키지 28,198개에 정적 참조 87,375개를 수집했습니다(hard 70,973 / soft 16,402). 관리 참조 조회는 0건입니다.
- 설정 진입점·Cook 대상·JSON 등록 대상을 루트로 한 closure는 13,963개입니다.
- C++/설정/JSON에 명시된 패키지 경로까지 보수적으로 포함한 closure는 13,990개입니다.
- 그 밖의 일반 패키지 13,344개는 **판단 보류**입니다. 11,609개 외부 Actor/Object 패키지는 별도 소유관계 검토가 필요하며,
  위 도달 집합과 서로 겹칩니다. 이 수치들을 서로 배타적인 그룹처럼 더하지 않습니다.
- 역참조 0건 패키지는 5,735개입니다. 설정·동적 문자열·테스트·저작 도구에서 진입할 수 있으므로 삭제 근거로 단독 사용하지 않습니다.

`QuakeLike_1_0` 패키지 6,826개 중 보수적 closure는 1,049개이고,
`QuakeLike_Base`는 1,289개 중 329개입니다. 경로 기반의 실제 정적 연결 수치이며
앞쪽 나머지가 폐기되었다거나 뒤쪽 전부가 레거시라는 의미는 아닙니다.
CompShooterHUD와 외부 리소스 팩에도 도달 관계가 남아 있습니다.

전체 경로별 판정 근거는 실행 폴더의 `package-reachability.csv`, `roots.json`, `text-references.json`,
`typed-dependencies.csv`에서 확인합니다. 후보를 지우기 전 개별 참조·동적 로딩·에셋 소유관계를 다시 검토해야 합니다.

## 그래프 범위

Blueprint 923개에서 최상위 JSON 그래프 2,605개(노드 36,421개)를 수집하고,
JSON 조회기 외 그래프 4,158개를 MCP 경유 Unreal T3D로 추가 내보냈습니다.
컴파일/병합 파생 그래프 2,467개는 별도 목록으로 구분했습니다. 추가 내보내기 실패는 0건입니다.
T3D에는 중첩 AnimGraph·매크로·생성 이벤트 그래프가 섞일 수 있으므로, JSON 그래프 수와 단순히 더해
고유한 원본 그래프 수라고 해석하지 않습니다. Blueprint의 디자인 의도와 모든 핀의 실행 타당성을 사람이 검증한 결과도 아닙니다.

## 4단계에서 검토할 파일 종류

| 종류 | 처리 전 필요한 근거 |
|---|---|
| `.vs`, Intermediate, DDC 등 | Editor/빌드 사용 상태, 재생성 가능성, 바이너리 전용 플러그인 여부 |
| Saved/Profiling·TraceAnalysis·Logs·Crashes | 성능·장애 기록 보존 필요와 기간 |
| Saved/Analyze·SaveGames·각종 Backup | 경기 DB·사용자 저장·복구 자료 구분 |
| 미도달 에셋·ObjectRedirector | 실제 진입점, 동적 경로, 맵/외부 Actor 소유관계, 이동 경로 호환성 |
| 추적된 `__pycache__/*.pyc` | `.gitignore`/추적 정책 변경과 실제 불필요 파일 제거의 분리 |
| 일회성 migration 도구 | 현재 호출자, 재현/복구 필요, 과거 자료 역할 |

4단계에서는 작은 변경 묶음으로 삭제 계획을 만들고 변경 종류에 맞는 빌드·Cook·실행 검증을 진행합니다.
3단계에서는 에셋 수정·저장·컴파일·Live Coding·PIE·삭제를 수행하지 않았습니다.
