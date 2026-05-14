/**
 * @file AboutUSPage.h
 * @brief 关于页面 - 使用 DataHub 数据中心
 *
 * @details 功能：
 * - 产品信息展示
 * - 版本信息
 * - 更新检查
 * - 许可证信息
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#ifndef ABOUTUSPAGE_H
#define ABOUTUSPAGE_H

#include <memory>
#include <ui/components/DataHubPageBase.h>

/**
 * @brief 关于页面
 *
 * @details 继承 DataHubPageBase，自动管理数据订阅：
 * - 订阅版本信息（app:version）
 * - 订阅更新状态（app:update）
 * - 页面销毁时自动取消订阅
 */
class AboutUSPage : public WealthPilot::DataHubPageBase
{
    Q_OBJECT

public:
    explicit AboutUSPage(QWidget *parent = nullptr);
    ~AboutUSPage() override;

    // ========== 页面信息 ==========

    QString pageId() const override;
    QString pageName() const override { return QStringLiteral("关于"); }

    /**
     * @brief 初始化页面
     *
     * @details 初始化流程：
     * 1. 设置 UI 组件
     * 2. 订阅 DataHub 版本数据
     * 3. 加载初始数据
     */
    void initializePage() override;

private slots:
    // ========== UI 交互槽函数 ==========

    void onCheckUpdateClicked();
    void onVisitWebsiteClicked();
    void onViewLicenseClicked();

private:
    // ========== UI 初始化 ==========

    void setupUI();
    void setupConnections();

    // ========== DataHub 数据订阅 ==========

    /**
     * @brief 设置 DataHub 数据订阅
     *
     * @details 订阅流程：
     * 1. 订阅版本信息（app:version）
     * 2. 订阅更新状态（app:update）
     */
    void setupDataHubSubscriptions();

    // ========== 私有实现类（PIMPL） ==========
    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // ABOUTUSPAGE_H