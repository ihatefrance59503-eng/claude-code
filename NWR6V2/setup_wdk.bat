@echo off
echo === Updating WDK Toolset with Kernel Paths ===

set "TOOLSETPATH=C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\MSBuild\Microsoft\VC\v180\Platforms\x64\PlatformToolsets\WindowsKernelModeDriver10.0"

>"%TOOLSETPATH%\Toolset.props" (
echo ^<?xml version="1.0" encoding="utf-8"?^>
echo ^<Project xmlns="http://schemas.microsoft.com/developer/msbuild/2003"^>
echo   ^<PropertyGroup^>
echo     ^<WindowsSdkDir Condition="'$(WindowsSdkDir)' == ''"^>C:\Program Files ^(x86^)\Windows Kits\10\^</WindowsSdkDir^>
echo     ^<WindowsSdkVersion^>10.0.26100.0^</WindowsSdkVersion^>
echo     ^<KernelEnabled>true^</KernelEnabled^>
echo   ^</PropertyGroup^>
echo   ^<Import Project="$(VCTargetsPath)\Microsoft.Cpp.MSVC.Toolset.x64.props"/^>
echo   ^<PropertyGroup^>
echo     ^<IncludePath^>$(WindowsSdkDir)Include\10.0.26100.0\km;$(WindowsSdkDir)Include\10.0.26100.0\shared;$(WindowsSdkDir)Include\10.0.26100.0\um;$(WindowsSdkDir)Include\10.0.26100.0\ucrt;$(IncludePath)^</IncludePath^>
echo     ^<LibraryPath^>$(WindowsSdkDir)Lib\10.0.26100.0\km\x64;$(WindowsSdkDir)Lib\10.0.26100.0\um\x64;$(WindowsSdkDir)Lib\10.0.26100.0\ucrt\x64;$(LibraryPath)^</LibraryPath^>
echo     ^<LibraryPath^>$(WindowsSdkDir)Lib\10.0.26100.0\km\x64;$(LibraryPath)^</LibraryPath^>
echo     ^<NtTargetArchPath^>x64^</NtTargetArchPath^>
echo   ^</PropertyGroup^>
echo   ^<ItemDefinitionGroup^>
echo     ^<ClCompile^>
echo       ^<PreprocessorDefinitions^>NT_KERNEL_MODE;%(PreprocessorDefinitions)^</PreprocessorDefinitions^>
echo       ^<RuntimeLibrary^>MultiThreadedDLL^</RuntimeLibrary^>
echo       ^<DisableSpecificWarnings^>4201;4244;4100;4996;%(DisableSpecificWarnings)^</DisableSpecificWarnings^>
echo     ^</ClCompile^>
echo   ^</ItemDefinitionGroup^>
echo ^</Project^>
)

>"%TOOLSETPATH%\Toolset.targets" (
echo ^<?xml version="1.0" encoding="utf-8"?^>
echo ^<Project xmlns="http://schemas.microsoft.com/developer/msbuild/2003"^>
echo   ^<Import Project="$(VCTargetsPath)\Microsoft.CppCommon.targets"/^>
echo   ^<Import Project="$(VCTargetsPath)\Microsoft.Cpp.WindowsSDK.targets"/^>
echo ^</Project^>
)

echo Done!
pause
