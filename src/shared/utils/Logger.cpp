/**
 * @file Logger.cpp
 * @brief 日志管理器实现
 * @author WealthPilot Team
 * @version 2.1.0
 */

#include "Logger.h"
#include <QDebug>
#include <QDateTime>
#include <QDir>
#include <QMutexLocker>
#include <QRegularExpression>

// ========== PIMPL 实现 ==========

struct Logger::Impl {
    Level level = Level::Info;
    std::unique_ptr<QFile> logFile;
    std::unique_ptr<QTextStream> stream;
    mutable QMutex mutex;
    QString logDir;           ///< 日志目录
    QString logFileBaseName;  ///< 日志文件基础名称（不含日期）
    QDate currentDate;        ///< 当前日期（用于检测日期变化）
    int maxDays = 30;         ///< 保留日志的最大天数
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

void Logger::init(const QString& logFile, int maxDays)
{
    QMutexLocker locker(&d->mutex);

    // 关闭现有文件
    if (d->logFile) {
        d->logFile->close();
        d->logFile.reset();
        d->stream.reset();
    }

    d->maxDays = maxDays;
    d->currentDate = QDate::currentDate();

    if (!logFile.isEmpty()) {
        // 解析日志文件路径
        QFileInfo fileInfo(logFile);
        d->logDir = fileInfo.absolutePath();
        QString baseName = fileInfo.completeBaseName();
        QString suffix = fileInfo.suffix();
        
        // 生成带日期的日志文件名
        d->logFileBaseName = baseName;
        
        // 确保日志目录存在
        QDir dir(d->logDir);
        if (!dir.exists()) {
            dir.mkpath(".");
        }

        // 创建带日期的日志文件名
        QString datedLogFile = getDatedLogFileName(d->currentDate);
        
        d->logFile = std::make_unique<QFile>(datedLogFile);
        if (d->logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            d->stream = std::make_unique<QTextStream>(d->logFile.get());
            d->stream->setEncoding(QStringConverter::Utf8);
            *d->stream << QString("\n=== Log started at %1 ===\n")
                             .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
            d->stream->flush();
        } else {
            qWarning() << "Failed to open log file:" << datedLogFile;
            d->logFile.reset();
        }
        
        // 清理过期日志
        cleanOldLogs();
    }
}

void Logger::setLevel(Level level)
{
    QMutexLocker locker(&d->mutex);
    d->level = level;
}

void Logger::log(Level level, const QString& message)
{
    // 检查日期变化
    checkDateChange();
    
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

void Logger::checkDateChange()
{
    QDate today = QDate::currentDate();
    if (d->currentDate != today) {
        QMutexLocker locker(&d->mutex);
        if (d->currentDate != today) {
            d->currentDate = today;
            rotateLogFile();
        }
    }
}

void Logger::rotateLogFile()
{
    if (d->logFileBaseName.isEmpty()) {
        return;
    }
    
    // 关闭当前文件
    if (d->logFile) {
        *d->stream << QString("\n=== Log rotated at %1 ===\n")
                        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
        d->stream->flush();
        d->logFile->close();
        d->logFile.reset();
        d->stream.reset();
    }
    
    // 创建新的日志文件
    QString datedLogFile = getDatedLogFileName(d->currentDate);
    d->logFile = std::make_unique<QFile>(datedLogFile);
    if (d->logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        d->stream = std::make_unique<QTextStream>(d->logFile.get());
        d->stream->setEncoding(QStringConverter::Utf8);
        *d->stream << QString("\n=== Log started at %1 ===\n")
                         .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
        d->stream->flush();
    } else {
        qWarning() << "Failed to open new log file:" << datedLogFile;
        d->logFile.reset();
    }
    
    // 清理过期日志
    cleanOldLogs();
}

void Logger::cleanOldLogs()
{
    if (d->logDir.isEmpty() || d->maxDays <= 0) {
        return;
    }
    
    QDir dir(d->logDir);
    QDate cutoffDate = QDate::currentDate().addDays(-d->maxDays);
    
    // 获取所有日志文件
    QStringList filters;
    filters << "*.log";
    QStringList logFiles = dir.entryList(filters, QDir::Files);
    
    // 匹配日志文件名中的日期
    QRegularExpression dateRegex(R"((\d{4}-\d{2}-\d{2}))");
    
    for (const QString& fileName : logFiles) {
        QRegularExpressionMatch match = dateRegex.match(fileName);
        if (match.hasMatch()) {
            QString dateStr = match.captured(1);
            QDate fileDate = QDate::fromString(dateStr, "yyyy-MM-dd");
            
            if (fileDate.isValid() && fileDate < cutoffDate) {
                QString filePath = dir.filePath(fileName);
                if (QFile::remove(filePath)) {
                    qDebug() << "Cleaned old log file:" << fileName;
                }
            }
        }
    }
}

QString Logger::getDatedLogFileName(const QDate& date) const
{
    QString dateStr = date.toString("yyyy-MM-dd");
    return QString("%1/%2-%3.log").arg(d->logDir, d->logFileBaseName, dateStr);
}
