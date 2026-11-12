# 촬영용 UI 표시 설정

설정 에셋: `/Game/QuakeLike_1_0/UI/Recording/DA_UIRecordingSettings`.
15개 체크박스의 기본값은 모두 표시입니다. 전투(체력·방어구·무기), 타이머, 상단 순위,
Tab 점수판, 조준점·히트마커·줌, 킬 알림, 메달, 버프, 피격 효과, 사망·리스폰,
메뉴·로비, 결과, 닉네임, 커스텀 UI 커서, 로딩·맵 전환을 각각 제어합니다.

촬영 전에 DA를 열고 필요한 항목만 체크한 뒤 저장합니다. 에디터 PIE 실행 중 같은 DA의 체크값을
바꾸면 생성된 패널도 다음 그리기부터 반영합니다. 패키징 실행은 시작 시 저장된 DA 값을 사용합니다.
별도 런타임 체크 메뉴·단축키는 추가하지 않았습니다.

`URecordingUIMask`는 기존 위젯 바깥에서 Slate 그리기 스타일의 투명도를 0으로 만들고 그리기 영역을 비웁니다. 자식 위젯의 Visibility,
RenderOpacity, 애니메이션 값·타이머·생성/삭제 흐름과 게임 상태를 변경하지 않습니다.
따라서 감춘 메뉴도 원래대로 입력과 포커스를 처리합니다. 보이지 않는 메뉴를 닫으려면 기존 F10/뒤로가기를
사용합니다. 커스텀 UI 커서 설정은 OS/하드웨어 마우스 커서를 제어하지 않습니다.

58개 Blueprint 연결은 [근거 목록](evidence/ui-recording.json)에 실제 로드된 경로로 기록했습니다.
`QuakeLike_Base` 일부 HUD와 `CompShooterHUD`는 현재 컨트롤러가 참조하므로 함께 연결했습니다.
Native 순위·연속 킬 메달·버프 화면 효과는 생성 시 동일한 패널을 사용하며 닉네임과 로딩 화면도
표시 설정을 읽습니다. 새로운 UI를 추가하면 그 표시 영역에 Recording UI Mask를 넣고 Section을 지정합니다.
복합 HUD의 전체 부모를 감추면 자식 카테고리를 독립적으로 켤 수 없으므로 해당 영역별로 배치합니다.

DA는 **에셋 자체가 설정 원본**입니다. JsonAssetSync Registry/외부 JSON에는 등록하지 않았습니다.
`DefaultGame.ini`의 AlwaysCook에 Recording 폴더를 추가해 문자열 경로로 로드하는 DA를 패키징에 포함합니다.

## 구현·검증 범위

- `Source/ShootingArena/Public/UI/UIRecordingSettings.h`, `Private/UI/UIRecordingSettings.cpp`: 설정과 표시 패널.
- `Scripts/ConfigureRecordingUI.py`: MCP execute_python 전용, 기존 위젯 이름과 이벤트를 유지하는 재실행 가능한 연결 스크립트.
- `ShootingArena.UI.RecordingSettings`: 체크박스 기본값·독립성 검사.
- `ShootingArena.UI.RecordingPaintMask`: 실제 렌더타깃에서 표시→숨김→표시와 자식 표시/투명도 보존 검사.
- `ShootingArena.UI.RecordingRespawnProgress`: 실제 사망바 머티리얼·배경 텍스처로 진행률 0/50/100%에서 표시→숨김→표시와 진행률 보존 검사.
- 2026-11-09: 최종 Editor 일반 빌드 성공, 위 자동 검사 2개 성공, 58개 연결 스크립트 재실행·Blueprint 컴파일/저장 성공. 초기 Live Coding 패치 적용 중 Editor 충돌 후 일반 빌드·재시작으로 복구했으며 최종 MCP 응답을 확인했습니다.
- MainMenu PIE는 시작/종료했지만 ShowUI 캡처가 실제 메뉴 표시를 증명하지 않아 화면 검증 성공으로 기록하지 않습니다.
- 전체 UI 상태 조합과 패키징 실행은 별도 검증 범위입니다. 기존 전체 조사 원본은 재생성하지 않습니다.

## 2026-11-10 사망바 잔상 수정

