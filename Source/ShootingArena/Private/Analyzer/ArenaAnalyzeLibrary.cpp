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