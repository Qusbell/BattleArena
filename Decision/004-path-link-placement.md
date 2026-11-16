# 두 맵의 Path Link 배치 보정

2026-11-16 결정.

- `/Game/QuakeLike_Base/Lvl_02Map`과 `/Game/QuakeLike_Base/Lvl_PortalMap`에서 현재 저장본의 검증에 실패한 Path Link를 보정합니다.
- 진입점 높이(Z)를 먼저 맞춥니다. 높이만으로 해결되지 않을 때 원래 발판 안에서 진입점 XY를 최소한으로 조정합니다. 연결 대상과 링크 종류는 유지합니다.
- 두 맵을 저장한 뒤 데디케이티드 서버 게임 월드에서 기존 Path Link 도구로 검사합니다. 배치와 NavMesh 검사를 모두 통과하고 실패한 링크가 0개이면 완료합니다.

## 검증 결과

`Attach To Ground`로 `Lvl_02Map`의 11개 링크와 `Lvl_PortalMap`의 `BP_PathLink19`를 높이만 조정했습니다. 저장본을 로컬 데디케이티드 서버에서 열어 `ValidateAndLog`를 실행한 결과 각각 39/39개와 33/33개가 통과했습니다. 두 맵에서 저장된 경로 캐시도 로드됐습니다.
