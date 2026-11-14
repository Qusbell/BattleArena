# Codex 작업 지시서 — 싱글 플레이 맵·난이도 선택 화면 통합 (C2 시안)

- 대상 프로젝트: `ShootingArena` (UE 5.6), 작업 범위는 `Content/QuakeLike_1_0/GameFlow/UI` 중심
- 기준 시안: `C2_reference_1920x1080.png` (이 문서와 함께 전달됨, 1920×1080 실측)
- 작성 근거: 2026-11-14에 `Content` 폴더의 에셋을 바이너리 문자열로 조사한 결과. **노드 연결과 C++ 함수 시그니처는 열어 보지 못했으므로, 아래 "확인 필요" 표시 항목은 작업 전에 에디터/`Source`에서 직접 확인할 것.**

---

## 1. 목표 (한 문장)

현재 2화면(맵 선택 → 난이도 선택)인 싱글 플레이 진입 흐름을, **맵 2장 + 난이도 3개 + 게임 시작 + 나가기**가 한 화면에 있는 C2 시안으로 통합한다.

## 2. 최우선 원칙: 새로 만들지 말고 재활용

이 작업은 **배치(레이아웃) 변경과 기존 로직의 이동**이다. 아래 3장에 있는 것은 이미 구현되어 있으므로 다시 만들지 않는다.

- 새 C++ 클래스·새 머티리얼·새 스타일 에셋·새 데이터 테이블을 만들기 전에, 3장의 목록에 같은 역할이 있는지 먼저 확인한다.
- 3장에 없는 것이 꼭 필요하면, 만들기 전에 "무엇이 왜 필요한지"를 보고하고 승인을 받는다.
- 기존 에셋은 **삭제·이름 변경·이동하지 않는다.** (기존 `_Scripts/*.py`의 안전 규칙과 동일: 같은 이름이 있으면 건너뛰고, 아무것도 지우지 않는다.)

## 3. 이미 구현된 것 (재활용 대상)

### 3-1. 현재 실제로 쓰이는 흐름 (라이브)

`WBP_MainMenu_SDM` → `WBP_MapSelect_SDM` → `WBP_DifficultySelect_SDM` (모두 `GameFlow/UI/`, 부모는 `UserWidget`)

`GameFlow/UI/03_Widget/WBP_MainMenu`(CommonUI 버전)도 `WBP_MapSelect_SDM`을 연다. 따라서 **`WBP_MapSelect_SDM`의 에셋 경로와 이름을 유지하면 메인 메뉴 2종은 수정할 필요가 없다.**

### 3-2. `WBP_MapSelect_SDM`에 이미 있는 것

| 기능 | 기존 구현 | 처리 |
|---|---|---|
| 맵 목록 읽기 | `DT_MapData` + `GetDataTableRowNames`, 변수 `MapRowNames` | 그대로 사용 |
| 맵 카드 2장 | `LevelFrame`, `LevelFrame_2` (`BS_MapFrame`, `T_UI_MapFrame`), `Lvl_Image`, `Lvl_Image_2`, `MapTT_1`, `MapTT_2` | 위치·크기만 변경 |
| 페이지 갱신·선택 | 함수 `RefreshMapPage`, `SelectMapByIndex`, 변수 `Selected_Map_ID_Local` | 그대로 사용 |
| **맵 미리보기 영상** | `MapPreviewLibrary`의 `ConfigureMapPreview` / `ReleaseMapPreviews`, `DT_MapPreviewMedia`(행: `PreviewVideo`, `StillImage`, `HoverDelaySeconds`), `M_MapPreviewVideo` | **그대로 사용. 영상 관련 신규 구현 금지** |
| 뒤로 가기 | `Btn_Back`, `Txt_BackLabel` (`BS_BackFooter_HoverGlow`, `MI_UI_BackHoverGlow`/`PressedGlow`) | 위치만 변경 (5장) |
| 효과음 | `DA_Sounds`, `TrimmedSoundLibrary.PlayTrimmedSound2D`, `Button_MouseOver_Cue`, `Button_Select_Cue` | 그대로 사용 |
| 배경 | `BackGroundImage`(`BackGround_RedHall`), `WS_Match_Scrim` | 5-1 참고 |
| 다국어 | `ST_Game` 문자열 테이블, `LanguageLibrary` | 그대로 사용 |
| 녹화 마스크 | `RecordingUIMask`, `RecordingUI_Menus_CanvasPanel` | 유지 |

### 3-3. `WBP_DifficultySelect_SDM`에 이미 있는 것 (통합 화면으로 옮길 로직)

