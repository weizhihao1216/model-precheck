@echo off
title model-precheck Build Script
setlocal EnableDelayedExpansion

set PROJECT_DIR=%~dp0
set BUILD_DIR=%PROJECT_DIR%build
set DIST_DIR=%PROJECT_DIR%dist

REM ============================================================
REM Resolve Qt path dynamically via Environment Variables (NO hardcoded paths)
REM Supported variables:
REM   1. %QTDIR% or %QT_DIR%
REM   2. %ThirdPart% or %THIRDPARTY%
REM ============================================================
REM If user requested packaging, forward to package.bat
if /I "%~1"=="package" (
    call "%~dp0package.bat" %2 %3
    exit /b !ERRORLEVEL!
)

REM Check command line parameter first (e.g. build.bat D:\Qt\5.14.2\msvc2017_64 or build.bat 5.11)
set "QT_DIR="
if /I "%~1"=="5.11" (
    if defined ThirdPart if exist "%ThirdPart%\Qt5.11\vc140\x64\bin\windeployqt.exe" set "QT_DIR=%ThirdPart%\Qt5.11\vc140\x64"
    if defined ThirdPart if exist "%ThirdPart%\Qt5.11\msvc2017_64\bin\windeployqt.exe" set "QT_DIR=%ThirdPart%\Qt5.11\msvc2017_64"
    if defined THIRDPARTY if exist "%THIRDPARTY%\Qt5.11\vc140\x64\bin\windeployqt.exe" set "QT_DIR=%THIRDPARTY%\Qt5.11\vc140\x64"
)
if /I "%~1"=="5.14" (
    if defined ThirdPart if exist "%ThirdPart%\Qt5.14\vc143\x64\bin\windeployqt.exe" set "QT_DIR=%ThirdPart%\Qt5.14\vc143\x64"
    if defined ThirdPart if exist "%ThirdPart%\Qt5.14\msvc2017_64\bin\windeployqt.exe" set "QT_DIR=%ThirdPart%\Qt5.14\msvc2017_64"
    if defined THIRDPARTY if exist "%THIRDPARTY%\Qt5.14\vc143\x64\bin\windeployqt.exe" set "QT_DIR=%THIRDPARTY%\Qt5.14\vc143\x64"
)
if not "%~1"=="" if exist "%~1\bin\windeployqt.exe" set "QT_DIR=%~1"
if not defined QT_DIR if defined QTDIR if exist "%QTDIR%\bin\windeployqt.exe" set "QT_DIR=%QTDIR%"
if not defined QT_DIR if defined Qt5_DIR if exist "%Qt5_DIR%\bin\windeployqt.exe" set "QT_DIR=%Qt5_DIR%"

REM Fallback: search under %ThirdPart% or %THIRDPARTY% environment variable
if not defined QT_DIR if defined ThirdPart if exist "%ThirdPart%\Qt5.14\vc143\x64\bin\windeployqt.exe" set "QT_DIR=%ThirdPart%\Qt5.14\vc143\x64"
if not defined QT_DIR if defined ThirdPart if exist "%ThirdPart%\Qt5.14\msvc2017_64\bin\windeployqt.exe" set "QT_DIR=%ThirdPart%\Qt5.14\msvc2017_64"
if not defined QT_DIR if defined ThirdPart if exist "%ThirdPart%\Qt5.11\vc140\x64\bin\windeployqt.exe" set "QT_DIR=%ThirdPart%\Qt5.11\vc140\x64"

if not defined QT_DIR if defined THIRDPARTY if exist "%THIRDPARTY%\Qt5.14\vc143\x64\bin\windeployqt.exe" set "QT_DIR=%THIRDPARTY%\Qt5.14\vc143\x64"
if not defined QT_DIR if defined THIRDPARTY if exist "%THIRDPARTY%\Qt5.14\msvc2017_64\bin\windeployqt.exe" set "QT_DIR=%THIRDPARTY%\Qt5.14\msvc2017_64"
if not defined QT_DIR if defined THIRDPARTY if exist "%THIRDPARTY%\Qt5.11\vc140\x64\bin\windeployqt.exe" set "QT_DIR=%THIRDPARTY%\Qt5.11\vc140\x64"

