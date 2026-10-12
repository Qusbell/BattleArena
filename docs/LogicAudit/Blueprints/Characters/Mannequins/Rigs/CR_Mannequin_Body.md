# CR_Mannequin_Body

에셋: `/Game/Characters/Mannequins/Rigs/CR_Mannequin_Body`

부모: `/Script/CoreUObject.Class'/Script/ControlRig.ControlRig'`

수집일: 2026-10-12. MCP에서 읽은 그래프·핀 연결의 자동 전사입니다. 상속된 그래프는 부모 문서를 함께 확인합니다.

정적 루트 참조 도달: 있음. 실행 여부 또는 삭제 가능 여부의 확정이 아닙니다.

컴포넌트 목록은 이 BP가 직접 정의한 SCS만 포함합니다. 부모·네이티브 생성 컴포넌트, 인스턴스 설정은 별도입니다.

## 변수와 기본값

| 이름 | 타입 | CDO 기본값 |
|---|---|---|
| `L Arm IK Mode` | `bool` | False |
| `R Arm IK Mode` | `bool` | False |
| `L Leg IK Mode ` | `bool` | True |
| `R Leg IK Mode` | `bool` | True |
| `Spine IK Mode` | `bool` | False |
| `Neck IK Mode` | `bool` | False |
| `Spine Offsets` | `Transform` | [] |
| `Spine Length` | `real` | 0.0 |
| `Neck Length` | `real` | 0.0 |
| `L Arm IK Align` | `bool` | False |
| `R Arm IK Align` | `bool` | False |

## 그래프 목록

| 그래프 | 전체 노드 수 |
|---|---|
| BSpline Passing thru 4 Points | 49 |
| Backward Arm | 33 |
| Backward Clavicle | 4 |
| Backward Fingers | 6 |
| Backward Foot | 18 |
| Backward Leg | 23 |
| Backward Neck | 29 |
| Backward Spine | 17 |
| Compute Pole Vector Parent | 7 |
| FK Point At | 33 |
| Find Finger Controls | 19 |
| Forward Arm | 102 |
| Forward Clavicle | 4 |
| Forward Fingers | 27 |
| Forward Foot | 75 |
| Forward Leg | 49 |
| Forward Neck | 67 |
| Forward Reverse Foot | 84 |
| Forward Spine | 64 |
| Get Negative Transform | 7 |
| IK 2 Bones | 53 |
| Interpolate Interacting Controls | 53 |
| Match 2 Bones IK onto FK | 14 |
| Project to New Parents | 6 |
| Rig Graph | 237 |
| Set IKFK Keys | 14 |
| Set IKFK Visibility | 12 |
| Setup Arm | 65 |
| Setup Clavicle | 7 |
| Setup Fingers | 14 |
| Setup Foot | 30 |
| Setup Leg | 57 |
| Setup Neck | 36 |
| Setup Reverse Foot | 29 |
| Setup Spine | 43 |
| Spine Match IK onto FK | 22 |

## 그래프별 실행 연결

실선은 출력 exec 핀의 실제 연결입니다. 여러 분기는 출력 핀 이름을 표시합니다. 함수 호출·delegate·타이머의 다음 그래프 실행은 이 그림에서 자동으로 이어 붙이지 않습니다.

순수 계산 노드 및 데이터 핀은 아래 목록과 원본 JSON을 확인합니다. 연결되지 않은 이벤트도 표시되므로, 노드가 있다는 이유만으로 실행된다고 판단하면 안 됩니다.

### BSpline Passing thru 4 Points

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `Entry` | Entry |  |
| `Return` | Return | Return.Execute=(); Return.Control Points=() |
| `MathVectorDistance` | Distance Between | MathVectorDistance.A.X=0.000000; MathVectorDistance.A.Y=0.000000; MathVectorDistance.A.Z=0.000000; MathVectorDistance.B.X=0.000000; MathVectorDistance.B.Y=0.000000; MathVectorDistance.B.Z=0.000000 |
| `MathVectorDistance_1` | Distance Between | MathVectorDistance_1.A.X=0.000000; MathVectorDistance_1.A.Y=0.000000; MathVectorDistance_1.A.Z=0.000000; MathVectorDistance_1.B.X=0.000000; MathVectorDistance_1.B.Y=0.000000; MathVectorDistance_1.B.Z=0.000000 |
| `MathVectorDistance_2` | Distance Between | MathVectorDistance_2.A.X=0.000000; MathVectorDistance_2.A.Y=0.000000; MathVectorDistance_2.A.Z=0.000000; MathVectorDistance_2.B.X=0.000000; MathVectorDistance_2.B.Y=0.000000; MathVectorDistance_2.B.Z=0.000000 |
| `MathFloatAdd` | Add |  |
| `MathFloatAdd_1` | Add |  |
| `MathFloatDiv` | Divide |  |
| `MathFloatDiv_1` | Divide |  |
| `EdGraphNode_Comment_16` | t1 |  |
| `EdGraphNode_Comment_17` | t2 |  |
| `MathVectorScale` | Scale | MathVectorScale.Value.X=0.000000; MathVectorScale.Value.Y=0.000000; MathVectorScale.Value.Z=0.000000 |
| `MathVectorScale_1` | Scale | MathVectorScale_1.Value.X=0.000000; MathVectorScale_1.Value.Y=0.000000; MathVectorScale_1.Value.Z=0.000000 |
| `MathVectorSub` | Subtract | MathVectorSub.A.X=0.000000; MathVectorSub.A.Y=0.000000; MathVectorSub.A.Z=0.000000; MathVectorSub.B.X=0.000000; MathVectorSub.B.Y=0.000000; MathVectorSub.B.Z=0.000000 |
| `MathVectorSub_1` | Subtract | MathVectorSub_1.A.X=0.000000; MathVectorSub_1.A.Y=0.000000; MathVectorSub_1.A.Z=0.000000; MathVectorSub_1.B.X=0.000000; MathVectorSub_1.B.Y=0.000000; MathVectorSub_1.B.Z=0.000000 |
| `VariableNode` | Get Point 1<br/>Input parameter |  |
| `MathVectorScale_2` | Scale | MathVectorScale_2.Value.X=0.000000; MathVectorScale_2.Value.Y=0.000000; MathVectorScale_2.Value.Z=0.000000 |
| `MathVectorScale_1_1` | Scale | MathVectorScale_1_1.Value.X=0.000000; MathVectorScale_1_1.Value.Y=0.000000; MathVectorScale_1_1.Value.Z=0.000000 |
| `VariableNode_2` | Get Point 0<br/>Input parameter |  |
| `VariableNode_3` | Get Point 3<br/>Input parameter |  |
| `VariableNode_1` | Get Point 2<br/>Input parameter |  |
| `MathVectorSub_4` | Subtract | MathVectorSub_4.A.X=0.000000; MathVectorSub_4.A.Y=0.000000; MathVectorSub_4.A.Z=0.000000; MathVectorSub_4.B.X=0.000000; MathVectorSub_4.B.Y=0.000000; MathVectorSub_4.B.Z=0.000000 |
| `MathVectorSub_1_3` | Subtract | MathVectorSub_1_3.A.X=0.000000; MathVectorSub_1_3.A.Y=0.000000; MathVectorSub_1_3.A.Z=0.000000; MathVectorSub_1_3.B.X=0.000000; MathVectorSub_1_3.B.Y=0.000000; MathVectorSub_1_3.B.Z=0.000000 |
| `Solve` | Solve<br/>Collapsed Graph | Solve.c.X=0.000000; Solve.c.Y=0.000000; Solve.c.Z=0.000000; Solve.f.X=0.000000; Solve.f.Y=0.000000; Solve.f.Z=0.000000 |
| `b1t1` | b1t1<br/>Collapsed Graph |  |
| `b2t1` | b2t1<br/>Collapsed Graph |  |
| `b1t2` | b1t2<br/>Collapsed Graph |  |
| `b2t2` | b2t2<br/>Collapsed Graph |  |
| `b0t1` | b0t1<br/>Collapsed Graph |  |
| `b3t1` | b3t1<br/>Collapsed Graph |  |
| `b0t2` | b0t2<br/>Collapsed Graph |  |
| `b3t2` | b3t2<br/>Collapsed Graph |  |
| `RerouteNode` | Float |  |
| `RerouteNode_1` | Float |  |
| `ControlRigSplineFromPoints` | Spline From Points | ControlRigSplineFromPoints.Points.0.X=0.000000; ControlRigSplineFromPoints.Points.0.Y=0.000000; ControlRigSplineFromPoints.Points.0.Z=0.000000; ControlRigSplineFromPoints.Points.1.X=0.000000; ControlRigSplineFromPoints.Points.1.Y=0.000000; ControlRigSplineFromPoints.Points.1.Z=0.000000; ControlRigSplineFromPoints.Points.2.X=0.000000; ControlRigSplineFromPoints.Points.2.Y=0.000000; ControlRigSplineFromPoints.Points.2.Z=0.000000; ControlRigSplineFromPoints.Points.3.X=0.000000; ControlRigSplineFromPoints.Points.3.Y=0.000000; ControlRigSplineFromPoints.Points.3.Z=0.000000; ControlRigSplineFromPoints.SplineMode=BSpline; ControlRigSplineFromPoints.bClosed=False; ControlRigSplineFromPoints.SamplesPerSegment=16 |
| `VariableNode_4` | Get Point 0<br/>Input parameter |  |
| `VariableNode_5` | Get Point 3<br/>Input parameter |  |
| `VariableNode_6` | Get Outputs<br/>Local Variable<br/>Default () |  |
| `ArraySetNum` |  |  |
| `ArraySetAtIndex` |  |  |
| `ArraySetAtIndex_1` |  |  |
| `ArraySetAtIndex_2` |  |  |
| `ArraySetAtIndex_3` |  |  |
| `RerouteNode_2` | 실행 컨텍스트 |  |
| `If` |  |  |
| `If_1` |  |  |
| `VariableNode_7` | Get Auto Weights<br/>Input parameter |  |
| `VariableNode_8` | Get Weight 1<br/>Input parameter |  |
| `VariableNode_9` | Get Weight 2<br/>Input parameter |  |

</details>

### Backward Arm

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `Entry` | Entry |  |
| `Return` | Return |  |
| `ArrayIterator_2` |  |  |
| `ArrayGetAtIndex` |  |  |
| `ProjectTransformToNewParent` | Project to new Parent | ProjectTransformToNewParent.Child.Type=Bone; ProjectTransformToNewParent.Child.Name=None; ProjectTransformToNewParent.bChildInitial=True; ProjectTransformToNewParent.OldParent.Type=Bone; ProjectTransformToNewParent.OldParent.Name=None; ProjectTransformToNewParent.bOldParentInitial=True; ProjectTransformToNewParent.NewParent.Type=Bone; ProjectTransformToNewParent.NewParent.Name=None |
| `SetTransform` | Set Transform - Bone | SetTransform.ExecuteContext=(); SetTransform.Item.Type=Bone; SetTransform.Item.Name=None; SetTransform.Space=GlobalSpace; SetTransform.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform.Value.Translation.X=0.000000; SetTransform.Value.Translation.Y=0.000000; SetTransform.Value.Translation.Z=0.000000; SetTransform.Value.Scale3D.X=1.000000; SetTransform.Value.Scale3D.Y=1.000000; SetTransform.Value.Scale3D.Z=1.000000; SetTransform.Weight=1.000000; SetTransform.bPropagateToChildren=True |
| `ProjectTransformToNewParent_24_3` | Project to new Parent | ProjectTransformToNewParent_24_3.Child.Type=Control; ProjectTransformToNewParent_24_3.Child.Name=arm_l_pv_ik_ctrl; ProjectTransformToNewParent_24_3.bChildInitial=True; ProjectTransformToNewParent_24_3.OldParent.Type=Bone; ProjectTransformToNewParent_24_3.OldParent.Name=upperarm_l; ProjectTransformToNewParent_24_3.bOldParentInitial=True; ProjectTransformToNewParent_24_3.NewParent.Type=Bone; ProjectTransformToNewParent_24_3.NewParent.Name=upperarm_l |
| `SetTransform_20_3` | Set Transform - Control | SetTransform_20_3.Item.Type=Control; SetTransform_20_3.Item.Name=hand_l_ik_ctrl; SetTransform_20_3.Space=GlobalSpace; SetTransform_20_3.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform_20_3.Value.Translation.X=0.000000; SetTransform_20_3.Value.Translation.Y=0.000000; SetTransform_20_3.Value.Translation.Z=0.000000; SetTransform_20_3.Value.Scale3D.X=1.000000; SetTransform_20_3.Value.Scale3D.Y=1.000000; SetTransform_20_3.Value.Scale3D.Z=1.000000; SetTransform_20_3.Weight=1.000000; SetTransform_20_3.bPropagateToChildren=True |
| `ProjectTransformToNewParent_24_1_3` | Project to new Parent | ProjectTransformToNewParent_24_1_3.Child.Type=Control; ProjectTransformToNewParent_24_1_3.Child.Name=hand_l_ik_ctrl; ProjectTransformToNewParent_24_1_3.bChildInitial=True; ProjectTransformToNewParent_24_1_3.OldParent.Type=Bone; ProjectTransformToNewParent_24_1_3.OldParent.Name=hand_l; ProjectTransformToNewParent_24_1_3.bOldParentInitial=True; ProjectTransformToNewParent_24_1_3.NewParent.Type=Bone; ProjectTransformToNewParent_24_1_3.NewParent.Name=hand_l |
| `SetTransform_21_2` | Set Transform - Control | SetTransform_21_2.Item.Type=Control; SetTransform_21_2.Item.Name=arm_l_pv_ik_ctrl; SetTransform_21_2.Space=GlobalSpace; SetTransform_21_2.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform_21_2.Value.Translation.X=0.000000; SetTransform_21_2.Value.Translation.Y=0.000000; SetTransform_21_2.Value.Translation.Z=0.000000; SetTransform_21_2.Value.Scale3D.X=1.000000; SetTransform_21_2.Value.Scale3D.Y=1.000000; SetTransform_21_2.Value.Scale3D.Z=1.000000; SetTransform_21_2.Weight=1.000000; SetTransform_21_2.bPropagateToChildren=True |
| `ProjectTransformToNewParent_2_1_1_2` | Project to new Parent | ProjectTransformToNewParent_2_1_1_2.Child.Type=Null; ProjectTransformToNewParent_2_1_1_2.Child.Name=upperarm_l_fk_ctrl_space; ProjectTransformToNewParent_2_1_1_2.bChildInitial=True; ProjectTransformToNewParent_2_1_1_2.OldParent.Type=Control; ProjectTransformToNewParent_2_1_1_2.OldParent.Name=clavicle_l_ctrl; ProjectTransformToNewParent_2_1_1_2.bOldParentInitial=True; ProjectTransformToNewParent_2_1_1_2.NewParent.Type=Control; ProjectTransformToNewParent_2_1_1_2.NewParent.Name=clavicle_l_ctrl |
| `ProjectTransformToNewParent_2_1_1_1_2` | Project to new Parent | ProjectTransformToNewParent_2_1_1_1_2.Child.Type=Null; ProjectTransformToNewParent_2_1_1_1_2.Child.Name=upperarm_l_fk_ctrl_space; ProjectTransformToNewParent_2_1_1_1_2.bChildInitial=True; ProjectTransformToNewParent_2_1_1_1_2.OldParent.Type=Control; ProjectTransformToNewParent_2_1_1_1_2.OldParent.Name=body_ctrl; ProjectTransformToNewParent_2_1_1_1_2.bOldParentInitial=True; ProjectTransformToNewParent_2_1_1_1_2.NewParent.Type=Control; ProjectTransformToNewParent_2_1_1_1_2.NewParent.Name=body_ctrl |
| `SetTransform_25` | Set Transform - Null | SetTransform_25.Item.Type=Null; SetTransform_25.Item.Name=upperarm_l_fk_ctrl_space; SetTransform_25.Space=GlobalSpace; SetTransform_25.Value.Translation.X=0.000000; SetTransform_25.Value.Translation.Y=0.000000; SetTransform_25.Value.Translation.Z=0.000000; SetTransform_25.Value.Scale3D.X=1.000000; SetTransform_25.Value.Scale3D.Y=1.000000; SetTransform_25.Value.Scale3D.Z=1.000000; SetTransform_25.Weight=1.000000; SetTransform_25.bPropagateToChildren=True |
| `EdGraphNode_Comment_12` | FK |  |
| `VariableNode` | Get UpV Control<br/>Input parameter |  |
| `VariableNode_1` | Get Ik Control<br/>Input parameter |  |
| `VariableNode_2` | Get Bones<br/>Input parameter |  |
| `ArrayGetAtIndex_1` |  |  |
| `ArrayGetAtIndex_1_1` |  |  |
| `VariableNode_2_1` | Get Bones<br/>Input parameter |  |
| `EdGraphNode_Comment_13` | IK |  |
| `ProjectTransformToNewParent_24_3_1` | Project to new Parent | ProjectTransformToNewParent_24_3_1.Child.Type=Control; ProjectTransformToNewParent_24_3_1.Child.Name=arm_l_pv_ik_ctrl; ProjectTransformToNewParent_24_3_1.bChildInitial=True; ProjectTransformToNewParent_24_3_1.OldParent.Type=Bone; ProjectTransformToNewParent_24_3_1.OldParent.Name=upperarm_l; ProjectTransformToNewParent_24_3_1.bOldParentInitial=True; ProjectTransformToNewParent_24_3_1.NewParent.Type=Bone; ProjectTransformToNewParent_24_3_1.NewParent.Name=upperarm_l |
| `SetTransform_21_2_1` | Set Transform - Control | SetTransform_21_2_1.Item.Type=Control; SetTransform_21_2_1.Item.Name=arm_l_pv_ik_ctrl; SetTransform_21_2_1.Space=GlobalSpace; SetTransform_21_2_1.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform_21_2_1.Value.Translation.X=0.000000; SetTransform_21_2_1.Value.Translation.Y=0.000000; SetTransform_21_2_1.Value.Translation.Z=0.000000; SetTransform_21_2_1.Value.Scale3D.X=1.000000; SetTransform_21_2_1.Value.Scale3D.Y=1.000000; SetTransform_21_2_1.Value.Scale3D.Z=1.000000; SetTransform_21_2_1.Weight=1.000000; SetTransform_21_2_1.bPropagateToChildren=True |
| `VariableNode_3` | Get Fingers Space<br/>Input parameter |  |
| `ArrayGetAtIndex_1_1_1` |  |  |
| `VariableNode_2_1_1` | Get Bones<br/>Input parameter |  |
| `EdGraphNode_Comment_14` | Fingers Space |  |
| `VariableNode_4` | Get FK Orient References<br/>Input parameter |  |
| `ArrayGetAtIndex_2` |  |  |
| `ArrayGetAtIndex_3` |  |  |
| `VariableNode_5` | Get FK Arm Space<br/>Input parameter |  |
| `EdGraphNode_Comment_0` | FK Arm Space |  |
| `SequenceExecution` | 시퀀스 |  |

</details>

### Backward Clavicle

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `Entry` | Entry |  |
| `Return` | Return |  |
| `ProjectTransformToNewParent_22_1_1_1` | Project to new Parent | ProjectTransformToNewParent_22_1_1_1.Child.Type=Control; ProjectTransformToNewParent_22_1_1_1.Child.Name=clavicle_r_ctrl; ProjectTransformToNewParent_22_1_1_1.bChildInitial=True; ProjectTransformToNewParent_22_1_1_1.OldParent.Type=Bone; ProjectTransformToNewParent_22_1_1_1.OldParent.Name=clavicle_r; ProjectTransformToNewParent_22_1_1_1.bOldParentInitial=True; ProjectTransformToNewParent_22_1_1_1.NewParent.Type=Bone; ProjectTransformToNewParent_22_1_1_1.NewParent.Name=clavicle_r |
| `SetTransform_17_1_1_1` | Set Transform - Control | SetTransform_17_1_1_1.Item.Type=Control; SetTransform_17_1_1_1.Item.Name=clavicle_r_ctrl; SetTransform_17_1_1_1.Space=GlobalSpace; SetTransform_17_1_1_1.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform_17_1_1_1.Value.Translation.X=0.000000; SetTransform_17_1_1_1.Value.Translation.Y=0.000000; SetTransform_17_1_1_1.Value.Translation.Z=0.000000; SetTransform_17_1_1_1.Value.Scale3D.X=1.000000; SetTransform_17_1_1_1.Value.Scale3D.Y=1.000000; SetTransform_17_1_1_1.Value.Scale3D.Z=1.000000; SetTransform_17_1_1_1.Weight=1.000000; SetTransform_17_1_1_1.bPropagateToChildren=True |

</details>

### Backward Fingers

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `Entry` | Entry |  |
| `Return` | Return | Return.Execute=() |
| `ArrayIterator` |  |  |
| `NameConcat_4_1` | Concat | NameConcat_4_1.A=None; NameConcat_4_1.B=_ctrl |
| `SetTransform_2` | Set Transform - Control | SetTransform_2.ExecuteContext=(); SetTransform_2.Item.Type=Control; SetTransform_2.Space=GlobalSpace; SetTransform_2.bInitial=false; SetTransform_2.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform_2.Value.Translation.X=0.000000; SetTransform_2.Value.Translation.Y=0.000000; SetTransform_2.Value.Translation.Z=0.000000; SetTransform_2.Value.Scale3D.X=1.000000; SetTransform_2.Value.Scale3D.Y=1.000000; SetTransform_2.Value.Scale3D.Z=1.000000; SetTransform_2.Weight=1.000000; SetTransform_2.bPropagateToChildren=True |
| `ProjectTransformToNewParent` | Project to new Parent | ProjectTransformToNewParent.Child.Type=Control; ProjectTransformToNewParent.bChildInitial=True; ProjectTransformToNewParent.OldParent.Type=Bone; ProjectTransformToNewParent.OldParent.Name=None; ProjectTransformToNewParent.bOldParentInitial=True; ProjectTransformToNewParent.NewParent.Type=Bone; ProjectTransformToNewParent.NewParent.Name=None |

</details>

### Backward Foot

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `Entry` | Entry |  |
| `Return` | Return |  |
| `ProjectTransformToNewParent_24_1_2` | Project to new Parent | ProjectTransformToNewParent_24_1_2.Child.Type=Control; ProjectTransformToNewParent_24_1_2.Child.Name=ball_l_ik_ctrl; ProjectTransformToNewParent_24_1_2.bChildInitial=True; ProjectTransformToNewParent_24_1_2.OldParent.Type=Bone; ProjectTransformToNewParent_24_1_2.OldParent.Name=ball_l; ProjectTransformToNewParent_24_1_2.bOldParentInitial=True; ProjectTransformToNewParent_24_1_2.NewParent.Type=Bone; ProjectTransformToNewParent_24_1_2.NewParent.Name=ball_l |
| `SetTransform_20_2` | Set Transform - Control | SetTransform_20_2.Item.Type=Control; SetTransform_20_2.Item.Name=ball_l_ik_ctrl; SetTransform_20_2.Space=GlobalSpace; SetTransform_20_2.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform_20_2.Value.Translation.X=0.000000; SetTransform_20_2.Value.Translation.Y=0.000000; SetTransform_20_2.Value.Translation.Z=0.000000; SetTransform_20_2.Value.Scale3D.X=1.000000; SetTransform_20_2.Value.Scale3D.Y=1.000000; SetTransform_20_2.Value.Scale3D.Z=1.000000; SetTransform_20_2.Weight=1.000000; SetTransform_20_2.bPropagateToChildren=True |
| `VariableNode` | Get Bone<br/>Input parameter |  |
| `VariableNode_1` | Get IK Control<br/>Input parameter |  |
| `ProjectTransformToNewParent_24_1_2_1` | Project to new Parent | ProjectTransformToNewParent_24_1_2_1.Child.Type=Control; ProjectTransformToNewParent_24_1_2_1.Child.Name=ball_l_ik_ctrl; ProjectTransformToNewParent_24_1_2_1.bChildInitial=True; ProjectTransformToNewParent_24_1_2_1.OldParent.Type=Bone; ProjectTransformToNewParent_24_1_2_1.OldParent.Name=ball_l; ProjectTransformToNewParent_24_1_2_1.bOldParentInitial=True; ProjectTransformToNewParent_24_1_2_1.NewParent.Type=Bone; ProjectTransformToNewParent_24_1_2_1.NewParent.Name=ball_l |
| `SetTransform_20_2_1` | Set Transform - Control | SetTransform_20_2_1.Item.Type=Control; SetTransform_20_2_1.Item.Name=ball_l_ik_ctrl; SetTransform_20_2_1.Space=GlobalSpace; SetTransform_20_2_1.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform_20_2_1.Value.Translation.X=0.000000; SetTransform_20_2_1.Value.Translation.Y=0.000000; SetTransform_20_2_1.Value.Translation.Z=0.000000; SetTransform_20_2_1.Value.Scale3D.X=1.000000; SetTransform_20_2_1.Value.Scale3D.Y=1.000000; SetTransform_20_2_1.Value.Scale3D.Z=1.000000; SetTransform_20_2_1.Weight=1.000000; SetTransform_20_2_1.bPropagateToChildren=True |
| `VariableNode_2` | Get Bone<br/>Input parameter |  |
| `VariableNode_3` | Get FK Control<br/>Input parameter |  |
| `EdGraphNode_Comment_2` | FK |  |
| `EdGraphNode_Comment_3` | IK |  |
| `SetTransform_2_1_1` | Set Transform - Bone | SetTransform_2_1_1.Item.Type=Bone; SetTransform_2_1_1.Item.Name=None; SetTransform_2_1_1.Space=GlobalSpace; SetTransform_2_1_1.bInitial=false; SetTransform_2_1_1.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform_2_1_1.Value.Translation.X=0.000000; SetTransform_2_1_1.Value.Translation.Y=0.000000; SetTransform_2_1_1.Value.Translation.Z=0.000000; SetTransform_2_1_1.Value.Scale3D.X=1.000000; SetTransform_2_1_1.Value.Scale3D.Y=1.000000; SetTransform_2_1_1.Value.Scale3D.Z=1.000000; SetTransform_2_1_1.Weight=1.000000; SetTransform_2_1_1.bPropagateToChildren=True |
| `EdGraphNode_Comment_9_1` | Initial Toes Space |  |
| `VariableNode_3_1` | Get Bone<br/>Input parameter |  |
| `GetTransform_1` | Get Transform - Bone | GetTransform_1.Item.Type=Bone; GetTransform_1.Item.Name=None; GetTransform_1.Space=GlobalSpace |
| `VariableNode_4` | Get Toes Space<br/>Input parameter |  |
| `SequenceExecution` | 시퀀스 |  |

</details>

### Backward Leg

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `Entry` | Entry |  |
| `Return` | Return |  |
| `ArrayIterator_2` |  |  |
| `ArrayGetAtIndex` |  |  |
| `ProjectTransformToNewParent` | Project to new Parent | ProjectTransformToNewParent.Child.Type=Bone; ProjectTransformToNewParent.Child.Name=None; ProjectTransformToNewParent.bChildInitial=True; ProjectTransformToNewParent.OldParent.Type=Bone; ProjectTransformToNewParent.OldParent.Name=None; ProjectTransformToNewParent.bOldParentInitial=True; ProjectTransformToNewParent.NewParent.Type=Bone; ProjectTransformToNewParent.NewParent.Name=None |
| `SetTransform` | Set Transform - Bone | SetTransform.ExecuteContext=(); SetTransform.Item.Type=Bone; SetTransform.Item.Name=None; SetTransform.Space=GlobalSpace; SetTransform.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform.Value.Translation.X=0.000000; SetTransform.Value.Translation.Y=0.000000; SetTransform.Value.Translation.Z=0.000000; SetTransform.Value.Scale3D.X=1.000000; SetTransform.Value.Scale3D.Y=1.000000; SetTransform.Value.Scale3D.Z=1.000000; SetTransform.Weight=1.000000; SetTransform.bPropagateToChildren=True |
| `ProjectTransformToNewParent_24_3` | Project to new Parent | ProjectTransformToNewParent_24_3.Child.Type=Control; ProjectTransformToNewParent_24_3.Child.Name=arm_l_pv_ik_ctrl; ProjectTransformToNewParent_24_3.bChildInitial=True; ProjectTransformToNewParent_24_3.OldParent.Type=Bone; ProjectTransformToNewParent_24_3.OldParent.Name=upperarm_l; ProjectTransformToNewParent_24_3.bOldParentInitial=True; ProjectTransformToNewParent_24_3.NewParent.Type=Bone; ProjectTransformToNewParent_24_3.NewParent.Name=upperarm_l |
| `SetTransform_20_3` | Set Transform - Control | SetTransform_20_3.Item.Type=Control; SetTransform_20_3.Item.Name=hand_l_ik_ctrl; SetTransform_20_3.Space=GlobalSpace; SetTransform_20_3.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform_20_3.Value.Translation.X=0.000000; SetTransform_20_3.Value.Translation.Y=0.000000; SetTransform_20_3.Value.Translation.Z=0.000000; SetTransform_20_3.Value.Scale3D.X=1.000000; SetTransform_20_3.Value.Scale3D.Y=1.000000; SetTransform_20_3.Value.Scale3D.Z=1.000000; SetTransform_20_3.Weight=1.000000; SetTransform_20_3.bPropagateToChildren=True |
| `ProjectTransformToNewParent_24_1_3` | Project to new Parent | ProjectTransformToNewParent_24_1_3.Child.Type=Control; ProjectTransformToNewParent_24_1_3.Child.Name=hand_l_ik_ctrl; ProjectTransformToNewParent_24_1_3.bChildInitial=True; ProjectTransformToNewParent_24_1_3.OldParent.Type=Bone; ProjectTransformToNewParent_24_1_3.OldParent.Name=hand_l; ProjectTransformToNewParent_24_1_3.bOldParentInitial=True; ProjectTransformToNewParent_24_1_3.NewParent.Type=Bone; ProjectTransformToNewParent_24_1_3.NewParent.Name=hand_l |
| `SetTransform_21_2` | Set Transform - Control | SetTransform_21_2.Item.Type=Control; SetTransform_21_2.Item.Name=arm_l_pv_ik_ctrl; SetTransform_21_2.Space=GlobalSpace; SetTransform_21_2.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform_21_2.Value.Translation.X=0.000000; SetTransform_21_2.Value.Translation.Y=0.000000; SetTransform_21_2.Value.Translation.Z=0.000000; SetTransform_21_2.Value.Scale3D.X=1.000000; SetTransform_21_2.Value.Scale3D.Y=1.000000; SetTransform_21_2.Value.Scale3D.Z=1.000000; SetTransform_21_2.Weight=1.000000; SetTransform_21_2.bPropagateToChildren=True |
| `EdGraphNode_Comment_12` | FK |  |
| `VariableNode` | Get Pole Vector Control<br/>Input parameter |  |
| `VariableNode_1` | Get Ik Control<br/>Input parameter |  |
| `VariableNode_2` | Get Bones<br/>Input parameter |  |
| `ArrayGetAtIndex_1` |  |  |
| `ArrayGetAtIndex_1_1` |  |  |
| `VariableNode_2_1` | Get Bones<br/>Input parameter |  |
| `EdGraphNode_Comment_13` | IK |  |
| `SequenceExecution` | 시퀀스 |  |
| `EdGraphNode_Comment_0` | UpV |  |
| `Compute Pole Vector Parent` | Compute Pole Vector Parent<br/>Local Function | Compute Pole Vector Parent.Foot Control.Type=Control; Compute Pole Vector Parent.Foot Control.Name=foot_l_ik_ctrl; Compute Pole Vector Parent.Root Bone.Type=Bone; Compute Pole Vector Parent.Root Bone.Name=thigh_l; Compute Pole Vector Parent.Pole Vector Null.Type=Control; Compute Pole Vector Parent.Pole Vector Null.Name=FootParent; Compute Pole Vector Parent.Initial=False |
| `VariableNode_1_1` | Get Ik Control<br/>Input parameter |  |
| `VariableNode_3` | Get Pole Vector Null<br/>Input parameter |  |

</details>

