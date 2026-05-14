/**
 * @file Logger.h
 * @brief 结构化日志系统 - 统一的日志记录
 *
 * @details 提供结构化日志记录，支持标签、级别、格式化输出。
 * 参考 FinceptTerminal 的日志系统。
 *
 * @example
 * LOG_INFO("market", "Fetching quote for {}", symbol);
 * LOG_ERROR("trading", "Order failed: {}", error);
 * LOG_DEBUG("network", "Request completed in {}ms", elapsed);
 */

#ifndef WEALTHPILOT_LOGGER_H
#define WEALTHPILOT_LOGGER_H

#include <QObject>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMap>
#include <QMutex>
#include <QMutexLocker>
#include <QDebug>
#include <memory>
#include <functional>

namespace WealthPilot {

/**
 * @brief 日志级别
 */
enum class LogLevel {
    Debug = 0,
    Info = 1,
    Warning = 2,
    Error = 3,
    Critical = 4
};

/**
 * @brief 日志记录
 */
struct LogRecord {
    LogLevel level;
    QString tag;
    QString message;
    QDateTime timestamp;
    QString file;
    int line;
    QString function;
};

/**
 * @brief 日志格式化器
 */
class LogFormatter {
public:
    virtual ~LogFormatter() = default;
    virtual QString format(const LogRecord& record) = 0;
};

/**
 * @brief 默认格式化器
 */
class DefaultFormatter : public LogFormatter {
public:
    QString format(const LogRecord& record) override {
        QString levelStr;
        switch (record.level) {
            case LogLevel::Debug:    levelStr = "DEBUG"; break;
            case LogLevel::Info:     levelStr = "INFO"; break;
            case LogLevel::Warning:  levelStr = "WARN"; break;
            case LogLevel::Error:    levelStr = "ERROR"; break;
            case LogLevel::Critical: levelStr = "CRITICAL"; break;
        }

        return QString("[%1] [%2] [%3] %4")
            .arg(record.timestamp.toString("yyyy-MM-dd HH:mm:ss.zzz"))
            .arg(levelStr)
            .arg(record.tag)
            .arg(record.message);
    }
};

/**
 * @brief 详细格式化器（包含位置信息）
 */
class DetailedFormatter : public LogFormatter {
public:
    QString format(const LogRecord& record) override {
        QString levelStr;
        switch (record.level) {
            case LogLevel::Debug:    levelStr = "DEBUG"; break;
            case LogLevel::Info:     levelStr = "INFO"; break;
            case LogLevel::Warning:  levelStr = "WARN"; break;
            case LogLevel::Error:    levelStr = "ERROR"; break;
            case LogLevel::Critical: levelStr = "CRITICAL"; break;
        }

        return QString("[%1] [%2] [%3] [%4:%5] %6")
            .arg(record.timestamp.toString("yyyy-MM-dd HH:mm:ss.zzz"))
            .arg(levelStr)
            .arg(record.tag)
            .arg(record.file)
            .arg(record.line)
            .arg(record.message);
    }
};

/**
 * @brief JSON 格式化器
 */
class JsonFormatter : public LogFormatter {
public:
    QString format(const LogRecord& record) override {
        QString levelStr;
        switch (record.level) {
            case LogLevel::Debug:    levelStr = "debug"; break;
            case LogLevel::Info:     levelStr = "info"; break;
            case LogLevel::Warning:  levelStr = "warning"; break;
            case LogLevel::Error:    levelStr = "error"; break;
            case LogLevel::Critical: levelStr = "critical"; break;
        }

        return QString("{\"timestamp\":\"%1\",\"level\":\"%2\",\"tag\":\"%3\",\"message\":\"%4\",\"file\":\"%5\",\"line\":%6}")
            .arg(record.timestamp.toString("yyyy-MM-ddTHH:mm:ss.zzz"))
            .arg(levelStr)
            .arg(record.tag)
            .arg(escapeJson(record.message))
            .arg(record.file)
            .arg(record.line);
    }

private:
    QString escapeJson(const QString& str) {
        QString result = str;
        result.replace("\\", "\\\\");
        result.replace("\"", "\\\"");
        result.replace("\n", "\\n");
        result.replace("\r", "\\r");
        result.replace("\t", "\\t");
        return result;
    }
};

/**
 * @brief 日志输出器
 */
class LogWriter {
public:
    virtual ~LogWriter() = default;
    virtual void write(const QString& formatted) = 0;
    virtual void flush() {}
};

/**
 * @brief 文件输出器
 */
class FileWriter : public LogWriter {
public:
    explicit FileWriter(const QString& path) {
        m_file.setFileName(path);
        if (m_file.open(QIODevice::WriteOnly | QIODevice::Append)) {
            m_stream.setDevice(&m_file);
        }
    }

    ~FileWriter() {
        if (m_file.isOpen()) {
            m_file.close();
        }
    }

    void write(const QString& formatted) override {
        if (m_file.isOpen()) {
            m_stream << formatted << "\n";
        }
    }

    void flush() override {
        if (m_file.isOpen()) {
            m_stream.flush();
        }
    }

private:
    QFile m_file;
    QTextStream m_stream;
};

/**
 * @brief 控制台输出器
 */
class ConsoleWriter : public LogWriter {
public:
    void write(const QString& formatted) override {
        // 根据内容判断颜色
        if (formatted.contains("[ERROR]") || formatted.contains("[CRITICAL]")) {
            qDebug() << "\033[31m" << formatted << "\033[0m";  // 红色
        } else if (formatted.contains("[WARN]")) {
            qDebug() << "\033[33m" << formatted << "\033[0m";  // 黄色
        } else if (formatted.contains("[DEBUG]")) {
            qDebug() << "\033[36m" << formatted << "\033[0m";  // 青色
        } else {
            qDebug() << formatted;
        }
    }
};

/**
 * @brief 日志管理器
 */
class Logger : public QObject {
    Q_OBJECT

public:
    static Logger& instance() {
        static Logger instance;
        return instance;
    }

