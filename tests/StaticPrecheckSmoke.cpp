// Smoke: run PackageScanner + Header/Lib/PE analyzers on all dist/sample_models
// without Qt UI. Build: cmake --build build --config Release --target StaticPrecheckSmoke
#include "../src/core/PackageScanner.h"
#include "../src/utils/SehHelper.h"
#include "../src/core/HeaderAnalyzer.h"
#include "../src/core/LibAnalyzer.h"
#include "../src/core/PeAnalyzer.h"
#include "../src/core/FunctionalVerifier.h"
#include "../src/utils/QtEncoding.h"

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QTextStream>
#include <iostream>
#include <string>
#include <vector>

static void AnalyzePackage(const QString& pkgDir, QTextStream& out) {
    out << "==== " << pkgDir << QT_ENDL;
    out.flush();
    ModelPackageFiles pkg = PackageScanner::ScanPackageDirectory(qToUtf8(pkgDir));
    out << "  headers=" << pkg.allHeaderFiles.size()
        << " libs=" << pkg.allLibFiles.size()
        << " dlls=" << pkg.allDllFiles.size() << QT_ENDL;
    out.flush();

    for (const auto& h : pkg.allHeaderFiles) {
        out << "  HDR " << qUtf8(h) << QT_ENDL;
        out.flush();
        (void)HeaderAnalyzer::AnalyzeHeader(h);
    }
    out << "  HDR-SET..." << QT_ENDL;
    out.flush();
    (void)HeaderAnalyzer::AnalyzeHeaderSet(pkg.allHeaderFiles);

    for (const auto& l : pkg.allLibFiles) {
        out << "  LIB " << qUtf8(l) << QT_ENDL;
        out.flush();
        (void)LibAnalyzer::AnalyzeLib(l);
    }
    for (const auto& d : pkg.allDllFiles) {
        out << "  PE  " << qUtf8(d) << QT_ENDL;
        out.flush();
        std::vector<std::string> search;
        search.push_back(qToUtf8(pkgDir));
        (void)PeAnalyzer::AnalyzeDll(d, search, {});
    }
    out << "  OK " << QDir(pkgDir).dirName() << QT_ENDL;
    out.flush();
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);
    out.setCodec("UTF-8");

    QString root = QCoreApplication::applicationDirPath() + "/sample_models";
    if (argc >= 2) root = QString::fromLocal8Bit(argv[1]);
    out << "Root: " << root << QT_ENDL;

    QDir dir(root);
    if (!dir.exists()) {
        out << "FAIL: root missing" << QT_ENDL;
        return 2;
    }
    CleanupPrivateModuleCopies(dir.absolutePath().toUtf8().toStdString());

    const QFileInfoList pkgs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo& fi : pkgs) {
        AnalyzePackage(fi.absoluteFilePath(), out);
    }

    // Cross-model header set like one-click precheck
    std::vector<std::string> allHeaders;
    for (const QFileInfo& fi : pkgs) {
        ModelPackageFiles pkg =
            PackageScanner::ScanPackageDirectory(qToUtf8(fi.absoluteFilePath()));
        allHeaders.insert(allHeaders.end(),
                          pkg.allHeaderFiles.begin(), pkg.allHeaderFiles.end());
    }
    out << "CROSS headers=" << allHeaders.size() << QT_ENDL;
    out.flush();
    (void)HeaderAnalyzer::AnalyzeHeaderSet(allHeaders);
    // Test FunctionalVerifier angle unit heuristic
    {
        // 1. Small angle level flight (pitch 2 deg, yaw 1.5 deg): should be DEGREES with no warnings
        std::vector<WeaponModelOutput> levelFlight(10);
        for (int i = 0; i < 10; ++i) {
            levelFlight[i].sim_time = i * 0.1;
            levelFlight[i].lat = 30.0 + i * 0.001;
            levelFlight[i].lon = 110.0 + i * 0.001;
            levelFlight[i].alt = 1000.0;
            levelFlight[i].vx = 300.0;
            levelFlight[i].pitch = 2.0;  // 2 degrees
            levelFlight[i].yaw = 1.5;    // 1.5 degrees
            levelFlight[i].roll = 0.0;
        }
        auto repLevel = FunctionalVerifier::VerifyTrajectory(levelFlight);
        if (!repLevel.isDegreeUnit || !repLevel.warnings.empty()) {
            out << "FAIL: Level flight false positive warning on degrees: isDegree="
                << repLevel.isDegreeUnit << " warns=" << repLevel.warnings.size() << QT_ENDL;
            return 3;
        }

        // 2. Trajectory with yaw wrap around ±pi: should flag suspicious radians
        std::vector<WeaponModelOutput> radWrap(10);
        for (int i = 0; i < 10; ++i) {
            radWrap[i].sim_time = i * 0.1;
            radWrap[i].lat = 30.0;
            radWrap[i].lon = 110.0;
            radWrap[i].alt = 1000.0;
            radWrap[i].vx = 300.0;
            radWrap[i].pitch = 0.05;
            radWrap[i].yaw = (i < 5) ? 3.10 : -3.10;
        }
        auto repWrap = FunctionalVerifier::VerifyTrajectory(radWrap);
        if (repWrap.isDegreeUnit || repWrap.warnings.empty()) {
            out << "FAIL: Radian wrap not detected!" << QT_ENDL;
            return 4;
        }

        out << "VERIFY_TRAJECTORY: PASS (level-flight degree & radian wrap tests passed)" << QT_ENDL;
    }

    out << "ALL DONE" << QT_ENDL;
    return 0;
}
