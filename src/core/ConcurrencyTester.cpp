#include "ConcurrencyTester.h"
#include "../utils/QtEncoding.h"
#include <thread>
#include <atomic>
#include <chrono>
#include <sstream>
#include <algorithm>
#include <cmath>

static void FinalizeReport(ConcurrencyTestReport& report) {
    report.crashed = report.exceptionCount > 0;
    report.multiThreadSafe = (report.mode == ConcurrencyTestMode::MultiThread)
        ? (report.exceptionCount == 0 && report.dataRaceCount == 0 && report.userFailCount == 0) : false;
    report.multiModelOk = (report.mode == ConcurrencyTestMode::MultiModel)
        ? (report.exceptionCount == 0 && report.dataRaceCount == 0 && report.userFailCount == 0) : false;

    if (report.exceptionCount > 0) {
        report.verdict = "FAIL";
        report.summary = (report.mode == ConcurrencyTestMode::MultiModel)
            ? "多型号并行执行 UserMain 时发生 SEH 崩溃/硬件异常"
            : "多线程执行 UserMain 时发生 SEH 崩溃/硬件异常";
    } else if (report.dataRaceCount > 0) {
        report.verdict = "FAIL";
        report.summary = (report.mode == ConcurrencyTestMode::MultiModel)
            ? "检测到多型号并发数据竞态：并行输出与基准不一致（全局/静态状态串扰）"
            : "检测到多线程数据竞态：并发轨迹与单线程基准不一致（存在全局变量串扰，非线程安全）";
    } else if (report.userFailCount > 0) {
        report.verdict = "FAIL";
        report.summary = "部分 UserMain 返回失败码（无崩溃）";
    } else {
        report.verdict = "PASS";
        if (report.mode == ConcurrencyTestMode::MultiModel) {
            report.summary = "多型号并行完成且与基线一致（型号数=" + std::to_string(report.modelTypeCount)
                + ", 总实例=" + std::to_string(report.workerCount) + "）";
        } else {
            report.summary = "全部线程成功完成 UserMain 且无状态串扰（线程数="
                + std::to_string(report.workerCount) + "）";
        }
    }

    report.logMessages.push_back("INFO: 成功=" + std::to_string(report.successCount)
        + " 用户失败=" + std::to_string(report.userFailCount)
        + (report.dataRaceCount > 0 ? (" 竞态=" + std::to_string(report.dataRaceCount)) : "")
        + " 异常=" + std::to_string(report.exceptionCount)
        + " 判定=" + report.verdict);
}

