#include "LanguageAuthoringLibrary.h"
#include "Localization/LanguageSubsystem.h"
#include "Internationalization/StringTable.h"
#include "Internationalization/StringTableCore.h"
#include "Internationalization/TextLocalizationResource.h"
#include "Internationalization/Internationalization.h"
#include "Serialization/BufferArchive.h"

bool ULanguageAuthoringLibrary::BuildLocalizationResource(const FString& Culture,
    const TMap<FString,FString>& Translations, TArray<uint8>& ResourceData)
{
    ResourceData.Reset();
    const FCulturePtr Language = FInternationalization::Get().GetCulture(Culture);
    if (!Language.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[Language] Invalid culture: %s"), *Culture);
        return false;
    }
    UStringTable* Asset=LoadObject<UStringTable>(nullptr,ULanguageLibrary::StringTablePath());if(!Asset)return false;
    bool Valid=true;FTextLocalizationResource Resource;
    Asset->GetStringTable()->EnumerateSourceStrings([&](const FTextKey& Key,const FString& Source)
    {
        const FString* Translation=Translations.Find(Key.ToString());
        if (!Translation || Translation->IsEmpty()) { UE_LOG(LogTemp,Error,TEXT("[Language] Missing %s translation: %s"),*Culture,*Key.ToString());Valid=false; }
        else
        {
            const FTextFormat SourcePattern(FText::AsCultureInvariant(Source));
            const FTextFormat TranslationPattern(FText::AsCultureInvariant(*Translation));
            TArray<FString> SourceArgs, TranslationArgs, PatternErrors;
            SourcePattern.GetFormatArgumentNames(SourceArgs);
            TranslationPattern.GetFormatArgumentNames(TranslationArgs);
            SourceArgs.Sort(); TranslationArgs.Sort();
            if (!SourcePattern.ValidatePattern(Language, PatternErrors)
                || !TranslationPattern.ValidatePattern(Language, PatternErrors) || SourceArgs != TranslationArgs)
            {
                UE_LOG(LogTemp, Error, TEXT("[Language] Invalid format or differing arguments in %s/%s"), *Culture, *Key.ToString());
                Valid = false;
            }
            else Resource.AddEntry(TEXT("Game"),Key,Source,*Translation,0);
        }
        return true;
    });
    if (!Valid) return false;
    FBufferArchive Writer;
    if (!Resource.SaveToArchive(Writer, FTextKey(TEXT("Game"))) || Writer.IsError())
    {
        UE_LOG(LogTemp, Error, TEXT("[Language] Could not build %s localization resource."), *Culture);
        return false;
    }
    ResourceData.Append(Writer.GetData(), Writer.Num());
    return true;
}
