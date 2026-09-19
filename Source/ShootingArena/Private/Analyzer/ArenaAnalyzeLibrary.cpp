#include "Analyzer/ArenaAnalyzeLibrary.h"

#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"

#include "Misc/Paths.h"
#include "HAL/FileManager.h"



bool UArenaAnalyzeLibrary::LoadAnalyzeFile(const FString& FilePath, FArenaAnalyzeSession& OutSession)
{
    FString JsonString;

    if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
    {
        return false;
    }

    return FJsonObjectConverter::JsonObjectStringToUStruct(
        JsonString,
        &OutSession
    );
}


TArray<FString> UArenaAnalyzeLibrary::GetAnalyzeFiles()
{
    TArray<FString> Files;

    const FString Directory =
        FPaths::ProjectSavedDir() / TEXT("Analyze");

    IFileManager::Get().FindFiles(
        Files,
        *(Directory / TEXT("*.json")),
        true,
        false
    );

    Files.Sort([](const FString& A, const FString& B)
        {
            return A > B;
        });

    return Files;
}


bool UArenaAnalyzeLibrary::LoadAnalyzeFileByName(
    const FString& FileName,
    FArenaAnalyzeSession& OutSession)
{
    const FString FilePath =
        FPaths::ProjectSavedDir()
        / TEXT("Analyze")
        / FileName;

    return LoadAnalyzeFile(FilePath, OutSession);
}

TArray<FArenaLineSegment> UArenaAnalyzeLibrary::BuildMovementLineSegments(
    const TArray<FArenaMovementSample>& MovementSamples)
{
    TArray<FArenaMovementSample> SortedSamples = MovementSamples;
    SortedSamples.Sort([](const FArenaMovementSample& A, const FArenaMovementSample& B)
    {
        if (A.ControllerId != B.ControllerId)
        {
            return A.ControllerId < B.ControllerId;
        }
        if (A.TrackId != B.TrackId)
        {
            return A.TrackId < B.TrackId;
        }
        return A.TimeSeconds < B.TimeSeconds;
    });

    TArray<FArenaLineSegment> Segments;
    Segments.Reserve(FMath::Max(0, SortedSamples.Num() - 1));
    for (int32 Index = 1; Index < SortedSamples.Num(); ++Index)
    {
        const FArenaMovementSample& Previous = SortedSamples[Index - 1];
        const FArenaMovementSample& Current = SortedSamples[Index];
        if (Previous.ControllerId != Current.ControllerId || Previous.TrackId != Current.TrackId)
        {
            continue;
        }

        FArenaLineSegment& Segment = Segments.AddDefaulted_GetRef();
        Segment.StartLocation = Previous.Location;
        Segment.EndLocation = Current.Location;
        Segment.ControllerId = Current.ControllerId;
        Segment.TrackId = Current.TrackId;
    }
    return Segments;
}

TArray<FArenaMovementSample> UArenaAnalyzeLibrary::FilterMovementSamples(
    const TArray<FArenaMovementSample>& MovementSamples,
    int32 ControllerId, int32 TrackId, double StartTime, double EndTime)
{
    TArray<FArenaMovementSample> Result;
    for (const FArenaMovementSample& Sample : MovementSamples)
    {
        if ((ControllerId == INDEX_NONE || Sample.ControllerId == ControllerId) &&
            (TrackId == INDEX_NONE || Sample.TrackId == TrackId) &&
            Sample.TimeSeconds >= StartTime &&
            (EndTime < 0.0 || Sample.TimeSeconds <= EndTime))
        {
            Result.Add(Sample);
        }
    }
    return Result;
}

TArray<FArenaDamageSample> UArenaAnalyzeLibrary::FilterDamageSamples(
    const TArray<FArenaDamageSample>& DamageSamples,
    int32 DamagedControllerId, int32 DamagedTrackId,
    int32 InstigatorControllerId, int32 InstigatorTrackId,
    double StartTime, double EndTime)
{
    TArray<FArenaDamageSample> Result;
    for (const FArenaDamageSample& Sample : DamageSamples)
    {
        if ((DamagedControllerId == INDEX_NONE || Sample.DamagedControllerId == DamagedControllerId) &&
            (DamagedTrackId == INDEX_NONE || Sample.DamagedTrackId == DamagedTrackId) &&
            (InstigatorControllerId == INDEX_NONE || Sample.InstigatorControllerId == InstigatorControllerId) &&
            (InstigatorTrackId == INDEX_NONE || Sample.InstigatorTrackId == InstigatorTrackId) &&
            Sample.TimeSeconds >= StartTime &&
            (EndTime < 0.0 || Sample.TimeSeconds <= EndTime))
        {
            Result.Add(Sample);
        }
    }
    return Result;
}
