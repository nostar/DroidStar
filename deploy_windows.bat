@echo off
REM
REM Windows Deployment Script for DroidStar
REM Creates a portable Windows executable with all dependencies
REM
REM Prerequisites:
REM   - Qt6 installed and in PATH
REM   - CMake and Visual Studio Build Tools
REM   - vcpkg (optional, for MIDI support)
REM
REM Usage: deploy_windows.bat [Release|Debug] [x64|x86]
REM

setlocal enabledelayedexpansion

REM Default parameters
set BUILD_TYPE=Release
set ARCHITECTURE=x64

REM Parse command line arguments
if not "%1"=="" set BUILD_TYPE=%1
if not "%2"=="" set ARCHITECTURE=%2

echo.
echo ======================================
echo DroidStar Windows Deployment Script
echo ======================================
echo.
echo Build Type: %BUILD_TYPE%
echo Architecture: %ARCHITECTURE%
echo.

REM Check for required tools
where cmake >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo ERROR: CMake not found in PATH
    echo Please install CMake and add it to your PATH
    exit /b 1
)

where qmake6.exe >nul 2>&1 || where qmake.exe >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo ERROR: Qt qmake not found in PATH
    echo Please install Qt6 and add it to your PATH
    exit /b 1
)

where windeployqt.exe >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo ERROR: windeployqt not found in PATH
    echo Please ensure Qt6 tools are in your PATH
    exit /b 1
)

echo ^> Checking Visual Studio Build Tools...
if "%ARCHITECTURE%"=="x64" (
    set VS_ARCH=x64
    set CMAKE_ARCH=x64
) else (
    set VS_ARCH=x86
    set CMAKE_ARCH=Win32
)

REM Set up build environment
call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" %VS_ARCH% 2>nul
if %ERRORLEVEL% neq 0 (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" %VS_ARCH% 2>nul
    if %ERRORLEVEL% neq 0 (
        call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" %VS_ARCH% 2>nul
        if %ERRORLEVEL% neq 0 (
            echo ERROR: Visual Studio Build Tools not found
            echo Please install Visual Studio 2019 or 2022 with C++ build tools
            exit /b 1
        )
    )
)

echo ^> Setting up build directory...
if exist build rmdir /s /q build
mkdir build
cd build

echo ^> Configuring with CMake...
cmake .. ^
    -G "Visual Studio 17 2022" ^
    -A %CMAKE_ARCH% ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    -DENABLE_MIDI=ON

if %ERRORLEVEL% neq 0 (
    echo ERROR: CMake configuration failed
    cd ..
    exit /b 1
)

echo ^> Building DroidStar...
cmake --build . --config %BUILD_TYPE% --parallel

if %ERRORLEVEL% neq 0 (
    echo ERROR: Build failed
    cd ..
    exit /b 1
)

echo ^> Checking build output...
if not exist "%BUILD_TYPE%\DroidStar.exe" (
    echo ERROR: DroidStar.exe not found in %BUILD_TYPE% directory
    cd ..
    exit /b 1
)

REM Create deployment directory
set DEPLOY_DIR=DroidStar_Windows_%ARCHITECTURE%_%BUILD_TYPE%
echo ^> Creating portable bundle: %DEPLOY_DIR%...
if exist %DEPLOY_DIR% rmdir /s /q %DEPLOY_DIR%
mkdir %DEPLOY_DIR%

REM Copy executable
copy "%BUILD_TYPE%\DroidStar.exe" "%DEPLOY_DIR%\"

REM Deploy Qt dependencies
echo ^> Deploying Qt frameworks and dependencies...
if "%BUILD_TYPE%"=="Debug" (
    windeployqt.exe --debug --qmldir .. --verbose 2 "%DEPLOY_DIR%\DroidStar.exe"
) else (
    windeployqt.exe --release --qmldir .. --verbose 2 "%DEPLOY_DIR%\DroidStar.exe"
)

if %ERRORLEVEL% neq 0 (
    echo WARNING: windeployqt reported issues, but continuing...
)

REM Copy additional runtime libraries if available
echo ^> Checking for additional dependencies...
if exist "C:\Windows\System32\msvcp140.dll" (
    echo   Found Visual C++ Redistributable
) else (
    echo   WARNING: Visual C++ Redistributable may be required on target systems
)

REM Copy vcpkg dependencies if available
if defined VCPKG_ROOT (
    echo ^> Copying vcpkg dependencies...
    if exist "%VCPKG_ROOT%\installed\%ARCHITECTURE%-windows\bin\*.dll" (
        copy "%VCPKG_ROOT%\installed\%ARCHITECTURE%-windows\bin\*.dll" "%DEPLOY_DIR%\" >nul 2>&1
        echo   Copied vcpkg runtime libraries
    )
)

REM Test the deployment
echo ^> Testing portable executable...
cd %DEPLOY_DIR%
DroidStar.exe --version >nul 2>&1
if %ERRORLEVEL% equ 0 (
    echo   ^> Executable test PASSED
) else (
    echo   ^> WARNING: Executable test failed - may be missing dependencies
)
cd ..

REM Calculate sizes
for /f %%i in ('dir /s "%DEPLOY_DIR%" ^| find "bytes"') do set BUNDLE_SIZE=%%i

echo.
echo ======================================
echo Deployment Complete!
echo ======================================
echo.
echo Portable bundle: %DEPLOY_DIR%\
echo Build type: %BUILD_TYPE%
echo Architecture: %ARCHITECTURE%
echo.
echo To test: cd %DEPLOY_DIR% ^&^& DroidStar.exe
echo To distribute: Archive the %DEPLOY_DIR% folder
echo.
echo Note: The portable bundle includes all necessary Qt frameworks
echo       and should run on other Windows systems without requiring
echo       Qt installation.
echo.

cd ..
pause