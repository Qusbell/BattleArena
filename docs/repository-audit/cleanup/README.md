# 로직 삭제 후보 명세 — TestMainMenu_Level 기준

2026-11-05 조사입니다. **삭제하지 않았습니다.** 단순 아트 리소스가 아닌 게임 로직·UI 로직·입력·BT/EQS/StateTree·데이터 정의·테스트 맵을 대상으로 분류했습니다.
현재 명세는 448개이며 [CSV 전체 목록](candidates.csv), [JSON 근거](candidates.json), [검증 요약](summary.json)을 제공합니다.

## 기준과 해석

진입점은 `/Game/QuakeLike_1_0/GameFlow/MainLevel/TestMainMenu_Level`입니다. MCP로 조회한 맵의 GameMode override는
`/Game/QuakeLike_1_0/GameFlow/Lobby/BP_MenuGameMode.BP_MenuGameMode_C`입니다.

“의존성이 없다”는 **다른 패키지나 알려진 호출자가 이 항목에 의존하지 않는다**는 뜻입니다. 후보 자신이 Engine/native 클래스나
다른 에셋을 참조하는 outgoing dependency가 0이라는 뜻은 아닙니다. 부모 클래스까지 함께 삭제한다는 뜻도 아닙니다.

| 분류 | 수량 | 기준 |
|---|---:|---|
| 1. 정적 근거상 독립된 우선 제거 후보 | 129 | 현재 역참조 0, BP 문자열 참조 0, 알려진 코드/테스트/도구의 경로·이름 참조 0, 보호된 진입 흐름 밖 |
| 2. 의존성이 얇은 후보 | 100 | 보호된 흐름 밖이며 직접 참조자 2개 이하, 간접 로직 영향 5개 이하, 전체 간접 패키지 영향 10개 이하 |
| 3. 레거시/미연결 프로토타입 묶음 검토 후보 | 219 | 보호된 흐름 밖이지만 연결 폭이 크거나 별도의 구형 기능 묶음이며, 기능/테스트/수동 업무 제거 판단이 필요 |

1번은 정적 근거로 바로 제거 순서를 잡을 수 있는 목록입니다. **실제 삭제·Cook·실행 회귀를 하지 않았으므로 무조건 무영향이라는
실행 검증까지 끝낸 목록은 아닙니다.** 임의 콘솔/외부 입력으로 경로를 직접 구성하는 경우나 수동 Editor 사용 의도까지 증명하지는 않습니다.

2번도 개별 파일만 먼저 지우면 남은 참조자의 로드/컴파일이 깨질 수 있습니다. 아래에 적은 참조자부터 제거하거나 참조를 해제합니다.
3번의 “레거시”는 폴더명만으로 확정한 것이 아니라 현재 흐름 밖이라는 사실과 계통/역할을 조합한 판단이며, 일부는 보류 중인 프로토타입일 수 있습니다.

## 1. 우선 제거 후보 — 대표 항목

전체 129개는 [분류 1 전체 목록](lists/tier-1.md)에 있습니다. 아래 항목의 역참조와 알려진 소스 참조는 모두 0건입니다.

| 패키지 경로 | 확인한 역할 |
|---|---|
| `/Game/QuakeLike_1_0/Controller/BP_QuakeAIController` | AddTarget/RemoveTarget/Debug_TargetLine을 가진 별도 Controller. 현행 `AI/AIC/BP_QuakeAIController2`와 다릅니다. |
| `/Game/QuakeLike_1_0/AI/BP/BPC_AIWeaponSelection1` | GetBestWeaponScore/InitializeWeaponSelection을 가진 별도 무기 선택 컴포넌트 |
| `/Game/QuakeLike_1_0/AI/Area/BPC_AreaRiskComponent` | Area Manager에 Recognize Unit/Mark Combat Detected를 전달하는 미연결 컴포넌트 |
| `/Game/QuakeLike_1_0/AI/Area/BTS_SelectRetreatMovePosition` | 후퇴 위치 계산 BT Service |
| `/Game/QuakeLike_1_0/AI/Area/BTT_SelectRetreatMovePosition` | 후퇴 위치 계산 BT Task |
| `/Game/QuakeLike_1_0/AI/Area/BTT_ClearRetreatState` | 후퇴 Blackboard 상태 정리 Task |
| `/Game/QuakeLike_1_0/AI/BTT/BTTask_Unstuck` | 미연결 Unstuck Task |
| `/Game/QuakeLike_1_0/AI/EQS/EQS_RetreatPosition` | 현재 어떤 패키지도 참조하지 않는 후퇴 EQS |
| `/Game/QuakeLike_1_0/AI/Hiding/EQS_GetHidingLocation` | 현재 어떤 패키지도 참조하지 않는 은신 위치 EQS |
| `/Game/QuakeLike_1_0/Data/PDA/PDA_AIGameMode` | 현재 어떤 패키지도 참조하지 않는 AI 설정 schema |

