@echo off
echo Checking Event Log for driver errors...
echo.
powershell -NoProfile -ExecutionPolicy Bypass -Command "Get-WinEvent -LogName System -MaxEvents 30 | Where-Object { $_.ProviderName -eq 'Service Control Manager' } | Select-Object -First 5 | ForEach-Object { Write-Host '---'; Write-Host 'Time:' $_.TimeCreated; Write-Host 'ID:' $_.Id; Write-Host $_.Message; Write-Host '' }"
pause
