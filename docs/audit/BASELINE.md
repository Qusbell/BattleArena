# 저장소 조사 기준선

- 수집 시작: 2026-11-05T10:40:26.913915+00:00 (UTC)
- 기준 Git 커밋: `cdaf33cd0af41b26226781ee064297b2db8b3fb8` / 브랜치 `main`
- 실행 데이터: `docs/audit/generated/20261105T104026913682Z` (로컬 재생성 자료, Git 제외)
- 파일: 36,264개 / 논리 용량 81.729 GiB
- 파일 조회 오류: 0건
- 디스크 프로젝트 패키지 후보: 28,198개
- 에셋 객체: 28211개
- 디스크/Registry 비교: {"disk_only": 0, "registry_only": 0}

## 상위 폴더 용량

| 폴더 | 파일 수 | GiB |
|---|---:|---:|
| `Content` | 28,384 | 44.302 |
| `Saved` | 2,700 | 13.031 |
| `.vs` | 53 | 12.157 |
| `Intermediate` | 1,547 | 9.432 |
| `Binaries` | 36 | 1.315 |
| `Plugins` | 756 | 0.777 |
| `DerivedDataCache` | 69 | 0.663 |
| `Tools` | 2,279 | 0.034 |
| `SourceArt` | 15 | 0.010 |
| `docs` | 42 | 0.004 |
| `Source` | 319 | 0.002 |
| `GamePlayDB` | 6 | 0.001 |
| `ExternalData` | 1 | 0.000 |
| `ShootingArena.sln` | 1 | 0.000 |
| `Scripts` | 20 | 0.000 |

## Saved 하위 폴더 용량

| 폴더 | 파일 수 | GiB |
|---|---:|---:|
| `Profiling` | 35 | 6.519 |
| `TraceAnalysis` | 199 | 3.372 |
| `Logs` | 465 | 1.335 |
| `DeathSpikeInvestigation` | 15 | 0.832 |
| `Crashes` | 1,177 | 0.588 |
| `Autosaves` | 300 | 0.129 |
| `CodexHudRefactorBackup` | 17 | 0.076 |
| `CodexGraphToolsPackage` | 14 | 0.054 |
| `Codex` | 16 | 0.035 |
| `AnalyzerMergeBackups` | 52 | 0.019 |
| `<root>` | 11 | 0.015 |
| `GraphAudit` | 76 | 0.011 |
| `Analyze` | 25 | 0.011 |
| `Screenshots` | 5 | 0.006 |
| `InventoryStage3` | 19 | 0.005 |

## 진행 상태

```json
{
  "filesystem": "complete",
  "asset_registry": "complete",
  "dependencies": "not_started",
  "roles": "not_started",
  "cleanup": "not_started"
}
```

이 기준선은 파일 메타데이터와 저장된 에셋의 Registry 목록 조사입니다. 사용 여부,
역할, 의존 관계와 삭제 안전성은 판정하지 않았습니다. Engine 및 엔진 플러그인 에셋,
저장 전 에셋, 실행 중 Actor 인스턴스는 Registry 수집 범위에 포함하지 않습니다.
용량은 압축/중복 제거를 고려한 실제 디스크 점유량이 아니며, 수집 중 Editor가
기록하는 로그 등은 변경될 수 있습니다. 제외 경로와 오류는 실행 데이터에 기록합니다.
