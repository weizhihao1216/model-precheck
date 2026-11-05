@echo off
title model-precheck Packaging Script
setlocal EnableDelayedExpansion

set PROJECT_DIR=%~dp0
set DIST_ROOT=%PROJECT_DIR%dist
set APP_CODE=model-precheck
set APP_VERSION=v1.0.0
set APP_PLATFORM=windows
set PACKAGE_NAME=%APP_CODE%_%APP_VERSION%_%APP_PLATFORM%

echo ============================================================
echo   %APP_CODE% Packaging Script (%PACKAGE_NAME%)
echo ============================================================
echo.

REM 1. Ensure CMake is in PATH (check system PATH first, then standard %ProgramFiles%\CMake)
where cmake.exe >nul 2>nul
if %ERRORLEVEL% neq 0 (
    if exist "%ProgramFiles%\CMake\bin\cmake.exe" set "PATH=%ProgramFiles%\CMake\bin;!PATH!"
    if exist "%ProgramFiles(x86)%\CMake\bin\cmake.exe" set "PATH=%ProgramFiles(x86)%\CMake\bin;!PATH!"
)
where cmake.exe >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake not found. Please install CMake or add it to PATH.
    exit /b 1
)

REM 2. Discover Qt 5.14 via Environment Variables (NO hardcoded paths)
set "QT_DIR="

if not "%~1"=="" if exist "%~1\bin\windeployqt.exe" set "QT_DIR=%~1"

if not defined QT_DIR if defined ThirdPart if exist "%ThirdPart%\Qt5.14\vc143\x64\bin\windeployqt.exe" set "QT_DIR=%ThirdPart%\Qt5.14\vc143\x64"
if not defined QT_DIR if defined ThirdPart if exist "%ThirdPart%\Qt5.14\msvc2017_64\bin\windeployqt.exe" set "QT_DIR=%ThirdPart%\Qt5.14\msvc2017_64"
if not defined QT_DIR if defined THIRDPARTY if exist "%THIRDPARTY%\Qt5.14\vc143\x64\bin\windeployqt.exe" set "QT_DIR=%THIRDPARTY%\Qt5.14\vc143\x64"
if not defined QT_DIR if defined THIRDPARTY if exist "%THIRDPARTY%\Qt5.14\msvc2017_64\bin\windeployqt.exe" set "QT_DIR=%THIRDPARTY%\Qt5.14\msvc2017_64"

REM Fallback check %QTDIR% or %QT_DIR%
if not defined QT_DIR if defined QTDIR if exist "%QTDIR%\bin\windeployqt.exe" (
    echo %QTDIR% | findstr /I "5.14" >nul && set "QT_DIR=%QTDIR%"
)
if not defined QT_DIR if defined QTDIR if exist "%QTDIR%\bin\windeployqt.exe" set "QT_DIR=%QTDIR%"

if not defined QT_DIR (
    echo [ERROR] Qt installation not found. Please set %%QTDIR%% or %%ThirdPart%% to your Qt 5.14 directory.
    exit /b 1
)

echo Qt Path       : %QT_DIR%
echo Package Name  : %PACKAGE_NAME%
echo Output Dir    : %DIST_ROOT%
echo Target Base   : Visual Studio 2022 (MSVC v143) / Qt 5.14 (x64)
echo.

REM 3. Clean and prepare dist directory
if exist "%DIST_ROOT%" (
    echo [0/6] Cleaning previous dist directory...
    rmdir /S /Q "%DIST_ROOT%"
)
mkdir "%DIST_ROOT%"

set "PKG_DIR=%DIST_ROOT%\%PACKAGE_NAME%"
set "PKG_ZIP=%DIST_ROOT%\%PACKAGE_NAME%.zip"
set "BUILD_DIR=%PROJECT_DIR%build_pkg"

if exist "%PKG_DIR%" rmdir /S /Q "%PKG_DIR%"
mkdir "%PKG_DIR%"

echo [1/6] Configuring CMake (Prioritizing VS2022)...
set "QTDIR=%QT_DIR%"
set "PATH=%QT_DIR%\bin;%PATH%"

REM Detect VS2022 generator availability
set "VS_GENERATOR="
cmake -G "Visual Studio 17 2022" -A x64 -B "%DIST_ROOT%\_probe_vs" >nul 2>nul
if !ERRORLEVEL! equ 0 (
    set "VS_GENERATOR=Visual Studio 17 2022"
    rmdir /S /Q "%DIST_ROOT%\_probe_vs" >nul 2>nul
)

if defined VS_GENERATOR (
    echo Using Generator: !VS_GENERATOR!
    cmake -G "!VS_GENERATOR!" -A x64 -B "%BUILD_DIR%" -DCMAKE_INSTALL_PREFIX="%PKG_DIR%" -DCMAKE_PREFIX_PATH="%QT_DIR%"
) else (
    echo Using Generator: Default MSVC ^(Fallback^)
    cmake -B "%BUILD_DIR%" -A x64 -DCMAKE_INSTALL_PREFIX="%PKG_DIR%" -DCMAKE_PREFIX_PATH="%QT_DIR%"
)
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake configure failed
    exit /b 1
)

