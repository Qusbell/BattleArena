# 보고된 문제 실행 조사 — 2026-11-10

이 문서는 수정 명세가 아니라 실행 조사 기록입니다. 기존 사용자 변경을 보존하고, 제품 Blueprint 및 C++ 구현은 수정하지 않았습니다. 진단 스크립트와 원본 관측 자료는 `Saved/Diagnostics/ReportedIssues/`에 있습니다. Blueprint 조회 및 PIE 객체 조작은 Unreal MCP의 Python 호출로 수행했습니다.

## 환경과 범위

- UE 5.6.1, 현재 Editor의 `Lvl_PortalMap`, 기존 설정에 따른 데디케이티드 서버 PIE(`UEDPIE_0`)와 클라이언트 PIE(`UEDPIE_1`).
- 시작 시 맵과 디버그 Blueprint 등에 기존 사용자 변경이 있었습니다. 맵 전환·저장·제품 에셋 수정·Live Coding 없이 조사했습니다.
- 첫 입력 시험은 Enhanced Input 액션 주입입니다. 물리 키, OS 포커스, 콘솔 입력 경로를 검증한 것으로 해석하지 않습니다.
- 제어된 아이템 시험은 서버 폰과 아이템의 위치를 PIE 안에서 변경했습니다. 일부 시험은 AI 로직·이동을 멈추고 원거리 공간에서 수행했습니다. 실제 맵을 걷는 자연 재현과 구분합니다.

## 1. 아이템 중복 획득: 공격 버프에서 재현, 재진입 경로 확인

### 관측

1. 서버 플레이어 폰과 AI 폰을 90 유닛 간격으로 배치했습니다.
2. `Buff.Multi.Atk` 행을 사용하는 하나의 `BP_BuffItem`을 두 폰 사이로 이동시켰습니다. Sphere 반지름은 105입니다.
3. 원래 Blueprint 핸들러 그대로 수행한 첫 시험에서 두 폰의 `buffEndServerTime`이 동일한 양의 값으로 바뀌었습니다. 두 폰 모두 같은 버프 행을 받았습니다.
4. 추가 시험에서는 해당 PIE 아이템의 기존 BeginOverlap 핸들러를 보존하면서 그 앞에 관측 콜백을 추가했습니다. 같은 동기 호출 안에서 첫 폰과 두 번째 폰이 각각 진입했고, 두 진입 모두 `IsPickedUp=false`였습니다. 두 번째 진입에서는 아이템의 부착 부모가 첫 폰으로 이미 바뀌어 있었습니다. 반대 폰이 먼저 진입하는 경우에도 같은 현상이 나타났습니다.
5. 유효한 대조 시험 한 건에서 관측 콜백이 진입 직후 테스트 아이템의 충돌을 끄도록 했을 때에는 한 폰의 버프 종료 시각만 갱신됐습니다. 다른 두 대조 시도는 Overlap 자체가 발생하지 않아 판정에서 제외합니다.

### 원인

`BP_DropItem.EventGraph`는 `IsPickedUp`을 검사한 뒤 자식 `ApplyItemEffect`를 실행하고, 성공 반환 이후에 `IsPickedUp=true`로 설정합니다. `BP_BuffItem.ApplyItemEffect`는 버프 적용 뒤, 반환하기 전에 아이템을 획득 폰에 부착합니다. 이 부착 과정에서 Overlap이 재진입하면 두 번째 폰도 아직 false인 획득 플래그를 통과합니다.

따라서 이 재현은 단순히 같은 프레임에 두 이벤트가 순서대로 실행되는 경우가 아니라 **효과 적용이 끝나기 전 부착으로 Overlap이 재진입하는 경우**입니다. 동일 아이템에 대한 서버 획득 예약/재진입 방지 처리가 효과 적용보다 앞에 필요합니다. 이 문서는 해결책을 구현하거나 검증하지 않았습니다.

### 기록의 한계

- 방어구 소형 아이템 16개에서는 동일 배치 조건에서 한 폰만 방어구 10을 받았습니다. 다른 아이템 종류 전체가 안전하다는 뜻은 아닙니다.
- `UArenaMatchLogSubsystem::RecordItemPickup`은 `PickupSeqs.Contains(ItemId)`이면 후속 기록을 생략합니다. DB `pickup`에도 아이템별 UNIQUE 제약이 있습니다. 획득 로그 한 건만 있다는 사실은 실제 효과가 한 번만 적용됐다는 증거가 아닙니다.
- 근거: `buff-overlap-two-pawns.json`, `buff-overlap-entry-trace.json`, `buff-overlap-attach-control.json`, `overlap-two-pawns-first.json`, `overlap-two-pawns-batch.json`, `buff-apply-graph.json`, `drop-item-event-graph.json`.

