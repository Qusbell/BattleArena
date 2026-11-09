# 저장소 전수조사 작업 안내

이 문서는 저장소 조사와 정리를 이어갈 AI의 시작점입니다. **1단계 조사 기반 구축**과
**2단계 BGM 시스템 시험 조사**, **3단계 정적 전수조사와 문서 대조**를 수행했습니다.
실제 플레이·Cook 검증과 삭제는 아직 수행하지 않았습니다. 현재 상태는 [progress.json](progress.json),
전체 결과는 [객체 카탈로그](../architecture/README.md), 시험 조사 결과는 [BGM 시스템](../systems/BGM.md)에 있습니다.

## 먼저 읽을 자료

1. 프로젝트 루트 `AGENTS.md`: Blueprint는 MCP로만 접근하며, MCP 실패 시 우회하지 않습니다.
2. [BASELINE.md](BASELINE.md): 최신 수집 결과와 폴더별 용량입니다.
3. [baseline.json](baseline.json): 기준 커밋, 수집 범위, 단계 상태, 산출물 해시입니다.
4. `baseline.json`의 `run_directory`: 상세 CSV와 재개 체크포인트가 있는 로컬 폴더입니다.

문서와 스크립트는 Git에 포함할 대상으로 작성했습니다. 상세 산출물은 대량의 재생성 자료이므로
`generated/.gitignore`로 제외합니다. 다른 PC에는 상세 CSV가 전달되지 않으므로 다시 수집해야 합니다.
현재 파일들은 작업 트리에 작성한 상태이며 자동 커밋하지 않았습니다.

## 수집 범위와 판정 한계

- 파일 조사: 워크스페이스 전체의 경로·논리 바이트·수정 시각·Git 추적 분류를 수집합니다.
- `.git`, 조사 생성 폴더, 심볼릭 링크와 junction은 제외하고 제외 이유를 기록합니다.
- Windows 경로는 Git 추적 여부 비교 시 대소문자를 구분하지 않습니다.
- 파일 내용 전체의 해시를 계산하지 않으며 `.uasset`/`.umap` 바이너리를 읽지 않습니다.
- 에셋 조사: MCP를 통해 `/Game`과 로컬 `Plugins/*/*.uplugin`의 마운트 후보를 조회합니다.
- Asset Registry에서 디스크에 저장된 에셋 메타데이터만 읽습니다. 에셋 로드·컴파일·저장은 하지 않습니다.
- Engine/엔진 플러그인, 다른 위치의 플러그인, 저장 전 에셋, Actor 인스턴스는 에셋 수집 범위 밖입니다.
- 플러그인 조회 결과가 0개인 경우 비활성/미마운트와 실제 빈 콘텐츠를 아직 구분하지 않습니다.
- 문서 목록은 Markdown 전체 및 `docs`, `Manual`, `perf_docs`, `GamePlayDB`의 txt/pdf/docx를 포함합니다.
- 설정 진입점은 Config의 주요 map/mode/cook/PrimaryAsset 키를 추출한 것입니다. 외부 JSON과 동적 경로까지
  분석한 완전한 런타임 진입점 목록이 아닙니다.
- 실행 중 파일이 바뀔 수 있는 비원자적 스냅샷입니다. 용량은 실제 디스크 할당량과 다를 수 있습니다.
- 디스크/Registry 목록 일치는 누락 비교 결과이며 미사용 또는 삭제 안전성을 증명하지 않습니다.

## 다시 실행하는 방법

프로젝트 루트에서 Python 3.12 이상으로 실행합니다(현재 환경은 3.13.2).
일반 Python은 파일 메타데이터 수집만 담당하며, Unreal 내장 Python과 구분합니다.

```powershell
python Scripts/RepositoryAudit.py
```

출력의 `run` 경로를 기록합니다. 실패한 파일 조사는 새 실행으로 다시 수집합니다.
중단된 실행은 `run.json`의 `filesystem: running` 상태로 남으며 최종 결과로 취급하지 않습니다.