ConcurrencyTestReport ConcurrencyTester::Run(UserCodeHarness& harness, const ConcurrencyTestConfig& config) {
    ConcurrencyTestReport report;
    report.mode = ConcurrencyTestMode::MultiThread;
    report.workerCount = config.count > 0 ? config.count : 1;

    if (!harness.IsLoaded()) {
        report.verdict = "FAIL";
        report.summary = "用户 UserMain 未编译/未加载，请先在「用户代码」页编译";
        report.logMessages.push_back("FAIL: " + report.summary);
        return report;
    }

    report.logMessages.push_back("INFO: 多线程测试 — 同时启动 " + std::to_string(report.workerCount)
        + " 个线程，各跑一次完整 UserMain 并与单线程基准核验");

    struct Baseline {
        int ret = 0;
        bool seh = false;
        double lat = 0.0;
        double lon = 0.0;
        double csum = 0.0;
        std::string err;
        bool ok = false;
    };
    std::vector<Baseline> baselines(static_cast<size_t>(report.workerCount));
    std::vector<RandomValueBlob> blobs(static_cast<size_t>(report.workerCount));

    // 1. 各线程独立种子的单线程基线运行
    for (int i = 0; i < report.workerCount; ++i) {
        uint32_t seed = config.randomSeed + static_cast<uint32_t>(i) * 9973u;
        blobs[static_cast<size_t>(i)] = harness.Sample(seed);
        Baseline& b = baselines[static_cast<size_t>(i)];
        b.ok = harness.RunOnce(blobs[static_cast<size_t>(i)],
                               &b.ret, &b.seh, b.err,
                               &b.lat, &b.lon, &b.csum);
    }

    // 2. 多线程并发同时起跑
    std::vector<ConcurrencyThreadResult> results(static_cast<size_t>(report.workerCount));
    std::vector<double> threadLats(static_cast<size_t>(report.workerCount), 0.0);
    std::vector<double> threadLons(static_cast<size_t>(report.workerCount), 0.0);
    std::vector<double> threadCsums(static_cast<size_t>(report.workerCount), 0.0);
    std::atomic<bool> anyCrash{ false };
    std::atomic<bool> startGate{ false };

    auto workerFn = [&](int id) {
        while (!startGate.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        int userRet = 0;
        bool seh = false;
        std::string err;
        double lat = 0.0, lon = 0.0, csum = 0.0;
        bool ok = harness.RunOnce(blobs[static_cast<size_t>(id)],
                                  &userRet, &seh, err,
                                  &lat, &lon, &csum);
        threadLats[static_cast<size_t>(id)] = lat;
        threadLons[static_cast<size_t>(id)] = lon;
        threadCsums[static_cast<size_t>(id)] = csum;

        ConcurrencyThreadResult tr;
        tr.threadId = id;
        tr.modelIndex = 0;
        tr.instanceId = id;
        tr.modelName = "default";
        tr.randomSummary = blobs[static_cast<size_t>(id)].summary;
        tr.userReturnCode = userRet;
        if (!ok || seh) {
            tr.exceptionOccurred = true;
            tr.errorLog = err.empty() ? "SEH 硬件异常" : err;
            anyCrash = true;
        } else if (userRet != 0) {
            tr.userReportedFail = true;
            tr.errorLog = "UserMain 返回非 0: " + std::to_string(userRet);
        }
        results[static_cast<size_t>(id)] = tr;
    };

    std::vector<std::thread> threads;
    threads.reserve(static_cast<size_t>(report.workerCount));
    for (int i = 0; i < report.workerCount; ++i) {
        threads.emplace_back(workerFn, i);
    }
    startGate.store(true, std::memory_order_release);
    for (auto& t : threads) t.join();

    // 3. 静默数据竞态比对
    for (int i = 0; i < report.workerCount; ++i) {
        ConcurrencyThreadResult& tr = results[static_cast<size_t>(i)];
        const Baseline& b = baselines[static_cast<size_t>(i)];
        if (!tr.exceptionOccurred && !tr.userReportedFail && b.ok && !b.seh && b.ret == 0) {
            double dLat = std::abs(threadLats[static_cast<size_t>(i)] - b.lat);
            double dLon = std::abs(threadLons[static_cast<size_t>(i)] - b.lon);
            double dCsum = std::abs(threadCsums[static_cast<size_t>(i)] - b.csum);
            double maxDev = (std::max)({ dLat, dLon, dCsum });
            tr.deviation = maxDev;

            // 阈值：经纬度容差 1e-5 (~1米)，累积轨迹校验和容差 1e-4
            if (dLat > 1e-5 || dLon > 1e-5 || dCsum > 1e-4) {
                tr.userReportedFail = true;
                tr.dataRaceDetected = true;
                std::ostringstream ss;
                ss << "检测到并发数据竞态（非线程安全全局串扰）：输出与单线程基准偏离 [dLat="
                   << dLat << ", dLon=" << dLon << ", dChecksum=" << dCsum << "]";
                tr.errorLog = ss.str();
            }
        }
    }

    report.threadResults = results;
    report.crashed = anyCrash.load();
    for (const auto& tr : results) {
        if (tr.exceptionOccurred) report.exceptionCount++;
        else if (tr.userReportedFail) {
            report.userFailCount++;
            if (tr.dataRaceDetected) report.dataRaceCount++;
        }
        else report.successCount++;

        if (!tr.errorLog.empty()) {
            report.logMessages.push_back("Thread#" + std::to_string(tr.threadId)
                + " [" + tr.randomSummary + "] " + tr.errorLog);
        } else {
            report.logMessages.push_back("Thread#" + std::to_string(tr.threadId)
                + " OK [" + tr.randomSummary + "] ret=" + std::to_string(tr.userReturnCode)
                + " (基准对比无偏差)");
        }
    }

    FinalizeReport(report);
    return report;
}

ConcurrencyTestReport ConcurrencyTester::RunMultiModel(const ConcurrencyTestConfig& config) {
    ConcurrencyTestReport report;
    report.mode = ConcurrencyTestMode::MultiModel;
    report.modelTypeCount = static_cast<int>(config.models.size());

    struct Job {
        UserCodeHarness* harness;
        int modelIndex;
        int instanceId;
        std::string modelName;
    };
    std::vector<Job> jobs;
    for (size_t mi = 0; mi < config.models.size(); ++mi) {
        const MultiModelSpec& spec = config.models[mi];
        if (!spec.harness || !spec.harness->IsLoaded()) {
            report.verdict = "FAIL";
            report.summary = "型号未编译/未加载: " + (spec.modelName.empty() ? ("#" + std::to_string(mi)) : spec.modelName);
            report.logMessages.push_back("FAIL: " + report.summary);
            return report;
        }
        int n = spec.count > 0 ? spec.count : 1;
        for (int k = 0; k < n; ++k) {
            Job j;
            j.harness = spec.harness;
            j.modelIndex = static_cast<int>(mi);
            j.instanceId = k;
            j.modelName = spec.modelName.empty() ? ("model" + std::to_string(mi)) : spec.modelName;
            jobs.push_back(j);
        }
    }

    report.workerCount = static_cast<int>(jobs.size());
    if (jobs.empty()) {
        report.verdict = "FAIL";
        report.summary = "未配置任何型号实例（请添加型号并设置数量）";
        report.logMessages.push_back("FAIL: " + report.summary);
        return report;
    }

    {
        std::ostringstream info;
        info << "INFO: 多型号并行 — 型号数=" << report.modelTypeCount << " 总实例=" << report.workerCount << " [";
        for (size_t mi = 0; mi < config.models.size(); ++mi) {
            if (mi) info << ", ";
            info << config.models[mi].modelName << "×" << config.models[mi].count;
        }
        info << "]";
        report.logMessages.push_back(info.str());
    }

    struct Baseline {
        int ret = 0;
        bool seh = false;
        double lat = 0.0;
        double lon = 0.0;
        double csum = 0.0;
        std::string err;
        bool ok = false;
    };
    std::vector<Baseline> baselines(jobs.size());
    std::vector<RandomValueBlob> blobs(jobs.size());

    // 1. 各实例单线程基线预跑
    for (size_t j = 0; j < jobs.size(); ++j) {
        const Job& job = jobs[j];
        uint32_t seed = config.randomSeed
            + static_cast<uint32_t>(job.modelIndex) * 100003u
            + static_cast<uint32_t>(job.instanceId) * 9973u;
        blobs[j] = job.harness->Sample(seed);
        Baseline& b = baselines[j];
        b.ok = job.harness->RunOnce(blobs[j], &b.ret, &b.seh, b.err, &b.lat, &b.lon, &b.csum);
    }

    // 2. 并行执行全部实例
    std::vector<ConcurrencyThreadResult> results(jobs.size());
    std::vector<double> threadLats(jobs.size(), 0.0);
    std::vector<double> threadLons(jobs.size(), 0.0);
    std::vector<double> threadCsums(jobs.size(), 0.0);
    std::atomic<bool> anyCrash{ false };
    std::atomic<bool> startGate{ false };

    auto workerFn = [&](int jobId) {
        while (!startGate.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        const Job& job = jobs[static_cast<size_t>(jobId)];
        int userRet = 0;
        bool seh = false;
        std::string err;
        double lat = 0.0, lon = 0.0, csum = 0.0;
        bool ok = job.harness->RunOnce(blobs[static_cast<size_t>(jobId)],
                                      &userRet, &seh, err,
                                      &lat, &lon, &csum);
        threadLats[static_cast<size_t>(jobId)] = lat;
        threadLons[static_cast<size_t>(jobId)] = lon;
        threadCsums[static_cast<size_t>(jobId)] = csum;

        ConcurrencyThreadResult tr;
        tr.threadId = jobId;
        tr.modelIndex = job.modelIndex;
        tr.instanceId = job.instanceId;
        tr.modelName = job.modelName;
        tr.randomSummary = blobs[static_cast<size_t>(jobId)].summary;
        tr.userReturnCode = userRet;
        if (!ok || seh) {
            tr.exceptionOccurred = true;
            tr.errorLog = err.empty() ? "SEH 硬件异常" : err;
            anyCrash = true;
        } else if (userRet != 0) {
            tr.userReportedFail = true;
            tr.errorLog = "UserMain 返回非 0: " + std::to_string(userRet);
        }
        results[static_cast<size_t>(jobId)] = tr;
    };

    std::vector<std::thread> threads;
    threads.reserve(jobs.size());
    for (int i = 0; i < static_cast<int>(jobs.size()); ++i) {
        threads.emplace_back(workerFn, i);
    }
    startGate.store(true, std::memory_order_release);
    for (auto& t : threads) t.join();

    // 3. 静默竞态比对
    for (size_t i = 0; i < jobs.size(); ++i) {
        ConcurrencyThreadResult& tr = results[i];
        const Baseline& b = baselines[i];
        if (!tr.exceptionOccurred && !tr.userReportedFail && b.ok && !b.seh && b.ret == 0) {
            double dLat = std::abs(threadLats[i] - b.lat);
            double dLon = std::abs(threadLons[i] - b.lon);
            double dCsum = std::abs(threadCsums[i] - b.csum);
            double maxDev = (std::max)({ dLat, dLon, dCsum });
            tr.deviation = maxDev;

            if (dLat > 1e-5 || dLon > 1e-5 || dCsum > 1e-4) {
                tr.userReportedFail = true;
                tr.dataRaceDetected = true;
                std::ostringstream ss;
                ss << "检测到多型号并发数据竞态（跨型号/跨实例全局串扰）：输出与基准偏离 [dLat="
                   << dLat << ", dLon=" << dLon << ", dChecksum=" << dCsum << "]";
                tr.errorLog = ss.str();
            }
        }
    }

    report.threadResults = results;
    report.crashed = anyCrash.load();
    for (const auto& tr : results) {
        if (tr.exceptionOccurred) report.exceptionCount++;
        else if (tr.userReportedFail) {
            report.userFailCount++;
            if (tr.dataRaceDetected) report.dataRaceCount++;
        }
        else report.successCount++;

        std::string tag = tr.modelName + "[" + std::to_string(tr.instanceId) + "]";
        if (!tr.errorLog.empty()) {
            report.logMessages.push_back(tag + " [" + tr.randomSummary + "] " + tr.errorLog);
        } else {
            report.logMessages.push_back(tag + " OK [" + tr.randomSummary + "] ret="
                + std::to_string(tr.userReturnCode) + " (基准对比无偏差)");
        }
    }

    FinalizeReport(report);
    return report;
}

ConcurrencyTestWorker::ConcurrencyTestWorker(UserCodeHarness* harness, const ConcurrencyTestConfig& config)
    : m_pHarness(harness)
    , m_config(config) {
}

void ConcurrencyTestWorker::process() {
    const bool isMulti = (m_config.mode == ConcurrencyTestMode::MultiModel);
    emit logMessage(QString("INFO: ") + (isMulti ? "多型号并行测试开始..." : "多线程（并行）测试开始..."));

    ConcurrencyTestReport report;
    if (isMulti) {
        report = ConcurrencyTester::RunMultiModel(m_config);
    } else {
        if (!m_pHarness) {
            report.verdict = "FAIL";
            report.summary = "Harness 为空";
            emit finished(report);
            return;
        }
        report = ConcurrencyTester::Run(*m_pHarness, m_config);
    }

    for (const auto& msg : report.logMessages) {
        emit logMessage(qDecodeLog(msg));
    }
    emit finished(report);
}
