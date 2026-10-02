@echo off
echo ============================================
echo   Loading WinDrvMgr Kernel Driver
echo ============================================

set "DRIVER_PATH=%~dp0Build\Release\SebwettKM.sys"
set "SERVICE_NAME=WinDrvMgr"

echo.
echo [1] Creating kernel service...
sc create %SERVICE_NAME% type= kernel start= demand binPath= "%DRIVER_PATH%" 2>nul
if errorlevel 1 (
    echo     Service may already exist, deleting and recreating...
    sc delete %SERVICE_NAME% 2>nul
    timeout /t 2 /nobreak >nul
    sc create %SERVICE_NAME% type= kernel start= demand binPath= "%DRIVER_PATH%"
)

echo [2] Starting driver...
sc start %SERVICE_NAME%
if errorlevel 1 (
    echo.
    echo ERROR: Failed to start driver.
    echo Make sure:
    echo   - You ran this as Administrator
    echo   - Test Signing is enabled (bcdedit /set testsigning on)
    echo   - Secure Boot is disabled in BIOS
    echo.
    pause
    exit /b 1
)

echo [3] Driver loaded! Waiting 2 seconds for device creation...
timeout /t 2 /nobreak >nul

echo [4] Launching NWR6 client...
start "" "%~dp0x64\Release\NWR6.exe"

echo.
echo Done! Driver service: %SERVICE_NAME%
echo To unload later: sc stop %SERVICE_NAME% ^& sc delete %SERVICE_NAME%