MCP `util.get_project_info`로 프로젝트를 확인한 다음, **MCP `util.execute_python`을 통해서만** 아래 코드를 실행합니다.
`RUN_DIRECTORY`는 앞서 출력된 실행 폴더의 절대 경로로 바꿉니다.

```python
import runpy
from pathlib import Path
import unreal
root = Path(unreal.Paths.project_dir()).resolve()
collector = runpy.run_path(str(root / 'Scripts/RepositoryAuditUnreal.py'))
collector['collect'](RUN_DIRECTORY)
```

Asset Registry가 아직 로딩 중이면 완료 후 재시도합니다. 같은 실행 폴더로 다시 호출하면 완료한 마운트는
건너뛰고 실패한 마운트를 재시도합니다. 수집기 코드가 변경되면 체크포인트를 무효화하고 다시 수집합니다.
프로젝트 콘텐츠 자체가 변경되었다면 같은 실행을 재개하지 말고 새 파일 조사부터 시작합니다.

수집 후 일반 셸에서 요약을 갱신하고 산출물을 검증합니다.

```powershell
python Scripts/RepositoryAudit.py --finalize docs/repository-audit/audit/generated/RUN_ID
python Scripts/RepositoryAudit.py --verify docs/repository-audit/audit/generated/RUN_ID
```

`--finalize`는 최신 `baseline.json`과 `BASELINE.md`를 갱신합니다. 수집 상태가 partial/pending이면
1단계 완료로 판단하지 않습니다. `--verify`는 수집기·산출물 해시, 파일 총량, 에셋 중복과 클래스 집계를 검증합니다.
게임 빌드·Cook·실행 검증을 대신하지 않습니다.

## 상세 산출물

| 파일 | 내용 |
|---|---|
| `run.json` | 파일 수집 직전 Git 상태, 단계 상태, 시각, 산출물 해시 |
| `files.csv` | 전체 파일 메타데이터와 tracked/untracked/ignored 분류 |
| `folders.csv`, `saved-folders.csv`, `extensions.csv` | 폴더·Saved 하위 폴더·확장자별 파일 수와 논리 용량 |
| `disk-assets.csv` | 디스크 에셋 파일과 프로젝트/플러그인 패키지 경로 후보 |
| `documents.csv` | 문서 후보 목록; 내용의 최신성은 미검토 |
| `entrypoints.json` | 주요 시작 맵·GameInstance·GameMode·Cook 관련 설정과 행 번호 |
| `filesystem-errors.json`, `excluded-paths.json` | 수집 실패와 제외 경로 |
| `tracked-not-observed.json` | 추적 파일 중 스캔에서 관찰하지 못한 경로; 제외/누락 여부 확인용 |
| `asset-registry.csv`, `registry-*.csv` | 에셋 객체·패키지·클래스·마운트 |
| `asset-registry-state.json` | 마운트별 체크포인트와 오류 |
| `asset-registry-summary.json` | 클래스 집계, 엔진 버전, 수집 전후 dirty 패키지 상태 |
| `registry-comparison.json` | 디스크에만/Registry에만 있는 패키지 |

`run.json`의 Git 상태는 파일 수집 직전입니다. 도구 작성과 MCP 접속 후 상태이므로 사용자 요청 직전 상태와
동일하지 않을 수 있습니다. 이번 작업 시작 시 기존 변경은 `ExternalData/JsonAssetSyncManifest.json` 한 개였으며
이 파일을 수정하거나 되돌리지 않았습니다. MCP 접속으로 갱신된 추적 Python 캐시는 작업 종료 시 원복합니다.

## 다음 단계

1. [발견 사항](../architecture/Findings.md)과 `package-reachability.csv`로 정리 후보를 검토합니다.
2. 미도달·역참조 0건을 삭제 확정으로 취급하지 않고 동적 경로·외부 Actor 소유관계를 확인합니다.
3. 캐시·기록·사용자 저장·백업을 구분하고 삭제 계획을 작은 변경 묶음으로 만듭니다.
4. 정리 범위에서 필요한 빌드·Cook·실행 검증을 수행합니다. 현재 4단계 삭제는 시작하지 않았습니다.