### Backward Neck

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `Entry` | Entry |  |
| `Return` | Return |  |
| `VariableNode_3` | Get FK Controls<br/>Input parameter |  |
| `Project to New Parents` | Project to New Parents<br/>Local Function | Project to New Parents.Propagate to Children=True |
| `EdGraphNode_Comment_2` | FK |  |
| `VariableNode_4_2` | Get IK Control<br/>Input parameter |  |
| `VariableNode_12_2` | Get Bones<br/>Input parameter |  |
| `ArrayGetAtIndex` |  |  |
| `ArrayGetNum` |  |  |
| `MathIntSub` | Subtract | MathIntSub.B=1 |
| `EdGraphNode_Comment_2_1` | IK |  |
| `ProjectTransformToNewParent` | Project to new Parent | ProjectTransformToNewParent.Child.Type=Bone; ProjectTransformToNewParent.Child.Name=None; ProjectTransformToNewParent.bChildInitial=True; ProjectTransformToNewParent.OldParent.Type=Bone; ProjectTransformToNewParent.OldParent.Name=None; ProjectTransformToNewParent.bOldParentInitial=True; ProjectTransformToNewParent.NewParent.Type=Bone; ProjectTransformToNewParent.NewParent.Name=None |
| `SetTransform` | Set Transform - Bone | SetTransform.Item.Type=Bone; SetTransform.Item.Name=None; SetTransform.Space=GlobalSpace; SetTransform.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform.Value.Translation.X=0.000000; SetTransform.Value.Translation.Y=0.000000; SetTransform.Value.Translation.Z=0.000000; SetTransform.Value.Scale3D.X=1.000000; SetTransform.Value.Scale3D.Y=1.000000; SetTransform.Value.Scale3D.Z=1.000000; SetTransform.Weight=1.000000; SetTransform.bPropagateToChildren=True |
| `ProjectTransformToNewParent_1` | Project to new Parent | ProjectTransformToNewParent_1.Child.Type=Null; ProjectTransformToNewParent_1.Child.Name=head_fk_space; ProjectTransformToNewParent_1.bChildInitial=True; ProjectTransformToNewParent_1.OldParent.Type=Control; ProjectTransformToNewParent_1.OldParent.Name=neck_02_ctrl; ProjectTransformToNewParent_1.bOldParentInitial=True; ProjectTransformToNewParent_1.NewParent.Type=Control; ProjectTransformToNewParent_1.NewParent.Name=neck_02_ctrl |
| `ProjectTransformToNewParent_1_1` | Project to new Parent | ProjectTransformToNewParent_1_1.Child.Type=Null; ProjectTransformToNewParent_1_1.Child.Name=head_fk_space; ProjectTransformToNewParent_1_1.bChildInitial=True; ProjectTransformToNewParent_1_1.OldParent.Type=Control; ProjectTransformToNewParent_1_1.OldParent.Name=body_ctrl; ProjectTransformToNewParent_1_1.bOldParentInitial=True; ProjectTransformToNewParent_1_1.NewParent.Type=Control; ProjectTransformToNewParent_1_1.NewParent.Name=body_ctrl |
| `ArrayGetAtIndex_1` |  |  |
| `VariableNode_3_1` | Get Head FK Space<br/>Input parameter |  |
| `VariableNode_5` | Get Head Orient Reference<br/>Input parameter |  |
| `EdGraphNode_Comment_11` | Head with Orient Space |  |
| `VariableNode_21` | Get Head IK Space<br/>Input parameter |  |
| `ProjectTransformToNewParent_1_2` | Project to new Parent | ProjectTransformToNewParent_1_2.Child.Type=Null; ProjectTransformToNewParent_1_2.Child.Name=head_fk_space; ProjectTransformToNewParent_1_2.bChildInitial=True; ProjectTransformToNewParent_1_2.OldParent.Type=Control; ProjectTransformToNewParent_1_2.OldParent.Name=neck_02_ctrl; ProjectTransformToNewParent_1_2.bOldParentInitial=True; ProjectTransformToNewParent_1_2.NewParent.Type=Control; ProjectTransformToNewParent_1_2.NewParent.Name=neck_02_ctrl |
| `ProjectTransformToNewParent_1_1_1_2` | Project to new Parent | ProjectTransformToNewParent_1_1_1_2.Child.Type=Null; ProjectTransformToNewParent_1_1_1_2.Child.Name=head_fk_space; ProjectTransformToNewParent_1_1_1_2.bChildInitial=True; ProjectTransformToNewParent_1_1_1_2.OldParent.Type=Control; ProjectTransformToNewParent_1_1_1_2.OldParent.Name=body_ctrl; ProjectTransformToNewParent_1_1_1_2.bOldParentInitial=True; ProjectTransformToNewParent_1_1_1_2.NewParent.Type=Control; ProjectTransformToNewParent_1_1_1_2.NewParent.Name=body_ctrl |
| `VariableNode_5_1_2` | Get Head Orient Reference<br/>Input parameter |  |
| `HierarchyGetParent` | Get Parent | HierarchyGetParent.Child.Type=Bone; HierarchyGetParent.Child.Name=None; HierarchyGetParent.bDefaultParent=True |
| `SetTransform_1` | Set Transform - Bone | SetTransform_1.Item.Type=Bone; SetTransform_1.Item.Name=None; SetTransform_1.Space=GlobalSpace; SetTransform_1.Value.Translation.X=0.000000; SetTransform_1.Value.Translation.Y=0.000000; SetTransform_1.Value.Translation.Z=0.000000; SetTransform_1.Value.Scale3D.X=1.000000; SetTransform_1.Value.Scale3D.Y=1.000000; SetTransform_1.Value.Scale3D.Z=1.000000; SetTransform_1.Weight=1.000000; SetTransform_1.bPropagateToChildren=True |
| `SetTransform_2` | Set Transform - Bone | SetTransform_2.Item.Type=Bone; SetTransform_2.Item.Name=None; SetTransform_2.Space=GlobalSpace; SetTransform_2.Value.Translation.X=0.000000; SetTransform_2.Value.Translation.Y=0.000000; SetTransform_2.Value.Translation.Z=0.000000; SetTransform_2.Value.Scale3D.X=1.000000; SetTransform_2.Value.Scale3D.Y=1.000000; SetTransform_2.Value.Scale3D.Z=1.000000; SetTransform_2.Weight=1.000000; SetTransform_2.bPropagateToChildren=True |
| `VariableNode_2` | Get Bones<br/>Input parameter |  |
| `VariableNode_2_1` | Get Bones<br/>Input parameter |  |
| `SequenceExecution` | 시퀀스 |  |

</details>

### Backward Spine

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `Entry` | Entry |  |
| `Return` | Return |  |
| `VariableNode_2_2_1` | Get Bones<br/>Input parameter |  |
| `EdGraphNode_Comment_4_1` | IK |  |
| `Project to New Parent_1` | Project to New Parents<br/>Local Function | Project to New Parent_1.Propagate to Children=True |
| `VariableNode` | Get FK Controls<br/>Input parameter |  |
| `VariableNode_1` | Get Bones<br/>Input parameter |  |
| `VariableNode_2` | Get Offsets<br/>Input parameter |  |
| `VariableNode_3` | Get IK Controls<br/>Input parameter |  |
| `EdGraphNode_Comment_0` | FK |  |
| `Spine Match IK onto FK` | Spine Match IK onto FK<br/>Local Function |  |
| `RigUnit_SetTransform` | Set Transform - Control | RigUnit_SetTransform.Item.Type=Control; RigUnit_SetTransform.Item.Name=hips_ctrl; RigUnit_SetTransform.Space=GlobalSpace; RigUnit_SetTransform.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); RigUnit_SetTransform.Value.Translation.X=0.000000; RigUnit_SetTransform.Value.Translation.Y=2.063274; RigUnit_SetTransform.Value.Translation.Z=89.104362; RigUnit_SetTransform.Value.Scale3D.X=1.000000; RigUnit_SetTransform.Value.Scale3D.Y=1.000000; RigUnit_SetTransform.Value.Scale3D.Z=1.000000; RigUnit_SetTransform.Weight=1.000000; RigUnit_SetTransform.bPropagateToChildren=True |
| `ProjectTransformToNewParent` | Project to new Parent | ProjectTransformToNewParent.Child.Type=Control; ProjectTransformToNewParent.Child.Name=hips_ctrl; ProjectTransformToNewParent.bChildInitial=True; ProjectTransformToNewParent.OldParent.Type=Bone; ProjectTransformToNewParent.OldParent.Name=pelvis; ProjectTransformToNewParent.bOldParentInitial=True; ProjectTransformToNewParent.NewParent.Type=Bone; ProjectTransformToNewParent.NewParent.Name=pelvis |
| `VariableNode_4` | Get FK Controls<br/>Input parameter |  |
| `ArrayGetAtIndex` |  |  |
| `VariableNode_5` | Get Pelvis Bone<br/>Input parameter |  |
| `SequenceExecution` | 시퀀스 |  |

</details>

### Compute Pole Vector Parent

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `Entry` | Entry |  |
| `Return` | Return |  |
| `RigUnit_GetTransform_4_1` | Get Transform - Bone | RigUnit_GetTransform_4_1.Item.Type=Bone; RigUnit_GetTransform_4_1.Item.Name=thigh_l; RigUnit_GetTransform_4_1.Space=GlobalSpace |
| `RigUnit_SetTransform_2_1` | Set Transform - Control | RigUnit_SetTransform_2_1.Item.Type=Control; RigUnit_SetTransform_2_1.Item.Name=FootParent; RigUnit_SetTransform_2_1.Space=GlobalSpace; RigUnit_SetTransform_2_1.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); RigUnit_SetTransform_2_1.Value.Translation.X=0.000000; RigUnit_SetTransform_2_1.Value.Translation.Y=0.000000; RigUnit_SetTransform_2_1.Value.Translation.Z=0.000000; RigUnit_SetTransform_2_1.Value.Scale3D.X=1.000000; RigUnit_SetTransform_2_1.Value.Scale3D.Y=1.000000; RigUnit_SetTransform_2_1.Value.Scale3D.Z=1.000000; RigUnit_SetTransform_2_1.Weight=1.000000; RigUnit_SetTransform_2_1.bPropagateToChildren=True |
| `AimBoneMath` | Aim Math | AimBoneMath.InputTransform.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); AimBoneMath.InputTransform.Translation.X=0.000000; AimBoneMath.InputTransform.Translation.Y=0.000000; AimBoneMath.InputTransform.Translation.Z=0.000000; AimBoneMath.InputTransform.Scale3D.X=1.000000; AimBoneMath.InputTransform.Scale3D.Y=1.000000; AimBoneMath.InputTransform.Scale3D.Z=1.000000; AimBoneMath.Primary.Weight=1.000000; AimBoneMath.Primary.Axis.X=1.000000; AimBoneMath.Primary.Axis.Y=0.000000; AimBoneMath.Primary.Axis.Z=0.000000; AimBoneMath.Primary.Target.X=0.000000; AimBoneMath.Primary.Target.Y=0.000000; AimBoneMath.Primary.Target.Z=0.000000; AimBoneMath.Primary.Kind=Location; AimBoneMath.Primary.Space.Type=Bone; AimBoneMath.Primary.Space.Name=None; AimBoneMath.Secondary.Weight=1.000000; AimBoneMath.Secondary.Axis.X=0.000000; AimBoneMath.Secondary.Axis.Y=0.000000; AimBoneMath.Secondary.Axis.Z=1.000000; AimBoneMath.Secondary.Target.X=1.000000; AimBoneMath.Secondary.Target.Y=0.000000; AimBoneMath.Secondary.Target.Z=0.000000; AimBoneMath.Secondary.Kind=Direction; AimBoneMath.Secondary.Space.Type=Bone; AimBoneMath.Secondary.Space.Name=None; AimBoneMath.Weight=1.000000 |
| `VariableNode` | Get Pole Vector Null<br/>Input parameter |  |
| `VariableNode_1` | Get Foot Control<br/>Input parameter |  |

</details>

### FK Point At

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `Entry` | Entry |  |
| `Return` | Return |  |
| `ArrayIterator_4` |  |  |
| `MathIntAdd` | Add | MathIntAdd.B=1 |
| `GetTransformItemArray` | Get Transform Array | GetTransformItemArray.Space=GlobalSpace |
| `ArrayGetAtIndex_11` |  |  |
| `SetTransform_2` | Set Transform - Bone | SetTransform_2.ExecuteContext=(); SetTransform_2.Item.Type=Bone; SetTransform_2.Item.Name=None; SetTransform_2.Space=GlobalSpace; SetTransform_2.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform_2.Value.Translation.X=0.000000; SetTransform_2.Value.Translation.Y=0.000000; SetTransform_2.Value.Translation.Z=0.000000; SetTransform_2.Value.Scale3D.X=1.000000; SetTransform_2.Value.Scale3D.Y=1.000000; SetTransform_2.Value.Scale3D.Z=1.000000; SetTransform_2.Weight=1.000000; SetTransform_2.bPropagateToChildren=True |
| `AimBoneMath` | Aim Math | AimBoneMath.InputTransform.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); AimBoneMath.InputTransform.Translation.X=0.000000; AimBoneMath.InputTransform.Translation.Y=0.000000; AimBoneMath.InputTransform.Translation.Z=0.000000; AimBoneMath.InputTransform.Scale3D.X=1.000000; AimBoneMath.InputTransform.Scale3D.Y=1.000000; AimBoneMath.InputTransform.Scale3D.Z=1.000000; AimBoneMath.Primary.Weight=1.000000; AimBoneMath.Primary.Axis.X=1.000000; AimBoneMath.Primary.Axis.Y=0.000000; AimBoneMath.Primary.Axis.Z=0.000000; AimBoneMath.Primary.Target.X=0.000000; AimBoneMath.Primary.Target.Y=0.000000; AimBoneMath.Primary.Target.Z=0.000000; AimBoneMath.Primary.Kind=Location; AimBoneMath.Primary.Space.Type=Bone; AimBoneMath.Primary.Space.Name=None; AimBoneMath.Secondary.Weight=1.000000; AimBoneMath.Secondary.Axis.X=0.000000; AimBoneMath.Secondary.Axis.Y=0.000000; AimBoneMath.Secondary.Axis.Z=1.000000; AimBoneMath.Secondary.Target.X=1.000000; AimBoneMath.Secondary.Target.Y=0.000000; AimBoneMath.Secondary.Target.Z=0.000000; AimBoneMath.Secondary.Kind=Direction; AimBoneMath.Secondary.Space.Type=Bone; AimBoneMath.Secondary.Space.Name=None; AimBoneMath.Weight=1.000000 |
| `ArrayGetAtIndex_10` |  |  |
| `ArrayGetAtIndex_10_1` |  |  |
| `MathIntGreaterEqual` | Greater Equal |  |
| `MathIntSub` | Subtract | MathIntSub.B=1 |
| `Branch_1` |  |  |
| `ArrayGetAtIndex_12` |  |  |
| `ArrayGetNum` |  |  |
| `MathIntSub_1` | Subtract | MathIntSub_1.B=1 |
| `ArrayGetAtIndex_12_1` |  |  |
| `ProjectTransformToNewParent_12_1_1_1` | Project to new Parent | ProjectTransformToNewParent_12_1_1_1.Child.Type=Bone; ProjectTransformToNewParent_12_1_1_1.Child.Name=None; ProjectTransformToNewParent_12_1_1_1.bChildInitial=True; ProjectTransformToNewParent_12_1_1_1.OldParent.Type=Control; ProjectTransformToNewParent_12_1_1_1.OldParent.Name=None; ProjectTransformToNewParent_12_1_1_1.bOldParentInitial=True; ProjectTransformToNewParent_12_1_1_1.NewParent.Type=Control; ProjectTransformToNewParent_12_1_1_1.NewParent.Name=None |
| `SetTransform_3` | Set Transform - Bone | SetTransform_3.Item.Type=Bone; SetTransform_3.Item.Name=None; SetTransform_3.Space=GlobalSpace; SetTransform_3.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform_3.Value.Translation.X=0.000000; SetTransform_3.Value.Translation.Y=0.000000; SetTransform_3.Value.Translation.Z=0.000000; SetTransform_3.Value.Scale3D.X=1.000000; SetTransform_3.Value.Scale3D.Y=1.000000; SetTransform_3.Value.Scale3D.Z=1.000000; SetTransform_3.Weight=1.000000; SetTransform_3.bPropagateToChildren=True |
| `VariableNode_1` | Get Bones<br/>Input parameter |  |
| `VariableNode_2` | Get FK Controls<br/>Input parameter |  |
| `VariableNode_3` | Get Bones<br/>Input parameter |  |
| `VariableNode_4` | Get FK Controls<br/>Input parameter |  |
| `EdGraphNode_Comment_0` | Look at Next Control |  |
| `EdGraphNode_Comment_1` | Last Joint |  |
| `VariableNode_6` | Get Point At Axis<br/>Input parameter |  |
| `VariableNode_7` | Get Up Vector Axis<br/>Input parameter |  |
| `VariableNode_8` | Get Up Vector Target<br/>Input parameter |  |
| `VariableNode_9` | Get Negative Side<br/>Input parameter |  |
| `MathTransformMul` | Multiply | MathTransformMul.A.Translation.X=0.000000; MathTransformMul.A.Translation.Y=0.000000; MathTransformMul.A.Translation.Z=0.000000; MathTransformMul.A.Scale3D.X=1.000000; MathTransformMul.A.Scale3D.Y=1.000000; MathTransformMul.A.Scale3D.Z=1.000000; MathTransformMul.B.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); MathTransformMul.B.Translation.X=0.000000; MathTransformMul.B.Translation.Y=0.000000; MathTransformMul.B.Translation.Z=0.000000; MathTransformMul.B.Scale3D.X=1.000000; MathTransformMul.B.Scale3D.Y=1.000000; MathTransformMul.B.Scale3D.Z=1.000000 |
| `If_2` |  |  |
| `SequenceExecution` | 시퀀스 |  |
| `MathQuaternionFromRotator` | From Rotator | MathQuaternionFromRotator.Value.Pitch=180.000000; MathQuaternionFromRotator.Value.Yaw=0.000000; MathQuaternionFromRotator.Value.Roll=0.000000 |

</details>

### Find Finger Controls

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `Entry` | Entry |  |
| `Return` | Return |  |
| `VariableNode` | Get Matching Items<br/>Local Variable<br/>Default () |  |
| `SequenceAggregate` | 시퀀스 |  |
| `ArrayReset` |  |  |
| `VariableNode_2` | Get Matching Items<br/>Local Variable<br/>Default () |  |
| `Chop_1` | Chop | Chop_1.Count=6; Chop_1.FromEnd=True |
| `CollectionNameSearchArray_1_1` | Item Name Search | CollectionNameSearchArray_1_1.TypeToSearch=Control |
| `Chop_1_1` | Chop | Chop_1_1.Count=5; Chop_1_1.FromEnd=false |
| `EndsWith_1` | Ends With | EndsWith_1.Name=None |
| `ArrayIterator` |  |  |
| `Contains` | Contains | Contains.Name=None; Contains.Search=meta |
| `MathBoolNot` | Not |  |
| `VariableNode_1_1` | Get Matching Items<br/>Local Variable<br/>Default () |  |
| `ArrayAdd_1` |  |  |
| `Branch_1` |  |  |
| `MathBoolNot_1` | Not |  |
| `Contains_1` | Contains | Contains_1.Name=None; Contains_1.Search=toe |
| `MathBoolAnd` | And |  |

</details>

### Forward Arm

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `Entry` | Entry |  |
| `Return` | Return |  |
| `VariableNode` | Get FK Controls<br/>Input parameter |  |
| `VariableNode_2` | Get IK Control<br/>Input parameter |  |
| `VariableNode_3` | Get Pole Vector Control<br/>Input parameter |  |
| `EdGraphNode_Comment_1` | Visibility |  |
| `Branch` |  |  |
| `EdGraphNode_Comment_0` | FK |  |
| `VariableNode_6` | Get Pole Vector Control<br/>Input parameter |  |
| `VariableNode_5_1` | Get Bones<br/>Input parameter |  |
| `VariableNode_7` | Get IK Control<br/>Input parameter |  |
| `VariableNode_11` | Get Bones<br/>Input parameter |  |
| `VariableNode_18` | Get Pole Vector Control<br/>Input parameter |  |
| `VariableNode_20` | Get Bones<br/>Input parameter |  |
| `VariableNode_21` | Get FK Controls<br/>Input parameter |  |
| `EdGraphNode_Comment_6` | Match FK onto IK |  |
| `ProjectTransformToNewParent_2_1_1` | Project to new Parent | ProjectTransformToNewParent_2_1_1.Child.Type=Null; ProjectTransformToNewParent_2_1_1.Child.Name=upperarm_l_fk_ctrl_space; ProjectTransformToNewParent_2_1_1.bChildInitial=True; ProjectTransformToNewParent_2_1_1.OldParent.Type=Control; ProjectTransformToNewParent_2_1_1.OldParent.Name=clavicle_l_ctrl; ProjectTransformToNewParent_2_1_1.bOldParentInitial=True; ProjectTransformToNewParent_2_1_1.NewParent.Type=Control; ProjectTransformToNewParent_2_1_1.NewParent.Name=clavicle_l_ctrl |
| `ProjectTransformToNewParent_2_1_1_1` | Project to new Parent | ProjectTransformToNewParent_2_1_1_1.Child.Type=Null; ProjectTransformToNewParent_2_1_1_1.Child.Name=upperarm_l_fk_ctrl_space; ProjectTransformToNewParent_2_1_1_1.bChildInitial=True; ProjectTransformToNewParent_2_1_1_1.OldParent.Type=Control; ProjectTransformToNewParent_2_1_1_1.OldParent.Name=body_ctrl; ProjectTransformToNewParent_2_1_1_1.bOldParentInitial=True; ProjectTransformToNewParent_2_1_1_1.NewParent.Type=Control; ProjectTransformToNewParent_2_1_1_1.NewParent.Name=body_ctrl |
| `GetControlBool_5` | Get Control Bool |  |
| `VariableNode_9` | Get Arm Space<br/>Input parameter |  |
| `SetTransform` | Set Transform - Bone | SetTransform.Item.Type=Bone; SetTransform.Item.Name=None; SetTransform.Space=GlobalSpace; SetTransform.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform.Value.Translation.X=0.000000; SetTransform.Value.Translation.Y=0.000000; SetTransform.Value.Translation.Z=0.000000; SetTransform.Value.Scale3D.X=1.000000; SetTransform.Value.Scale3D.Y=1.000000; SetTransform.Value.Scale3D.Z=1.000000; SetTransform.Weight=1.000000; SetTransform.bPropagateToChildren=True |
| `If` |  |  |
| `VariableNode_22` | Get FK Orient Control<br/>Input parameter |  |
| `VariableNode_23` | Get FK Orient References<br/>Input parameter |  |
| `ArrayGetAtIndex_7` |  |  |
| `ArrayGetAtIndex_8` |  |  |
| `EdGraphNode_Comment_7` | FK Orient |  |
| `SetTransform_1` | Set Transform - Bone | SetTransform_1.Item.Type=Bone; SetTransform_1.Item.Name=None; SetTransform_1.Space=GlobalSpace; SetTransform_1.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform_1.Value.Translation.X=0.000000; SetTransform_1.Value.Translation.Y=0.000000; SetTransform_1.Value.Translation.Z=0.000000; SetTransform_1.Value.Scale3D.X=1.000000; SetTransform_1.Value.Scale3D.Y=1.000000; SetTransform_1.Value.Scale3D.Z=1.000000; SetTransform_1.Weight=1.000000; SetTransform_1.bPropagateToChildren=True |
| `VariableNode_24` | Get Finger Space<br/>Input parameter |  |
| `GetTransform` | Get Transform - Bone | GetTransform.Item.Type=Bone; GetTransform.Item.Name=None; GetTransform.Space=GlobalSpace |
| `VariableNode_5_2` | Get Bones<br/>Input parameter |  |
| `ArrayGetAtIndex_9` |  |  |
| `EdGraphNode_Comment_14` | Fingers Space |  |
| `GetTransform_1_1` | Get Transform - Bone | GetTransform_1_1.Item.Type=Bone; GetTransform_1_1.Item.Name=None; GetTransform_1_1.Space=LocalSpace |
| `MathFloatGreater_1` | Greater | MathFloatGreater_1.B=0.500000 |
| `VariableNode_15_1_1` | Get IKFK Switch Control<br/>Input parameter |  |
| `VariableNode_25` | Get Negative Side<br/>Input parameter |  |
| `GetTransform_1_2` | Get Transform - Bone | GetTransform_1_2.Item.Type=Bone; GetTransform_1_2.Item.Name=None; GetTransform_1_2.Space=LocalSpace |
| `MathFloatGreater_2` | Greater |  |
| `If_3` |  |  |
| `VariableNode_26` | Get Negative Side<br/>Input parameter |  |
| `Get Negative Transform` | Get Negative Transform<br/>Local Function | Get Negative Transform.Input.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); Get Negative Transform.Input.Translation.X=0.000000; Get Negative Transform.Input.Translation.Y=0.000000; Get Negative Transform.Input.Translation.Z=0.000000; Get Negative Transform.Input.Scale3D.X=0.000000; Get Negative Transform.Input.Scale3D.Y=0.000000; Get Negative Transform.Input.Scale3D.Z=0.000000 |
| `EdGraphNode_Comment_9` | Set Keys |  |
| `FK Point At` | FK Point At<br/>Local Function | FK Point At.ExecuteContext=(); FK Point At.Point At Axis.X=1.000000; FK Point At.Point At Axis.Y=0.000000; FK Point At.Point At Axis.Z=0.000000; FK Point At.Up Vector Axis.X=0.000000; FK Point At.Up Vector Axis.Y=0.000000; FK Point At.Up Vector Axis.Z=1.000000; FK Point At.Up Vector Target.X=0.000000; FK Point At.Up Vector Target.Y=0.000000; FK Point At.Up Vector Target.Z=1.000000 |
| `VariableNode_1` | Get Negative Side<br/>Input parameter |  |
| `VariableNode_4` | Get Bones<br/>Input parameter |  |
| `VariableNode_5` | Get FK Controls<br/>Input parameter |  |
| `VariableNode_12` | Get Enable Stretch<br/>Input parameter |  |
| `VariableNode_13` | Set IK Mode<br/>Local Variable |  |
| `VariableNode_28` | Get IK Mode<br/>Local Variable |  |
| `Set IKFK Visibility` | Set IKFK Visibility<br/>Local Function | Set IKFK Visibility.IK Controls.0.Type=None; Set IKFK Visibility.IK Controls.0.Name=None; Set IKFK Visibility.IK Controls.1.Type=None; Set IKFK Visibility.IK Controls.1.Name=None; Set IKFK Visibility.Show Controls=true |
| `VariableNode_29` | Get IK Mode<br/>Local Variable |  |
| `Match IK onto FK` | Match 2 Bones IK onto FK<br/>Local Function | Match IK onto FK.IK Control.Type=Control; Match IK onto FK.IK Control.Name=hand_l_ik_ctrl; Match IK onto FK.Pole Vector Control.Type=Control; Match IK onto FK.Pole Vector Control.Name=arm_l_pv_ik_ctrl |
| `IK 2 Bones` | IK 2 Bones<br/>Local Function | IK 2 Bones.IK Control.Type=Control; IK 2 Bones.IK Control.Name=hand_l_ik_ctrl; IK 2 Bones.Pole Vector Control.Type=Control; IK 2 Bones.Pole Vector Control.Name=arm_l_pv_ik_ctrl; IK 2 Bones.Primary Axis.X=1.000000; IK 2 Bones.Primary Axis.Y=0.000000; IK 2 Bones.Primary Axis.Z=0.000000; IK 2 Bones.Secondary Axis.X=0.000000; IK 2 Bones.Secondary Axis.Y=-1.000000; IK 2 Bones.Secondary Axis.Z=0.000000 |
| `EdGraphNode_Comment_3` | IK |  |
| `VariableNode_8` | Get IK Effector<br/>Input parameter |  |
| `Set IKFK Keys` | Set IKFK Keys<br/>Local Function | Set IKFK Keys.IK Controls .0.Type=None; Set IKFK Keys.IK Controls .0.Name=None; Set IKFK Keys.IK Controls .1.Type=None; Set IKFK Keys.IK Controls .1.Name=None; Set IKFK Keys.Switch Control.Type=None; Set IKFK Keys.Switch Control.Name=None |
| `VariableNode_10` | Get FK Controls<br/>Input parameter |  |
| `VariableNode_16` | Get IK Control<br/>Input parameter |  |
| `VariableNode_17` | Get Pole Vector Control<br/>Input parameter |  |
| `VariableNode_19` | Get IKFK Switch Control<br/>Input parameter |  |
| `VariableNode_30` | Get IK Mode<br/>Local Variable |  |
| `VariableNode_31` | Get Previous IK Mode<br/>Input parameter |  |
| `Project to New Parents` | Project to New Parents<br/>Local Function | Project to New Parents.Propagate to Children=True |
| `VariableNode_14` | Get Softness<br/>Input parameter |  |
| `SequenceExecution` | 시퀀스 |  |
| `SequenceExecution_1` | 시퀀스 |  |
| `Branch_1` |  |  |
| `ProjectTransformToNewParent` | Project to new Parent | ProjectTransformToNewParent.Child.Type=Bone; ProjectTransformToNewParent.Child.Name=hand_l; ProjectTransformToNewParent.bChildInitial=True; ProjectTransformToNewParent.OldParent.Type=Control; ProjectTransformToNewParent.OldParent.Name=hand_l_ikorient_ctrl; ProjectTransformToNewParent.bOldParentInitial=True; ProjectTransformToNewParent.NewParent.Type=Control; ProjectTransformToNewParent.NewParent.Name=hand_l_ikorient_ctrl |
| `SequenceAggregate` | 시퀀스 |  |
| `VariableNode_15` | Get IK Orient Null<br/>Input parameter |  |
| `VariableNode_27_1` | Get IK Orient Control<br/>Input parameter |  |
| `VariableNode_33` | Get Bones<br/>Input parameter |  |
| `ArrayGetAtIndex_1` |  |  |
| `ArrayGetAtIndex_1_1` |  |  |
| `VariableNode_33_1` | Get Bones<br/>Input parameter |  |
| `VariableNode_27_2` | Get IK Orient Control<br/>Input parameter |  |
| `VariableNode_32_1` | Get IK Orient<br/>Input parameter |  |
| `SetControlVisibility_1` | Set Control Visibility | SetControlVisibility_1.Item.Type=Control; SetControlVisibility_1.Item.Name=None |
| `VariableNode_28_1` | Get IK Mode<br/>Local Variable |  |
| `MathBoolAnd` | And |  |
| `VariableNode_32_1_1` | Get IK Orient<br/>Input parameter |  |
| `EdGraphNode_Comment_2` | IK Orient |  |
| `ProjectTransformToNewParent_1` | Project to new Parent | ProjectTransformToNewParent_1.Child.Type=Bone; ProjectTransformToNewParent_1.Child.Name=hand_l; ProjectTransformToNewParent_1.bChildInitial=True; ProjectTransformToNewParent_1.OldParent.Type=Control; ProjectTransformToNewParent_1.OldParent.Name=hand_l_ikorient_ctrl; ProjectTransformToNewParent_1.bOldParentInitial=True; ProjectTransformToNewParent_1.NewParent.Type=Control; ProjectTransformToNewParent_1.NewParent.Name=hand_l_ikorient_ctrl |
| `Set Transform` | Set Rotation - Bone | Set Transform.Item.Type=Bone; Set Transform.Item.Name=None; Set Transform.Space=GlobalSpace; Set Transform.bInitial=False; Set Transform.Weight=1.000000; Set Transform.bPropagateToChildren=True |
| `RigUnit_SetTransform_1` | Set Rotation - Bone | RigUnit_SetTransform_1.ExecuteContext=(); RigUnit_SetTransform_1.Item.Type=Bone; RigUnit_SetTransform_1.Item.Name=hand_l; RigUnit_SetTransform_1.Space=GlobalSpace; RigUnit_SetTransform_1.bInitial=False; RigUnit_SetTransform_1.Weight=1.000000; RigUnit_SetTransform_1.bPropagateToChildren=True |
| `VariableNode_7_1` | Get IK Control<br/>Input parameter |  |
| `VariableNode_7_2` | Get IK Control<br/>Input parameter |  |
| `Set Transform_1` | Set Rotation - Bone | Set Transform_1.ExecuteContext=(); Set Transform_1.Item.Type=Bone; Set Transform_1.Item.Name=None; Set Transform_1.Space=GlobalSpace; Set Transform_1.bInitial=False; Set Transform_1.Weight=1.000000; Set Transform_1.bPropagateToChildren=True |
| `Set Transform_1_1` | Set Rotation - Bone | Set Transform_1_1.ExecuteContext=(); Set Transform_1_1.Item.Type=Bone; Set Transform_1_1.Item.Name=None; Set Transform_1_1.Space=GlobalSpace; Set Transform_1_1.bInitial=False; Set Transform_1_1.Weight=1.000000; Set Transform_1_1.bPropagateToChildren=True |
| `Branch_2` |  |  |
| `VariableNode_32_1_1_1` | Get IK Orient<br/>Input parameter |  |
| `DISPATCH_RigDispatch_CoreNotEquals` | Not Equals |  |
| `VariableNode_27` | Get Previous Align<br/>Input parameter |  |
| `Branch_3` |  |  |
| `SequenceAggregate_1` | 시퀀스 | SequenceAggregate_1.ExecuteContext=() |
| `GetTransform_1` | Get Transform - Bone | GetTransform_1.Item.Type=Bone; GetTransform_1.Item.Name=None; GetTransform_1.Space=GlobalSpace |
| `VariableNode_27_1_1_1` | Get IK Orient Control<br/>Input parameter |  |
| `GetTransform_2` | Get Transform - Bone | GetTransform_2.Item.Type=Bone; GetTransform_2.Item.Name=None; GetTransform_2.Space=GlobalSpace |
| `SendEvent` | Send Event | SendEvent.Event=RequestAutoKey; SendEvent.Item.Type=None; SendEvent.Item.Name=None; SendEvent.bEnable=True; SendEvent.bOnlyDuringInteraction=True |
| `SendEvent_1` | Send Event | SendEvent_1.Event=RequestAutoKey; SendEvent_1.Item.Type=None; SendEvent_1.Item.Name=None; SendEvent_1.bEnable=True; SendEvent_1.bOnlyDuringInteraction=True |
| `EdGraphNode_Comment_4` | Match IK Align |  |

