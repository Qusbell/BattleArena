@echo off
setlocal

REM ============================================================
REM  [TEST ONLY] Start a dedicated server from the editor build.
REM
REM  The game no longer spawns a multiplayer server by itself, so when
REM  testing from the editor (Play / TestStandalone.bat / RunClient.bat)
REM  run this FIRST, wait until the server window stops scrolling
REM  (about 10 seconds), then enter 127.0.0.1 in the multiplayer IP box.
REM
REM  Same launch as the packaged RunServer.bat, but through
REM  UnrealEditor.exe so it always uses your latest local build.
REM  Edit ENGINE_PATH if your engine is installed somewhere else.
REM ============================================================

set "ENGINE_PATH=D:\UE_5.6\Engine\Binaries\Win64\UnrealEditor.exe"
set "MAP=Lobby_Level"
set "PORT=7777"

if not exist "%ENGINE_PATH%" (
    echo [ERROR] UnrealEditor.exe not found:
    echo         %ENGINE_PATH%
    echo         Edit the ENGINE_PATH line in this file.
    echo.
    pause
    exit /b 1
)

echo ============================================================
echo  Dedicated Server (editor build)
echo   MAP  : %MAP%
echo   PORT : %PORT%
echo ============================================================
echo.

"%ENGINE_PATH%" "%~dp0ShootingArena.uproject" %MAP% -server -log -port=%PORT%

echo.
echo Server process ended.
pause

endlocal
