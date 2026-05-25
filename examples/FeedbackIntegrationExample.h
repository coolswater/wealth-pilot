/**
 * @file FeedbackIntegrationExample.h
 * @brief 用户反馈系统集成示例
 *
 * @details 展示如何在各个页面中使用 UserFeedbackManager
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef FEEDBACKINTEGRATIONEXAMPLE_H
#define FEEDBACKINTEGRATIONEXAMPLE_H

#include <QObject>
#include "../core/feedback/UserFeedbackManager.h"

namespace WealthPilot {
namespace Examples {

/**
 * @brief 反馈系统集成示例类
 *
 * 展示各种场景下的反馈使用方式
 */
class FeedbackIntegrationExample : public QObject {
    Q_OBJECT

public:
    explicit FeedbackIntegrationExample(QObject* parent = nullptr)
        : QObject(parent) {}

public slots:
    // ========== 登录场景 ==========

    /**
     * @brief 登录成功
     */
    void onLoginSuccess() {
        // 使用便捷函数
        showSuccess(tr("登录成功"), tr("欢迎回来！"));
    }

    /**
     * @brief 登录失败
     */
    void onLoginFailed(const QString& reason) {
        // 使用管理器实例
        UserFeedbackManager::instance()->showError(
            tr("登录失败"),
            reason,
            UserFeedbackManager::FeedbackLevel::Dialog
        );
    }

    // ========== 数据加载场景 ==========

    /**
     * @brief 开始加载数据
     */
    void onLoadDataStart() {
        // 显示进度
        ProgressConfig config;
        config.title = tr("加载数据");
        config.cancelButtonText = tr("取消");
        config.minimum = 0;
        config.maximum = 100;
        config.cancellable = true;

        UserFeedbackManager::instance()->beginProgress("load_data", config);
    }

    /**
     * @brief 加载进度更新
     */
    void onLoadDataProgress(int percent, const QString& status) {
        UserFeedbackManager::instance()->updateProgress("load_data", percent, status);
    }

    /**
     * @brief 数据加载完成
     */
    void onLoadDataComplete(bool success, const QString& message) {
        UserFeedbackManager::instance()->endProgress("load_data", success, message);
    }

    /**
     * @brief 数据加载失败
     */
    void onLoadDataError(const QString& error) {
        showError(tr("数据加载失败"), error);
    }

    // ========== 操作确认场景 ==========

    /**
     * @brief 删除确认
     */
    bool confirmDelete(const QString& itemName) {
        return showConfirm(
            tr("确认删除"),
            tr("确定要删除 '%1' 吗？此操作不可撤销。").arg(itemName),
            tr("删除"),
            tr("取消")
        );
    }

    /**
     * @brief 保存确认
     */
    bool confirmSave() {
        return showConfirm(
            tr("保存更改"),
            tr("是否保存当前更改？"),
            tr("保存"),
            tr("不保存")
        );
    }

    // ========== 网络请求场景 ==========

    /**
     * @brief 网络请求超时
     */
    void onNetworkTimeout() {
        showWarning(
            tr("网络超时"),
            tr("请求超时，请检查网络连接后重试"),
            UserFeedbackManager::FeedbackLevel::Toast
        );
    }

    /**
     * @brief 网络连接失败
     */
    void onNetworkError(const QString& error) {
        showError(
            tr("网络错误"),
            tr("无法连接到服务器：%1").arg(error),
            UserFeedbackManager::FeedbackLevel::Dialog
        );
    }

    // ========== 数据验证场景 ==========

    /**
     * @brief 输入验证失败
     */
    void onValidationFailed(const QString& field, const QString& reason) {
        showWarning(
            tr("输入错误"),
            tr("%1：%2").arg(field, reason),
            UserFeedbackManager::FeedbackLevel::Toast
        );
    }

    // ========== 操作成功场景 ==========

    /**
     * @brief 订单提交成功
     */
    void onOrderSubmitted(const QString& orderId) {
        showSuccess(
            tr("订单提交成功"),
            tr("订单号：%1").arg(orderId),
            UserFeedbackManager::FeedbackLevel::Toast
        );
    }

    /**
     * @brief 设置保存成功
     */
    void onSettingsSaved() {
        showInfo(
            tr("设置已保存"),
            tr("您的设置已成功保存"),
            UserFeedbackManager::FeedbackLevel::Toast,
            2000
        );
    }

    // ========== 风险提示场景 ==========

    /**
     * @brief 风险警告
     */
    void onRiskWarning(const QString& risk) {
        showWarning(
            tr("风险警告"),
            risk,
            UserFeedbackManager::FeedbackLevel::Dialog
        );
    }

    // ========== 批量操作场景 ==========

    /**
     * @brief 批量导入开始
     */
    void onBatchImportStart(int total) {
        ProgressConfig config;
        config.title = tr("批量导入");
        config.cancelButtonText = tr("取消");
        config.minimum = 0;
        config.maximum = total;
        config.cancellable = true;

        UserFeedbackManager::instance()->beginProgress("batch_import", config);
    }

    /**
     * @brief 批量导入进度
     */
    void onBatchImportProgress(int current, int total, const QString& currentItem) {
        int percent = static_cast<int>((current * 100.0) / total);
        UserFeedbackManager::instance()->updateProgress(
            "batch_import",
            current,
            tr("正在导入：%1 (%2/%3)").arg(currentItem).arg(current).arg(total)
        );
    }

    /**
     * @brief 批量导入完成
     */
    void onBatchImportComplete(int success, int failed) {
        QString message;
        if (failed > 0) {
            message = tr("成功导入 %1 项，失败 %2 项").arg(success).arg(failed);
            showWarning(tr("导入完成"), message);
        } else {
            message = tr("成功导入 %1 项").arg(success);
            showSuccess(tr("导入完成"), message);
        }

        UserFeedbackManager::instance()->endProgress("batch_import", true, message);
    }

    // ========== 使用宏的示例 ==========

    void exampleUsingMacros() {
        // 信息提示
        FEEDBACK_INFO("提示", "这是一条信息");

        // 警告提示
        FEEDBACK_WARNING("警告", "这是一条警告");

        // 错误提示
        FEEDBACK_ERROR("错误", "这是一条错误");

        // 成功提示
        FEEDBACK_SUCCESS("成功", "操作成功");

        // 进度操作
        ProgressConfig config;
        config.title = "处理中";
        PROGRESS_BEGIN("operation", config);
        PROGRESS_UPDATE("operation", 50, "处理中...");
        PROGRESS_END("operation", true, "处理完成");
    }
};

} // namespace Examples
} // namespace WealthPilot

#endif // FEEDBACKINTEGRATIONEXAMPLE_H