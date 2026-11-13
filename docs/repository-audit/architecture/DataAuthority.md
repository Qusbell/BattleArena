# 설정 데이터의 원본과 편집 기준

2026-11-13 추가. 좌하단 HP·아머 변경 연출의 원본은 `/Game/QuakeLike_1_0/Data/UI/DA_VitalsHUDFeedback`입니다.
피격·비피격 각각에 대해 숫자 잔상과 가로 색상 잔상의 불투명도와 블러를 에디터에서 직접 편집하고 저장합니다.
이 에셋은 JsonAssetSync에 등록하지 않습니다.
`WB_HealthBar`와 `WB_ArmorBar`는 기존 갱신 이벤트의 피격 구분을 사용합니다.
가로 색상 잔상은 비피격 갱신 시 `WB_HealthAnim`, 피격 시 `WB_DamageAnim`에서 재생됩니다.
초기 불투명도는 숫자·색상 순서로 비피격 0.45·0.30, 피격 0.65·0.50입니다. 초기 블러는 모두 0입니다.

2026-11-12 보강: 언어 리소스의 native 도구는 형식을 검증한 바이트만 생성합니다. `Scripts/Localization/CompileLocalization.py`가 전체 언어 검증 후 임시 파일과 복구용 파일을 준비하고 locres를 파일별로 교체합니다. 저장 오류 시 이미 교체한 파일을 복구하며, 원본은 계속 String Table·PO입니다. 잘못된 PO와 엔진 형식 인수는 실행 파일을 수정하기 전에 거부합니다.

2026-11-13 현황: 언어 목록·기본 언어·Font는 `/Game/QuakeLike_1_0/Localization/DT_Language`이며 현재 JsonAssetSync Registry의 `Content/DataTables/DT_Language.json`과 연결되어 있습니다. 이 등록은 UI 배치 수정 시작 전에 이미 존재했으므로 보존했습니다. 번역 원문은 같은 폴더의 `ST_Game`, 번역은 `Content/Localization/Game`의 Dashboard archive/PO, 실행 산출물은 locres/locmeta입니다. 사용자 선택은 `GGameUserSettingsIni`의 `[Internationalization] Culture`이며 기존 PlayerSettings SaveGame과 분리합니다. 메인 메뉴 바로가기와 게임 플레이 탭은 같은 저장값을 사용합니다. 초기 이관 seed와 스크립트는 실행·편집 원본이 아니며 저장소 밖의 작업 백업으로 분리했습니다. `DT_MapData.json`의 기존 설명 4개는 `LOCTABLE` 참조로 바꾸었으므로 에셋만 수정하지 않습니다. `DT_MultiRules.Label`은 에셋 원본이고 `Unit` 내부 String은 유지합니다. [언어 시스템](../systems/LanguageSystem.md)을 참조합니다.

