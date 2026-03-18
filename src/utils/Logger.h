/**
 * @file Logger.h
 * @brief 日志管理器
 *
 * @details 线程安全的日志系统，支持：
 * - 多级别日志 (Debug/Info/Warning/Error)
 * - 文件输出
 * - 控制台输出
 * - 日志级别过滤
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <memory>

/**
 * @brief 日志管理器
 */
class Logger
{
public:
    /**
     * @brief 日志级别
     */
    enum Level {
        Debug,      ///< 调试信息
        Info,       ///< 普通信息
        Warning,    ///< 警告
        Error       ///< 错误
    };

    /**
     * @brief 获取单例实例
     */
    static Logger* instance();

    /**
     * @brief 初始化日志系统
     * @param logFile 日志文件路径，为空则只输出到控制台
     */
    void init(const QString& logFile = QString());

    /**
     * @brief 设置日志级别
     * @param level 最低日志级别
     */
    void setLevel(Level level);

    /**
     * @brief 记录日志
     * @param level 日志级别
     * @param message 日志消息
     */
    void log(Level level, const QString& message);

    /**
     * @brief 记录调试信息
     */
    void debug(const QString& message);

    /**
     * @brief 记录普通信息
     */
    void info(const QString& message);

    /**
     * @brief 记录警告
     */
    void warning(const QString& message);

    /**
     * @brief 记录错误
     */
    void error(const QString& message);

private:
    Logger();
    ~Logger();

    // 禁用拷贝
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    QString levelToString(Level level);
    void writeToFile(const QString& message);
    void writeToConsole(const QString& message);
    void rotateLogFile();

    std::unique_ptr<QFile> m_logFile;
    std::unique_ptr<QTextStream> m_stream;
    Level m_level = Info;
    QMutex m_mutex;
    QStringList m_logBuffer; // 日志缓冲区

    static Logger* s_instance;
};

// ========== 便捷宏 ==========

#define LOG_DEBUG(msg) Logger::instance()->debug(msg)
#define LOG_INFO(msg) Logger::instance()->info(msg)
#define LOG_WARNING(msg) Logger::instance()->warning(msg)
#define LOG_ERROR(msg) Logger::instance()->error(msg)

// 带格式化的日志宏
#define LOG_DEBUG_FMT(fmt, ...) LOG_DEBUG(QString(fmt).arg(__VA_ARGS__))
#define LOG_INFO_FMT(fmt, ...) LOG_INFO(QString(fmt).arg(__VA_ARGS__))
#define LOG_WARNING_FMT(fmt, ...) LOG_WARNING(QString(fmt).arg(__VA_ARGS__))
#define LOG_ERROR_FMT(fmt, ...) LOG_ERROR(QString(fmt).arg(__VA_ARGS__))

#endif // LOGGER_H
