# 싱글플레이 선택 화면

`/Game/QuakeLike_1_0/GameFlow/UI/WBP_MapSelect_SDM`에서 맵과 난이도를 선택하고 게임을 시작합니다. 기존 두 메인 메뉴의 진입 에셋 경로를 유지했습니다. `SelectMapByIndex`는 GameInstance에 선택을 반영한 뒤 같은 위젯의 `RefreshMapPage`를 호출합니다. `Btn_Back`과 `Destruct → ReleaseMapPreviews`는 기존 경로를 사용합니다.

Unreal MCP에서 UE의 그래프 복사 API로 `WBP_DifficultySelect_SDM`의 시작 노드와 핀 연결 및 `RefreshDifficultyStyle`을 옮겼습니다. 시작 그래프는 기존 로컬 서버 맵 설정, 준비 확인 및 타임아웃, `Get Login Command`와 `?Difficulty=` 전달을 사용합니다. 통합 화면에는 닉네임 입력란이 없으므로 시작 시 GameInstance의 기존 닉네임을 사용합니다. 서버 요청 실패와 준비 타임아웃에는 `ST_Game.Loading_Failed`를 표시합니다. 첫 진입 시 첫 번째 맵과 보통을 사용하고, 기존 맵 선택이 유효하면 GameInstance의 맵과 난이도를 읽습니다.

## 1920×1080 기준 슬롯

| 위젯 | 부모 | 화면 위치와 크기 |
| --- | --- | --- |
| Overlay_3 | CanvasPanel_32 | (120, 116), 620×44 |
| BackButtonBounds | CanvasPanel_32 | (120, 988), 158×44 |
| SinglePlayMapHeading | CanvasPanel_32 | (120, 224), 520×32 |
| LvlImageBox | CanvasPanel_1 | (120, 272), 1680×500 |
| Overlay_110, Overlay | LvlImageBox | (120, 272), (972, 272), 각각 828×500 |
| SinglePlayDifficultyHeading | CanvasPanel_32 | (120, 823), 520×32 |
| Button_Easy, Button_Normal, Button_Hard | SinglePlayControls | (120, 876), (330, 876), (540, 876), 각각 209×88 |
| Btn_Start | SinglePlayControls | (1440, 876), 360×88 |

## 검증 범위

`Play As Client`에서 메인 메뉴의 싱글플레이 버튼, 맵 카드 두 장, 난이도 버튼 세 개, 시작 버튼에 연결된 실제 클릭 델리게이트를 호출했습니다. 맵 카드를 바꿔도 같은 위젯이 유지됐습니다. 두 서버 이동 요청과 클라이언트 참가를 로그에서 확인했습니다.

| 선택 | 서버 이동 | 로그 |
| --- | --- | --- |
| Map_03 / 쉬움 | `Lvl_PortalMap?Difficulty=0` | `Saved/Logs/ShootingArena_2.log` 1134, 3691행, 클라이언트 `ShootingArena.log` 3963행 |
| Map_06 / 어려움 | `Lvl_02Map?Difficulty=2` | `Saved/Logs/ShootingArena_3.log` 1134, 1589행, 클라이언트 `ShootingArena-backup-2026.11.14-07.06.10.log` 1849행 |

최종 저장본의 첫 진입은 Map_03 / 보통이었습니다. 난이도 원본의 SHA-256은 작업 전후 모두 `c38c08bf1e8eb651b1a7ee5b12ec83d1a787bfaae04c158dbf9e45a7f773b270`입니다.

카드 하단에는 `M_UI_MapFade`를 사용합니다. `M_UI_SinglePlayC2`는 Retainer Box 미리보기의 채도·밝기와 난이도 포커스 외곽선을 처리합니다. 맵 배경에는 `BackgroundBlur`와 어두운 스크림을 적용합니다. 미리보기 재생과 해제는 `MapPreviewLibrary`가 담당합니다. 선택된 맵은 `ConfigureMapPreview`의 `bAlwaysPlay`로 반복 재생하며, 비선택 맵은 기존 호버 지연 후 재생합니다. 맵 변경 시 이전 배경은 0.24초 동안 페이드아웃합니다. 선택한 맵은 흰색 하단 바로, 선택한 난이도는 팔레트 색의 밑줄로 표시합니다. 난이도 버튼의 키보드·패드 탐색 포커스는 `Accent` 테두리로 표시하며 이동 중에도 선택 밑줄은 유지합니다.

C2 시각 검증은 1920×1080, DPI 1.0의 Play As Client 뷰포트를 PNG로 직접 캡처해 진행했습니다. 첫 맵·보통·시작 포커스와 두 번째 맵·어려움 포커스 상태를 각각 확인했습니다. 맵 변경 직후 이전 배경의 알파는 1.0, 전환 완료 후 0.0이며 위젯은 Hidden입니다.

Designer에서 MapName_1 또는 MapName_2의 글꼴, 색상, 그림자, 텍스트 정렬 등 서식을 수정하면 편집기 모듈이 변경한 속성을 상대쪽에도 적용합니다. 두 이름 위젯은 일반 Text Block이므로 카드의 공용 버튼 스타일이 글꼴을 덮어쓰지 않습니다. 카드 슬롯 위치와 표시 문구는 개별로 유지하며, 표시 문구는 RefreshMapPage가 계속 갱신합니다.
