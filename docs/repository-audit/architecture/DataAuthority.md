# 설정 데이터의 원본과 편집 기준

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