</details>

### Forward Clavicle

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `Entry` | Entry |  |
| `Return` | Return |  |
| `ProjectTransformToNewParent_2_2` | Project to new Parent | ProjectTransformToNewParent_2_2.Child.Type=Bone; ProjectTransformToNewParent_2_2.Child.Name=clavicle_r; ProjectTransformToNewParent_2_2.bChildInitial=True; ProjectTransformToNewParent_2_2.OldParent.Type=Control; ProjectTransformToNewParent_2_2.OldParent.Name=clavicle_r_ctrl; ProjectTransformToNewParent_2_2.bOldParentInitial=True; ProjectTransformToNewParent_2_2.NewParent.Type=Control; ProjectTransformToNewParent_2_2.NewParent.Name=clavicle_r_ctrl |
| `SetTransform_9_1_1` | Set Transform - Bone | SetTransform_9_1_1.Item.Type=Bone; SetTransform_9_1_1.Item.Name=clavicle_r; SetTransform_9_1_1.Space=GlobalSpace; SetTransform_9_1_1.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform_9_1_1.Value.Translation.X=0.000000; SetTransform_9_1_1.Value.Translation.Y=0.000000; SetTransform_9_1_1.Value.Translation.Z=0.000000; SetTransform_9_1_1.Value.Scale3D.X=1.000000; SetTransform_9_1_1.Value.Scale3D.Y=1.000000; SetTransform_9_1_1.Value.Scale3D.Z=1.000000; SetTransform_9_1_1.Weight=1.000000; SetTransform_9_1_1.bPropagateToChildren=True |

</details>

### Forward Fingers

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `Entry` | Entry |  |
| `Return` | Return |  |
| `NameConcat_4_2` | Concat | NameConcat_4_2.A=None; NameConcat_4_2.B=_ctrl |
| `ProjectTransformToNewParent_19` | Project to new Parent | ProjectTransformToNewParent_19.Child.Type=Bone; ProjectTransformToNewParent_19.Child.Name=None; ProjectTransformToNewParent_19.bChildInitial=True; ProjectTransformToNewParent_19.OldParent.Type=Control; ProjectTransformToNewParent_19.bOldParentInitial=True; ProjectTransformToNewParent_19.NewParent.Type=Control |
| `SetTransform_6` | Set Transform - Bone | SetTransform_6.ExecuteContext=(); SetTransform_6.Item.Type=Bone; SetTransform_6.Item.Name=None; SetTransform_6.Space=GlobalSpace; SetTransform_6.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform_6.Value.Translation.X=0.000000; SetTransform_6.Value.Translation.Y=0.000000; SetTransform_6.Value.Translation.Z=0.000000; SetTransform_6.Value.Scale3D.X=1.000000; SetTransform_6.Value.Scale3D.Y=1.000000; SetTransform_6.Value.Scale3D.Z=1.000000; SetTransform_6.Weight=1.000000; SetTransform_6.bPropagateToChildren=True |
| `ArrayIterator` |  |  |
| `VariableNode_2` | Get Fingers Bones<br/>Input parameter |  |
| `Interpolate Interacting Controls` | Interpolate Interacting Controls<br/>Local Function |  |
| `VariableNode_5` | Get Metacarpals Controls<br/>Input parameter |  |
| `SequenceAggregate` | 시퀀스 |  |
| `ArrayIterator_1` |  |  |
| `IsInteracting` | Get Interaction |  |
| `ArrayFind` |  |  |
| `Branch` |  |  |
| `RigUnit_GetTransform_3` | Get Transform - Control | RigUnit_GetTransform_3.Item.Type=Control; RigUnit_GetTransform_3.Item.Name=pinky_curl_l_ctrl; RigUnit_GetTransform_3.Space=LocalSpace |
| `ArrayIterator_1_1` |  |  |
| `OffsetTransformForItem` | Offset Transform | OffsetTransformForItem.ExecuteContext=(); OffsetTransformForItem.Item.Type=Bone; OffsetTransformForItem.Item.Name=None; OffsetTransformForItem.OffsetTransform.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); OffsetTransformForItem.OffsetTransform.Translation.X=0.000000; OffsetTransformForItem.OffsetTransform.Translation.Y=0.000000; OffsetTransformForItem.OffsetTransform.Translation.Z=0.000000; OffsetTransformForItem.OffsetTransform.Scale3D.X=1.000000; OffsetTransformForItem.OffsetTransform.Scale3D.Y=1.000000; OffsetTransformForItem.OffsetTransform.Scale3D.Z=1.000000; OffsetTransformForItem.Weight=1.000000; OffsetTransformForItem.bPropagateToChildren=True |
| `RigUnit_SetTransform_1` | Set Transform - Control | RigUnit_SetTransform_1.ExecuteContext=(); RigUnit_SetTransform_1.Item.Type=Control; RigUnit_SetTransform_1.Item.Name=pinky_curl_l_ctrl; RigUnit_SetTransform_1.Space=LocalSpace; RigUnit_SetTransform_1.bInitial=False; RigUnit_SetTransform_1.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); RigUnit_SetTransform_1.Value.Translation.X=0.000000; RigUnit_SetTransform_1.Value.Translation.Y=0.000000; RigUnit_SetTransform_1.Value.Translation.Z=0.000000; RigUnit_SetTransform_1.Value.Scale3D.X=1.000000; RigUnit_SetTransform_1.Value.Scale3D.Y=1.000000; RigUnit_SetTransform_1.Value.Scale3D.Z=1.000000; RigUnit_SetTransform_1.Weight=1.000000; RigUnit_SetTransform_1.bPropagateToChildren=True |
| `Find Finger Controls` | Find Finger Controls<br/>Local Function | Find Finger Controls.ExecuteContext=(); Find Finger Controls.Value=None |
| `Interpolate Interacting Controls_1` | Interpolate Interacting Controls<br/>Local Function |  |
| `VariableNode` | Get Base Fingers Controls<br/>Input parameter |  |
| `VariableNode_1` | Get Auto Base<br/>Input parameter |  |
| `SendEvent` | Send Event | SendEvent.Event=OpenUndoBracket; SendEvent.Item.Type=None; SendEvent.Item.Name=None; SendEvent.bEnable=True; SendEvent.bOnlyDuringInteraction=True |
| `SendEvent_1` | Send Event | SendEvent_1.Event=CloseUndoBracket; SendEvent_1.Item.Type=None; SendEvent_1.Item.Name=None; SendEvent_1.bEnable=True; SendEvent_1.bOnlyDuringInteraction=True |
| `VariableNode_4` | Get Proxy Controls<br/>Input parameter |  |
| `VariableNode_6` | Get Auto Meta<br/>Input parameter |  |
| `SendEvent_2` | Send Event | SendEvent_2.Event=RequestAutoKey; SendEvent_2.Item.Type=None; SendEvent_2.Item.Name=None; SendEvent_2.bEnable=True; SendEvent_2.bOnlyDuringInteraction=True |

</details>

### Forward Foot

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `Entry` | Entry |  |
| `Return` | Return |  |
| `SetTransform_13` | Set Rotation - Bone | SetTransform_13.ExecuteContext=(); SetTransform_13.Item.Type=Bone; SetTransform_13.Item.Name=ball_l; SetTransform_13.Space=GlobalSpace; SetTransform_13.bInitial=False; SetTransform_13.Weight=1.000000; SetTransform_13.bPropagateToChildren=True |
| `ProjectTransformToNewParent_3_1` | Project to new Parent | ProjectTransformToNewParent_3_1.Child.Type=Bone; ProjectTransformToNewParent_3_1.Child.Name=ball_l; ProjectTransformToNewParent_3_1.bChildInitial=True; ProjectTransformToNewParent_3_1.OldParent.Type=Control; ProjectTransformToNewParent_3_1.OldParent.Name=ball_l_ik_ctrl; ProjectTransformToNewParent_3_1.bOldParentInitial=True; ProjectTransformToNewParent_3_1.NewParent.Type=Control; ProjectTransformToNewParent_3_1.NewParent.Name=ball_l_ik_ctrl |
| `VariableNode_15_1` | Get IKFK Switch Control<br/>Input parameter |  |
| `GetTransform_1_1` | Get Transform - Bone | GetTransform_1_1.Item.Type=Bone; GetTransform_1_1.Item.Name=None; GetTransform_1_1.Space=LocalSpace |
| `MathFloatGreater_1` | Greater | MathFloatGreater_1.B=0.500000 |
| `Branch` |  |  |
| `VariableNode` | Get Bone<br/>Input parameter |  |
| `VariableNode_1` | Get IK Control<br/>Input parameter |  |
| `EdGraphNode_Comment_4` | IK |  |
| `VariableNode_2` | Get Bone<br/>Input parameter |  |
| `ProjectTransformToNewParent_3_1_1` | Project to new Parent | ProjectTransformToNewParent_3_1_1.Child.Type=Bone; ProjectTransformToNewParent_3_1_1.Child.Name=ball_l; ProjectTransformToNewParent_3_1_1.bChildInitial=True; ProjectTransformToNewParent_3_1_1.OldParent.Type=Control; ProjectTransformToNewParent_3_1_1.OldParent.Name=ball_l_ik_ctrl; ProjectTransformToNewParent_3_1_1.bOldParentInitial=True; ProjectTransformToNewParent_3_1_1.NewParent.Type=Control; ProjectTransformToNewParent_3_1_1.NewParent.Name=ball_l_ik_ctrl |
| `VariableNode_4` | Get FK Control<br/>Input parameter |  |
| `SetTransform_13_1` | Set Transform - Bone | SetTransform_13_1.ExecuteContext=(); SetTransform_13_1.Item.Type=Bone; SetTransform_13_1.Item.Name=ball_l; SetTransform_13_1.Space=GlobalSpace; SetTransform_13_1.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform_13_1.Value.Translation.X=0.000000; SetTransform_13_1.Value.Translation.Y=0.000000; SetTransform_13_1.Value.Translation.Z=0.000000; SetTransform_13_1.Value.Scale3D.X=1.000000; SetTransform_13_1.Value.Scale3D.Y=1.000000; SetTransform_13_1.Value.Scale3D.Z=1.000000; SetTransform_13_1.Weight=1.000000; SetTransform_13_1.bPropagateToChildren=True |
| `EdGraphNode_Comment_5` | FK |  |
| `ProjectTransformToNewParent_3_1_1_1` | Project to new Parent | ProjectTransformToNewParent_3_1_1_1.Child.Type=Bone; ProjectTransformToNewParent_3_1_1_1.Child.Name=ball_l; ProjectTransformToNewParent_3_1_1_1.bChildInitial=True; ProjectTransformToNewParent_3_1_1_1.OldParent.Type=Control; ProjectTransformToNewParent_3_1_1_1.OldParent.Name=ball_l_ik_ctrl; ProjectTransformToNewParent_3_1_1_1.bOldParentInitial=True; ProjectTransformToNewParent_3_1_1_1.NewParent.Type=Control; ProjectTransformToNewParent_3_1_1_1.NewParent.Name=ball_l_ik_ctrl |
| `VariableNode_1_1` | Get IK Control<br/>Input parameter |  |
| `VariableNode_2_1` | Get Bone<br/>Input parameter |  |
| `SetTransform_13_1_1` | Set Transform - Bone | SetTransform_13_1_1.Item.Type=Bone; SetTransform_13_1_1.Item.Name=ball_l; SetTransform_13_1_1.Space=GlobalSpace; SetTransform_13_1_1.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform_13_1_1.Value.Translation.X=0.000000; SetTransform_13_1_1.Value.Translation.Y=0.000000; SetTransform_13_1_1.Value.Translation.Z=0.000000; SetTransform_13_1_1.Value.Scale3D.X=1.000000; SetTransform_13_1_1.Value.Scale3D.Y=1.000000; SetTransform_13_1_1.Value.Scale3D.Z=1.000000; SetTransform_13_1_1.Weight=1.000000; SetTransform_13_1_1.bPropagateToChildren=True |
| `EdGraphNode_Comment_6` | Match Ik onto FK |  |
| `EdGraphNode_Comment_4_1_1` | Set Keys |  |
| `Branch_4_1_1` |  |  |
| `VariableNode_12_1_1` | Get IK Mode<br/>Local Variable<br/>Default True |  |
| `VariableNode_13_1_1` | Set IK Mode<br/>Local Variable<br/>Default True | VariableNode_13_1_1.Value=True |
| `SendEvent_2_1` | Send Event | SendEvent_2_1.ExecuteContext=(); SendEvent_2_1.Event=RequestAutoKey; SendEvent_2_1.Item.Type=None; SendEvent_2_1.Item.Name=None; SendEvent_2_1.bEnable=True; SendEvent_2_1.bOnlyDuringInteraction=True |
| `VariableNode_3_1` | Get IK Control<br/>Input parameter |  |
| `SendEvent_2_1_1` | Send Event | SendEvent_2_1_1.Event=RequestAutoKey; SendEvent_2_1_1.Item.Type=None; SendEvent_2_1_1.Item.Name=None; SendEvent_2_1_1.bEnable=True; SendEvent_2_1_1.bOnlyDuringInteraction=True |
| `VariableNode_5` | Get FK Control<br/>Input parameter |  |
| `Branch_4_1_1_1` |  |  |
| `VariableNode_12_1_1_1` | Get IK Mode<br/>Local Variable<br/>Default True |  |
| `SendEvent_2_1_2` | Send Event | SendEvent_2_1_2.ExecuteContext=(); SendEvent_2_1_2.Event=RequestAutoKey; SendEvent_2_1_2.Item.Type=None; SendEvent_2_1_2.Item.Name=None; SendEvent_2_1_2.bEnable=True; SendEvent_2_1_2.bOnlyDuringInteraction=True |
| `VariableNode_3_1_1` | Get IK Control<br/>Input parameter |  |
| `SendEvent_2_1_1_1` | Send Event | SendEvent_2_1_1_1.Event=RequestAutoKey; SendEvent_2_1_1_1.Item.Type=None; SendEvent_2_1_1_1.Item.Name=None; SendEvent_2_1_1_1.bEnable=True; SendEvent_2_1_1_1.bOnlyDuringInteraction=True |
| `VariableNode_5_1` | Get FK Control<br/>Input parameter |  |
| `EdGraphNode_Comment_4_1_1_1` | Set Keys |  |
| `VariableNode_13_1_1_1` | Set IK Mode<br/>Local Variable<br/>Default True | VariableNode_13_1_1_1.Value=True |
| `EdGraphNode_Comment_6_1` | Match Ik onto FK |  |
| `ProjectTransformToNewParent_3_1_1_1_1` | Project to new Parent | ProjectTransformToNewParent_3_1_1_1_1.Child.Type=Bone; ProjectTransformToNewParent_3_1_1_1_1.Child.Name=ball_l; ProjectTransformToNewParent_3_1_1_1_1.bChildInitial=True; ProjectTransformToNewParent_3_1_1_1_1.OldParent.Type=Control; ProjectTransformToNewParent_3_1_1_1_1.OldParent.Name=ball_l_ik_ctrl; ProjectTransformToNewParent_3_1_1_1_1.bOldParentInitial=True; ProjectTransformToNewParent_3_1_1_1_1.NewParent.Type=Control; ProjectTransformToNewParent_3_1_1_1_1.NewParent.Name=ball_l_ik_ctrl |
| `VariableNode_2_1_1` | Get Bone<br/>Input parameter |  |
| `SetTransform_13_1_1_1` | Set Transform - Bone | SetTransform_13_1_1_1.Item.Type=Bone; SetTransform_13_1_1_1.Item.Name=ball_l; SetTransform_13_1_1_1.Space=GlobalSpace; SetTransform_13_1_1_1.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform_13_1_1_1.Value.Translation.X=0.000000; SetTransform_13_1_1_1.Value.Translation.Y=0.000000; SetTransform_13_1_1_1.Value.Translation.Z=0.000000; SetTransform_13_1_1_1.Value.Scale3D.X=1.000000; SetTransform_13_1_1_1.Value.Scale3D.Y=1.000000; SetTransform_13_1_1_1.Value.Scale3D.Z=1.000000; SetTransform_13_1_1_1.Weight=1.000000; SetTransform_13_1_1_1.bPropagateToChildren=True |
| `VariableNode_3` | Get FK Control<br/>Input parameter |  |
| `SetControlVisibility_4` | Set Control Visibility | SetControlVisibility_4.Item.Type=Control; SetControlVisibility_4.Item.Name=upperarm_l_fk_ctrl |
| `MathBoolNot_3` | Not |  |
| `VariableNode_2_2` | Get IK Control<br/>Input parameter |  |
| `EdGraphNode_Comment_1` | Visibility |  |
| `VariableNode_6` | Get FK Control<br/>Input parameter |  |
| `SetControlVisibility_4_1` | Set Control Visibility | SetControlVisibility_4_1.Item.Type=Control; SetControlVisibility_4_1.Item.Name=upperarm_l_fk_ctrl |
| `SetTransform_1` | Set Transform - Bone | SetTransform_1.Item.Type=Bone; SetTransform_1.Item.Name=None; SetTransform_1.Space=GlobalSpace; SetTransform_1.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform_1.Value.Translation.X=0.000000; SetTransform_1.Value.Translation.Y=0.000000; SetTransform_1.Value.Translation.Z=0.000000; SetTransform_1.Value.Scale3D.X=1.000000; SetTransform_1.Value.Scale3D.Y=1.000000; SetTransform_1.Value.Scale3D.Z=1.000000; SetTransform_1.Weight=1.000000; SetTransform_1.bPropagateToChildren=True |
| `EdGraphNode_Comment_14` | Toes Space |  |
| `VariableNode_7` | Get Toes Space<br/>Input parameter |  |
| `ProjectTransformToNewParent` | Project to new Parent | ProjectTransformToNewParent.Child.Type=Bone; ProjectTransformToNewParent.Child.Name=None; ProjectTransformToNewParent.bChildInitial=True; ProjectTransformToNewParent.OldParent.Type=Bone; ProjectTransformToNewParent.OldParent.Name=None; ProjectTransformToNewParent.bOldParentInitial=True; ProjectTransformToNewParent.NewParent.Type=Bone; ProjectTransformToNewParent.NewParent.Name=None |
| `VariableNode_8` | Get Bone<br/>Input parameter |  |
| `Branch_4_1_1_2` |  |  |
| `EdGraphNode_Comment_2` | Set Keys |  |
| `SendEvent_2_1_4` | Send Event | SendEvent_2_1_4.ExecuteContext=(); SendEvent_2_1_4.Event=RequestAutoKey; SendEvent_2_1_4.Item.Type=None; SendEvent_2_1_4.Item.Name=None; SendEvent_2_1_4.bEnable=True; SendEvent_2_1_4.bOnlyDuringInteraction=True |
| `VariableNode_3_1_2` | Get IK Control<br/>Input parameter |  |
| `SendEvent_2_1_1_3` | Send Event | SendEvent_2_1_1_3.Event=RequestAutoKey; SendEvent_2_1_1_3.Item.Type=None; SendEvent_2_1_1_3.Item.Name=None; SendEvent_2_1_1_3.bEnable=True; SendEvent_2_1_1_3.bOnlyDuringInteraction=True |
| `VariableNode_5_2` | Get FK Control<br/>Input parameter |  |
| `VariableNode_9` | Get Previous IK Mode<br/>Input parameter |  |
| `VariableNode_10` | Set IK Mode<br/>Local Variable<br/>Default True |  |
| `VariableNode_11` | Get IK Mode<br/>Local Variable<br/>Default True |  |
| `VariableNode_12` | Get IK Mode<br/>Local Variable<br/>Default True |  |
| `VariableNode_13` | Get IK Mode<br/>Local Variable<br/>Default True |  |
| `SequenceExecution` | 시퀀스 |  |
| `MathBoolNotEquals_2_1` | Not Equals |  |
| `OffsetTransformForItem` | Offset Transform | OffsetTransformForItem.Item.Type=Bone; OffsetTransformForItem.Item.Name=None; OffsetTransformForItem.OffsetTransform.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); OffsetTransformForItem.OffsetTransform.Translation.X=0.000000; OffsetTransformForItem.OffsetTransform.Translation.Y=0.000000; OffsetTransformForItem.OffsetTransform.Translation.Z=0.000000; OffsetTransformForItem.OffsetTransform.Scale3D.X=1.000000; OffsetTransformForItem.OffsetTransform.Scale3D.Y=1.000000; OffsetTransformForItem.OffsetTransform.Scale3D.Z=1.000000; OffsetTransformForItem.Weight=1.000000; OffsetTransformForItem.bPropagateToChildren=True |
| `GetTransform` | Get Transform - Bone | GetTransform.Item.Type=Bone; GetTransform.Item.Name=None; GetTransform.Space=GlobalSpace |
| `GetTransform_1_2` | Get Transform - Bone | GetTransform_1_2.Item.Type=Bone; GetTransform_1_2.Item.Name=None; GetTransform_1_2.Space=GlobalSpace; GetTransform_1_2.bInitial=false |
| `Make Relative` | Make Relative | Make Relative.Global.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); Make Relative.Global.Translation.X=0.000000; Make Relative.Global.Translation.Y=0.000000; Make Relative.Global.Translation.Z=0.000000; Make Relative.Global.Scale3D.X=1.000000; Make Relative.Global.Scale3D.Y=1.000000; Make Relative.Global.Scale3D.Z=1.000000; Make Relative.Parent.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); Make Relative.Parent.Translation.X=0.000000; Make Relative.Parent.Translation.Y=0.000000; Make Relative.Parent.Translation.Z=0.000000; Make Relative.Parent.Scale3D.X=1.000000; Make Relative.Parent.Scale3D.Y=1.000000; Make Relative.Parent.Scale3D.Z=1.000000 |
| `GetTransform_1` | Get Transform - Bone | GetTransform_1.Item.Type=Bone; GetTransform_1.Item.Name=None; GetTransform_1.Space=GlobalSpace |
| `Add` | Add | Add.A.X=0.000000; Add.A.Y=0.000000; Add.A.Z=0.000000; Add.B.X=0.000000; Add.B.Y=0.000000; Add.B.Z=0.000000 |
| `GetTransform_1_3` | Get Transform - Bone | GetTransform_1_3.Item.Type=Bone; GetTransform_1_3.Item.Name=None; GetTransform_1_3.Space=LocalSpace |
| `Make Relative_1` | Make Relative | Make Relative_1.Global.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); Make Relative_1.Global.Translation.X=0.000000; Make Relative_1.Global.Translation.Y=0.000000; Make Relative_1.Global.Translation.Z=0.000000; Make Relative_1.Global.Scale3D.X=1.000000; Make Relative_1.Global.Scale3D.Y=1.000000; Make Relative_1.Global.Scale3D.Z=1.000000; Make Relative_1.Parent.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); Make Relative_1.Parent.Translation.X=0.000000; Make Relative_1.Parent.Translation.Y=0.000000; Make Relative_1.Parent.Translation.Z=0.000000; Make Relative_1.Parent.Scale3D.X=1.000000; Make Relative_1.Parent.Scale3D.Y=1.000000; Make Relative_1.Parent.Scale3D.Z=1.000000 |
| `Make Absolute` | Make Absolute | Make Absolute.Local.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); Make Absolute.Local.Translation.X=0.000000; Make Absolute.Local.Translation.Y=0.000000; Make Absolute.Local.Translation.Z=0.000000; Make Absolute.Local.Scale3D.X=1.000000; Make Absolute.Local.Scale3D.Y=1.000000; Make Absolute.Local.Scale3D.Z=1.000000; Make Absolute.Parent.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); Make Absolute.Parent.Translation.X=0.000000; Make Absolute.Parent.Translation.Y=0.000000; Make Absolute.Parent.Translation.Z=0.000000; Make Absolute.Parent.Scale3D.X=1.000000; Make Absolute.Parent.Scale3D.Y=1.000000; Make Absolute.Parent.Scale3D.Z=1.000000 |

</details>

### Forward Leg

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `Entry` | Entry |  |
| `Return` | Return |  |
| `VariableNode_2` | Get IK Control<br/>Input parameter |  |
| `VariableNode_3` | Get Pole Vector Control<br/>Input parameter |  |
| `Branch` |  |  |
| `VariableNode_4` | Get FK Controls<br/>Input parameter |  |
| `VariableNode_5` | Get Bones<br/>Input parameter |  |
| `EdGraphNode_Comment_0` | FK |  |
| `VariableNode_6` | Get Pole Vector Control<br/>Input parameter |  |
| `VariableNode_7` | Get IK Control<br/>Input parameter |  |
| `VariableNode_15_1` | Get IKFK Switch Control<br/>Input parameter |  |
| `VariableNode_18` | Get Pole Vector Control<br/>Input parameter |  |
| `GetTransform_1_1` | Get Transform - Bone | GetTransform_1_1.Item.Type=Bone; GetTransform_1_1.Item.Name=None; GetTransform_1_1.Space=LocalSpace |
| `MathFloatGreater_1` | Greater | MathFloatGreater_1.B=0.500000 |
| `VariableNode_22` | Get Previous IK Mode<br/>Input parameter |  |
| `EdGraphNode_Comment_7` | Set Keys |  |
| `FK Point At` | FK Point At<br/>Local Function | FK Point At.ExecuteContext=(); FK Point At.Point At Axis.X=-1.000000; FK Point At.Point At Axis.Y=0.000000; FK Point At.Point At Axis.Z=0.000000; FK Point At.Up Vector Axis.X=0.000000; FK Point At.Up Vector Axis.Y=0.000000; FK Point At.Up Vector Axis.Z=1.000000; FK Point At.Up Vector Target.X=0.000000; FK Point At.Up Vector Target.Y=0.000000; FK Point At.Up Vector Target.Z=1.000000 |
| `VariableNode_1` | Get Negative Side<br/>Input parameter |  |
| `Match IK onto FK` | Match 2 Bones IK onto FK<br/>Local Function | Match IK onto FK.IK Control.Type=Control; Match IK onto FK.IK Control.Name=hand_l_ik_ctrl; Match IK onto FK.Pole Vector Control.Type=Control; Match IK onto FK.Pole Vector Control.Name=arm_l_pv_ik_ctrl |
| `VariableNode_5_1` | Get Bones<br/>Input parameter |  |
| `VariableNode_11_1` | Get Bones<br/>Input parameter |  |
| `VariableNode_20_1` | Get Bones<br/>Input parameter |  |
| `VariableNode_21_1` | Get FK Controls<br/>Input parameter |  |
| `EdGraphNode_Comment_6_1` | Match FK onto IK |  |
| `VariableNode_25` | Get Negative Side<br/>Input parameter |  |
| `VariableNode_12_1` | Get Enable Stretch<br/>Input parameter |  |
| `IK 2 Bones` | IK 2 Bones<br/>Local Function | IK 2 Bones.IK Control.Type=Control; IK 2 Bones.IK Control.Name=hand_l_ik_ctrl; IK 2 Bones.Pole Vector Control.Type=Control; IK 2 Bones.Pole Vector Control.Name=arm_l_pv_ik_ctrl; IK 2 Bones.Primary Axis.X=-1.000000; IK 2 Bones.Primary Axis.Y=0.000000; IK 2 Bones.Primary Axis.Z=0.000000; IK 2 Bones.Secondary Axis.X=0.000000; IK 2 Bones.Secondary Axis.Y=1.000000; IK 2 Bones.Secondary Axis.Z=0.000000 |
| `EdGraphNode_Comment_3` | IK |  |
| `VariableNode_8` | Set IK Mode<br/>Local Variable<br/>Default True |  |
| `VariableNode_9` | Get IK Mode<br/>Local Variable<br/>Default True |  |
| `Set IKFK Visibility` | Set IKFK Visibility<br/>Local Function | Set IKFK Visibility.IK Controls.0.Type=None; Set IKFK Visibility.IK Controls.0.Name=None; Set IKFK Visibility.IK Controls.1.Type=None; Set IKFK Visibility.IK Controls.1.Name=None; Set IKFK Visibility.Show Controls=true |
| `VariableNode_21_1_1` | Get FK Controls<br/>Input parameter |  |
| `VariableNode_9_1` | Get IK Mode<br/>Local Variable<br/>Default True |  |
| `VariableNode_9_1_1` | Get IK Mode<br/>Local Variable<br/>Default True |  |
| `EdGraphNode_Comment_1` | Visibility |  |
| `Set IKFK Keys` | Set IKFK Keys<br/>Local Function | Set IKFK Keys.IK Controls .0.Type=None; Set IKFK Keys.IK Controls .0.Name=None; Set IKFK Keys.IK Controls .1.Type=None; Set IKFK Keys.IK Controls .1.Name=None; Set IKFK Keys.Switch Control.Type=None; Set IKFK Keys.Switch Control.Name=None |
| `VariableNode` | Get FK Controls<br/>Input parameter |  |
| `VariableNode_10` | Get IKFK Switch Control<br/>Input parameter |  |
| `VariableNode_11` | Get IK Control<br/>Input parameter |  |
| `VariableNode_12` | Get Pole Vector Control<br/>Input parameter |  |
| `VariableNode_13` | Get IK Effector<br/>Input parameter |  |
| `Project to New Parents` | Project to New Parents<br/>Local Function | Project to New Parents.Propagate to Children=True |
| `VariableNode_14` | Get Softness<br/>Input parameter |  |
| `SequenceExecution` | 시퀀스 |  |
| `VariableNode_10_1` | Get IK Control<br/>Input parameter |  |
| `VariableNode_1_2_1_1` | Get Bones<br/>Input parameter |  |
| `ArrayGetAtIndex_2` |  |  |
| `VariableNode_15` | Get Pole Vector Null<br/>Input parameter |  |
| `Compute Pole Vector Parent` | Compute Pole Vector Parent<br/>Local Function | Compute Pole Vector Parent.ExecuteContext=(); Compute Pole Vector Parent.Foot Control.Type=Control; Compute Pole Vector Parent.Foot Control.Name=foot_l_ik_ctrl; Compute Pole Vector Parent.Root Bone.Type=Bone; Compute Pole Vector Parent.Root Bone.Name=thigh_l; Compute Pole Vector Parent.Pole Vector Null.Type=Control; Compute Pole Vector Parent.Pole Vector Null.Name=FootParent; Compute Pole Vector Parent.Initial=False |

</details>

