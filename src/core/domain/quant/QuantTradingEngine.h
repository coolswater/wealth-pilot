/**
 * @file QuantTradingEngine.h
 * @brief 量化交易引擎 - 量化交易系统核心
 *
 * @details 功能：
 * - 策略执行引擎
 * - 实时数据处理
 * - 订单管理
 * - 风控集成
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef QUANTTRADINGENGINE_H
#define QUANTTRADINGENGINE_H

#include <QObject>
#include <QVector>
#include <QString>
#include <QTimer>
#include <QHash>
#include "../backtest/BacktestEngine.h"

/**
 * @brief 交易状态
 */
enum class TradingState {
    Idle,               ///< 空闲
    Running,            ///< 运行中
    Paused,             ///< 已暂停
    Error               ///< 错误
};

/**
 * @brief 交易模式
 */
enum class TradingMode {
    Simulation,         ///< 模拟盘
    Live                ///< 实盘
};

/**
 * @brief 实时交易记录
 */
struct LiveTrade {
    QString id;                 ///< 交易ID
    QString strategyId;         ///< 策略ID
    QString symbol;             ///< 股票代码
    QString direction;          ///< 方向
    double price;               ///< 价格
    int quantity;               ///< 数量
    QDateTime time;             ///< 时间
    QString status;             ///< 状态
    double profit = 0.0;        ///< 盈亏
    QString orderId;            ///< 订单ID
};

/**
 * @brief 策略运行状态
 */
struct StrategyRuntime {
    QString strategyId;         ///< 策略ID
    QString name;               ///< 策略名称
    TradingState state;         ///< 状态
    double totalProfit = 0.0;   ///< 总盈亏
    int tradeCount = 0;         ///< 交易次数
    double winRate = 0.0;       ///< 胜率
    QDateTime startTime;        ///< 开始时间
    QVector<QString> symbols;   ///< 监控标的
};

/**
 * @brief 风控规则（量化交易）
 */
struct QuantRiskRule {
    QString id;                 ///< 规则ID
    QString name;               ///< 规则名称
    QString type;               ///< 类型
    double threshold = 0.0;     ///< 阈值
    QString action;             ///< 动作
    bool enabled = true;        ///< 是否启用
};

/**
 * @brief 量化交易引擎
 *
 * 提供量化交易功能：
 * - 策略运行管理
 * - 实时数据处理
 * - 风控集成
 * - 交易记录
 */
class QuantTradingEngine : public QObject {
    Q_OBJECT

public:
    static QuantTradingEngine* instance();

    // ========== 策略管理 ==========

    /**
     * @brief 添加策略
     */
    void addStrategy(std::shared_ptr<IStrategy> strategy, const QString& strategyId);

    /**
     * @brief 移除策略
     */
    void removeStrategy(const QString& strategyId);

    /**
     * @brief 启动策略
     */
    bool startStrategy(const QString& strategyId);

    /**
     * @brief 停止策略
     */
    bool stopStrategy(const QString& strategyId);

    /**
     * @brief 暂停策略
     */
    bool pauseStrategy(const QString& strategyId);

    /**
     * @brief 恢复策略
     */
    bool resumeStrategy(const QString& strategyId);

    /**
     * @brief 获取策略状态
     */
    StrategyRuntime getStrategyStatus(const QString& strategyId) const;

    /**
     * @brief 获取所有策略状态
     */
    QVector<StrategyRuntime> getAllStrategyStatus() const;

    // ========== 交易模式 ==========

    /**
     * @brief 设置交易模式
     */
    void setTradingMode(TradingMode mode);

    /**
     * @brief 获取交易模式
     */
    TradingMode tradingMode() const { return m_mode; }

    // ========== 数据处理 ==========

    /**
     * @brief 接收市场数据
     */
    void onMarketData(const QString& symbol, const QVariantMap& data);

    /**
     * @brief 设置数据订阅
     */
    void subscribeSymbols(const QString& strategyId, const QStringList& symbols);

    // ========== 风控管理 ==========

    /**
     * @brief 添加风控规则
     */
    void addRiskRule(const QuantRiskRule& rule);

    /**
     * @brief 移除风控规则
     */
    void removeRiskRule(const QString& ruleId);

    /**
     * @brief 检查风控
     */
    bool checkRiskControl(const QString& strategyId, const StrategySignal& signal);

    // ========== 交易记录 ==========

    /**
     * @brief 获取交易记录
     */
    QVector<LiveTrade> getTradeHistory(const QString& strategyId = QString()) const;

    /**
     * @brief 获取持仓
     */
    QHash<QString, int> getPositions(const QString& strategyId = QString()) const;

    // ========== 统计 ==========

    /**
     * @brief 获取总体统计
     */
    struct OverallStats {
        double totalProfit = 0.0;
        int totalTrades = 0;
        double winRate = 0.0;
        double maxDrawdown = 0.0;
        int activeStrategies = 0;
    };
    OverallStats getOverallStats() const;

signals:
    /**
     * @brief 策略状态变化
     */
    void strategyStateChanged(const QString& strategyId, TradingState state);

    /**
     * @brief 交易发生
     */
    void tradeExecuted(const LiveTrade& trade);

    /**
     * @brief 风控触发
     */
    void riskTriggered(const QString& ruleId, const QString& message);

    /**
     * @brief 错误发生
     */
    void errorOccurred(const QString& strategyId, const QString& error);

private:
    explicit QuantTradingEngine(QObject* parent = nullptr);
    ~QuantTradingEngine() override;

    void processSignal(const QString& strategyId, const StrategySignal& signal);
    void executeOrder(const QString& strategyId, const StrategySignal& signal);
    void updatePositions(const QString& symbol, double price);
    void checkAllRiskRules();

    TradingMode m_mode = TradingMode::Simulation;
    QHash<QString, std::shared_ptr<IStrategy>> m_strategies;
    QHash<QString, StrategyRuntime> m_strategyStatus;
    QHash<QString, QStringList> m_strategySymbols;

    QVector<LiveTrade> m_tradeHistory;
    QHash<QString, QHash<QString, int>> m_positions; // strategyId -> symbol -> quantity

    QVector<QuantRiskRule> m_riskRules;
    QTimer* m_riskCheckTimer = nullptr;
};

#endif // QUANTTRADINGENGINE_H