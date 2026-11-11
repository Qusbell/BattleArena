# 촬영용 UI 표시 설정

설정 에셋: `/Game/QuakeLike_1_0/UI/Recording/DA_UIRecordingSettings`.
15개 체크박스의 기본값은 모두 표시입니다. 전투(체력·방어구·무기), 타이머, 상단 순위,
Tab 점수판, 조준점·히트마커·줌, 킬 알림, 메달, 버프, 피격 효과, 사망·리스폰,
메뉴·로비, 결과, 닉네임, 커스텀 UI 커서, 로딩·맵 전환을 각각 제어합니다.

촬영 전에 DA를 열고 필요한 항목만 체크한 뒤 저장합니다. 에디터 PIE 실행 중 같은 DA의 체크값을
바꾸면 생성된 패널도 다음 그리기부터 반영합니다. 패키징 실행은 시작 시 저장된 DA 값을 사용합니다.
별도 런타임 체크 메뉴·단축키는 추가하지 않았습니다.

`URecordingUIMask`는 기존 위젯 바깥에서 Slate 그리기 스타일의 투명도를 0으로 만듭니다. 자식 위젯의 Visibility,
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