| 기능 | 기존 구현 | 처리 |
|---|---|---|
| 난이도 버튼 3개 | `Button_Easy`, `Button_Normal`, `Button_Hard` (`BS_DiffTab_Easy/Normal/Hard`) | 통합 화면으로 이동 |
| 선택 상태 표시 | 함수 `RefreshDifficultyStyle`, `ReusableStyleLibrary.SetStyledButtonSelected`, `UIThemePaletteLibrary.GetColor`, 변수 `SelectedDifficultyIndex` | 로직 그대로 이동 |
| 난이도 값 | `Enum_Selected_Difficulty` (Easy / Normal / Hard) | 그대로 사용 |
| 시작 버튼 | `Btn_Start` (`BS_DifficultySelect_Start`, `TS_DifficultySelect_StartYellow`, `MI_UI_StartBevel_*`) | 통합 화면으로 이동 |
| **게임 시작 로직** | `LocalDedicatedServerLibrary`(`SetLocalDedicatedServerMap`, `IsLocalDedicatedServerReady`), `Get Login Command`, `ExecuteConsoleCommand`, URL 옵션 `?Difficulty=` | **노드 그래프를 그대로 옮긴다. 다시 짜지 않는다** |
| 로딩 화면 | `LoadingBlueprintLibrary`의 `BeginLoadingScreen` / `EndLoadingScreen`, `ST_Game`의 `Loading_*` 키(`Loading_Failed` 포함) | 그대로 사용 |
| 선택 값 보관 | `BP_QuakeGameInstance`의 `Selected_Map_ID`, `Selected_Difficulty` | 그대로 사용 (6장) |

### 3-4. 공용 스타일 (새로 만들지 말 것)

- 색: `02_Styles/03_Palette/PDA_UIPalette` — `Accent`(FFC400), `OnAccent`, `Text1`, `Text2`, `DiffEasy`, `DiffNormal`, `DiffHard`, `Line*`, `Bg*` 등. **색은 팔레트에서만 가져온다. 헥스 값을 위젯에 직접 넣지 않는다.**
- 글꼴: `01_Fonts/F_Pretendard` (Regular / SemiBold / Bold / Black 사용)
- 텍스트 스타일: `01_TextStyles/` 아래 `TS_Match_Title`, `TS_UI_SectionCaption`, `TS_UI_Body`, `TS_Heading*`, `TS_DifficultySelect_*`
- 버튼 스타일: `02_ButtonStyles/` 아래 `BS_MapFrame`, `BS_DiffTab_*`, `BS_DifficultySelect_Start`, `BS_BackFooter_HoverGlow`
- 머티리얼 인스턴스: `04_Materials/Instance/` 아래 `MI_UI_DifficultyGlow_*`, `MI_UI_DifficultyBevel_*`, `MI_UI_TabGlow_*`, `MI_UI_StartBevel_*`, `MI_UI_Cut_Key`(키 표시용)

### 3-5. 건드리지 않는 것

- `GameFlow/UI/03_Widget/WBP_SinglePlay`, `WBP_MapRow`, `WBP_DifficultyRow`, `WBP_MapPickCard` 등: **`TestMainMenu_Level`에서만 참조되는 실험용**이다. 이번 작업에서 사용·수정하지 않는다.
- 구버전 `WBP_MainMenu`, `WBP_MapSelect`, `WBP_MapSelect1`, `WBP_DifficultySelect` (접미사 없는 것): 수정하지 않는다.
- `WBP_DifficultySelect_SDM`: 로직을 옮긴 뒤에도 **삭제하지 않는다.** 참조만 끊는다.

## 4. 작업 방식

1. **먼저 읽을 것:** 저장소 루트의 `AGENTS.md`, `.codex/`, `.agents/`, `Docs/`, `Scripts/`, `Tools/`, 그리고 `GameFlow/UI/_Scripts/*.py` 머리말. 이 프로젝트의 기존 작업 방식(에디터 Python 스크립트 실행, 안전 규칙)을 그대로 따른다.
2. **확인 필요:** 3장의 C++ 라이브러리(`MapPreviewLibrary`, `LocalDedicatedServerLibrary`, `ReusableStyleLibrary`, `UIThemePaletteLibrary`, `LoadingBlueprintLibrary`, `TrimmedSoundLibrary`, `LanguageLibrary`)의 실제 함수 시그니처를 `Source/`에서 확인한다.
3. **수정 대상은 `WBP_MapSelect_SDM` 하나다.** 이 위젯에 난이도 줄과 시작 버튼을 추가하고, 3-3의 로직을 옮긴다. 난이도 화면으로 넘어가던 호출은 제거한다.
4. `.uasset`은 텍스트로 편집할 수 없다. 위젯 트리 변경을 스크립트로 자동화할 수 없는 부분은 억지로 우회하지 말고, **에디터에서 사람이 따라 할 수 있는 단계별 수작업 목록**(위젯 이름, 부모, 슬롯 값 포함)으로 정리해 보고한다.
5. 작업 전 `WBP_MapSelect_SDM`, `WBP_DifficultySelect_SDM`의 현재 상태가 Git에 커밋되어 있는지 확인한다. 커밋되지 않은 변경이 있으면 멈추고 보고한다.

