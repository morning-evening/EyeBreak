@echo off
::: ============================================================
::: EyeBreak - Release Build Script
::: Optimized release build (no logging, smallest size)
::: ============================================================
setlocal

set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"

if not exist "%VS_PATH%" (
    echo [ERROR] Visual Studio build environment not found:
    echo   %VS_PATH%
    echo.
    echo Please update VS_PATH in this script to point to your vcvars64.bat.
    pause
    exit /b 1
)

call "%VS_PATH%" >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Failed to initialize VS build environment
    pause
    exit /b 1
)

cd /d "%~dp0"

taskkill /f /im EyeBreak.exe >nul 2>&1

echo ========================================
echo   EyeBreak Release Build
echo ========================================

if not exist build mkdir build
if not exist dist mkdir dist
if not exist log mkdir log

rc.exe /I include /Fo build\resources.res res\resources.rc
if errorlevel 1 (
    echo [ERROR] Resource compilation failed
    pause
    exit /b 1
)

cl.exe /MT /O2 /utf-8 /W3 /I include /Fe:dist\EyeBreak.exe /Fobuild\ src\eye_break.c build\resources.res /link /SUBSYSTEM:WINDOWS user32.lib shell32.lib advapi32.lib ole32.lib wtsapi32.lib >log\build_output.txt 2>&1
if errorlevel 1 (
    echo [ERROR] C compilation failed, see log\build_output.txt
    pause
    exit /b 1
)

echo.
if exist dist\EyeBreak.exe (
    echo === SUCCESS ===
    for %%A in (dist\EyeBreak.exe) do @echo   Size: %%~zA bytes
) else (
    echo === FAILED ===
)

pause