echo [2/6] Building Release target and installing...
cmake --build "%BUILD_DIR%" --config Release --target install
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Build failed
    exit /b 1
)

echo [3/6] Deploying runtime dependencies (optimized, excluding debug/translations/opengl-sw)...
"%QT_DIR%\bin\windeployqt.exe" --release --no-translations --no-opengl-sw --no-virtualkeyboard --qmldir "%PROJECT_DIR%src\qml" "%PKG_DIR%\model-precheck.exe"
if %ERRORLEVEL% neq 0 (
    echo [ERROR] windeployqt failed
    exit /b 1
)

REM Strip development/unused tooling and redundant styles
echo [3.1/6] Cleaning development tooling and unused styles...
if exist "%PKG_DIR%\qmltooling" rmdir /S /Q "%PKG_DIR%\qmltooling"
if exist "%PKG_DIR%\bearer" rmdir /S /Q "%PKG_DIR%\bearer"
if exist "%PKG_DIR%\QtQuick\Controls.2\Imagine" rmdir /S /Q "%PKG_DIR%\QtQuick\Controls.2\Imagine"
if exist "%PKG_DIR%\QtQuick\Controls.2\Material" rmdir /S /Q "%PKG_DIR%\QtQuick\Controls.2\Material"
if exist "%PKG_DIR%\QtQuick\Controls.2\Universal" rmdir /S /Q "%PKG_DIR%\QtQuick\Controls.2\Universal"
for %%F in (qicns.dll qtga.dll qwbmp.dll qtiff.dll qwebp.dll) do (
    if exist "%PKG_DIR%\imageformats\%%F" del /F /Q "%PKG_DIR%\imageformats\%%F"
)

echo [4/6] Copying prerequisites and sample models...
set "PREREQ_SRC=%PROJECT_DIR%deploy\prerequisites"
set "PREREQ_DST=%PKG_DIR%\prerequisites"
if not exist "%PREREQ_DST%" mkdir "%PREREQ_DST%"
copy /Y "%PREREQ_SRC%\README.md" "%PKG_DIR%\README.md" >nul
copy /Y "%PREREQ_SRC%\README.md" "%PREREQ_DST%\README.md" >nul
copy /Y "%PREREQ_SRC%\install_build_tools.bat" "%PREREQ_DST%\install_build_tools.bat" >nul
copy /Y "%PREREQ_SRC%\download_prerequisites.ps1" "%PREREQ_DST%\download_prerequisites.ps1" >nul
if exist "%PREREQ_SRC%\vc_redist.x64.exe" copy /Y "%PREREQ_SRC%\vc_redist.x64.exe" "%PREREQ_DST%\vc_redist.x64.exe" >nul
if exist "%PREREQ_SRC%\vs_BuildTools.exe" copy /Y "%PREREQ_SRC%\vs_BuildTools.exe" "%PREREQ_DST%\vs_BuildTools.exe" >nul

xcopy /E /I /Y /Q "%PROJECT_DIR%sample_models" "%PKG_DIR%\sample_models" >nul

echo [5/6] Verifying runtime startup smoke test...
powershell -NoProfile -ExecutionPolicy Bypass -Command "$psi = New-Object System.Diagnostics.ProcessStartInfo; $psi.FileName = '%PKG_DIR%\model-precheck.exe'; $psi.WorkingDirectory = '%PKG_DIR%'; $psi.UseShellExecute = $false; $p = [System.Diagnostics.Process]::Start($psi); $exited = $p.WaitForExit(2000); if ($exited) { Write-Host '[ERROR] model-precheck.exe crashed or exited prematurely with code:' $p.ExitCode; exit 1 } else { $p.Kill(); Write-Host '[OK] runtime verification passed.'; exit 0 }"
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Smoke test failed for package! Aborting package creation.
    exit /b 1
)

REM Clean smoke test temporary directory/files before archiving
if exist "%PKG_DIR%\session" rmdir /S /Q "%PKG_DIR%\session"
for /R "%PKG_DIR%\sample_models" %%F in (*__run*.dll) do if exist "%%F" del /F /Q "%%F"

echo [6/6] Creating ZIP archive (%PACKAGE_NAME%.zip)...
if exist "%PKG_ZIP%" del /F /Q "%PKG_ZIP%"
powershell -NoProfile -Command "Compress-Archive -Path '%PKG_DIR%' -DestinationPath '%PKG_ZIP%' -Force"
echo [OK] Package Created: %PKG_ZIP%
echo.

echo ============================================================
echo   Packaging Finished Successfully
echo   Output package in: %DIST_ROOT%
echo   - %PKG_DIR%
echo   - %PKG_ZIP%
echo ============================================================
echo.
exit /b 0
