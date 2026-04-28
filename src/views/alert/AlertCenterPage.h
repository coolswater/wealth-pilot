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
#include "trading/AlertManager.h"  // 使用已有的预警类型定义
#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QLabel>
#include <memory>

// 使用 AlertManager.h 中已定义的类型
// AlertType, AlertRule 已在 AlertManager.h 中定义

/**
 * @brief 预警记录数据结构（用于历史显示）
 */
struct AlertRecord {
    QString symbol;             ///< 标的代码
    QString name;               ///< 标的名称
    AlertType type;             ///< 预警类型
    double threshold = 0.0;     ///< 阈值
    double actualValue = 0.0;   ///< 实际值
    QDateTime triggerTime;      ///< 触发时间
    QString message;            ///< 预警消息
};

/**
 * @brief 预警中心页面类
 */
class AlertCenterPage : public BasePage
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口
     */
    explicit AlertCenterPage(QWidget *parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~AlertCenterPage() override;

    QString pageId() const override { return QStringLiteral("AlertCenter"); }
    QString pageName() const override { return QStringLiteral("预警中心"); }

    void initializePage() override;
    void refresh();

signals:
    /**
     * @brief 预警选中信号
     */
    void alertSelected(const QString& ruleId);

private slots:
    /**
     * @brief 添加预警
     */
    void onAddAlert();

    /**
     * @brief 删除预警
     */
    void onRemoveAlert();

    /**
     * @brief 启用/禁用预警
     */
    void onToggleAlert();

    /**
     * @brief 预警表格点击
     */
    void onAlertClicked(int row, int column);

private:
    /**
     * @brief 初始化UI
     */
    void setupUI();

    /**
     * @brief 加载预警规则列表
     */
    void loadAlertRules();

    /**
     * @brief 加载预警历史记录
     */
    void loadAlertHistory();

    /**
     * @brief 更新预警表格
     */
    void updateAlertTable();

    /**
     * @brief 更新历史表格
     */
    void updateHistoryTable();

    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // ALERTCENTERPAGE_H
