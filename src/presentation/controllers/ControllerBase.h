/**
 * @file ControllerBase.h
 * @brief Controller 基类 - Widget 页面的业务逻辑控制器
 * 
 * @details 用于 Widget 页面的 MVP 模式：
 * - View 只负责 UI 渲染
 * - Controller 处理用户交互和业务逻辑
 * - Model 负责数据
 * 
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef CONTROLLERBASE_H
#define CONTROLLERBASE_H

#include <QObject>
#include <QVariant>
#include <QHash>
#include <QString>
#include <QtConcurrent>
#include "services/di/ServiceLocator.h"
#include <functional>
#include <memory>

namespace WealthPilot
{
    /**
 * @brief Controller 基类
 *
 * @details 职责：
 * - 处理用户交互逻辑
 * - 协调 View 和 Service
 * - 管理页面状态
 * - 提供数据给 View 显示
 *
 * @example
 * @code
 * // StockQuotesController.h
 * class StockQuotesController : public ControllerBase {
 *     Q_OBJECT
 * public:
 *     void refreshData();
 *     void searchData(const QString& keyword);
 * signals:
 *     void dataRefreshed(int count);
 *     void errorOccurred(const QString& error);
 * };
 *
 * // StockQuotesPage.cpp
 * void StockQuotesPage::setupUI() {
 *     m_controller = new StockQuotesController(this);
 *     connect(m_refreshBtn, &QPushButton::clicked,
 *             m_controller, &StockQuotesController::refreshData);
 *     connect(m_controller, &StockQuotesController::dataRefreshed,
 *             this, [this](int count) {
 *                 m_statusLabel->setText(QString("已加载 %1 条").arg(count));
 *             });
 * }
 * @endcode
 */
    class ControllerBase : public QObject
    {
        Q_OBJECT

    public:
        explicit ControllerBase(QObject* parent = nullptr);
        virtual ~ControllerBase();

        // ========== 生命周期 ==========

        /**
     * @brief 初始化控制器
     */
        virtual void initialize();

        /**
     * @brief 清理资源
     */
        virtual void cleanup();

        /**
     * @brief 重置状态
     */
        virtual void reset();

        // ========== 服务注入 ==========

        /**
     * @brief 获取服务
     */
        template <typename T>
        T* getService() const;

        /**
     * @brief 检查服务是否可用
     */
        template <typename T>
        bool hasService() const;

        // ========== 状态管理 ==========

        /**
     * @brief 设置状态
     */
        void setState(const QString& key, const QVariant& value);

        /**
     * @brief 获取状态
     */
        QVariant getState(const QString& key) const;

        /**
     * @brief 检查状态是否存在
     */
        bool hasState(const QString& key) const;

        /**
     * @brief 清除状态
     */
        void clearState(const QString& key);

        /**
     * @brief 清除所有状态
     */
        void clearAllStates();

        // ========== 异步操作 ==========

        /**
     * @brief 执行异步任务
     * @param task 任务函数
     * @param onSuccess 成功回调
     * @param onError 错误回调
     */
        template <typename TaskFunc, typename SuccessFunc, typename ErrorFunc>
        void executeAsync(TaskFunc task, SuccessFunc onSuccess, ErrorFunc onError);

        /**
     * @brief 执行异步任务（带进度）
     */
        template <typename TaskFunc, typename ProgressFunc, typename SuccessFunc, typename ErrorFunc>
        void executeAsyncWithProgress(TaskFunc task, ProgressFunc onProgress,
                                      SuccessFunc onSuccess, ErrorFunc onError);

        // ========== 错误处理 ==========

        /**
     * @brief 设置错误
     */
        void setError(const QString& error);

        /**
     * @brief 清除错误
     */
        void clearError();

        /**
     * @brief 获取错误信息
     */
        QString lastError() const { return m_lastError; }

        /**
     * @brief 是否有错误
     */
        bool hasError() const { return !m_lastError.isEmpty(); }

        signals :
        /**
     * @brief 状态变化
     */

        void stateChanged(const QString& key, const QVariant& value);

        /**
     * @brief 错误发生
     */
        void errorOccurred(const QString& error);

        /**
     * @brief 操作开始
     */
        void operationStarted(const QString& operation);

        /**
     * @brief 操作完成
     */
        void operationCompleted(const QString& operation, bool success);

        /**
     * @brief 进度更新
     */
        void progressChanged(int progress, const QString& message);

    protected:
        /**
     * @brief 开始操作
     */
        void beginOperation(const QString& operation);

        /**
     * @brief 结束操作
     */
        void endOperation(const QString& operation, bool success = true);

        /**
     * @brief 更新进度
     */
        void updateProgress(int progress, const QString& message = QString());

    private:
        QHash<QString, QVariant> m_states;
        QString m_lastError;
        QString m_currentOperation;
        bool m_isOperating = false;
    };

    // ============================================================================
    // 模板实现
    // ============================================================================

    template <typename T>
    T* ControllerBase::getService() const
    {
        return ServiceLocator::instance().template resolve<T>();
    }

    template <typename T>
    bool ControllerBase::hasService() const
    {
        return ServiceLocator::instance().template isRegistered<T>();
    }

    template <typename TaskFunc, typename SuccessFunc, typename ErrorFunc>
    void ControllerBase::executeAsync(TaskFunc task, SuccessFunc onSuccess, ErrorFunc onError)
    {
        QtConcurrent::run([this, task, onSuccess, onError]()
        {
            try
            {
                auto result = task();
                QMetaObject::invokeMethod(this, [this, result, onSuccess]()
                {
                    if (onSuccess)
                    {
                        onSuccess(result);
                    }
                }, Qt::QueuedConnection);
            }
            catch (const std::exception& e)
            {
                QString error = QString::fromStdString(e.what());
                QMetaObject::invokeMethod(this, [this, error, onError]()
                {
                    setError(error);
                    if (onError)
                    {
                        onError(error);
                    }
                }, Qt::QueuedConnection);
            }
        });
    }
} // namespace WealthPilot

#endif // CONTROLLERBASE_H