## 2. 사망·부활 후 조작 불가: 첫 주입 시험에서는 미재현

`runtime_probe.py`가 발사 액션을 10초 동안 매 tick 주입하고, 2초 시점에 서버 폰에 치명 피해를 적용했습니다. 이후 14–22초 구간에 이동 액션을 주입했습니다. 24초 동안 49개의 서버·클라이언트 상태 표본을 기록했습니다.

- 약 2초에 사망, 약 5.031초에 동일 폰이 부활했습니다.
- 사망 중 MovementMode는 None, 부활 후 Walking으로 복구됐습니다.
- 표본에서 이동·시점 Ignore 플래그와 캐릭터 이동 차단 플래그는 false였습니다. 사망 UI에서 켜진 커서는 부활 후 꺼졌습니다.
- 부활 후 주입한 이동 입력으로 서버·클라이언트 위치가 변했습니다.
- 발사 액션에는 Pressed와 Released 트리거가 모두 있습니다. `Completed`에 중지 노드가 없다는 사실만으로 입력 중지 누락을 확정할 수 없습니다.

이 첫 시험은 물리 마우스 버튼을 누른 상태, OS/Slate 포커스 상실, 콘솔 팝업, 모든 무기 종류의 실제 발사 성공·중지 여부를 검증하지 않았습니다. 이후 추가 시험과 최종 판정은 아래에 기록합니다.

근거: `runtime_probe.py`, `death-hold-fire.json`.

### 추가 제보와 잔탄 소진 경계 시험

사용자는 중복 획득이 버프 두 명 적용이라는 점을 확인했으며, 조작 불가는 **잔탄이 거의 없는 상태에서 0이 되는 순간 사망**하는 타이밍으로 추정된다고 설명했습니다. 무기 종류는 확인되지 않았으며, 실행 환경은 현재 프로젝트의 PIE 또는 개발 빌드라고 답했습니다.

추가 실행 시험은 다음과 같습니다.

| 시험 | 완료 조건 수 | 실행 결과 |
|---|---:|---|
| 기관총의 잔탄을 1로 준비하고 클라이언트 발사로 0이 된 뒤 사망 | 6 | 지연 0/150/500ms, 즉시 또는 짧은 지연 후 사망에서 모두 복구 |
| 잔탄 1인 상태에서 사망을 먼저 처리하고 늦은 발사 요청 관찰 | 3 | 모두 복구. 이 세 건은 사망 이후 해당 구 무기의 0 도달이 관측되지 않았으므로 잔탄 0 재현 시험과 구분 |
| 샷건·레일건·로켓 런처·플라즈마건의 마지막 탄 소진 직후 사망 | 4 | 모두 복구 |

13개 완료 조건 모두 부활 후 이동·시점 Ignore 플래그와 캐릭터 이동 차단 플래그가 false였습니다. 이후 주입한 이동 입력으로 위치가 변하고, 시점 입력으로 yaw가 바뀌며, 새 무기의 실제 탄 수가 120보다 줄어드는 것까지 확인했습니다. 이 중 잔탄 0을 실제 관측한 조건은 10개입니다. 지정한 사망 지연과 실제 지연은 프레임 간격 때문에 다를 수 있어 요약 JSON에 둘 다 기록했습니다.

여러 PIE 세션이 메모리에 남으면 `ObjectIterator`의 첫 입력 서브시스템이 이전 세션을 가리킬 수 있음을 확인했습니다. 추가 행렬 시험은 현재 클라이언트 PlayerController를 기준으로 `GetLocalPlayerSubSystemFromPlayerController`를 호출해 대상 서브시스템을 선택했습니다. 초기 `last-ammo-death.json`은 서버에서 `TryUse`로 마지막 탄 발사와 사망을 같은 시각에 실행한 관측으로만 사용하며, 이후 입력 주입의 성공 증거로 사용하지 않습니다. 이후 `client-last-ammo-death.json`과 행렬 시험은 현재 서브시스템을 명시적으로 선택했습니다.