`WBP_DeathScreen.PB_RespawnBar`의 `M_RespawnFillGradient`가 Opacity에 텍스처 알파만 사용해
부모 Recording UI Mask의 투명도를 무시했습니다. Opacity를 `TextureSample.A * VertexColor.A`로
연결해 DA의 **사망·리스폰 화면** 체크를 해제하면 머티리얼 채움도 함께 숨겨지도록 수정했습니다.
참조 에셋은 `WBP_DeathScreen` 하나로 확인했으며, 리스폰 타이머/게임 로직과 DA 기본값은 변경하지 않습니다.
새 UI 머티리얼도 Opacity에 `VertexColor.A`를 반영해야 이 표시 마스크가 적용됩니다.
일반 Editor 빌드와 Recording 계열 자동 검사 3개가 통과했습니다. 회귀 검사는 실제 사망바의
머티리얼·배경 텍스처로 진행률 0/50/100%에서 숨김 시 투명 픽셀만 남고, 재표시되며,
진행률 값이 유지되는지 확인합니다. 실제 전투 사망 장면과 패키징 실행은 수행하지 않았습니다.

## 2026-11-11 중앙 알림의 사망 우선 처리

현재 사용 중인 `/Game/QuakeLike_Base/SDM/UI/Combat_UI/WBP_PlayerHUD_sdm`의
`ShowCompetitiveDeathMessage`는 기존 중앙 알림을 `RemoveKillMessages`로 제거한 다음 사망 문구를 표시합니다.
`ShowCompetitiveKillMessage`는 소유 컨트롤러의 `deathScreen`이 Visible/HitTestInvisible인 동안 요청을 무시합니다.
기존 `Toggle Death UI(false)`가 화면을 Hidden으로 전환하면 처치 알림이 다시 허용됩니다.
별도 사망 플래그나 타이머를 추가하지 않았으며, 전체 킬피드·점수·처치 판정과 DA 체크값은 변경하지 않았습니다.
촬영 마스크는 원래 Visibility를 보존하므로 DA에서 사망 화면을 숨겨도 사망 우선순위는 유지됩니다.

`Scripts/Diagnostics/VerifyPersonalMessagePriority.py`를 MCP에서 실행했습니다.
실제 Lvl_PortalMap 서버/클라이언트 PIE의 중앙 위젯에서 단독 처치, 처치→사망, 사망→처치,
사망→연속 처치 요청, 사망 UI를 닫은 뒤 처치 요청의 5개 조건을 검사했습니다.
0.6초 뒤 남은 위젯 수와 `Head1/Head2/Child1/Child2` 문구를 읽어 모두 통과했습니다.
[실행 근거](evidence/personal-message-priority.json)는 제어된 UI 이벤트 시험이며,
실제 총격으로 두 플레이어가 동시에 사망하는 네트워크 전투·패키징·화면 캡처 검증은 포함하지 않습니다.

PR #972 통합에서는 `main`의 `3282d7830` HUD에 중앙 알림 함수 두 곳을 다시 적용했습니다.
위젯 트리 49개와 EventGraph 84개 노드를 변경 전후 비교했으며, 통합본에서도 중앙 알림 PIE 검사 5개가 모두 통과했습니다.
클라이언트 HUD에서 버프 링, 아이콘 테두리와 흰색 반짝임 위젯이 생성됐고 `ShootingArena.UI.BuffDurationRing`도 통과했습니다.
`ShootingArena.UI.BuffHUDStyle`의 링 분할·대비 검사는 통합본과 수정 전 `main`에서 같은 항목이 실패했습니다.

## 2026-11-11 킬 메달과 처치 알림 스타일

`/Game/QuakeLike_1_0/UI/PlayerUI/KillMedals/DA_KillMedalUISettings`에서 아래 값을 편집합니다.

| 항목 | 확인할 값 |
|---|---|
| 메달 → 배경 | 배경 RGB와 불투명도. 각 일반 메달과 큰 메달에 하나씩 적용합니다. |
| You Killed → 배경 | 배경 RGB, 불투명도, 그림자 위치 보정. X는 좌우, Y는 상하이며 기본값은 (-8, -9) UI 단위입니다. 불투명도 0에서 글자만 표시합니다. |
| You Killed → 그림자 | 그림자 색상·알파와 거리 배율. 기본값 0은 끄기, 0.5는 기존 거리의 절반입니다. |

`WBP_PlayerHUD_sdm.ShowCompetitiveKillMessage`는 기존 알림을 만든 직후 현재 DA 스타일을 적용합니다.
`You Killed`와 대상 이름을 담은 텍스트 그룹과 배경을 같은 세로 중앙에 배치합니다.
메달마다 `CompetitiveRankingHUD`가 `You Killed`와 같은 `T_Blur_800x200` 그림자를 이미지 뒤에 표시합니다.
그림자의 바깥쪽은 이미지보다 넓게 그려 가장자리가 자연스럽게 흐려지며, 기존 도장 애니메이션을 따라갑니다.
DA 저장 시 JsonAssetSync가 `Content/DataAssets/DA_KillMedalUISettings.json`에 값을 내보내며 다음 시작에서 JSON을 적용합니다.

전투 맵의 PIE에서 캐릭터가 스폰된 뒤 에디터 Output Log에 아래 명령을 입력합니다.

