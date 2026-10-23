# UI 뒤로가기 / 취소 / 창 닫기 키 (UIBackInput)

모든 메뉴에서 **한 키**로 "뒤로가기 / 취소 / 창 닫기"를 처리하는 공용 시스템입니다.
에디터에서는 Esc가 PIE를 종료시키므로 임시로 **F10**을 사용합니다. 위젯 블루프린트를 따로 수정할 필요가 없습니다.

## 1. 사용자 입장 (플레이어)

| 상황 | F10을 누르면 |
|---|---|
| 서브 화면(싱글플레이, 설정, 멀티플레이, 커스터마이즈) | 뒤로 감 |
| 맵 선택 팝업(`WBP_MapPickPop`) | 팝업만 닫힘 (취소와 동일) |
| 종료 확인창(`WBP_QuitDialog`) | 취소 |
| 메인 메뉴 | 아무 일도 없음 (뒤로 갈 곳이 없음) |

팝업 위에 서브 화면이 있으면 **가장 위의 창 하나만** 닫힙니다.

## 2. 동작 방식

1. 키 입력은 Slate 입력 프리프로세서로 받습니다. 위젯에 포커스가 없어도 동작합니다.
2. 가장 위에 떠 있는 창부터 아래 순서로 처리합니다.
   1. 뷰포트에 직접 붙은 팝업 위젯 (CommonUI 스택이 없는 최상위 위젯)
   2. CommonUI 스택의 활성 화면 (이름에 `Modal`이 든 스택 → 그 외 스택)
      - 일반 스택은 화면이 2개 이상 쌓였을 때만 처리합니다. 바닥(메인 메뉴)은 닫지 않습니다.
3. 각 위젯은 아래 순서로 시도하고, 처리되면 거기서 끝납니다.
   1. `IUIBackHandler`를 구현했으면 `HandleBackAction()` 호출 (true를 반환하면 처리 완료)
   2. 설정의 **Back Button Names**에 있는 이름의 버튼을 찾아 **실제 클릭**
   3. (CommonUI 스택 화면만) `DeactivateWidget` 호출

버튼을 직접 누르는 것과 같은 경로라서 기존 `Btn_Back` 로직(`OnBack` 디스패처 등)이 그대로 실행됩니다.

## 3. 새 메뉴를 만들 때

가장 쉬운 방법은 **버튼 이름 규칙**을 따르는 것입니다.

| 용도 | 위젯 이름 |
|---|---|
| 뒤로가기 | `Btn_Back` |
| 취소 | `Btn_Cancel` |
| 닫기 | `Btn_Close` |

위젯 블루프린트 **Designer 탭에서 버튼 이름을 위 중 하나로** 지으면 끝입니다. 코드나 설정 수정이 필요 없습니다.

- 버튼이 보이지 않거나(Collapsed/Hidden) 비활성화되어 있으면 건너뜁니다.
- 한 화면에 이름이 여럿이면 설정 배열의 **앞쪽 이름이 우선**입니다.

## 4. 설정 (키 / 버튼 이름 변경)

**Edit → Project Settings → Game → UI Back Input**

| 항목 | 설명 | 기본값 |
|---|---|---|
| Back Keys | 뒤로가기로 쓸 키 목록 | `F10` |
| Back Button Names | 클릭해 줄 버튼 위젯 이름 목록 | `Btn_Back`, `Btn_Cancel`, `Btn_Close` |

- 값은 `Config/DefaultGame.ini`에 저장됩니다. **커밋해야 팀원에게 적용됩니다.**
- 이름은 Designer 탭의 위젯 이름과 **대소문자까지 정확히** 같아야 합니다.
- 다른 이름의 버튼을 쓰고 싶으면 Back Button Names에 `+`로 추가하면 됩니다. 예: `Btn_Exit`

### 패키징(출시) 시 Esc로 바꾸기

Back Keys에서 `F10`을 지우고 `Escape`를 추가합니다. 둘을 같이 둬도 됩니다.

## 5. 화면별로 직접 처리하고 싶을 때 (`IUIBackHandler`)

예: 닫기 전에 "저장하시겠습니까?" 확인창을 띄우고 싶을 때.

1. 위젯 블루프린트 **Class Settings → Interfaces → Add**에서 `UIBackHandler`를 추가합니다.
2. `Handle Back Action` 함수를 구현합니다.
   - 직접 처리했으면 **true** 반환 → 버튼 클릭과 `DeactivateWidget`은 실행되지 않습니다.
   - **false** 반환 → 기본 처리(버튼 클릭 → 비활성화)로 넘어갑니다.

## 6. 문제 해결

| 증상 | 확인할 것 |
|---|---|
| F10을 눌러도 아무 일 없음 | 그 화면의 뒤로 버튼 위젯 이름이 Back Button Names에 있는지 |
| 화면이 닫히지 않고 버튼 로직만 안 도는 경우 | 버튼이 Collapsed/Hidden이거나 비활성화 상태인지 |
| 메인 메뉴에서 안 먹음 | 정상입니다. 스택 바닥 화면은 의도적으로 닫지 않습니다. |
| 팝업이 아닌 일반 HUD 위젯이 닫힘 | HUD 위젯 안에 `Btn_Close` 같은 이름의 보이는 버튼이 없는지 (이름이 겹치면 눌립니다) |
| 에디터 PIE 창이 여럿일 때 | 활성 창의 인스턴스만 반응합니다. |

## 7. 관련 코드

| 파일 | 역할 |
|---|---|
| `Source/ShootingArena/Public/UI/UIBackInputSubsystem.h`, `Private/UI/UIBackInputSubsystem.cpp` | 키 감지와 처리 본체 (GameInstanceSubsystem, 데디케이트 서버에서는 생성되지 않음) |
| `Source/ShootingArena/Public/UI/UIBackInputSettings.h`, `Private/UI/UIBackInputSettings.cpp` | 키 / 버튼 이름 설정 (Project Settings) |
| `Source/ShootingArena/Public/UI/UIBackHandler.h` | 화면별 직접 처리용 선택 인터페이스 |

BP에서 코드로 직접 닫고 싶으면 `UIBackInputSubsystem`의 `Handle Back Action` 함수를 호출하면 됩니다.
