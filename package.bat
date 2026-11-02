@echo off
title model-precheck Dual-Version Packaging Script
setlocal EnableDelayedExpansion

set PROJECT_DIR=%~dp0
set DIST_ROOT=%PROJECT_DIR%dist

echo ============================================================
echo   model-precheck Dual-Version Packaging Script
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
    if "%CI%"=="" (
    rem pause when run interactively
    timeout /t 3 >nul 2>&1 || pause
)
    exit /b 1
)

REM 2. Discover Qt 5.11 and Qt 5.14 via Environment Variables (NO hardcoded paths)
set "QT511_DIR="
set "QT514_DIR="

if defined QT511_DIR if exist "%QT511_DIR%\bin\windeployqt.exe" set "QT511_DIR=%QT511_DIR%"
if defined QT514_DIR if exist "%QT514_DIR%\bin\windeployqt.exe" set "QT514_DIR=%QT514_DIR%"

if not defined QT511_DIR if defined ThirdPart if exist "%ThirdPart%\Qt5.11\vc140\x64\bin\windeployqt.exe" set "QT511_DIR=%ThirdPart%\Qt5.11\vc140\x64"
if not defined QT511_DIR if defined ThirdPart if exist "%ThirdPart%\Qt5.11\msvc2017_64\bin\windeployqt.exe" set "QT511_DIR=%ThirdPart%\Qt5.11\msvc2017_64"
if not defined QT511_DIR if defined THIRDPARTY if exist "%THIRDPARTY%\Qt5.11\vc140\x64\bin\windeployqt.exe" set "QT511_DIR=%THIRDPARTY%\Qt5.11\vc140\x64"
if not defined QT511_DIR if defined THIRDPARTY if exist "%THIRDPARTY%\Qt5.11\msvc2017_64\bin\windeployqt.exe" set "QT511_DIR=%THIRDPARTY%\Qt5.11\msvc2017_64"

if not defined QT514_DIR if defined ThirdPart if exist "%ThirdPart%\Qt5.14\vc143\x64\bin\windeployqt.exe" set "QT514_DIR=%ThirdPart%\Qt5.14\vc143\x64"
if not defined QT514_DIR if defined ThirdPart if exist "%ThirdPart%\Qt5.14\msvc2017_64\bin\windeployqt.exe" set "QT514_DIR=%ThirdPart%\Qt5.14\msvc2017_64"
if not defined QT514_DIR if defined THIRDPARTY if exist "%THIRDPARTY%\Qt5.14\vc143\x64\bin\windeployqt.exe" set "QT514_DIR=%THIRDPARTY%\Qt5.14\vc143\x64"
if not defined QT514_DIR if defined THIRDPARTY if exist "%THIRDPARTY%\Qt5.14\msvc2017_64\bin\windeployqt.exe" set "QT514_DIR=%THIRDPARTY%\Qt5.14\msvc2017_64"

REM Fallback check %QTDIR% or %QT_DIR%
if defined QTDIR if exist "%QTDIR%\bin\windeployqt.exe" (
    echo %QTDIR% | findstr /I "5.11" >nul && if not defined QT511_DIR set "QT511_DIR=%QTDIR%"
    echo %QTDIR% | findstr /I "5.14" >nul && if not defined QT514_DIR set "QT514_DIR=%QTDIR%"
)
if defined QT_DIR if exist "%QT_DIR%\bin\windeployqt.exe" (
    echo %QT_DIR% | findstr /I "5.11" >nul && if not defined QT511_DIR set "QT511_DIR=%QT_DIR%"
    echo %QT_DIR% | findstr /I "5.14" >nul && if not defined QT514_DIR set "QT514_DIR=%QT_DIR%"
)

set "DO_511=1"
set "DO_514=1"

if /I "%~1"=="5.11" set "DO_514=0"
if /I "%~1"=="qt5.11" set "DO_514=0"
if /I "%~1"=="qt511" set "DO_514=0"

if /I "%~1"=="5.14" set "DO_511=0"
if /I "%~1"=="qt5.14" set "DO_511=0"
if /I "%~1"=="qt514" set "DO_511=0"

echo Qt 5.11 Path : %QT511_DIR%
echo Qt 5.14 Path : %QT514_DIR%
echo.

if not exist "%DIST_ROOT%" mkdir "%DIST_ROOT%"

if %DO_511% equ 1 call :build_package_511
if %DO_514% equ 1 call :build_package_514

goto finish_packaging

REM ============================================================
REM Subroutine: Package Qt 5.11 Release
REM ============================================================
:build_package_511
if not defined QT511_DIR (
    echo [WARN] Qt 5.11 installation not found, skipping Qt 5.11 package.
    exit /b 0
)

