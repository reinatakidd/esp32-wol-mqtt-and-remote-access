@echo off
setlocal

set "PC=HOST PC"

echo.
echo ========================================
echo           %PC% Status
echo ========================================
echo.

tailscale ping %PC% >nul 2>&1

if errorlevel 1 (
    echo [OFFLINE] %PC% is dead.
) else (
    echo [ONLINE] %PC% is alive!
)

echo.

endlocal