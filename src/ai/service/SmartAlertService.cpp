/**
 * @file SmartAlertService.cpp
 * @brief 智能预警服务实现
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "SmartAlertService.h"
#include "AIService.h"
#include "utils/Logger.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QUuid>
#include <QRandomGenerator>

namespace WealthPilot
{
    // ============================================================================
    // 数据结构实现
    // ============================================================================

    AlertRule AlertRule::fromJson(const QJsonObject& json)
    {
        AlertRule rule;
        rule.id = json["id"].toString();
        rule.name = json["name"].toString();
        rule.type = static_cast<AlertType>(json["type"].toInt());
        rule.level = static_cast<AlertLevel>(json["level"].toInt());
        rule.stockCode = json["stockCode"].toString();
        rule.threshold = json["threshold"].toDouble();
        rule.enabled = json["enabled"].toBool(true);
        rule.createdAt = QDateTime::fromString(json["createdAt"].toString(), Qt::ISODate);
        return rule;
    }

    QJsonObject AlertRule::toJson() const
    {
        QJsonObject json;
        json["id"] = id;
        json["name"] = name;
        json["type"] = static_cast<int>(type);
        json["level"] = static_cast<int>(level);
        json["stockCode"] = stockCode;
        json["threshold"] = threshold;
        json["enabled"] = enabled;
        json["createdAt"] = createdAt.toString(Qt::ISODate);
        return json;
    }

    AlertInfo AlertInfo::fromJson(const QJsonObject& json)
    {
        AlertInfo alert;
        alert.id = json["id"].toString();
        alert.type = static_cast<AlertType>(json["type"].toInt());
        alert.level = static_cast<AlertLevel>(json["level"].toInt());
        alert.stockCode = json["stockCode"].toString();
        alert.stockName = json["stockName"].toString();
        alert.title = json["title"].toString();
        alert.message = json["message"].toString();
        alert.suggestion = json["suggestion"].toString();
        alert.value = json["value"].toDouble();
        alert.threshold = json["threshold"].toDouble();
        alert.triggeredAt = QDateTime::fromString(json["triggeredAt"].toString(), Qt::ISODate);
        alert.isRead = json["isRead"].toBool();
        alert.isHandled = json["isHandled"].toBool();
        return alert;
    }

    QJsonObject AlertInfo::toJson() const
    {
        QJsonObject json;
        json["id"] = id;
        json["type"] = static_cast<int>(type);
        json["level"] = static_cast<int>(level);
        json["stockCode"] = stockCode;
        json["stockName"] = stockName;
        json["title"] = title;
        json["message"] = message;
        json["suggestion"] = suggestion;
        json["value"] = value;
        json["threshold"] = threshold;
        json["triggeredAt"] = triggeredAt.toString(Qt::ISODate);
        json["isRead"] = isRead;
        json["isHandled"] = isHandled;
        return json;
    }

    // ============================================================================
    // SmartAlertService 实现
    // ============================================================================

    SmartAlertService::SmartAlertService(QObject* parent)
        : QObject(parent)
          , m_monitorTimer(new QTimer(this))
    {
        QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        m_storagePath = appDataPath + "/smart_alerts";
        QDir dir(m_storagePath);
        if (!dir.exists())
        {
            dir.mkpath(".");
        }

        initializeDefaultRules();

        connect(m_monitorTimer, &QTimer::timeout, this, &SmartAlertService::onMonitorTimer);

        LOG_DEBUG("SmartAlertService created");
    }

    SmartAlertService::~SmartAlertService()
    {
        stopMonitoring();
        LOG_DEBUG("SmartAlertService destroyed");
    }

    QString SmartAlertService::addRule(const AlertRule& rule)
    {
        QString id = rule.id.isEmpty() ? generateAlertId() : rule.id;
        AlertRule newRule = rule;
        newRule.id = id;
        newRule.createdAt = QDateTime::currentDateTime();

        m_rules[id] = newRule;
        LOG_INFO("Added alert rule: " + newRule.name);
        return id;
    }

    bool SmartAlertService::removeRule(const QString& ruleId)
    {
        if (m_rules.remove(ruleId) > 0)
        {
            LOG_INFO("Removed alert rule: " + ruleId);
            return true;
        }
        return false;
    }

    QList<AlertRule> SmartAlertService::getRules() const
    {
        return m_rules.values();
    }

    void SmartAlertService::setRuleEnabled(const QString& ruleId, bool enabled)
    {
        if (m_rules.contains(ruleId))
        {
            m_rules[ruleId].enabled = enabled;
        }
    }

    QList<AlertInfo> SmartAlertService::detectAnomalies(const QString& stockCode)
    {
        QList<AlertInfo> alerts;

        // TODO: 从数据源获取实时数据
        // 这里使用模拟检测

        // 模拟价格暴涨检测
        double changePercent = QRandomGenerator::global()->bounded(-10, 10);

        if (changePercent > 5)
        {
            AlertInfo alert;
            alert.id = generateAlertId();
            alert.type = AlertType::PriceSurge;
            alert.level = AlertLevel::Warning;
            alert.stockCode = stockCode;
            alert.stockName = stockCode;
            alert.title = QStringLiteral("价格暴涨预警");
            alert.message = QString(QStringLiteral("股票 %1 涨幅达到 %2%")).arg(stockCode).arg(changePercent, 0, 'f', 2);
            alert.suggestion = QStringLiteral("注意追高风险，建议设置止盈");
            alert.value = changePercent;
            alert.threshold = 5;
            alert.triggeredAt = QDateTime::currentDateTime();
            alerts.append(alert);
        }

        if (changePercent < -5)
        {
            AlertInfo alert;
            alert.id = generateAlertId();
            alert.type = AlertType::PricePlunge;
            alert.level = AlertLevel::Warning;
            alert.stockCode = stockCode;
            alert.stockName = stockCode;
            alert.title = QStringLiteral("价格暴跌预警");
            alert.message = QString(QStringLiteral("股票 %1 跌幅达到 %2%")).arg(stockCode).arg(-changePercent, 0, 'f', 2);
            alert.suggestion = QStringLiteral("注意止损，评估是否需要减仓");
            alert.value = changePercent;
            alert.threshold = -5;
            alert.triggeredAt = QDateTime::currentDateTime();
            alerts.append(alert);
        }

        // 模拟成交量放大检测
        double volumeRatio = QRandomGenerator::global()->bounded(1, 10);
        if (volumeRatio > 5)
        {
            AlertInfo alert;
            alert.id = generateAlertId();
            alert.type = AlertType::VolumeSurge;
            alert.level = AlertLevel::Info;
            alert.stockCode = stockCode;
            alert.stockName = stockCode;
            alert.title = QStringLiteral("成交量放大预警");
            alert.message = QString(QStringLiteral("股票 %1 成交量放大 %2 倍")).arg(stockCode).arg(volumeRatio, 0, 'f', 1);
            alert.suggestion = QStringLiteral("关注资金动向，可能有重大消息");
            alert.value = volumeRatio;
            alert.threshold = 5;
            alert.triggeredAt = QDateTime::currentDateTime();
            alerts.append(alert);
        }

        return alerts;
    }

    QList<AlertInfo> SmartAlertService::detectSignals(const QString& stockCode)
    {
        QList<AlertInfo> alerts;

        // TODO: 实现技术信号检测
        // 这里使用模拟检测

        bool hasGoldenCross = QRandomGenerator::global()->bounded(10) == 0;
        if (hasGoldenCross)
        {
            AlertInfo alert;
            alert.id = generateAlertId();
            alert.type = AlertType::GoldenCross;
            alert.level = AlertLevel::Info;
            alert.stockCode = stockCode;
            alert.stockName = stockCode;
            alert.title = QStringLiteral("金叉信号");
            alert.message = QString(QStringLiteral("股票 %1 出现金叉信号")).arg(stockCode);
            alert.suggestion = QStringLiteral("金叉为买入信号，但需结合其他指标确认");
            alert.triggeredAt = QDateTime::currentDateTime();
            alerts.append(alert);
        }

        bool hasBreakout = QRandomGenerator::global()->bounded(15) == 0;
        if (hasBreakout)
        {
            AlertInfo alert;
            alert.id = generateAlertId();
            alert.type = AlertType::Breakout;
            alert.level = AlertLevel::Warning;
            alert.stockCode = stockCode;
            alert.stockName = stockCode;
            alert.title = QStringLiteral("突破信号");
            alert.message = QString(QStringLiteral("股票 %1 突破阻力位")).arg(stockCode);
            alert.suggestion = QStringLiteral("突破确认后可考虑跟进，设置止损");
            alert.triggeredAt = QDateTime::currentDateTime();
            alerts.append(alert);
        }

        return alerts;
    }

    void SmartAlertService::analyzeWithAI(const QString& stockCode,
                                          std::function<void(const QList<AlertInfo> &)> callback)
    {
        QString prompt = QString(QStringLiteral(
            "请分析股票 %1 的风险和机会：\n\n"
            "请从以下角度分析：\n"
            "1. 技术面风险和机会\n"
            "2. 基本面风险和机会\n"
            "3. 市场情绪风险和机会\n\n"
            "请给出具体的预警建议。"
        )).arg(stockCode);

        AIService::instance()->chat(prompt, [this, stockCode, callback](Result<QString> result)
        {
            if (result.isError())
            {
                emit errorOccurred(result.errorMessage());
                callback(QList<AlertInfo>());
                return;
            }

            QList<AlertInfo> alerts;

            AlertInfo alert;
            alert.id = generateAlertId();
            alert.type = AlertType::AIRecommendation;
            alert.level = AlertLevel::Info;
            alert.stockCode = stockCode;
            alert.stockName = stockCode;
            alert.title = QStringLiteral("AI 分析预警");
            alert.message = result.value();
            alert.suggestion = QStringLiteral("请结合自身判断做出决策");
            alert.triggeredAt = QDateTime::currentDateTime();
            alerts.append(alert);

            callback(alerts);
        });
    }

    QList<AlertInfo> SmartAlertService::getUnreadAlerts() const
    {
        QList<AlertInfo> unread;
        for (const auto& alert : m_alerts)
        {
            if (!alert.isRead)
            {
                unread.append(alert);
            }
        }
        return unread;
    }

    QList<AlertInfo> SmartAlertService::getAllAlerts() const
    {
        return m_alerts;
    }

    void SmartAlertService::markAsRead(const QString& alertId)
    {
        for (auto& alert : m_alerts)
        {
            if (alert.id == alertId)
            {
                alert.isRead = true;
                emit alertsUpdated();
                break;
            }
        }
    }

    void SmartAlertService::clearAlerts()
    {
        m_alerts.clear();
        emit alertsUpdated();
    }

    void SmartAlertService::startMonitoring(int intervalMs)
    {
        if (m_monitoring) return;

        m_monitoring = true;
        m_monitorTimer->start(intervalMs);
        LOG_INFO("Started alert monitoring, interval: " + QString::number(intervalMs) + "ms");
    }

    void SmartAlertService::stopMonitoring()
    {
        if (!m_monitoring) return;

        m_monitoring = false;
        m_monitorTimer->stop();
        LOG_INFO("Stopped alert monitoring");
    }

    void SmartAlertService::addWatchStock(const QString& stockCode)
    {
        if (!m_watchStocks.contains(stockCode))
        {
            m_watchStocks.append(stockCode);
            LOG_DEBUG("Added watch stock: " + stockCode);
        }
    }

    void SmartAlertService::removeWatchStock(const QString& stockCode)
    {
        m_watchStocks.removeAll(stockCode);
        LOG_DEBUG("Removed watch stock: " + stockCode);
    }

    QString SmartAlertService::getAlertTypeName(AlertType type)
    {
        switch (type)
        {
        case AlertType::PriceSurge: return QStringLiteral("价格暴涨");
        case AlertType::PricePlunge: return QStringLiteral("价格暴跌");
        case AlertType::PriceHigh: return QStringLiteral("创新高");
        case AlertType::PriceLow: return QStringLiteral("创新低");
        case AlertType::VolumeSurge: return QStringLiteral("成交量放大");
        case AlertType::VolumeShrink: return QStringLiteral("成交量萎缩");
        case AlertType::Breakout: return QStringLiteral("突破");
        case AlertType::Breakdown: return QStringLiteral("跌破");
        case AlertType::GoldenCross: return QStringLiteral("金叉");
        case AlertType::DeathCross: return QStringLiteral("死叉");
        case AlertType::SupportTest: return QStringLiteral("支撑测试");
        case AlertType::ResistanceTest: return QStringLiteral("阻力测试");
        case AlertType::AIRecommendation: return QStringLiteral("AI推荐");
        case AlertType::AIRiskWarning: return QStringLiteral("AI风险警告");
        case AlertType::Custom: return QStringLiteral("自定义");
        default: return QStringLiteral("未知");
        }
    }

    QString SmartAlertService::getAlertLevelName(AlertLevel level)
    {
        switch (level)
        {
        case AlertLevel::Info: return QStringLiteral("信息");
        case AlertLevel::Warning: return QStringLiteral("警告");
        case AlertLevel::Critical: return QStringLiteral("严重");
        case AlertLevel::Emergency: return QStringLiteral("紧急");
        default: return QStringLiteral("未知");
        }
    }

    void SmartAlertService::onMonitorTimer()
    {
        for (const auto& stockCode : m_watchStocks)
        {
            QList<AlertInfo> alerts = checkStock(stockCode);
            for (const auto& alert : alerts)
            {
                m_alerts.append(alert);
                saveAlert(alert);
                emit alertTriggered(alert);
            }
        }

        if (!m_alerts.isEmpty())
        {
            emit alertsUpdated();
        }
    }

    void SmartAlertService::initializeDefaultRules()
    {
        // 价格暴涨预警
        AlertRule priceSurgeRule;
        priceSurgeRule.id = "default_price_surge";
        priceSurgeRule.name = QStringLiteral("价格暴涨预警");
        priceSurgeRule.type = AlertType::PriceSurge;
        priceSurgeRule.level = AlertLevel::Warning;
        priceSurgeRule.threshold = 5;
        m_rules[priceSurgeRule.id] = priceSurgeRule;

        // 价格暴跌预警
        AlertRule pricePlungeRule;
        pricePlungeRule.id = "default_price_plunge";
        pricePlungeRule.name = QStringLiteral("价格暴跌预警");
        pricePlungeRule.type = AlertType::PricePlunge;
        pricePlungeRule.level = AlertLevel::Warning;
        pricePlungeRule.threshold = -5;
        m_rules[pricePlungeRule.id] = pricePlungeRule;

        // 成交量放大预警
        AlertRule volumeSurgeRule;
        volumeSurgeRule.id = "default_volume_surge";
        volumeSurgeRule.name = QStringLiteral("成交量放大预警");
        volumeSurgeRule.type = AlertType::VolumeSurge;
        volumeSurgeRule.level = AlertLevel::Info;
        volumeSurgeRule.threshold = 5;
        m_rules[volumeSurgeRule.id] = volumeSurgeRule;
    }

    QList<AlertInfo> SmartAlertService::checkStock(const QString& stockCode)
    {
        QList<AlertInfo> alerts;

        // 检测异常
        alerts.append(detectAnomalies(stockCode));

        // 检测技术信号
        alerts.append(detectSignals(stockCode));

        return alerts;
    }

    void SmartAlertService::saveAlert(const AlertInfo& alert)
    {
        QString filePath = m_storagePath + "/" + alert.id + ".json";

        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly))
        {
            QJsonDocument doc(alert.toJson());
            file.write(doc.toJson());
            file.close();
        }
    }

    QString SmartAlertService::generateAlertId() const
    {
        return QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
} // namespace WealthPilot