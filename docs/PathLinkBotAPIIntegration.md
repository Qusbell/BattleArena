# 현재 Bot의 PathLink API 적용 위치

확인일: 2026-10-21. Blueprint는 Unreal MCP로 읽기만 했으며 수정·컴파일·저장하지 않았습니다.

## 실제 실행 흐름

BP_AIShooter 기본 AIControllerClass는 BP_QuakeAIController2입니다. 조사한 시작점은 /Game/QuakeLike_1_0/AI/AIC/BP_QuakeAIController2입니다.

Controller On Possess → BT_QuakeTree → BTT_VirtualInputAIMoveTo → BPC_VirtualInputAIMovement → Build Path → UPathLinkSubsystem::FindShortestRoute 순서로 실행됩니다.

BT_QuakeTree의 STG·LTG·순회·적 마지막 위치·전투 이동 위치는 공통 BTT_VirtualInputAIMoveTo를 사용합니다. Task Initialize는 Blackboard 목표가 Actor이면 Request Build NavPath To Actor, 아니면 Request Build NavPath To Location을 호출합니다. 이동 컴포넌트 Build Path는 상세 Route의 Segments를 보관하고 PathPoints를 추종합니다. 실제 AddMovementInput 및 Link별 이동 처리도 이 컴포넌트에 있습니다.

따라서 상세 이동 경로는 이미 이번 C++ 최적화의 혜택을 받습니다. 그 노드를 추정 API로 바꾸면 이동에 필요한 경로점이 없어집니다. 추가 적용 대상은 이동 전 후보 평가입니다.

## 우선 적용할 위치

| 위치 | 현재 처리 | 권장 처리 |
|---|---|---|
| Controller.Set LTG | CachedSpawners의 Having Item을 순회하며 Try Can Reach로 검사하고 Get Best Goal 호출 | 후보별 추정 거리 평가를 한 번 수행해 도달성의 임시 판단·점수 계산에 재사용. 필요한 최종 검증은 후보 소수에 한정 |
| Controller.Get Best Goal / Get Goal Weight | NavMesh 투영 여부 확인 후, 아이템 기본 Weight를 직선거리×0.1로 나누어 순위 계산 | 자기 위치→아이템 위치의 추정 거리를 점수 분모에 사용. 다른 점수 계산은 유지 |
| Controller.Filtering STG | Actor 유효성, Can Arrive In Time, Is Shorter Then LTG로 필터 | 후보별로 얻은 추정 거리를 시간·LTG 방향 판단에 재사용 |
| Controller.Can Arrive In Time | 직선거리÷GetChaserSpeed ≤ TravelTimeLimit | 추정 Route 거리÷속도. 이는 Link 비용을 속도로 나눈 휴리스틱이며 실제 낙하·점프 패드 소요 시간의 정밀 예측은 아님 |
| Controller.Is Shorter Then LTG | 세 가지 직선거리 제곱을 비교 | 자기→STG, STG→LTG, 자기→LTG의 방향성 있는 추정 거리로 같은 비교 수행 |
| BPC_PointsTourist.GetCloser | 방문 횟수가 같을 때 직선거리 제곱으로 순회 후보 비교 | 낮은 우선순위의 추가 적용. 방문 횟수 정책은 유지하고 동률 후보에 추정 거리 사용 |

Set LTG의 Try Can Reach는 컴파일된 함수 정보상 UPathLinkSubsystem::CanReach 호출을 포함합니다. CanReach는 동기 TestPathSync 연결 검사를 수행하며 최종 PathPoints를 생성하는 함수는 아닙니다. 따라서 현재 모든 아이템 후보에 FindShortestRoute를 실행하고 있다는 설명은 맞지 않습니다. 이 위치의 개선 목적은 반복 동기 연결 검사 축소와 거리 기반 평가 통합입니다.

반면 Filtering STG의 Test Path Exists Sync 및 Try Can Reach 노드는 현재 진입 실행선이 끊겨 있습니다. 실제 실행은 Actor Is Valid에서 바로 시간 검사로 넘어갑니다. 이 두 노드를 현재 작동 중인 병목으로 집계하면 안 됩니다. STG의 후보 평가를 통합할 때 도달성의 임시 검사를 넣을 위치입니다.

Set LTG Timer는 0.2초 반복이지만, Set LTG 안에서 MyCharacter가 유효하고 기존 LTG가 유효하지 않을 때만 후보 순회를 수행합니다. 0.2초마다 항상 전 후보를 정밀 탐색하는 것은 아닙니다. 이 기존 목표 유지 정책은 API 연결과 별개입니다.

## 권장 데이터 흐름

후보 수집 → 각 후보의 추정 비용을 1회 조회·저장 → STG 필터와 점수 계산에서 재사용 → 최고 후보 선택 → 공통 이동 컴포넌트에서 상세 Route 생성 → 실패 시 다음 후보 또는 기존 목표 유지.

한 번의 목표 선택 동안 다음 정보를 지역 배열 또는 구조체로 보관하는 편이 좋습니다.

- 대상 Actor와 이번 평가에 사용한 목표 좌표.
- 자기→후보의 추정 거리와 조회 성공 여부.
- 필요한 STG에 대해서만 STG→LTG 추정 거리.
- 아이템 가치와 최종 점수.

