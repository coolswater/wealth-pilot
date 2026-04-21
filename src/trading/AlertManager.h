/**
 * @file AlertManager.h
 * @brief 预警管理器 - 价格预警和风控预警
 *
 * @details 功能：
 * - 价格预警（涨跌幅、价格突破）
 * - 持仓预警（盈亏预警）
 * - 风控预警（保证金、回撤）
 * - 通知推送（系统托盘、声音）
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef ALERTMANAGER_H
#define ALERTMANAGER_H

#include <QObject>
#include <QTimer>
#include <QHash>
#include <QMutex>
#include <memory>
#include <QDateTime>

/**
 * @brief 预警类型
 */
enum class AlertType {
    PriceAbove,         ///< 价格高于
    PriceBelow,         ///< 价格低于
    ChangePercentAbove, ///< 涨幅超过
    ChangePercentBelow, ///< 跌幅超过
    ProfitAbove,        ///< 盈利超过
    LossAbove,          ///< 亏损超过
    MarginWarning,      ///< 保证金预警
    DrawdownWarning     ///< 回撤预警
};

/**
 * @brief 预警规则
 */
struct AlertRule {
    QString ruleId;
    QString instrumentId;
    AlertType type;
    double threshold;
    bool isActive = true;
    bool isTriggered = false;
    QDateTime createTime;
    QDateTime triggerTime;
    QString message;
};

/**
 * @brief 预警管理器
 */
class AlertManager : public QObject
{
    Q_OBJECT

public:
    static AlertManager& instance();

    bool initialize();
    void shutdown();

    // ========== 预警规则管理 ==========

    QString addAlert(const AlertRule& rule);
    bool removeAlert(const QString& ruleId);
    void setAlertActive(const QString& ruleId, bool active);
    QVector<AlertRule> getAlerts() const;
    QVector<AlertRule> getAlerts(const QString& instrumentId) const;

    // ========== 价格更新 ==========

    void updatePrice(const QString& instrumentId, double lastPrice, double changePercent);

    // ========== 通知设置 ==========

    void setSoundEnabled(bool enabled);
    void setSystemTrayEnabled(bool enabled);
    void setAlertSound(const QString& soundFile);

signals:
    void alertTriggered(const AlertRule& rule);
    void alertAdded(const QString& ruleId);
    void alertRemoved(const QString& ruleId);

private slots:
    void onCheckTimer();

private:
    AlertManager(QObject* parent = nullptr);
    ~AlertManager() override;
    Q_DISABLE_COPY(AlertManager)

    void checkAlert(const AlertRule& rule, double lastPrice, double changePercent);
    void triggerAlert(AlertRule& rule);
    void playAlertSound();
    void showSystemTrayNotification(const QString& title, const QString& message);
    void saveAlerts();
    void loadAlerts();

    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // ALERTMANAGER_H
