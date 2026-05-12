/**
 * @file SmartAlertService.h
 * @brief 智能预警服务
 *
 * @details 功能：
 * - 异常检测（价格暴涨暴跌、成交量异常）
 * - 机会识别（突破、金叉、支撑测试）
 * - AI 预警建议
 * - 预警推送管理
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef SMARTALERTSERVICE_H
#define SMARTALERTSERVICE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QMap>
#include <QList>
#include <QDateTime>
#include <QTimer>
#include <functional>

namespace WealthPilot
{
    /**
 * @brief 预警类型枚举
 */
    enum class AlertType
    {
        // 价格异常
        PriceSurge, ///< 价格暴涨（涨幅 > 5%）
        PricePlunge, ///< 价格暴跌（跌幅 > 5%）
        PriceHigh, ///< 创新高
        PriceLow, ///< 创新低

        // 成交量异常
        VolumeSurge, ///< 成交量放大（> 5倍均量）
        VolumeShrink, ///< 成交量萎缩

        // 技术信号
        Breakout, ///< 突破
        Breakdown, ///< 跌破
        GoldenCross, ///< 金叉
        DeathCross, ///< 死叉
        SupportTest, ///< 支撑测试
        ResistanceTest, ///< 阻力测试

        // AI 推荐
        AIRecommendation, ///< AI 推荐
        AIRiskWarning, ///< AI 风险警告

        // 自定义
        Custom ///< 自定义预警
    };

    /**
 * @brief 预警级别
 */
    enum class AlertLevel
    {
        Info = 0, ///< 信息
        Warning = 1, ///< 警告
        Critical = 2, ///< 严重
        Emergency = 3 ///< 紧急
    };

    /**
 * @brief 预警规则
 */
    struct AlertRule
    {
        QString id; ///< 规则 ID
        QString name; ///< 规则名称
        AlertType type; ///< 预警类型
        AlertLevel level; ///< 预警级别
        QString stockCode; ///< 股票代码（空表示全部）
        double threshold = 0; ///< 阈值
        bool enabled = true; ///< 是否启用
        QDateTime createdAt; ///< 创建时间

        static AlertRule fromJson(const QJsonObject& json);
        QJsonObject toJson() const;
    };

    /**
 * @brief 预警信息
 */
    struct AlertInfo
    {
        QString id; ///< 预警 ID
        AlertType type; ///< 预警类型
        AlertLevel level; ///< 预警级别
        QString stockCode; ///< 股票代码
        QString stockName; ///< 股票名称
        QString title; ///< 预警标题
        QString message; ///< 预警消息
        QString suggestion; ///< 操作建议
        double value = 0; ///< 触发值
        double threshold = 0; ///< 阈值
        QDateTime triggeredAt; ///< 触发时间
        bool isRead = false; ///< 是否已读
        bool isHandled = false; ///< 是否已处理

        static AlertInfo fromJson(const QJsonObject& json);
        QJsonObject toJson() const;
    };

    /**
 * @brief 智能预警服务
 */
    class SmartAlertService : public QObject
    {
        Q_OBJECT

    public:
        explicit SmartAlertService(QObject* parent = nullptr);
        ~SmartAlertService() override;

        // ========== 规则管理 ==========

        /**
     * @brief 添加预警规则
     */
        QString addRule(const AlertRule& rule);

        /**
     * @brief 删除预警规则
     */
        bool removeRule(const QString& ruleId);

        /**
     * @brief 获取所有规则
     */
        QList<AlertRule> getRules() const;

        /**
     * @brief 启用/禁用规则
     */
        void setRuleEnabled(const QString& ruleId, bool enabled);

        // ========== 预警检测 ==========

        /**
     * @brief 检测股票异常
     */
        QList<AlertInfo> detectAnomalies(const QString& stockCode);

        /**
     * @brief 检测技术信号
     */
        QList<AlertInfo> detectSignals(const QString& stockCode);

        /**
     * @brief AI 预警分析
     */
        void analyzeWithAI(const QString& stockCode,
                           std::function<void(const QList<AlertInfo> &)> callback);

        // ========== 预警管理 ==========

        /**
     * @brief 获取未读预警
     */
        QList<AlertInfo> getUnreadAlerts() const;

        /**
     * @brief 获取所有预警
     */
        QList<AlertInfo> getAllAlerts() const;

        /**
     * @brief 标记预警已读
     */
        void markAsRead(const QString& alertId);

        /**
     * @brief 清除所有预警
     */
        void clearAlerts();

        // ========== 监控控制 ==========

        /**
     * @brief 开始监控
     */
        void startMonitoring(int intervalMs = 60000);

        /**
     * @brief 停止监控
     */
        void stopMonitoring();

        /**
     * @brief 添加监控股票
     */
        void addWatchStock(const QString& stockCode);

        /**
     * @brief 移除监控股票
     */
        void removeWatchStock(const QString& stockCode);

        // ========== 工具方法 ==========

        static QString getAlertTypeName(AlertType type);
        static QString getAlertLevelName(AlertLevel level);

        signals :

        void alertTriggered(const AlertInfo& alert);
        void alertsUpdated();
        void errorOccurred(const QString& error);

    private
        slots :

        void onMonitorTimer();

    private:
        void initializeDefaultRules();
        QList<AlertInfo> checkStock(const QString& stockCode);
        void saveAlert(const AlertInfo& alert);
        QString generateAlertId() const;

    private:
        QMap<QString, AlertRule> m_rules;
        QList<AlertInfo> m_alerts;
        QStringList m_watchStocks;
        QTimer* m_monitorTimer;
        QString m_storagePath;
        bool m_monitoring = false;
    };
} // namespace WealthPilot

#endif // SMARTALERTSERVICE_H