### Forward Neck

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `Entry` | Entry |  |
| `Return` | Return |  |
| `ProjectTransformToNewParent_1` | Project to new Parent | ProjectTransformToNewParent_1.Child.Type=Null; ProjectTransformToNewParent_1.Child.Name=head_fk_space; ProjectTransformToNewParent_1.bChildInitial=True; ProjectTransformToNewParent_1.OldParent.Type=Control; ProjectTransformToNewParent_1.OldParent.Name=neck_02_ctrl; ProjectTransformToNewParent_1.bOldParentInitial=True; ProjectTransformToNewParent_1.NewParent.Type=Control; ProjectTransformToNewParent_1.NewParent.Name=neck_02_ctrl |
| `ProjectTransformToNewParent_1_1` | Project to new Parent | ProjectTransformToNewParent_1_1.Child.Type=Null; ProjectTransformToNewParent_1_1.Child.Name=head_fk_space; ProjectTransformToNewParent_1_1.bChildInitial=True; ProjectTransformToNewParent_1_1.OldParent.Type=Control; ProjectTransformToNewParent_1_1.OldParent.Name=body_ctrl; ProjectTransformToNewParent_1_1.bOldParentInitial=True; ProjectTransformToNewParent_1_1.NewParent.Type=Control; ProjectTransformToNewParent_1_1.NewParent.Name=body_ctrl |
| `VariableNode` | Get Bones<br/>Input parameter |  |
| `VariableNode_1` | Get FK Controls<br/>Input parameter |  |
| `EdGraphNode_Comment_10` | FK |  |
| `ArrayGetAtIndex_1` |  |  |
| `VariableNode_3` | Get Head FK Space<br/>Input parameter |  |
| `If` |  |  |
| `VariableNode_5` | Get Head Orient Reference<br/>Input parameter |  |
| `EdGraphNode_Comment_11` | Head with Orient Space |  |
| `VariableNode_8` | Set Local Space<br/>Local Variable |  |
| `VariableNode_4_1` | Get Head Orient Control<br/>Input parameter |  |
| `GetTransform_1` | Get Transform - Bone | GetTransform_1.Item.Type=Bone; GetTransform_1.Item.Name=None; GetTransform_1.Space=LocalSpace |
| `MathFloatGreater_1` | Greater | MathFloatGreater_1.B=0.500000 |
| `VariableNode_9` | Get Local Space<br/>Local Variable |  |
| `Branch` |  |  |
| `VariableNode_4` | Get IK Control<br/>Input parameter |  |
| `VariableNode_6_1` | Get Bones<br/>Input parameter |  |
| `VariableNode_7` | Get Positions<br/>Local Variable<br/>Default () |  |
| `VariableNode_6` | Get Mid IK Null<br/>Input parameter |  |
| `VariableNode_10` | Get Length<br/>Input parameter |  |
| `VariableNode_11` | Get Enable Stretch<br/>Input parameter |  |
| `Neck IK Neck` | Neck IK Neck<br/>Collapsed Graph | Neck IK Neck.ExecuteContext=(); Neck IK Neck.IK Control.Type=Bone; Neck IK Neck.IK Control.Name=None; Neck IK Neck.Mid Null.Type=Null; Neck IK Neck.Mid Null.Name=head_mid |
| `EdGraphNode_Comment_0` | IK |  |
| `VariableNode_1_1` | Get FK Controls<br/>Input parameter |  |
| `VariableNode_12` | Get Bones<br/>Input parameter |  |
| `EdGraphNode_Comment_1` | Match FK onto IK |  |
| `VariableNode_4_2` | Get IK Control<br/>Input parameter |  |
| `VariableNode_12_2` | Get Bones<br/>Input parameter |  |
| `ArrayGetAtIndex` |  |  |
| `ArrayGetNum` |  |  |
| `MathIntSub` | Subtract | MathIntSub.B=1 |
| `VariableNode_2` | Set IK Mode<br/>Local Variable<br/>Default true |  |
| `VariableNode_13` | Get IK Mode<br/>Local Variable |  |
| `VariableNode_14` | Get FK Controls<br/>Input parameter |  |
| `EdGraphNode_Comment_2` | Match IK onto FK |  |
| `Project to New Parents` | Project to New Parents<br/>Local Function | Project to New Parents.Propagate to Children=True |
| `Project to New Parents_2` | Project to New Parents<br/>Local Function | Project to New Parents_2.ExecuteContext=(); Project to New Parents_2.Propagate to Children=True |
| `Set IKFK Visibility` | Set IKFK Visibility<br/>Local Function | Set IKFK Visibility.IK Controls.0.Type=Control; Set IKFK Visibility.IK Controls.0.Name=head_ik_ctrl; Set IKFK Visibility.Show Controls=true |
| `VariableNode_15` | Get FK Controls<br/>Input parameter |  |
| `VariableNode_13_1_1` | Get IK Mode<br/>Local Variable |  |
| `EdGraphNode_Comment_3` | Visibility |  |
| `GetTransform_1_1` | Get Transform - Bone | GetTransform_1_1.Item.Type=Bone; GetTransform_1_1.Item.Name=None; GetTransform_1_1.Space=LocalSpace |
| `MathFloatGreater_1_1` | Greater | MathFloatGreater_1_1.B=0.500000 |
| `VariableNode_16` | Get IKFK Switch Control<br/>Input parameter |  |
| `Set IKFK Keys` | Set IKFK Keys<br/>Local Function | Set IKFK Keys.IK Controls .0.Type=Control; Set IKFK Keys.IK Controls .0.Name=head_ik_ctrl; Set IKFK Keys.Switch Control.Type=None; Set IKFK Keys.Switch Control.Name=None |
| `VariableNode_13_2` | Get IK Mode<br/>Local Variable |  |
| `VariableNode_17` | Get Previous IK Mode<br/>Input parameter |  |
| `VariableNode_18` | Get FK Controls<br/>Input parameter |  |
| `VariableNode_20` | Get IKFK Switch Control<br/>Input parameter |  |
| `EdGraphNode_Comment_4` | Set Keys |  |
| `ProjectTransformToNewParent` | Project to new Parent | ProjectTransformToNewParent.Child.Type=Bone; ProjectTransformToNewParent.Child.Name=None; ProjectTransformToNewParent.bChildInitial=True; ProjectTransformToNewParent.OldParent.Type=Bone; ProjectTransformToNewParent.OldParent.Name=None; ProjectTransformToNewParent.bOldParentInitial=True; ProjectTransformToNewParent.NewParent.Type=Bone; ProjectTransformToNewParent.NewParent.Name=None |
| `SetTransform_1` | Set Transform - Bone | SetTransform_1.Item.Type=Bone; SetTransform_1.Item.Name=None; SetTransform_1.Space=GlobalSpace; SetTransform_1.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform_1.Value.Translation.X=0.000000; SetTransform_1.Value.Translation.Y=0.000000; SetTransform_1.Value.Translation.Z=0.000000; SetTransform_1.Value.Scale3D.X=1.000000; SetTransform_1.Value.Scale3D.Y=1.000000; SetTransform_1.Value.Scale3D.Z=1.000000; SetTransform_1.Weight=1.000000; SetTransform_1.bPropagateToChildren=True |
| `VariableNode_21` | Get Head IK Space<br/>Input parameter |  |
| `ProjectTransformToNewParent_1_2` | Project to new Parent | ProjectTransformToNewParent_1_2.Child.Type=Null; ProjectTransformToNewParent_1_2.Child.Name=head_fk_space; ProjectTransformToNewParent_1_2.bChildInitial=True; ProjectTransformToNewParent_1_2.OldParent.Type=Control; ProjectTransformToNewParent_1_2.OldParent.Name=neck_02_ctrl; ProjectTransformToNewParent_1_2.bOldParentInitial=True; ProjectTransformToNewParent_1_2.NewParent.Type=Control; ProjectTransformToNewParent_1_2.NewParent.Name=neck_02_ctrl |
| `ProjectTransformToNewParent_1_1_1` | Project to new Parent | ProjectTransformToNewParent_1_1_1.Child.Type=Null; ProjectTransformToNewParent_1_1_1.Child.Name=head_fk_space; ProjectTransformToNewParent_1_1_1.bChildInitial=True; ProjectTransformToNewParent_1_1_1.OldParent.Type=Control; ProjectTransformToNewParent_1_1_1.OldParent.Name=body_ctrl; ProjectTransformToNewParent_1_1_1.bOldParentInitial=True; ProjectTransformToNewParent_1_1_1.NewParent.Type=Control; ProjectTransformToNewParent_1_1_1.NewParent.Name=body_ctrl |
| `If_1` |  |  |
| `VariableNode_5_1` | Get Head Orient Reference<br/>Input parameter |  |
| `VariableNode_9_1` | Get Local Space<br/>Local Variable |  |
| `HierarchyGetParent` | Get Parent | HierarchyGetParent.Child.Type=Bone; HierarchyGetParent.Child.Name=None; HierarchyGetParent.bDefaultParent=True |
| `SetTransform` | Set Transform - Bone | SetTransform.Item.Type=Bone; SetTransform.Item.Name=None; SetTransform.Space=GlobalSpace; SetTransform.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform.Value.Translation.X=0.000000; SetTransform.Value.Translation.Y=0.000000; SetTransform.Value.Translation.Z=0.000000; SetTransform.Value.Scale3D.X=1.000000; SetTransform.Value.Scale3D.Y=1.000000; SetTransform.Value.Scale3D.Z=1.000000; SetTransform.Weight=1.000000; SetTransform.bPropagateToChildren=True |
| `SetTransform_2` | Set Transform - Bone | SetTransform_2.Item.Type=Bone; SetTransform_2.Item.Name=None; SetTransform_2.Space=GlobalSpace; SetTransform_2.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform_2.Value.Translation.X=0.000000; SetTransform_2.Value.Translation.Y=0.000000; SetTransform_2.Value.Translation.Z=0.000000; SetTransform_2.Value.Scale3D.X=1.000000; SetTransform_2.Value.Scale3D.Y=1.000000; SetTransform_2.Value.Scale3D.Z=1.000000; SetTransform_2.Weight=1.000000; SetTransform_2.bPropagateToChildren=True |
| `VariableNode_4_2_1` | Get IK Control<br/>Input parameter |  |
| `VariableNode_4_2_1_1` | Get IK Control<br/>Input parameter |  |
| `SequenceExecution` | 시퀀스 |  |

</details>

### Forward Reverse Foot

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `Entry` | Entry |  |
| `Return` | Return |  |
| `RigUnit_GetTransform` | Get Transform - Control | RigUnit_GetTransform.Item.Type=Control; RigUnit_GetTransform.Item.Name=foot_roll_l_ctrl; RigUnit_GetTransform.Space=LocalSpace |
| `MathQuaternionToRotator` | To Rotator |  |
| `MathFloatClamp_1` | Clamp | MathFloatClamp_1.Minimum=0.000000; MathFloatClamp_1.Maximum=180.000000 |
| `VariableNode_2` | Get Roll Control<br/>Input parameter |  |
| `VariableNode_3` | Get BK Buffers<br/>Input parameter |  |
| `ArrayClone` |  |  |
| `ArrayRemove` |  |  |
| `ArrayReverse` |  |  |
| `ArrayIterator` |  |  |
| `MathFloatClamp_2` | Clamp | MathFloatClamp_2.Minimum=0.000000; MathFloatClamp_2.Maximum=180.000000 |
| `MathFloatMul` | Multiply |  |
| `MathIntToFloat` | To Float |  |
| `RigUnit_GetTransform_1_1_1` | Get Transform - Null Initial | RigUnit_GetTransform_1_1_1.Item.Type=Null; RigUnit_GetTransform_1_1_1.Item.Name=heel_l; RigUnit_GetTransform_1_1_1.Space=LocalSpace; RigUnit_GetTransform_1_1_1.bInitial=true |
| `MathTransformMul_1_1` | Multiply | MathTransformMul_1_1.A.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); MathTransformMul_1_1.A.Translation.X=0.000000; MathTransformMul_1_1.A.Translation.Y=0.000000; MathTransformMul_1_1.A.Translation.Z=0.000000; MathTransformMul_1_1.A.Scale3D.X=1.000000; MathTransformMul_1_1.A.Scale3D.Y=1.000000; MathTransformMul_1_1.A.Scale3D.Z=1.000000; MathTransformMul_1_1.B.Translation.X=0.000000; MathTransformMul_1_1.B.Translation.Y=0.000000; MathTransformMul_1_1.B.Translation.Z=0.000000; MathTransformMul_1_1.B.Scale3D.X=1.000000; MathTransformMul_1_1.B.Scale3D.Y=1.000000; MathTransformMul_1_1.B.Scale3D.Z=1.000000 |
| `RigUnit_SetRotation_1_1` | Set Rotation - Null | RigUnit_SetRotation_1_1.ExecuteContext=(); RigUnit_SetRotation_1_1.Item.Type=Null; RigUnit_SetRotation_1_1.Item.Name=heel_l; RigUnit_SetRotation_1_1.Space=LocalSpace; RigUnit_SetRotation_1_1.Weight=1.000000; RigUnit_SetRotation_1_1.bPropagateToChildren=True |
| `MathFloatSub` | Subtract |  |
| `RigUnit_GetTransform_2` | Get Transform - Control | RigUnit_GetTransform_2.Item.Type=Control; RigUnit_GetTransform_2.Item.Name=foot_roll_l_ctrl; RigUnit_GetTransform_2.Space=LocalSpace |
| `MathQuaternionToRotator_1` | To Rotator |  |
| `VariableNode_2_1` | Get Roll Control<br/>Input parameter |  |
| `EdGraphNode_Comment_0` | Heel Auto Roll |  |
| `VariableNode_1` | Get Roll Clamp<br/>Input parameter |  |
| `MathIntSub` | Subtract | MathIntSub.B=1 |
| `If` |  |  |
| `EdGraphNode_Comment_1` | BK Auto Roll |  |
| `EdGraphNode_Comment_2` | Last Control doesnt have limit  |  |
| `VariableNode_4` | Get BK Controls<br/>Input parameter |  |
| `ArrayIterator_1` |  |  |
| `SetControlVisibility` | Set Control Visibility | SetControlVisibility.ExecuteContext=(); SetControlVisibility.Item.Type=Control; SetControlVisibility.Item.Name=None |
| `VariableNode_5` | Get IKFK Switch Control<br/>Input parameter |  |
| `GetTransform` | Get Transform - Bone | GetTransform.Item.Type=Bone; GetTransform.Item.Name=None; GetTransform.Space=LocalSpace |
| `MathFloatGreater` | Greater | MathFloatGreater.B=0.500000 |
| `EdGraphNode_Comment_3` | Visibility |  |
| `VariableNode_6` | Get Roll Control<br/>Input parameter |  |
| `SetControlVisibility_1` | Set Control Visibility | SetControlVisibility_1.ExecuteContext=(); SetControlVisibility_1.Item.Type=Control; SetControlVisibility_1.Item.Name=None |
| `MathFloatNegate_1` | Negate |  |
| `MathFloatNegate_2` | Negate |  |
| `MathTransformMakeRelative` | Make Relative | MathTransformMakeRelative.Global.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); MathTransformMakeRelative.Global.Translation.X=0.000000; MathTransformMakeRelative.Global.Translation.Y=0.000000; MathTransformMakeRelative.Global.Translation.Z=0.000000; MathTransformMakeRelative.Global.Scale3D.X=1.000000; MathTransformMakeRelative.Global.Scale3D.Y=1.000000; MathTransformMakeRelative.Global.Scale3D.Z=1.000000; MathTransformMakeRelative.Parent.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); MathTransformMakeRelative.Parent.Translation.X=0.000000; MathTransformMakeRelative.Parent.Translation.Y=0.000000; MathTransformMakeRelative.Parent.Translation.Z=0.000000; MathTransformMakeRelative.Parent.Scale3D.X=1.000000; MathTransformMakeRelative.Parent.Scale3D.Y=1.000000; MathTransformMakeRelative.Parent.Scale3D.Z=1.000000 |
| `MathTransformMul_1` | Multiply | MathTransformMul_1.A.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); MathTransformMul_1.A.Translation.X=0.000000; MathTransformMul_1.A.Translation.Y=0.000000; MathTransformMul_1.A.Translation.Z=0.000000; MathTransformMul_1.A.Scale3D.X=1.000000; MathTransformMul_1.A.Scale3D.Y=1.000000; MathTransformMul_1.A.Scale3D.Z=1.000000; MathTransformMul_1.B.Translation.X=0.000000; MathTransformMul_1.B.Translation.Y=0.000000; MathTransformMul_1.B.Translation.Z=0.000000; MathTransformMul_1.B.Scale3D.X=1.000000; MathTransformMul_1.B.Scale3D.Y=1.000000; MathTransformMul_1.B.Scale3D.Z=1.000000 |
| `MathTransformMakeAbsolute` | Make Absolute | MathTransformMakeAbsolute.Local.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); MathTransformMakeAbsolute.Local.Translation.X=0.000000; MathTransformMakeAbsolute.Local.Translation.Y=0.000000; MathTransformMakeAbsolute.Local.Translation.Z=0.000000; MathTransformMakeAbsolute.Local.Scale3D.X=1.000000; MathTransformMakeAbsolute.Local.Scale3D.Y=1.000000; MathTransformMakeAbsolute.Local.Scale3D.Z=1.000000; MathTransformMakeAbsolute.Parent.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); MathTransformMakeAbsolute.Parent.Translation.X=0.000000; MathTransformMakeAbsolute.Parent.Translation.Y=0.000000; MathTransformMakeAbsolute.Parent.Translation.Z=0.000000; MathTransformMakeAbsolute.Parent.Scale3D.X=1.000000; MathTransformMakeAbsolute.Parent.Scale3D.Y=1.000000; MathTransformMakeAbsolute.Parent.Scale3D.Z=1.000000 |
| `RigUnit_SetTransform` | Set Transform - Null | RigUnit_SetTransform.Item.Type=Null; RigUnit_SetTransform.Item.Name=pivot_l; RigUnit_SetTransform.Space=LocalSpace; RigUnit_SetTransform.Value.Rotation=(X=-0.063632,Y=-0.704247,Z=-0.041375,W=0.705886); RigUnit_SetTransform.Value.Translation.X=12.560760; RigUnit_SetTransform.Value.Translation.Y=-3.342936; RigUnit_SetTransform.Value.Translation.Z=0.632515; RigUnit_SetTransform.Value.Scale3D.X=1.000000; RigUnit_SetTransform.Value.Scale3D.Y=1.000000; RigUnit_SetTransform.Value.Scale3D.Z=1.000000; RigUnit_SetTransform.Weight=1.000000; RigUnit_SetTransform.bPropagateToChildren=True |
| `MathTransformMul_2` | Multiply | MathTransformMul_2.A.Translation.X=0.000000; MathTransformMul_2.A.Translation.Y=0.000000; MathTransformMul_2.A.Translation.Z=0.000000; MathTransformMul_2.A.Scale3D.X=1.000000; MathTransformMul_2.A.Scale3D.Y=1.000000; MathTransformMul_2.A.Scale3D.Z=1.000000; MathTransformMul_2.B.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); MathTransformMul_2.B.Translation.X=0.000000; MathTransformMul_2.B.Translation.Y=0.000000; MathTransformMul_2.B.Translation.Z=0.000000; MathTransformMul_2.B.Scale3D.X=1.000000; MathTransformMul_2.B.Scale3D.Y=1.000000; MathTransformMul_2.B.Scale3D.Z=1.000000 |
| `VariableNode_4_1` | Get BK Controls<br/>Input parameter |  |
| `ArrayGetAtIndex_1` |  |  |
| `EdGraphNode_Comment_4` | Heel Free Pivot |  |
| `EdGraphNode_Comment_5` | Heel |  |
| `RigUnit_GetTransform_3_1` | Get Transform - Control | RigUnit_GetTransform_3_1.Item.Type=Control; RigUnit_GetTransform_3_1.Item.Name=heel_l_ctrl; RigUnit_GetTransform_3_1.Space=LocalSpace; RigUnit_GetTransform_3_1.bInitial=false |
| `VariableNode_7` | Get BK Buffers<br/>Input parameter |  |
| `ArrayGetAtIndex_2` |  |  |
| `RigUnit_GetTransform_1_1_2` | Get Transform - Null Initial | RigUnit_GetTransform_1_1_2.Item.Type=Null; RigUnit_GetTransform_1_1_2.Item.Name=pivot_l; RigUnit_GetTransform_1_1_2.Space=LocalSpace; RigUnit_GetTransform_1_1_2.bInitial=true |
| `VariableNode_8` | Get BK Buffers<br/>Input parameter |  |
| `ArrayGetAtIndex_3` |  |  |
| `MathTransformMul` | Multiply | MathTransformMul.A.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); MathTransformMul.A.Translation.X=0.000000; MathTransformMul.A.Translation.Y=0.000000; MathTransformMul.A.Translation.Z=0.000000; MathTransformMul.A.Scale3D.X=1.000000; MathTransformMul.A.Scale3D.Y=1.000000; MathTransformMul.A.Scale3D.Z=1.000000; MathTransformMul.B.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); MathTransformMul.B.Translation.X=0.000000; MathTransformMul.B.Translation.Y=0.000000; MathTransformMul.B.Translation.Z=0.000000; MathTransformMul.B.Scale3D.X=1.000000; MathTransformMul.B.Scale3D.Y=1.000000; MathTransformMul.B.Scale3D.Z=1.000000 |
| `MathTransformMakeRelative_1` | Make Relative | MathTransformMakeRelative_1.Global.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); MathTransformMakeRelative_1.Global.Translation.X=0.000000; MathTransformMakeRelative_1.Global.Translation.Y=0.000000; MathTransformMakeRelative_1.Global.Translation.Z=0.000000; MathTransformMakeRelative_1.Global.Scale3D.X=1.000000; MathTransformMakeRelative_1.Global.Scale3D.Y=1.000000; MathTransformMakeRelative_1.Global.Scale3D.Z=1.000000; MathTransformMakeRelative_1.Parent.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); MathTransformMakeRelative_1.Parent.Translation.X=0.000000; MathTransformMakeRelative_1.Parent.Translation.Y=0.000000; MathTransformMakeRelative_1.Parent.Translation.Z=0.000000; MathTransformMakeRelative_1.Parent.Scale3D.X=1.000000; MathTransformMakeRelative_1.Parent.Scale3D.Y=1.000000; MathTransformMakeRelative_1.Parent.Scale3D.Z=1.000000 |
| `MathTransformMul_1_2` | Multiply | MathTransformMul_1_2.A.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); MathTransformMul_1_2.A.Translation.X=0.000000; MathTransformMul_1_2.A.Translation.Y=0.000000; MathTransformMul_1_2.A.Translation.Z=0.000000; MathTransformMul_1_2.A.Scale3D.X=1.000000; MathTransformMul_1_2.A.Scale3D.Y=1.000000; MathTransformMul_1_2.A.Scale3D.Z=1.000000; MathTransformMul_1_2.B.Translation.X=0.000000; MathTransformMul_1_2.B.Translation.Y=0.000000; MathTransformMul_1_2.B.Translation.Z=0.000000; MathTransformMul_1_2.B.Scale3D.X=1.000000; MathTransformMul_1_2.B.Scale3D.Y=1.000000; MathTransformMul_1_2.B.Scale3D.Z=1.000000 |
| `MathTransformMakeAbsolute_1` | Make Absolute | MathTransformMakeAbsolute_1.Local.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); MathTransformMakeAbsolute_1.Local.Translation.X=0.000000; MathTransformMakeAbsolute_1.Local.Translation.Y=0.000000; MathTransformMakeAbsolute_1.Local.Translation.Z=0.000000; MathTransformMakeAbsolute_1.Local.Scale3D.X=1.000000; MathTransformMakeAbsolute_1.Local.Scale3D.Y=1.000000; MathTransformMakeAbsolute_1.Local.Scale3D.Z=1.000000; MathTransformMakeAbsolute_1.Parent.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); MathTransformMakeAbsolute_1.Parent.Translation.X=0.000000; MathTransformMakeAbsolute_1.Parent.Translation.Y=0.000000; MathTransformMakeAbsolute_1.Parent.Translation.Z=0.000000; MathTransformMakeAbsolute_1.Parent.Scale3D.X=1.000000; MathTransformMakeAbsolute_1.Parent.Scale3D.Y=1.000000; MathTransformMakeAbsolute_1.Parent.Scale3D.Z=1.000000 |
| `VariableNode_4_1_1` | Get BK Controls<br/>Input parameter |  |
| `ArrayGetAtIndex_1_1` |  |  |
| `EdGraphNode_Comment_4_1` | Tip Free Pivot |  |
| `RigUnit_GetTransform_3_1_1` | Get Transform - Control | RigUnit_GetTransform_3_1_1.Item.Type=Control; RigUnit_GetTransform_3_1_1.Item.Name=tip_l_ctrl; RigUnit_GetTransform_3_1_1.Space=LocalSpace; RigUnit_GetTransform_3_1_1.bInitial=false |
| `RigUnit_GetTransform_1_1_2_1` | Get Transform - Null Initial | RigUnit_GetTransform_1_1_2_1.Item.Type=Null; RigUnit_GetTransform_1_1_2_1.Item.Name=pivot_l; RigUnit_GetTransform_1_1_2_1.Space=LocalSpace; RigUnit_GetTransform_1_1_2_1.bInitial=true |
| `VariableNode_8_1` | Get BK Buffers<br/>Input parameter |  |
| `ArrayGetAtIndex_3_1` |  |  |
| `MathTransformMul_3` | Multiply | MathTransformMul_3.A.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); MathTransformMul_3.A.Translation.X=0.000000; MathTransformMul_3.A.Translation.Y=0.000000; MathTransformMul_3.A.Translation.Z=0.000000; MathTransformMul_3.A.Scale3D.X=1.000000; MathTransformMul_3.A.Scale3D.Y=1.000000; MathTransformMul_3.A.Scale3D.Z=1.000000; MathTransformMul_3.B.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); MathTransformMul_3.B.Translation.X=0.000000; MathTransformMul_3.B.Translation.Y=0.000000; MathTransformMul_3.B.Translation.Z=0.000000; MathTransformMul_3.B.Scale3D.X=1.000000; MathTransformMul_3.B.Scale3D.Y=1.000000; MathTransformMul_3.B.Scale3D.Z=1.000000 |
| `RigUnit_SetTransform_1` | Set Transform - Null | RigUnit_SetTransform_1.Item.Type=Null; RigUnit_SetTransform_1.Item.Name=pivot_l; RigUnit_SetTransform_1.Space=LocalSpace; RigUnit_SetTransform_1.Value.Rotation=(X=-0.063632,Y=-0.704247,Z=-0.041375,W=0.705886); RigUnit_SetTransform_1.Value.Translation.X=12.560760; RigUnit_SetTransform_1.Value.Translation.Y=-3.342936; RigUnit_SetTransform_1.Value.Translation.Z=0.632515; RigUnit_SetTransform_1.Value.Scale3D.X=1.000000; RigUnit_SetTransform_1.Value.Scale3D.Y=1.000000; RigUnit_SetTransform_1.Value.Scale3D.Z=1.000000; RigUnit_SetTransform_1.Weight=1.000000; RigUnit_SetTransform_1.bPropagateToChildren=True |
| `GetTransform_1` | Get Transform - Bone | GetTransform_1.Item.Type=Bone; GetTransform_1.Item.Name=None; GetTransform_1.Space=LocalSpace |
| `MathTransformMul_4` | Multiply | MathTransformMul_4.A.Translation.X=0.000000; MathTransformMul_4.A.Translation.Y=0.000000; MathTransformMul_4.A.Translation.Z=0.000000; MathTransformMul_4.A.Scale3D.X=1.000000; MathTransformMul_4.A.Scale3D.Y=1.000000; MathTransformMul_4.A.Scale3D.Z=1.000000; MathTransformMul_4.B.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); MathTransformMul_4.B.Translation.X=0.000000; MathTransformMul_4.B.Translation.Y=0.000000; MathTransformMul_4.B.Translation.Z=0.000000; MathTransformMul_4.B.Scale3D.X=1.000000; MathTransformMul_4.B.Scale3D.Y=1.000000; MathTransformMul_4.B.Scale3D.Z=1.000000 |
| `MathFloatMul_1` | Multiply |  |
| `VariableNode_9` | Get Roll Blend<br/>Input parameter |  |
| `ProjectTransformToNewParent` | Project to new Parent | ProjectTransformToNewParent.Child.Type=Null; ProjectTransformToNewParent.Child.Name=foot_roll_l_bfr; ProjectTransformToNewParent.bChildInitial=True; ProjectTransformToNewParent.OldParent.Type=Control; ProjectTransformToNewParent.OldParent.Name=foot_l_ik_ctrl; ProjectTransformToNewParent.bOldParentInitial=True; ProjectTransformToNewParent.NewParent.Type=Control; ProjectTransformToNewParent.NewParent.Name=foot_l_ik_ctrl |
| `RigUnit_SetRotation` | Set Rotation - Null | RigUnit_SetRotation.Item.Type=Null; RigUnit_SetRotation.Item.Name=foot_roll_l_bfr; RigUnit_SetRotation.Space=GlobalSpace; RigUnit_SetRotation.Weight=1.000000; RigUnit_SetRotation.bPropagateToChildren=True |
| `VariableNode` | Get BK Buffers<br/>Input parameter |  |
| `ArrayGetAtIndex` |  |  |
| `HierarchyGetParent` | Get Parent | HierarchyGetParent.Child.Type=Bone; HierarchyGetParent.Child.Name=None; HierarchyGetParent.bDefaultParent=True |
| `VariableNode_10` | Get Roll Buffer<br/>Input parameter |  |
| `RerouteNode` | Int32 |  |
| `VariableNode_11` | Set IK Mode<br/>Local Variable |  |
| `VariableNode_12` | Get IK Mode<br/>Local Variable |  |
| `SequenceExecution` | 시퀀스 |  |
| `MathQuaternionFromRotator` | From Rotator | MathQuaternionFromRotator.Value.Pitch=0.000000; MathQuaternionFromRotator.Value.Roll=0.000000 |
| `MathIntEquals` | Equals |  |
| `MathQuaternionFromRotator_1_1` | From Rotator | MathQuaternionFromRotator_1_1.Value.Pitch=0.000000; MathQuaternionFromRotator_1_1.Value.Roll=0.000000 |
| `SequenceExecution_1` | 시퀀스 |  |

</details>

### Forward Spine

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `Entry` | Entry |  |
| `Return` | Return |  |
| `ProjectTransformToNewParent_1_2_1_4_1` | Project to new Parent | ProjectTransformToNewParent_1_2_1_4_1.Child.Type=Bone; ProjectTransformToNewParent_1_2_1_4_1.Child.Name=spine_01; ProjectTransformToNewParent_1_2_1_4_1.bChildInitial=True; ProjectTransformToNewParent_1_2_1_4_1.OldParent.Type=Control; ProjectTransformToNewParent_1_2_1_4_1.OldParent.Name=hips_ctrl; ProjectTransformToNewParent_1_2_1_4_1.bOldParentInitial=True; ProjectTransformToNewParent_1_2_1_4_1.NewParent.Type=Control; ProjectTransformToNewParent_1_2_1_4_1.NewParent.Name=hips_ctrl |
| `VariableNode` | Get Pelvis Bone<br/>Input parameter |  |
| `ArrayGetAtIndex_1_1` |  |  |
| `VariableNode_1` | Get FK Controls<br/>Input parameter |  |
| `SetTransform` | Set Transform - Bone | SetTransform.Item.Type=Bone; SetTransform.Item.Name=None; SetTransform.Space=GlobalSpace; SetTransform.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform.Value.Translation.X=0.000000; SetTransform.Value.Translation.Y=0.000000; SetTransform.Value.Translation.Z=0.000000; SetTransform.Value.Scale3D.X=1.000000; SetTransform.Value.Scale3D.Y=1.000000; SetTransform.Value.Scale3D.Z=1.000000; SetTransform.Weight=1.000000; SetTransform.bPropagateToChildren=True |
| `EdGraphNode_Comment_7` | Pelvis |  |
| `EdGraphNode_Comment_8` | FK Chain |  |
| `VariableNode_2_1` | Get Bones<br/>Input parameter |  |
| `Branch` |  |  |
| `VariableNode_4` | Get IK Controls<br/>Input parameter |  |
| `VariableNode_5` | Get Spine Offsets<br/>Variable |  |
| `EdGraphNode_Comment_0` | IK |  |
| `VariableNode_3_1` | Get FK Controls<br/>Input parameter |  |
| `VariableNode_2_2` | Get Bones<br/>Input parameter |  |
| `EdGraphNode_Comment_4` | Match FK onto IK |  |
| `VariableNode_7` | Get FK Controls<br/>Input parameter |  |
| `VariableNode_8` | Get IK Controls<br/>Input parameter |  |
| `ArrayRemove` |  |  |
| `ArrayClone` |  |  |
| `ArrayClone_1` |  |  |
| `ArrayRemove_1` |  |  |
| `VariableNode_2_2_1` | Get Bones<br/>Input parameter |  |
| `EdGraphNode_Comment_4_1` | Match IK onto FK |  |
| `VariableNode_9` | Get IK Controls<br/>Input parameter |  |
| `VariableNode_5_1` | Get Spine Offsets<br/>Variable |  |
| `EdGraphNode_Comment_1` | Visibility |  |
| `VariableNode_3` | Get FK Controls<br/>Input parameter |  |
| `VariableNode_2` | Get Bones<br/>Input parameter |  |
| `EdGraphNode_Comment_2` | Chest Space |  |
| `Spine IK Solve` | Spine IK Solve<br/>Collapsed Graph | Spine IK Solve.Completed=() |
| `EdGraphNode_Comment_9` | Set Keys |  |
| `VariableNode_10` | Get Previous IK Mode<br/>Input parameter |  |
| `VariableNode_11` | Get FK Controls<br/>Input parameter |  |
| `VariableNode_12` | Get IK Controls<br/>Input parameter |  |
| `VariableNode_13` | Get IKFK Switch Control<br/>Input parameter |  |
| `VariableNode_14` | Get Default Length<br/>Input parameter |  |
| `VariableNode_15` | Set IK Mode<br/>Local Variable |  |
| `VariableNode_6_3` | Get IKFK Switch Control<br/>Input parameter |  |
| `GetTransform_3` | Get Transform - Bone | GetTransform_3.Item.Type=Bone; GetTransform_3.Item.Name=None; GetTransform_3.Space=LocalSpace |
| `MathFloatGreater_3` | Greater | MathFloatGreater_3.B=0.500000 |
| `VariableNode_16` | Get IK Mode<br/>Local Variable |  |
| `VariableNode_17` | Get IK Mode<br/>Local Variable |  |
| `VariableNode_6_1_2_1` | Get Chest Space<br/>Input parameter |  |
| `ProjectTransformToNewParent_1_2_1_4_1_1` | Project to new Parent | ProjectTransformToNewParent_1_2_1_4_1_1.Child.Type=Bone; ProjectTransformToNewParent_1_2_1_4_1_1.Child.Name=spine_01; ProjectTransformToNewParent_1_2_1_4_1_1.bChildInitial=True; ProjectTransformToNewParent_1_2_1_4_1_1.OldParent.Type=Control; ProjectTransformToNewParent_1_2_1_4_1_1.OldParent.Name=hips_ctrl; ProjectTransformToNewParent_1_2_1_4_1_1.bOldParentInitial=True; ProjectTransformToNewParent_1_2_1_4_1_1.NewParent.Type=Control; ProjectTransformToNewParent_1_2_1_4_1_1.NewParent.Name=hips_ctrl |
| `VariableNode_2_3` | Get Bones<br/>Input parameter |  |
| `ArrayGetNum` |  |  |
| `MathIntSub` | Subtract | MathIntSub.B=1 |
| `ArrayGetAtIndex` |  |  |
| `SetTransform_1` | Set Transform - Bone | SetTransform_1.Item.Type=Bone; SetTransform_1.Item.Name=None; SetTransform_1.Space=GlobalSpace; SetTransform_1.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform_1.Value.Translation.X=0.000000; SetTransform_1.Value.Translation.Y=0.000000; SetTransform_1.Value.Translation.Z=0.000000; SetTransform_1.Value.Scale3D.X=1.000000; SetTransform_1.Value.Scale3D.Y=1.000000; SetTransform_1.Value.Scale3D.Z=1.000000; SetTransform_1.Weight=1.000000; SetTransform_1.bPropagateToChildren=True |
| `VariableNode_18` | Get Enable Stretch <br/>Input parameter |  |
| `Set IKFK Visibility` | Set IKFK Visibility<br/>Local Function | Set IKFK Visibility.ExecuteContext=(); Set IKFK Visibility.FK Controls=(); Set IKFK Visibility.IK Controls=(); Set IKFK Visibility.Show Controls=true |
| `Set IKFK Keys_1` | Set IKFK Keys<br/>Local Function | Set IKFK Keys_1.Switch Control.Type=None; Set IKFK Keys_1.Switch Control.Name=None |
| `FK Point At_1` | FK Point At<br/>Local Function | FK Point At_1.ExecuteContext=(); FK Point At_1.Point At Axis.X=1.000000; FK Point At_1.Point At Axis.Y=0.000000; FK Point At_1.Point At Axis.Z=0.000000; FK Point At_1.Up Vector Axis.X=0.000000; FK Point At_1.Up Vector Axis.Y=0.000000; FK Point At_1.Up Vector Axis.Z=-1.000000; FK Point At_1.Up Vector Target.X=1.000000; FK Point At_1.Up Vector Target.Y=0.000000; FK Point At_1.Up Vector Target.Z=0.000000 |
| `VariableNode_16_1` | Get IK Mode<br/>Local Variable |  |
| `Project to New Parent` | Project to New Parents<br/>Local Function | Project to New Parent.ExecuteContext=(); Project to New Parent.Children=(); Project to New Parent.Parents=(); Project to New Parent.Propagate to Children=True |
| `ArrayClone_2` |  |  |
| `ArrayRemove_2` |  |  |
| `ArrayClone_2_1` |  |  |
| `ArrayRemove_2_1` |  |  |
| `Spine Match IK onto FK` | Spine Match IK onto FK<br/>Local Function |  |
| `SequenceExecution` | 시퀀스 |  |
| `SequenceExecution_1` | 시퀀스 |  |