## 5. 화면 규격 (1920×1080 기준)

### 5-1. 전체

- 좌우 안전 여백 120, 내용 폭 **1680**. 내용 묶음은 세로 가운데 정렬(위·아래 여백 약 124).
- 배경: **고른 맵의 정지 이미지**(`DT_MapData`의 `Map Image`)를 화면 전체에 깔고, 그 위에 어두운 세로 그라데이션(위 78% → 아래 97%, 팔레트 `Bg900` 계열)을 덮는다. 블러는 쓰지 않는다. 그라데이션은 기존 `WS_Match_Scrim` 또는 `M_UI_MapFade`로 구현 가능한지 먼저 확인한다. 맵을 바꾸면 배경도 바뀐다(전환 0.24초 페이드).
- 세로 간격 규칙 3단계: 묶음 안 **16**, 구역 사이 **56**, 화면 제목 아래 **64**.

### 5-2. 요소 (위에서 아래로)

| # | 요소 | 위치·크기 | 글자 | 재활용 |
|---|---|---|---|---|
| 1 | 화면 제목 "싱글 플레이" | 왼쪽 위 | Pretendard Black 40 | 기존 `MainLevelText` |
| 2 | 나가기 `[ESC] 나가기` | 오른쪽 위, 높이 44 | 키 표시 SemiBold 13 + 라벨 SemiBold 18 | 기존 `Btn_Back`, `Txt_BackLabel`. 왼쪽 아래에서 오른쪽 위로 이동 |
| 3 | 구역 제목 "맵 선택" | 제목 아래 64 | Bold 24, `Text1` | — |
| 4 | 맵 카드 2장 | 각 **828×500**, 사이 24, 구역 제목 아래 16 | 맵 이름 Black 56, 카드 안 왼쪽 36·아래 32 | 기존 `LevelFrame`, `LevelFrame_2` |
| 5 | 구역 제목 "난이도 선택" | 맵 카드 아래 56 | Bold 24, `Text1` | — |
| 6 | 난이도 버튼 3개 | 각 **200×88**, 사이 16, 구역 제목 아래 16, 왼쪽 정렬 | 이름 Bold 24 | 기존 `Button_Easy/Normal/Hard` |
| 7 | 게임 시작 | **360×88**, 난이도 버튼과 같은 줄, 오른쪽 끝(맵 카드 오른쪽 끝과 일치) | Black 32, 가운데 정렬 | 기존 `Btn_Start` |

### 5-3. 맵 카드

- 카드 전체에 미리보기(기존 영상 미리보기 동작 유지), 아래쪽에 어두운 그라데이션(맵 이름 가독성), 그 위에 맵 이름.
- **선택된 카드:** 네 모서리에 꺾쇠 표시(팔 길이 16, 두께 2, 카드 바깥 7), 컬러 그대로.
- **선택 안 된 카드:** 채도 약 15%, 밝기 약 78%로 낮춘다. 마우스를 올리거나 포커스가 가면 컬러로 돌아온다(0.2초).
- 꺾쇠와 채도 처리는 기존 `BS_MapFrame` / `T_UI_MapFrame` / 미리보기 머티리얼 파라미터로 가능한지 먼저 확인한다. 불가능하면 필요한 최소 추가물을 보고한다.

### 5-4. 난이도 버튼

- 버튼 안 배치: **위에 눈금 3칸**(각 22×6, 사이 4), **아래에 이름**. 안쪽 여백 위아래 18, 좌우 22.
- 눈금: 쉬움 1칸, 보통 2칸, 어려움 3칸을 각 난이도 색(`DiffEasy` / `DiffNormal` / `DiffHard`)으로 채운다. 꺼진 칸은 `LineSoft` 계열. 눈금 색은 선택 여부와 관계없이 항상 표시한다.
- 이름 색: **선택된 버튼만 난이도 색**, 나머지는 `Text2`.
- 선택된 버튼: 맵 카드와 같은 꺾쇠 + 바탕을 한 단계 밝게.
- 기존 `MI_UI_DifficultyGlow_*`의 색 빛 효과는 이 시안에서 쓰지 않는다(에셋은 그대로 둔다).

### 5-5. 게임 시작 버튼

- 바탕 `Accent`(FFC400), 글자 `OnAccent`. 화면에서 `Accent`를 쓰는 곳은 이 버튼과 키보드 포커스 테두리뿐이다.
- 참고: 기존 `WBP_DifficultySelect_SDM`에는 `StartYellowFDD835`라는 변수가 있다. 팔레트 `Accent`와 값이 다르므로 **팔레트 값으로 통일**하고, 통일했음을 보고한다.

