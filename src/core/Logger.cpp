#include "core/Logger.h"
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <QtGlobal>
#include <QString>

// ── Qt message handler ────────────────────────────────────────────────────────
// Routes qDebug / qInfo / qWarning / qCritical / qFatal into spdlog so that
// all Qt-internal messages also land in elcad.log.  The source location from
// Qt's context is preserved: the log pattern will show the Qt header/source
// file and line rather than Logger.cpp.
static void qtMessageHandler(QtMsgType          type,
                             const QMessageLogContext& ctx,
                             const QString&     msg)
{
    auto* logger = spdlog::default_logger_raw();
    if (!logger) return;

    std::string text = msg.toStdString();
    spdlog::source_loc loc{
        ctx.file     ? ctx.file     : "",
        ctx.line,
        ctx.function ? ctx.function : ""
    };

    spdlog::level::level_enum level;
    switch (type) {
    case QtDebugMsg:    level = spdlog::level::debug;    break;
    case QtInfoMsg:     level = spdlog::level::info;     break;
    case QtWarningMsg:  level = spdlog::level::warn;     break;
    case QtCriticalMsg: level = spdlog::level::err;      break;
    case QtFatalMsg:    level = spdlog::level::critical; break;
    default:            level = spdlog::level::info;     break;
    }

    logger->log(loc, level, "[Qt] {}", text);
}

namespace elcad {

void Logger::init(const std::string& logDir)
{
    try {
        std::string logPath = logDir + "/elcad.log";

        // ── File sink: 10 MB max, 3 rotating backup files ─────────────────────
        // Files cycle as: elcad.log → elcad.1.log → elcad.2.log → elcad.3.log
        auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            logPath,
            10 * 1024 * 1024,   // 10 MB per file
            3                    // keep 3 backup files
        );
        fileSink->set_level(spdlog::level::trace);

        // ── Console sink: WARN and above to stderr ─────────────────────────────
        auto consoleSink = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
        consoleSink->set_level(spdlog::level::warn);

        auto logger = std::make_shared<spdlog::logger>(
            "elcad",
            spdlog::sinks_init_list{fileSink, consoleSink}
        );
        logger->set_level(spdlog::level::trace);

        // Pattern: timestamp  level(padded)  source_file:line  message
        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%-8l] [%s:%#] %v");

        // Flush to disk immediately on WARN and above so errors are never lost
        logger->flush_on(spdlog::level::warn);

        spdlog::set_default_logger(logger);

        // Redirect all Qt messages (qDebug / qWarning / etc.) into spdlog
        qInstallMessageHandler(qtMessageHandler);

        SPDLOG_INFO("Logger initialised — log: {}", logPath);
    } catch (const spdlog::spdlog_ex& ex) {
        fprintf(stderr, "elcad: Logger init failed: %s\n", ex.what());
    }
}

} // namespace elcad
