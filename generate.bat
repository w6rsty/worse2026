@echo off
rem Configure the worse build (UE-style "generate project files"): set up the per-machine
rem toolchain env, then run the CMake preset against engine\. This only CONFIGURES — build
rem afterwards with your IDE or:  cmake --build engine\build\<preset>
rem
rem Usage:  generate.bat [preset]   rem preset defaults to "debug" (release / relwithdebinfo / asan / lto)
setlocal
set "ROOT=%~dp0"
set "PRESET=%~1"
if "%PRESET%"=="" set "PRESET=debug"

rem Dot-source windows.ps1 (sets CXX/CC + VCPKG_ROOT) then configure from inside engine\.
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  ". '%ROOT%scripts\toolchain\windows.ps1'; Push-Location '%ROOT%engine'; cmake --preset %PRESET%; $code = $LASTEXITCODE; Pop-Location; exit $code"
if errorlevel 1 exit /b %errorlevel%

echo.
echo Configured engine\build\%PRESET%. Build it with:
echo     cmake --build engine\build\%PRESET%
endlocal
