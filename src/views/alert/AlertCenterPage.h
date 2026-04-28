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
#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QLabel>
#include <memory>

/**
 * @brief 预警类型枚举
 */
enum class AlertType {
    PriceAbove,     ///< 价格高于
    PriceBelow,     ///< 价格低于
    ChangeAbove,    ///< 涨幅高于
    ChangeBelow,    ///< 跌幅低于
    VolumeAbove,    ///< 成交量高于
    TurnoverAbove   ///< 换手率高于
};

/**
 * @brief 预警状态枚举
 */
enum class AlertStatus {
    Active,         ///< 激活中
    Triggered,      ///< 已触发
    Disabled        ///< 已禁用
};

/**
 * @brief 预警规则数据结构
 */
struct AlertRule {
    QString id;                 ///< 预警ID
    QString symbol;             ///< 标的代码
    QString name;               ///< 标的名称
    AlertType type;             ///< 预警类型
    double threshold = 0.0;     ///< 阈值
    AlertStatus status;         ///< 状态
    QDateTime createTime;       ///< 创建时间
    QDateTime triggerTime;      ///< 触发时间
    QString message;            ///< 预警消息
};

/**
 * @brief 预警记录数据结构
 */
struct AlertRecord {
    QString id;                 ///< 记录ID
    QString symbol;             ///< 标的代码
    QString name;               ///< 标的名称
    AlertType type;             ///< 预警类型
    double threshold = 0.0;     ///< 阈值
    double actualValue = 0.0;   ///< 实际值
    QDateTime triggerTime;      ///< 触发时间
    QString message;            ///< 消息
    bool isRead = false;        ///< 是否已读
};

/**
 * @brief 预警中心页面类
 */
class AlertCenterPage : public BasePage
{
    Q_OBJECT

public:
    explicit AlertCenterPage(QWidget *parent = nullptr);
    ~AlertCenterPage() override;

    QString pageId() const override { return QStringLiteral("AlertCenter"); }
    QString pageName() const override { return QStringLiteral("预警中心"); }

    void initialize() override;
    void refresh() override;

signals:
    void alertTriggered(const AlertRecord& record);

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
    void removeAlertRule(const QString& id);
    void toggleAlertRule(const QString& id);
    static QString formatAlertType(AlertType type);
    static QString formatAlertStatus(AlertStatus status);

    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // ALERTCENTERPAGE_H