@echo off
setlocal enabledelayedexpansion

rem Builds the AresEditor target. Run from anywhere; paths are worked out here.
rem
rem   Build.bat              editor, Development  (what you want 99% of the time)
rem   Build.bat Ares         packaged game target
rem   Build.bat AresEditor Debug
rem
rem CLOSE UNREAL FIRST. Live Coding holds the DLLs open, the build fails on a
rem locked file, and the editor carries on running the stale binaries — which
rem shows up as your C++ classes silently not existing rather than as an error.

set "TARGET=%~1"
if "%TARGET%"=="" set "TARGET=AresEditor"
set "CONFIG=%~2"
if "%CONFIG%"=="" set "CONFIG=Development"

set "PROJECT=%~dp0Ares.uproject"
if not exist "%PROJECT%" (
    echo [ares] Ares.uproject not found next to this script. Expected: %PROJECT%
    exit /b 1
)

rem --- find the engine ------------------------------------------------------
rem UE_ROOT wins if you set it. Otherwise ask the registry, which is where the
rem Epic launcher records the install, and only then guess at the usual paths.

if defined UE_ROOT goto :have_root

for /f "tokens=2,*" %%A in (
    'reg query "HKLM\SOFTWARE\EpicGames\Unreal Engine\5.8" /v InstalledDirectory 2^>nul ^| find "InstalledDirectory"'
) do set "UE_ROOT=%%B"

if defined UE_ROOT goto :have_root

for %%P in (
    "C:\Program Files\Epic Games\UE_5.8"
    "D:\Program Files\Epic Games\UE_5.8"
    "C:\Epic Games\UE_5.8"
    "D:\Epic Games\UE_5.8"
) do (
    if exist "%%~P\Engine\Build\BatchFiles\Build.bat" set "UE_ROOT=%%~P"
)

:have_root
if not defined UE_ROOT (
    echo [ares] Could not find Unreal Engine 5.8.
    echo [ares] Set UE_ROOT to the install directory and re-run, e.g.
    echo [ares]     set UE_ROOT=C:\Program Files\Epic Games\UE_5.8
    exit /b 1
)

set "UBT=%UE_ROOT%\Engine\Build\BatchFiles\Build.bat"
if not exist "%UBT%" (
    echo [ares] UE_ROOT is set to "%UE_ROOT%" but there is no Engine\Build\BatchFiles\Build.bat under it.
    exit /b 1
)

echo [ares] engine  %UE_ROOT%
echo [ares] target  %TARGET% Win64 %CONFIG%
echo.

rem -WaitMutex: if the editor or another build already holds the build lock,
rem queue behind it instead of failing outright.
call "%UBT%" %TARGET% Win64 %CONFIG% -Project="%PROJECT%" -WaitMutex
set "RESULT=%ERRORLEVEL%"

echo.
if not "%RESULT%"=="0" (
    echo [ares] BUILD FAILED ^(exit %RESULT%^)
    echo [ares] If it names a locked file, Unreal is still open. Close it and re-run.
) else (
    echo [ares] build ok
)

exit /b %RESULT%
