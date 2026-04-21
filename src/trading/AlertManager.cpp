/**
 * @file AlertManager.cpp
 * @brief 预警管理器实现
 */

#include "AlertManager.h"
#include "TradingService.h"
#include "PositionManager.h"
#include "utils/Logger.h"

#include <QMutexLocker>
#include <QSettings>
#include <QSoundEffect>
#include <QSystemTrayIcon>
#include <QApplication>

struct AlertManager::Impl {
    QHash<QString, AlertRule> alerts;
    QHash<QString, QPair<double, double>> priceCache;  // instrumentId -> (lastPrice, changePercent)
    QTimer* checkTimer = nullptr;
    QSoundEffect* soundEffect = nullptr;
    QSystemTrayIcon* trayIcon = nullptr;
    mutable QMutex mutex;
    bool initialized = false;
    bool soundEnabled = true;
    bool trayEnabled = true;
    QString alertSoundFile;
};

AlertManager& AlertManager::instance()
{
    static AlertManager instance;
    return instance;
}

AlertManager::AlertManager(QObject* parent)
    : QObject(parent)
    , d(std::make_unique<Impl>())
{
    d->checkTimer = new QTimer(this);
    connect(d->checkTimer, &QTimer::timeout, this, &AlertManager::onCheckTimer);

    // 初始化系统托盘
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        d->trayIcon = new QSystemTrayIcon(this);
        d->trayIcon->setIcon(QApplication::windowIcon().isNull()
            ? QIcon(":/icons/app.ico") : QApplication::windowIcon());
        d->trayIcon->show();
    }

    LOG_DEBUG("AlertManager created");
}

AlertManager::~AlertManager()
{
    shutdown();
    LOG_DEBUG("AlertManager destroyed");
}

bool AlertManager::initialize()
{
    QMutexLocker locker(&d->mutex);

    if (d->initialized) {
        return true;
    }

    loadAlerts();
    d->checkTimer->start(1000);  // 1秒检查一次

    d->initialized = true;
    LOG_INFO("AlertManager initialized");
    return true;
}

void AlertManager::shutdown()
{
    QMutexLocker locker(&d->mutex);

    if (!d->initialized) {
        return;
    }

    d->checkTimer->stop();
    saveAlerts();

    if (d->soundEffect) {
        d->soundEffect->stop();
    }

    d->alerts.clear();
    d->priceCache.clear();
    d->initialized = false;

    LOG_INFO("AlertManager shutdown");
}

QString AlertManager::addAlert(const AlertRule& rule)
{
    QMutexLocker locker(&d->mutex);

    QString id = rule.ruleId.isEmpty()
        ? QUuid::createUuid().toString(QUuid::WithoutBraces)
        : rule.ruleId;

    AlertRule newRule = rule;
    newRule.ruleId = id;
    newRule.createTime = QDateTime::currentDateTime();
    newRule.isActive = true;
    newRule.isTriggered = false;

    d->alerts[id] = newRule;
    saveAlerts();

    LOG_INFO(QString("Alert added: %1, type: %2, threshold: %3")
        .arg(id).arg(static_cast<int>(rule.type)).arg(rule.threshold));

    emit alertAdded(id);
    return id;
}

bool AlertManager::removeAlert(const QString& ruleId)
{
    QMutexLocker locker(&d->mutex);

    if (!d->alerts.contains(ruleId)) {
        return false;
    }

    d->alerts.remove(ruleId);
    saveAlerts();

    LOG_INFO(QString("Alert removed: %1").arg(ruleId));
    emit alertRemoved(ruleId);
    return true;
}

void AlertManager::setAlertActive(const QString& ruleId, bool active)
{
    QMutexLocker locker(&d->mutex);

    if (d->alerts.contains(ruleId)) {
        d->alerts[ruleId].isActive = active;
        saveAlerts();
    }
}

QVector<AlertRule> AlertManager::getAlerts() const
{
    QMutexLocker locker(&d->mutex);
    return d->alerts.values().toVector();
}

QVector<AlertRule> AlertManager::getAlerts(const QString& instrumentId) const
{
    QMutexLocker locker(&d->mutex);

    QVector<AlertRule> result;
    for (const auto& alert : d->alerts) {
        if (alert.instrumentId == instrumentId) {
            result.append(alert);
        }
    }
    return result;
}

void AlertManager::updatePrice(const QString& instrumentId, double lastPrice, double changePercent)
{
    QMutexLocker locker(&d->mutex);
    d->priceCache[instrumentId] = qMakePair(lastPrice, changePercent);
}

void AlertManager::setSoundEnabled(bool enabled)
{
    d->soundEnabled = enabled;
}

void AlertManager::setSystemTrayEnabled(bool enabled)
{
    d->trayEnabled = enabled;
}

void AlertManager::setAlertSound(const QString& soundFile)
{
    d->alertSoundFile = soundFile;
}

void AlertManager::onCheckTimer()
{
    QMutexLocker locker(&d->mutex);

    for (auto& alert : d->alerts) {
        if (!alert.isActive || alert.isTriggered) {
            continue;
        }

        if (!d->priceCache.contains(alert.instrumentId)) {
            continue;
        }

        auto [price, changePercent] = d->priceCache[alert.instrumentId];
        checkAlert(alert, price, changePercent);
    }
}

