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

// 针对不同 Qt 版本的高分屏（High-DPI）跨机器缩放策略：
// 1. Qt 5.14+：原生支持 HighDpiScaleFactorRoundingPolicy::PassThrough，
//    完全由操作系统与 Qt 引擎无损浮点缩放。
// 2. Qt 5.11：使用 Qt 官方标准的 AA_EnableHighDpiScaling 与 AA_UseHighDpiPixmaps。
//    严禁在 Qt 5.11 下人为设置非整数 QT_SCALE_FACTOR（如 0.75），否则 Qt 5.11 的
//    文本排版引擎（QTextDocument / TextArea）会出现严重的字形错位、重影与行重叠 Bug。

} // namespace

int main(int argc, char* argv[]) {
    QT_INIT_HIGH_DPI_ROUNDING();
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QQuickStyle::setStyle(QT_DEFAULT_QUICK_STYLE);

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("model-precheck"));

    app.setFont(QFont(QStringLiteral("Microsoft YaHei UI"), 10));
    app.setWindowIcon(BuildAppIcon());

    qRegisterMetaType<PerfProfileReport>("PerfProfileReport");
    qRegisterMetaType<PerfSample>("PerfSample");
    // 专项测试通过跨线程信号回传结果，显式注册避免队列连接无法投递导致一直等待。
    qRegisterMetaType<ConcurrencyTestReport>("ConcurrencyTestReport");
    qRegisterMetaType<ConcurrencyThreadResult>("ConcurrencyThreadResult");

    // 清理上次运行遗留的被测模块副本（本次运行尚未加载任何模块，删除一定安全）。
    const QString appDirPath = QCoreApplication::applicationDirPath();
    CleanupPrivateModuleCopies(QDir(appDirPath).filePath(QStringLiteral("TestModel")).toUtf8().toStdString());
    CleanupPrivateModuleCopies(QDir(appDirPath).filePath(QStringLiteral("sample_models")).toUtf8().toStdString());
    CleanupPrivateModuleCopies(QDir::current().filePath(QStringLiteral("sample_models")).toUtf8().toStdString());

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
    }   // 这里完成 Qt 侧的正常析构（会话保存、编译进程回收等）
    // 被测模块的私有副本始终留在进程内，其 DETACH 可能在退出时死循环；
    // 只要加载过副本就直接终止进程，避免出现关不掉的假死进程。
    if (HasLoadedPrivateModule())
        TerminateProcess(GetCurrentProcess(), static_cast<UINT>(exitCode));
    return exitCode;
}
