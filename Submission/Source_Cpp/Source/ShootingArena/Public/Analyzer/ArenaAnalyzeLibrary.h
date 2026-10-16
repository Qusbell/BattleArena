#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Analyzer/ArenaAnalyzeSubsystem.h"

#include "ArenaAnalyzeLibrary.generated.h"


UCLASS()
class SHOOTINGARENA_API UArenaAnalyzeLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	

public:
	UFUNCTION(BlueprintCallable, Category = "Arena Analyze")
	static bool LoadAnalyzeFile(
		const FString& FilePath,
		FArenaAnalyzeSession& OutSession
	);

	UFUNCTION(BlueprintCallable, Category = "Arena Analyze")
	static TArray<FString> GetAnalyzeFiles();

	UFUNCTION(BlueprintCallable, Category = "Arena Analyze")
	static bool LoadAnalyzeFileByName(
		const FString& FileName,
		FArenaAnalyzeSession& OutSession
	);

	UFUNCTION(BlueprintCallable, Category = "Arena Analyze")
	static TArray<FArenaLineSegment> BuildMovementLineSegments(
		const TArray<FArenaMovementSample>& MovementSamples);

	// INDEX_NONE disables an ID filter; a negative EndTime disables the upper time bound.
	UFUNCTION(BlueprintCallable, Category = "Arena Analyze")
	static TArray<FArenaMovementSample> FilterMovementSamples(
		const TArray<FArenaMovementSample>& MovementSamples,
		int32 ControllerId,
		int32 TrackId,
		double StartTime,
		double EndTime);

	UFUNCTION(BlueprintCallable, Category = "Arena Analyze")
	static TArray<FArenaDamageSample> FilterDamageSamples(
		const TArray<FArenaDamageSample>& DamageSamples,
		int32 DamagedControllerId,
		int32 DamagedTrackId,
		int32 InstigatorControllerId,
		int32 InstigatorTrackId,
		double StartTime,
		double EndTime);
};
