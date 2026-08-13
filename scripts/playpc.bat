@echo off
setlocal

set "MQTT_CONFIG=%USERPROFILE%\.config\mosquitto\wakepc.conf"
set "MQTT_EXE=C:\Program Files\mosquitto\mosquitto_pub.exe"
set "MOONLIGHT=C:\Program Files\Moonlight Game Streaming\Moonlight.exe"

set "PC=HOST PC"
set "APP=Desktop"

echo.
echo ========================================
echo          Remote PC Launcher
echo ========================================
echo.

echo [1/3] Sending wake command to %PC%...

"%MQTT_EXE%" -o "%MQTT_CONFIG%" -t "home/pc/wake" -m "WAKE"

if errorlevel 1 (
    echo.
    echo ERROR: Could not send wake command.
    echo.
    pause
    exit /b 1
)

echo Wake command sent.
echo.

echo [2/3] Checking whether %PC% is online...

set /a ATTEMPTS=0

:WAIT_PC

tailscale ping %PC% >nul 2>&1

if not errorlevel 1 (
    if %ATTEMPTS% EQU 0 (
        goto PC_ALREADY_ONLINE
    ) else (
        goto PC_ONLINE
    )
)

set /a ATTEMPTS+=1
set /a ELAPSED=ATTEMPTS*2

if %ATTEMPTS% GEQ 45 (
    echo.
    echo ERROR: %PC% did not come online within 90 seconds.
    echo.
    pause
    exit /b 1
)

echo Waiting... %ELAPSED%s / 90s
timeout /t 2 /nobreak >nul
goto WAIT_PC


:PC_ALREADY_ONLINE

echo.
echo %PC% is already online!
echo.
goto START_MOONLIGHT


:PC_ONLINE

echo.
echo %PC% is online!
echo.

echo Waiting for Windows and Sunshine...
timeout /t 8 /nobreak >nul


:START_MOONLIGHT

echo.
echo [3/3] Starting Moonlight...
echo.

start "" "%MOONLIGHT%" stream %PC% "%APP%" --1440 --fps 60 --bitrate 21000 --display-mode fullscreen --keep-awake

echo Moonlight launched.
echo.

endlocal