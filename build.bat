@echo off
::: ============================================================
::: EyeBreak - Release Build Script
::: 编译优化版本（无日志输出，体积最小）
::: ============================================================
setlocal

set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"

if not exist "%VS_PATH%" (
    echo [ERROR] 未找到 Visual Studio 编译环境:
    echo   %VS_PATH%
    echo.
    echo 请修改本脚本中的 VS_PATH 变量指向正确的 vcvars64.bat 路径。
    pause
    exit /b 1
)

call "%VS_PATH%" >nul 2>&1
if errorlevel 1 (
    echo [ERROR] VS 编译环境初始化失败
    pause
    exit /b 1
)

cd /d "%~dp0"

echo ========================================
echo   EyeBreak Release Build
echo ========================================

if not exist build mkdir build
if not exist dist mkdir dist
if not exist log mkdir log

rc.exe /I include /Fo build\resources.res res\resources.rc
if errorlevel 1 (
    echo [ERROR] 资源编译失败
    pause
    exit /b 1
)

cl.exe /MT /O2 /utf-8 /W3 /I include /Fe:dist\EyeBreak.exe /Fobuild\ src\eye_break.c build\resources.res /link /SUBSYSTEM:WINDOWS user32.lib shell32.lib advapi32.lib ole32.lib wtsapi32.lib >log\build_output.txt 2>&1
if errorlevel 1 (
    echo [ERROR] C 编译失败，详见 log\build_output.txt
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
