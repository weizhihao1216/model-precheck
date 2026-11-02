#ifndef QT_COMPAT_H
#define QT_COMPAT_H

#include <QtCore/qglobal.h>
#include <QtCore/QString>
#include <QtCore/QStringList>

// Ensure NOMINMAX is defined globally before any Windows header inclusion
#ifndef NOMINMAX
#define NOMINMAX
#endif

// ============================================================================
// 1. Qt Version Compatibility Macros (Qt 5.11 vs Qt 5.14+ / Qt 6)
// ============================================================================

// QString::SkipEmptyParts / KeepEmptyParts compatibility:
// QString::SkipEmptyParts was deprecated in Qt 5.14 in favor of Qt::SkipEmptyParts.
// In Qt 5.11, Qt::SkipEmptyParts does not exist.
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    #define QT_SKIP_EMPTY_PARTS Qt::SkipEmptyParts
    #define QT_KEEP_EMPTY_PARTS Qt::KeepEmptyParts
    using QtSplitBehavior = Qt::SplitBehavior;
#else
    #define QT_SKIP_EMPTY_PARTS QString::SkipEmptyParts
    #define QT_KEEP_EMPTY_PARTS QString::KeepEmptyParts
    using QtSplitBehavior = QString::SplitBehavior;
#endif

// QTextStream endl compatibility:
// Qt 5.14 deprecated global endl in favor of Qt::endl.
// In Qt 5.11, Qt::endl does not exist; only global endl exists.
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    #define QT_ENDL Qt::endl
#else
    #define QT_ENDL endl
#endif

// High-DPI Rounding Policy:
// Qt 5.14+ introduced setHighDpiScaleFactorRoundingPolicy to prevent fractional
// scaling blurriness. In Qt 5.11, this API does not exist.
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    #define QT_INIT_HIGH_DPI_ROUNDING() \
        QGuiApplication::setHighDpiScaleFactorRoundingPolicy( \
            Qt::HighDpiScaleFactorRoundingPolicy::PassThrough)
#else
    #define QT_INIT_HIGH_DPI_ROUNDING() ((void)0)
#endif

// QQuickStyle default theme compatibility:
// In Qt 5 (5.11, 5.14, 5.15), the built-in default style for QtQuickControls2 is "Default".
// In Qt 6, "Default" was renamed to "Basic". Passing "Basic" in Qt 5 triggers an unknown style warning.
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    #define QT_DEFAULT_QUICK_STYLE QStringLiteral("Basic")
#else
    #define QT_DEFAULT_QUICK_STYLE QStringLiteral("Default")
#endif

// QRecursiveMutex compatibility:
// Qt 5.14 introduced QRecursiveMutex as an independent class in <QtCore/QRecursiveMutex>.
// In Qt 5.11, recursive mutexes are instantiated as QMutex(QMutex::Recursive).
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    #include <QtCore/QRecursiveMutex>
    using QtRecursiveMutex = QRecursiveMutex;
#else
    #include <QtCore/QMutex>
    class QtRecursiveMutex : public QMutex {
    public:
        QtRecursiveMutex() : QMutex(QMutex::Recursive) {}
    };
#endif

// QFontMetrics horizontalAdvance vs width:
// Qt 5.11 introduced horizontalAdvance(text), and Qt 5.13 deprecated width(text).
#if (QT_VERSION >= QT_VERSION_CHECK(5, 11, 0))
    #define QT_FONT_HORIZONTAL_ADVANCE(fm, text) ((fm).horizontalAdvance(text))
#else
    #define QT_FONT_HORIZONTAL_ADVANCE(fm, text) ((fm).width(text))
#endif

// ============================================================================
// 2. MSVC Compiler Version Detection Macros (VS2017 vs VS2019 vs VS2022 / vNext)
// ============================================================================
#if defined(_MSC_VER)
    #define MODEL_PRECHECK_MSVC_VER _MSC_VER
    #define MODEL_PRECHECK_MSVC_AT_LEAST(ver) (_MSC_VER >= (ver))
    #if _MSC_VER >= 1930
        #define MODEL_PRECHECK_MSVC_2022 1
    #elif _MSC_VER >= 1920
        #define MODEL_PRECHECK_MSVC_2019 1
    #elif _MSC_VER >= 1910
        #define MODEL_PRECHECK_MSVC_2017 1
    #endif
#else
    #define MODEL_PRECHECK_MSVC_VER 0
    #define MODEL_PRECHECK_MSVC_AT_LEAST(ver) 0
#endif

#endif // QT_COMPAT_H
