@echo off
cd /d "C:\Users\Merlyn\Desktop\NWR6V2"
"C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\MSBuild\Current\Bin\amd64\MSBuild.exe" reverse.sln /p:Configuration=Release /p:Platform=x64 /t:Build /v:minimal
echo.
pause
