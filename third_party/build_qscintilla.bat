@echo off
REM Build static QScintilla for ModelPrecheck.
setlocal EnableDelayedExpansion

REM Dynamically discover Qt directory
if not "%~1"=="" if exist "%~1\bin\qmake.exe" set "QTDIR=%~1"
if not defined QTDIR if defined QT_DIR set "QTDIR=%QT_DIR%"
if not defined QTDIR if defined ThirdPart if exist "%ThirdPart%\Qt5.14\vc143\x64\bin\qmake.exe" set "QTDIR=%ThirdPart%\Qt5.14\vc143\x64"
if not defined QTDIR if defined ThirdPart if exist "%ThirdPart%\Qt5.14\msvc2017_64\bin\qmake.exe" set "QTDIR=%ThirdPart%\Qt5.14\msvc2017_64"
if not defined QTDIR if defined ThirdPart if exist "%ThirdPart%\Qt5.11\vc140\x64\bin\qmake.exe" set "QTDIR=%ThirdPart%\Qt5.11\vc140\x64"
if not defined QTDIR if defined THIRDPARTY if exist "%THIRDPARTY%\Qt5.14\vc143\x64\bin\qmake.exe" set "QTDIR=%THIRDPARTY%\Qt5.14\vc143\x64"
if not defined QTDIR if defined THIRDPARTY if exist "%THIRDPARTY%\Qt5.11\vc140\x64\bin\qmake.exe" set "QTDIR=%THIRDPARTY%\Qt5.11\vc140\x64"

REM Dynamically discover vcvars64.bat
if not defined VCVARS if defined VCINSTALLDIR if exist "%VCINSTALLDIR%\Auxiliary\Build\vcvars64.bat" set "VCVARS=%VCINSTALLDIR%\Auxiliary\Build\vcvars64.bat"
if not defined VCVARS (
    set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
    if exist "!VSWHERE!" (
        for /f "usebackq tokens=*" %%i in (`"!VSWHERE!" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
            if exist "%%i\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=%%i\VC\Auxiliary\Build\vcvars64.bat"
        )
    )
)
if not defined VCVARS (
    if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat"
)

if not defined QTDIR (
    echo [ERROR] QTDIR not set or found.
    exit /b 1
)
if not defined VCVARS (
    echo [ERROR] vcvars64.bat not found.
    exit /b 1
)

set SRC=%~dp0QScintilla_src-2.14.1\src
set INST=%~dp0qscintilla-install

call "%VCVARS%"
if errorlevel 1 exit /b 1
cd /d "%SRC%"
"%QTDIR%\bin\qmake.exe" CONFIG+=staticlib
if errorlevel 1 exit /b 1
nmake release
if errorlevel 1 exit /b 1

if not exist "%INST%\include" mkdir "%INST%\include"
if not exist "%INST%\lib" mkdir "%INST%\lib"
xcopy /E /I /Y "%SRC%\Qsci" "%INST%\include\Qsci" >nul
copy /Y "%SRC%\release\qscintilla2_qt5.lib" "%INST%\lib\" >nul
echo Installed to %INST%
endlocal
