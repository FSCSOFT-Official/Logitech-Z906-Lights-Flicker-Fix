@echo off
setlocal enabledelayedexpansion

echo ===================================================
echo  Building Logitech Z906 Keep-Alive Tray App
echo ===================================================

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "!VSWHERE!" (
    echo [ERROR] vswhere.exe not found.
    exit /b 1
)

:: Try VS 2019 first (known to have full MSVC toolchain), then latest
set "VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
if not exist "!VCVARS!" (
    for /f "usebackq tokens=*" %%i in (`"!VSWHERE!" -latest -property installationPath`) do (
        set "VCVARS=%%i\VC\Auxiliary\Build\vcvars64.bat"
    )
)

if not exist "!VCVARS!" (
    echo [ERROR] vcvars64.bat not found.
    exit /b 1
)

echo [INFO] Setting up Visual Studio environment...
call "!VCVARS!" >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Failed to initialize Visual Studio build environment.
    exit /b 1
)

cd /d "%~dp0"

echo [INFO] Compiling Windows resource script...
rc.exe /nologo /fo resource.res resource.rc
if errorlevel 1 (
    echo [ERROR] Resource compilation failed.
    exit /b 1
)

echo [INFO] Compiling C++ application...
cl.exe /nologo /O2 /EHsc /MD /std:c++17 /W4 /DUNICODE /D_UNICODE ^
    main.cpp Config.cpp AudioPlayer.cpp AudioDetector.cpp TrayManager.cpp resource.res ^
    /Fe:LogitechKeepAlive.exe ^
    /link /SUBSYSTEM:WINDOWS /INCREMENTAL:NO ^
    user32.lib gdi32.lib shell32.lib winmm.lib ole32.lib

if errorlevel 1 (
    echo [ERROR] Build failed.
    exit /b 1
)

echo [INFO] Cleaning up intermediate build artifacts...
del /q *.obj resource.res 2>nul

echo ===================================================
echo  Build Succeeded: LogitechKeepAlive.exe
echo ===================================================
