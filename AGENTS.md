# AGENTS.md - Antigravity Agent Guidelines & Context

This file provides system context, tech stack rules, and component locations for AI coding assistants working on the **ModelPrecheck (第三方武器模型DLL集成预检工具)** codebase.

## 1. Core Specification & Architecture

- **Project Purpose**: Validate third-party C++ weapon model DLLs prior to simulation engine integration.
- **Primary Spec File**: See [`README.md`](file:///d:/HR/Game/ModelPrecheck/README.md) for full requirement mapping (F1-F13).
- **Environment**: Visual Studio 2022 (MSVC v143 x64) [Primary/Recommended] / VS 2017 (MSVC v141) [Compatible], Qt 5.14.2 x64 (resolved dynamically via %QTDIR% or %ThirdPart%), CMake 3.12+.

## 2. Mandatory Rules

1. **NO MINMAX MACRO CONTAMINATION**: Always define `NOMINMAX` before including `<windows.h>` to prevent `windows.h` `min`/`max` macros from corrupting Qt headers (`<QtCharts/QValueAxis>`).
2. **SEH HARDWARE EXCEPTION SAFETY**: All DLL invocations (`LoadLibrary`, `Model_Init`, `Model_Step`, `Model_Destroy`) MUST be executed via MSVC SEH wrappers (`SafeCallInit`, `SafeCallStep`) in [`src/utils/SehHelper.cpp`](file:///d:/HR/Game/ModelPrecheck/src/utils/SehHelper.cpp).
3. **DECOUPLE QTCHARTS FROM HEADERS**: Do NOT include `QT_CHARTS_USE_NAMESPACE` inside header (`.h`) files to prevent namespace pollution. Forward declare QtCharts classes or include QtCharts headers only in `.cpp` files.

## 3. Key Entry Points & Class Roles

- `PeAnalyzer`: [`src/core/PeAnalyzer.h`](file:///d:/HR/Game/ModelPrecheck/src/core/PeAnalyzer.h) - PE header parser (x86/x64, CRT MD/MT, Import/Export directory).
- `DllLoader`: [`src/core/DllLoader.h`](file:///d:/HR/Game/ModelPrecheck/src/core/DllLoader.h) - Dynamic loading & SEH function binding.
- `PerfProfiler`: [`src/core/PerfProfiler.h`](file:///d:/HR/Game/ModelPrecheck/src/core/PerfProfiler.h) - Multithreaded stress testing, jitter analysis, memory leak detection.
- `FunctionalVerifier`: [`src/core/FunctionalVerifier.h`](file:///d:/HR/Game/ModelPrecheck/src/core/FunctionalVerifier.h) - Trajectory NaN/Inf, bounds, and degree/radian unit checks.
- `ReportGenerator`: [`src/core/ReportGenerator.h`](file:///d:/HR/Game/ModelPrecheck/src/core/ReportGenerator.h) - HTML/PDF test report generation.

## 4. Build & Package Commands

```powershell
# One-click build script (defaults to best available Qt)
.\build.bat

# One-click packaging (model-precheck_v1.0.0_windows.zip)
.\package.bat

# Or manual CMake commands
cmake -G "Visual Studio 17 2022" -A x64 -B build_pkg -DCMAKE_INSTALL_PREFIX=dist\model-precheck_v1.0.0_windows -DCMAKE_PREFIX_PATH="%ThirdPart%\Qt5.14\vc143\x64"
cmake --build build_pkg --config Release --target install
```
