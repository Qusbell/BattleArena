#include "Analyzer/ArenaAnalyzeLibrary.h"

#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"


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