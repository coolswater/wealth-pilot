/**
 * @file UseCaseBase.h
 * @brief 用例基类 - 定义用例执行模式
 */

#ifndef USECASE_BASE_H
#define USECASE_BASE_H

#include <QObject>
#include <QFuture>
#include <QtConcurrent>
#include <functional>

namespace WealthPilot {
namespace Application {

/**
 * @brief 用例执行结果状态
 */
enum class UseCaseStatus {
    Idle,
    Running,
    Success,
    Failed,
    Cancelled
};

/**
 * @brief 用例基类 - 定义执行模式和生命周期
 *
 * @details 用例 (Use Case) 是 Application 层的核心概念：
 * - 封装一个完整的业务操作流程
 * - 协调多个 Domain 服务和 Infrastructure
 * - 管理执行状态和错误处理
 */
class UseCaseBase : public QObject
{
    Q_OBJECT

public:
    explicit UseCaseBase(QObject* parent = nullptr);
    virtual ~UseCaseBase();

    /**
     * @brief 执行用例
     * @return 异步结果
     */
    QFuture<bool> execute();

    /**
     * @brief 取消执行
     */
    void cancel();

    /**
     * @brief 获取当前状态
     */
    UseCaseStatus status() const { return m_status; }

    /**
     * @brief 是否正在执行
     */
    bool isRunning() const { return m_status == UseCaseStatus::Running; }

    /**
     * @brief 获取错误信息
     */
    QString errorMessage() const { return m_errorMessage; }

signals:
    /**
     * @brief 状态变更信号
     */
    void statusChanged(UseCaseStatus status);

    /**
     * @brief 进度更新信号
     */
    void progressUpdated(int percent);

    /**
     * @brief 执行完成信号
     */
    void completed(bool success);

protected:
    /**
     * @brief 实际执行逻辑 - 子类实现
     */
    virtual bool doExecute() = 0;

    /**
     * @brief 取消逻辑 - 子类可选实现
     */
    virtual void doCancel() {}

    /**
     * @brief 设置错误信息
     */
    void setError(const QString& message);

    /**
     * @brief 更新进度
     */
    void setProgress(int percent);

private:
    UseCaseStatus m_status = UseCaseStatus::Idle;
    QString m_errorMessage;
    int m_progress = 0;
};

} // namespace Application
} // namespace WealthPilot

#endif // USECASE_BASE_H