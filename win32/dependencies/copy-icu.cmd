set ICU=C:\Dev\icu

cd %~dp0

xcopy /Y /I %ICU%\include\unicode include\unicode

xcopy /Y %ICU%\bin\*51.dll lib_x86_release
xcopy /Y %ICU%\bin\*51d.dll lib_x86_debug
xcopy /Y %ICU%\bin\icudt51.dll lib_x86_release
xcopy /Y %ICU%\bin\icudt51.dll lib_x86_debug

xcopy /Y %ICU%\bin64\*51.dll lib_x64_release
xcopy /Y %ICU%\bin64\*51d.dll lib_x64_debug
xcopy /Y %ICU%\bin64\icudt51.dll lib_x64_release
xcopy /Y %ICU%\bin64\icudt51.dll lib_x64_debug

xcopy /Y %ICU%\lib\icuin.* lib_x86_release
xcopy /Y %ICU%\lib\icuind.* lib_x86_debug
xcopy /Y %ICU%\lib\icuuc.* lib_x86_release
xcopy /Y %ICU%\lib\icuucd.* lib_x86_debug
xcopy /Y %ICU%\lib\icudt.* lib_x86_release
xcopy /Y %ICU%\lib\icudt.* lib_x86_debug

xcopy /Y %ICU%\lib64\icuin.* lib_x64_release
xcopy /Y %ICU%\lib64\icuind.* lib_x64_debug
xcopy /Y %ICU%\lib64\icuuc.* lib_x64_release
xcopy /Y %ICU%\lib64\icuucd.* lib_x64_debug
xcopy /Y %ICU%\lib64\icudt.* lib_x64_release
xcopy /Y %ICU%\lib64\icudt.* lib_x64_debug
