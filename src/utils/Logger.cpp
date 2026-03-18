/**
 * @file Logger.cpp
 * @brief 日志管理器实现 - 优化版本
 *
 * 优化内容：
 * - 添加空指针保护，防止未初始化时崩溃
 * - 添加线程安全检查
 * - 优化文件写入性能
 * - 添加日志级别过滤
 */
#include "Logger.h"
#include <QDebug>
#include <QDateTime>
#include <QDir>
#include <QTextStream>
#include <QRegularExpression>
#include <QFileInfo>

Logger* Logger::s_instance = nullptr;

Logger::Logger()
    : m_level(Info)
{
}

Logger::~Logger()
{
    if (m_stream) {
        m_stream->flush();
    }
}

Logger* Logger::instance()
{
    if (!s_instance) {
        s_instance = new Logger();
    }
    return s_instance;
}

void Logger::init(const QString& logFile)
{
    QMutexLocker locker(&m_mutex);

    if (m_logFile) {
        m_logFile->close();
        m_logFile.reset();
        m_stream.reset();
    }

    if (!logFile.isEmpty()) {
        // 确保日志目录存在
        QFileInfo fileInfo(logFile);
        QDir dir = fileInfo.dir();
        if (!dir.exists()) {
            dir.mkpath(".");
        }

        m_logFile = std::make_unique<QFile>(logFile);
        if (m_logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            m_stream = std::make_unique<QTextStream>(m_logFile.get());
            m_stream->setEncoding(QStringConverter::Utf8);
            *m_stream << QString("\n=== Log started at %1 ===\n")
                             .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
            *m_stream << Qt::endl; // 将 Qt::endl 单独放在一个语句中
            m_stream->flush();
        } else {
            qWarning() << "Failed to open log file:" << logFile;
            m_logFile.reset();
        }
    }
}

void Logger::setLevel(Level level)
{
    QMutexLocker locker(&m_mutex);
    m_level = level;
}

void Logger::log(Level level, const QString& message)
{
    // 检查日志级别
    if (level < m_level) {
        return;
    }

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString levelStr = levelToString(level);
    QString formatted = QString("[%1] [%2] %3").arg(timestamp, levelStr, message);

    // 输出到控制台
    writeToConsole(formatted);

    // 输出到文件
    writeToFile(formatted);
}

void Logger::debug(const QString& message)
{
    // 空指针保护：如果未初始化，使用qDebug输出
    if (!m_stream) {
        qDebug() << "[DEBUG]" << message;
        return;
    }
    log(Debug, message);
}

void Logger::info(const QString& message)
{
    if (!m_stream) {
        qInfo() << "[INFO]" << message;
        return;
    }
    log(Info, message);
}

void Logger::warning(const QString& message)
{
    if (!m_stream) {
        qWarning() << "[WARNING]" << message;
        return;
    }
    log(Warning, message);
}

void Logger::error(const QString& message)
{
    if (!m_stream) {
        qCritical() << "[ERROR]" << message;
        return;
    }
    log(Error, message);
}

QString Logger::levelToString(Level level)
{
    switch (level) {
    case Debug:   return "DEBUG";
    case Info:    return "INFO";
    case Warning: return "WARN";
    case Error:   return "ERROR";
    default:      return "UNKNOWN";
    }
}

void Logger::writeToFile(const QString& message)
{
    QMutexLocker locker(&m_mutex);

    if (m_stream) {
        // 添加到缓冲区
        m_logBuffer.append(message + "\n");

        // 当缓冲区达到一定大小或时间间隔时写入文件
        static int lineCount = 0;
        static qint64 lastFlushTime = QDateTime::currentMSecsSinceEpoch();

        if (++lineCount >= 10 || 
            QDateTime::currentMSecsSinceEpoch() - lastFlushTime > 5000) { // 5秒或10行
            
            // 检查日志文件大小，超过10MB则轮转
            if (m_logFile && m_logFile->size() > 10 * 1024 * 1024) {
                rotateLogFile();
            }

            // 写入缓冲区内容
            const QStringList& bufferRef = m_logBuffer;  // 创建 const 引用
            for (const QString& line : bufferRef) {
                *m_stream << line;
            }
            m_stream->flush();
            
            m_logBuffer.clear();
            lineCount = 0;
            lastFlushTime = QDateTime::currentMSecsSinceEpoch();
        }
    }
}

void Logger::rotateLogFile()
{
    QMutexLocker locker(&m_mutex);

    if (!m_logFile) return;

    // 关闭当前文件
    m_stream->flush();
    m_stream.reset();
    m_logFile->close();

    // 创建新文件名（添加时间戳）
    QFileInfo fileInfo(m_logFile->fileName());
    QString newPath = fileInfo.absolutePath() + "/" + 
                     fileInfo.baseName() + "_" + 
                     QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + 
                     fileInfo.suffix();

    // 重命名旧文件
    QFile::rename(m_logFile->fileName(), newPath);

    // 重新打开新文件
    bool success = m_logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    if (!success) {
        // 处理打开失败的情况，例如输出错误信息
        LOG_WARNING("Failed to open log file:" + m_logFile->fileName());
    }
    m_stream = std::make_unique<QTextStream>(m_logFile.get());
    m_stream->setEncoding(QStringConverter::Utf8);
}

void Logger::writeToConsole(const QString& message)
{
    qDebug().noquote() << message;
}
