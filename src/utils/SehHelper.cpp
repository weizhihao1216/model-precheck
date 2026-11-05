#include "SehHelper.h"
#include <atomic>
#include <sstream>
#include <malloc.h>

std::string SehCodeToString(DWORD code) {
    switch (code) {
    case EXCEPTION_ACCESS_VIOLATION:
        return "EXCEPTION_ACCESS_VIOLATION (0xC0000005): Memory read/write access violation";
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED (0xC000008C): Out of bounds array index";
    case EXCEPTION_DATATYPE_MISALIGNMENT:
        return "EXCEPTION_DATATYPE_MISALIGNMENT (0xC000008E): Data alignment fault";
    case EXCEPTION_FLT_DENORMAL_OPERAND:
        return "EXCEPTION_FLT_DENORMAL_OPERAND (0xC000008D): Floating point denormal operand";
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        return "EXCEPTION_FLT_DIVIDE_BY_ZERO (0xC000008E): Floating point divide by zero";
    case EXCEPTION_FLT_INEXACT_RESULT:
        return "EXCEPTION_FLT_INEXACT_RESULT (0xC000008F): Floating point inexact result";
    case EXCEPTION_FLT_INVALID_OPERATION:
        return "EXCEPTION_FLT_INVALID_OPERATION (0xC0000090): Floating point invalid operation";
    case EXCEPTION_FLT_OVERFLOW:
        return "EXCEPTION_FLT_OVERFLOW (0xC0000091): Floating point overflow";
    case EXCEPTION_FLT_STACK_CHECK:
        return "EXCEPTION_FLT_STACK_CHECK (0xC0000092): Floating point stack overflow/underflow";
    case EXCEPTION_FLT_UNDERFLOW:
        return "EXCEPTION_FLT_UNDERFLOW (0xC0000093): Floating point underflow";
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        return "EXCEPTION_INT_DIVIDE_BY_ZERO (0xC0000094): Integer divide by zero";
    case EXCEPTION_INT_OVERFLOW:
        return "EXCEPTION_INT_OVERFLOW (0xC0000095): Integer overflow";
    case EXCEPTION_ILLEGAL_INSTRUCTION:
        return "EXCEPTION_ILLEGAL_INSTRUCTION (0xC000001D): Illegal CPU instruction";
    case EXCEPTION_IN_PAGE_ERROR:
        return "EXCEPTION_IN_PAGE_ERROR (0xC0000006): Page fault / I/O error";
    case EXCEPTION_STACK_OVERFLOW:
        return "EXCEPTION_STACK_OVERFLOW (0xC00000FD): Stack overflow exception";
    default: {
        std::stringstream ss;
        ss << "UNKNOWN_HARDWARE_EXCEPTION (0x" << std::hex << code << ")";
        return ss.str();
    }
    }
}

static DWORD FilterSehException(DWORD code) {
    return EXCEPTION_EXECUTE_HANDLER;
}

static inline void CheckAndResetStackOverflow(DWORD code) {
    if (code == EXCEPTION_STACK_OVERFLOW) {
        _resetstkoflw();
    }
}

bool SafeCallVoidNoArg(FnVoidNoArg fn, DWORD* outExceptionCode) {
    if (!fn) return false;
    __try {
        fn();
        if (outExceptionCode) *outExceptionCode = 0;
        return true;
    }
    __except (FilterSehException(GetExceptionCode())) {
        DWORD code = GetExceptionCode();
        CheckAndResetStackOverflow(code);
        if (outExceptionCode) *outExceptionCode = code;
        return false;
    }
}

bool SafeCallIntNoArg(FnIntNoArg fn, int* outResult, DWORD* outExceptionCode) {
    if (!fn) return false;
    __try {
        int res = fn();
        if (outResult) *outResult = res;
        if (outExceptionCode) *outExceptionCode = 0;
        return true;
    }
    __except (FilterSehException(GetExceptionCode())) {
        DWORD code = GetExceptionCode();
        CheckAndResetStackOverflow(code);
        if (outExceptionCode) *outExceptionCode = code;
        return false;
    }
}

bool SafeCallInit(FnModelInit fn, const WeaponModelParams* params, int* outResult, DWORD* outExceptionCode) {
    if (!fn) return false;
    __try {
        int res = fn(params);
        if (outResult) *outResult = res;
        if (outExceptionCode) *outExceptionCode = 0;
        return true;
    }
    __except (FilterSehException(GetExceptionCode())) {
        DWORD code = GetExceptionCode();
        CheckAndResetStackOverflow(code);
        if (outExceptionCode) *outExceptionCode = code;
        return false;
    }
}

