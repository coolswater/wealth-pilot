/**
 * @file SignalService.cpp
 * @brief 实时行情信号服务实现
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "SignalService.h"
#include <QDebug>
#include <QTimer>

namespace WealthPilot {
namespace Analysis {

struct SignalService::Impl {
    QVector<IAnalyzer*> analyzers;
    SignalFilter* filter = nullptr;

    QMap<QString, QVector<UnifiedSignal>> symbolSignalMap;
    QMap<QString, CompositeSignal> compositeSignalMap;
    QVector<SignalSubscription> subscriptionList;

    AlertConfig alertConfig;
    QTimer* updateTimer = nullptr;

    int updateIntervalMs = 60000; // 1分钟更新间隔
};

SignalService::SignalService(QObject* parent)
    : QObject(parent)
    , d(std::make_unique<Impl>())
{
    d->updateTimer = new QTimer(this);
    connect(d->updateTimer, &QTimer::timeout, this, &SignalService::onTimerUpdate);
}

SignalService::~SignalService()
{
    if (d->updateTimer) {
        d->updateTimer->stop();
    }
}

void SignalService::initialize()
{
    // 启动定时更新
    d->updateTimer->start(d->updateIntervalMs);

    qDebug() << "SignalService initialized";
}

void SignalService::registerAnalyzer(IAnalyzer* analyzer)
{
    if (!analyzer) return;

    d->analyzers.append(analyzer);

    // 连接分析器信号
    connect(analyzer, &IAnalyzer::signalDetected,
            this, &SignalService::onAnalyzerSignal);

    qDebug() << "Analyzer registered:" << analyzer->name();
}

void SignalService::setSignalFilter(SignalFilter* filter)
{
    d->filter = filter;
}

void SignalService::analyzeSymbol(const QString& symbol, const QVector<KLine>& klines)
{
    if (klines.isEmpty()) return;

    QVector<UnifiedSignal> allSignalList;

    // 使用所有注册的分析器进行分析
    for (auto* analyzer : d->analyzers) {
        auto result = analyzer->analyze(klines);
        if (result.isValid) {
            allSignalList.append(result.generatedSignals);
        }
    }

    // 保存信号
    d->symbolSignalMap[symbol] = allSignalList;

    // 使用信号过滤器生成综合信号
    if (d->filter && !allSignalList.isEmpty()) {
        auto composite = d->filter->filter(allSignalList);
        composite.symbol = symbol;
        d->compositeSignalMap[symbol] = composite;

        // 发送综合信号
        emit compositeSignalGenerated(composite);

        // 检查预警
        if (shouldAlert(composite)) {
            sendAlert(symbol, composite);
        }
    }

    // 发送信号更新
    emit signalsUpdated(symbol, allSignalList);
    emit analysisCompleted(symbol);
}

void SignalService::analyzeBatch(const QMap<QString, QVector<KLine>>& data)
{
    for (auto it = data.begin(); it != data.end(); ++it) {
        analyzeSymbol(it.key(), it.value());
    }
}

QVector<UnifiedSignal> SignalService::getSignals(const QString& symbol) const
{
    return d->symbolSignalMap.value(symbol);
}

QVector<CompositeSignal> SignalService::getCompositeSignals() const
{
    return d->compositeSignalMap.values();
}

void SignalService::addSubscription(const SignalSubscription& subscription)
{
    d->subscriptionList.append(subscription);
}

void SignalService::removeSubscription(const QString& symbol)
{
    for (int i = d->subscriptionList.size() - 1; i >= 0; --i) {
        if (d->subscriptionList[i].symbol == symbol) {
            d->subscriptionList.removeAt(i);
        }
    }
}

QVector<SignalSubscription> SignalService::subscriptions() const
{
    return d->subscriptionList;
}

void SignalService::setAlertConfig(const AlertConfig& config)
{
    d->alertConfig = config;
}

const AlertConfig& SignalService::alertConfig() const
{
    return d->alertConfig;
}

QMap<QString, QVariant> SignalService::getIndicatorData(const QString& symbol) const
{
    QMap<QString, QVariant> data;

    auto signalList = d->symbolSignalMap.value(symbol);
    auto composite = d->compositeSignalMap.value(symbol);

    // 添加各理论指标数据
    for (const auto& signal : signalList) {
        QString key = signal.theoryName() + "_signal";
        data[key] = QVariant::fromValue(signal);
    }

    // 添加综合信号
    data["composite_signal"] = QVariant::fromValue(composite);

    return data;
}

QMap<QString, QVariant> SignalService::getOverlayConfig(const QString& symbol) const
{
    QMap<QString, QVariant> config;

    auto composite = d->compositeSignalMap.value(symbol);

    // 配置K线图叠加显示
    config["show_signals"] = true;
    config["signal_direction"] = static_cast<int>(composite.direction);
    config["signal_confidence"] = composite.confidence;
    config["theory_count"] = composite.theoryCount;

    // 配置信号标记样式
    if (composite.direction == Analysis::SignalDirection::Bullish) {
        config["marker_color"] = "#00AA00"; // 绿色
        config["marker_shape"] = "arrow_up";
    } else if (composite.direction == Analysis::SignalDirection::Bearish) {
        config["marker_color"] = "#AA0000"; // 红色
        config["marker_shape"] = "arrow_down";
    } else {
        config["marker_color"] = "#AAAAAA"; // 灰色
        config["marker_shape"] = "circle";
    }

    return config;
}

void SignalService::onAnalyzerSignal(const UnifiedSignal& signal)
{
    // 处理单个分析器产生的信号
    qDebug() << "Signal from analyzer:" << signal.theoryName()
             << "Direction:" << signal.directionText()
             << "Confidence:" << signal.confidence;
}

void SignalService::onTimerUpdate()
{
    // 定期更新信号（如果有实时数据源）
    // 这里可以触发重新分析或数据更新
}

bool SignalService::shouldAlert(const CompositeSignal& signal)
{
    if (!d->alertConfig.enabled) return false;

    // 检查置信度
    if (signal.confidence < d->alertConfig.minConfidence) return false;

    // 检查理论数量
    if (signal.theoryCount < d->alertConfig.minTheoryCount) return false;

    // 检查是否为强信号
    if (d->alertConfig.alertOnStrongSignals && signal.isStrongSignal()) {
        return true;
    }

    return false;
}

void SignalService::sendAlert(const QString& symbol, const CompositeSignal& signal)
{
    qDebug() << "Alert triggered for" << symbol
             << "Direction:" << static_cast<int>(signal.direction)
             << "Confidence:" << signal.confidence;

    // 发送预警信号
    emit alertTriggered(symbol, signal);

    // 通知订阅者
    notifySubscribers(symbol, signal);
}

void SignalService::notifySubscribers(const QString& symbol, const CompositeSignal& signal)
{
    for (const auto& subscription : d->subscriptionList) {
        if (subscription.symbol == symbol || subscription.symbol == "ALL") {
            // 检查订阅条件
            if (signal.confidence >= subscription.minConfidence) {
                // 发送通知（实际应用中应该发送到指定渠道）
                qDebug() << "Notifying subscriber:" << subscription.symbol
                         << "Channel:" << subscription.alertChannel;
            }
        }
    }
}

} // namespace Analysis
} // namespace WealthPilot

