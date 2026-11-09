# 전수조사 객체 카탈로그

기준 커밋 `6956f5d2bec90ea7171da34cb48a117a2211b944` / 조사일 2026-11-05 / [진행 상태](../audit/progress.json)

소스 선언과 Blueprint 정의는 아래 그룹에서 찾습니다. 리소스·맵·외부 Actor 레코드를 포함한 전체 에셋은
`docs/repository-audit/audit/generated/whole-20261105T110518059081Z/asset-roles.csv`에 클래스 기반 역할, 분류 추론, 도달 관계를 함께 기록했습니다.

| 분류 추론 | C++ 타입 | Blueprint |
|---|---:|---:|
| [AI.Decision](catalogs/AI-Decision.md) | 6 | 66 |
| [AI.Navigation](catalogs/AI-Navigation.md) | 61 | 14 |
| [Audio.Camera](catalogs/Audio-Camera.md) | 19 | 3 |
| [Character](catalogs/Character.md) | 21 | 12 |
| [Combat.Weapons](catalogs/Combat-Weapons.md) | 47 | 52 |
| [Data.Config](catalogs/Data-Config.md) | 28 | 27 |
| [GameFlow.Network](catalogs/GameFlow-Network.md) | 34 | 33 |
| [Items.Buffs](catalogs/Items-Buffs.md) | 13 | 35 |
| [Movement.World](catalogs/Movement-World.md) | 20 | 25 |
| [Other](catalogs/Other.md) | 114 | 345 |
| [Telemetry.Debug](catalogs/Telemetry-Debug.md) | 32 | 2 |
| [UI](catalogs/UI.md) | 39 | 309 |

## 해석 기준

- C++ 타입에는 class·struct·enum 선언이 포함됩니다. 자동 추출이며 완전한 C++ AST 분석은 아닙니다.
- Blueprint는 클래스 정의입니다. 레벨에 배치된 인스턴스의 설정과 같다고 가정하지 않습니다.
- 외부 Actor/Object 패키지는 Asset Registry 레코드와 의존 관계로 조사했습니다. 맵의 모든 인스턴스 속성을 로드하지 않았습니다.
- Texture·Mesh·Material·Sound 등은 클래스별 리소스 역할을 전수 기록했습니다. 미술적 의도는 이름만으로 확정하지 않았습니다.
- 동적 문자열 로딩·실제 실행·Cook 성공은 별도 검증이 필요합니다. 도달하지 못한 에셋을 삭제 대상으로 확정하지 않습니다.

## 자료

| 자료 | 내용 |
|---|---|
| [SystemMap.md](SystemMap.md) | 사람이 검토한 시스템 경계와 실제 연결 |
| [DataAuthority.md](DataAuthority.md) | JSON과 에셋 편집 기준 |
| [Findings.md](Findings.md) | 정리 전 확인할 구체적 발견 |
| [blueprint-index.csv](blueprint-index.csv) | Blueprint 경로·부모·그래프 색인 |
| [native-types.json](native-types.json) | 소스 타입·주석·위치 |
| [data-bindings.json](data-bindings.json) | 실제 Registry binding |
| [module-dependencies.json](module-dependencies.json) | Build.cs 모듈 의존 문자열 |
| [plugins.json](plugins.json) | 로컬 플러그인 선언 |
| [workflows.json](workflows.json) | 스크립트·실행 도구의 원본 설명/파일 색인 |
| [maps.json](maps.json) | 전체 World 패키지 연결 |
| [document-review.json](document-review.json) | 기존 문서 분류·대조 기록 |
| [audit-summary.json](audit-summary.json) | 범위·수치·소스 해시 |
