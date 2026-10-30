#pragma once

#include "CoreMinimal.h"

/**
 * Master switch shared by both recording subsystems (UArenaAnalyzeSubsystem, UArenaMatchLogSubsystem). Off by default.
 * It is read when a world is created, so a change applies from the next world on, not to a match already running.
 *
 * Turn it on with any of:
 *   - Data Asset: tick "Logging Enabled" in /Game/QuakeLike_1_0/Data/DA_ArenaLogging (see ArenaLoggingDataAsset.h)
 *   - command line: -ArenaLogging  (or -ArenaLogging=1 / =0; the command line wins over everything else)
 *   - console variable: arena.Logging 1  (also settable under [ConsoleVariables] in an ini file)
 */
SHOOTINGARENA_API bool IsArenaLoggingEnabled();