bool SafeCallStep(FnModelStep fn, WeaponModelOutput* output, int* outResult, DWORD* outExceptionCode) {
    if (!fn) return false;
    __try {
        int res = fn(output);
        if (outResult) *outResult = res;
        if (outExceptionCode) *outExceptionCode = 0;
        return true;
    }
    __except (FilterSehException(GetExceptionCode())) {
        DWORD code = GetExceptionCode();
        CheckAndResetStackOverflow(code);
        if (outExceptionCode) *outExceptionCode = code;
        return false;
    }
}

bool SafeCallDestroy(FnModelDestroy fn, DWORD* outExceptionCode) {
    if (!fn) return false;
    __try {
        fn();
        if (outExceptionCode) *outExceptionCode = 0;
        return true;
    }
    __except (FilterSehException(GetExceptionCode())) {
        DWORD code = GetExceptionCode();
        CheckAndResetStackOverflow(code);
        if (outExceptionCode) *outExceptionCode = code;
        return false;
    }
}

static bool SafeCallGetInfoRaw(FnModelGetInfo fn, const char** outStr, DWORD* outExceptionCode) {
    if (!fn || !outStr) return false;
    __try {
        *outStr = fn();
        if (outExceptionCode) *outExceptionCode = 0;
        return true;
    }
    __except (FilterSehException(GetExceptionCode())) {
        DWORD code = GetExceptionCode();
        CheckAndResetStackOverflow(code);
        if (outExceptionCode) *outExceptionCode = code;
        *outStr = nullptr;
        return false;
    }
}

static bool SafeCopyCStringGuarded(const char* src, char* dst, size_t maxBytes, size_t* outLen, DWORD* outExceptionCode) {
    if (outLen) *outLen = 0;
    if (!src || !dst || maxBytes == 0) return false;
    __try {
        size_t i = 0;
        while (i + 1 < maxBytes && src[i] != '\0') {
            dst[i] = src[i];
            ++i;
        }
        dst[i] = '\0';
        if (outLen) *outLen = i;
        if (outExceptionCode) *outExceptionCode = 0;
        return true;
    }
    __except (FilterSehException(GetExceptionCode())) {
        DWORD code = GetExceptionCode();
        CheckAndResetStackOverflow(code);
        if (outExceptionCode) *outExceptionCode = code;
        return false;
    }
}

bool SafeCallGetInfo(FnModelGetInfo fn, std::string& outInfo, DWORD* outExceptionCode) {
    outInfo.clear();
    if (!fn) return false;
    const char* str = nullptr;
    DWORD fnExc = 0;
    if (!SafeCallGetInfoRaw(fn, &str, &fnExc)) {
        if (outExceptionCode) *outExceptionCode = fnExc;
        return false;
    }
    if (!str) {
        if (outExceptionCode) *outExceptionCode = 0;
        return true;
    }

    char buffer[4096];
    size_t copyLen = 0;
    DWORD copyExc = 0;
    if (!SafeCopyCStringGuarded(str, buffer, sizeof(buffer), &copyLen, &copyExc)) {
        if (outExceptionCode) *outExceptionCode = copyExc;
        return false;
    }
    outInfo.assign(buffer, copyLen);
    if (outExceptionCode) *outExceptionCode = 0;
    return true;
}

bool SafeCallCreate(FnModelCreate fn, ModelHandle* outHandle, DWORD* outExceptionCode) {
    if (!fn || !outHandle) return false;
    __try {
        *outHandle = fn();
        if (outExceptionCode) *outExceptionCode = 0;
        return (*outHandle != nullptr);
    }
    __except (FilterSehException(GetExceptionCode())) {
        DWORD code = GetExceptionCode();
        CheckAndResetStackOverflow(code);
        if (outExceptionCode) *outExceptionCode = code;
        *outHandle = nullptr;
        return false;
    }
}

bool SafeCallInitEx(FnModelInitEx fn, ModelHandle handle, const WeaponModelParams* params, int* outResult, DWORD* outExceptionCode) {
    if (!fn || !handle) return false;
    __try {
        int res = fn(handle, params);
        if (outResult) *outResult = res;
        if (outExceptionCode) *outExceptionCode = 0;
        return true;
    }
    __except (FilterSehException(GetExceptionCode())) {
        DWORD code = GetExceptionCode();
        CheckAndResetStackOverflow(code);
        if (outExceptionCode) *outExceptionCode = code;
        return false;
    }
}

bool SafeCallStepEx(FnModelStepEx fn, ModelHandle handle, WeaponModelOutput* output, int* outResult, DWORD* outExceptionCode) {
    if (!fn || !handle) return false;
    __try {
        int res = fn(handle, output);
        if (outResult) *outResult = res;
        if (outExceptionCode) *outExceptionCode = 0;
        return true;
    }
    __except (FilterSehException(GetExceptionCode())) {
        DWORD code = GetExceptionCode();
        CheckAndResetStackOverflow(code);
        if (outExceptionCode) *outExceptionCode = code;
        return false;
    }
}

