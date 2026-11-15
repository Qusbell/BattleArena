@echo off
setlocal

set "ENGINE=%~1\Engine"
set "PROJECT=%~2"
set "EDITOR_CMD=%ENGINE%\Binaries\Win64\UnrealEditor-Cmd.exe"

if not exist "%EDITOR_CMD%" (
    echo [ERROR] UnrealEditor-Cmd.exe not found: "%EDITOR_CMD%"
    exit /b 1
)

echo.
echo === Preparing PathLink routing caches before cook ===
call "%ENGINE%\Build\BatchFiles\Build.bat" ShootingArenaEditor Win64 Development -Project="%PROJECT%" -WaitMutex -NoHotReload
if errorlevel 1 exit /b 1

"%EDITOR_CMD%" "%PROJECT%" -run=PreparePathLinkCaches -unattended -nop4 -nullrhi -stdout -FullStdOutLogOutput
exit /b %ERRORLEVEL%
