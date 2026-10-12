# WB_MedalContent

에셋: `/Game/CompShooterHUD/UI/Content/Medals/WB_MedalContent`

부모: `/Script/CoreUObject.Class'/Script/UMG.UserWidget'`

수집일: 2026-10-12. MCP에서 읽은 그래프·핀 연결의 자동 전사입니다. 상속된 그래프는 부모 문서를 함께 확인합니다.

정적 루트 참조 도달: 있음. 실행 여부 또는 삭제 가능 여부의 확정이 아닙니다.

컴포넌트 목록은 이 BP가 직접 정의한 SCS만 포함합니다. 부모·네이티브 생성 컴포넌트, 인스턴스 설정은 별도입니다.

## 그래프 목록

| 그래프 | 전체 노드 수 |
|---|---|
| EventGraph | 8 |

## 그래프별 실행 연결

실선은 출력 exec 핀의 실제 연결입니다. 여러 분기는 출력 핀 이름을 표시합니다. 함수 호출·delegate·타이머의 다음 그래프 실행은 이 그림에서 자동으로 이어 붙이지 않습니다.

순수 계산 노드 및 데이터 핀은 아래 목록과 원본 JSON을 확인합니다. 연결되지 않은 이벤트도 표시되므로, 노드가 있다는 이유만으로 실행된다고 판단하면 안 됩니다.

### EventGraph

<details>
<summary>실행 핀 플로우차트</summary>

```mermaid
flowchart LR
  n0["Set Text Color"]
  n1["Set Text"]
  n2["Add Medal to OV"]
  n3["Is Valid"]
  n4["Update Content Visibility"]
  n5["Delay"]
  n6["UpdateMedalContent<br/>Custom Event"]
  n0 -->|"then"| n2
  n1 -->|"then"| n0
  n2 -->|"then"| n3
  n3 -->|"Is Valid"| n5
  n5 -->|"then"| n4
  n6 -->|"then"| n1
```

</details>

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `K2Node_MacroInstance_1953` | Set Text Color |  |
| `K2Node_MacroInstance_1962` | Set Text |  |
| `K2Node_MacroInstance_2073` | Add Medal to OV |  |
| `K2Node_MacroInstance_108` | Is Valid |  |
| `K2Node_MacroInstance_2257` | Update Content Visibility | bVisible=true |
| `K2Node_CallFunction_102` | Delay | Duration=0.1 |
| `K2Node_CustomEvent_0` | UpdateMedalContent<br/>Custom Event |  |
| `EdGraphNode_Comment_151` | Event Update Medal Content |  |

</details>

## 조회 범위의 한계

ObjectIterator로 발견했으나 MCP helper가 반환하지 않은 그래프입니다. delegate signature·컴파일된 중간 그래프·macro·애니메이션 내부 그래프가 섞일 수 있습니다.

- `AddMedalToOV`: Graph 'AddMedalToOV' not found in Blueprint.
- `SetText`: Graph 'SetText' not found in Blueprint.
- `SetTextColor`: Graph 'SetTextColor' not found in Blueprint.
- `UpdateContentVisibility`: Graph 'UpdateContentVisibility' not found in Blueprint.
- `UpdateMedalContent`: Graph 'UpdateMedalContent' not found in Blueprint.

[원본 MCP 스냅샷](../../../../../Evidence/blueprints-281.json)
