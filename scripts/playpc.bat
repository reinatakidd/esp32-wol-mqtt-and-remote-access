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

echo [1/3] Checking %PC%...

tailscale ping %PC% >nul 2>&1

if not errorlevel 1 (
    echo %PC% is already online.
    goto PC_ONLINE
)

echo %PC% is offline.
echo.
echo Sending wake command...

"%MQTT_EXE%" -o "%MQTT_CONFIG%" -t "home/pc/wake" -m "WAKE" -q 1

if errorlevel 1 (
    echo.
    echo ERROR: Could not send wake command.
    echo.
    pause
    exit /b 1
)

echo Wake command sent.
echo.

echo [2/3] Waiting for %PC% to come online...

set /a ATTEMPTS=0

:WAIT_PC
tailscale ping %PC% >nul 2>&1

if not errorlevel 1 goto PC_ONLINE

set /a ATTEMPTS+=1

if %ATTEMPTS% GEQ 45 (
    echo.
    echo ERROR: %PC% did not come online within 90 seconds.
    echo.
    pause
    exit /b 1
)

echo Waiting...
timeout /t 2 /nobreak >nul
goto WAIT_PC

:PC_ONLINE

echo.
echo %PC% is online!
echo.

echo Waiting for Windows and Sunshine...
timeout /t 8 /nobreak >nul

echo.
echo [3/3] Starting Moonlight...
echo.

start "" "%MOONLIGHT%" stream %PC% "%APP%" --1080 --fps 60 --bitrate 10000 --display-mode fullscreen --keep-awake

echo Moonlight launched.
echo.

endlocal