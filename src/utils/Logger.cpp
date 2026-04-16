/**
 * @file Logger.cpp
 * @brief 日志管理器实现
 * @author WealthPilot Team
 * @version 2.0.0
 */

#include "Logger.h"
#include <QDebug>
#include <QDateTime>
#include <QDir>
#include <QMutexLocker>

// ========== PIMPL 实现 ==========

struct Logger::Impl {
    Level level = Level::Info;
    std::unique_ptr<QFile> logFile;
    std::unique_ptr<QTextStream> stream;
    mutable QMutex mutex;
};

// ========== 构造和析构 ==========

Logger::Logger()
    : d(std::make_unique<Impl>())
{
}

Logger::~Logger()
{
    if (d->stream) {
        d->stream->flush();
    }
}

// ========== 单例访问 ==========

Logger* Logger::instance()
{
    static Logger instance;
    return &instance;
}

// ========== 公共方法 ==========

void Logger::init(const QString& logFile)
{
    QMutexLocker locker(&d->mutex);

    // 关闭现有文件
    if (d->logFile) {
        d->logFile->close();
        d->logFile.reset();
        d->stream.reset();
    }

    if (!logFile.isEmpty()) {
        // 确保日志目录存在
        QFileInfo fileInfo(logFile);
        QDir dir = fileInfo.dir();
        if (!dir.exists()) {
            dir.mkpath(".");
        }

        d->logFile = std::make_unique<QFile>(logFile);
        if (d->logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            d->stream = std::make_unique<QTextStream>(d->logFile.get());
            d->stream->setEncoding(QStringConverter::Utf8);
            *d->stream << QString("\n=== Log started at %1 ===\n")
                             .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
            d->stream->flush();
        } else {
            qWarning() << "Failed to open log file:" << logFile;
            d->logFile.reset();
        }
    }
}

void Logger::setLevel(Level level)
{
    QMutexLocker locker(&d->mutex);
    d->level = level;
}

void Logger::log(Level level, const QString& message)
{
    // 检查日志级别
    if (level < d->level) {
        return;
    }

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString levelStr = levelToString(level);
    QString formatted = QString("[%1] [%2] %3").arg(timestamp, levelStr, message);

    // 输出到控制台
    {
        QMutexLocker locker(&d->mutex);
        switch (level) {
            case Level::Debug:
                qDebug().noquote() << formatted;
                break;
            case Level::Info:
                qInfo().noquote() << formatted;
                break;
            case Level::Warning:
                qWarning().noquote() << formatted;
                break;
            case Level::Error:
                qCritical().noquote() << formatted;
                break;
        }
    }

    // 输出到文件
    if (d->stream) {
        QMutexLocker locker(&d->mutex);
        *d->stream << formatted << Qt::endl;
        d->stream->flush();
    }
}

void Logger::debug(const QString& message)
{
    log(Level::Debug, message);
}

void Logger::info(const QString& message)
{
    log(Level::Info, message);
}

void Logger::warning(const QString& message)
{
    log(Level::Warning, message);
}

void Logger::error(const QString& message)
{
    log(Level::Error, message);
}

// ========== 私有方法 ==========

QString Logger::levelToString(Level level) const
{
    switch (level) {
        case Level::Debug:   return "DEBUG";
        case Level::Info:    return "INFO";
        case Level::Warning: return "WARN";
        case Level::Error:   return "ERROR";
        default:             return "UNKNOWN";
    }
}

QString Logger::currentTime() const
{
    return QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
}