echo ------------------------------------------------------------
echo Packaging Qt 5.11 x64 Release
echo ------------------------------------------------------------
set "PKG_DIR_511=%DIST_ROOT%\model-precheck_Qt5.11_x64"
set "BUILD_DIR_511=%PROJECT_DIR%build_pkg_qt511"

if exist "%PKG_DIR_511%" rmdir /S /Q "%PKG_DIR_511%"
mkdir "%PKG_DIR_511%"

echo [1/5] Configuring CMake for Qt 5.11...
set "QTDIR=%QT511_DIR%"
set "PATH=%QT511_DIR%\bin;%PATH%"
cmake -B "%BUILD_DIR_511%" -A x64 -DCMAKE_INSTALL_PREFIX="%PKG_DIR_511%" -DCMAKE_PREFIX_PATH="%QT511_DIR%"
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake configure failed for Qt 5.11
    if "%CI%"=="" (
    rem pause when run interactively
    timeout /t 3 >nul 2>&1 || pause
)
    exit /b 1
)

echo [2/5] Building Release target...
cmake --build "%BUILD_DIR_511%" --config Release --target install
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Build failed for Qt 5.11
    if "%CI%"=="" (
    rem pause when run interactively
    timeout /t 3 >nul 2>&1 || pause
)
    exit /b 1
)

echo [3/5] Deploying Qt 5.11 runtime dependencies...
"%QT511_DIR%\bin\windeployqt.exe" --qmldir "%PROJECT_DIR%src\qml" "%PKG_DIR_511%\model-precheck.exe"
if %ERRORLEVEL% neq 0 (
    echo [ERROR] windeployqt failed for Qt 5.11
    if "%CI%"=="" (
    rem pause when run interactively
    timeout /t 3 >nul 2>&1 || pause
)
    exit /b 1
)

echo [4/5] Copying prerequisites and sample models...
set "PREREQ_SRC=%PROJECT_DIR%deploy\prerequisites"
set "PREREQ_DST=%PKG_DIR_511%\prerequisites"
if not exist "%PREREQ_DST%" mkdir "%PREREQ_DST%"
copy /Y "%PREREQ_SRC%\README.md" "%PKG_DIR_511%\README.md" >nul
copy /Y "%PREREQ_SRC%\README.md" "%PREREQ_DST%\README.md" >nul
copy /Y "%PREREQ_SRC%\install_build_tools.bat" "%PREREQ_DST%\install_build_tools.bat" >nul
copy /Y "%PREREQ_SRC%\download_prerequisites.ps1" "%PREREQ_DST%\download_prerequisites.ps1" >nul
if exist "%PREREQ_SRC%\vc_redist.x64.exe" copy /Y "%PREREQ_SRC%\vc_redist.x64.exe" "%PREREQ_DST%\vc_redist.x64.exe" >nul
if exist "%PREREQ_SRC%\vs_BuildTools.exe" copy /Y "%PREREQ_SRC%\vs_BuildTools.exe" "%PREREQ_DST%\vs_BuildTools.exe" >nul

xcopy /E /I /Y "%PROJECT_DIR%sample_models" "%PKG_DIR_511%\sample_models" >nul

echo [5/6] Verifying Qt 5.11 runtime startup smoke test...
powershell -NoProfile -ExecutionPolicy Bypass -Command "$psi = New-Object System.Diagnostics.ProcessStartInfo; $psi.FileName = '%PKG_DIR_511%\model-precheck.exe'; $psi.WorkingDirectory = '%PKG_DIR_511%'; $psi.UseShellExecute = $false; $p = [System.Diagnostics.Process]::Start($psi); $exited = $p.WaitForExit(2000); if ($exited) { Write-Host '[ERROR] Qt 5.11 model-precheck.exe crashed or exited prematurely with code:' $p.ExitCode; exit 1 } else { $p.Kill(); Write-Host '[OK] Qt 5.11 runtime verification passed.'; exit 0 }"
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Smoke test failed for Qt 5.11 package! Aborting package creation.
    exit /b 1
)

echo [6/6] Creating ZIP archive...
if exist "%PKG_DIR_511%.zip" del /F /Q "%PKG_DIR_511%.zip"
powershell -NoProfile -Command "Compress-Archive -Path '%PKG_DIR_511%' -DestinationPath '%PKG_DIR_511%.zip' -Force"
echo [OK] Qt 5.11 Package Created: %PKG_DIR_511%.zip
echo.
exit /b 0

REM ============================================================
REM Subroutine: Package Qt 5.14 Release
REM ============================================================
:build_package_514
if not defined QT514_DIR (
    echo [WARN] Qt 5.14 installation not found, skipping Qt 5.14 package.
    exit /b 0
)

echo ------------------------------------------------------------
echo Packaging Qt 5.14 x64 Release
echo ------------------------------------------------------------
set "PKG_DIR_514=%DIST_ROOT%\model-precheck_Qt5.14_x64"
set "BUILD_DIR_514=%PROJECT_DIR%build_pkg_qt514"

