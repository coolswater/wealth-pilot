/**
 * @file FeedbackTranslations.h
 * @brief 用户反馈系统国际化支持
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef FEEDBACKTRANSLATIONS_H
#define FEEDBACKTRANSLATIONS_H

#include <QCoreApplication>

namespace WealthPilot {
namespace Feedback {

/**
 * @brief 反馈系统翻译上下文
 *
 * 提供所有反馈相关的可翻译字符串
 */
class FeedbackTranslations {
    Q_DECLARE_TR_FUNCTIONS(FeedbackTranslations)

public:
    // ========== 通用反馈消息 ==========

    static QString operationSuccess() {
        return tr("操作成功");
    }

    static QString operationFailed() {
        return tr("操作失败");
    }

    static QString operationCancelled() {
        return tr("操作已取消");
    }

    // ========== 登录相关 ==========

    static QString loginSuccess() {
        return tr("登录成功");
    }

    static QString loginFailed() {
        return tr("登录失败");
    }

    static QString logoutSuccess() {
        return tr("已退出登录");
    }

    static QString welcomeBack() {
        return tr("欢迎回来！");
    }

    // ========== 数据加载 ==========

    static QString loadingData() {
        return tr("加载数据");
    }

    static QString dataLoadSuccess() {
        return tr("数据加载成功");
    }

    static QString dataLoadFailed() {
        return tr("数据加载失败");
    }

    static QString noDataAvailable() {
        return tr("暂无数据");
    }

    // ========== 网络相关 ==========

    static QString networkError() {
        return tr("网络错误");
    }

    static QString networkTimeout() {
        return tr("网络超时");
    }

    static QString networkDisconnected() {
        return tr("网络连接已断开");
    }

    static QString checkNetworkConnection() {
        return tr("请检查网络连接后重试");
    }

    static QString unableToConnectServer() {
        return tr("无法连接到服务器");
    }

    // ========== 文件操作 ==========

    static QString fileSaveSuccess() {
        return tr("文件保存成功");
    }

    static QString fileSaveFailed() {
        return tr("文件保存失败");
    }

    static QString fileOpenFailed() {
        return tr("文件打开失败");
    }

    static QString fileNotFound() {
        return tr("文件不存在");
    }

    // ========== 删除确认 ==========

    static QString confirmDelete() {
        return tr("确认删除");
    }

    static QString confirmDeleteItem(const QString& itemName) {
        return tr("确定要删除 '%1' 吗？此操作不可撤销。").arg(itemName);
    }

    static QString confirmDeleteMultiple(int count) {
        return tr("确定要删除选中的 %1 项吗？此操作不可撤销。").arg(count);
    }

    // ========== 保存确认 ==========

    static QString saveChanges() {
        return tr("保存更改");
    }

    static QString confirmSaveChanges() {
        return tr("是否保存当前更改？");
    }

    static QString unsavedChanges() {
        return tr("未保存的更改");
    }

    // ========== 输入验证 ==========

    static QString inputError() {
        return tr("输入错误");
    }

    static QString fieldRequired(const QString& fieldName) {
        return tr("%1不能为空").arg(fieldName);
    }

    static QString fieldInvalid(const QString& fieldName, const QString& reason) {
        return tr("%1格式错误：%2").arg(fieldName, reason);
    }

    static QString fieldTooLong(const QString& fieldName, int maxLength) {
        return tr("%1长度不能超过 %2 个字符").arg(fieldName).arg(maxLength);
    }

    static QString fieldTooShort(const QString& fieldName, int minLength) {
        return tr("%1长度不能少于 %2 个字符").arg(fieldName).arg(minLength);
    }

    // ========== 订单相关 ==========

    static QString orderSubmitted() {
        return tr("订单提交成功");
    }

    static QString orderCancelled() {
        return tr("订单已取消");
    }

    static QString orderFailed() {
        return tr("订单提交失败");
    }

    static QString orderId(const QString& id) {
        return tr("订单号：%1").arg(id);
    }

    // ========== 设置相关 ==========

    static QString settingsSaved() {
        return tr("设置已保存");
    }

    static QString settingsReset() {
        return tr("设置已重置");
    }

    static QString settingsImported() {
        return tr("设置已导入");
    }

    static QString settingsExported() {
        return tr("设置已导出");
    }

    // ========== 批量操作 ==========

    static QString batchImport() {
        return tr("批量导入");
    }

    static QString batchExport() {
        return tr("批量导出");
    }

    static QString importProgress(int current, int total) {
        return tr("正在导入：%1/%2").arg(current).arg(total);
    }

    static QString importComplete(int success, int failed) {
        if (failed > 0) {
            return tr("导入完成：成功 %1 项，失败 %2 项").arg(success).arg(failed);
        }
        return tr("导入完成：成功 %1 项").arg(success);
    }

    static QString exportComplete(int count) {
        return tr("导出完成：共 %1 项").arg(count);
    }

    // ========== 风险提示 ==========

    static QString riskWarning() {
        return tr("风险警告");
    }

    static QString highRiskOperation() {
        return tr("此操作风险较高，请谨慎操作");
    }

    static QString positionLimitExceeded() {
        return tr("持仓超过限制");
    }

    static QString insufficientBalance() {
        return tr("余额不足");
    }

    // ========== 进度提示 ==========

    static QString processing() {
        return tr("处理中...");
    }

    static QString pleaseWait() {
        return tr("请稍候...");
    }

    static QString cancel() {
        return tr("取消");
    }

    static QString confirm() {
        return tr("确定");
    }

    static QString ok() {
        return tr("确定");
    }

    // ========== 按钮文本 ==========

    static QString save() {
        return tr("保存");
    }

    static QString discard() {
        return tr("不保存");
    }

    static QString delete_() {
        return tr("删除");
    }

    static QString retry() {
        return tr("重试");
    }

    static QString close() {
        return tr("关闭");
    }
};

} // namespace Feedback
} // namespace WealthPilot

#endif // FEEDBACKTRANSLATIONS_H