```text
py "D:/Unreal_ShootingArena/ShootingArena/Scripts/Diagnostics/PreviewKillNotificationUI.py" 3
```

마지막 숫자를 `1`, `3`, `7`로 바꿔 단일 메달, 개별 배경이 있는 메달 3개, 현재 기준의 큰 메달을 확인합니다.
미리보기는 현재 PIE 플레이어의 연속 처치 수를 초기화한 다음 지정한 메달 수와 `Preview Target` 처치 알림을 표시합니다.
DA에서 색상이나 그림자 값을 바꾼 뒤 같은 명령을 실행해 비교합니다.

PIE에서 단일·다중·큰 메달과 파란 메달/빨간 처치 알림, 배경 0, 그림자 배율 0/0.5/1을 확인했습니다.
최초 스타일 캡처는 `Saved/Diagnostics/KillNotificationStyles/`에 저장했습니다.
최종 메달 그림자는 기본값, 불투명도 0, 파란색의 3개 PIE 조건으로 비교했습니다. 캡처와 결과는 `Saved/Diagnostics/KillNotificationSoftShadow/`에 있습니다.
`ShootingArena.UI.KillMedalAutomaticAwards`와 `ShootingArena.UI.KillMedalBridge`가 통과했고, 재시작 로그에서 DA의 JSON 적용 성공을 확인했습니다.
기존 중앙 알림의 사망 우선순위 5개 조건도 다시 확인했습니다. 패키징 실행과 실전 상호 처치는 검증 범위에 포함하지 않았습니다.

## 2026-11-12 Respawn 글자 색상

`/Game/QuakeLike_1_0/UI/DeathUI/DA_RespawnBarStyle`에서 **Respawn Font Color 1**과 **Respawn Font Color 2**를 각각 편집합니다.
기본값은 `MI_Respawn_Font`의 기존 흰색과 회색을 상속합니다. `WBP_DeathScreen.InitDeathUI`는 사망 UI를 초기화할 때
DA의 두 색상을 `Text_RespawnCount`의 동적 폰트 머티리얼에 적용합니다.
PIE에서 같은 사망 위젯을 반복 초기화해 두 값의 독립 적용과 공유 머티리얼의 원래 값을 확인했습니다.

## 2026-11-12 사망 후 도착하는 개인 처치 알림

`BPC_Life`는 처치자의 `possessed pawn`이 유효할 때만 `Client_ShowKillMessage`를 보냅니다.
이 참조는 서버의 `OnPawnDeath`에서 비우고 `On Possess`에서 채웁니다.
`WBP_PlayerHUD_sdm.ShowCompetitiveKillMessage`는 기존 사망 위젯 확인에 더해 로컬 플레이어의 Pawn과
`BPC_Life.Is Death`를 확인하므로 위젯이 먼저 접혀도 늦게 도착한 개인 알림을 차단합니다.

`Lvl_PortalMap` 서버와 클라이언트 2개로 PIE를 실행해 사망 후 지연 피해로 상대가 사망하면 처치 수가 증가하고
중앙 사망 알림이 유지되는 것을 확인했습니다. 사망 위젯을 접은 상태에서 직접 전달한 알림 차단과 부활 후 표시도 확인했습니다.

## 2026-11-12 원격 클라이언트의 사망 알림 복구

`BP_QuakePlayerController.OnRep_possessed pawn`의 사망 분기가 `WBP_DeathScreen.InitDeathUI`를 호출합니다.
`InitDeathUI`는 카운트다운과 중앙 사망 알림을 함께 시작합니다. 이름이 없으면 `You Died`를 표시하고,
처치자 이름이 전달되면 기존 `Killed by`와 이름을 표시합니다. 이름이 있는 알림을 이미 초기화한 경우에는
복제 알림에서 사망 위젯을 다시 초기화하지 않습니다.

전투 맵 서버·클라이언트 PIE에서 실제 사망, 지연 처치 시 점수 집계와 중앙 알림 유지, 부활 후 처치 알림을 확인했습니다.
`VerifyPersonalMessagePriority.py`는 사망 RPC 처리 경로와 서버에서 피해를 입혀 발생시킨 복제 사망을 포함해 6개 조건을 검사합니다.

## 2026-11-12 점수판 강조 테두리 잔상

포탈맵 PIE에서 촬영용 체크를 모두 끄고 사망 점수판을 열면 `Img_ScoreLocalOutline`의 파란 테두리가 남았습니다.
`WS_Common_LocalOutline`의 테두리 알파가 위젯의 알파와 별도로 적용됩니다. **Tab 점수판**을 끄면
촬영 마스크가 Slate 그리기 영역도 비워 테두리까지 숨깁니다. 실제 강조 스타일을 사용하는 렌더 회귀 검사를 추가했습니다.