</details>

### Get Negative Transform

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `Entry` | Entry |  |
| `Return` | Return | Return.Output.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); Return.Output.Translation.X=0.000000; Return.Output.Translation.Y=0.000000; Return.Output.Translation.Z=0.000000; Return.Output.Scale3D.X=0.000000; Return.Output.Scale3D.Y=0.000000; Return.Output.Scale3D.Z=0.000000 |
| `MathFloatAdd` | Add | MathFloatAdd.B=180.000000 |
| `MathQuaternionToRotator` | To Rotator |  |
| `MathFloatNegate` | Negate |  |
| `RerouteNode` |  |  |
| `MathQuaternionFromRotator` | From Rotator | MathQuaternionFromRotator.Value=() |

</details>

### IK 2 Bones

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `Entry` | Entry |  |
| `Return` | Return |  |
| `ProjectTransformToNewParent_11` | Project to new Parent | ProjectTransformToNewParent_11.Child.Type=Bone; ProjectTransformToNewParent_11.Child.Name=hand_l; ProjectTransformToNewParent_11.bChildInitial=True; ProjectTransformToNewParent_11.OldParent.Type=Control; ProjectTransformToNewParent_11.OldParent.Name=hand_l_ik_ctrl; ProjectTransformToNewParent_11.bOldParentInitial=True; ProjectTransformToNewParent_11.NewParent.Type=Control; ProjectTransformToNewParent_11.NewParent.Name=hand_l_ik_ctrl |
| `TwoBoneIKSimplePerItem_2` | Basic IK | TwoBoneIKSimplePerItem_2.ItemA.Type=Bone; TwoBoneIKSimplePerItem_2.ItemA.Name=upperarm_l; TwoBoneIKSimplePerItem_2.ItemB.Type=Bone; TwoBoneIKSimplePerItem_2.ItemB.Name=lowerarm_l; TwoBoneIKSimplePerItem_2.EffectorItem.Type=Bone; TwoBoneIKSimplePerItem_2.EffectorItem.Name=hand_l; TwoBoneIKSimplePerItem_2.Effector.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); TwoBoneIKSimplePerItem_2.Effector.Translation.X=0.000000; TwoBoneIKSimplePerItem_2.Effector.Translation.Y=0.000000; TwoBoneIKSimplePerItem_2.Effector.Translation.Z=0.000000; TwoBoneIKSimplePerItem_2.Effector.Scale3D.X=1.000000; TwoBoneIKSimplePerItem_2.Effector.Scale3D.Y=1.000000; TwoBoneIKSimplePerItem_2.Effector.Scale3D.Z=1.000000; TwoBoneIKSimplePerItem_2.PrimaryAxis.X=1.000000; TwoBoneIKSimplePerItem_2.PrimaryAxis.Y=0.000000; TwoBoneIKSimplePerItem_2.PrimaryAxis.Z=0.000000; TwoBoneIKSimplePerItem_2.SecondaryAxis.X=0.000000; TwoBoneIKSimplePerItem_2.SecondaryAxis.Y=-1.000000; TwoBoneIKSimplePerItem_2.SecondaryAxis.Z=0.000000; TwoBoneIKSimplePerItem_2.SecondaryAxisWeight=1.000000; TwoBoneIKSimplePerItem_2.PoleVector.X=0.000000; TwoBoneIKSimplePerItem_2.PoleVector.Y=0.000000; TwoBoneIKSimplePerItem_2.PoleVector.Z=0.000000; TwoBoneIKSimplePerItem_2.PoleVectorKind=Location; TwoBoneIKSimplePerItem_2.PoleVectorSpace.Type=Bone; TwoBoneIKSimplePerItem_2.PoleVectorSpace.Name=None; TwoBoneIKSimplePerItem_2.StretchStartRatio=1.000000; TwoBoneIKSimplePerItem_2.StretchMaximumRatio=2.000000; TwoBoneIKSimplePerItem_2.Weight=1.000000; TwoBoneIKSimplePerItem_2.bPropagateToChildren=True |
| `DebugLineItemSpace_1_1` | Draw Line (OutDated) | DebugLineItemSpace_1_1.A.X=0.000000; DebugLineItemSpace_1_1.A.Y=0.000000; DebugLineItemSpace_1_1.A.Z=0.000000; DebugLineItemSpace_1_1.B.X=0.000000; DebugLineItemSpace_1_1.B.Y=0.000000; DebugLineItemSpace_1_1.B.Z=0.000000; DebugLineItemSpace_1_1.Color.R=0.200000; DebugLineItemSpace_1_1.Color.G=0.200000; DebugLineItemSpace_1_1.Color.B=0.200000; DebugLineItemSpace_1_1.Color.A=1.000000; DebugLineItemSpace_1_1.Thickness=0.500000; DebugLineItemSpace_1_1.Space.Type=Bone; DebugLineItemSpace_1_1.Space.Name=None; DebugLineItemSpace_1_1.WorldOffset.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); DebugLineItemSpace_1_1.WorldOffset.Translation.X=0.000000; DebugLineItemSpace_1_1.WorldOffset.Translation.Y=0.000000; DebugLineItemSpace_1_1.WorldOffset.Translation.Z=0.000000; DebugLineItemSpace_1_1.WorldOffset.Scale3D.X=1.000000; DebugLineItemSpace_1_1.WorldOffset.Scale3D.Y=1.000000; DebugLineItemSpace_1_1.WorldOffset.Scale3D.Z=1.000000; DebugLineItemSpace_1_1.bEnabled=True |
| `GetTransform_12_2_1` | Get Transform - Bone | GetTransform_12_2_1.Item.Type=Bone; GetTransform_12_2_1.Item.Name=lowerarm_l; GetTransform_12_2_1.Space=GlobalSpace |
| `GetTransform_12_1_1_1` | Get Transform - Control | GetTransform_12_1_1_1.Item.Type=Control; GetTransform_12_1_1_1.Item.Name=arm_l_pv_ik_ctrl; GetTransform_12_1_1_1.Space=GlobalSpace |
| `ArrayGetAtIndex_3` |  |  |
| `ArrayGetAtIndex_4` |  |  |
| `ArrayGetAtIndex_4_1` |  |  |
| `ArrayGetAtIndex_4_2` |  |  |
| `ArrayGetAtIndex_5` |  |  |
| `If_2` |  |  |
| `If_1` |  |  |
| `VariableNode` | Get Bones<br/>Input parameter |  |
| `VariableNode_1` | Get IK Control<br/>Input parameter |  |
| `VariableNode_2` | Get Bones<br/>Input parameter |  |
| `VariableNode_3` | Get Bones<br/>Input parameter |  |
| `VariableNode_4` | Get Pole Vector Control<br/>Input parameter |  |
| `VariableNode_5` | Get Pole Vector Control<br/>Input parameter |  |
| `VariableNode_6` | Get Enable Stretch<br/>Input parameter |  |
| `VariableNode_7` | Get Negative Side<br/>Input parameter |  |
| `EdGraphNode_Comment_2` | Effector |  |
| `EdGraphNode_Comment_3` | Draw Pole Vector Line |  |
| `EdGraphNode_Comment_4` | IK Solve |  |
| `VariableNode_8` | Get Primary Axis<br/>Input parameter |  |
| `VariableNode_9` | Get Secondary Axis<br/>Input parameter |  |
| `MathVectorNegate` | Negate | MathVectorNegate.Value.X=0.000000; MathVectorNegate.Value.Y=0.000000; MathVectorNegate.Value.Z=0.000000 |
| `MathVectorNegate_1` | Negate | MathVectorNegate_1.Value.X=0.000000; MathVectorNegate_1.Value.Y=0.000000; MathVectorNegate_1.Value.Z=0.000000 |
| `VariableNode_10` | Get Bones<br/>Input parameter |  |
| `VariableNode_1_1` | Get IK Control<br/>Input parameter |  |
| `VariableNode_11` | Get Softness<br/>Input parameter |  |
| `Softness` | Softness<br/>Collapsed Graph | Softness.Effector.Type=Control; Softness.Effector.Name=IK |
| `SequenceExecution` | 시퀀스 |  |
| `RigUnit_GetBoolAnimationChannel_2` | Get Bool Channel | RigUnit_GetBoolAnimationChannel_2.Channel=Keep Aligned; RigUnit_GetBoolAnimationChannel_2.bInitial=False |
| `IsInteracting` | Get Interaction |  |
| `ArrayFind` |  |  |
| `Branch` |  |  |
| `MathBoolAnd` | And |  |
| `VariableNode_1_1_1` | Get IK Control<br/>Input parameter |  |
| `DISPATCH_RigDispatch_SetMetadata` | Set Transform Metadata | DISPATCH_RigDispatch_SetMetadata.ExecuteContext=(); DISPATCH_RigDispatch_SetMetadata.Item.Type=Control; DISPATCH_RigDispatch_SetMetadata.Item.Name=hand_l_ik_ctrl; DISPATCH_RigDispatch_SetMetadata.Name=Alignment; DISPATCH_RigDispatch_SetMetadata.NameSpace=Self; DISPATCH_RigDispatch_SetMetadata.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); DISPATCH_RigDispatch_SetMetadata.Value.Translation.X=0.000000; DISPATCH_RigDispatch_SetMetadata.Value.Translation.Y=0.000000; DISPATCH_RigDispatch_SetMetadata.Value.Translation.Z=0.000000; DISPATCH_RigDispatch_SetMetadata.Value.Scale3D.X=1.000000; DISPATCH_RigDispatch_SetMetadata.Value.Scale3D.Y=1.000000; DISPATCH_RigDispatch_SetMetadata.Value.Scale3D.Z=1.000000 |
| `GetTransform_1` | Get Transform - Bone | GetTransform_1.Item.Type=Bone; GetTransform_1.Item.Name=None; GetTransform_1.Space=GlobalSpace |
| `RigUnit_GetTransform` | Get Transform - Bone | RigUnit_GetTransform.Item.Type=Bone; RigUnit_GetTransform.Item.Name=lowerarm_l; RigUnit_GetTransform.Space=GlobalSpace |
| `Make Relative` | Make Relative | Make Relative.Global.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); Make Relative.Global.Translation.X=0.000000; Make Relative.Global.Translation.Y=0.000000; Make Relative.Global.Translation.Z=0.000000; Make Relative.Global.Scale3D.X=1.000000; Make Relative.Global.Scale3D.Y=1.000000; Make Relative.Global.Scale3D.Z=1.000000; Make Relative.Parent.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); Make Relative.Parent.Translation.X=0.000000; Make Relative.Parent.Translation.Y=0.000000; Make Relative.Parent.Translation.Z=0.000000; Make Relative.Parent.Scale3D.X=1.000000; Make Relative.Parent.Scale3D.Y=1.000000; Make Relative.Parent.Scale3D.Z=1.000000 |
| `VariableNode_1_1_1_1` | Get IK Control<br/>Input parameter |  |
| `VariableNode_10_1` | Get Bones<br/>Input parameter |  |
| `ArrayGetAtIndex` |  |  |
| `Set Transform` | Set Rotation - Bone | Set Transform.ExecuteContext=(); Set Transform.Item.Type=Bone; Set Transform.Item.Name=None; Set Transform.Space=GlobalSpace; Set Transform.bInitial=False; Set Transform.Weight=1.000000; Set Transform.bPropagateToChildren=True |
| `RigUnit_GetTransform_1` | Get Transform - Bone | RigUnit_GetTransform_1.Item.Type=Bone; RigUnit_GetTransform_1.Item.Name=lowerarm_l; RigUnit_GetTransform_1.Space=GlobalSpace |
| `Make Absolute` | Make Absolute | Make Absolute.Local.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); Make Absolute.Local.Translation.X=0.000000; Make Absolute.Local.Translation.Y=0.000000; Make Absolute.Local.Translation.Z=0.000000; Make Absolute.Local.Scale3D.X=1.000000; Make Absolute.Local.Scale3D.Y=1.000000; Make Absolute.Local.Scale3D.Z=1.000000; Make Absolute.Parent.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); Make Absolute.Parent.Translation.X=0.000000; Make Absolute.Parent.Translation.Y=0.000000; Make Absolute.Parent.Translation.Z=0.000000; Make Absolute.Parent.Scale3D.X=1.000000; Make Absolute.Parent.Scale3D.Y=1.000000; Make Absolute.Parent.Scale3D.Z=1.000000 |
| `DISPATCH_RigDispatch_GetMetadata` | Get Transform Metadata | DISPATCH_RigDispatch_GetMetadata.Item.Type=Control; DISPATCH_RigDispatch_GetMetadata.Item.Name=hand_l_ik_ctrl; DISPATCH_RigDispatch_GetMetadata.Name=Alignment; DISPATCH_RigDispatch_GetMetadata.NameSpace=Self; DISPATCH_RigDispatch_GetMetadata.Default.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); DISPATCH_RigDispatch_GetMetadata.Default.Translation.X=0.000000; DISPATCH_RigDispatch_GetMetadata.Default.Translation.Y=0.000000; DISPATCH_RigDispatch_GetMetadata.Default.Translation.Z=0.000000; DISPATCH_RigDispatch_GetMetadata.Default.Scale3D.X=1.000000; DISPATCH_RigDispatch_GetMetadata.Default.Scale3D.Y=1.000000; DISPATCH_RigDispatch_GetMetadata.Default.Scale3D.Z=1.000000 |
| `VariableNode_10_1_1` | Get Bones<br/>Input parameter |  |
| `ArrayGetAtIndex_1` |  |  |

</details>

### Interpolate Interacting Controls

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `Entry` | Entry |  |
| `Return` | Return |  |
| `ArrayGetNum` |  |  |
| `Greater` | Greater | Greater.A=0; Greater.B=2 |
| `Branch_1` |  |  |
| `IsInteracting_1` | Get Interaction |  |
| `MathBoolAnd` | And |  |
| `EdGraphNode_Comment_0` | Is Interacting |  |
| `VariableNode` | Get Controls<br/>Input parameter |  |
| `IsInteracting_2` | Get Interaction |  |
| `ArrayGetAtIndex_1` |  |  |
| `ArrayFind_1` |  |  |
| `Branch_3` |  |  |
| `DISPATCH_RigDispatch_CoreEquals_1` | Equals | DISPATCH_RigDispatch_CoreEquals_1.A=0; DISPATCH_RigDispatch_CoreEquals_1.B=0 |
| `ArrayGetNum_1` |  |  |
| `Subtract_1` | Subtract | Subtract_1.A=0; Subtract_1.B=1 |
| `DISPATCH_RigDispatch_CoreEquals_2` | Equals | DISPATCH_RigDispatch_CoreEquals_2.A=0 |
| `MathBoolOr_1` | Or |  |
| `VariableNode_1` | Set Interacting Item<br/>Local Variable<br/>Default () | VariableNode_1.ExecuteContext=(); VariableNode_1.Value.Type=None; VariableNode_1.Value.Name=None |
| `VariableNode_2` | Get Interacting Item<br/>Local Variable<br/>Default () |  |
| `VariableNode_3` | Get Controls<br/>Input parameter |  |
| `VariableNode_3_1` | Get Controls<br/>Input parameter |  |
| `EdGraphNode_Comment_1` | Interacting with First or last Control |  |
| `ArrayReverse` |  |  |
| `Branch_4` |  |  |
| `DISPATCH_RigDispatch_SetMetadata_1` | Set Transform Metadata | DISPATCH_RigDispatch_SetMetadata_1.ExecuteContext=(); DISPATCH_RigDispatch_SetMetadata_1.Item.Type=Control; DISPATCH_RigDispatch_SetMetadata_1.Item.Name=pinky_metacarpal_l_ctrl; DISPATCH_RigDispatch_SetMetadata_1.Name=Previous Transform; DISPATCH_RigDispatch_SetMetadata_1.NameSpace=Self; DISPATCH_RigDispatch_SetMetadata_1.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); DISPATCH_RigDispatch_SetMetadata_1.Value.Translation.X=0.000000; DISPATCH_RigDispatch_SetMetadata_1.Value.Translation.Y=0.000000; DISPATCH_RigDispatch_SetMetadata_1.Value.Translation.Z=0.000000; DISPATCH_RigDispatch_SetMetadata_1.Value.Scale3D.X=1.000000; DISPATCH_RigDispatch_SetMetadata_1.Value.Scale3D.Y=1.000000; DISPATCH_RigDispatch_SetMetadata_1.Value.Scale3D.Z=1.000000 |
| `RigUnit_GetTransform_2` | Get Transform - Control | RigUnit_GetTransform_2.Item.Type=Control; RigUnit_GetTransform_2.Item.Name=pinky_metacarpal_l_ctrl; RigUnit_GetTransform_2.Space=LocalSpace |
| `SequenceAggregate_3` | 시퀀스 | SequenceAggregate_3.ExecuteContext=() |
| `VariableNode_3_2` | Set Loop Controls<br/>Local Variable<br/>Default () |  |
| `VariableNode_4` | Get Loop Controls<br/>Local Variable<br/>Default () |  |
| `EdGraphNode_Comment_2` | Save Pose |  |
| `VariableNode_6` | Get Controls<br/>Input parameter |  |
| `EdGraphNode_Comment_3` | If Interacting with last Control we reverse the array |  |
| `RigUnit_GetTransform_1_1` | Get Transform - Control | RigUnit_GetTransform_1_1.Item.Type=Control; RigUnit_GetTransform_1_1.Item.Name=pinky_metacarpal_l_ctrl; RigUnit_GetTransform_1_1.Space=LocalSpace |
| `DISPATCH_RigDispatch_GetMetadata_1` | Get Transform Metadata | DISPATCH_RigDispatch_GetMetadata_1.Item.Type=Control; DISPATCH_RigDispatch_GetMetadata_1.Item.Name=pinky_metacarpal_l_ctrl; DISPATCH_RigDispatch_GetMetadata_1.Name=Previous Transform; DISPATCH_RigDispatch_GetMetadata_1.NameSpace=Self; DISPATCH_RigDispatch_GetMetadata_1.Default.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); DISPATCH_RigDispatch_GetMetadata_1.Default.Translation.X=0.000000; DISPATCH_RigDispatch_GetMetadata_1.Default.Translation.Y=0.000000; DISPATCH_RigDispatch_GetMetadata_1.Default.Translation.Z=0.000000; DISPATCH_RigDispatch_GetMetadata_1.Default.Scale3D.X=1.000000; DISPATCH_RigDispatch_GetMetadata_1.Default.Scale3D.Y=1.000000; DISPATCH_RigDispatch_GetMetadata_1.Default.Scale3D.Z=1.000000 |
| `Make Relative_1` | Make Relative | Make Relative_1.Global.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); Make Relative_1.Global.Translation.X=0.000000; Make Relative_1.Global.Translation.Y=0.000000; Make Relative_1.Global.Translation.Z=0.000000; Make Relative_1.Global.Scale3D.X=1.000000; Make Relative_1.Global.Scale3D.Y=1.000000; Make Relative_1.Global.Scale3D.Z=1.000000; Make Relative_1.Parent.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); Make Relative_1.Parent.Translation.X=0.000000; Make Relative_1.Parent.Translation.Y=0.000000; Make Relative_1.Parent.Translation.Z=0.000000; Make Relative_1.Parent.Scale3D.X=1.000000; Make Relative_1.Parent.Scale3D.Y=1.000000; Make Relative_1.Parent.Scale3D.Z=1.000000 |
| `Interpolate_1` | Interpolate | Interpolate_1.A.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); Interpolate_1.A.Translation.X=0.000000; Interpolate_1.A.Translation.Y=0.000000; Interpolate_1.A.Translation.Z=0.000000; Interpolate_1.A.Scale3D.X=1.000000; Interpolate_1.A.Scale3D.Y=1.000000; Interpolate_1.A.Scale3D.Z=1.000000; Interpolate_1.B.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); Interpolate_1.B.Translation.X=0.000000; Interpolate_1.B.Translation.Y=0.000000; Interpolate_1.B.Translation.Z=0.000000; Interpolate_1.B.Scale3D.X=1.000000; Interpolate_1.B.Scale3D.Y=1.000000; Interpolate_1.B.Scale3D.Z=1.000000 |
| `ArrayIterator_1_1` |  |  |
| `DISPATCH_RigDispatch_CoreEquals_3` | Equals | DISPATCH_RigDispatch_CoreEquals_3.A=0; DISPATCH_RigDispatch_CoreEquals_3.B=0 |
| `Branch_2_1` |  |  |
| `Subtract_2` | Subtract | Subtract_2.A=1.000000; Subtract_2.B=0.000000 |
| `OffsetTransformForItem_1` | Offset Transform | OffsetTransformForItem_1.ExecuteContext=(); OffsetTransformForItem_1.Item.Type=Bone; OffsetTransformForItem_1.Item.Name=None; OffsetTransformForItem_1.OffsetTransform.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); OffsetTransformForItem_1.OffsetTransform.Translation.X=0.000000; OffsetTransformForItem_1.OffsetTransform.Translation.Y=0.000000; OffsetTransformForItem_1.OffsetTransform.Translation.Z=0.000000; OffsetTransformForItem_1.OffsetTransform.Scale3D.X=1.000000; OffsetTransformForItem_1.OffsetTransform.Scale3D.Y=1.000000; OffsetTransformForItem_1.OffsetTransform.Scale3D.Z=1.000000; OffsetTransformForItem_1.Weight=1.000000; OffsetTransformForItem_1.bPropagateToChildren=True |
| `VariableNode_4_1` | Get Loop Controls<br/>Local Variable<br/>Default () |  |
| `VariableNode_5_1` | Get Interacting Item<br/>Local Variable<br/>Default () |  |
| `EdGraphNode_Comment_4` | Interpolate in between controls |  |
| `SequenceAggregate` | 시퀀스 |  |
| `VariableNode_7` | Get Controls<br/>Input parameter |  |
| `ArrayIterator` |  |  |
| `VariableNode_5` | Get Controls<br/>Input parameter |  |
| `ArrayIterator_1` |  |  |
| `SendEvent` | Send Event | SendEvent.ExecuteContext=(); SendEvent.Event=RequestAutoKey; SendEvent.Item.Type=None; SendEvent.Item.Name=None; SendEvent.bEnable=True; SendEvent.bOnlyDuringInteraction=True |
| `EdGraphNode_Comment_5` | Auto Key |  |
| `Branch` |  |  |

</details>

### Match 2 Bones IK onto FK

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `Entry` | Entry |  |
| `Return` | Return |  |
| `ProjectTransformToNewParent_13` | Project to new Parent | ProjectTransformToNewParent_13.Child.Type=Control; ProjectTransformToNewParent_13.Child.Name=arm_l_pv_ik_ctrl; ProjectTransformToNewParent_13.bChildInitial=True; ProjectTransformToNewParent_13.OldParent.Type=Bone; ProjectTransformToNewParent_13.OldParent.Name=upperarm_l; ProjectTransformToNewParent_13.bOldParentInitial=True; ProjectTransformToNewParent_13.NewParent.Type=Bone; ProjectTransformToNewParent_13.NewParent.Name=upperarm_l |
| `ProjectTransformToNewParent_13_1` | Project to new Parent | ProjectTransformToNewParent_13_1.Child.Type=Control; ProjectTransformToNewParent_13_1.Child.Name=hand_l_ik_ctrl; ProjectTransformToNewParent_13_1.bChildInitial=True; ProjectTransformToNewParent_13_1.OldParent.Type=Bone; ProjectTransformToNewParent_13_1.OldParent.Name=hand_l; ProjectTransformToNewParent_13_1.bOldParentInitial=True; ProjectTransformToNewParent_13_1.NewParent.Type=Bone; ProjectTransformToNewParent_13_1.NewParent.Name=hand_l |
| `SetTransform_4` | Set Transform - Control | SetTransform_4.Item.Type=Control; SetTransform_4.Item.Name=arm_l_pv_ik_ctrl; SetTransform_4.Space=GlobalSpace; SetTransform_4.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform_4.Value.Translation.X=0.000000; SetTransform_4.Value.Translation.Y=0.000000; SetTransform_4.Value.Translation.Z=0.000000; SetTransform_4.Value.Scale3D.X=1.000000; SetTransform_4.Value.Scale3D.Y=1.000000; SetTransform_4.Value.Scale3D.Z=1.000000; SetTransform_4.Weight=1.000000; SetTransform_4.bPropagateToChildren=True |
| `SetTransform_4_1` | Set Transform - Control | SetTransform_4_1.Item.Type=Control; SetTransform_4_1.Item.Name=hand_l_ik_ctrl; SetTransform_4_1.Space=GlobalSpace; SetTransform_4_1.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform_4_1.Value.Translation.X=0.000000; SetTransform_4_1.Value.Translation.Y=0.000000; SetTransform_4_1.Value.Translation.Z=0.000000; SetTransform_4_1.Value.Scale3D.X=1.000000; SetTransform_4_1.Value.Scale3D.Y=1.000000; SetTransform_4_1.Value.Scale3D.Z=1.000000; SetTransform_4_1.Weight=1.000000; SetTransform_4_1.bPropagateToChildren=True |
| `ArrayGetAtIndex_1` |  |  |
| `ArrayGetAtIndex_2` |  |  |
| `VariableNode` | Get Bones<br/>Input parameter |  |
| `VariableNode_1` | Get Bones<br/>Input parameter |  |
| `VariableNode_2` | Get Pole Vector Control<br/>Input parameter |  |
| `VariableNode_3` | Get IK Control<br/>Input parameter |  |
| `EdGraphNode_Comment_0` | Up Vector |  |
| `EdGraphNode_Comment_1` | IK Handle |  |

</details>

### Project to New Parents

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `Entry` | Entry |  |
| `Return` | Return | Return.ExecuteContext=() |
| `ArrayGetAtIndex_6` |  |  |
| `ArrayIterator_3` |  |  |
| `ProjectTransformToNewParent_12_1` | Project to new Parent | ProjectTransformToNewParent_12_1.Child.Type=Control; ProjectTransformToNewParent_12_1.Child.Name=None; ProjectTransformToNewParent_12_1.bChildInitial=True; ProjectTransformToNewParent_12_1.OldParent.Type=Bone; ProjectTransformToNewParent_12_1.OldParent.Name=None; ProjectTransformToNewParent_12_1.bOldParentInitial=True; ProjectTransformToNewParent_12_1.NewParent.Type=Bone; ProjectTransformToNewParent_12_1.NewParent.Name=None |
| `SetTransform_10_1_1_1_2_1` | Set Transform - Control | SetTransform_10_1_1_1_2_1.ExecuteContext=(); SetTransform_10_1_1_1_2_1.Item.Type=Control; SetTransform_10_1_1_1_2_1.Item.Name=None; SetTransform_10_1_1_1_2_1.Space=GlobalSpace; SetTransform_10_1_1_1_2_1.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform_10_1_1_1_2_1.Value.Translation.X=0.000000; SetTransform_10_1_1_1_2_1.Value.Translation.Y=0.000000; SetTransform_10_1_1_1_2_1.Value.Translation.Z=0.000000; SetTransform_10_1_1_1_2_1.Value.Scale3D.X=1.000000; SetTransform_10_1_1_1_2_1.Value.Scale3D.Y=1.000000; SetTransform_10_1_1_1_2_1.Value.Scale3D.Z=1.000000; SetTransform_10_1_1_1_2_1.Weight=1.000000 |

</details>

