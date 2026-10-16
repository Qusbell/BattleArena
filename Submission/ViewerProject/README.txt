[ViewerProject - Blueprint / C++ 확인용]

- 블루프린트 로직(노드 그래프)과 C++ 소스를 확인하기 위한 프로젝트입니다.
  메시/머티리얼/애니메이션 등 리소스는 포함하지 않았습니다 (용량 문제).
- Unreal Engine 5.6 에서 확인했습니다.
  (ShootingArena.uproject 우클릭 > Switch Unreal Engine version > 5.6)
- 열 때 "모듈을 다시 빌드하시겠습니까?" 창이 나오면 Yes 를 누르면 C++ 모듈이 자동 컴파일됩니다.
  (Visual Studio 2022 + C++ 게임 개발 워크로드 필요)

Content/  : 블루프린트, 구조체/열거형 uasset
Source/   : C++ 소스 (ShootingArena, ShootingArenaEditorTools)
Plugins/  : 프로젝트가 사용하는 플러그인 소스
Config/   : 프로젝트 설정
