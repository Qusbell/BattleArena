@echo off
setlocal EnableExtensions EnableDelayedExpansion

REM ============================================================================
REM ShootingArena portable build verification
REM Regenerates Visual Studio project files, then builds:
REM   - Development
REM   - Development Editor
REM Keep this file beside ShootingArena.uproject for team-wide portable use.
REM ============================================================================

REM Optional override: absolute path or path relative to this project directory.
REM Example: set "SHOOTINGARENA_ENGINE_ROOT=..\UnrealEngine"
set "PROJECT=%~dp0ShootingArena.uproject"
set "ENGINE_ROOT="
for /f "usebackq delims=" %%I in (`powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0ResolveUnrealEngine.ps1" -Project "%PROJECT%"`) do set "ENGINE_ROOT=%%I"
if not defined ENGINE_ROOT (
    if /i not "%~1"=="--no-pause" pause
    exit /b 1
)
set "BUILD_BAT=%ENGINE_ROOT%\Engine\Build\BatchFiles\Build.bat"
set "GENERATE_PROJECT_FILES_BAT=%ENGINE_ROOT%\Engine\Build\BatchFiles\GenerateProjectFiles.bat"
set "FAILED=0"

if not exist "%PROJECT%" (
    echo [ERROR] ShootingArena.uproject was not found beside this BAT file.
    set "FAILED=1"
    goto :end
)
if not exist "%BUILD_BAT%" (
    echo [ERROR] Unreal Build.bat was not found:
    echo         %BUILD_BAT%
    echo Set SHOOTINGARENA_ENGINE_ROOT to your Unreal Engine root.
    set "FAILED=1"
    goto :end
)

echo ============================================================================
echo ShootingArena Portable Build and Verify
echo Engine: %ENGINE_ROOT%
echo ============================================================================
echo.
echo [1/3] Regenerating Visual Studio project files...
if exist "%GENERATE_PROJECT_FILES_BAT%" (
    call "%GENERATE_PROJECT_FILES_BAT%" "%PROJECT%" -game -engine
) else (
    REM Launcher engines generate project files through UnrealBuildTool.
    call "%BUILD_BAT%" -ProjectFiles -Project="%PROJECT%" -Game -Engine
)
if errorlevel 1 (
    echo [FAILED] Visual Studio project-file generation
    set "FAILED=1"
    goto :end
)

echo.
echo [2/3] Building Development...
call "%BUILD_BAT%" ShootingArena Win64 Development "%PROJECT%" -WaitMutex
if errorlevel 1 (
    echo [FAILED] Development
    set "FAILED=1"
    goto :end
)
echo [DONE] Development

echo.
echo [3/3] Building Development Editor...
call "%BUILD_BAT%" ShootingArenaEditor Win64 Development "%PROJECT%" -WaitMutex
if errorlevel 1 (
    echo [FAILED] Development Editor
    set "FAILED=1"
    goto :end
)
echo [DONE] Development Editor

:end
echo.
if "!FAILED!"=="0" (
    echo [PASSED] Project files, Development, and Development Editor are ready.
) else (
    echo [FAILED] Build verification did not complete. Review the errors above.
)
if /i not "%~1"=="--no-pause" pause
endlocal & exit /b %FAILED%
