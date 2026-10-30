#include "Analyzer/ArenaLoggingSettings.h"

#include "Analyzer/ArenaLoggingDataAsset.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UObject/UObjectGlobals.h"

static TAutoConsoleVariable<int32> CVarArenaLogging(
	TEXT("arena.Logging"), 0,
	TEXT("1 records matches into the analyzer JSON and the match log database; 0 (default) records nothing.\n")
	TEXT("Applied when a world is created. The -ArenaLogging[=0|1] command-line switch overrides it."),
	ECVF_Default);

bool IsArenaLoggingEnabled()
{
	bool bFromCommandLine = false;
	if (FParse::Bool(FCommandLine::Get(), TEXT("ArenaLogging="), bFromCommandLine))
	{
		return bFromCommandLine;
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("ArenaLogging")))
	{
		return true;
	}
	if (CVarArenaLogging.GetValueOnAnyThread() != 0)
	{
		return true;
	}
	// DA_ArenaLogging: every process (editor, PIE dedicated server, standalone) loads the same asset from Content.
	// A missing asset counts as off, without a load warning.
	const UArenaLoggingDataAsset* Asset = LoadObject<UArenaLoggingDataAsset>(
		nullptr, UArenaLoggingDataAsset::GetAssetPath(), nullptr, LOAD_NoWarn | LOAD_Quiet);
	return Asset && Asset->bLoggingEnabled;
}
