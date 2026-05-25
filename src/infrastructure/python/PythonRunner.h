/**
 * @file PythonRunner.h
 * @brief Python 脚本执行器 - 嵌入式 Python 分析支持
 *
 * @details 提供 C++ 调用 Python 脚本的能力，用于量化分析、机器学习等场景。
 * 参考 FinceptTerminal 的 PythonRunner 实现。
 *
 * @example
 * // 执行 Python 脚本
 * auto result = PythonRunner::instance().execute("scripts/analytics/dcf.py", {"AAPL"});
 * if (result.isOk()) {
 *     qDebug() << "Result:" << result.value();
 * }
 */

#ifndef WEALTHPILOT_PYTHON_RUNNER_H
#define WEALTHPILOT_PYTHON_RUNNER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QProcess>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMutex>
#include <memory>
#include "../core/base/Result.h"

namespace WealthPilot {

/**
 * @brief Python 执行结果
 */
struct PythonResult {
    bool success = false;
    QString output;           // 标准输出
    QString error;            // 标准错误
    int exitCode = -1;        // 退出码
    QVariant data;            // 解析后的数据（如果是 JSON）
    qint64 executionTime = 0; // 执行时间（毫秒）
};

/**
 * @brief Python 脚本执行器
 */
class PythonRunner : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 获取单例实例
     */
    static PythonRunner& instance() {
        static PythonRunner instance;
        return instance;
    }

    /**
     * @brief 设置 Python 解释器路径
     */
    void setPythonPath(const QString& path) {
        m_pythonPath = path;
    }

    /**
     * @brief 设置脚本目录
     */
    void setScriptsDir(const QString& dir) {
        m_scriptsDir = dir;
    }

    /**
     * @brief 设置超时时间（毫秒）
     */
    void setTimeout(int ms) {
        m_timeout = ms;
    }

    /**
     * @brief 检查 Python 是否可用
     */
    bool isPythonAvailable() const {
        QProcess process;
        process.start(m_pythonPath, {"--version"});
        bool available = process.waitForStarted() && process.waitForFinished(5000);
        return available && process.exitCode() == 0;
    }

    /**
     * @brief 获取 Python 版本
     */
    QString pythonVersion() const {
        QProcess process;
        process.start(m_pythonPath, {"--version"});
        if (process.waitForFinished(5000)) {
            return QString::fromUtf8(process.readAllStandardOutput()).trimmed();
        }
        return QString();
    }

    /**
     * @brief 执行 Python 脚本（同步）
     * @param scriptPath 脚本路径（相对于 scripts 目录或绝对路径）
     * @param args 参数列表
     * @param env 环境变量
     * @return 执行结果
     */
    Result<PythonResult> execute(const QString& scriptPath,
                                  const QStringList& args = {},
                                  const QMap<QString, QString>& env = {}) {
        QMutexLocker locker(&m_mutex);

        PythonResult result;
        QElapsedTimer timer;
        timer.start();

        // 构建完整路径
        QString fullPath = scriptPath;
        if (!QFileInfo(scriptPath).isAbsolute()) {
            fullPath = m_scriptsDir + "/" + scriptPath;
        }

        // 检查文件是否存在
        if (!QFileInfo::exists(fullPath)) {
            return Result<PythonResult>::error("SCRIPT_NOT_FOUND",
                QString("Script not found: %1").arg(fullPath));
        }

        // 构建命令
        QStringList allArgs;
        allArgs << fullPath << args;

        // 创建进程
        QProcess process;
        
        // 设置环境变量
        QProcessEnvironment processEnv = QProcessEnvironment::systemEnvironment();
        for (auto it = env.begin(); it != env.end(); ++it) {
            processEnv.insert(it.key(), it.value());
        }
        process.setProcessEnvironment(processEnv);

        // 启动进程
        process.start(m_pythonPath, allArgs);

        if (!process.waitForStarted()) {
            return Result<PythonResult>::error("PROCESS_START_FAILED",
                QString("Failed to start Python process: %1").arg(process.errorString()));
        }

        // 等待完成
        if (!process.waitForFinished(m_timeout)) {
            process.kill();
            return Result<PythonResult>::error("TIMEOUT",
                QString("Script execution timed out after %1ms").arg(m_timeout));
        }

        // 收集输出
        result.success = (process.exitCode() == 0);
        result.output = QString::fromUtf8(process.readAllStandardOutput());
        result.error = QString::fromUtf8(process.readAllStandardError());
        result.exitCode = process.exitCode();
        result.executionTime = timer.elapsed();

        // 尝试解析 JSON 输出
        if (result.success && !result.output.isEmpty()) {
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(result.output.toUtf8(), &parseError);
            if (parseError.error == QJsonParseError::NoError) {
                if (doc.isObject()) {
                    result.data = doc.object().toVariantMap();
                } else if (doc.isArray()) {
                    result.data = doc.array().toVariantList();
                }
            }
        }

        if (!result.success) {
            return Result<PythonResult>::error("SCRIPT_ERROR", result.error);
        }

        return Result<PythonResult>::ok(result);
    }