### 5-6. 상태 (모든 버튼 공통)

| 상태 | 표시 |
|---|---|
| hover | 밝기 약 +10% |
| 누름 | 크기 0.98 |
| 키보드·패드 포커스 | `Accent` 2px 테두리(바깥 4). 게임 시작 버튼은 `Text1` 테두리 |
| 비활성 | 불투명도 0.4 |

## 6. 동작

1. **초기 선택:** 화면이 열릴 때 `BP_QuakeGameInstance`의 `Selected_Map_ID`, `Selected_Difficulty` 값으로 선택 상태를 맞춘다. 값이 없으면 첫 번째 맵 + Normal. (저장 파일에 기록하는 기능은 만들지 않는다.)
2. **첫 포커스:** 게임 시작 버튼.
3. **맵 선택:** 기존 `SelectMapByIndex` 호출. 이제 화면을 넘기지 않고 선택 상태와 배경만 바꾼다.
4. **난이도 선택:** 기존 `RefreshDifficultyStyle` 흐름 유지.
5. **게임 시작:** 3-3의 기존 시작 로직을 그대로 실행한다. 로딩 표시는 기존 `BeginLoadingScreen` 흐름을 쓴다. 버튼 안에 별도 로딩 표시를 새로 만들지 않는다.
6. **나가기(ESC):** 기존 `Btn_Back` 동작(메인 메뉴로) 유지. 화면을 닫을 때 기존대로 `ReleaseMapPreviews`를 호출한다.
7. **실패 처리:** 기존 흐름에 실패 시 이 화면으로 돌아오는 경로가 있으면, 게임 시작 버튼 왼쪽에 `ST_Game`의 `Loading_Failed` 문구를 표시한다. 그런 경로가 없으면 만들지 말고 보고만 한다.

## 7. 문구·다국어

- 모든 문구는 `ST_Game` 키로 넣는다. 한국어/영어 전환(`LanguageLibrary`)이 그대로 동작해야 한다.
- 기존 키를 먼저 찾아 쓴다(예: `Menu_Start`). 없는 것만 추가하고, 추가한 키 목록을 보고한다.
- 영어 표기: Single Player / Select Map / Select Difficulty / Easy · Normal · Hard / Start.
- "나가기" 라벨: 메인 메뉴의 "나가기(게임 종료)"와 뜻이 겹칠 수 있다. **기본은 기존 뒤로가기 키(`Txt_BackLabel`이 쓰는 키)를 그대로 쓴다.** 문구를 바꿔야 하면 보고한다.

## 8. 이번 작업에서 하지 않는 것

- 단축키 추가(Q/E, 방향키 등), 난이도 설명 문구, 선택 요약 문구
- 영상 미리보기 신규 구현(기존 것 사용), 새 로딩 UI
- 메인 메뉴·멀티플레이·설정 화면 변경
- 에셋 삭제, 이름 변경, 폴더 이동
- 새 색·새 글꼴 도입

## 9. 완료 기준

1. 메인 메뉴 → 싱글 플레이를 누르면 통합 화면 하나만 뜬다. 난이도 화면으로 넘어가지 않는다.
2. 1920×1080에서 5장의 크기·간격과 일치한다(오차 ±2px). 참조 이미지와 나란히 비교한 스크린샷을 첨부한다.
3. 맵 2장·난이도 3개가 선택되고, 게임 시작으로 **기존과 같은 맵·난이도로** 게임에 진입한다(`?Difficulty=` 값 확인).
4. ESC와 나가기 버튼으로 메인 메뉴에 돌아간다.
5. 한국어/영어 전환 시 모든 문구가 바뀐다.
6. 맵 미리보기 영상이 기존처럼 재생되고, 화면을 닫으면 해제된다.
7. `WBP_DifficultySelect_SDM`과 3-5의 에셋이 삭제·변경되지 않았다.
8. 블루프린트 컴파일 오류·경고 0건.

## 10. 보고 형식

작업을 마치면 아래 순서로 보고한다.

1. 변경한 에셋 목록(경로)과 각 변경 요약
2. 새로 만든 에셋·`ST_Game` 키 목록과 만든 이유 (없으면 "없음")
3. "확인 필요" 항목의 확인 결과 (라이브러리 시그니처, 꺾쇠·채도 구현 방법, 실패 경로 유무)
4. 자동화하지 못해 사람이 에디터에서 해야 하는 단계 (있다면)
5. 9장 완료 기준별 통과 여부와 증거(스크린샷, 로그)
6. 지시서와 다르게 처리한 부분과 이유
