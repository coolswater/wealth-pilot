/**
 * @file SignalService.h
 * @brief 实时行情信号服务
 *
 * @details 提供实时行情数据的信号分析服务：
 * 1. 实时行情订阅
 * 2. 实时信号生成
 * 3. 信号预警推送
 * 4. K线图指标集成
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef SIGNAL_SERVICE_H
#define SIGNAL_SERVICE_H

#include "AnalysisTypes.h"
#include "SignalFilter.h"
#include "../IAnalyzer.h"
#include <QObject>
#include <QMap>
#include <QTimer>
#include <memory>

namespace WealthPilot {
namespace Analysis {

/**
 * @brief 信号订阅配置
 */
struct SignalSubscription {
    QString symbol;             ///< 标的代码
    TheoryType theory;          ///< 理论类型（可选，All表示所有）
    SignalDirection direction;  ///< 信号方向（可选）
    double minConfidence;       ///< 最小置信度
    bool enableAlert;           ///< 是否启用预警
    QString alertChannel;       ///< 预警渠道
};

/**
 * @brief 信号预警配置
 */
struct AlertConfig {
    bool enabled = true;
    double minConfidence = 70.0;
    int minTheoryCount = 3;
    bool alertOnStrongSignals = true;
    bool alertOnReversals = true;
    QString channel = "default";
};

/**
 * @brief 实时行情信号服务
 */
class SignalService : public QObject
{
    Q_OBJECT

public:
    explicit SignalService(QObject* parent = nullptr);
    ~SignalService() override;

    // ========== 初始化 ==========

    /**
     * @brief 初始化服务
     */
    void initialize();

    /**
     * @brief 注册分析器
     */
    void registerAnalyzer(IAnalyzer* analyzer);

    /**
     * @brief 设置信号过滤器
     */
    void setSignalFilter(SignalFilter* filter);

    // ========== 实时分析 ==========

    /**
     * @brief 分析指定标的
     */
    void analyzeSymbol(const QString& symbol, const QVector<KLine>& klines);

    /**
     * @brief 批量分析多个标的
     */
    void analyzeBatch(const QMap<QString, QVector<KLine>>& data);

    /**
     * @brief 获取指定标的的当前信号
     */
    QVector<UnifiedSignal> getSignals(const QString& symbol) const;

    /**
     * @brief 获取所有标的的综合信号
     */
    QVector<CompositeSignal> getCompositeSignals() const;

    // ========== 订阅管理 ==========

    /**
     * @brief 添加信号订阅
     */
    void addSubscription(const SignalSubscription& subscription);

    /**
     * @brief 移除信号订阅
     */
    void removeSubscription(const QString& symbol);

    /**
     * @brief 获取所有订阅
     */
    QVector<SignalSubscription> subscriptions() const;

    // ========== 预警配置 ==========

    /**
     * @brief 设置预警配置
     */
    void setAlertConfig(const AlertConfig& config);

    /**
     * @brief 获取预警配置
     */
    const AlertConfig& alertConfig() const;

    // ========== K线图指标集成 ==========

    /**
     * @brief 获取K线图指标数据
     */
    QMap<QString, QVariant> getIndicatorData(const QString& symbol) const;

    /**
     * @brief 获取指标叠加配置
     */
    QMap<QString, QVariant> getOverlayConfig(const QString& symbol) const;

signals:
    /**
     * @brief 信号更新
     */
    void signalsUpdated(const QString& symbol, const QVector<UnifiedSignal>& signals);

    /**
     * @brief 综合信号生成
     */
    void compositeSignalGenerated(const CompositeSignal& signal);

    /**
     * @brief 预警触发
     */
    void alertTriggered(const QString& symbol, const CompositeSignal& signal);

    /**
     * @brief 分析完成
     */
    void analysisCompleted(const QString& symbol);

private slots:
    /**
     * @brief 处理分析器信号
     */
    void onAnalyzerSignal(const UnifiedSignal& signal);

    /**
     * @brief 定期更新信号
     */
    void onTimerUpdate();

private:
    // ========== 内部方法 ==========

    /**
     * @brief 检查是否需要预警
     */
    bool shouldAlert(const CompositeSignal& signal);

    /**
     * @brief 发送预警
     */
    void sendAlert(const QString& symbol, const CompositeSignal& signal);

    /**
     * @brief 更新订阅者
     */
    void notifySubscribers(const QString& symbol, const CompositeSignal& signal);

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace Analysis
} // namespace WealthPilot

#endif // SIGNAL_SERVICE_H