### Rig Graph

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `RigUnit_BeginExecution_1` | Forwards Solve |  |
| `GetTransform` | Get Transform - Control | GetTransform.Item.Type=Control; GetTransform.Item.Name=root_ctrl; GetTransform.Space=GlobalSpace |
| `GetTransform_5` | Get Transform - Bone Initial | GetTransform_5.Item.Type=Bone; GetTransform_5.Item.Name=spine_01; GetTransform_5.Space=GlobalSpace; GetTransform_5.bInitial=true |
| `GetTransform_5_1` | Get Transform - Bone Initial | GetTransform_5_1.Item.Type=Bone; GetTransform_5_1.Item.Name=spine_02; GetTransform_5_1.Space=GlobalSpace; GetTransform_5_1.bInitial=true |
| `MathTransformLerp` | Interpolate | MathTransformLerp.A.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); MathTransformLerp.A.Translation.X=0.000000; MathTransformLerp.A.Translation.Y=0.000000; MathTransformLerp.A.Translation.Z=0.000000; MathTransformLerp.A.Scale3D.X=1.000000; MathTransformLerp.A.Scale3D.Y=1.000000; MathTransformLerp.A.Scale3D.Z=1.000000; MathTransformLerp.B.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); MathTransformLerp.B.Translation.X=0.000000; MathTransformLerp.B.Translation.Y=0.000000; MathTransformLerp.B.Translation.Z=0.000000; MathTransformLerp.B.Scale3D.X=1.000000; MathTransformLerp.B.Scale3D.Y=1.000000; MathTransformLerp.B.Scale3D.Z=1.000000; MathTransformLerp.T=0.500000 |
| `EdGraphNode_Comment_9` | Setup Sine / Neck |  |
| `SetTransform_8` | Set Transform - Bone | SetTransform_8.Item.Type=Bone; SetTransform_8.Item.Name=root; SetTransform_8.Space=GlobalSpace; SetTransform_8.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform_8.Value.Translation.X=0.000000; SetTransform_8.Value.Translation.Y=0.000000; SetTransform_8.Value.Translation.Z=0.000000; SetTransform_8.Value.Scale3D.X=1.000000; SetTransform_8.Value.Scale3D.Y=1.000000; SetTransform_8.Value.Scale3D.Z=1.000000; SetTransform_8.Weight=1.000000; SetTransform_8.bPropagateToChildren=True |
| `ProjectTransformToNewParent_24` | Project to new Parent | ProjectTransformToNewParent_24.Child.Type=Null; ProjectTransformToNewParent_24.Child.Name=None; ProjectTransformToNewParent_24.bChildInitial=True; ProjectTransformToNewParent_24.OldParent.Type=Control; ProjectTransformToNewParent_24.OldParent.Name=hips_ctrl; ProjectTransformToNewParent_24.bOldParentInitial=True; ProjectTransformToNewParent_24.NewParent.Type=Control; ProjectTransformToNewParent_24.NewParent.Name=hips_ctrl |
| `SetTransform_16` | Set Transform - Bone | SetTransform_16.ExecuteContext=(); SetTransform_16.Item.Type=Bone; SetTransform_16.Item.Name=None; SetTransform_16.Space=GlobalSpace; SetTransform_16.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform_16.Value.Translation.X=0.000000; SetTransform_16.Value.Translation.Y=0.000000; SetTransform_16.Value.Translation.Z=0.000000; SetTransform_16.Value.Scale3D.X=1.000000; SetTransform_16.Value.Scale3D.Y=1.000000; SetTransform_16.Value.Scale3D.Z=1.000000; SetTransform_16.Weight=1.000000; SetTransform_16.bPropagateToChildren=True |
| `GetTransform_9` | Get Transform - Bone | GetTransform_9.Item.Type=Bone; GetTransform_9.Item.Name=root; GetTransform_9.Space=GlobalSpace; GetTransform_9.bInitial=false |
| `SetTransform_14` | Set Transform - Control | SetTransform_14.Item.Type=Control; SetTransform_14.Item.Name=root_ctrl; SetTransform_14.Space=GlobalSpace; SetTransform_14.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform_14.Value.Translation.X=0.000000; SetTransform_14.Value.Translation.Y=0.000000; SetTransform_14.Value.Translation.Z=0.000000; SetTransform_14.Value.Scale3D.X=1.000000; SetTransform_14.Value.Scale3D.Y=1.000000; SetTransform_14.Value.Scale3D.Z=1.000000; SetTransform_14.Weight=1.000000; SetTransform_14.bPropagateToChildren=True |
| `SetTransform_15` | Set Transform - Control | SetTransform_15.Item.Type=Control; SetTransform_15.Item.Name=body_ctrl; SetTransform_15.Space=GlobalSpace; SetTransform_15.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform_15.Value.Translation.X=0.000000; SetTransform_15.Value.Translation.Y=0.000000; SetTransform_15.Value.Translation.Z=0.000000; SetTransform_15.Value.Scale3D.X=1.000000; SetTransform_15.Value.Scale3D.Y=1.000000; SetTransform_15.Value.Scale3D.Z=1.000000; SetTransform_15.Weight=1.000000; SetTransform_15.bPropagateToChildren=True |
| `ProjectTransformToNewParent_21_2` | Project to new Parent | ProjectTransformToNewParent_21_2.Child.Type=Control; ProjectTransformToNewParent_21_2.Child.Name=body_ctrl; ProjectTransformToNewParent_21_2.bChildInitial=True; ProjectTransformToNewParent_21_2.OldParent.Type=Bone; ProjectTransformToNewParent_21_2.OldParent.Name=spine_01; ProjectTransformToNewParent_21_2.bOldParentInitial=True; ProjectTransformToNewParent_21_2.NewParent.Type=Bone; ProjectTransformToNewParent_21_2.NewParent.Name=spine_01 |
| `InverseExecution` | Backwards Solve |  |
| `SetTransform_19` | Set Transform - Control | SetTransform_19.Item.Type=Control; SetTransform_19.Item.Name=body_offset_ctrl; SetTransform_19.Space=GlobalSpace; SetTransform_19.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform_19.Value.Translation.X=0.000000; SetTransform_19.Value.Translation.Y=0.000000; SetTransform_19.Value.Translation.Z=0.000000; SetTransform_19.Value.Scale3D.X=1.000000; SetTransform_19.Value.Scale3D.Y=1.000000; SetTransform_19.Value.Scale3D.Z=1.000000; SetTransform_19.Weight=1.000000; SetTransform_19.bPropagateToChildren=True |
| `EdGraphNode_Comment_24` | Backward Root / Spine / Neck |  |
| `ProjectTransformToNewParent_24_4` | Project to new Parent | ProjectTransformToNewParent_24_4.Child.Type=Null; ProjectTransformToNewParent_24_4.Child.Name=None; ProjectTransformToNewParent_24_4.bChildInitial=True; ProjectTransformToNewParent_24_4.OldParent.Type=Control; ProjectTransformToNewParent_24_4.OldParent.Name=hips_ctrl; ProjectTransformToNewParent_24_4.bOldParentInitial=True; ProjectTransformToNewParent_24_4.NewParent.Type=Control; ProjectTransformToNewParent_24_4.NewParent.Name=hips_ctrl |
| `SetTransform_16_1` | Set Transform - Bone | SetTransform_16_1.ExecuteContext=(); SetTransform_16_1.Item.Type=Bone; SetTransform_16_1.Item.Name=None; SetTransform_16_1.Space=GlobalSpace; SetTransform_16_1.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform_16_1.Value.Translation.X=0.000000; SetTransform_16_1.Value.Translation.Y=0.000000; SetTransform_16_1.Value.Translation.Z=0.000000; SetTransform_16_1.Value.Scale3D.X=1.000000; SetTransform_16_1.Value.Scale3D.Y=1.000000; SetTransform_16_1.Value.Scale3D.Z=1.000000; SetTransform_16_1.Weight=1.000000; SetTransform_16_1.bPropagateToChildren=True |
| `RigUnit_SetTransform` | Set Transform - Control Initial | RigUnit_SetTransform.Item.Type=Control; RigUnit_SetTransform.Item.Name=body_ctrl; RigUnit_SetTransform.Space=GlobalSpace; RigUnit_SetTransform.bInitial=true; RigUnit_SetTransform.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); RigUnit_SetTransform.Value.Translation.X=0.000008; RigUnit_SetTransform.Value.Translation.Y=2.543382; RigUnit_SetTransform.Value.Translation.Z=91.183441; RigUnit_SetTransform.Value.Scale3D.X=1.000000; RigUnit_SetTransform.Value.Scale3D.Y=1.000000; RigUnit_SetTransform.Value.Scale3D.Z=1.000000; RigUnit_SetTransform.Weight=1.000000; RigUnit_SetTransform.bPropagateToChildren=True |
| `Setup Arm` | Setup Arm<br/>Local Function | Setup Arm.Bones=(); Setup Arm.FK Space.Type=Null; Setup Arm.FK Space.Name=upperarm_l_fk_ctrl_space; Setup Arm.FK Controls=(); Setup Arm.IK Control.Type=Control; Setup Arm.IK Control.Name=hand_l_ik_ctrl; Setup Arm.Pole Vector Control.Type=Control; Setup Arm.Pole Vector Control.Name=arm_l_pv_ik_ctrl; Setup Arm.Fingers Space.Type=Null; Setup Arm.Fingers Space.Name=fingers_l_space; Setup Arm.IK Parents=() |
| `EdGraphNode_Comment_30` | Setup Arms / Fingers |  |
| `Setup Arm_1` | Setup Arm<br/>Local Function | Setup Arm_1.Bones=(); Setup Arm_1.FK Space.Type=Null; Setup Arm_1.FK Space.Name=upperarm_r_fk_ctrl_space; Setup Arm_1.FK Controls=(); Setup Arm_1.IK Control.Type=Control; Setup Arm_1.IK Control.Name=hand_r_ik_ctrl; Setup Arm_1.Pole Vector Control.Type=Control; Setup Arm_1.Pole Vector Control.Name=arm_r_pv_ik_ctrl; Setup Arm_1.Fingers Space.Type=Null; Setup Arm_1.Fingers Space.Name=fingers_r_space; Setup Arm_1.IK Parents=(); Setup Arm_1.Negative Side=true |
| `Setup Clavicle` | Setup Clavicle<br/>Local Function | Setup Clavicle.Bone.Type=Bone; Setup Clavicle.Bone.Name=clavicle_l; Setup Clavicle.Control.Type=Control; Setup Clavicle.Control.Name=clavicle_l_ctrl |
| `Setup Clavicle_1` | Setup Clavicle<br/>Local Function | Setup Clavicle_1.Bone.Type=Bone; Setup Clavicle_1.Bone.Name=clavicle_r; Setup Clavicle_1.Control.Type=Control; Setup Clavicle_1.Control.Name=clavicle_r_ctrl; Setup Clavicle_1.Negative Side=true |
| `Setup Fingers` | Setup Fingers<br/>Local Function | Setup Fingers.Fingers Bones=(); Setup Fingers.Negative Side=False |
| `Setup Fingers_1` | Setup Fingers<br/>Local Function | Setup Fingers_1.Fingers Bones=(); Setup Fingers_1.Negative Side=true |
| `Setup Leg` | Setup Leg<br/>Local Function | Setup Leg.Bones=(); Setup Leg.FK Space.Type=Null; Setup Leg.FK Space.Name=thigh_l_fk_ctrl_space; Setup Leg.FK Controls=(); Setup Leg.IK Control.Type=Control; Setup Leg.IK Control.Name=foot_l_ik_ctrl; Setup Leg.Pole Vector Control.Type=Control; Setup Leg.Pole Vector Control.Name=leg_l_pv_ik_ctrl; Setup Leg.IK Parents=(); Setup Leg.Negative Side=False; Setup Leg.Pole Vector Null.Type=Null; Setup Leg.Pole Vector Null.Name=leg_l_pv_footFollow |
| `Setup Leg_1` | Setup Leg<br/>Local Function | Setup Leg_1.Bones=(); Setup Leg_1.FK Space.Type=Null; Setup Leg_1.FK Space.Name=thigh_r_fk_ctrl_space; Setup Leg_1.FK Controls=(); Setup Leg_1.IK Control.Type=Control; Setup Leg_1.IK Control.Name=foot_r_ik_ctrl; Setup Leg_1.Pole Vector Control.Type=Control; Setup Leg_1.Pole Vector Control.Name=leg_r_pv_ik_ctrl; Setup Leg_1.IK Parents=(); Setup Leg_1.Negative Side=true; Setup Leg_1.Pole Vector Null.Type=Null; Setup Leg_1.Pole Vector Null.Name=leg_r_pv_footFollow |
| `Setup Foot` | Setup Foot<br/>Local Function | Setup Foot.Bone.Type=Bone; Setup Foot.Bone.Name=ball_l; Setup Foot.FK Control.Type=Control; Setup Foot.FK Control.Name=ball_l_fk_ctrl; Setup Foot.IK Control.Type=Control; Setup Foot.IK Control.Name=ball_l_ik_ctrl; Setup Foot.Toes Space.Type=Null; Setup Foot.Toes Space.Name=toes_l_space |
| `Setup Foot_1` | Setup Foot<br/>Local Function | Setup Foot_1.Bone.Type=Bone; Setup Foot_1.Bone.Name=ball_r; Setup Foot_1.FK Control.Type=Control; Setup Foot_1.FK Control.Name=ball_r_fk_ctrl; Setup Foot_1.IK Control.Type=Control; Setup Foot_1.IK Control.Name=ball_r_ik_ctrl; Setup Foot_1.Toes Space.Type=Null; Setup Foot_1.Toes Space.Name=toes_r_space; Setup Foot_1.Negative Side=true |
| `Setup Fingers_2` | Setup Fingers<br/>Local Function | Setup Fingers_2.Fingers Bones=(); Setup Fingers_2.Negative Side=False |
| `Setup Fingers_2_1` | Setup Fingers<br/>Local Function | Setup Fingers_2_1.Fingers Bones=(); Setup Fingers_2_1.Negative Side=true |
| `EdGraphNode_Comment_10` | Setup Legs Toes |  |
| `Forward Arm` | Forward Arm<br/>Local Function | Forward Arm.FK Controls=(); Forward Arm.IK Control.Type=Control; Forward Arm.IK Control.Name=hand_l_ik_ctrl; Forward Arm.IK Effector.Type=Control; Forward Arm.IK Effector.Name=hand_l_ik_ctrl; Forward Arm.Pole Vector Control.Type=Control; Forward Arm.Pole Vector Control.Name=arm_l_pv_ik_ctrl; Forward Arm.IKFK Switch Control.Type=Control; Forward Arm.IKFK Switch Control.Name=arm_l_fk_ik_switch; Forward Arm.FK Orient Control.Type=Control; Forward Arm.FK Orient Control.Name=arm_l_fk_local; Forward Arm.Arm Space.Type=Null; Forward Arm.Arm Space.Name=upperarm_l_fk_ctrl_space; Forward Arm.FK Orient References=(); Forward Arm.Finger Space.Type=Null; Forward Arm.Finger Space.Name=fingers_l_space; Forward Arm.Bones=(); Forward Arm.Negative Side=False; Forward Arm.IK Orient Null.Type=Null; Forward Arm.IK Orient Null.Name=hand_l_ikorient; Forward Arm.IK Orient Control.Type=Control; Forward Arm.IK Orient Control.Name=hand_l_ikorient_ctrl |
| `Forward Arm_1` | Forward Arm<br/>Local Function | Forward Arm_1.FK Controls=(); Forward Arm_1.IK Control.Type=Control; Forward Arm_1.IK Control.Name=hand_r_ik_ctrl; Forward Arm_1.IK Effector.Type=Control; Forward Arm_1.IK Effector.Name=hand_r_ik_ctrl; Forward Arm_1.Pole Vector Control.Type=Control; Forward Arm_1.Pole Vector Control.Name=arm_r_pv_ik_ctrl; Forward Arm_1.IKFK Switch Control.Type=Control; Forward Arm_1.IKFK Switch Control.Name=arm_r_fk_ik_switch; Forward Arm_1.FK Orient Control.Type=Control; Forward Arm_1.FK Orient Control.Name=arm_r_fk_local; Forward Arm_1.Arm Space.Type=Null; Forward Arm_1.Arm Space.Name=upperarm_r_fk_ctrl_space; Forward Arm_1.FK Orient References=(); Forward Arm_1.Finger Space.Type=Null; Forward Arm_1.Finger Space.Name=fingers_r_space; Forward Arm_1.Bones=(); Forward Arm_1.Negative Side=true; Forward Arm_1.IK Orient Null.Type=Null; Forward Arm_1.IK Orient Null.Name=hand_r_ikorient; Forward Arm_1.IK Orient Control.Type=Control; Forward Arm_1.IK Orient Control.Name=hand_r_ikorient_ctrl |
| `EdGraphNode_Comment_3` | Forward Arms / Fingers |  |
| `EdGraphNode_Comment_5` | Forward Spine / Neck |  |
| `ArrayIterator` |  |  |
| `Forward Clavicle` | Forward Clavicle<br/>Local Function | Forward Clavicle.Control.Type=Control; Forward Clavicle.Control.Name=clavicle_r_ctrl; Forward Clavicle.Bone.Type=Bone; Forward Clavicle.Bone.Name=clavicle_r |
| `Forward Clavicle_1` | Forward Clavicle<br/>Local Function | Forward Clavicle_1.Control.Type=Control; Forward Clavicle_1.Control.Name=clavicle_l_ctrl; Forward Clavicle_1.Bone.Type=Bone; Forward Clavicle_1.Bone.Name=clavicle_l |
| `Forward Leg` | Forward Leg<br/>Local Function | Forward Leg.FK Controls=(); Forward Leg.IK Control.Type=Control; Forward Leg.IK Control.Name=foot_r_ik_ctrl; Forward Leg.IK Effector.Type=Null; Forward Leg.IK Effector.Name=foot_ik_r; Forward Leg.Pole Vector Control.Type=Control; Forward Leg.Pole Vector Control.Name=leg_r_pv_ik_ctrl; Forward Leg.IKFK Switch Control.Type=Control; Forward Leg.IKFK Switch Control.Name=leg_r_fk_ik_switch; Forward Leg.Bones=(); Forward Leg.Negative Side=true; Forward Leg.Pole Vector Null.Type=Null; Forward Leg.Pole Vector Null.Name=leg_r_pv_footFollow |
| `Forward Leg_1` | Forward Leg<br/>Local Function | Forward Leg_1.FK Controls=(); Forward Leg_1.IK Control.Type=Control; Forward Leg_1.IK Control.Name=foot_l_ik_ctrl; Forward Leg_1.IK Effector.Type=Null; Forward Leg_1.IK Effector.Name=foot_ik_l; Forward Leg_1.Pole Vector Control.Type=Control; Forward Leg_1.Pole Vector Control.Name=leg_l_pv_ik_ctrl; Forward Leg_1.IKFK Switch Control.Type=Control; Forward Leg_1.IKFK Switch Control.Name=leg_l_fk_ik_switch; Forward Leg_1.Bones=(); Forward Leg_1.Negative Side=False; Forward Leg_1.Pole Vector Null.Type=Null; Forward Leg_1.Pole Vector Null.Name=leg_l_pv_footFollow |
| `EdGraphNode_Comment_0` | Forward Legs / Feet |  |
| `Forward Foot` | Forward Foot<br/>Local Function | Forward Foot.IK Control.Type=Control; Forward Foot.IK Control.Name=ball_l_ik_ctrl; Forward Foot.FK Control.Type=Control; Forward Foot.FK Control.Name=ball_l_fk_ctrl; Forward Foot.IKFK Switch Control.Type=Control; Forward Foot.IKFK Switch Control.Name=leg_l_fk_ik_switch; Forward Foot.Toes Space.Type=Null; Forward Foot.Toes Space.Name=toes_l_space; Forward Foot.Bone.Type=Bone; Forward Foot.Bone.Name=ball_l |
| `Forward Foot_1` | Forward Foot<br/>Local Function | Forward Foot_1.IK Control.Type=Control; Forward Foot_1.IK Control.Name=ball_r_ik_ctrl; Forward Foot_1.FK Control.Type=Control; Forward Foot_1.FK Control.Name=ball_r_fk_ctrl; Forward Foot_1.IKFK Switch Control.Type=Control; Forward Foot_1.IKFK Switch Control.Name=leg_r_fk_ik_switch; Forward Foot_1.Toes Space.Type=Null; Forward Foot_1.Toes Space.Name=toes_r_space; Forward Foot_1.Bone.Type=Bone; Forward Foot_1.Bone.Name=ball_r |
| `EdGraphNode_Comment_7` | Controls Visibility |  |
| `Set IKFK Visibility` | Set IKFK Visibility<br/>Local Function | Set IKFK Visibility.ExecuteContext=(); Set IKFK Visibility.FK Controls=(); Set IKFK Visibility.IK Controls=() |
| `Set IKFK Visibility_1` | Set IKFK Visibility<br/>Local Function | Set IKFK Visibility_1.FK Controls=(); Set IKFK Visibility_1.IK Controls=() |
| `Set IKFK Visibility_1_1` | Set IKFK Visibility<br/>Local Function | Set IKFK Visibility_1_1.FK Controls=(); Set IKFK Visibility_1_1.IK Controls=() |
| `Set IKFK Visibility_1_2` | Set IKFK Visibility<br/>Local Function | Set IKFK Visibility_1_2.FK Controls=(); Set IKFK Visibility_1_2.IK Controls=() |
| `ArrayIterator_1_1` |  |  |
| `SetControlVisibility_1` | Set Control Visibility | SetControlVisibility_1.ExecuteContext=(); SetControlVisibility_1.Item.Type=Control; SetControlVisibility_1.Item.Name=None |
| `Backward Spine` | Backward Spine<br/>Local Function | Backward Spine.Bones=(); Backward Spine.FK Controls=(); Backward Spine.IK Controls=(); Backward Spine.Pelvis Bone.Type=Bone; Backward Spine.Pelvis Bone.Name=pelvis |
| `Backward Neck_1` | Backward Neck<br/>Local Function | Backward Neck_1.Bones=(); Backward Neck_1.FK Controls=(); Backward Neck_1.IK Control.Type=Control; Backward Neck_1.IK Control.Name=head_ik_ctrl; Backward Neck_1.Head FK Space.Type=Null; Backward Neck_1.Head FK Space.Name=head_fk_space; Backward Neck_1.Head IK Space.Type=Null; Backward Neck_1.Head IK Space.Name=head_ik_space; Backward Neck_1.Head Orient Reference.Type=Control; Backward Neck_1.Head Orient Reference.Name=body_ctrl |
| `Backward Clavicle` | Backward Clavicle<br/>Local Function | Backward Clavicle.Control.Type=Control; Backward Clavicle.Control.Name=clavicle_r_ctrl; Backward Clavicle.Bone.Type=Bone; Backward Clavicle.Bone.Name=clavicle_r |
| `Backward Clavicle_1` | Backward Clavicle<br/>Local Function | Backward Clavicle_1.Control.Type=Control; Backward Clavicle_1.Control.Name=clavicle_l_ctrl; Backward Clavicle_1.Bone.Type=Bone; Backward Clavicle_1.Bone.Name=clavicle_l |
| `EdGraphNode_Comment_11` | Backward Arms |  |
| `Backward Arm` | Backward Arm<br/>Local Function | Backward Arm.FK Controls=(); Backward Arm.Bones=(); Backward Arm.Ik Control.Type=Control; Backward Arm.Ik Control.Name=hand_l_ik_ctrl; Backward Arm.UpV Control.Type=Control; Backward Arm.UpV Control.Name=arm_l_pv_ik_ctrl; Backward Arm.FK Orient References=(); Backward Arm.Fingers Space.Type=Null; Backward Arm.Fingers Space.Name=fingers_l_space; Backward Arm.FK Arm Space.Type=Null; Backward Arm.FK Arm Space.Name=upperarm_l_fk_ctrl_space |
| `Backward Arm_1` | Backward Arm<br/>Local Function | Backward Arm_1.FK Controls=(); Backward Arm_1.Bones=(); Backward Arm_1.Ik Control.Type=Control; Backward Arm_1.Ik Control.Name=hand_r_ik_ctrl; Backward Arm_1.UpV Control.Type=Control; Backward Arm_1.UpV Control.Name=arm_r_pv_ik_ctrl; Backward Arm_1.FK Orient References=(); Backward Arm_1.Fingers Space.Type=Null; Backward Arm_1.Fingers Space.Name=fingers_r_space; Backward Arm_1.FK Arm Space.Type=Null; Backward Arm_1.FK Arm Space.Name=upperarm_r_fk_ctrl_space |
| `Backward Fingers` | Backward Fingers<br/>Local Function | Backward Fingers.Bones=() |
| `Backward Fingers_1` | Backward Fingers<br/>Local Function | Backward Fingers_1.Bones=() |
| `EdGraphNode_Comment_1` | IK FK Controls Visibility |  |
| `Backward Leg` | Backward Leg<br/>Local Function | Backward Leg.FK Controls=(); Backward Leg.Bones=(); Backward Leg.Ik Control.Type=Control; Backward Leg.Ik Control.Name=foot_l_ik_ctrl; Backward Leg.Pole Vector Control.Type=Control; Backward Leg.Pole Vector Control.Name=leg_l_pv_ik_ctrl; Backward Leg.Pole Vector Null.Type=Null; Backward Leg.Pole Vector Null.Name=leg_l_pv_footFollow |
| `EdGraphNode_Comment_11_1` | Backward Legs |  |
| `Backward Leg_1` | Backward Leg<br/>Local Function | Backward Leg_1.FK Controls=(); Backward Leg_1.Bones=(); Backward Leg_1.Ik Control.Type=Control; Backward Leg_1.Ik Control.Name=foot_r_ik_ctrl; Backward Leg_1.Pole Vector Control.Type=Control; Backward Leg_1.Pole Vector Control.Name=leg_r_pv_ik_ctrl; Backward Leg_1.Pole Vector Null.Type=Null; Backward Leg_1.Pole Vector Null.Name=leg_r_pv_footFollow |
| `Backward Foot` | Backward Foot<br/>Local Function | Backward Foot.Bone.Type=Bone; Backward Foot.Bone.Name=ball_r; Backward Foot.FK Control.Type=Control; Backward Foot.FK Control.Name=ball_r_fk_ctrl; Backward Foot.IK Control.Type=Control; Backward Foot.IK Control.Name=ball_r_ik_ctrl; Backward Foot.Toes Space.Type=Null; Backward Foot.Toes Space.Name=toes_r_space |
| `Backward Foot_1` | Backward Foot<br/>Local Function | Backward Foot_1.Bone.Type=Bone; Backward Foot_1.Bone.Name=ball_l; Backward Foot_1.FK Control.Type=Control; Backward Foot_1.FK Control.Name=ball_l_fk_ctrl; Backward Foot_1.IK Control.Type=Control; Backward Foot_1.IK Control.Name=ball_l_ik_ctrl; Backward Foot_1.Toes Space.Type=Null; Backward Foot_1.Toes Space.Name=toes_l_space |
| `Backward Fingers_2` | Backward Fingers<br/>Local Function | Backward Fingers_2.Bones=() |
| `Backward Fingers_1_1` | Backward Fingers<br/>Local Function | Backward Fingers_1_1.Bones=() |
| `ArrayIterator_2` |  |  |
| `PrepareForExecution` | Construction Event |  |
| `RerouteNode_51` |  |  |
| `RerouteNode_54` |  |  |
| `RerouteNode_55` |  |  |
| `RerouteNode_56` |  |  |
| `RerouteNode_57` |  |  |
| `RerouteNode_58` |  |  |
| `RerouteNode_60` |  |  |
| `RerouteNode_27` |  |  |
| `RerouteNode_61` |  |  |
| `RerouteNode_62` |  |  |
| `RerouteNode_64` |  |  |
| `RerouteNode_2` |  |  |
| `RerouteNode_63` |  |  |
| `RerouteNode_65` |  |  |
| `RerouteNode_66` |  |  |
| `RerouteNode_67` |  |  |
| `RerouteNode_68` |  |  |
| `RerouteNode_60_1` |  |  |
| `RerouteNode_7` |  |  |
| `RerouteNode_9` |  |  |
| `RerouteNode_12` |  |  |
| `RerouteNode_13` |  |  |
| `RerouteNode_14` |  |  |
| `RerouteNode_15` |  |  |
| `RerouteNode_16` |  |  |
| `RerouteNode_17` |  |  |
| `RerouteNode_2_1` |  |  |
| `RerouteNode_63_1` |  |  |
| `RerouteNode_22` |  |  |
| `RerouteNode_26` |  |  |
| `RerouteNode_38` |  |  |
| `RerouteNode_39` |  |  |
| `RerouteNode_40` |  |  |
| `RerouteNode_41` |  |  |
| `RerouteNode_42` |  |  |
| `RerouteNode_43` |  |  |
| `RerouteNode_52_1` |  |  |
| `RerouteNode_53_1` |  |  |
| `RerouteNode_20` |  |  |
| `RerouteNode_24` |  |  |
| `RerouteNode_55_1` |  |  |
| `RerouteNode_56_1` |  |  |
| `RerouteNode_57_1` |  |  |
| `RerouteNode_58_1` |  |  |
| `RerouteNode_59_2` |  |  |
| `RerouteNode_60_2` |  |  |
| `RerouteNode_12_1` |  |  |
| `RerouteNode_13_1` |  |  |
| `RerouteNode_1` |  |  |
| `RerouteNode_6` |  |  |
| `RerouteNode_8` |  |  |
| `RerouteNode_10` |  |  |
| `RerouteNode_2_2` |  |  |
| `RerouteNode_63_2` |  |  |
| `RerouteNode_3` |  |  |
| `VariableNode` | Get L Arm IK Mode<br/>Variable |  |
| `VariableNode_1` | Get R Arm IK Mode<br/>Variable |  |
| `VariableNode_2` | Get L Leg IK Mode <br/>Variable |  |
| `VariableNode_3` | Get R Leg IK Mode<br/>Variable |  |
| `VariableNode_4` | Set L Arm IK Mode<br/>Variable |  |
| `VariableNode_5` | Set R Arm IK Mode<br/>Variable |  |
| `VariableNode_2_1` | Get L Leg IK Mode <br/>Variable |  |
| `VariableNode_3_1` | Get R Leg IK Mode<br/>Variable |  |
| `VariableNode_6` | Set L Leg IK Mode <br/>Variable |  |
| `VariableNode_7` | Set R Leg IK Mode<br/>Variable |  |
| `Setup Reverse Foot` | Setup Reverse Foot<br/>Local Function | Setup Reverse Foot.BK Buffers=(); Setup Reverse Foot.BK Controls=(); Setup Reverse Foot.Bones=(); Setup Reverse Foot.Roll Control.Type=Control; Setup Reverse Foot.Roll Control.Name=foot_roll_l_ctrl; Setup Reverse Foot.Roll Buffer.Type=Null; Setup Reverse Foot.Roll Buffer.Name=foot_roll_l_bfr |
| `Setup Reverse Foot_1` | Setup Reverse Foot<br/>Local Function | Setup Reverse Foot_1.BK Buffers=(); Setup Reverse Foot_1.BK Controls=(); Setup Reverse Foot_1.Bones=(); Setup Reverse Foot_1.Roll Control.Type=Control; Setup Reverse Foot_1.Roll Control.Name=foot_roll_r_ctrl; Setup Reverse Foot_1.Roll Buffer.Type=Null; Setup Reverse Foot_1.Roll Buffer.Name=foot_roll_r_bfr; Setup Reverse Foot_1.Negative Side=true |
| `RerouteNode_11` |  |  |
| `RerouteNode_18` |  |  |
| `RerouteNode_23` |  |  |
| `RerouteNode_29` |  |  |
| `RerouteNode_30` |  |  |
| `RerouteNode_31` |  |  |
| `Forward Reverse Foot` | Forward Reverse Foot<br/>Local Function | Forward Reverse Foot.ExecuteContext=(); Forward Reverse Foot.BK Buffers=(); Forward Reverse Foot.BK Controls=(); Forward Reverse Foot.Roll Control.Type=Control; Forward Reverse Foot.Roll Control.Name=foot_roll_l_ctrl; Forward Reverse Foot.Roll Buffer.Type=Null; Forward Reverse Foot.Roll Buffer.Name=foot_roll_l_bfr; Forward Reverse Foot.IKFK Switch Control.Type=Control; Forward Reverse Foot.IKFK Switch Control.Name=leg_l_fk_ik_switch; Forward Reverse Foot.Roll Clamp=40.000000 |
| `Forward Reverse Foot_1` | Forward Reverse Foot<br/>Local Function | Forward Reverse Foot_1.BK Buffers=(); Forward Reverse Foot_1.BK Controls=(); Forward Reverse Foot_1.Roll Control.Type=Control; Forward Reverse Foot_1.Roll Control.Name=foot_roll_r_ctrl; Forward Reverse Foot_1.Roll Buffer.Type=Null; Forward Reverse Foot_1.Roll Buffer.Name=foot_roll_r_bfr; Forward Reverse Foot_1.IKFK Switch Control.Type=Control; Forward Reverse Foot_1.IKFK Switch Control.Name=leg_r_fk_ik_switch; Forward Reverse Foot_1.Roll Clamp=40.000000 |
| `RigUnit_GetControlFloat` | Get Control Float | RigUnit_GetControlFloat.Control=roll_blend_l_ctrl |
| `RigUnit_GetControlFloat_2` | Get Control Float | RigUnit_GetControlFloat_2.Control=roll_blend_r_ctrl |
| `RerouteNode_28` |  |  |
| `RerouteNode_25` |  |  |
| `RerouteNode_32` |  |  |
| `RerouteNode_33` |  |  |
| `Setup Spine` | Setup Spine<br/>Local Function | Setup Spine.Bones=(); Setup Spine.FK Controls=(); Setup Spine.IK Controls=(); Setup Spine.Chest Space.Type=Null; Setup Spine.Chest Space.Name=chest_space |
| `RerouteNode_52` |  |  |
| `RerouteNode_53` |  |  |
| `RerouteNode_11_1` |  |  |
| `VariableNode_8` | Set Spine Offsets<br/>Variable |  |
| `VariableNode_12` | Set Spine Length<br/>Variable |  |
| `Forward Spine` | Forward Spine<br/>Local Function | Forward Spine.FK Controls=(); Forward Spine.IK Controls=(); Forward Spine.Bones=(); Forward Spine.Pelvis Bone.Type=Bone; Forward Spine.Pelvis Bone.Name=pelvis; Forward Spine.Chest Space.Type=Null; Forward Spine.Chest Space.Name=chest_space; Forward Spine.IKFK Switch Control.Type=Control; Forward Spine.IKFK Switch Control.Name=spine_fk_ik_switch |
| `RerouteNode_4` |  |  |
| `RerouteNode_21` |  |  |
| `VariableNode_9` | Get Spine Offsets<br/>Variable |  |
| `RerouteNode_11_1_1` |  |  |
| `VariableNode_11` | Get Spine IK Mode<br/>Variable |  |
| `VariableNode_13` | Get Spine Length<br/>Variable |  |
| `VariableNode_10` | Set Spine IK Mode<br/>Variable |  |
| `RerouteNode_11_1_2` |  |  |
| `VariableNode_14` | Get Spine Offsets<br/>Variable |  |
| `Setup Neck_1` | Setup Neck<br/>Local Function | Setup Neck_1.Bones=(); Setup Neck_1.FK Controls=(); Setup Neck_1.IK Control.Type=Control; Setup Neck_1.IK Control.Name=head_ik_ctrl; Setup Neck_1.Head FK Space.Type=Null; Setup Neck_1.Head FK Space.Name=head_fk_space; Setup Neck_1.Head IK Space.Type=Null; Setup Neck_1.Head IK Space.Name=head_ik_space |
| `VariableNode_15` | Set Neck Length<br/>Variable |  |
| `Forward Neck_1` | Forward Neck<br/>Local Function | Forward Neck_1.FK Controls=(); Forward Neck_1.IK Control.Type=Control; Forward Neck_1.IK Control.Name=head_ik_ctrl; Forward Neck_1.Bones=(); Forward Neck_1.Head Orient Control.Type=Control; Forward Neck_1.Head Orient Control.Name=head_local; Forward Neck_1.IKFK Switch Control.Type=Control; Forward Neck_1.IKFK Switch Control.Name=neck_fk_ik_switch; Forward Neck_1.Mid IK Null.Type=Null; Forward Neck_1.Mid IK Null.Name=head_mid; Forward Neck_1.Head FK Space.Type=Null; Forward Neck_1.Head FK Space.Name=head_fk_space; Forward Neck_1.Head IK Space.Type=Null; Forward Neck_1.Head IK Space.Name=head_ik_space; Forward Neck_1.Head Orient Reference.Type=Control; Forward Neck_1.Head Orient Reference.Name=body_ctrl |
| `VariableNode_16` | Get Neck Length<br/>Variable |  |
| `RerouteNode_5_1` |  |  |
| `VariableNode_17` | Get Neck IK Mode<br/>Variable |  |
| `VariableNode_18` | Set Neck IK Mode<br/>Variable |  |
| `Set IKFK Visibility_1_2_1` | Set IKFK Visibility<br/>Local Function | Set IKFK Visibility_1_2_1.FK Controls=(); Set IKFK Visibility_1_2_1.IK Controls=() |
| `Set IKFK Visibility_1_2_1_1` | Set IKFK Visibility<br/>Local Function | Set IKFK Visibility_1_2_1_1.FK Controls=(); Set IKFK Visibility_1_2_1_1.IK Controls.0.Type=None; Set IKFK Visibility_1_2_1_1.IK Controls.0.Name=None |
| `RerouteNode_21_1` |  |  |
| `RerouteNode_11_1_1_1` |  |  |
| `RerouteNode_5_1_1` |  |  |
| `RerouteNode_5` |  |  |
| `RerouteNode_19` |  |  |
| `EdGraphNode_Comment_2` | Clavicles |  |
| `RerouteNode` |  |  |
| `RerouteNode_34` |  |  |
| `RerouteNode_35` |  |  |
| `RerouteNode_35_1` |  |  |
| `SequenceExecution_1` | 시퀀스 |  |
| `SequenceExecution_3` | 시퀀스 |  |
| `SequenceExecution` | 시퀀스 |  |
| `RigUnit_GetFloatAnimationChannel` | Get Float Channel | RigUnit_GetFloatAnimationChannel.Control=hand_r_ik_ctrl; RigUnit_GetFloatAnimationChannel.Channel=Softness; RigUnit_GetFloatAnimationChannel.bInitial=False |
| `RigUnit_GetFloatAnimationChannel_1` | Get Float Channel | RigUnit_GetFloatAnimationChannel_1.Control=hand_l_ik_ctrl; RigUnit_GetFloatAnimationChannel_1.Channel=Softness; RigUnit_GetFloatAnimationChannel_1.bInitial=False |
| `RigUnit_GetBoolAnimationChannel` | Get Bool Channel | RigUnit_GetBoolAnimationChannel.Control=hand_l_ik_ctrl; RigUnit_GetBoolAnimationChannel.Channel=Stretch; RigUnit_GetBoolAnimationChannel.bInitial=False |
| `RigUnit_GetBoolAnimationChannel_1` | Get Bool Channel | RigUnit_GetBoolAnimationChannel_1.Control=hand_r_ik_ctrl; RigUnit_GetBoolAnimationChannel_1.Channel=Stretch; RigUnit_GetBoolAnimationChannel_1.bInitial=False |
| `RigUnit_GetFloatAnimationChannel_2` | Get Float Channel | RigUnit_GetFloatAnimationChannel_2.Control=foot_l_ik_ctrl; RigUnit_GetFloatAnimationChannel_2.Channel=Softness; RigUnit_GetFloatAnimationChannel_2.bInitial=False |
| `RigUnit_GetFloatAnimationChannel_3` | Get Float Channel | RigUnit_GetFloatAnimationChannel_3.Control=foot_r_ik_ctrl; RigUnit_GetFloatAnimationChannel_3.Channel=Softness; RigUnit_GetFloatAnimationChannel_3.bInitial=False |
| `RerouteNode_44` |  |  |
| `RerouteNode_45` |  |  |
| `RigUnit_GetBoolAnimationChannel_2` | Get Bool Channel | RigUnit_GetBoolAnimationChannel_2.Control=pinky_metacarpal_l_ctrl; RigUnit_GetBoolAnimationChannel_2.Channel=Auto Interpolate; RigUnit_GetBoolAnimationChannel_2.bInitial=False |
| `RigUnit_GetBoolAnimationChannel_3` | Get Bool Channel | RigUnit_GetBoolAnimationChannel_3.Control=hand_l_ik_ctrl; RigUnit_GetBoolAnimationChannel_3.Channel=Keep Aligned; RigUnit_GetBoolAnimationChannel_3.bInitial=False |
| `RigUnit_GetBoolAnimationChannel_4` | Get Bool Channel | RigUnit_GetBoolAnimationChannel_4.Control=hand_r_ik_ctrl; RigUnit_GetBoolAnimationChannel_4.Channel=Keep Aligned; RigUnit_GetBoolAnimationChannel_4.bInitial=False |
| `RerouteNode_45_1` |  |  |
| `RerouteNode_36_1` |  |  |
| `RigUnit_GetBoolAnimationChannel_5` | Get Bool Channel | RigUnit_GetBoolAnimationChannel_5.Control=pinky_metacarpal_r_ctrl; RigUnit_GetBoolAnimationChannel_5.Channel=Auto Interpolate; RigUnit_GetBoolAnimationChannel_5.bInitial=False |
| `RerouteNode_47` |  |  |
| `RerouteNode_37` |  |  |
| `RerouteNode_46` |  |  |
| `RigUnit_GetBoolAnimationChannel_6` | Get Bool Channel | RigUnit_GetBoolAnimationChannel_6.Control=pinky_01_l_ctrl; RigUnit_GetBoolAnimationChannel_6.Channel=Auto Interpolate; RigUnit_GetBoolAnimationChannel_6.bInitial=False |
| `RerouteNode_48` |  |  |
| `RigUnit_GetBoolAnimationChannel_7` | Get Bool Channel | RigUnit_GetBoolAnimationChannel_7.Control=pinky_01_r_ctrl; RigUnit_GetBoolAnimationChannel_7.Channel=Auto Interpolate; RigUnit_GetBoolAnimationChannel_7.bInitial=False |
| `RigUnit_GetBoolAnimationChannel_8` | Get Bool Channel | RigUnit_GetBoolAnimationChannel_8.Control=chest_ctrl; RigUnit_GetBoolAnimationChannel_8.Channel=spine_stretch_switch; RigUnit_GetBoolAnimationChannel_8.bInitial=False |
| `RigUnit_GetBoolAnimationChannel_9` | Get Bool Channel | RigUnit_GetBoolAnimationChannel_9.Control=head_ctrl; RigUnit_GetBoolAnimationChannel_9.Channel=Stretch; RigUnit_GetBoolAnimationChannel_9.bInitial=False |
| `RigUnit_GetBoolAnimationChannel_10` | Get Bool Channel | RigUnit_GetBoolAnimationChannel_10.Control=global_ctrl; RigUnit_GetBoolAnimationChannel_10.Channel=neck_fk_ik_switch; RigUnit_GetBoolAnimationChannel_10.bInitial=False |
| `RigUnit_GetBoolAnimationChannel_11` | Get Bool Channel | RigUnit_GetBoolAnimationChannel_11.Control=global_ctrl; RigUnit_GetBoolAnimationChannel_11.Channel=spine_fk_ik_switch; RigUnit_GetBoolAnimationChannel_11.bInitial=False |
| `RigUnit_GetBoolAnimationChannel_12` | Get Bool Channel | RigUnit_GetBoolAnimationChannel_12.Control=global_ctrl; RigUnit_GetBoolAnimationChannel_12.Channel=arm_l_fk_ik_switch; RigUnit_GetBoolAnimationChannel_12.bInitial=False |
| `RigUnit_GetBoolAnimationChannel_13` | Get Bool Channel | RigUnit_GetBoolAnimationChannel_13.Control=global_ctrl; RigUnit_GetBoolAnimationChannel_13.Channel=arm_r_fk_ik_switch; RigUnit_GetBoolAnimationChannel_13.bInitial=False |
| `RigUnit_GetBoolAnimationChannel_14` | Get Bool Channel | RigUnit_GetBoolAnimationChannel_14.Control=foot_l_ik_ctrl; RigUnit_GetBoolAnimationChannel_14.Channel=Stretch; RigUnit_GetBoolAnimationChannel_14.bInitial=False |
| `RigUnit_GetBoolAnimationChannel_15` | Get Bool Channel | RigUnit_GetBoolAnimationChannel_15.Control=foot_r_ik_ctrl; RigUnit_GetBoolAnimationChannel_15.Channel=Stretch; RigUnit_GetBoolAnimationChannel_15.bInitial=False |
| `RigUnit_GetBoolAnimationChannel_16` | Get Bool Channel | RigUnit_GetBoolAnimationChannel_16.Control=global_ctrl; RigUnit_GetBoolAnimationChannel_16.Channel=leg_l_fk_ik_switch; RigUnit_GetBoolAnimationChannel_16.bInitial=False |
| `RigUnit_GetBoolAnimationChannel_17` | Get Bool Channel | RigUnit_GetBoolAnimationChannel_17.Control=global_ctrl; RigUnit_GetBoolAnimationChannel_17.Channel=leg_r_fk_ik_switch; RigUnit_GetBoolAnimationChannel_17.bInitial=False |
| `RigUnit_GetBoolAnimationChannel_18` | Get Bool Channel | RigUnit_GetBoolAnimationChannel_18.Control=global_ctrl; RigUnit_GetBoolAnimationChannel_18.Channel=arm_l_fk_ik_switch; RigUnit_GetBoolAnimationChannel_18.bInitial=False |
| `RigUnit_GetBoolAnimationChannel_19` | Get Bool Channel | RigUnit_GetBoolAnimationChannel_19.Control=global_ctrl; RigUnit_GetBoolAnimationChannel_19.Channel=leg_l_fk_ik_switch; RigUnit_GetBoolAnimationChannel_19.bInitial=False |
| `RigUnit_GetBoolAnimationChannel_20` | Get Bool Channel | RigUnit_GetBoolAnimationChannel_20.Control=global_ctrl; RigUnit_GetBoolAnimationChannel_20.Channel=arm_r_fk_ik_switch; RigUnit_GetBoolAnimationChannel_20.bInitial=False |
| `RigUnit_GetBoolAnimationChannel_21` | Get Bool Channel | RigUnit_GetBoolAnimationChannel_21.Control=global_ctrl; RigUnit_GetBoolAnimationChannel_21.Channel=leg_r_fk_ik_switch; RigUnit_GetBoolAnimationChannel_21.bInitial=False |
| `RigUnit_GetBoolAnimationChannel_22` | Get Bool Channel | RigUnit_GetBoolAnimationChannel_22.Control=global_ctrl; RigUnit_GetBoolAnimationChannel_22.Channel=ShowBodyControls; RigUnit_GetBoolAnimationChannel_22.bInitial=False |
| `RigUnit_GetBoolAnimationChannel_23` | Get Bool Channel | RigUnit_GetBoolAnimationChannel_23.Control=global_ctrl; RigUnit_GetBoolAnimationChannel_23.Channel=spine_fk_ik_switch; RigUnit_GetBoolAnimationChannel_23.bInitial=False |
| `RigUnit_GetBoolAnimationChannel_24` | Get Bool Channel | RigUnit_GetBoolAnimationChannel_24.Control=global_ctrl; RigUnit_GetBoolAnimationChannel_24.Channel=neck_fk_ik_switch; RigUnit_GetBoolAnimationChannel_24.bInitial=False |
| `Forward Fingers_4` | Forward Fingers<br/>Local Function | Forward Fingers_4.Fingers Bones=(); Forward Fingers_4.Metacarpals Controls=(); Forward Fingers_4.Base Fingers Controls=(); Forward Fingers_4.Proxy Controls=() |
| `Forward Fingers` | Forward Fingers<br/>Local Function | Forward Fingers.Fingers Bones=(); Forward Fingers.Metacarpals Controls=(); Forward Fingers.Base Fingers Controls=(); Forward Fingers.Proxy Controls=() |
| `Forward Fingers_1` | Forward Fingers<br/>Local Function | Forward Fingers_1.Fingers Bones=(); Forward Fingers_1.Metacarpals Controls=(); Forward Fingers_1.Auto Meta=False; Forward Fingers_1.Base Fingers Controls=(); Forward Fingers_1.Auto Base=False; Forward Fingers_1.Proxy Controls=() |
| `Forward Fingers_5` | Forward Fingers<br/>Local Function | Forward Fingers_5.Fingers Bones=(); Forward Fingers_5.Metacarpals Controls=(); Forward Fingers_5.Auto Meta=False; Forward Fingers_5.Base Fingers Controls=(); Forward Fingers_5.Auto Base=False; Forward Fingers_5.Proxy Controls=() |
| `VariableNode_19` | Get L Arm IK Align<br/>Variable |  |
| `VariableNode_20` | Get R Arm IK Align<br/>Variable |  |
| `VariableNode_21` | Set L Arm IK Align<br/>Variable |  |
| `RigUnit_GetBoolAnimationChannel_3_1` | Get Bool Channel | RigUnit_GetBoolAnimationChannel_3_1.Control=hand_l_ik_ctrl; RigUnit_GetBoolAnimationChannel_3_1.Channel=Keep Aligned; RigUnit_GetBoolAnimationChannel_3_1.bInitial=False |
| `VariableNode_22` | Set R Arm IK Align<br/>Variable |  |
| `RigUnit_GetBoolAnimationChannel_4_1` | Get Bool Channel | RigUnit_GetBoolAnimationChannel_4_1.Control=hand_r_ik_ctrl; RigUnit_GetBoolAnimationChannel_4_1.Channel=Keep Aligned; RigUnit_GetBoolAnimationChannel_4_1.bInitial=False |

