/**
 * @file AIPlugin.h
 * @brief AI插件实现 - 基于插件接口的AI服务
 *
 * @details 功能：
 * - 实现IAIPlugin接口
 * - 集成ServiceLocator依赖注入
 * - 集成EnvironmentConfig配置管理
 * - 集成CacheManager缓存系统
 * - 高性能AI请求处理
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */
#ifndef AIPLUGIN_H
#define AIPLUGIN_H

#include "../../plugins/IAIPlugin.h"
#include "../../core/di/ServiceLocator.h"
#include "../../core/config/EnvironmentConfig.h"
#include "../../core/cache/CacheManager.h"
#include <QTimer>
#include <QNetworkAccessManager>
#include <QQueue>
#include <memory>

namespace AI {

/**
 * @brief AI插件实现类
 * @note 不使用 Q_PLUGIN_METADATA，作为普通服务类使用
 */
class AIPlugin : public IAIPlugin
{
    Q_OBJECT
    Q_INTERFACES(IAIPlugin)

public:
    explicit AIPlugin();
    ~AIPlugin() override;

    // ========== IPlugin接口实现 ==========

    PluginMetaData metaData() const override;
    PluginState state() const override;

    bool load() override;
    bool initialize(const QJsonObject& config = QJsonObject()) override;
    bool start() override;
    void stop() override;
    void unload() override;

    QJsonObject configuration() const override;
    void setConfiguration(const QJsonObject& config) override;

    bool checkDependencies() const override;
    QStringList dependencies() const override;

    // ========== IAIPlugin接口实现 ==========

    // 对话接口
    QString sendMessage(const QString& message,
                       const QJsonObject& context = QJsonObject()) override;
    
    void sendMessageAsync(const QString& message,
                         const QJsonObject& context = QJsonObject()) override;
    
    void clearHistory() override;

    // 分析接口
    AIAnalysisResult analyzeMarket(const QString& instrumentId,
                                  const QMap<QString, double>& data) override;
    
    AIAnalysisResult analyzeIndicators(const QString& instrumentId,
                                      const QMap<QString, double>& indicators) override;
    
    AIAnalysisResult analyzeSentiment(const QStringList& news,
                                     const QStringList& comments) override;

    // 预测接口
    MarketPrediction predictPrice(const QString& instrumentId,
                                 const QMap<QString, double>& historicalData) override;
    
    QString predictTrend(const QString& instrumentId,
                        const QMap<QString, double>& data) override;

    // 投顾接口
    InvestmentAdvice getAdvice(const QString& instrumentId,
                              const QJsonObject& portfolio) override;
    
    QString generateStrategy(const QString& instrumentId,
                            const QJsonObject& params) override;
    
    QJsonObject optimizePortfolio(const QJsonObject& currentPortfolio,
                                 const QJsonObject& constraints) override;

    // 风险评估
    double assessRisk(const QString& instrumentId,
                     const QJsonObject& position) override;
    
    double calculateVaR(const QJsonObject& portfolio,
                       double confidence = 0.95) override;

signals:
    // IPlugin信号
    void stateChanged(PluginState newState);
    void errorOccurred(const QString& error);
    void configurationChanged();

    // IAIPlugin信号
    void messageReceived(const QString& response);
    void analysisCompleted(const AIAnalysisResult& result);
    void predictionCompleted(const MarketPrediction& prediction);
    void adviceGenerated(const InvestmentAdvice& advice);

private:
    // 内部实现
    class Impl;
    std::unique_ptr<Impl> d;
    
    // 状态管理
    PluginState m_state;
    QJsonObject m_config;
    
    // 网络管理器
    QNetworkAccessManager* m_networkManager;
    
    // 对话历史
    QList<QJsonObject> m_conversationHistory;
    int m_maxHistorySize;
    
    // 性能优化：请求缓存
    QMap<QString, QString> m_responseCache;
    mutable QMutex m_cacheMutex;
    
    // 批量请求队列（性能优化）
    QQueue<QJsonObject> m_requestQueue;
    QTimer* m_batchTimer;
    void processBatchRequests();
    
    // 集成CacheManager
    QString getCachedResponse(const QString& cacheKey);
    void cacheResponse(const QString& cacheKey, const QString& response);
    
    // 集成EnvironmentConfig
    void loadEnvironmentConfig();
    
    // 辅助方法
    void setState(PluginState newState);
    QString buildPrompt(const QString& message, const QJsonObject& context);
    QJsonObject callAI(const QString& prompt);
    AIAnalysisResult parseAnalysisResult(const QJsonObject& json);
    MarketPrediction parsePredictionResult(const QJsonObject& json);
    InvestmentAdvice parseAdviceResult(const QJsonObject& json);
    QString generateCacheKey(const QString& message, const QJsonObject& context);
};

} // namespace AI

#endif // AIPLUGIN_H