`ammo-zero-predeath-first-attempt.json`은 오래된 무기 UObject를 조회하다 진단 스크립트가 중단된 첫 시도입니다. 게임 조작 불가가 발생한 증거가 아니며 완료 조건 수에 포함하지 않습니다. 오래된 객체 조회를 방어한 뒤 동일 시험을 다시 완료했습니다.

근거: `ammo_zero_matrix.py`, `ammo-zero-matrix.json`, `ammo-zero-predeath-matrix.json`, `ammo-zero-weapons-matrix.json`, `ammo-zero-all-summary.json`, `last_ammo_death_probe.py`, `client-last-ammo-death.json`. `ammo-zero-matrix.utrace`는 행렬 시험 일부 구간의 CPU/frame/bookmark/log Trace입니다. 이 Trace 자체에서 조작 불가를 발견한 것은 아닙니다. 네트워크 지연·손실 설정은 각 진단 종료 시 0으로 복구했고 PIE를 종료했습니다.

### 물리 키와 콘솔 대조

Computer Use의 Windows 화면 캡처는 `FrameArrived timed out` / `window capture timed out`으로 실패했습니다. 이후 텍스트 기반 창 제어로 물리 키를 보내고, MCP의 게임 상태 조회 및 엔진 `Shot SHOWUI` 스크린샷으로 결과를 확인했습니다.

1. 물리 `W`가 현재 PIE 클라이언트의 pressed/released 기록에 나타났습니다.
2. 물리 grave 키로 콘솔을 열고 `W`를 누르면 콘솔의 입력/자동완성으로 들어갔으며 게임 PlayerController에는 `W`가 도착하지 않았습니다. `console-open.png`에 해당 화면을 보관했습니다.
3. 입력 문자가 남아 있으면 첫 Escape는 문자를 지우고, 빈 상태에서 다음 Escape가 콘솔을 닫습니다. 그 뒤 물리 `W`가 다시 PlayerController에 도착했습니다. `console-closed.png`는 콘솔 종료 화면입니다.

이 시험은 콘솔이 게임 입력을 가로채는 경로를 확인한 것입니다. 탄 소진이 콘솔을 자동으로 열었다는 증거는 없으며, 보고된 영구 조작 불가의 원인으로 확정하지 않습니다. 후속으로 마지막 탄 발사·사망 직후 콘솔을 열었으나 매치 종료가 겹쳐 부활 후 콘솔 지속 시험의 판정에는 사용하지 않습니다.

`passive_input_capture.py`는 게임 상태를 바꾸지 않고 PIE의 물리 키 상태/pressed/released, 폰 소유, 사망 상태, 이동 차단, 입력 컴포넌트, 사망 UI, 무기·탄 수를 프레임별 JSONL로 기록하는 수단입니다. `passive-input-20261110-150047.jsonl`과 `passive-input-20261110-150330.jsonl`에 물리 콘솔 대조 근거가 있습니다. 보고 당시 환경에서 추가 관측할 수 있도록 준비했지만 해당 환경에서 실행한 기록은 아직 없습니다.

## 3. 결과창 AI 이름 표시: 기본 이름으로 재현, 조회 실패 출력 재사용 확인

### 관측

실제 서버 GameMode의 `EndMatch`를 호출해 결과를 생성하고 클라이언트의 결과 위젯을 읽었습니다. 테스트용 이름/순위 문자열을 주입하지 않은 기준 시험입니다.

- 사람 PlayerState: `PlayerName=Player`, `CampaignName` 빈 문자열, 0킬 1데스.
- 결과창의 사람 성적 행: 0킬 1데스이지만 이름이 바로 앞 AI 행과 같은 `Eschatologist`.
- 같은 세션의 스코어보드 행은 `Player`를 표시했습니다.
- 대조 시험으로 생성된 서버 `FinalRankingList`의 순서만 역전하고 복제 후 결과 위젯을 다시 채웠습니다. 같은 사람 행이 맨 앞일 때에는 `Player`로 정상 표시됐습니다.

### 원인

`WBP_Result.PopulateResults`는 결과의 `CharacterName`을 매 반복마다 `DT_Character`의 행 키로 조회하고, 성공과 실패 실행 핀을 같은 위젯 생성 경로에 연결합니다. 조회 출력의 `Character_Name`이 None인지에 따라 원래 문자열과 캐릭터 표시 이름 중 하나를 선택합니다.

