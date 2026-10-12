# StandardMacros

에셋: `/Game/EditorBlueprintResources/StandardMacros`

부모: `/Script/CoreUObject.Class'/Script/CoreUObject.Object'`

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

- `Can Execute Cosmetic Events`: Graph 'Can Execute Cosmetic Events' not found in Blueprint.
- `CompareFloat`: Graph 'CompareFloat' not found in Blueprint.
- `CompareInt`: Graph 'CompareInt' not found in Blueprint.
- `Create and Assign MID`: Graph 'Create and Assign MID' not found in Blueprint.
- `CreateTransaction`: Graph 'CreateTransaction' not found in Blueprint.
- `DecrementFloat`: Graph 'DecrementFloat' not found in Blueprint.
- `DecrementInt`: Graph 'DecrementInt' not found in Blueprint.
- `Do N`: Graph 'Do N' not found in Blueprint.
- `DoOnce`: Graph 'DoOnce' not found in Blueprint.
- `FlipFlop`: Graph 'FlipFlop' not found in Blueprint.
- `ForEachLoop`: Graph 'ForEachLoop' not found in Blueprint.
- `ForEachLoopWithBreak`: Graph 'ForEachLoopWithBreak' not found in Blueprint.
- `ForLoop`: Graph 'ForLoop' not found in Blueprint.
- `ForLoopWithBreak`: Graph 'ForLoopWithBreak' not found in Blueprint.
- `Gate`: Graph 'Gate' not found in Blueprint.
- `IncrementFloat`: Graph 'IncrementFloat' not found in Blueprint.
- `IncrementInt`: Graph 'IncrementInt' not found in Blueprint.
- `IsValid`: Graph 'IsValid' not found in Blueprint.
- `ManipulateFloatInternal`: Graph 'ManipulateFloatInternal' not found in Blueprint.
- `ManipulateIntInternal`: Graph 'ManipulateIntInternal' not found in Blueprint.
- `NegateFloat`: Graph 'NegateFloat' not found in Blueprint.
- `NegateInt`: Graph 'NegateInt' not found in Blueprint.
- `ReverseForEachLoop`: Graph 'ReverseForEachLoop' not found in Blueprint.
- `WhileLoop`: Graph 'WhileLoop' not found in Blueprint.

[원본 MCP 스냅샷](../../Evidence/blueprints-281.json)
