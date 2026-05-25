/**
 * @file IAIPlugin.h
 * @brief AI插件接口 - 定义AI服务的插件接口
 *
 * @details 功能：
 * - 智能对话
 * - 市场分析
 * - 投顾建议
 * - 预测算法
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */
#ifndef IAIPLUGIN_H
#define IAIPLUGIN_H

#include "IPlugin.h"
#include <QString>
#include <QMap>
#include <QJsonObject>

/**
 * @brief AI分析结果
 */
struct AIAnalysisResult {
    QString summary;                // 分析摘要
    QString recommendation;         // 建议
    double confidence;              // 置信度（0-1）
    QMap<QString, double> indicators;  // 指标数据
    QJsonObject details;            // 详细信息
};

/**
 * @brief 市场预测结果
 */
struct MarketPrediction {
    QString instrumentId;           // 合约代码
    double predictedPrice;          // 预测价格
    double confidence;              // 置信度
    QString trend;                  // 趋势（上涨/下跌/震荡）
    QString reason;                 // 原因分析
    QJsonObject factors;            // 影响因素
};

/**
 * @brief 投顾建议
 */
struct InvestmentAdvice {
    QString title;                  // 标题
    QString content;                // 内容
    QString riskLevel;              // 风险等级
    double expectedReturn;          // 预期收益
    double maxDrawdown;             // 最大回撤
    QStringList actions;            // 操作建议
    QJsonObject details;            // 详细信息
};

/**
 * @brief AI插件接口
 */
class IAIPlugin : public IPlugin
{
    Q_OBJECT

public:
    virtual ~IAIPlugin() = default;

    // ========== 对话接口 ==========

    /**
     * @brief 发送消息
     */
    virtual QString sendMessage(const QString& message, 
                               const QJsonObject& context = QJsonObject()) = 0;

    /**
     * @brief 异步发送消息
     */
    virtual void sendMessageAsync(const QString& message,
                                 const QJsonObject& context = QJsonObject()) = 0;

    /**
     * @brief 清除对话历史
     */
    virtual void clearHistory() = 0;

    // ========== 分析接口 ==========

    /**
     * @brief 分析市场行情
     */
    virtual AIAnalysisResult analyzeMarket(const QString& instrumentId,
                                          const QMap<QString, double>& data) = 0;

    /**
     * @brief 分析技术指标
     */
    virtual AIAnalysisResult analyzeIndicators(const QString& instrumentId,
                                               const QMap<QString, double>& indicators) = 0;

    /**
     * @brief 分析市场情绪
     */
    virtual AIAnalysisResult analyzeSentiment(const QStringList& news,
                                             const QStringList& comments) = 0;

    // ========== 预测接口 ==========

    /**
     * @brief 预测价格走势
     */
    virtual MarketPrediction predictPrice(const QString& instrumentId,
                                         const QMap<QString, double>& historicalData) = 0;

    /**
     * @brief 预测趋势
     */
    virtual QString predictTrend(const QString& instrumentId,
                                const QMap<QString, double>& data) = 0;

    // ========== 投顾接口 ==========

    /**
     * @brief 获取投资建议
     */
    virtual InvestmentAdvice getAdvice(const QString& instrumentId,
                                      const QJsonObject& portfolio) = 0;

    /**
     * @brief 生成交易策略
     */
    virtual QString generateStrategy(const QString& instrumentId,
                                    const QJsonObject& params) = 0;

    /**
     * @brief 优化投资组合
     */
    virtual QJsonObject optimizePortfolio(const QJsonObject& currentPortfolio,
                                         const QJsonObject& constraints) = 0;

    // ========== 风险评估 ==========

    /**
     * @brief 评估风险
     */
    virtual double assessRisk(const QString& instrumentId,
                             const QJsonObject& position) = 0;

    /**
     * @brief 计算VaR（风险价值）
     */
    virtual double calculateVaR(const QJsonObject& portfolio,
                               double confidence = 0.95) = 0;

signals:
    /**
     * @brief 消息响应信号
     */
    void messageReceived(const QString& response);

    /**
     * @brief 分析完成信号
     */
    void analysisCompleted(const AIAnalysisResult& result);

    /**
     * @brief 预测完成信号
     */
    void predictionCompleted(const MarketPrediction& prediction);

    /**
     * @brief 建议生成信号
     */
    void adviceGenerated(const InvestmentAdvice& advice);
};

Q_DECLARE_INTERFACE(IAIPlugin, "com.wealthpilot.IAIPlugin/2.0")

#endif // IAIPLUGIN_H
