# BP_CSH_Widget_Macros

에셋: `/Game/CompShooterHUD/Blueprints/Libraries/BP_CSH_Widget_Macros`

부모: `/Script/CoreUObject.Class'/Script/UMG.UserWidget'`

수집일: 2026-10-12. MCP에서 읽은 그래프·핀 연결의 자동 전사입니다. 상속된 그래프는 부모 문서를 함께 확인합니다.

정적 루트 참조 도달: 있음. 실행 여부 또는 삭제 가능 여부의 확정이 아닙니다.

컴포넌트 목록은 이 BP가 직접 정의한 SCS만 포함합니다. 부모·네이티브 생성 컴포넌트, 인스턴스 설정은 별도입니다.

## 그래프 목록

| 그래프 | 전체 노드 수 |
|---|---|

## 그래프별 실행 연결

실선은 출력 exec 핀의 실제 연결입니다. 여러 분기는 출력 핀 이름을 표시합니다. 함수 호출·delegate·타이머의 다음 그래프 실행은 이 그림에서 자동으로 이어 붙이지 않습니다.

순수 계산 노드 및 데이터 핀은 아래 목록과 원본 JSON을 확인합니다. 연결되지 않은 이벤트도 표시되므로, 노드가 있다는 이유만으로 실행된다고 판단하면 안 됩니다.

## 조회 범위의 한계

ObjectIterator로 발견했으나 MCP helper가 반환하지 않은 그래프입니다. delegate signature·컴파일된 중간 그래프·macro·애니메이션 내부 그래프가 섞일 수 있습니다.

- `AnimHandler`: Graph 'AnimHandler' not found in Blueprint.
- `ForEachLoopWithDelay`: Graph 'ForEachLoopWithDelay' not found in Blueprint.
- `GetFillImage`: Graph 'GetFillImage' not found in Blueprint.
- `GetSize`: Graph 'GetSize' not found in Blueprint.
- `GetTexture2DSize`: Graph 'GetTexture2DSize' not found in Blueprint.
- `GetTexture2DSizeClamped`: Graph 'GetTexture2DSizeClamped' not found in Blueprint.
- `OnceBranch`: Graph 'OnceBranch' not found in Blueprint.
- `OnceBranchDelayed`: Graph 'OnceBranchDelayed' not found in Blueprint.
- `OnceBranchWithReset`: Graph 'OnceBranchWithReset' not found in Blueprint.
- `ReverseForEachLoopWithBreak`: Graph 'ReverseForEachLoopWithBreak' not found in Blueprint.
- `ReversedBranch`: Graph 'ReversedBranch' not found in Blueprint.
- `SetBackgroundTint`: Graph 'SetBackgroundTint' not found in Blueprint.
- `SetBorderBrush`: Graph 'SetBorderBrush' not found in Blueprint.
- `SetBorderBrushSize`: Graph 'SetBorderBrushSize' not found in Blueprint.
- `SetBorderBrushTiling`: Graph 'SetBorderBrushTiling' not found in Blueprint.
- `SetFillImage`: Graph 'SetFillImage' not found in Blueprint.
- `SetFillImageDrawAs`: Graph 'SetFillImageDrawAs' not found in Blueprint.
- `SetFillImageMargin`: Graph 'SetFillImageMargin' not found in Blueprint.
- `SetFillImageSize`: Graph 'SetFillImageSize' not found in Blueprint.
- `SetFillImageTiling`: Graph 'SetFillImageTiling' not found in Blueprint.
- `SetImageBrush`: Graph 'SetImageBrush' not found in Blueprint.
- `SetImageBrushSize`: Graph 'SetImageBrushSize' not found in Blueprint.
- `SetImageBrushTiling`: Graph 'SetImageBrushTiling' not found in Blueprint.
- `SetMarqueeDrawAs`: Graph 'SetMarqueeDrawAs' not found in Blueprint.
- `SetMarqueeImage`: Graph 'SetMarqueeImage' not found in Blueprint.
- `SetMarqueeImageSize`: Graph 'SetMarqueeImageSize' not found in Blueprint.
- `SetMarqueeMargin`: Graph 'SetMarqueeMargin' not found in Blueprint.
- `SetMarqueeMirroring`: Graph 'SetMarqueeMirroring' not found in Blueprint.
- `SetMarqueeTiling`: Graph 'SetMarqueeTiling' not found in Blueprint.
- `SetMarqueeTint`: Graph 'SetMarqueeTint' not found in Blueprint.
- `SetSize`: Graph 'SetSize' not found in Blueprint.
- `SubtractBrightnessValue`: Graph 'SubtractBrightnessValue' not found in Blueprint.
- `SubtractFromColor`: Graph 'SubtractFromColor' not found in Blueprint.

[원본 MCP 스냅샷](../../../../Evidence/blueprints-281.json)
