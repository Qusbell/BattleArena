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

    FArenaAnalyzeSession Loaded;
    if (!FJsonObjectConverter::JsonObjectStringToUStruct(JsonString, &Loaded) || Loaded.SchemaVersion > 2) return false;
    if (!Loaded.SessionId.IsEmpty() && Loaded.SchemaVersion == 2)
    {
        TArray<FString> Chunks;
        IFileManager::Get().FindFiles(Chunks, *(FPaths::GetPath(FilePath) / (TEXT("ArenaAnalyze_") + Loaded.SessionId + TEXT("_*.json"))), true, false);
        Chunks.Sort();
        if (!Chunks.IsEmpty())
        {
            FArenaAnalyzeSession Merged;
            Merged.SchemaVersion = 2; Merged.SessionId = Loaded.SessionId;
            int32 ExpectedChunk = 0;
            for (const FString& Chunk : Chunks)
            {
                FString Text; FArenaAnalyzeSession Part;
                if (!FFileHelper::LoadFileToString(Text, *(FPaths::GetPath(FilePath) / Chunk))
                    || !FJsonObjectConverter::JsonObjectStringToUStruct(Text, &Part)
                    || Part.SchemaVersion != 2 || Part.SessionId != Loaded.SessionId || Part.ChunkIndex != ExpectedChunk++) return false;
                Merged.MapName = Part.MapName; Merged.SampleInterval = Part.SampleInterval;
                Merged.SessionDuration = FMath::Max(Merged.SessionDuration, Part.SessionDuration);
                Merged.Participants = Part.Participants;
                Merged.MovementSamples.Append(Part.MovementSamples); Merged.DamageSamples.Append(Part.DamageSamples);
                Merged.ShotSamples.Append(Part.ShotSamples); Merged.HitSamples.Append(Part.HitSamples); Merged.DeathSamples.Append(Part.DeathSamples);
            }
            Loaded = MoveTemp(Merged);
        }
    }
    OutSession = MoveTemp(Loaded);
    return true;
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


FArenaAnalyzeSession UArenaAnalyzeLibrary::FilterSession(const FArenaAnalyzeSession& S, const FString& Weapon,
    int32 Kind, int32 ControllerId, int32 TrackId, double Start, double End)
{
    FArenaAnalyzeSession R = S;
    R.MovementSamples.Reset(); R.DamageSamples.Reset(); R.ShotSamples.Reset(); R.HitSamples.Reset(); R.DeathSamples.Reset();
    TSet<int32> Actors;
    for (const auto& P : S.Participants) if (Kind == 0 || (Kind == 1 && P.bPlayer) || (Kind == 2 && !P.bPlayer)) Actors.Add(P.ControllerId);
    auto Actor = [&](int32 C, int32 T) { return (Kind == 0 || Actors.Contains(C)) && (ControllerId < 0 || C == ControllerId) && (TrackId < 0 || T == TrackId); };
    auto Time = [&](double T) { return T >= Start && (End < 0 || T <= End); };
    TSet<int32> Shots;
    for (const auto& Shot : S.ShotSamples)
        if ((Weapon.IsEmpty() || Shot.WeaponId == Weapon) && Actor(Shot.ControllerId, Shot.TrackId))
        { Shots.Add(Shot.ShotId); if (Time(Shot.TimeSeconds)) R.ShotSamples.Add(Shot); }
    // Movement has no weapon identity; the weapon filter affects combat records.
    for (const auto& M : S.MovementSamples) if (Actor(M.ControllerId, M.TrackId) && Time(M.TimeSeconds)) R.MovementSamples.Add(M);
    for (const auto& H : S.HitSamples) if (Shots.Contains(H.ShotId) && Time(H.TimeSeconds)) R.HitSamples.Add(H);
    for (const auto& D : S.DamageSamples)
        if (Time(D.TimeSeconds) && (Weapon.IsEmpty() || D.WeaponId == Weapon)
            && (Actor(D.DamagedControllerId, D.DamagedTrackId) || Actor(D.InstigatorControllerId, D.InstigatorTrackId))) R.DamageSamples.Add(D);
    for (const auto& D : S.DeathSamples)
        if (Time(D.TimeSeconds) && (Weapon.IsEmpty() || D.WeaponId == Weapon)
            && (Actor(D.VictimControllerId, D.VictimTrackId) || Actor(D.KillerControllerId, D.KillerTrackId))) R.DeathSamples.Add(D);
    return R;
}