void AlertManager::checkAlert(const AlertRule& rule, double lastPrice, double changePercent)
{
    bool triggered = false;

    switch (rule.type) {
    case AlertType::PriceAbove:
        triggered = (lastPrice >= rule.threshold);
        break;
    case AlertType::PriceBelow:
        triggered = (lastPrice <= rule.threshold);
        break;
    case AlertType::ChangePercentAbove:
        triggered = (changePercent >= rule.threshold);
        break;
    case AlertType::ChangePercentBelow:
        triggered = (changePercent <= -rule.threshold);
        break;
    case AlertType::ProfitAbove: {
        double profit = PositionManager::instance().getInstrumentProfit(rule.instrumentId);
        triggered = (profit >= rule.threshold);
        break;
    }
    case AlertType::LossAbove: {
        double profit = PositionManager::instance().getInstrumentProfit(rule.instrumentId);
        triggered = (profit <= -rule.threshold);
        break;
    }
    default:
        break;
    }

    if (triggered) {
        AlertRule& r = d->alerts[rule.ruleId];
        triggerAlert(r);
    }
}

void AlertManager::triggerAlert(AlertRule& rule)
{
    rule.isTriggered = true;
    rule.triggerTime = QDateTime::currentDateTime();

    // 生成预警消息
    QString title = "预警通知";
    QString message;

    switch (rule.type) {
    case AlertType::PriceAbove:
        message = QString("%1 价格已突破 %2").arg(rule.instrumentId).arg(rule.threshold);
        break;
    case AlertType::PriceBelow:
        message = QString("%1 价格已跌破 %2").arg(rule.instrumentId).arg(rule.threshold);
        break;
    case AlertType::ChangePercentAbove:
        message = QString("%1 涨幅已超过 %2%").arg(rule.instrumentId).arg(rule.threshold);
        break;
    case AlertType::ChangePercentBelow:
        message = QString("%1 跌幅已超过 %2%").arg(rule.instrumentId).arg(rule.threshold);
        break;
    case AlertType::ProfitAbove:
        message = QString("%1 盈利已超过 %2").arg(rule.instrumentId).arg(rule.threshold);
        break;
    case AlertType::LossAbove:
        message = QString("%1 亏损已超过 %2").arg(rule.instrumentId).arg(rule.threshold);
        break;
    default:
        message = QString("%1 触发预警").arg(rule.instrumentId);
    }

    rule.message = message;

    LOG_INFO(QString("Alert triggered: %1 - %2").arg(rule.ruleId, message));

    // 播放声音
    if (d->soundEnabled) {
        playAlertSound();
    }

    // 显示系统托盘通知
    if (d->trayEnabled && d->trayIcon) {
        showSystemTrayNotification(title, message);
    }

    saveAlerts();
    emit alertTriggered(rule);
}

void AlertManager::playAlertSound()
{
    if (d->alertSoundFile.isEmpty()) {
        return;
    }

    if (!d->soundEffect) {
        d->soundEffect = new QSoundEffect(this);
    }

    d->soundEffect->setSource(QUrl::fromLocalFile(d->alertSoundFile));
    d->soundEffect->play();
}

void AlertManager::showSystemTrayNotification(const QString& title, const QString& message)
{
    if (d->trayIcon) {
        d->trayIcon->showMessage(title, message, QSystemTrayIcon::Warning, 5000);
    }
}

void AlertManager::saveAlerts()
{
    QSettings settings("WealthPilot", "Alerts");
    settings.clear();

    int index = 0;
    for (const auto& alert : d->alerts) {
        QString key = QString("alert_%1/").arg(index++);
        settings.setValue(key + "id", alert.ruleId);
        settings.setValue(key + "instrumentId", alert.instrumentId);
        settings.setValue(key + "type", static_cast<int>(alert.type));
        settings.setValue(key + "threshold", alert.threshold);
        settings.setValue(key + "isActive", alert.isActive);
        settings.setValue(key + "isTriggered", alert.isTriggered);
    }

    settings.setValue("count", index);
    settings.sync();
}

void AlertManager::loadAlerts()
{
    QSettings settings("WealthPilot", "Alerts");
    int count = settings.value("count", 0).toInt();

    for (int i = 0; i < count; ++i) {
        QString key = QString("alert_%1/").arg(i);
        AlertRule alert;
        alert.ruleId = settings.value(key + "id").toString();
        alert.instrumentId = settings.value(key + "instrumentId").toString();
        alert.type = static_cast<AlertType>(settings.value(key + "type").toInt());
        alert.threshold = settings.value(key + "threshold").toDouble();
        alert.isActive = settings.value(key + "isActive", true).toBool();
        alert.isTriggered = settings.value(key + "isTriggered", false).toBool();

        if (!alert.ruleId.isEmpty() && !alert.instrumentId.isEmpty()) {
            d->alerts[alert.ruleId] = alert;
        }
    }

    LOG_DEBUG(QString("Loaded %1 alerts").arg(d->alerts.size()));
}
