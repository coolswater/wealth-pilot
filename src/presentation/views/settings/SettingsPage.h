/**
 * @file SettingsPage.h
 * @brief 设置页面 - 使用 DataHub 数据中心
 *
 * @details 功能：
 * - 外观设置（主题、字体大小）
 * - 通知设置
 * - 安全设置
 * - AI 配置
 * - 关于信息
 *
 * DataHub 集成：
 * - 通过 DataHub 订阅设置数据
 * - 自动生命周期管理
 * - 设置变更实时同步
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#ifndef SETTINGSPAGE_H
#define SETTINGSPAGE_H

#include <memory>
#include <presentation/components/DataHubPageBase.h>

class QComboBox;
class QSlider;
class QLineEdit;

/**
 * @brief 设置页面
 *
 * @details 继承 DataHubPageBase，自动管理数据订阅：
 * - 订阅设置变更（settings:*）
 * - 页面销毁时自动取消订阅
 */
class SettingsPage : public WealthPilot::DataHubPageBase
{
    Q_OBJECT

public:
    explicit SettingsPage(QWidget *parent = nullptr);
    ~SettingsPage() override;

    // ========== 页面信息 ==========

    QString pageId() const override;
    QString pageName() const override { return QStringLiteral("设置"); }

    /**
     * @brief 初始化页面
     *
     * @details 初始化流程：
     * 1. 设置 UI 组件
     * 2. 订阅 DataHub 设置数据
     * 3. 加载当前设置
     */
    void initializePage() override;

private slots:
    // ========== UI 交互槽函数 ==========

    void onThemeChanged(int index);
    void onFontSizeChanged(int value);
    void onNotificationChanged();
    void onClearCacheClicked();
    void onExportDataClicked();
    void onAIConfigChanged();
    void onSaveClicked();
    void onResetClicked();

private:
    // ========== UI 初始化 ==========

    void setupUI();
    void createAppearanceSection();
    void createNotificationSection();
    void createSecuritySection();
    void createAISection();
    void createAboutSection();
    void setupConnections();

    // ========== DataHub 数据订阅 ==========

    /**
     * @brief 设置 DataHub 数据订阅
     *
     * @details 订阅流程：
     * 1. 订阅设置变更（settings:changed）
     * 2. 回调函数中更新显示
     */
    void setupDataHubSubscriptions();

    // ========== 数据加载 ==========

    void loadSettings();
    void saveSettings();

    // ========== 私有实现类（PIMPL） ==========
    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // SETTINGSPAGE_H