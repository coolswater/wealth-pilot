/**
 * @file AlertSettingsWidget.h
 * @brief 智能预警设置面板 - 集成到设置页面
 *
 * @details 功能：
 * - 推送渠道配置（微信/邮件/Webhook等）
 * - 预警规则设置
 * - 渠道测试
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef ALERTSETTINGSWIDGET_H
#define ALERTSETTINGSWIDGET_H

#include <QWidget>
#include <QMap>
#include <memory>

class QCheckBox;
class QLineEdit;
class QSpinBox;
class QPushButton;
class QGroupBox;

namespace WealthPilot {

// 前向声明
enum class AlertChannel;

/**
 * @brief 智能预警设置面板
 */
class AlertSettingsWidget : public QWidget {
    Q_OBJECT

public:
    explicit AlertSettingsWidget(QWidget* parent = nullptr);
    ~AlertSettingsWidget() override;

    /**
     * @brief 加载配置
     */
    void loadConfig();

    /**
     * @brief 保存配置
     */
    void saveConfig();

signals:
    /**
     * @brief 配置变更信号
     */
    void configChanged();

private slots:
    void onTestWeChat();
    void onTestWeChatWork();
    void onTestEmail();
    void onTestDingTalk();

private:
    void setupUI();
    QGroupBox* createWeChatGroup();
    QGroupBox* createWeChatWorkGroup();
    QGroupBox* createEmailGroup();
    QGroupBox* createDingTalkGroup();
    QGroupBox* createRuleGroup();
    void setupConnections();
    void syncToService();

    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace WealthPilot

#endif // ALERTSETTINGSWIDGET_H
