/**
 * @file AlertCenterPage.h
 * @brief 预警中心页面 - 价格预警与消息通知
 *
 * @details 功能：
 * - 价格预警设置（涨跌幅、价格突破）
 * - 预警触发记录
 * - 消息推送设置
 * - 预警历史查询
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef ALERTCENTERPAGE_H
#define ALERTCENTERPAGE_H

#include "core/base/BasePage.h"
#include "trading/AlertManager.h"
#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QLabel>
#include <QDateTime>
#include <memory>

enum class AlertStatus {
    Active,
    Triggered,
    Disabled
};

struct AlertRecord {
    QString symbol;
    QString name;
    AlertType type;
    double threshold = 0.0;
    double actualValue = 0.0;
    QDateTime triggerTime;
    QString message;
};

class AlertCenterPage : public BasePage
{
    Q_OBJECT

public:
    explicit AlertCenterPage(QWidget *parent = nullptr);
    ~AlertCenterPage() override;

    QString pageId() const override { return QStringLiteral("AlertCenter"); }
    QString pageName() const override { return QStringLiteral("预警中心"); }

    void initializePage() override;
    void refresh();

signals:
    void alertSelected(const QString& ruleId);

private slots:
    void onAddAlert();
    void onDeleteAlert();
    void onToggleAlert();
    void onAlertListClicked(int row, int column);
    void onRefreshData();
    void onClearHistory();

private:
    void setupUI();
    void initToolBar();
    void initAlertList();
    void initHistoryList();
    void initConnections();
    void loadAlertRules();
    void loadAlertHistory();
    void addAlertRule(const AlertRule& rule);
    void removeAlertRule(const QString& ruleId);
    void toggleAlertRule(const QString& ruleId);
    QString formatAlertType(AlertType type) const;
    QString formatAlertStatus(AlertStatus status) const;

    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // ALERTCENTERPAGE_H
