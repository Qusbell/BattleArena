# CR_Mannequin_FootIK

에셋: `/Game/Characters/Mannequins/Rigs/CR_Mannequin_FootIK`

부모: `/Script/CoreUObject.Class'/Script/ControlRig.ControlRig'`

수집일: 2026-10-12. MCP에서 읽은 그래프·핀 연결의 자동 전사입니다. 상속된 그래프는 부모 문서를 함께 확인합니다.

정적 루트 참조 도달: 있음. 실행 여부 또는 삭제 가능 여부의 확정이 아닙니다.

컴포넌트 목록은 이 BP가 직접 정의한 SCS만 포함합니다. 부모·네이티브 생성 컴포넌트, 인스턴스 설정은 별도입니다.

## 변수와 기본값

| 이름 | 타입 | CDO 기본값 |
|---|---|---|
| `ZOffset_L_Target` | `real` | 0.0 |
| `ZOffset_R_Target` | `real` | 0.0 |
| `ZOffset_L` | `real` | 0.0 |
| `ZOffset_R` | `real` | 0.0 |
| `ZOffset_Pelvis` | `real` | 0.0 |
| `ShouldDoIKTrace` | `bool` | False |

## 그래프 목록

| 그래프 | 전체 노드 수 |
|---|---|
| FootTrace | 8 |
| Rig | 0 |

## 그래프별 실행 연결

실선은 출력 exec 핀의 실제 연결입니다. 여러 분기는 출력 핀 이름을 표시합니다. 함수 호출·delegate·타이머의 다음 그래프 실행은 이 그림에서 자동으로 이어 붙이지 않습니다.

순수 계산 노드 및 데이터 핀은 아래 목록과 원본 JSON을 확인합니다. 연결되지 않은 이벤트도 표시되므로, 노드가 있다는 이유만으로 실행된다고 판단하면 안 됩니다.

### FootTrace

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `Entry` | Entry |  |
| `Return` | Return | Return.Hit Location.X=0.000000; Return.Hit Location.Y=0.000000; Return.Hit Location.Z=0.000000 |
| `GetTransform` | Get Transform - Bone | GetTransform.Item.Type=Bone; GetTransform.Item.Name=animation_root; GetTransform.Space=GlobalSpace |
| `GetTransform_1` | Get Transform - Bone | GetTransform_1.Item.Type=Bone; GetTransform_1.Item.Name=root; GetTransform_1.Space=GlobalSpace |
| `MathVectorAdd` | Add | MathVectorAdd.B.X=0.000000; MathVectorAdd.B.Y=0.000000; MathVectorAdd.B.Z=30.000000 |
| `MathVectorSub` | Subtract | MathVectorSub.B.X=0.000000; MathVectorSub.B.Y=0.000000; MathVectorSub.B.Z=50.000000 |
| `MathFloatAdd` | Add | MathFloatAdd.B=5.000000 |
| `SphereTraceByTraceChannel_1` | Sphere Trace By Trace Channel | SphereTraceByTraceChannel_1.Start.X=0.000000; SphereTraceByTraceChannel_1.Start.Y=0.000000; SphereTraceByTraceChannel_1.Start.Z=0.000000; SphereTraceByTraceChannel_1.End.X=0.000000; SphereTraceByTraceChannel_1.End.Y=0.000000; SphereTraceByTraceChannel_1.End.Z=0.000000; SphereTraceByTraceChannel_1.TraceChannel=TraceTypeQuery1; SphereTraceByTraceChannel_1.Radius=5.000000 |

</details>

### Rig

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|

</details>

## 조회 범위의 한계

ObjectIterator로 발견했으나 MCP helper가 반환하지 않은 그래프입니다. delegate signature·컴파일된 중간 그래프·macro·애니메이션 내부 그래프가 섞일 수 있습니다.

- `RigVMFunctionLibraryEdGraph`: Graph 'RigVMFunctionLibraryEdGraph' not found in Blueprint.

[원본 MCP 스냅샷](../../../../Evidence/blueprints-351.json)