    /**
     * @brief 执行 Python 代码（同步）
     * @param code Python 代码
     * @return 执行结果
     */
    Result<PythonResult> executeCode(const QString& code) {
        QMutexLocker locker(&m_mutex);

        PythonResult result;
        QElapsedTimer timer;
        timer.start();

        QProcess process;
        process.start(m_pythonPath, {"-c", code});

        if (!process.waitForStarted()) {
            return Result<PythonResult>::error("PROCESS_START_FAILED",
                QString("Failed to start Python process: %1").arg(process.errorString()));
        }

        if (!process.waitForFinished(m_timeout)) {
            process.kill();
            return Result<PythonResult>::error("TIMEOUT",
                QString("Code execution timed out after %1ms").arg(m_timeout));
        }

        result.success = (process.exitCode() == 0);
        result.output = QString::fromUtf8(process.readAllStandardOutput());
        result.error = QString::fromUtf8(process.readAllStandardError());
        result.exitCode = process.exitCode();
        result.executionTime = timer.elapsed();

        if (!result.success) {
            return Result<PythonResult>::error("CODE_ERROR", result.error);
        }

        return Result<PythonResult>::ok(result);
    }

    /**
     * @brief 异步执行 Python 脚本
     */
    void executeAsync(const QString& scriptPath,
                      const QStringList& args = {},
                      std::function<void(const Result<PythonResult>&)> callback = nullptr) {
        QtConcurrent::run([this, scriptPath, args, callback]() {
            auto result = execute(scriptPath, args);
            if (callback) {
                callback(result);
            }
            emit executionFinished(scriptPath, result.isOk());
        });
    }

    /**
     * @brief 安装 Python 依赖
     */
    Result<void> installPackage(const QString& packageName) {
        QProcess process;
        process.start(m_pythonPath, {"-m", "pip", "install", packageName});

        if (!process.waitForStarted()) {
            return Result<void>::error("PROCESS_START_FAILED",
                QString("Failed to start pip: %1").arg(process.errorString()));
        }

        if (!process.waitForFinished(120000)) { // 2 分钟超时
            process.kill();
            return Result<void>::error("TIMEOUT", "Package installation timed out");
        }

        if (process.exitCode() != 0) {
            return Result<void>::error("INSTALL_FAILED",
                QString::fromUtf8(process.readAllStandardError()));
        }

        return Result<void>::ok();
    }

    /**
     * @brief 检查包是否已安装
     */
    bool isPackageInstalled(const QString& packageName) {
        QProcess process;
        process.start(m_pythonPath, {"-c", 
            QString("import importlib.util; print(importlib.util.find_spec('%1') is not None)")
                .arg(packageName)});
        
        if (process.waitForFinished(5000)) {
            QString output = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
            return output == "True";
        }
        return false;
    }

signals:
    /**
     * @brief 执行完成信号
     */
    void executionFinished(const QString& scriptPath, bool success);

    /**
     * @brief 执行进度信号
     */
    void executionProgress(const QString& scriptPath, const QString& output);

private:
    PythonRunner() 
        : m_pythonPath("python")
        , m_timeout(30000) // 默认 30 秒超时
    {
        // 尝试检测 Python 路径
#ifdef Q_OS_WIN
        // Windows: 尝试 python 和 python3
        if (!isPythonAvailable()) {
            m_pythonPath = "python3";
        }
#else
        // Linux/macOS: 优先使用 python3
        m_pythonPath = "python3";
        if (!isPythonAvailable()) {
            m_pythonPath = "python";
        }
#endif
    }

    PythonRunner(const PythonRunner&) = delete;
    PythonRunner& operator=(const PythonRunner&) = delete;

    QString m_pythonPath;
    QString m_scriptsDir;
    int m_timeout;
    mutable QMutex m_mutex;
};

} // namespace WealthPilot

#endif // WEALTHPILOT_PYTHON_RUNNER_H