2026-11-11 추가. `DA_KillMedalUISettings`는 `Content/DataAssets/DA_KillMedalUISettings.json`에 연결됩니다.
메달과 처치 알림의 색상·불투명도·그림자를 DA에서 편집하고 저장하면 JsonAssetSync가 JSON에 내보냅니다.
다음 Editor·런타임 시작은 이 JSON을 적용합니다. [킬 알림 스타일과 검증 방법](../systems/UIRecording.md#2026-11-11-킬-메달과-처치-알림-스타일)

2026-11-10 추가: 로컬 에임 어시스트의 수렴 시간은 `UAimAssistSettings`/`DefaultGame.ini`의 `[/Script/ShootingArena.AimAssistSettings]`가 원본입니다. Project Settings > Game > Aim Assist에서 조정합니다. JsonAssetSync에 연결하지 않으며 활성화 상태는 컨트롤러별 메모리에만 보관합니다. [시스템 문서](../systems/AimAssist.md)를 참조합니다.

2026-11-09 추가: 촬영용 `/Game/QuakeLike_1_0/UI/Recording/DA_UIRecordingSettings`는 DA 에셋 자체가 원본입니다. JsonAssetSync에 등록하지 않았으므로 이 에셋의 체크박스를 직접 편집·저장합니다. [촬영 UI](../systems/UIRecording.md).

2026-11-05 조사에서 실제 `DA_JsonApplyRegistry` binding 29개와
`ExternalData/JsonAssetSyncManifest.json`의 source/target 29쌍이 일치했습니다.
원본 파일·대상 에셋 누락은 0건입니다. 전체 목록은 [data-bindings.json](data-bindings.json)에 있습니다.

## 원본 → 적용 대상

| 원본 | 대상/용도 |
|---|---|
| `Content/DataTables/DT_Character.json`, `DT_Characteristic.json` | 캐릭터·AI 특성 |
| `DT_MapData.json` | 메뉴/경기 맵 데이터 |
| `DT_Item_Weight.json`, `DT_WeaponChange_Weight.json` | AI 아이템·무기 선택 가중치 |
| `DT_HeldItemTable.json`, `DT_BuffTable.json`, `DT_LifeItemTable.json`, `DT_AmmoItemTable.json` | 아이템 4종 DropTable, 총 25행 |
| `DT_DropItemSetting.json` | 사망 드롭 설정 |
| `Content/DataAssets/DA_*Gun.json`, `DA_RocketLauncher.json`, `DA_Pistol.json` | 무기 설정 |
| `DA_MatchSetting.json`, `DA_Respawn.json`, `DA_DefaultPlayer.json` | 경기·리스폰·플레이어 |
| `DA_DefaultLife.json`, `DA_MovementOption.json` | Life·이동; 대상 폴더 이름은 실제로 `Data/Chracter`입니다. |
| `DA_DeathCamera_Default.json`, `DA_KillStreakSettings.json`, `DA_ShooterOutline_Default.json`, `DA_ShooterNickname_Default.json` | 카메라·음성·표시 |
| `DA_BGMSetting_Default.json` | BGM |
| `DA_AIAimBoundary.json`, `DA_InterpSpeed.json`, `DA_DefaultAggression.json` | AI 조준·보간·공격 성향 |

위 표의 파일은 `DataTables`/`DataAssets` 폴더 기준이며 완전한 경로·대상 클래스는 목록 JSON을 기준으로 합니다.
현재 Registry에는 DataTable 10개·DataAsset 19개가 있고 CurveTable/FloatCurve binding은 0개입니다.

## 직접 DT/DA 편집 시 주의할 적용 순서

현재 `applyOnEditorStartup=True`, `editorApplyMode=MemoryOnly`, `applyOnRuntimeStartup=True`입니다.
`UJsonAssetSyncSubsystem`은 EngineSubsystem이며 패키징 런타임에서는 OnPostEngineInit에서 초기 적용을 요청합니다.
에셋의 저장된 값, Editor 메모리 값, 외부 JSON 원본은 서로 다른 시점의 값일 수 있습니다.

지속할 변경은 **원본 JSON과 대상 에셋의 연결을 함께 관리**합니다. Editor에서 DT/DA만 저장하면 다음 시작에
JSON 값이 다시 적용될 수 있습니다. MemoryOnly는 디스크 저장을 하지 않는 적용 모드이며 JSON이 무시된다는 뜻이 아닙니다.
이 조사에서는 JSON 적용·재저장·데이터 값 변경을 수행하지 않았습니다.

Manifest는 연결/스키마의 보조 자료이며 실제 Registry와 소스 파일이 기준입니다. 사용자 작업 중인 Manifest 변경을
자동 생성으로 덮어쓰지 않습니다. 새 행/프로퍼티 추가는 기존 JsonAssetSync 등록·검증 경로를 따릅니다.

## 코드·경로 근거

- `Config/DefaultGame.ini`: Registry, 적용 모드, AlwaysCook와 NonUFS stage 설정
- `Plugins/JsonAssetSync/Source/JsonAssetSync/Private/JsonAssetSyncSubsystem.cpp`: 초기 적용 시점
- `Plugins/JsonAssetSync/Source/JsonAssetSync/Public/JsonApplyRegistry.h`: binding 형식
- [실제 binding 목록](data-bindings.json): 대상 경로·DataTable 행·컬럼

존재/연결이 일치한다는 결과가 모든 데이터 값의 적절성·범위·런타임 적용 성공을 증명하지는 않습니다.
