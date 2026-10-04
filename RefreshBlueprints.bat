@echo off
setlocal

REM Refresh and resave Blueprints under /Game/QuakeLike_1_0 without opening
REM the Unreal Editor window. Resolve the engine from ShootingArena.uproject,
REM using the same association configured by "Switch Unreal Engine Version".

set "PROJECT=%~dp0ShootingArena.uproject"

if not exist "%PROJECT%" (
    echo [ERROR] ShootingArena.uproject was not found beside this BAT file.
    pause
    exit /b 1
)

echo Resolving the Unreal Engine selected for this project...
for /f "usebackq delims=" %%E in (`powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0ResolveUnrealEngine.ps1" -ProjectPath "%PROJECT%"`) do set "ENGINE_ROOT=%%E"

if not defined ENGINE_ROOT (
    echo [ERROR] Could not find the engine selected for this project.
    echo On this PC, right-click ShootingArena.uproject and choose
    echo "Switch Unreal Engine Version", select the intended engine, and retry.
    pause
    exit /b 1
)

set "BUILD_BAT=%ENGINE_ROOT%\Engine\Build\BatchFiles\Build.bat"
set "EDITOR_CMD=%ENGINE_ROOT%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"

if not exist "%BUILD_BAT%" (
    echo [ERROR] Build.bat was not found: %BUILD_BAT%
    pause
    exit /b 1
)
if not exist "%EDITOR_CMD%" (
    echo [ERROR] UnrealEditor-Cmd.exe was not found: %EDITOR_CMD%
    pause
    exit /b 1
)

echo ============================================================================
echo ShootingArena Blueprint Refresh
echo Engine: %ENGINE_ROOT%
echo Project: %PROJECT%
echo Folder: /Game/QuakeLike_1_0
echo ============================================================================
echo.
echo [1/2] Building the editor target...
call "%BUILD_BAT%" ShootingArenaEditor Win64 Development "%PROJECT%" -WaitMutex
if errorlevel 1 (
    echo [FAILED] Editor target build failed.
    pause
    exit /b 1
)

echo.
echo [2/2] Refreshing and resaving Blueprints in unattended mode...
"%EDITOR_CMD%" "%PROJECT%" -unattended -nop4 -nosplash -NullRHI -NoSound -UTF8Output -log -ExecCmds="ShootingArena.RefreshBlueprints /Game/QuakeLike_1_0 resaveall,Quit"
set "RESULT=%ERRORLEVEL%"

echo.
if "%RESULT%"=="0" (
    echo [DONE] Check the log above and Git status for the resaved assets.
) else (
    echo [FAILED] UnrealEditor-Cmd.exe returned exit code %RESULT%.
)
pause
exit /b %RESULT%
