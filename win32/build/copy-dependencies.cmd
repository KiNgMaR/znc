cd %~dp0

mkdir Win32-Debug
mkdir x64-Debug
mkdir Win32-Release
mkdir x64-Release

xcopy /y ..\dependencies\lib_x86_debug\* Win32-Debug
xcopy /y ..\dependencies\lib_x64_debug\* x64-Debug
xcopy /y ..\dependencies\lib_x86_release\*.dll Win32-Release
xcopy /y ..\dependencies\lib_x64_release\*.dll x64-Release
