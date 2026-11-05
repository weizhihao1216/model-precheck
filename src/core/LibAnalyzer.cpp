#include "LibAnalyzer.h"
#include "../utils/SehHelper.h"
#include <fstream>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <cctype>

#ifndef IMAGE_FILE_MACHINE_AMD64
#define IMAGE_FILE_MACHINE_AMD64 0x8664
#endif
#ifndef IMAGE_FILE_MACHINE_I386
#define IMAGE_FILE_MACHINE_I386 0x014c
#endif

LibAnalysisReport LibAnalyzer::AnalyzeLib(const std::string& libPath,
                                         const std::vector<std::string>& requiredExports) {
    LibAnalysisReport report;
    report.filePath = libPath;
    report.architecture = "Unknown";
    report.is64Bit = false;
    report.libType = "Unknown";
    report.isArchMatch = false;
    report.overallPass = false;

    const std::wstring wLibPath = Utf8ToWidePath(libPath);
    std::ifstream file(wLibPath.c_str(), std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        file.open(libPath.c_str(), std::ios::binary | std::ios::ate);
    }
    if (!file.is_open()) {
        report.logMessages.push_back("FAIL: 无法打开 LIB 文件: " + libPath);
        return report;
    }

    const std::streamsize sizeSigned = file.tellg();
    file.seekg(0, std::ios::beg);
    if (sizeSigned < 8) {
        report.logMessages.push_back("FAIL: LIB 文件大小无效 (< 8 字节)");
        return report;
    }
    const size_t contentSize = static_cast<size_t>(sizeSigned);
    std::string content(contentSize, '\0');
    if (!file.read(&content[0], sizeSigned)) {
        report.logMessages.push_back("FAIL: 读取 LIB 文件内容失败: " + libPath);
        return report;
    }
    file.close();

    // 1. Verify COFF Archive Header "!<arch>\n"
    const char magic[8] = {'!', '<', 'a', 'r', 'c', 'h', '>', '\n'};
    if (content.size() < 8 || std::memcmp(content.data(), magic, 8) != 0) {
        report.logMessages.push_back("FAIL: 文件非标准 COFF .lib 归档格式 (缺少 !<arch> 标志头)");
        return report;
    }

    report.logMessages.push_back("PASS: 格式符合标准 COFF .lib 归档规范");

    // 2. Machine Architecture & Import/Static Lib Detection via COFF Archive Members
    bool foundX64 = false;
    bool foundX86 = false;
    bool isImportLib = false;

    // Scan COFF archive members by standard 60-byte member headers
    size_t offset = 8;
    constexpr size_t kMemberHeaderSize = 60;
    int inspectedMembers = 0;

    while (offset + kMemberHeaderSize <= content.size() && inspectedMembers < 64) {
        const char* pMemberHdr = content.data() + offset;
        // Verify end of header signature "`\n"
        if (pMemberHdr[58] != '`' || pMemberHdr[59] != '\n') {
            break;
        }

        char sizeBuf[11] = {0};
        std::memcpy(sizeBuf, pMemberHdr + 48, 10);
        size_t memberSize = static_cast<size_t>(std::strtoul(sizeBuf, nullptr, 10));

        const size_t dataOffset = offset + kMemberHeaderSize;
        if (dataOffset > content.size() || memberSize > content.size() - dataOffset) {
            break;
        }

        const char* nameField = pMemberHdr;
        bool isLinkerMember = (nameField[0] == '/' && (nameField[1] == ' ' || nameField[1] == '/' || std::isdigit(static_cast<unsigned char>(nameField[1]))));

        if (!isLinkerMember && memberSize >= 4) {
            inspectedMembers++;
            const uint8_t* pData = reinterpret_cast<const uint8_t*>(content.data() + dataOffset);
            // Check for Short Import Header (Sig1 == 0x0000, Sig2 == 0xFFFF)
            uint16_t sig1 = *reinterpret_cast<const uint16_t*>(pData);
            uint16_t sig2 = *reinterpret_cast<const uint16_t*>(pData + 2);
            if (sig1 == 0x0000 && sig2 == 0xFFFF && memberSize >= 20) {
                isImportLib = true;
                uint16_t machine = *reinterpret_cast<const uint16_t*>(pData + 6);
                if (machine == IMAGE_FILE_MACHINE_AMD64) foundX64 = true;
                else if (machine == IMAGE_FILE_MACHINE_I386) foundX86 = true;
            } else if (memberSize >= 20) {
                // Standard COFF Object Header: Machine is at offset 0
                uint16_t machine = *reinterpret_cast<const uint16_t*>(pData);
                if (machine == IMAGE_FILE_MACHINE_AMD64) foundX64 = true;
                else if (machine == IMAGE_FILE_MACHINE_I386) foundX86 = true;
            }
            if (foundX64 || foundX86) {
                break;
            }
        }

        // Members are 2-byte aligned in COFF archive
        offset = dataOffset + memberSize + (memberSize % 2);
    }

    // Fallback: If archive member loop did not detect machine, scan for short import header signatures
    if (!foundX64 && !foundX86) {
        for (size_t i = 8; i + 20 < content.size(); i += 2) {
            uint16_t s1 = *reinterpret_cast<const uint16_t*>(content.data() + i);
            uint16_t s2 = *reinterpret_cast<const uint16_t*>(content.data() + i + 2);
            if (s1 == 0x0000 && s2 == 0xFFFF) {
                isImportLib = true;
                uint16_t m = *reinterpret_cast<const uint16_t*>(content.data() + i + 6);
                if (m == IMAGE_FILE_MACHINE_AMD64) { foundX64 = true; break; }
                else if (m == IMAGE_FILE_MACHINE_I386) { foundX86 = true; break; }
            }
        }
    }

    if (foundX64) {
        report.architecture = "x64";
        report.is64Bit = true;
        report.isArchMatch = true;
        report.logMessages.push_back("PASS: 目标 CPU 架构为 x64 (与 64 位仿真引擎相匹配)");
    } else if (foundX86) {
        report.architecture = "x86";
        report.is64Bit = false;
        report.isArchMatch = false;
        report.logMessages.push_back("FAIL: 目标 CPU 架构为 x86 (32位，与 64 位仿真引擎不一致!)");
    } else {
        report.architecture = "x64"; // Default fallback if unspecified
        report.is64Bit = true;
        report.isArchMatch = true;
        report.logMessages.push_back("INFO: 未提取到明确 Machine 标志，推断为通用 x64 归档");
    }

    if (isImportLib) {
        report.libType = "Import Library (.lib 动态库导入表)";
        report.logMessages.push_back("INFO: 鉴定库类型: DLL 动态链接导出表 (Import Library)");
    } else {
        report.libType = "Static Library (.lib 静态链接库)";
        report.logMessages.push_back("INFO: 鉴定库类型: 静态代码库 (Static Library)");
    }

    // 3. Symbol Search (Informational)
    for (const auto& funcName : requiredExports) {
        if (!funcName.empty() && content.find(funcName) != std::string::npos) {
            report.foundSymbols.push_back(funcName);
            report.logMessages.push_back("INFO: LIB 库包含符号: " + funcName);
        }
    }

    report.logMessages.push_back("INFO: LIB 库文件解包与 COFF 结构解析完毕");
    report.overallPass = report.isArchMatch;
    return report;
}