엔진 `UDataTableFunctionLibrary::Generic_GetDataTableRowFromName`은 행을 찾았을 때만 출력 구조체에 복사합니다. 조회 실패 시 출력 구조체를 초기화하지 않습니다. 같은 반복문의 직전 AI 조회 결과가 남아 있어 `Player` 조회 실패 뒤에도 직전 캐릭터 표시 이름을 선택합니다. 플레이어 행이 첫 번째이면 출력이 초기 상태이므로 원래 이름이 표시됩니다.

이는 기본 이름 `Player`가 `DT_Character`의 키와 충돌해서 생기는 문제가 아닙니다. 앞선 대화에서 키 충돌 가설만으로 기본 이름 사례를 배제했던 설명은 이 실행 조사로 정정합니다. 이름을 비워 접속해도 플레이어보다 앞에 유효한 AI 행이 있으면 재현할 수 있습니다.

스코어보드는 `CampaignName`이 비어 있으면 PlayerState의 `PlayerName`을 직접 선택하므로 같은 실패 출력에 의존하지 않습니다.

근거: `result-default-name.json`, `scoreboard-default-name.json`, `result-reversed-order.json`, `result-populate-graph.json`; 로컬 엔진 `Engine/Source/Runtime/Engine/Private/DataTableFunctionLibrary.cpp`의 `Generic_GetDataTableRowFromName`.

### 마지막 물리 마우스·콘솔 시도의 한계

`physical_last_ammo_probe.py`는 잔탄 1을 준비한 뒤 물리 입력에 의한 소진을 기다리도록 만들었습니다. 그러나 Windows 클릭 도구가 `coordinate input geometry is unavailable`을 반환해 물리 마우스로 마지막 탄을 발사하는 시험은 성립하지 않았습니다. 이후 서버 `TryUse`로 탄 소진·사망을 발생시키고 물리 콘솔 키를 사용한 시도로 전환했습니다. 해당 PIE가 15:10경 종료돼 마지막 대조 관측까지 완료하지 못했습니다. `physical-last-ammo.json`의 객체 조회 오류는 PIE 종료 후 관측 대상이 사라진 오류이며, 게임 조작 불가가 재현됐다는 증거로 사용하지 않습니다.

## 최종 판정과 조사 종료

- **1번 버프 중복 획득 — 재현 및 원인 확인:** 버프 부착 중 Overlap 재진입이 획득 완료 플래그보다 먼저 발생합니다.
- **3번 결과창 AI 이름 — 재현 및 원인 확인:** 테이블 조회 실패 후 직전 AI 조회 출력이 남아 이름 표시로 사용됩니다. 기본 이름 `Player`도 영향을 받습니다.
- **2번 부활 후 조작 불가 — 보고 증상 미재현, 원인 미확인:** 추가 13조건 중 잔탄 0 실제 도달은 10조건이며, 모두 부활 후 입력 복구를 확인했습니다. 개발 콘솔(grave/Tilde) 입력이 게임 입력을 가로채는 동작은 별도 확인했지만, 잔탄 소진이 콘솔을 열거나 보고 사례가 콘솔 때문에 발생했다는 근거는 없습니다.

2026-11-10 사용자 요청에 따라 부활 문제는 재현이 매우 까다로운 미확인 문제로 남기고, 개발 콘솔 입력을 **가능성**으로 기획에 전달하는 수준에서 이번 조사를 종료합니다. 세 문제 모두 원인이 확정됐다는 의미가 아닙니다. 기획 등 다른 사람에게 메시지를 직접 전송하지 않았습니다.

제품 구현은 수정하지 않았습니다. 이후 수정 시 1번은 효과 적용 전 서버 획득 예약과 실패 시 해제, 3번은 조회 성공 여부에 따른 명시적인 이름 선택이 필요합니다. 이 수정 방향의 구현·회귀 검증은 이번 조사 범위에 포함하지 않습니다.

## 후속 수정과 회귀 검증 — 2026-11-10

위 조사 종료 후 사용자 요청에 따라 확인된 두 문제를 수정했습니다. 이 절이 두 시스템의 최신 구현 상태입니다. 부활 문제는 수정 대상에 포함하지 않았습니다.