자기→LTG는 한 번의 STG 평가에서 공통이므로 한 번만 계산합니다. Get Goal Weight와 Can Arrive In Time이 각각 같은 거리를 다시 조회하지 않도록 거리 값을 인자로 전달하는 형태를 권장합니다. 현재 Can Arrive In Time, Is Shorter Then LTG, GetCloser는 Blueprint Pure 함수이므로 비용 조회를 내부에 그대로 추가하기보다 명시적인 평가 단계에서 계산해 전달하면 호출 횟수를 관리하기 쉽습니다.

## API 역할

- GetEstimatedRouteDistance: 후보 순위·대략적인 도달 시간·우회 판단. PathPoints와 동기 경로 탐색이 없음.
- GetRouteDistance: 경로점 없이 정확한 거리만 필요한 최종 소수 후보 검증. 이 결과 직후 같은 목표에 상세 경로가 필요하다면 FindShortestRoute를 바로 사용해 중복 요청을 피하는 편이 좋음.
- FindShortestRoute: 실제 이동 목표가 결정됐을 때 상세 Route 생성. 현재 Build Path에 유지.
- CanReach: 정확한 연결 검사가 반드시 필요한 개별 상황에 유지. 추정 성공 여부와 함께 모든 후보에 중복 호출하지 않음.

추정 API의 false는 미도달뿐 아니라 캐시 없음·지원하지 않는 Context·투영 실패일 수도 있습니다. 실패값을 거리 0으로 점수 계산에 넣거나 무조건 영구 미도달로 확정하지 않아야 합니다. 실패 사유 분리가 필요하면 공통 평가 래퍼에 상태값을 추가하고, 캐시가 없을 때는 기존 검사로 대체하거나 평가를 분산하는 정책이 필요합니다. false가 나온 모든 후보를 매번 정밀 조회하면 비용을 다시 키울 수 있습니다.

## 방향·단위·Context

현재 직선거리 노드의 핀 순서를 그대로 추정 API에 옮기면 안 됩니다. 직선거리는 대칭이지만 PathLink 경로는 OneWay 등으로 비대칭입니다.

- 점수 및 시간: Start=자기, Target=후보.
- STG→LTG 비교: Start=STG, Target=LTG. 역방향 LTG→STG가 아님.
- 자기→LTG: 평가 중 같은 값을 재사용.
- 반환 단위는 cm. Get Goal Weight의 기존 0.1 배율을 자동으로 m 변환이라고 해석하거나 제거하지 않음.
- 현재 Build Path의 Find Shortest Route 노드는 PathfindingContext 핀이 비어 있어 기본 Context를 사용함. 추정도 동일 기준으로 연결하는 것이 일관됨. 추후 에이전트별 Context가 필요하면 추정·거리 전용·상세 조회를 함께 맞춰야 함.
- Start/Target 좌표는 실제 이동 요청과 같은 기준을 사용. 임의 위치에 생성된 BP_DropItem도 Actor의 현재 좌표를 전달할 수 있음. 현재 Get Best Goal에는 별도의 작은 범위 NavMesh 투영 검사도 있으므로 떨어진 아이템의 수집 가능 위치와 투영 범위를 별도로 유지·검토해야 함.

Is Shorter Then LTG는 현재 자기→STG와 STG→LTG가 각각 자기→LTG보다 작은지를 검사합니다. 추정 거리로 바꾸더라도 이 정책을 먼저 유지할 수 있습니다. 합산 우회 비용을 검사하도록 바꾸는 것은 별도의 행동 정책 변경입니다.

## 유지할 거리와 재탐색 조건

적 사격 대상 선택, 사거리, 공격 각도, 물리적인 목표 이동 감지 거리, 현재 PathPoint 도달 반경까지 Route 거리로 바꾸는 것은 권장하지 않습니다. 특히 BTService_SelectTarget_R → AQuakeTargetSelectionAIController::SelectBestTarget은 시야·거리·공격성에 따른 사격 대상 선택이므로 이번 아이템 이동 비용 적용과 분리해서 판단해야 합니다.

BPC_VirtualInputAIMovement에는 0.5초 Try Rebuild NavPath 타이머와 목표 Actor의 Tether Distance 감지가 있습니다. 이들은 채택된 목표의 상세 경로 갱신이므로 추정 API로 대체하지 않습니다. 현재 Tick에서 AddMovementInput을 실행한다고 해서 매 Tick 상세 경로를 생성하는 구조는 아닙니다. 재계산 빈도 변경은 별도의 이동 회귀 검증이 필요한 작업입니다.

## 확인한 구체 노드

- Controller.Set LTG: K2Node_MacroInstance_1 (Try Can Reach).
- Controller.Get Goal Weight: K2Node_CallFunction_39 (Distance), K2Node_PromotableOperator_1 (×0.1), K2Node_CallFunction_40 (Weight÷거리).
- Controller.Can Arrive In Time: K2Node_CallFunction_2 (Distance), K2Node_CallFunction_4 (거리÷속도).
- Controller.Is Shorter Then LTG: K2Node_CallFunction_6 / 7 / 12 (Distance Squared).
- Controller.Filtering STG: K2Node_CallFunction_0 (Test Path Exists Sync), K2Node_MacroInstance_3 (Try Can Reach)는 비활성 실행 경로.
- Movement.Build Path: K2Node_CallFunction_6 (Find Shortest Route)는 유지.

현재는 적용 위치 조사만 완료했습니다. 권장 첫 적용 범위는 Controller의 LTG/STG 후보 평가와 공통 거리 재사용이며, 순회 후보 비교는 이후 단계로 둘 수 있습니다.
