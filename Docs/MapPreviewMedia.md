# 맵 미리보기 이미지·영상 설정

콘텐츠 브라우저에서 다음 데이터 테이블을 엽니다.

`/Game/QuakeLike_1_0/GameFlow/UI/MapPreviewMedia/DT_MapPreviewMedia`

행 이름은 기존 `DT_MapData`의 맵 ID와 같습니다(예: `Map_03`, `Map_06`).

| 필드 | 설정 |
| --- | --- |
| `StillImage` | 기본 상태에서 표시할 정지 이미지입니다. 비워 두면 기존 `DT_MapData.Map_Image`를 사용합니다. |
| `PreviewVideo` | 해당 맵의 `FileMediaSource` 등 Media Source 에셋을 지정합니다. |

1. 맵 영상을 `Content/Movies`에 넣습니다.
2. 콘텐츠 브라우저에서 **Media → File Media Source**를 생성하고 영상 파일을 지정합니다. File Path는 `./Movies/파일명.mp4`와 같이 프로젝트 상대 경로를 사용합니다.
3. `DT_MapPreviewMedia`의 해당 맵 행에서 `PreviewVideo`에 이 에셋을 지정하고 저장합니다. 이미지와 영상은 서로 다른 에셋으로 지정합니다.

맵 목록에 표시할 맵은 기존과 같이 `DT_MapData.Map_Image`도 지정되어 있어야 합니다. 미리보기 테이블은 맵 목록이나 게임 시작 데이터를 변경하지 않습니다.

- `WBP_MapSelect`: 기본 상태는 정지 이미지, 마우스 오버 시 반복 영상입니다. 마우스가 벗어나면 영상은 일시정지되고 정지 이미지로 바뀝니다. 다시 올리면 같은 위치부터 재생합니다.
- 맵을 클릭하면 `WBP_DifficultySelect`가 동일한 MediaPlayer를 이어받습니다. 영상 파일을 다시 열거나 재생 위치를 초기화하지 않습니다.
- `WBP_DifficultySelect`: 마우스 위치와 관계없이 계속 반복 재생합니다. 뒤로 돌아오면 맵 선택 카드는 다시 정지 이미지 상태입니다.
- 영상 미지정 또는 열기 실패 시 정지 이미지를 유지합니다. 미리보기에는 별도 오디오 출력을 연결하지 않았습니다.
- 메뉴 화면 간에는 맵별 재생 위치를 기억합니다. 게임 맵으로 이동하면 미리보기 플레이어를 닫고 텍스처·머티리얼·소스 참조와 캐시를 해제합니다. 메뉴 위젯 사이의 이동은 맵 전환이 아니므로 이어 재생이 유지됩니다.

구현은 `Source/ShootingArena/Public/UI/MapPreviewMedia.h` 및 `Private/UI/MapPreviewMedia.cpp`에 있습니다. 위젯 그래프 연결을 재구성해야 할 때는 C++ 빌드 후 에디터 Python에서 `Tools/UI/setup_map_preview.py`를 실행할 수 있습니다.

자동 테스트: `ShootingArena.UI.MapPreview.HoverResumeAndHandoff`. 기존 `Content/Movies/MainMenuPreview.mp4`를 임시 테스트 소스로 사용하며 실제 맵의 영상 설정은 변경하지 않습니다.
