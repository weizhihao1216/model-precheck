#include <QApplication>
#include <QGuiApplication>
#include <QDir>
#include <QIcon>
#include <QMetaType>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPixmap>
#include <QScreen>
#include <QFont>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include "core/ConcurrencyTester.h"
#include "core/PerfProfiler.h"
#include "qml/QmlAppController.h"
#include "utils/SehHelper.h"
#include "utils/QtCompat.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cstdio>

namespace {

// 搴旂敤鍥炬爣锛氶潚缁垮渾瑙掑簳 + 鐧借壊鐩剧墝涓庡鍕撅紝涓庣晫闈富棰樹繚鎸佷竴鑷淬€?
QIcon BuildAppIcon() {
    QIcon icon;
    for (int size : { 16, 24, 32, 48, 64, 128, 256 }) {
        QPixmap pixmap(size, size);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const qreal s = size / 100.0;

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(13, 148, 136));
        painter.drawRoundedRect(QRectF(6 * s, 6 * s, 88 * s, 88 * s), 24 * s, 24 * s);

        QPainterPath shield;
        shield.moveTo(50 * s, 20 * s);
        shield.lineTo(77 * s, 31 * s);
        shield.lineTo(77 * s, 54 * s);
        shield.cubicTo(77 * s, 70 * s, 64 * s, 79 * s, 50 * s, 85 * s);
        shield.cubicTo(36 * s, 79 * s, 23 * s, 70 * s, 23 * s, 54 * s);
        shield.lineTo(23 * s, 31 * s);
        shield.closeSubpath();
        painter.setBrush(QColor(255, 255, 255, 240));
        painter.drawPath(shield);

        QPen pen(QColor(13, 148, 136), 8 * s, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        QPainterPath check;
        check.moveTo(37 * s, 52 * s);
        check.lineTo(46 * s, 62 * s);
        check.lineTo(64 * s, 41 * s);
        painter.drawPath(check);
        painter.end();

        icon.addPixmap(pixmap);
    }
    return icon;
}

// 针对不同 Qt 版本的高分屏（High-DPI）跨机器缩放自适应策略：
// 1. Qt 5.14+：原生支持 HighDpiScaleFactorRoundingPolicy::PassThrough，
//    完全由操作系统与 Qt 引擎无损浮点缩放，不人为设置 QT_SCALE_FACTOR。
// 2. Qt 5.11：Windows 下默认只支持整数缩放（125% -> 1.0, 150% -> 2.0），
//    导致 150% 高分屏被放大为 200%。仅在 Qt 5.11 下对整数取整误差进行精准补偿，
//    使实际有效缩放严格还原为系统的真实比例（如 1.5 / 2.0 = 0.75，总缩放为 1.50）。
// 3. 彻底废除按 1920x1040 暴力缩小的错误逻辑，确保在各类物理屏幕上字体都保持舒适可读。
#if (QT_VERSION < QT_VERSION_CHECK(5, 14, 0))
qreal CalculateDpiCompensation() {
    qreal osScale = 1.0;
    const int logicalWidth = GetSystemMetrics(SM_CXSCREEN);
    DEVMODEW mode{};
    mode.dmSize = sizeof(mode);
    if (logicalWidth > 0 && EnumDisplaySettingsW(nullptr, ENUM_CURRENT_SETTINGS, &mode)
        && mode.dmPelsWidth > 0) {
        osScale = static_cast<qreal>(mode.dmPelsWidth) / logicalWidth;
        osScale = qRound(osScale * 4.0) / 4.0;
    }

    const qreal qtBaseScale = qMax<qreal>(1.0, qRound(osScale));
    const qreal compensation = osScale / qtBaseScale;
    return compensation;
}
#endif

} // namespace

int main(int argc, char* argv[]) {
    QT_INIT_HIGH_DPI_ROUNDING();
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

#if (QT_VERSION < QT_VERSION_CHECK(5, 14, 0))
    // 浠呴拡瀵?Qt 5.11 琛ュ伩鏁存暟鍙栨暣璇樊锛涚敤鎴锋樉寮忔寚瀹?QT_SCALE_FACTOR 鏃朵笉骞查
    if (!qEnvironmentVariableIsSet("QT_SCALE_FACTOR")) {
        const qreal factor = CalculateDpiCompensation();
        if (factor > 0.05 && qAbs(factor - 1.0) > 0.02)
            qputenv("QT_SCALE_FACTOR", QByteArray::number(factor, 'f', 3));
    }
#endif

    QQuickStyle::setStyle(QT_DEFAULT_QUICK_STYLE);

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("model-precheck"));

    app.setFont(QFont(QStringLiteral("Microsoft YaHei UI"), 10));
    app.setWindowIcon(BuildAppIcon());

    qRegisterMetaType<PerfProfileReport>("PerfProfileReport");
    qRegisterMetaType<PerfSample>("PerfSample");
    // 涓撻」娴嬭瘯閫氳繃璺ㄧ嚎绋嬩俊鍙峰洖浼犵粨鏋滐紝鏄惧紡娉ㄥ唽閬垮厤闃熷垪杩炴帴鏃犳硶鎶曢€掑鑷翠竴鐩寸瓑寰呫€?
    qRegisterMetaType<ConcurrencyTestReport>("ConcurrencyTestReport");
    qRegisterMetaType<ConcurrencyThreadResult>("ConcurrencyThreadResult");

    // 娓呯悊涓婃杩愯閬楃暀鐨勮娴嬫ā鍧楀壇鏈紙鏈杩愯灏氭湭鍔犺浇浠讳綍妯″潡锛屽垹闄や竴瀹氬畨鍏級銆?
    CleanupPrivateModuleCopies(
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("TestModel"))
            .toUtf8().toStdString());

    int exitCode = 0;
    {
        QmlAppController controller;
        QQmlApplicationEngine engine;
        engine.rootContext()->setContextProperty(
            QStringLiteral("appController"), &controller);
        engine.load(QUrl(QStringLiteral("qrc:/qml/Main.qml")));
        if (engine.rootObjects().isEmpty())
            return -1;
        exitCode = app.exec();
    }   // 杩欓噷瀹屾垚 Qt 渚х殑姝ｅ父鏋愭瀯锛堜細璇濅繚瀛樸€佺紪璇戣繘绋嬪洖鏀剁瓑锛?
    // 琚祴妯″潡鐨勭鏈夊壇鏈缁堢暀鍦ㄨ繘绋嬪唴锛屽叾 DETACH 鍙兘鍦ㄩ€€鍑烘椂姝诲惊鐜紱
    // 鍙鍔犺浇杩囧壇鏈氨鐩存帴缁堟杩涚▼锛岄伩鍏嶅嚭鐜板叧涓嶆帀鐨勫亣姝昏繘绋嬨€?
    if (HasLoadedPrivateModule())
        TerminateProcess(GetCurrentProcess(), static_cast<UINT>(exitCode));
    return exitCode;
}
