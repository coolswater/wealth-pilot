/**
 * @file WarningPage.h
 * @brief 预警页面 - 使用 DataHub 数据中心
 *
 * @details 功能：
 * - 预警消息展示
 * - 预警历史记录
 * - 预警设置入口
 *
 * DataHub 集成：
 * - 通过 DataHub 订阅预警数据
 * - 自动生命周期管理
 * - 实时预警通知
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#ifndef WARNINGPAGE_H
#define WARNINGPAGE_H

#include <memory>
#include "ui/components/DataHubPageBase.h"

namespace WealthPilot {

/**
 * @brief 预警页面
 *
 * @details 继承 DataHubPageBase，自动管理数据订阅：
 * - 订阅预警消息（warning:messages）
 * - 页面销毁时自动取消订阅
 */
class WarningPage : public DataHubPageBase
{
    Q_OBJECT

public:
    explicit WarningPage(QWidget *parent = nullptr);
    ~WarningPage() override;

    // ========== 页面信息 ==========

    QString pageId() const override { return "warning"; }
    QString pageName() const override { return QStringLiteral("预警"); }

    /**
     * @brief 初始化页面
     *
     * @details 初始化流程：
     * 1. 设置 UI 组件
     * 2. 订阅 DataHub 预警数据
     * 3. 加载初始数据
     */
    void initializePage() override;

private:
    // ========== UI 初始化 ==========

    void setupUI();
    void setupConnections();

    // ========== DataHub 数据订阅 ==========

    /**
     * @brief 设置 DataHub 数据订阅
     *
     * @details 订阅流程：
     * 1. 订阅预警消息（warning:messages）
     * 2. 回调函数中更新显示
     */
    void setupDataHubSubscriptions();

    // ========== 私有实现类（PIMPL） ==========
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace WealthPilot

#endif // WARNINGPAGE_H