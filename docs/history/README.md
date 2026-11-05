# 과거·보류 문서 보관소

현행 작업의 기준은 [저장소 조사 문서](../repository-audit/README.md)와 [현재 기능 문서](../README.md)입니다.
아래 자료는 당시 경위·설계 의도·측정 이력을 보존하기 위해 모았습니다. 보관했다는 이유만으로 과거 기록이 거짓이라는 뜻은 아닙니다.
다만 현재 구현과의 일치나 재현이 보장되지 않으므로 그대로 적용하거나 현재 테스트 통과의 증거로 사용하지 않습니다.

## 분류

- `legacy/`: 현재 경로·구조에 적용할 수 없는 사용 안내입니다.
- `plans/`: 당시 설계·개선·적용 제안입니다. 현재 적용 상태를 별도로 확인해야 합니다.
- `experiments/`: 과거 측정·배치·원인 분석 결과입니다. 당시 조건을 확인해야 합니다.
- `changes/`: 과거 구현·수정 이력입니다. 현행 규범을 대신하지 않습니다.

## 문서와 보관 이유

| 문서 | 원래 위치 | 보관 이유 |
|---|---|---|
| [PathLink27PlacementCheck.md](experiments/PathLink27PlacementCheck.md) | `docs/PathLink27PlacementCheck.md` | 과거 측정·배치·원인 분석 기록; 현재 조건에서 재실행하지 않았습니다. |
| [PathLinkAccuracyNextSteps.md](plans/PathLinkAccuracyNextSteps.md) | `docs/PathLinkAccuracyNextSteps.md` | 과거 개선 계획입니다. 현재 적용 완료 여부를 보장하는 문서가 아닙니다. |
| [PathLinkBlendResults.md](experiments/PathLinkBlendResults.md) | `docs/PathLinkBlendResults.md` | 과거 측정·배치·원인 분석 기록; 현재 조건에서 재실행하지 않았습니다. |
| [PathLinkBotAPIIntegration.md](plans/PathLinkBotAPIIntegration.md) | `docs/PathLinkBotAPIIntegration.md` | 당시 설계·적용 제안이며 이후 코드/캐시 구조/적용 상태와 차이가 있을 수 있습니다. |
| [PathLinkCacheConsistencySpecification.md](plans/PathLinkCacheConsistencySpecification.md) | `docs/PathLinkCacheConsistencySpecification.md` | 당시 설계·적용 제안이며 이후 코드/캐시 구조/적용 상태와 차이가 있을 수 있습니다. |
| [PathLinkDirectionCorrectionResults.md](experiments/PathLinkDirectionCorrectionResults.md) | `docs/PathLinkDirectionCorrectionResults.md` | 과거 측정·배치·원인 분석 기록; 현재 조건에서 재실행하지 않았습니다. |
| [PathLinkLargePolygons.md](experiments/PathLinkLargePolygons.md) | `docs/PathLinkLargePolygons.md` | 과거 측정·배치·원인 분석 기록; 현재 조건에서 재실행하지 않았습니다. |
| [PathLinkLazySearchResults.md](experiments/PathLinkLazySearchResults.md) | `docs/PathLinkLazySearchResults.md` | 과거 측정·배치·원인 분석 기록; 현재 조건에서 재실행하지 않았습니다. |
| [PathLinkOutlierRootCause.md](experiments/PathLinkOutlierRootCause.md) | `docs/PathLinkOutlierRootCause.md` | 과거 측정·배치·원인 분석 기록; 현재 조건에서 재실행하지 않았습니다. |
| [PathLinkPlayableBlendSweep.md](experiments/PathLinkPlayableBlendSweep.md) | `docs/PathLinkPlayableBlendSweep.md` | 과거 측정·배치·원인 분석 기록; 현재 조건에서 재실행하지 않았습니다. |
| [PathLinkPlayableResults.md](experiments/PathLinkPlayableResults.md) | `docs/PathLinkPlayableResults.md` | 과거 측정·배치·원인 분석 기록; 현재 조건에서 재실행하지 않았습니다. |
| [PathLinkRandomResults.md](experiments/PathLinkRandomResults.md) | `docs/PathLinkRandomResults.md` | 과거 측정·배치·원인 분석 기록; 현재 조건에서 재실행하지 않았습니다. |
| [PathLinkRoutingCache.md](plans/PathLinkRoutingCache.md) | `docs/PathLinkRoutingCache.md` | 당시 설계·적용 제안이며 이후 코드/캐시 구조/적용 상태와 차이가 있을 수 있습니다. |
| [PathLinkRoutingCacheResults.md](experiments/PathLinkRoutingCacheResults.md) | `docs/PathLinkRoutingCacheResults.md` | 과거 측정·배치·원인 분석 기록; 현재 조건에서 재실행하지 않았습니다. |
| [PathLinkRuntimeCacheFix.md](changes/PathLinkRuntimeCacheFix.md) | `docs/PathLinkRuntimeCacheFix.md` | 과거 구현 변경·수정 이력; 현행 동작 규범으로 사용하지 않습니다. |
| [PathLinkSTGInlineLTG.md](changes/PathLinkSTGInlineLTG.md) | `docs/PathLinkSTGInlineLTG.md` | 과거 구현 변경·수정 이력; 현행 동작 규범으로 사용하지 않습니다. |
| [BuffWarningSound_ImplementationNotes.md](changes/BuffWarningSound_ImplementationNotes.md) | `docs/BuffWarningSound_ImplementationNotes.md` | 버프 예고음의 구현·임시 설정·당시 검증 이력입니다. 현행 사용법은 BuffWarningSound.md를 읽습니다. |
| [Buff_Stabilization_Checklist.md](plans/Buff_Stabilization_Checklist.md) | `docs/Buff_Stabilization_Checklist.md` | 과거 안정화 점검 계획이며 현재 완료 상태나 테스트 통과를 보장하지 않습니다. |
| [PortalMapCollisionRepair.md](changes/PortalMapCollisionRepair.md) | `docs/PortalMapCollisionRepair.md` | 맵 충돌·내비메시 수정 이력입니다. 현행 맵 전체를 다시 검증하지 않았습니다. |
| [TextureOptimizationReview.md](experiments/TextureOptimizationReview.md) | `docs/TextureOptimizationReview.md` | 과거 텍스처 최적화 측정 기록입니다. 연결된 백업/비교 자료가 현재 워크스페이스에 없어 재검증 근거가 불완전합니다. |
| [Life관련 사용 설명서.txt](legacy/Life%EA%B4%80%EB%A0%A8%20%EC%82%AC%EC%9A%A9%20%EC%84%A4%EB%AA%85%EC%84%9C.txt) | `Manual/Life관련 사용 설명서.txt` | QuakeLike_Base/LJH 기준의 폐기된 사용 안내입니다. 현행 Life와 스폰 경로를 사용해야 합니다. |

## 근거 자료가 없는 기록

`TextureOptimizationReview.md`가 가리키던 TextureMipBackups, TextureMipComparison의 비교 이미지와 집계 JSON은
현재 워크스페이스에 없습니다. 해당 참조는 원래 경로를 텍스트로 남기고 자료 부재를 표시했습니다.
원본 위치·분류·해시는 [index.json](index.json)에 기록합니다. 삭제된 에셋이나 캐시가 있다는 의미는 아닙니다.