</details>

### Set IKFK Keys

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `Entry` | Entry |  |
| `Return` | Return |  |
| `ArrayIterator_2_2` |  |  |
| `SendEvent_1` | Send Event | SendEvent_1.ExecuteContext=(); SendEvent_1.Event=RequestAutoKey; SendEvent_1.Item.Type=None; SendEvent_1.Item.Name=None; SendEvent_1.bEnable=True; SendEvent_1.bOnlyDuringInteraction=True |
| `Branch_1_1` |  |  |
| `ArrayIterator_2_2_1` |  |  |
| `SendEvent_1_1` | Send Event | SendEvent_1_1.ExecuteContext=(); SendEvent_1_1.Event=RequestAutoKey; SendEvent_1_1.Item.Type=None; SendEvent_1_1.Item.Name=None; SendEvent_1_1.bEnable=True; SendEvent_1_1.bOnlyDuringInteraction=True |
| `SendEvent_1_1_1` | Send Event | SendEvent_1_1_1.Event=RequestAutoKey; SendEvent_1_1_1.Item.Type=None; SendEvent_1_1_1.Item.Name=None; SendEvent_1_1_1.bEnable=True; SendEvent_1_1_1.bOnlyDuringInteraction=True |
| `VariableNode` | Get FK Controls<br/>Input parameter |  |
| `VariableNode_1` | Get IK Controls <br/>Input parameter |  |
| `VariableNode_2` | Get Switch Control<br/>Input parameter |  |
| `SequenceExecution_1` | 시퀀스 |  |
| `MathBoolNotEquals` | Not Equals |  |
| `SequenceExecution` | 시퀀스 | SequenceExecution.ExecuteContext=() |

</details>

### Set IKFK Visibility

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `Entry` | Entry |  |
| `Return` | Return |  |
| `ArrayIterator_2_1` |  |  |
| `SetControlVisibility_2` | Set Control Visibility | SetControlVisibility_2.ExecuteContext=(); SetControlVisibility_2.Item.Type=Control; SetControlVisibility_2.Item.Name=None; SetControlVisibility_2.bVisible=True |
| `ArrayIterator_2_1_1` |  |  |
| `SetControlVisibility_2_1` | Set Control Visibility | SetControlVisibility_2_1.ExecuteContext=(); SetControlVisibility_2_1.Item.Type=Control; SetControlVisibility_2_1.Item.Name=None; SetControlVisibility_2_1.bVisible=True |
| `MathBoolNot_1` | Not |  |
| `VariableNode` | Get FK Controls<br/>Input parameter |  |
| `VariableNode_1` | Get IK Controls<br/>Input parameter |  |
| `If` |  |  |
| `If_1` |  |  |
| `SequenceExecution` | 시퀀스 |  |

</details>

### Setup Arm

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `Entry` | Entry |  |
| `Return` | Return | Return.ExecuteContext=() |
| `MathVectorDistance` | Distance Between | MathVectorDistance.A.X=0.000000; MathVectorDistance.A.Y=0.000000; MathVectorDistance.A.Z=0.000000; MathVectorDistance.B.X=0.000000; MathVectorDistance.B.Y=0.000000; MathVectorDistance.B.Z=0.000000 |
| `MathTransformMul` | Multiply | MathTransformMul.A.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); MathTransformMul.A.Translation.X=0.000000; MathTransformMul.A.Translation.Z=0.000000; MathTransformMul.A.Scale3D.X=1.000000; MathTransformMul.A.Scale3D.Y=1.000000; MathTransformMul.A.Scale3D.Z=1.000000; MathTransformMul.B.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); MathTransformMul.B.Translation.X=0.000000; MathTransformMul.B.Translation.Y=0.000000; MathTransformMul.B.Translation.Z=0.000000; MathTransformMul.B.Scale3D.X=1.000000; MathTransformMul.B.Scale3D.Y=1.000000; MathTransformMul.B.Scale3D.Z=1.000000 |
| `MathFloatDiv` | Divide | MathFloatDiv.B=2.000000 |
| `ArrayGetAtIndex_1` |  |  |
| `MathFloatNegate` | Negate |  |
| `ArrayGetAtIndex_3` |  |  |
| `VariableNode_2` | Get Pole Vector Control<br/>Input parameter |  |
| `SetTransform_1` | Set Transform - Bone Initial | SetTransform_1.Item.Type=Bone; SetTransform_1.Item.Name=None; SetTransform_1.Space=GlobalSpace; SetTransform_1.bInitial=true; SetTransform_1.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform_1.Value.Translation.X=0.000000; SetTransform_1.Value.Translation.Y=0.000000; SetTransform_1.Value.Translation.Z=0.000000; SetTransform_1.Value.Scale3D.X=1.000000; SetTransform_1.Value.Scale3D.Y=1.000000; SetTransform_1.Value.Scale3D.Z=1.000000; SetTransform_1.Weight=1.000000; SetTransform_1.bPropagateToChildren=True |
| `ArrayGetAtIndex_2_1` |  |  |
| `EdGraphNode_Comment_4` | Initial UpVector |  |
| `VariableNode_6` | Get FK Space<br/>Input parameter |  |
| `SetTransform_3` | Set Transform - Bone Initial | SetTransform_3.Item.Type=Bone; SetTransform_3.Item.Name=None; SetTransform_3.Space=GlobalSpace; SetTransform_3.bInitial=true; SetTransform_3.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform_3.Value.Translation.X=0.000000; SetTransform_3.Value.Translation.Y=0.000000; SetTransform_3.Value.Translation.Z=0.000000; SetTransform_3.Value.Scale3D.X=1.000000; SetTransform_3.Value.Scale3D.Y=1.000000; SetTransform_3.Value.Scale3D.Z=1.000000; SetTransform_3.Weight=1.000000; SetTransform_3.bPropagateToChildren=True |
| `EdGraphNode_Comment_8` | Initial FK Controls |  |
| `VariableNode_7` | Get Transforms<br/>Local Variable<br/>Default () |  |
| `VariableNode_1_2_1` | Get Bones<br/>Input parameter |  |
| `GetTransformItemArray_1` | Get Transform Array | GetTransformItemArray_1.Space=GlobalSpace |
| `ArrayIterator_2` |  |  |
| `VariableNode_7_1` | Get Transforms<br/>Local Variable<br/>Default () |  |
| `ArraySetNum` |  |  |
| `ArrayGetNum` |  |  |
| `ArraySetAtIndex` |  |  |
| `EdGraphNode_Comment_3_1` | Hand has an 90 degree offset |  |
| `If_3` |  |  |
| `MathQuaternionToRotator_2` | To Rotator | MathQuaternionToRotator_2.Value=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000) |
| `MathFloatAdd_2` | Add | MathFloatAdd_2.B=90.000000 |
| `MathIntSub_1` | Subtract | MathIntSub_1.B=1 |
| `VariableNode_8` | Get Transforms<br/>Local Variable<br/>Default () |  |
| `ArrayGetAtIndex_4` |  |  |
| `VariableNode` | Get Transforms<br/>Local Variable<br/>Default () |  |
| `SetTransformItemArray` | Set Transform Array | SetTransformItemArray.Space=GlobalSpace; SetTransformItemArray.bInitial=true; SetTransformItemArray.Weight=1.000000; SetTransformItemArray.bPropagateToChildren=True |
| `VariableNode_1` | Get FK Controls<br/>Input parameter |  |
| `VariableNode_4_1` | Get IK Control<br/>Input parameter |  |
| `SetTransform_2_1` | Set Transform - Bone Initial | SetTransform_2_1.Item.Type=Bone; SetTransform_2_1.Item.Name=None; SetTransform_2_1.Space=GlobalSpace; SetTransform_2_1.bInitial=true; SetTransform_2_1.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform_2_1.Value.Translation.X=0.000000; SetTransform_2_1.Value.Translation.Y=0.000000; SetTransform_2_1.Value.Translation.Z=0.000000; SetTransform_2_1.Value.Scale3D.X=1.000000; SetTransform_2_1.Value.Scale3D.Y=1.000000; SetTransform_2_1.Value.Scale3D.Z=1.000000; SetTransform_2_1.Weight=1.000000; SetTransform_2_1.bPropagateToChildren=True |
| `VariableNode_8_1` | Get Transforms<br/>Local Variable<br/>Default () |  |
| `ArrayGetAtIndex_4_1` |  |  |
| `ArrayGetNum_1` |  |  |
| `MathIntSub` | Subtract | MathIntSub.B=1 |
| `VariableNode_8_1_1` | Get Transforms<br/>Local Variable<br/>Default () |  |
| `EdGraphNode_Comment_9` | Initial IK |  |
| `EdGraphNode_Comment_10` | Calculate Initial Transforms |  |
| `Get Negative Transform` | Get Negative Transform<br/>Local Function | Get Negative Transform.Input.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); Get Negative Transform.Input.Translation.X=0.000000; Get Negative Transform.Input.Translation.Y=0.000000; Get Negative Transform.Input.Translation.Z=0.000000; Get Negative Transform.Input.Scale3D.X=1.000000; Get Negative Transform.Input.Scale3D.Y=1.000000; Get Negative Transform.Input.Scale3D.Z=1.000000 |
| `If_2` |  |  |
| `VariableNode_3` | Get Fingers Space<br/>Input parameter |  |
| `VariableNode_4` | Get Negative Side<br/>Input parameter |  |
| `SetTransform_2_1_1` | Set Transform - Bone Initial | SetTransform_2_1_1.Item.Type=Bone; SetTransform_2_1_1.Item.Name=None; SetTransform_2_1_1.Space=GlobalSpace; SetTransform_2_1_1.bInitial=true; SetTransform_2_1_1.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform_2_1_1.Value.Translation.X=0.000000; SetTransform_2_1_1.Value.Translation.Y=0.000000; SetTransform_2_1_1.Value.Translation.Z=0.000000; SetTransform_2_1_1.Value.Scale3D.X=1.000000; SetTransform_2_1_1.Value.Scale3D.Y=1.000000; SetTransform_2_1_1.Value.Scale3D.Z=1.000000; SetTransform_2_1_1.Weight=1.000000; SetTransform_2_1_1.bPropagateToChildren=True |
| `EdGraphNode_Comment_9_1` | Initial Finger Space |  |
| `VariableNode_5` | Get Bones<br/>Input parameter |  |
| `ArrayGetAtIndex` |  |  |
| `GetTransform` | Get Transform - Bone | GetTransform.Item.Type=Bone; GetTransform.Item.Name=None; GetTransform.Space=GlobalSpace |
| `Get Negative Transform_0` | Get Negative Transform<br/>Local Function | Get Negative Transform_0.Input.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); Get Negative Transform_0.Input.Translation.X=0.000000; Get Negative Transform_0.Input.Translation.Y=0.000000; Get Negative Transform_0.Input.Translation.Z=0.000000; Get Negative Transform_0.Input.Scale3D.X=1.000000; Get Negative Transform_0.Input.Scale3D.Y=1.000000; Get Negative Transform_0.Input.Scale3D.Z=1.000000 |
| `If_2_1` |  |  |
| `VariableNode_4_2` | Get Negative Side<br/>Input parameter |  |
| `If_1` |  |  |
| `VariableNode_4_2_1` | Get Negative Side<br/>Input parameter |  |
| `VariableNode_9` | Get IK Parents<br/>Input parameter |  |
| `ArrayIterator` |  |  |
| `AddParent` | 부모 관계 생성 | AddParent.ExecuteContext=(); AddParent.Child.Type=Control; AddParent.Child.Name=None; AddParent.Parent.Type=Control; AddParent.Parent.Name=None |
| `VariableNode_10` | Get IK Control<br/>Input parameter |  |
| `AddParent_1` | 부모 관계 생성 | AddParent_1.Child.Type=Control; AddParent_1.Child.Name=None; AddParent_1.Parent.Type=Control; AddParent_1.Parent.Name=None |
| `VariableNode_11` | Get Pole Vector Control<br/>Input parameter |  |
| `SequenceExecution` | 시퀀스 |  |
| `MathIntEquals_1` | Equals | MathIntEquals_1.A=0 |
| `MathQuaternionFromRotator_2` | From Rotator |  |

</details>

### Setup Clavicle

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `Entry` | Entry |  |
| `Return` | Return |  |
| `GetTransform` | Get Transform - Bone | GetTransform.Item.Type=Bone; GetTransform.Item.Name=None; GetTransform.Space=GlobalSpace |
| `SetTransform` | Set Transform - Bone Initial | SetTransform.Item.Type=Bone; SetTransform.Item.Name=None; SetTransform.Space=GlobalSpace; SetTransform.bInitial=true; SetTransform.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform.Value.Translation.X=0.000000; SetTransform.Value.Translation.Y=0.000000; SetTransform.Value.Translation.Z=0.000000; SetTransform.Value.Scale3D.X=1.000000; SetTransform.Value.Scale3D.Y=1.000000; SetTransform.Value.Scale3D.Z=1.000000; SetTransform.Weight=1.000000; SetTransform.bPropagateToChildren=True |
| `VariableNode` | Get Negative Side<br/>Input parameter |  |
| `If_2` |  |  |
| `Get Negative Transform_1` | Get Negative Transform<br/>Local Function | Get Negative Transform_1.Input.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); Get Negative Transform_1.Input.Translation.X=0.000000; Get Negative Transform_1.Input.Translation.Y=0.000000; Get Negative Transform_1.Input.Translation.Z=0.000000; Get Negative Transform_1.Input.Scale3D.X=0.000000; Get Negative Transform_1.Input.Scale3D.Y=0.000000; Get Negative Transform_1.Input.Scale3D.Z=0.000000 |

</details>

### Setup Fingers

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `Entry` | Entry |  |
| `Return` | Return | Return.Execute=() |
| `ArrayIterator` |  |  |
| `NameConcat_4_1` | Concat | NameConcat_4_1.A=None; NameConcat_4_1.B=_ctrl |
| `GetTransform_6` | Get Transform - Bone | GetTransform_6.Item.Type=Bone; GetTransform_6.Item.Name=None; GetTransform_6.Space=GlobalSpace |
| `SetTransform_2` | Set Transform - Control Initial | SetTransform_2.ExecuteContext=(); SetTransform_2.Item.Type=Control; SetTransform_2.Space=GlobalSpace; SetTransform_2.bInitial=true; SetTransform_2.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform_2.Value.Translation.X=0.000000; SetTransform_2.Value.Translation.Y=0.000000; SetTransform_2.Value.Translation.Z=0.000000; SetTransform_2.Value.Scale3D.X=1.000000; SetTransform_2.Value.Scale3D.Y=1.000000; SetTransform_2.Value.Scale3D.Z=1.000000; SetTransform_2.Weight=1.000000; SetTransform_2.bPropagateToChildren=True |
| `MathQuaternionToRotator_1_1` | To Rotator |  |
| `MathFloatAdd_1_1` | Add | MathFloatAdd_1_1.B=180.000000 |
| `If_2_1` |  |  |
| `If_1_1` |  |  |
| `VariableNode_5_1` | Get Negative Side<br/>Input parameter |  |
| `EdGraphNode_Comment_7_1` | Negative Side |  |
| `MathQuaternionFromRotator_1_1` | From Rotator |  |
| `VariableNode_1` | Get Fingers Bones<br/>Input parameter |  |

</details>

### Setup Foot

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `Entry` | Entry |  |
| `Return` | Return |  |
| `GetTransform` | Get Transform - Bone | GetTransform.Item.Type=Bone; GetTransform.Item.Name=None; GetTransform.Space=GlobalSpace |
| `VariableNode` | Get IK Control<br/>Input parameter |  |
| `SetTransform` | Set Transform - Bone Initial | SetTransform.Item.Type=Bone; SetTransform.Item.Name=None; SetTransform.Space=GlobalSpace; SetTransform.bInitial=true; SetTransform.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform.Value.Translation.X=0.000000; SetTransform.Value.Translation.Y=0.000000; SetTransform.Value.Translation.Z=0.000000; SetTransform.Value.Scale3D.X=1.000000; SetTransform.Value.Scale3D.Y=1.000000; SetTransform.Value.Scale3D.Z=1.000000; SetTransform.Weight=1.000000; SetTransform.bPropagateToChildren=True |
| `SetTransform_1` | Set Transform - Bone Initial | SetTransform_1.Item.Type=Bone; SetTransform_1.Item.Name=None; SetTransform_1.Space=GlobalSpace; SetTransform_1.bInitial=true; SetTransform_1.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform_1.Value.Translation.X=0.000000; SetTransform_1.Value.Translation.Y=0.000000; SetTransform_1.Value.Translation.Z=0.000000; SetTransform_1.Value.Scale3D.X=1.000000; SetTransform_1.Value.Scale3D.Y=1.000000; SetTransform_1.Value.Scale3D.Z=1.000000; SetTransform_1.Weight=1.000000; SetTransform_1.bPropagateToChildren=True |
| `VariableNode_1` | Get FK Control<br/>Input parameter |  |
| `MathQuaternionToRotator_2` | To Rotator |  |
| `MathFloatAdd_2` | Add | MathFloatAdd_2.B=90.000000 |
| `If` |  |  |
| `VariableNode_2` | Get Negative Side<br/>Input parameter |  |
| `MathQuaternionToEuler` | To Euler | MathQuaternionToEuler.RotationOrder=YXZ |
| `MathFloatAdd` | Add | MathFloatAdd.B=180.000000 |
| `MathQuaternionFromEuler` | From Euler | MathQuaternionFromEuler.RotationOrder=YXZ |
| `VariableNode_3` | Get Bone<br/>Input parameter |  |
| `SetTransform_2_1_1` | Set Transform - Bone Initial | SetTransform_2_1_1.Item.Type=Bone; SetTransform_2_1_1.Item.Name=None; SetTransform_2_1_1.Space=GlobalSpace; SetTransform_2_1_1.bInitial=true; SetTransform_2_1_1.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform_2_1_1.Value.Translation.X=0.000000; SetTransform_2_1_1.Value.Translation.Y=0.000000; SetTransform_2_1_1.Value.Translation.Z=0.000000; SetTransform_2_1_1.Value.Scale3D.X=1.000000; SetTransform_2_1_1.Value.Scale3D.Y=1.000000; SetTransform_2_1_1.Value.Scale3D.Z=1.000000; SetTransform_2_1_1.Weight=1.000000; SetTransform_2_1_1.bPropagateToChildren=True |
| `EdGraphNode_Comment_9_1` | Initial Toes Space |  |
| `VariableNode_3_1` | Get Bone<br/>Input parameter |  |
| `GetTransform_1` | Get Transform - Bone | GetTransform_1.Item.Type=Bone; GetTransform_1.Item.Name=None; GetTransform_1.Space=GlobalSpace |
| `EdGraphNode_Comment_8` | IK  |  |
| `GetTransform_2` | Get Transform - Bone | GetTransform_2.Item.Type=Bone; GetTransform_2.Item.Name=None; GetTransform_2.Space=GlobalSpace |
| `VariableNode_3_2` | Get Bone<br/>Input parameter |  |
| `EdGraphNode_Comment_9` | FK |  |
| `EdGraphNode_Comment_10` | Negative Side |  |
| `If_1` |  |  |
| `VariableNode_4` | Get Negative Side<br/>Input parameter |  |
| `If_1_1` |  |  |
| `VariableNode_5` | Get Toes Space<br/>Input parameter |  |
| `SequenceExecution` | 시퀀스 |  |
| `MathQuaternionFromRotator_2` | From Rotator |  |