    /**
     * @brief 初始化日志系统
     */
    void initialize(const QString& logDir = "logs") {
        QMutexLocker locker(&m_mutex);

        // 创建日志目录
        QDir dir(logDir);
        if (!dir.exists()) {
            dir.mkpath(".");
        }

        // 添加文件输出器
        QString logFile = QString("%1/wealthpilot_%2.log")
            .arg(logDir)
            .arg(QDateTime::currentDateTime().toString("yyyyMMdd"));
        m_writers.append(std::make_shared<FileWriter>(logFile));

        // 添加控制台输出器
        m_writers.append(std::make_shared<ConsoleWriter>());

        // 设置默认格式化器
        m_formatter = std::make_shared<DefaultFormatter>();

        m_initialized = true;
    }

    /**
     * @brief 设置日志级别
     */
    void setLevel(LogLevel level) {
        m_level = level;
    }

    /**
     * @brief 设置格式化器
     */
    void setFormatter(std::shared_ptr<LogFormatter> formatter) {
        m_formatter = formatter;
    }

    /**
     * @brief 添加输出器
     */
    void addWriter(std::shared_ptr<LogWriter> writer) {
        m_writers.append(writer);
    }

    /**
     * @brief 记录日志
     */
    void log(LogLevel level, const QString& tag, const QString& message,
             const QString& file = QString(), int line = 0, const QString& function = QString()) {
        if (level < m_level) {
            return;
        }

        LogRecord record;
        record.level = level;
        record.tag = tag;
        record.message = message;
        record.timestamp = QDateTime::currentDateTime();
        record.file = file;
        record.line = line;
        record.function = function;

        QString formatted = m_formatter->format(record);

        QMutexLocker locker(&m_mutex);
        for (auto& writer : m_writers) {
            writer->write(formatted);
        }

        emit logRecorded(record);
    }

    /**
     * @brief 格式化消息（支持 {} 占位符）
     */
    template <typename... Args>
    static QString formatMessage(const QString& templateStr, Args&&... args) {
        QString result = templateStr;
        QStringList values;
        
        // 将参数转换为字符串
        (values.append(QString::fromStdString(
            std::to_string(std::forward<Args>(args)))), ...);
        
        // 替换占位符
        for (const auto& value : values) {
            result.replace(result.indexOf("{}"), 2, value);
        }
        
        return result;
    }

signals:
    void logRecorded(const LogRecord& record);

private:
    Logger() : m_level(LogLevel::Info) {}
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    LogLevel m_level;
    std::shared_ptr<LogFormatter> m_formatter;
    QList<std::shared_ptr<LogWriter>> m_writers;
    mutable QMutex m_mutex;
    bool m_initialized = false;
};

// ========== 日志宏定义 ==========

#define LOG_DEBUG(tag, message) \
    WealthPilot::Logger::instance().log( \
        WealthPilot::LogLevel::Debug, tag, message, __FILE__, __LINE__, __FUNCTION__)

#define LOG_INFO(tag, message) \
    WealthPilot::Logger::instance().log( \
        WealthPilot::LogLevel::Info, tag, message, __FILE__, __LINE__, __FUNCTION__)

#define LOG_WARN(tag, message) \
    WealthPilot::Logger::instance().log( \
        WealthPilot::LogLevel::Warning, tag, message, __FILE__, __LINE__, __FUNCTION__)

#define LOG_ERROR(tag, message) \
    WealthPilot::Logger::instance().log( \
        WealthPilot::LogLevel::Error, tag, message, __FILE__, __LINE__, __FUNCTION__)

#define LOG_CRITICAL(tag, message) \
    WealthPilot::Logger::instance().log( \
        WealthPilot::LogLevel::Critical, tag, message, __FILE__, __LINE__, __FUNCTION__)

// 带格式化的日志宏
#define LOG_DEBUG_FMT(tag, fmt, ...) \
    LOG_DEBUG(tag, WealthPilot::Logger::formatMessage(fmt, __VA_ARGS__))

#define LOG_INFO_FMT(tag, fmt, ...) \
    LOG_INFO(tag, WealthPilot::Logger::formatMessage(fmt, __VA_ARGS__))

#define LOG_WARN_FMT(tag, fmt, ...) \
    LOG_WARN(tag, WealthPilot::Logger::formatMessage(fmt, __VA_ARGS__))

#define LOG_ERROR_FMT(tag, fmt, ...) \
    LOG_ERROR(tag, WealthPilot::Logger::formatMessage(fmt, __VA_ARGS__))

// 日志标签常量
namespace LogTags {
    constexpr const char* APP = "app";
    constexpr const char* MARKET = "market";
    constexpr const char* TRADING = "trading";
    constexpr const char* NETWORK = "network";
    constexpr const char* DATABASE = "database";
    constexpr const char* UI = "ui";
    constexpr const char* AI = "ai";
    constexpr const char* ANALYSIS = "analysis";
    constexpr const char* CONFIG = "config";
    constexpr const char* SECURITY = "security";
    constexpr const char* PERFORMANCE = "performance";
}

} // namespace WealthPilot

#endif // WEALTHPILOT_LOGGER_H