- `BP_DropItem.EventGraph`: 서버의 미획득 검사 뒤 `bPickupInProgress`를 검사·설정하고 효과 적용으로 진입합니다. 부착 중 재진입은 이 잠금에서 차단합니다. 효과 적용 실패 시 잠금을 해제해 다른 대상의 재시도를 허용합니다. 성공 시 기존 획득 완료 표시와 알림을 수행한 뒤 잠금을 해제합니다. 획득 상태 복제·소리·효과 처리 경로는 기존 흐름을 사용합니다.
- `WBP_Result.PopulateResults`: 각 행 처리에서 동일 `DT_Character` 키의 존재 여부를 먼저 확인합니다. 행이 없거나 표시 이름이 None이면 원래 결과 이름을 선택하고, 유효한 행에만 캐릭터 표시 이름을 사용합니다. 실패한 조회의 출력 구조체 값에 의존하지 않습니다.

MCP를 통해 두 Blueprint를 수정·컴파일·개별 저장했습니다. 둘 다 `UpToDate`로 컴파일됐습니다. 다른 dirty 에셋을 저장하지 않았으며 Live Coding을 사용하지 않았습니다.

`Scripts/Diagnostics/VerifyReportedIssueFixes.py`를 동일한 데디케이티드 서버+클라이언트 PIE에서 실행했습니다. 입력 서브시스템이나 제품 그래프를 우회해 효과를 직접 적용하지 않고, 이전 재현과 같이 실제 버프 아이템을 두 폰의 겹치는 충돌 범위로 이동시켰습니다.

| 회귀 항목 | 결과 |
|---|---|
| 두 폰 간격 90, Sphere 반지름 105, 공격 버프 동시 Overlap | 6/6에서 한 폰에만 적용 |
| 버프 컴포넌트 없는 대상의 획득 실패 후 정상 대상 재시도 | 잠금 해제 및 후속 획득 성공 |
| 획득된 아이템을 같은 범위로 재이동 | 추가 적용 없음 |
| 실제 EndMatch 결과에서 AI 뒤의 기본 이름 Player | 정상 표시 |
| 서버 결과 배열 순서를 뒤집어 Player를 앞으로 이동 | 정상 표시, AI 표시 이름과 행 데이터 유지 |

전체 fixture 결과는 `passed=true`입니다. 원본은 `Saved/Diagnostics/ReportedIssues/fix-regression.json`, 이식 가능한 요약은 [회귀 검증 요약](ReportedIssues-2026-11-10-regression.json)에 보관했습니다. fixture는 테스트 폰·아이템을 사용하는 PIE 전용이며, 실행 후 호출자가 PIE를 종료합니다. 실제 수행에서도 PIE를 종료했습니다. 패키징 빌드·Cook 검증은 수행하지 않았습니다.

## PR #962의 main 병합 및 재검증

main `4800a0d31b16e8c379bcfa8556a2d69f677f0477`과의 충돌은 `WBP_Result.uasset` 및 조사 문서 README 두 파일이었습니다. README는 두 문서 링크를 모두 보존했습니다. 결과창은 main 원본을 선택한 뒤, MCP로 이름 조회 실패 처리를 다시 적용했습니다. main에 추가된 촬영용 UI 마스크를 포함한 전체 위젯 트리는 재적용 전후 동일함을 확인했습니다. 버프 잠금 수정은 충돌 없이 유지됐습니다.

main의 새 native 클래스가 포함되지 않은 이전 Editor DLL에서는 `RecordingUIMask`/`UIRecordingSettings`를 조회할 수 없었고, RecordingUIMask 관련 로드 경고가 104건 발생했습니다. 사용자 승인 후 Editor를 종료하고 `ShootingArenaEditor Win64 Development -WaitMutex -NoHotReloadFromIDE` 일반 빌드를 수행해 성공했습니다. 재시작 후 두 클래스가 정상 조회됐으며 같은 종류의 시작 로드 경고는 0건이었습니다. Live Coding은 사용하지 않았습니다. 기존 `Unknown structure` 이슈는 사용자 지침에 따라 추가 조사하지 않았습니다.

통합 후 `Lvl_PortalMap`에서 fixture를 다시 실행해 `passed=true`를 확인했습니다. 버프 6회, 실패 후 재시도, 완료 후 재진입, Player의 원래/역순 표시와 AI 이름 보존이 모두 통과했습니다. 재시작 시 기본 메뉴 맵에서 잘못 시작한 최초 fixture는 전투 폰이 없어 중단됐으므로 검증 결과에 포함하지 않았습니다. fixture에 맵 전제 확인을 추가했으며, 검증 후 PIE를 종료하고 기본 메뉴 맵으로 복원했습니다. 위 회귀 요약 JSON은 이 통합 후 검증 결과로 갱신했습니다.