이 목록에 Area 또는 AI라는 이름이 있다고 해서 native `UAreaManagerSubsystem`, `AMyAIController` 등을 지워서는 안 됩니다.
예를 들어 현행 Controller2도 native Controller 계층을 사용합니다. 여기서는 **지정된 에셋 정의만** 후보입니다.

## 2. 얇은 의존 관계 — 대표 항목과 제거 순서

전체 100개는 [분류 2 전체 목록](lists/tier-2.md)에 있습니다. 표의 참조자는 모두 현재 보호 흐름 밖입니다.

| 후보 | 현재 직접 참조자 | 전체 간접 영향 |
|---|---|---:|
| `/Game/QuakeLike_1_0/AI/BP/BP_DroneTarget` | `Controller/BP_QuakeAIController` | 1개 패키지 |
| `/Game/QuakeLike_1_0/AITest_YJS/Trash/BT_AIaction` | `Controller/BP_QuakeAIController` | 1개 패키지 |
| `/Game/QuakeLike_1_0/AI/BTS/BTS_UpdateDroneLocation` | `AITest_YJS/Trash/BT_AIaction` | 2개 패키지 |
| `/Game/QuakeLike_1_0/AI/BTT/BTTask_MoveToTarget_HBC` | `AITest_YJS/Trash/BT_AIaction` | 2개 패키지 |
| `/Game/QuakeLike_1_0/AI/BTT/BTTask_TryUse` | `AITest_YJS/Trash/BT_AIaction` | 2개 패키지 |
| `/Game/QuakeLike_1_0/AI/Data/ST_AISetting` | `Data/PDA/PDA_AIGameMode` | 1개 패키지 |
| `/Game/QuakeLike_1_0/AI/EQS/EQC_RetreatAnchor` | `AI/EQS/EQS_RetreatPosition` | 1개 패키지 |
| `/Game/QuakeLike_1_0/AI/Hiding/EQC_Target` | `AI/Hiding/EQS_GetHidingLocation` | 1개 패키지 |

현재 의존 관계에 따른 제거 순서의 예시는 다음과 같습니다.

```text
BP_QuakeAIController
  → 위 항목 제거 후 BP_DroneTarget / BT_AIaction
  → BT_AIaction 제거 후 BTS_UpdateDroneLocation / BTTask_MoveToTarget_HBC / BTTask_TryUse

PDA_AIGameMode → ST_AISetting
EQS_RetreatPosition → EQC_RetreatAnchor
EQS_GetHidingLocation → EQC_Target
```

화살표는 왼쪽 참조자를 먼저 정리한다는 의미입니다. 실제 삭제 시점에는 역참조를 다시 확인합니다.

## 3. 레거시/프로토타입 후보 — 기능 단위로 검토

전체 219개는 [분류 3 전체 목록](lists/tier-3.md)에 있습니다.

| 묶음 / 대표 객체 | 직접 참조자 | 간접 로직 / 전체 패키지 영향 | 판단 근거와 남는 영향 |
|---|---:|---:|---|
| `QuakeLike_Base/BGH/BP_BGH_BaseChar` | 9 | 55 / 224 | HBC/LJH/PJH/YJS 구형 구현의 공유 기반입니다. 현재 보호 흐름에는 연결되지 않지만 하나만 지우면 구형 기능군이 함께 깨집니다. |
| `QuakeLike_Base/LJH/Item/BP_LJH_GameState` | 2 | 55 / 224 | 구형 ItemSpawner/PlayerSpawner가 참조하는 스폰·재스폰 큐입니다. 직접 참조 2개만 보고 2번으로 분류하지 않았습니다. |
| `QuakeLike_Base/LJH/Life/BPC_BGH_Life` | 7 | 55 / 224 | 구형 피해·회복·사망·과충전 구현이며 위 구형 캐릭터 기능군과 얽혀 있습니다. |
| `QuakeLike_Base/PJH/BP_HeldItemBase` | 20 | 55 / 224 | 구형 장착 아이템 기반이며 여러 구형 무기/투사체/객체 풀과 연결됩니다. |
| `QuakeLike_Base/PJH/Projectile/BP_PJH_ProjectileBase`, `Weapon/BP_PJH_ProjectileWeaponBase` | 각각 10 | 각각 55 / 224 | PJH 구형 투사체·무기 계통입니다. 현행 Weapon/Projectile BP와 구분해 묶음 검토해야 합니다. |
| `QuakeLike_1_0/Controller/BPC_TargetPriorityStack` | 3 | 3 / 3 | 별도 목표 가중치·LTG/STG 스택입니다. 참조자는 구 Controller와 `Test/BP_StackTester`, `Test/Test_AITargetStack`뿐입니다. 현재 미연결 프로토타입인지 폐기된 구현인지 의도 확인이 필요합니다. |
| `QuakeLike_1_0/JsonParser/*` 및 `Data/{PDA,JsonParser}/*JsonImporterConfig` 4개 | 0~2 | 세부 목록 참조 | 현행 등록 적용은 JsonAssetSync가 담당합니다. 구 EUW는 수동 DT JSON 생성/가져오기 기능이 있어 역참조 0건만으로 즉시 삭제하지 않습니다. |
| `TestMap_Refactor`, `TestMap_Change`, `Test/YesItWillBeNotEasy/TestMap_NonNevMesh`, Base의 HBC/LJH 테스트 맵 | 다수 | 세부 목록 참조 | 구형 배치·테스트 Actor가 맵과 연결됩니다. 테스트 자산을 없애는 영향과 외부 Actor/Object 소유관계를 함께 검토합니다. |

