#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MultiRulesLibrary.generated.h"

/**
 * DT_MultiRules(/Game/QuakeLike_1_0/GameFlow/Table/DT/DT_MultiRules) 의 멀티 매치 규칙을 읽는 헬퍼입니다.
 * 행 이름: "TimeLimit"(제한 시간, 분), "KillTarget"(목표 킬 수).
 * 각 행의 Options 는 선택지 목록, DefaultIndex 는 그 목록에서 기본으로 고르는 "인덱스"입니다
 * (예: TimeLimit Options=(1,5,10,15), DefaultIndex=1 -> 5분).
 * 행 구조체(ST_MultiRules)가 블루프린트 구조체라서 C++ 구조체 대신 리플렉션으로 필드를 읽습니다.
 */
UCLASS()
class SHOOTINGARENA_API UMultiRulesLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static const FName TimeLimitRowName;
	static const FName KillTargetRowName;

	// 규칙 행의 선택지 개수. 테이블/행이 없으면 0.
	UFUNCTION(BlueprintPure, Category = "MultiRules")
	static int32 GetRuleOptionCount(FName RuleRowName);

	// 규칙 행의 DefaultIndex. 테이블/행이 없거나 범위를 벗어나면 0.
	UFUNCTION(BlueprintPure, Category = "MultiRules")
	static int32 GetRuleDefaultIndex(FName RuleRowName);

	// 규칙 행의 선택지 값 목록 (예: 1,5,10,15).
	UFUNCTION(BlueprintPure, Category = "MultiRules")
	static TArray<int32> GetRuleOptions(FName RuleRowName);

	// Index 번째 선택지의 값. 범위를 벗어나면 DefaultIndex 의 값, 그것도 없으면 0.
	UFUNCTION(BlueprintPure, Category = "MultiRules")
	static int32 GetRuleValue(FName RuleRowName, int32 Index);

	// DefaultIndex 가 가리키는 값 (제한 시간이면 분, 킬 조건이면 킬 수).
	UFUNCTION(BlueprintPure, Category = "MultiRules")
	static int32 GetRuleDefaultValue(FName RuleRowName);

	// 규칙 행의 UI 표시용 Label / Unit (예: "제한 시간" / "분").
	UFUNCTION(BlueprintPure, Category = "MultiRules")
	static FText GetRuleLabel(FName RuleRowName);

	UFUNCTION(BlueprintPure, Category = "MultiRules")
	static FText GetRuleUnit(FName RuleRowName);

	// 값이 선택지 목록의 몇 번째인지. 없으면 INDEX_NONE.
	UFUNCTION(BlueprintPure, Category = "MultiRules")
	static int32 FindRuleOptionIndex(FName RuleRowName, int32 Value);
};