bool SafeCallDestroyEx(FnModelDestroyEx fn, ModelHandle handle, DWORD* outExceptionCode) {
    if (!fn || !handle) return false;
    __try {
        fn(handle);
        if (outExceptionCode) *outExceptionCode = 0;
        return true;
    }
    __except (FilterSehException(GetExceptionCode())) {
        DWORD code = GetExceptionCode();
        CheckAndResetStackOverflow(code);
        if (outExceptionCode) *outExceptionCode = code;
        return false;
    }
}

std::wstring Utf8ToWidePath(const std::string& utf8) {
    if (utf8.empty()) return std::wstring();
    const int n = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if (n <= 1) return std::wstring();
    std::wstring wide(static_cast<size_t>(n - 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wide[0], n);
    return wide;
}

namespace {

std::atomic<bool> g_loadedPrivateModule(false);
std::atomic<unsigned> g_privateCopySeq(0);

std::string DirectoryOfPath(const std::string& path) {
    const size_t slash = path.find_last_of("\\/");
    return slash == std::string::npos ? std::string() : path.substr(0, slash);
}

std::string FileStemOfPath(const std::string& path) {
    const size_t slash = path.find_last_of("\\/");
    const std::string name = (slash == std::string::npos) ? path : path.substr(slash + 1);
    const size_t dot = name.find_last_of('.');
    std::string stem = (dot == std::string::npos) ? name : name.substr(0, dot);
    const size_t runPos = stem.find("__run");
    if (runPos != std::string::npos) {
        stem = stem.substr(0, runPos);
    }
    return stem;
}

/** 独立函数里用 SEH 兜住加载异常（带 C++ 对象的函数不能直接写 __try）。 */
HMODULE LoadLibraryGuarded(const wchar_t* path) {
    HMODULE module = nullptr;
    __try {
        module = LoadLibraryExW(path, nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
        if (!module) module = LoadLibraryW(path);
    } __except (FilterSehException(GetExceptionCode())) {
        DWORD code = GetExceptionCode();
        CheckAndResetStackOverflow(code);
        module = nullptr;
    }
    return module;
}

static void RecursiveCleanRunDlls(const std::wstring& dir) {
    if (dir.empty()) return;
    const std::wstring pattern = dir + L"\\*";
    WIN32_FIND_DATAW data{};
    HANDLE hFind = FindFirstFileW(pattern.c_str(), &data);
    if (hFind == INVALID_HANDLE_VALUE) return;
    do {
        const std::wstring name = data.cFileName;
        if (name == L"." || name == L"..") continue;
        const std::wstring fullPath = dir + L"\\" + name;
        if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
            RecursiveCleanRunDlls(fullPath);
        } else {
            if (name.size() > 4 && _wcsicmp(name.c_str() + name.size() - 4, L".dll") == 0
                && name.find(L"__run") != std::wstring::npos) {
                DeleteFileW(fullPath.c_str());
            }
        }
    } while (FindNextFileW(hFind, &data));
    FindClose(hFind);
}

} // namespace

HMODULE LoadPrivateModuleCopy(const std::string& sourcePath, const std::string& copyDir,
                              std::string* outCopyPath, std::string* error) {
    if (sourcePath.empty()) {
        if (error) *error = "源 DLL 路径为空";
        return nullptr;
    }
    const std::string dir = copyDir.empty() ? DirectoryOfPath(sourcePath) : copyDir;
    const std::string stem = FileStemOfPath(sourcePath);
    if (dir.empty() || stem.empty()) {
        if (error) *error = "无法解析 DLL 路径：" + sourcePath;
        return nullptr;
    }

    const std::string copyPath = dir + "\\" + stem + "__run"
        + std::to_string(GetCurrentProcessId()) + "_"
        + std::to_string(++g_privateCopySeq) + ".dll";
    const std::wstring wSource = Utf8ToWidePath(sourcePath);
    const std::wstring wCopy = Utf8ToWidePath(copyPath);
    if (wSource.empty() || wCopy.empty() || !CopyFileW(wSource.c_str(), wCopy.c_str(), FALSE)) {
        if (error) *error = "复制被测模块失败, GetLastError=" + std::to_string(GetLastError());
        return nullptr;
    }

    HMODULE module = LoadLibraryGuarded(wCopy.c_str());
    if (!module) {
        // 没有加载成功，副本文件可以直接删掉，不必留到下次启动。
        DeleteFileW(wCopy.c_str());
        if (error) *error = "加载被测模块副本失败, GetLastError=" + std::to_string(GetLastError());
        return nullptr;
    }

    g_loadedPrivateModule.store(true);
    if (outCopyPath) *outCopyPath = copyPath;
    return module;
}

bool HasLoadedPrivateModule() {
    return g_loadedPrivateModule.load();
}

void CleanupPrivateModuleCopies(const std::string& modelsRootDir) {
    if (modelsRootDir.empty()) return;
    std::wstring wDir = Utf8ToWidePath(modelsRootDir);
    RecursiveCleanRunDlls(wDir);
}
