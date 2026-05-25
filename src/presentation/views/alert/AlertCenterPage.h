/**
 * @file AlertCenterPage.h
 * @brief 预警中心页面 - 使用 DataHub 数据中心
 *
 * @details 功能：
 * - 价格预警设置（涨跌幅、价格突破）
 * - 预警触发记录
 * - 消息推送设置
 * - 预警历史查询
 *
 * DataHub 集成：
 * - 通过 DataHub 订阅预警数据
 * - 自动生命周期管理
 * - 实时预警触发
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#ifndef ALERTCENTERPAGE_H
#define ALERTCENTERPAGE_H

#include "presentation/components/DataHubPageBase.h"
#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QLabel>
#include <QDateTime>
#include <memory>

/**
 * @brief 预警类型枚举
 */
enum class AlertType {
    PriceAbove,         ///< 价格高于
    PriceBelow,         ///< 价格低于
    ChangePercentAbove, ///< 涨幅超过
    ChangePercentBelow, ///< 跌幅超过
    VolumeAbove,        ///< 成交量高于
    VolumeBelow,        ///< 成交量低于
    MarginWarning,      ///< 保证金预警
    ProfitAbove,        ///< 盈利高于
    LossAbove,          ///< 亏损高于
    DrawdownWarning,    ///< 回撤预警
    Custom              ///< 自定义
};

/**
 * @brief 预警状态枚举
 */
enum class AlertStatus {
    Active,     ///< 活跃
    Triggered,  ///< 已触发
    Disabled    ///< 已禁用
};

/**
 * @brief 预警规则结构
 */
struct AlertRule {
    QString ruleId;         ///< 规则ID
    QString instrumentId;   ///< 合约代码
    AlertType type;         ///< 预警类型
    double threshold = 0.0; ///< 阈值
    bool isActive = true;   ///< 是否激活
    bool isTriggered = false; ///< 是否已触发
    QDateTime createTime;   ///< 创建时间
    QDateTime triggerTime;  ///< 触发时间
};

/**
 * @brief 预警记录结构
 */
struct AlertRecord {
    QString symbol;         ///< 股票代码
    QString name;           ///< 股票名称
    AlertType type;         ///< 预警类型
    double threshold = 0.0; ///< 阈值
    double actualValue = 0.0; ///< 实际值
    QDateTime triggerTime;  ///< 触发时间
    QString message;        ///< 消息
};

/**
 * @brief 预警中心页面
 *
 * @details 继承 DataHubPageBase，自动管理数据订阅：
 * - 订阅预警规则（alert:rules）
 * - 订阅预警触发（alert:triggered）
 * - 页面销毁时自动取消订阅
 */
class AlertCenterPage : public WealthPilot::DataHubPageBase
{
    Q_OBJECT

public:
    explicit AlertCenterPage(QWidget *parent = nullptr);
    ~AlertCenterPage() override;

    // ========== 页面信息 ==========

    QString pageId() const override { return QStringLiteral("AlertCenter"); }
    QString pageName() const override { return QStringLiteral("预警中心"); }

    /**
     * @brief 初始化页面
     *
     * @details 初始化流程：
     * 1. 设置 UI 组件
     * 2. 订阅 DataHub 预警数据
     * 3. 加载初始数据
     */
    void initializePage() override;

    /**
     * @brief 刷新数据
     */
    void refresh();

signals:
    /**
     * @brief 预警选中信号
     */
    void alertSelected(const QString& ruleId);

private slots:
    // ========== UI 交互槽函数 ==========

    void onAddAlert();
    void onDeleteAlert();
    void onToggleAlert();
    void onAlertListClicked(int row, int column);
    void onRefreshData();
    void onClearHistory();

private:
    // ========== UI 初始化 ==========

    void setupUI();
    void initToolBar();
    void initAlertList();
    void initHistoryList();
    void initConnections();

    // ========== DataHub 数据订阅 ==========

    /**
     * @brief 设置 DataHub 数据订阅
     *
     * @details 订阅流程：
     * 1. 订阅预警规则（alert:rules）
     * 2. 订阅预警触发事件（alert:triggered）
     * 3. 回调函数中更新列表显示
     */
    void setupDataHubSubscriptions();

    // ========== 数据加载 ==========

    void loadAlertRules();
    void loadAlertHistory();
    void addAlertRule(const AlertRule& rule);
    void removeAlertRule(const QString& ruleId);
    void toggleAlertRule(const QString& ruleId);

    // ========== 格式化函数 ==========

    QString formatAlertType(AlertType type) const;
    QString formatAlertStatus(AlertStatus status) const;

    // ========== 私有实现类（PIMPL） ==========
    struct Impl;
    std::unique_ptr<Impl> d;

    // ========== DataHub 相关 ==========

    /**
     * @brief 已订阅的预警ID列表
     */
    QStringList m_subscribedAlertIds;
};

#endif // ALERTCENTERPAGE_H