</details>

### Setup Leg

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `Entry` | Entry |  |
| `Return` | Return | Return.ExecuteContext=() |
| `MathVectorDistance` | Distance Between | MathVectorDistance.A.X=0.000000; MathVectorDistance.A.Y=0.000000; MathVectorDistance.A.Z=0.000000; MathVectorDistance.B.X=0.000000; MathVectorDistance.B.Y=0.000000; MathVectorDistance.B.Z=0.000000 |
| `MathTransformMul` | Multiply | MathTransformMul.A.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); MathTransformMul.A.Translation.X=0.000000; MathTransformMul.A.Translation.Z=0.000000; MathTransformMul.A.Scale3D.X=1.000000; MathTransformMul.A.Scale3D.Y=1.000000; MathTransformMul.A.Scale3D.Z=1.000000; MathTransformMul.B.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); MathTransformMul.B.Translation.X=0.000000; MathTransformMul.B.Translation.Y=0.000000; MathTransformMul.B.Translation.Z=0.000000; MathTransformMul.B.Scale3D.X=1.000000; MathTransformMul.B.Scale3D.Y=1.000000; MathTransformMul.B.Scale3D.Z=1.000000 |
| `MathFloatDiv` | Divide | MathFloatDiv.B=2.000000 |
| `ArrayGetAtIndex_1` |  |  |
| `ArrayGetAtIndex_3` |  |  |
| `VariableNode_2` | Get Pole Vector Control<br/>Input parameter |  |
| `SetTransform_1` | Set Transform - Bone Initial | SetTransform_1.Item.Type=Bone; SetTransform_1.Item.Name=None; SetTransform_1.Space=GlobalSpace; SetTransform_1.bInitial=true; SetTransform_1.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform_1.Value.Translation.X=0.000000; SetTransform_1.Value.Translation.Y=0.000000; SetTransform_1.Value.Translation.Z=0.000000; SetTransform_1.Value.Scale3D.X=1.000000; SetTransform_1.Value.Scale3D.Y=1.000000; SetTransform_1.Value.Scale3D.Z=1.000000; SetTransform_1.Weight=1.000000; SetTransform_1.bPropagateToChildren=True |
| `ArrayGetAtIndex_2_1` |  |  |
| `EdGraphNode_Comment_4` | Initial UpVector |  |
| `VariableNode_6` | Get FK Space<br/>Input parameter |  |
| `SetTransform_3` | Set Transform - Bone Initial | SetTransform_3.Item.Type=Bone; SetTransform_3.Item.Name=None; SetTransform_3.Space=GlobalSpace; SetTransform_3.bInitial=true; SetTransform_3.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform_3.Value.Translation.X=0.000000; SetTransform_3.Value.Translation.Y=0.000000; SetTransform_3.Value.Translation.Z=0.000000; SetTransform_3.Value.Scale3D.X=1.000000; SetTransform_3.Value.Scale3D.Y=1.000000; SetTransform_3.Value.Scale3D.Z=1.000000; SetTransform_3.Weight=1.000000; SetTransform_3.bPropagateToChildren=True |
| `EdGraphNode_Comment_8` | Initial FK Controls |  |
| `VariableNode_7` | Get Transforms<br/>Local Variable<br/>Default () |  |
| `VariableNode_1_2_1` | Get Bones<br/>Input parameter |  |
| `GetTransformItemArray_1` | Get Transform Array | GetTransformItemArray_1.Space=GlobalSpace |
| `ArrayIterator_2` |  |  |
| `VariableNode_7_1` | Get Transforms<br/>Local Variable<br/>Default () |  |
| `ArraySetNum` |  |  |
| `ArrayGetNum` |  |  |
| `ArraySetAtIndex` |  |  |
| `VariableNode_8` | Get Transforms<br/>Local Variable<br/>Default () |  |
| `ArrayGetAtIndex_4` |  |  |
| `VariableNode` | Get Transforms<br/>Local Variable<br/>Default () |  |
| `SetTransformItemArray` | Set Transform Array | SetTransformItemArray.Space=GlobalSpace; SetTransformItemArray.bInitial=true; SetTransformItemArray.Weight=1.000000; SetTransformItemArray.bPropagateToChildren=True |
| `VariableNode_1` | Get FK Controls<br/>Input parameter |  |
| `VariableNode_4_1` | Get IK Control<br/>Input parameter |  |
| `SetTransform_2_1` | Set Transform - Bone Initial | SetTransform_2_1.Item.Type=Bone; SetTransform_2_1.Item.Name=None; SetTransform_2_1.Space=GlobalSpace; SetTransform_2_1.bInitial=true; SetTransform_2_1.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform_2_1.Value.Translation.X=0.000000; SetTransform_2_1.Value.Translation.Y=0.000000; SetTransform_2_1.Value.Translation.Z=0.000000; SetTransform_2_1.Value.Scale3D.X=1.000000; SetTransform_2_1.Value.Scale3D.Y=1.000000; SetTransform_2_1.Value.Scale3D.Z=1.000000; SetTransform_2_1.Weight=1.000000; SetTransform_2_1.bPropagateToChildren=True |
| `VariableNode_8_1` | Get Transforms<br/>Local Variable<br/>Default () |  |
| `VariableNode_8_1_1` | Get Transforms<br/>Local Variable<br/>Default () |  |
| `EdGraphNode_Comment_9` | Initial IK |  |
| `EdGraphNode_Comment_10` | Calculate Initial Transforms |  |
| `If_2` |  |  |
| `VariableNode_3` | Get Negative Side<br/>Input parameter |  |
| `If` |  |  |
| `VariableNode_4` | Get Negative Side<br/>Input parameter |  |
| `If_1` |  |  |
| `VariableNode_4_2` | Get Negative Side<br/>Input parameter |  |
| `Get Negative Transform_1` | Get Negative Transform<br/>Local Function | Get Negative Transform_1.Input.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); Get Negative Transform_1.Input.Translation.X=0.000000; Get Negative Transform_1.Input.Translation.Y=0.000000; Get Negative Transform_1.Input.Translation.Z=0.000000; Get Negative Transform_1.Input.Scale3D.X=0.000000; Get Negative Transform_1.Input.Scale3D.Y=0.000000; Get Negative Transform_1.Input.Scale3D.Z=0.000000 |
| `ArrayIterator` |  |  |
| `AddParent` | 부모 관계 생성 | AddParent.ExecuteContext=(); AddParent.Child.Type=Control; AddParent.Child.Name=None; AddParent.Parent.Type=Control; AddParent.Parent.Name=None |
| `VariableNode_10` | Get IK Control<br/>Input parameter |  |
| `AddParent_1` | 부모 관계 생성 | AddParent_1.Child.Type=Control; AddParent_1.Child.Name=None; AddParent_1.Parent.Type=Control; AddParent_1.Parent.Name=None |
| `VariableNode_11` | Get Pole Vector Control<br/>Input parameter |  |
| `VariableNode_5` | Get IK Parents<br/>Input parameter |  |
| `SequenceExecution` | 시퀀스 |  |
| `ArrayGetAtIndex` |  |  |
| `VariableNode_10_1` | Get IK Control<br/>Input parameter |  |
| `VariableNode_1_2_1_1` | Get Bones<br/>Input parameter |  |
| `ArrayGetAtIndex_2` |  |  |
| `VariableNode_9` | Get Pole Vector Null<br/>Input parameter |  |
| `EdGraphNode_Comment_1` | PV Foot Attach |  |
| `Compute Pole Vector Parent` | Compute Pole Vector Parent<br/>Local Function | Compute Pole Vector Parent.Foot Control.Type=Control; Compute Pole Vector Parent.Foot Control.Name=foot_l_ik_ctrl; Compute Pole Vector Parent.Root Bone.Type=Bone; Compute Pole Vector Parent.Root Bone.Name=thigh_l; Compute Pole Vector Parent.Pole Vector Null.Type=Control; Compute Pole Vector Parent.Pole Vector Null.Name=FootParent; Compute Pole Vector Parent.Initial=true |
| `VariableNode_11_1` | Get Pole Vector Control<br/>Input parameter |  |
| `AddParent_1_1` | 부모 관계 생성 | AddParent_1_1.Child.Type=Control; AddParent_1_1.Child.Name=None; AddParent_1_1.Parent.Type=Control; AddParent_1_1.Parent.Name=None |
| `VariableNode_9_1` | Get Pole Vector Null<br/>Input parameter |  |

</details>

### Setup Neck

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `Entry` | Entry |  |
| `Return` | Return |  |
| `ArrayIterator` |  |  |
| `ArrayGetAtIndex` |  |  |
| `SetTransform_28` | Set Transform - Bone Initial | SetTransform_28.ExecuteContext=(); SetTransform_28.Item.Type=Bone; SetTransform_28.Item.Name=None; SetTransform_28.Space=GlobalSpace; SetTransform_28.bInitial=true; SetTransform_28.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform_28.Value.Translation.X=0.000000; SetTransform_28.Value.Translation.Y=0.000000; SetTransform_28.Value.Translation.Z=0.000000; SetTransform_28.Value.Scale3D.X=1.000000; SetTransform_28.Value.Scale3D.Y=1.000000; SetTransform_28.Value.Scale3D.Z=1.000000; SetTransform_28.Weight=1.000000; SetTransform_28.bPropagateToChildren=True |
| `GetTransform_15` | Get Transform - Bone | GetTransform_15.Item.Type=Bone; GetTransform_15.Item.Name=None; GetTransform_15.Space=GlobalSpace |
| `ArrayGetAtIndex_1` |  |  |
| `VariableNode` | Get FK Controls<br/>Input parameter |  |
| `VariableNode_1` | Get Bones<br/>Input parameter |  |
| `VariableNode_2` | Get Bones<br/>Input parameter |  |
| `ArrayGetNum` |  |  |
| `MathIntSub` | Subtract | MathIntSub.B=1 |
| `SetTransform` | Set Transform - Bone Initial | SetTransform.Item.Type=Bone; SetTransform.Item.Name=None; SetTransform.Space=GlobalSpace; SetTransform.bInitial=true; SetTransform.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform.Value.Translation.X=0.000000; SetTransform.Value.Translation.Y=0.000000; SetTransform.Value.Translation.Z=0.000000; SetTransform.Value.Scale3D.X=1.000000; SetTransform.Value.Scale3D.Y=1.000000; SetTransform.Value.Scale3D.Z=1.000000; SetTransform.Weight=1.000000; SetTransform.bPropagateToChildren=True |
| `GetTransform` | Get Transform - Bone | GetTransform.Item.Type=Bone; GetTransform.Item.Name=None; GetTransform.Space=GlobalSpace |
| `EdGraphNode_Comment_0` | Initial FK Controls |  |
| `EdGraphNode_Comment_1` | Initial Head Null  |  |
| `VariableNode_3` | Get Head FK Space<br/>Input parameter |  |
| `SetTransform_28_1` | Set Transform - Bone Initial | SetTransform_28_1.Item.Type=Bone; SetTransform_28_1.Item.Name=None; SetTransform_28_1.Space=GlobalSpace; SetTransform_28_1.bInitial=true; SetTransform_28_1.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform_28_1.Value.Translation.X=0.000000; SetTransform_28_1.Value.Translation.Y=0.000000; SetTransform_28_1.Value.Translation.Z=0.000000; SetTransform_28_1.Value.Scale3D.X=1.000000; SetTransform_28_1.Value.Scale3D.Y=1.000000; SetTransform_28_1.Value.Scale3D.Z=1.000000; SetTransform_28_1.Weight=1.000000; SetTransform_28_1.bPropagateToChildren=True |
| `VariableNode_4` | Get IK Control<br/>Input parameter |  |
| `VariableNode_1_1` | Get Bones<br/>Input parameter |  |
| `ArrayGetAtIndex_2` |  |  |
| `GetTransform_15_1` | Get Transform - Bone | GetTransform_15_1.Item.Type=Bone; GetTransform_15_1.Item.Name=None; GetTransform_15_1.Space=GlobalSpace |
| `ArrayGetNum_1` |  |  |
| `MathIntSub_1` | Subtract | MathIntSub_1.B=1 |
| `EdGraphNode_Comment_3` | Initial IK Control |  |
| `ControlRigSplineFromPoints` | Spline From Points | ControlRigSplineFromPoints.Points.0.X=0.000000; ControlRigSplineFromPoints.Points.0.Y=0.000000; ControlRigSplineFromPoints.Points.0.Z=0.000000; ControlRigSplineFromPoints.Points.1.X=0.000000; ControlRigSplineFromPoints.Points.1.Y=0.000000; ControlRigSplineFromPoints.Points.1.Z=0.000000; ControlRigSplineFromPoints.Points.2.X=0.000000; ControlRigSplineFromPoints.Points.2.Y=0.000000; ControlRigSplineFromPoints.Points.2.Z=0.000000; ControlRigSplineFromPoints.Points.3.X=0.000000; ControlRigSplineFromPoints.Points.3.Y=0.000000; ControlRigSplineFromPoints.Points.3.Z=0.000000; ControlRigSplineFromPoints.SplineMode=BSpline; ControlRigSplineFromPoints.bClosed=False; ControlRigSplineFromPoints.SamplesPerSegment=16 |
| `VariableNode_1_1_1` | Get Bones<br/>Input parameter |  |
| `GetTransformItemArray` | Get Transform Array | GetTransformItemArray.Space=GlobalSpace |
| `ArrayGetAtIndex_3` |  |  |
| `ArrayGetAtIndex_3_1` |  |  |
| `ArrayGetAtIndex_3_2` |  |  |
| `GetLengthControlRigSpline` | Get Length Of Spline |  |
| `EdGraphNode_Comment_2` | Default Length |  |
| `SetTransform_1` | Set Transform - Bone Initial | SetTransform_1.Item.Type=Bone; SetTransform_1.Item.Name=None; SetTransform_1.Space=GlobalSpace; SetTransform_1.bInitial=true; SetTransform_1.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform_1.Value.Translation.X=0.000000; SetTransform_1.Value.Translation.Y=0.000000; SetTransform_1.Value.Translation.Z=0.000000; SetTransform_1.Value.Scale3D.X=1.000000; SetTransform_1.Value.Scale3D.Y=1.000000; SetTransform_1.Value.Scale3D.Z=1.000000; SetTransform_1.Weight=1.000000; SetTransform_1.bPropagateToChildren=True |
| `VariableNode_5` | Get Head IK Space<br/>Input parameter |  |
| `SequenceExecution` | 시퀀스 |  |

</details>

### Setup Reverse Foot

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `Entry` | Entry |  |
| `Return` | Return |  |
| `GetTransform_1` | Get Transform - Bone | GetTransform_1.Item.Type=Bone; GetTransform_1.Item.Name=None; GetTransform_1.Space=GlobalSpace |
| `SetTransform` | Set Transform - Bone Initial | SetTransform.ExecuteContext=(); SetTransform.Item.Type=Bone; SetTransform.Item.Name=None; SetTransform.Space=GlobalSpace; SetTransform.bInitial=true; SetTransform.Value.Translation.X=0.000000; SetTransform.Value.Translation.Y=0.000000; SetTransform.Value.Translation.Z=0.000000; SetTransform.Value.Scale3D.X=1.000000; SetTransform.Value.Scale3D.Y=1.000000; SetTransform.Value.Scale3D.Z=1.000000; SetTransform.Weight=1.000000; SetTransform.bPropagateToChildren=True |
| `ArrayIterator_3` |  |  |
| `ArrayGetAtIndex` |  |  |
| `VariableNode` | Get BK Controls<br/>Input parameter |  |
| `VariableNode_1` | Get Bones<br/>Input parameter |  |
| `VariableNode_1_1` | Get Bones<br/>Input parameter |  |
| `ArrayGetAtIndex_1` |  |  |
| `ArrayGetNum` |  |  |
| `MathIntSub` | Subtract | MathIntSub.B=1 |
| `VariableNode_2` | Get Roll Control<br/>Input parameter |  |
| `SetTransform_1` | Set Transform - Bone Initial | SetTransform_1.Item.Type=Bone; SetTransform_1.Item.Name=None; SetTransform_1.Space=GlobalSpace; SetTransform_1.bInitial=true; SetTransform_1.Value.Translation.X=0.000000; SetTransform_1.Value.Translation.Y=0.000000; SetTransform_1.Value.Translation.Z=0.000000; SetTransform_1.Value.Scale3D.X=1.000000; SetTransform_1.Value.Scale3D.Y=1.000000; SetTransform_1.Value.Scale3D.Z=1.000000; SetTransform_1.Weight=1.000000; SetTransform_1.bPropagateToChildren=True |
| `GetTransform` | Get Transform - Bone | GetTransform.Item.Type=Bone; GetTransform.Item.Name=None; GetTransform.Space=GlobalSpace |
| `ArrayGetAtIndex_2` |  |  |
| `VariableNode_4` | Get BK Buffers<br/>Input parameter |  |
| `SetTransform_2` | Set Transform - Bone Initial | SetTransform_2.Item.Type=Bone; SetTransform_2.Item.Name=None; SetTransform_2.Space=GlobalSpace; SetTransform_2.bInitial=true; SetTransform_2.Value.Translation.X=0.000000; SetTransform_2.Value.Translation.Y=0.000000; SetTransform_2.Value.Translation.Z=0.000000; SetTransform_2.Value.Scale3D.X=1.000000; SetTransform_2.Value.Scale3D.Y=1.000000; SetTransform_2.Value.Scale3D.Z=1.000000; SetTransform_2.Weight=1.000000; SetTransform_2.bPropagateToChildren=True |
| `VariableNode_1_2` | Get Bones<br/>Input parameter |  |
| `ArrayGetAtIndex_3` |  |  |
| `GetTransform_2` | Get Transform - Bone | GetTransform_2.Item.Type=Bone; GetTransform_2.Item.Name=None; GetTransform_2.Space=GlobalSpace |
| `If` |  |  |
| `VariableNode_3` | Get Negative Side<br/>Input parameter |  |
| `If_1` |  |  |
| `VariableNode_3_1` | Get Negative Side<br/>Input parameter |  |
| `SetTransform_1_1` | Set Transform - Bone Initial | SetTransform_1_1.Item.Type=Bone; SetTransform_1_1.Item.Name=None; SetTransform_1_1.Space=GlobalSpace; SetTransform_1_1.bInitial=true; SetTransform_1_1.Value.Translation.X=0.000000; SetTransform_1_1.Value.Translation.Y=0.000000; SetTransform_1_1.Value.Translation.Z=0.000000; SetTransform_1_1.Value.Scale3D.X=1.000000; SetTransform_1_1.Value.Scale3D.Y=1.000000; SetTransform_1_1.Value.Scale3D.Z=1.000000; SetTransform_1_1.Weight=1.000000; SetTransform_1_1.bPropagateToChildren=True |
| `VariableNode_5` | Get Roll Buffer<br/>Input parameter |  |
| `Get Negative Transform_1` | Get Negative Transform<br/>Local Function | Get Negative Transform_1.Input.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); Get Negative Transform_1.Input.Translation.X=0.000000; Get Negative Transform_1.Input.Translation.Y=0.000000; Get Negative Transform_1.Input.Translation.Z=0.000000; Get Negative Transform_1.Input.Scale3D.X=0.000000; Get Negative Transform_1.Input.Scale3D.Y=0.000000; Get Negative Transform_1.Input.Scale3D.Z=0.000000 |
| `SequenceExecution` | 시퀀스 |  |

</details>

### Setup Spine

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `Entry` | Entry |  |
| `Return` | Return |  |
| `ArrayIterator` |  |  |
| `ArrayGetAtIndex` |  |  |
| `SetTransform_28` | Set Transform - Bone Initial | SetTransform_28.ExecuteContext=(); SetTransform_28.Item.Type=Bone; SetTransform_28.Item.Name=None; SetTransform_28.Space=GlobalSpace; SetTransform_28.bInitial=true; SetTransform_28.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform_28.Value.Translation.X=0.000000; SetTransform_28.Value.Translation.Y=0.000000; SetTransform_28.Value.Translation.Z=0.000000; SetTransform_28.Value.Scale3D.X=1.000000; SetTransform_28.Value.Scale3D.Y=1.000000; SetTransform_28.Value.Scale3D.Z=1.000000; SetTransform_28.Weight=1.000000; SetTransform_28.bPropagateToChildren=false |
| `GetTransform_15` | Get Transform - Bone | GetTransform_15.Item.Type=Bone; GetTransform_15.Item.Name=None; GetTransform_15.Space=GlobalSpace |
| `VariableNode_1` | Get Bones<br/>Input parameter |  |
| `VariableNode_2` | Get FK Controls<br/>Input parameter |  |
| `VariableNode_3` | Get IK Controls<br/>Input parameter |  |
| `ArrayGetAtIndex_3` |  |  |
| `SetTransform_28_2` | Set Transform - Bone Initial | SetTransform_28_2.Item.Type=Bone; SetTransform_28_2.Item.Name=None; SetTransform_28_2.Space=GlobalSpace; SetTransform_28_2.bInitial=true; SetTransform_28_2.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform_28_2.Value.Translation.X=0.000000; SetTransform_28_2.Value.Translation.Y=0.000000; SetTransform_28_2.Value.Translation.Z=0.000000; SetTransform_28_2.Value.Scale3D.X=1.000000; SetTransform_28_2.Value.Scale3D.Y=1.000000; SetTransform_28_2.Value.Scale3D.Z=1.000000; SetTransform_28_2.Weight=1.000000; SetTransform_28_2.bPropagateToChildren=false |
| `Spline From Element Setup` | Spline From Element Setup<br/>Collapsed Graph |  |
| `VariableNode_3_1` | Get IK Controls<br/>Input parameter |  |
| `ArrayIterator_1` |  |  |
| `TransformFromControlRigSpline` | 스플라인에서 트랜스폼(업 벡터 포함) | TransformFromControlRigSpline.UpVector.X=0.000000; TransformFromControlRigSpline.UpVector.Y=1.000000; TransformFromControlRigSpline.UpVector.Z=0.000000 |
| `VariableNode_4` | Get Bones<br/>Input parameter |  |
| `GetTransform` | Get Transform - Bone | GetTransform.Item.Type=Bone; GetTransform.Item.Name=None; GetTransform.Space=GlobalSpace |
| `MathTransformMakeRelative` | Make Relative | MathTransformMakeRelative.Global.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); MathTransformMakeRelative.Global.Translation.X=0.000000; MathTransformMakeRelative.Global.Translation.Y=0.000000; MathTransformMakeRelative.Global.Translation.Z=0.000000; MathTransformMakeRelative.Global.Scale3D.X=1.000000; MathTransformMakeRelative.Global.Scale3D.Y=1.000000; MathTransformMakeRelative.Global.Scale3D.Z=1.000000; MathTransformMakeRelative.Parent.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); MathTransformMakeRelative.Parent.Translation.X=0.000000; MathTransformMakeRelative.Parent.Translation.Y=0.000000; MathTransformMakeRelative.Parent.Translation.Z=0.000000; MathTransformMakeRelative.Parent.Scale3D.X=1.000000; MathTransformMakeRelative.Parent.Scale3D.Y=1.000000; MathTransformMakeRelative.Parent.Scale3D.Z=1.000000 |
| `VariableNode_5` | Get IK Offsets<br/>Local Variable<br/>Default () |  |
| `ArrayReset` |  |  |
| `ArrayAdd` |  |  |
| `VariableNode_5_1` | Get IK Offsets<br/>Local Variable<br/>Default () |  |
| `VariableNode_5_1_1` | Get IK Offsets<br/>Local Variable<br/>Default () |  |
| `VariableNode` | Get Chest Space<br/>Input parameter |  |
| `SetTransform` | Set Transform - Bone Initial | SetTransform.Item.Type=Bone; SetTransform.Item.Name=None; SetTransform.Space=GlobalSpace; SetTransform.bInitial=true; SetTransform.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform.Value.Translation.X=0.000000; SetTransform.Value.Translation.Y=0.000000; SetTransform.Value.Translation.Z=0.000000; SetTransform.Value.Scale3D.X=1.000000; SetTransform.Value.Scale3D.Y=1.000000; SetTransform.Value.Scale3D.Z=1.000000; SetTransform.Weight=1.000000; SetTransform.bPropagateToChildren=false |
| `VariableNode_4_1` | Get Bones<br/>Input parameter |  |
| `ArrayGetNum` |  |  |
| `ArrayGetAtIndex_1` |  |  |
| `MathIntSub` | Subtract | MathIntSub.B=1 |
| `GetTransform_1` | Get Transform - Bone | GetTransform_1.Item.Type=Bone; GetTransform_1.Item.Name=None; GetTransform_1.Space=GlobalSpace |
| `EdGraphNode_Comment_1` | Spline Offsets |  |
| `EdGraphNode_Comment_2` | Controls |  |
| `EdGraphNode_Comment_3` | Chest Space |  |
| `GetTransformItemArray` | Get Transform Array | GetTransformItemArray.Space=GlobalSpace |
| `ArrayGetAtIndex_4` |  |  |
| `ArrayGetAtIndex_4_1` |  |  |
| `ArrayGetAtIndex_4_2` |  |  |
| `ArrayGetAtIndex_4_3` |  |  |
| `VariableNode_2_1` | Get Bones<br/>Input parameter |  |
| `BSpline Passing thru 4 Points_1` | BSpline Passing thru 4 Points<br/>Local Function | BSpline Passing thru 4 Points_1.Point 0.X=0.000000; BSpline Passing thru 4 Points_1.Point 0.Y=0.000000; BSpline Passing thru 4 Points_1.Point 0.Z=0.000000; BSpline Passing thru 4 Points_1.Point 1.X=0.000000; BSpline Passing thru 4 Points_1.Point 1.Y=0.000000; BSpline Passing thru 4 Points_1.Point 1.Z=0.000000; BSpline Passing thru 4 Points_1.Point 2.X=0.000000; BSpline Passing thru 4 Points_1.Point 2.Y=0.000000; BSpline Passing thru 4 Points_1.Point 2.Z=0.000000; BSpline Passing thru 4 Points_1.Point 3.X=0.000000; BSpline Passing thru 4 Points_1.Point 3.Y=0.000000; BSpline Passing thru 4 Points_1.Point 3.Z=0.000000; BSpline Passing thru 4 Points_1.Auto Weights=False; BSpline Passing thru 4 Points_1.Weight 1=0.333000; BSpline Passing thru 4 Points_1.Weight 2=0.666000 |
| `ArrayGetAtIndex_2` |  |  |
| `GetLengthControlRigSpline` | Get Length Of Spline |  |
| `SequenceExecution` | 시퀀스 |  |

</details>

### Spine Match IK onto FK

<details>
<summary>전체 노드·기본 입력 목록</summary>

| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |
|---|---|---|
| `Entry` | Entry |  |
| `Return` | Return |  |
| `ArrayIterator_2_1` |  |  |
| `ArrayGetAtIndex_3_1` |  |  |
| `SetTransform_3_1` | Set Transform - Bone | SetTransform_3_1.ExecuteContext=(); SetTransform_3_1.Item.Type=Bone; SetTransform_3_1.Item.Name=None; SetTransform_3_1.Space=GlobalSpace; SetTransform_3_1.Value.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); SetTransform_3_1.Value.Translation.X=0.000000; SetTransform_3_1.Value.Translation.Y=0.000000; SetTransform_3_1.Value.Translation.Z=0.000000; SetTransform_3_1.Value.Scale3D.X=1.000000; SetTransform_3_1.Value.Scale3D.Y=1.000000; SetTransform_3_1.Value.Scale3D.Z=1.000000; SetTransform_3_1.Weight=1.000000; SetTransform_3_1.bPropagateToChildren=false |
| `Branch_1_1` |  |  |
| `ProjectTransformToNewParent_1` | Project to new Parent | ProjectTransformToNewParent_1.Child.Type=Bone; ProjectTransformToNewParent_1.Child.Name=None; ProjectTransformToNewParent_1.bChildInitial=True; ProjectTransformToNewParent_1.OldParent.Type=Bone; ProjectTransformToNewParent_1.OldParent.Name=None; ProjectTransformToNewParent_1.bOldParentInitial=True; ProjectTransformToNewParent_1.NewParent.Type=Bone; ProjectTransformToNewParent_1.NewParent.Name=None |
| `GetTransformItemArray` | Get Transform Array | GetTransformItemArray.Space=GlobalSpace |
| `ArrayGetAtIndex_4` |  |  |
| `ArrayGetAtIndex_4_1` |  |  |
| `ArrayGetAtIndex_4_2` |  |  |
| `ArrayGetAtIndex_4_3` |  |  |
| `ArrayGetAtIndex_8` |  |  |
| `VariableNode` | Get IK Controls<br/>Input parameter |  |
| `VariableNode_1` | Get Bones<br/>Input parameter |  |
| `VariableNode_2` | Get Bones<br/>Input parameter |  |
| `MathTransformMakeAbsolute` | Make Absolute | MathTransformMakeAbsolute.Local.Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000); MathTransformMakeAbsolute.Local.Translation.X=0.000000; MathTransformMakeAbsolute.Local.Translation.Y=0.000000; MathTransformMakeAbsolute.Local.Translation.Z=0.000000; MathTransformMakeAbsolute.Local.Scale3D.X=1.000000; MathTransformMakeAbsolute.Local.Scale3D.Y=1.000000; MathTransformMakeAbsolute.Local.Scale3D.Z=1.000000; MathTransformMakeAbsolute.Parent.Translation.X=0.000000; MathTransformMakeAbsolute.Parent.Translation.Y=0.000000; MathTransformMakeAbsolute.Parent.Translation.Z=0.000000; MathTransformMakeAbsolute.Parent.Scale3D.X=1.000000; MathTransformMakeAbsolute.Parent.Scale3D.Y=1.000000; MathTransformMakeAbsolute.Parent.Scale3D.Z=1.000000 |
| `VariableNode_3` | Get Offsets<br/>Input parameter |  |
| `ArrayGetAtIndex` |  |  |
| `BSpline Passing thru 4 Points_1` | BSpline Passing thru 4 Points<br/>Local Function | BSpline Passing thru 4 Points_1.Point 0.X=0.000000; BSpline Passing thru 4 Points_1.Point 0.Y=0.000000; BSpline Passing thru 4 Points_1.Point 0.Z=0.000000; BSpline Passing thru 4 Points_1.Point 1.X=0.000000; BSpline Passing thru 4 Points_1.Point 1.Y=0.000000; BSpline Passing thru 4 Points_1.Point 1.Z=0.000000; BSpline Passing thru 4 Points_1.Point 2.X=0.000000; BSpline Passing thru 4 Points_1.Point 2.Y=0.000000; BSpline Passing thru 4 Points_1.Point 2.Z=0.000000; BSpline Passing thru 4 Points_1.Point 3.X=0.000000; BSpline Passing thru 4 Points_1.Point 3.Y=0.000000; BSpline Passing thru 4 Points_1.Point 3.Z=0.000000; BSpline Passing thru 4 Points_1.Auto Weights=false; BSpline Passing thru 4 Points_1.Weight 1=0.330000; BSpline Passing thru 4 Points_1.Weight 2=0.660000 |
| `SequenceExecution` | 시퀀스 |  |
| `MathIntEquals_1` | Equals | MathIntEquals_1.A=0; MathIntEquals_1.B=0 |

</details>

## 조회 범위의 한계

ObjectIterator로 발견했으나 MCP helper가 반환하지 않은 그래프입니다. delegate signature·컴파일된 중간 그래프·macro·애니메이션 내부 그래프가 섞일 수 있습니다.

- `RigVMFunctionLibraryEdGraph`: Graph 'RigVMFunctionLibraryEdGraph' not found in Blueprint.

[원본 MCP 스냅샷](../../../../Evidence/blueprints-351.json)
