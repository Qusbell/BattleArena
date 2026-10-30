#pragma once

#include "CoreMinimal.h"

/**
 * Master switch shared by both recording subsystems (UArenaAnalyzeSubsystem, UArenaMatchLogSubsystem). Off by default.
 * It is read when a world is created, so a change applies from the next world on, not to a match already running.
 *
 * Turn it on with any of:
 *   - command line: -ArenaLogging  (or -ArenaLogging=1 / =0; the command line wins over the console variable)
 *   - console variable: arena.Logging 1  (also settable under [ConsoleVariables] in an ini file)
 */
SHOOTINGARENA_API bool IsArenaLoggingEnabled();