if exist "%PKG_DIR_514%" rmdir /S /Q "%PKG_DIR_514%"
mkdir "%PKG_DIR_514%"

echo [1/5] Configuring CMake for Qt 5.14...
set "QTDIR=%QT514_DIR%"
set "PATH=%QT514_DIR%\bin;%PATH%"
cmake -B "%BUILD_DIR_514%" -A x64 -DCMAKE_INSTALL_PREFIX="%PKG_DIR_514%" -DCMAKE_PREFIX_PATH="%QT514_DIR%"
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake configure failed for Qt 5.14
    if "%CI%"=="" (
    rem pause when run interactively
    timeout /t 3 >nul 2>&1 || pause
)
    exit /b 1
)

echo [2/5] Building Release target...
cmake --build "%BUILD_DIR_514%" --config Release --target install
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Build failed for Qt 5.14
    if "%CI%"=="" (
    rem pause when run interactively
    timeout /t 3 >nul 2>&1 || pause
)
    exit /b 1
)

echo [3/5] Deploying Qt 5.14 runtime dependencies...
"%QT514_DIR%\bin\windeployqt.exe" --qmldir "%PROJECT_DIR%src\qml" "%PKG_DIR_514%\model-precheck.exe"
if %ERRORLEVEL% neq 0 (
    echo [ERROR] windeployqt failed for Qt 5.14
    if "%CI%"=="" (
    rem pause when run interactively
    timeout /t 3 >nul 2>&1 || pause
)
    exit /b 1
)

echo [4/5] Copying prerequisites and sample models...
set "PREREQ_SRC=%PROJECT_DIR%deploy\prerequisites"
set "PREREQ_DST=%PKG_DIR_514%\prerequisites"
if not exist "%PREREQ_DST%" mkdir "%PREREQ_DST%"
copy /Y "%PREREQ_SRC%\README.md" "%PKG_DIR_514%\README.md" >nul
copy /Y "%PREREQ_SRC%\README.md" "%PREREQ_DST%\README.md" >nul
copy /Y "%PREREQ_SRC%\install_build_tools.bat" "%PREREQ_DST%\install_build_tools.bat" >nul
copy /Y "%PREREQ_SRC%\download_prerequisites.ps1" "%PREREQ_DST%\download_prerequisites.ps1" >nul
if exist "%PREREQ_SRC%\vc_redist.x64.exe" copy /Y "%PREREQ_SRC%\vc_redist.x64.exe" "%PREREQ_DST%\vc_redist.x64.exe" >nul
if exist "%PREREQ_SRC%\vs_BuildTools.exe" copy /Y "%PREREQ_SRC%\vs_BuildTools.exe" "%PREREQ_DST%\vs_BuildTools.exe" >nul

xcopy /E /I /Y "%PROJECT_DIR%sample_models" "%PKG_DIR_514%\sample_models" >nul

echo [5/6] Verifying Qt 5.14 runtime startup smoke test...
powershell -NoProfile -ExecutionPolicy Bypass -Command "$psi = New-Object System.Diagnostics.ProcessStartInfo; $psi.FileName = '%PKG_DIR_514%\model-precheck.exe'; $psi.WorkingDirectory = '%PKG_DIR_514%'; $psi.UseShellExecute = $false; $p = [System.Diagnostics.Process]::Start($psi); $exited = $p.WaitForExit(2000); if ($exited) { Write-Host '[ERROR] Qt 5.14 model-precheck.exe crashed or exited prematurely with code:' $p.ExitCode; exit 1 } else { $p.Kill(); Write-Host '[OK] Qt 5.14 runtime verification passed.'; exit 0 }"
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Smoke test failed for Qt 5.14 package! Aborting package creation.
    exit /b 1
)

echo [6/6] Creating ZIP archive...
if exist "%PKG_DIR_514%.zip" del /F /Q "%PKG_DIR_514%.zip"
powershell -NoProfile -Command "Compress-Archive -Path '%PKG_DIR_514%' -DestinationPath '%PKG_DIR_514%.zip' -Force"
echo [OK] Qt 5.14 Package Created: %PKG_DIR_514%.zip
echo.
exit /b 0

REM ============================================================
REM Packaging Completion Summary
REM ============================================================
:finish_packaging
echo ============================================================
echo   Packaging Finished Successfully
echo   Output packages in: %DIST_ROOT%
if exist "%DIST_ROOT%\model-precheck_Qt5.11_x64.zip" echo   - %DIST_ROOT%\model-precheck_Qt5.11_x64.zip
if exist "%DIST_ROOT%\model-precheck_Qt5.14_x64.zip" echo   - %DIST_ROOT%\model-precheck_Qt5.14_x64.zip
echo ============================================================
echo.

if "%CI%"=="" (
    rem pause when run interactively
    timeout /t 3 >nul 2>&1 || pause
)