## 3단계 재현

`Scripts/WholeProjectAuditUnreal.py`의 `prepare()`를 MCP `util.execute_python`으로 실행하고 출력 실행 경로를 기록합니다.
같은 경로로 `dependencies(run, limit=1000)`, `blueprints(run, limit=15)`를 완료까지 반복한 뒤 `bindings(run)`을 실행합니다.
배치 완료 후 체크포인트를 갱신합니다. timeout은 즉시 재호출하지 말고 `state.json`의 cursor와 Editor 응답을 먼저
확인합니다. 실행 중인 배치와 겹쳐 호출하지 않습니다.

`WholeProjectGraphSupplement.py`의 `collect(run, limit=300)`도 MCP로 반복 실행합니다. JSON 조회기가 지원하지 않는
그래프는 Unreal의 `ObjectExporterT3D`로 읽기 내보내기하며 에셋을 저장하지 않습니다. 컴파일 파생 그래프는 제외 목록에 기록합니다.
수집기 또는 콘텐츠가 변경되면 기존 결과와 섞지 않도록 새 실행을 시작합니다.

일반 셸에서 다음을 실행합니다.

```powershell
python Scripts/AnalyzeWholeProjectAudit.py docs/repository-audit/audit/generated/whole-RUN_ID
python Scripts/WriteWholeProjectCatalogs.py
python Scripts/VerifyWholeProjectAudit.py
```

카탈로그 작성기는 목록과 문서의 조사 주석을 생성합니다. SystemMap/DataAuthority/Findings의 사람 검토 내용은
자동 갱신하지 않으므로 새 수집 결과와 대조해야 합니다. 문서의 경로·에셋 이름·테스트 문자열 대조 결과는
`docs/repository-audit/architecture/document-review.json`에 있습니다. 이름 0건은 배치 Actor 라벨·생성 클래스 `_C`일 수 있고,
파일 경로 미발견은 Unreal 가상 경로·축약 표기일 수 있으므로 오류로 단정하지 않습니다.

BGM 시험 조사에서 UE 5.6.1의 `AssetRegistryDependencyOptions`는 생략된 hard/soft package 옵션을
true로 두는 것을 확인했습니다. 종류별 수집은 다섯 옵션을 모두 명시하고 하나만 켭니다.
예시는 `Scripts/AuditBGMSystem.py`에 있습니다. 1단계 `baseline.json`은 당시 이력으로 유지하고,
현재 진척은 별도 `progress.json`에서 관리합니다.

정리 단계까지 `Saved` 전체나 `QuakeLike_Base` 전체를 일괄 삭제하지 않습니다. `Saved/Analyze`의 경기 DB,
`SaveGames`, 백업·추적 자료는 재생성 캐시와 구분해야 합니다. `QuakeLike_Base` 맵은 Cook 설정에 포함되어 있습니다.
참조 0건, Git ignored, 오래된 수정 시각만으로 삭제를 결정하지 않습니다.

## 발견한 후속 검토 항목

- Git에 `Docs/`와 `docs/` 경로가 혼재합니다. Windows에서는 같은 폴더로 보이지만 대소문자를 구분하는
  환경에서는 링크와 경로가 달라질 수 있습니다. 새 문서 링크는 Git의 경로 표기를 따르며, 통일은 후속 정리 대상입니다.
- 루트 `AGENTS.md`는 `.gitignore`의 예외로 두어 조사 문서의 위치·읽는 순서·갱신 방법을 함께 버전 관리할 수 있게 했습니다.
- Git 추적 목록에 Python `__pycache__/*.pyc`가 있습니다. MCP 사용만으로 변경될 수 있으므로
  정리 단계에서 추적 제외 여부를 검토해야 합니다. 이번 단계에서는 기존 추적 정책을 변경하지 않습니다.
