# Steel_ABP_FP_BGH_ShotGun

에셋: `/Game/QuakeLike_1_0/Animation/Steel/ABP/Steel_ABP_FP_BGH_ShotGun`

부모: `/Script/CoreUObject.Class'/Script/Engine.AnimInstance'`

수집일: 2026-10-12. MCP에서 읽은 그래프·핀 연결의 자동 전사입니다. 상속된 그래프는 부모 문서를 함께 확인합니다.

정적 루트 참조 도달: 없음. 실행 여부 또는 삭제 가능 여부의 확정이 아닙니다.

컴포넌트 목록은 이 BP가 직접 정의한 SCS만 포함합니다. 부모·네이티브 생성 컴포넌트, 인스턴스 설정은 별도입니다.

## 그래프 목록

| 그래프 | 전체 노드 수 |
|---|---|
| AnimGraph | 8 |
| EventGraph | 0 |

## 그래프별 실행 연결

실선은 출력 exec 핀의 실제 연결입니다. 여러 분기는 출력 핀 이름을 표시합니다. 함수 호출·delegate·타이머의 다음 그래프 실행은 이 그림에서 자동으로 이어 붙이지 않습니다.

순수 계산 노드 및 데이터 핀은 아래 목록과 원본 JSON을 확인합니다. 연결되지 않은 이벤트도 표시되므로, 노드가 있다는 이유만으로 실행된다고 판단하면 안 됩니다.

### AnimGraph

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `AnimGraphNode_Root_43` | 출력 포즈<br/>AnimGraph |  |
| `AnimGraphNode_Slot_1` | Slot 'Arms'<br/>Group 'DefaultGroup' |  |
| `AnimGraphNode_SequencePlayer_2` | Steel_Trooper_AKRifle<br/>시퀀스 플레이어 |  |
| `AnimGraphNode_ModifyBone_0` | Transform (Modify) Bone<br/>Bone: None | ComponentPose=(LinkID=-1,SourceLinkID=-1); Alpha=1.000000; Translation=0.000000,0.000000,0.000000; Rotation=0.000000,0.000000,0.000000; Scale=1.000000,1.000000,1.000000 |
| `AnimGraphNode_SequencePlayer_6` | Steel_Sci-fi_Rifle_01_Idle_Stance_AnimSequence_Anim<br/>시퀀스 플레이어 |  |
| `AnimGraphNode_SequencePlayer_4` | Steel_Sci-fi_Sniper_Idle_Stance_AnimSequence_Anim<br/>시퀀스 플레이어 |  |
| `AnimGraphNode_SequencePlayer_7` | 시퀀스 플레이어 |  |
| `AnimGraphNode_SequencePlayer_0` | Steel_ShotGun<br/>시퀀스 플레이어 |  |

</details>

### EventGraph

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|

</details>

## 조회 범위의 한계

ObjectIterator로 발견했으나 MCP helper가 반환하지 않은 그래프입니다. delegate signature·컴파일된 중간 그래프·macro·애니메이션 내부 그래프가 섞일 수 있습니다.

- `AnimGraph__AnimFunc`: Graph 'AnimGraph__AnimFunc' not found in Blueprint.
- `AnimGraph__AnimFunc_MERGED`: Graph 'AnimGraph__AnimFunc_MERGED' not found in Blueprint.
- `ExecuteUbergraph_Steel_ABP_FP_BGH_ShotGun`: Graph 'ExecuteUbergraph_Steel_ABP_FP_BGH_ShotGun' not found in Blueprint.

[원본 MCP 스냅샷](../../../../../Evidence/blueprints-175.json)
