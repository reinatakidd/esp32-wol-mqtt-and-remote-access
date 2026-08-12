@echo off
setlocal

set "MQTT_CONFIG=%USERPROFILE%\.config\mosquitto\wakepc.conf"
set "MQTT_EXE=C:\Program Files\mosquitto\mosquitto_pub.exe"
set "PC=HOST PC"

echo.
echo ========================================
echo             Wake PC
echo ========================================
echo.

echo [1/2] Sending wake command...

"%MQTT_EXE%" -o "%MQTT_CONFIG%" -t "home/pc/wake" -m "WAKE" -q 1

if errorlevel 1 (
    echo.
    echo ERROR: Failed to send wake command.
    echo.
    pause
    exit /b 1
)

echo Wake command sent successfully.
echo.

echo [2/2] Waiting for %PC% to come online...

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

<nul set /p "=."
timeout /t 2 /nobreak >nul
goto WAIT_PC

:PC_ONLINE

echo.
echo.
echo ========================================
echo       %PC% is ONLINE!
echo ========================================
echo.
echo PC is ready.
echo.

endlocal