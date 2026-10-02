@echo off
set DUMPBIN="C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\dumpbin.exe"
set SYS=%~dp0Build\Release\SebwettKM.sys
echo Checking imports of %SYS%...
echo.
%DUMPBIN% /dependents %SYS%
echo.
pause
