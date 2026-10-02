@echo off
echo ============================================
echo   SIGN DRIVER
echo   Right-click this file, Run as Admin
echo ============================================
echo.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0sign_driver.ps1"
echo.
pause