간접 영향의 집합은 서로 겹칩니다. 위 숫자를 합산해 삭제 파일 수나 회수 용량으로 사용하지 않습니다.
이 명세는 폴더 전체 삭제 지시가 아닙니다.

## 삭제 후보에서 제외한 중요 항목

- 현행 `BP_MenuGameMode`, `BP_QuakeGameInstance`, `BP_QuakeAIController2`와 그 보호된 참조 흐름
- Config에 fallback으로 지정된 `BP_FirstPersonGameMode`: 현재 맵 override와 별개로 설정 참조가 남아 있습니다.
- `QuakeLike_Base`의 PortalMap/02Map 등 참조되는 맵
- `QuakeLike_Base/SDM/UI/Combat_UI/WBP_PlayerHUD_sdm`, KillLog/ScoreBoard 계열과 YJS 무기 입력: Base 아래여도 현행 보호 흐름에서 도달합니다.
- `EUW_ArenaAnalyzer`와 시각화 Actor: 현행 문서화된 분석 도구입니다.
- `EUW_UnusedAssetFinder`와 WBP_AssetRow: 수동 실행하는 유지보수 도구이며 폐기 의도를 확인하지 못했습니다.

## 조사 범위와 재현

- 이전 전수조사 커밋 이후 에셋·C++·Config·주요 데이터 JSON의 Git 변경이 없는 것을 확인했습니다. 기존 수집 원본은 수정하지 않았습니다.
- 기본 의존 관계 외에 MCP로 내보낸 Blueprint 문자열 기본값과 중첩 그래프 텍스트, 소스의 경로/이름 상수도 검사했습니다.
- World Partition 외부 Actor/Object는 맵 경로로 소유 World를 연결해 보호 흐름을 보충했습니다.
- GameInstance·서버 시작 맵·명시적 설정·실제 JSON binding·원본 JSON·runtime 코드의 명시 경로를 보수적으로 보호했습니다.
- 후보 452개의 현재 hard/soft/management 역참조를 MCP로 재조회했으며 이전 참조와 차이는 0건입니다. 수동 도구와 그 의존 객체 4개를 보류해 최종 후보는 448개입니다.
- 맵의 LevelScript 전체 의미, CDO의 모든 문자열, 임의 외부 입력으로 조합하는 경로와 미래 수동 사용 의도는 완전히 증명하지 않았습니다.
- Mesh·Material·Texture·Sound·애니메이션 클립·UI 스타일과 환경/캐릭터 아트팩 전시용 BP/맵은 제외했습니다. 목록에 있는 AnimBP는 포즈/상태 제어 정의이며 단순 클립 리소스와 구분합니다.
- 로직 에셋 949개 중 501개를 보호/보류했습니다. 이 숫자는 프로젝트 전체 아트 에셋이나 native 타입 개수와 비교하는 값이 아닙니다.

재현 도구: `Scripts/PrepareLogicCleanupReview.py` → MCP에서 `Scripts/CollectLogicCleanupReview.py`의 `collect(run)` →
`Scripts/WriteLogicCleanupReview.py <run>`입니다. 상세 실행 경로·해시는 `summary.json`에 기록합니다.
현재 dirty 패키지는 0건이며 실제 삭제·저장·컴파일·PIE·Cook을 수행하지 않았습니다.