REM Ensure CMake is in PATH (check system PATH first, then standard %ProgramFiles%\CMake)
where cmake.exe >nul 2>nul
if %ERRORLEVEL% neq 0 (
    if exist "%ProgramFiles%\CMake\bin\cmake.exe" set "PATH=%ProgramFiles%\CMake\bin;!PATH!"
    if exist "%ProgramFiles(x86)%\CMake\bin\cmake.exe" set "PATH=%ProgramFiles(x86)%\CMake\bin;!PATH!"
)
if defined QT_DIR set "PATH=%QT_DIR%\bin;!PATH!"

echo ============================================================
echo   model-precheck One-Click Build Script
echo ============================================================
echo.

if not defined QT_DIR (
    echo [ERROR] Qt installation not found.
    echo Please set environment variable QTDIR or ThirdPart.
    echo Example: set QTDIR=D:\HR\DEV\ThirdParty\Qt5.14\vc143\x64

    exit /b 1
)

echo Using Qt: %QT_DIR%
echo.

echo [1/4] Configuring CMake Project (x64)...
if defined CMAKE_GENERATOR (
    echo !CMAKE_GENERATOR! | findstr /I "Win64" >nul
    if !ERRORLEVEL! equ 0 (
        cmake -G "%CMAKE_GENERATOR%" -B "%BUILD_DIR%" -DCMAKE_INSTALL_PREFIX="%DIST_DIR%" -DCMAKE_PREFIX_PATH="%QT_DIR%"
    ) else (
        cmake -G "%CMAKE_GENERATOR%" -A x64 -B "%BUILD_DIR%" -DCMAKE_INSTALL_PREFIX="%DIST_DIR%" -DCMAKE_PREFIX_PATH="%QT_DIR%"
    )
) else (
    cmake -B "%BUILD_DIR%" -A x64 -DCMAKE_INSTALL_PREFIX="%DIST_DIR%" -DCMAKE_PREFIX_PATH="%QT_DIR%"
)
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake configuration failed!

    exit /b %ERRORLEVEL%
)

echo.
echo [2/4] Building and Installing Release Target...
cmake --build "%BUILD_DIR%" --config Release --target install
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Build or install failed!

    exit /b %ERRORLEVEL%
)

echo.
echo [3/4] Deploying Qt Runtime Dependencies...
"%QT_DIR%\bin\windeployqt.exe" --qmldir "%PROJECT_DIR%src\qml" "%DIST_DIR%\model-precheck.exe"
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Qt runtime deployment failed!

    exit /b %ERRORLEVEL%
)

echo.
echo [4/4] Copying environment prerequisites (VC++ / Build Tools)...
set "PREREQ_SRC=%PROJECT_DIR%deploy\prerequisites"
set "PREREQ_DST=%DIST_DIR%\prerequisites"
if not exist "%PREREQ_DST%" mkdir "%PREREQ_DST%"
copy /Y "%PREREQ_SRC%\README.md" "%DIST_DIR%\README.md" >nul
copy /Y "%PREREQ_SRC%\README.md" "%PREREQ_DST%\README.md" >nul
copy /Y "%PREREQ_SRC%\install_build_tools.bat" "%PREREQ_DST%\install_build_tools.bat" >nul
if exist "%PREREQ_SRC%\vc_redist.x64.exe" (
    copy /Y "%PREREQ_SRC%\vc_redist.x64.exe" "%PREREQ_DST%\vc_redist.x64.exe" >nul
) else (
    echo [WARN] Missing %PREREQ_SRC%\vc_redist.x64.exe
    echo        Run: powershell -ExecutionPolicy Bypass -File "%PREREQ_SRC%\download_prerequisites.ps1"
)
if exist "%PREREQ_SRC%\vs_BuildTools.exe" (
    copy /Y "%PREREQ_SRC%\vs_BuildTools.exe" "%PREREQ_DST%\vs_BuildTools.exe" >nul
) else (
    echo [WARN] Missing %PREREQ_SRC%\vs_BuildTools.exe
    echo        Run: powershell -ExecutionPolicy Bypass -File "%PREREQ_SRC%\download_prerequisites.ps1"
)

echo.
echo ============================================================
echo   Build Completed Successfully!
echo   Output Executable: %DIST_DIR%\model-precheck.exe
echo   Env README:        %DIST_DIR%\README.md
echo ============================================================
echo.
