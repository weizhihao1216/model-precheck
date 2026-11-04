#include "FunctionalVerifier.h"
#include <cmath>
#include <limits>
#include <algorithm>
#include <sstream>

static bool IsInvalid(double val) {
    return std::isnan(val) || std::isinf(val);
}

TrajectoryVerificationReport FunctionalVerifier::VerifyTrajectory(const std::vector<WeaponModelOutput>& history) {
    TrajectoryVerificationReport report;
    report.totalDataPoints = static_cast<int>(history.size());

    if (history.empty()) {
        report.overallPass = false;
        report.errors.push_back("ERROR: Trajectory history is empty.");
        return report;
    }

    report.timeList.reserve(history.size());
    report.latList.reserve(history.size());
    report.lonList.reserve(history.size());
    report.altList.reserve(history.size());
    report.speedList.reserve(history.size());
    report.pitchList.reserve(history.size());
    report.rollList.reserve(history.size());
    report.yawList.reserve(history.size());

    double maxAngleAbs = 0.0;
    double minPitch = 0.0, maxPitch = 0.0;
    double minRoll = 0.0, maxRoll = 0.0;
    double minYaw = 0.0, maxYaw = 0.0;
    bool hasYawWrap = false;

    for (size_t i = 0; i < history.size(); ++i) {
        const auto& pt = history[i];

        report.timeList.push_back(pt.sim_time);
        report.latList.push_back(pt.lat);
        report.lonList.push_back(pt.lon);
        report.altList.push_back(pt.alt);
        
        double speed = std::sqrt(pt.vx * pt.vx + pt.vy * pt.vy + pt.vz * pt.vz);
        report.speedList.push_back(speed);

        report.pitchList.push_back(pt.pitch);
        report.rollList.push_back(pt.roll);
        report.yawList.push_back(pt.yaw);

        // NaN / Inf Check
        if (IsInvalid(pt.sim_time) || IsInvalid(pt.lat) || IsInvalid(pt.lon) || IsInvalid(pt.alt) ||
            IsInvalid(pt.vx) || IsInvalid(pt.vy) || IsInvalid(pt.vz) ||
            IsInvalid(pt.pitch) || IsInvalid(pt.roll) || IsInvalid(pt.yaw)) {
            report.nanOrInfCount++;
            if (report.nanOrInfCount <= 5) {
                std::stringstream ss;
                ss << "ERROR: NaN or Inf detected at step " << i << " (Time: " << pt.sim_time << " s)";
                report.errors.push_back(ss.str());
            }
        }

        // Boundary Checks
        if (pt.lat < -90.0 || pt.lat > 90.0) {
            report.outOfBoundsCount++;
            if (report.outOfBoundsCount <= 3) {
                report.errors.push_back("ERROR: Latitude out of bounds [-90, 90]: " + std::to_string(pt.lat));
            }
        }
        if (pt.lon < -180.0 || pt.lon > 180.0) {
            report.outOfBoundsCount++;
            if (report.outOfBoundsCount <= 3) {
                report.errors.push_back("ERROR: Longitude out of bounds [-180, 180]: " + std::to_string(pt.lon));
            }
        }
        if (pt.alt < -1000.0 || pt.alt > 1000000.0) {
            report.outOfBoundsCount++;
            if (report.outOfBoundsCount <= 3) {
                report.warnings.push_back("WARNING: Altitude extreme value: " + std::to_string(pt.alt) + " m");
            }
        }

        // Track attitude angle statistics
        maxAngleAbs = (std::max)({ maxAngleAbs, std::abs(pt.pitch), std::abs(pt.roll), std::abs(pt.yaw) });
        if (i == 0) {
            minPitch = maxPitch = pt.pitch;
            minRoll = maxRoll = pt.roll;
            minYaw = maxYaw = pt.yaw;
        } else {
            minPitch = (std::min)(minPitch, pt.pitch);
            maxPitch = (std::max)(maxPitch, pt.pitch);
            minRoll = (std::min)(minRoll, pt.roll);
            maxRoll = (std::max)(maxRoll, pt.roll);
            minYaw = (std::min)(minYaw, pt.yaw);
            maxYaw = (std::max)(maxYaw, pt.yaw);

            const double dYaw = std::abs(pt.yaw - history[i - 1].yaw);
            // 弧度制偏航角在 ±π 附近突变环绕特征 (跳变幅度约为 2π，且前后绝对值均 <= 3.2)
            if (dYaw > 5.0 && dYaw < 6.4 && std::abs(pt.yaw) <= 3.2 && std::abs(history[i - 1].yaw) <= 3.2) {
                hasYawWrap = true;
            }
        }

        // Position Jump / Smoothness Check
        if (i > 0) {
            const auto& prevPt = history[i - 1];
            double dt = pt.sim_time - prevPt.sim_time;
            if (dt > 1e-6) {
                // Approximate distance in meters (using 1 deg ≈ 111,000m)
                double dLatM = (pt.lat - prevPt.lat) * 111000.0;
                double dLonM = (pt.lon - prevPt.lon) * 111000.0 * std::cos(pt.lat * 3.14159265 / 180.0);
                double dAltM = pt.alt - prevPt.alt;
                double dist = std::sqrt(dLatM * dLatM + dLonM * dLonM + dAltM * dAltM);
                double impliedSpeed = dist / dt;

                if (impliedSpeed > 15000.0) { // Unreasonable velocity jump (> 15 km/s)
                    report.positionJumpCount++;
                    if (report.positionJumpCount <= 3) {
                        std::stringstream ss;
                        ss << "WARNING: Position jump detected at step " << i << " (implied speed: " << impliedSpeed << " m/s)";
                        report.warnings.push_back(ss.str());
                    }
                }
            }
        }
    }

    const double pitchSpan = maxPitch - minPitch;
    const double rollSpan = maxRoll - minRoll;
    const double yawSpan = maxYaw - minYaw;

    // Angle Unit Inspection
    if (maxAngleAbs > 6.28318530718 || yawSpan > 6.28318530718 || rollSpan > 6.28318530718 || pitchSpan > 3.14159265359) {
        // 任何姿态角绝对值或跨度超过 2π（或俯仰跨度超过 π），数学上无法为标准弧度区间，确认为度数
        report.isDegreeUnit = true;
        report.unitCheckLog = "INFO: 姿态角确认采用度数单位（最大角度: " + std::to_string(maxAngleAbs) + " deg）。";
    } else if (maxAngleAbs <= 0.01) {
        // 近似平直飞行，角度变化微弱，保持默认度数
        report.isDegreeUnit = true;
        report.unitCheckLog = "INFO: 姿态角变化极小 (<= 0.01)，保持默认度数单位。";
    } else {
        // 处于 (0.01, 2π] 区间：综合核验是否具有弧度制特征
        bool nearPiBoundary = (std::abs(maxAngleAbs - 3.141592653589793) < 0.05)
                           || (std::abs(maxAngleAbs - 6.283185307179586) < 0.05);

        // 运动学吻合度核验：若存在有效前向速度与非微小俯仰角，核对实际垂向速度 vz 与航迹爬升的一致性
        bool kinematicsSuggestsRadians = false;
        double sumVzActual = 0.0;
        double sumVzIfRadian = 0.0;
        double sumVzIfDegree = 0.0;
        int kinematicSamples = 0;

        for (size_t i = 0; i < history.size(); ++i) {
            const auto& pt = history[i];
            const double spd = report.speedList[i];
            const double absP = std::abs(pt.pitch);
            if (spd > 10.0 && absP > 0.1 && absP <= 1.57 && std::abs(pt.vz) > 1e-3) {
                sumVzActual += std::abs(pt.vz);
                sumVzIfRadian += spd * std::abs(std::sin(absP));
                sumVzIfDegree += spd * std::abs(std::sin(absP * 3.14159265358979323846 / 180.0));
                kinematicSamples++;
            }
        }

        if (kinematicSamples >= 5) {
            double errRadian = std::abs(sumVzIfRadian - sumVzActual);
            double errDegree = std::abs(sumVzIfDegree - sumVzActual);
            // 弧度制预测误差明显小于度数制预测误差（至少偏离 3 倍以上）
            if (errRadian < errDegree * 0.33) {
                kinematicsSuggestsRadians = true;
            }
        }

        if (hasYawWrap || nearPiBoundary || kinematicsSuggestsRadians) {
            report.isDegreeUnit = false;
            std::string reason = hasYawWrap ? "检测到偏航角 ±π 环绕跳变"
                : (nearPiBoundary ? "姿态峰值极度贴合 π/2π 截断边界" : "垂向速度与弧度航迹动力学匹配");
            report.unitCheckLog = "WARNING: 姿态角最大值为 " + std::to_string(maxAngleAbs)
                + " (<= 2π)，且" + reason + "，疑似为弧度制 (RADIANS)！建议核对接口规范。";
            report.warnings.push_back(report.unitCheckLog);
        } else {
            // 平飞或常规小机动（如平飞俯仰 1~5 度），判定为度数单位，不报误报警告
            report.isDegreeUnit = true;
            report.unitCheckLog = "INFO: 姿态角处于小姿态平飞范围 (最大角 " + std::to_string(maxAngleAbs)
                + " deg <= 2π)，判定为度数单位；若想定包含大机动请确认接口规范。";
        }
    }

    report.overallPass = (report.nanOrInfCount == 0 && report.outOfBoundsCount == 0);
    return report;
}
