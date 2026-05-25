/**
 * @file BacktestEngine.h
 * @brief 策略回测引擎
 *
 * @details 功能：
 * - 策略定义和执行
 * - 历史数据回测
 * - 性能指标计算
 * - 回测报告生成
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef BACKTESTENGINE_H
#define BACKTESTENGINE_H

#include <QObject>
#include <QVector>
#include <QDateTime>
#include <QString>
#include <functional>

/**
 * @brief 回测交易记录
 */
struct BacktestTrade {
    QDateTime time;          ///< 交易时间
    QString symbol;          ///< 股票代码
    QString direction;       ///< 方向 (buy/sell)
    double price;            ///< 价格
    int quantity;            ///< 数量
    double amount;           ///< 金额
    double commission;       ///< 手续费
    double profit = 0.0;     ///< 盈亏
};

/**
 * @brief 回测持仓
 */
struct BacktestPosition {
    QString symbol;          ///< 股票代码
    int quantity = 0;        ///< 持仓数量
    double cost = 0.0;       ///< 成本
    double marketValue = 0.0; ///< 市值
    double profit = 0.0;     ///< 盈亏
};

/**
 * @brief 回测统计结果
 */
struct BacktestStats {
    double totalReturn = 0.0;        ///< 总收益率
    double annualizedReturn = 0.0;   ///< 年化收益率
    double maxDrawdown = 0.0;        ///< 最大回撤
    double sharpeRatio = 0.0;        ///< 夏普比率
    double winRate = 0.0;            ///< 胜率
    double profitFactor = 0.0;       ///< 盈亏比
    int totalTrades = 0;             ///< 总交易次数
    int winTrades = 0;               ///< 盈利次数
    int lossTrades = 0;              ///< 亏损次数
    double avgProfit = 0.0;          ///< 平均盈利
    double avgLoss = 0.0;            ///< 平均亏损
    double maxProfit = 0.0;          ///< 最大盈利
    double maxLoss = 0.0;            ///< 最大亏损
    double avgHoldingDays = 0.0;     ///< 平均持仓天数
};

/**
 * @brief 策略信号
 */
struct StrategySignal {
    QDateTime time;          ///< 时间
    QString symbol;          ///< 股票代码
    QString action;          ///< 动作 (buy/sell/hold)
    double price;            ///< 价格
    int quantity;            ///< 建议数量
    QString reason;          ///< 原因说明
};

/**
 * @brief 策略接口
 */
class IStrategy {
public:
    virtual ~IStrategy() = default;

    /**
     * @brief 初始化策略
     */
    virtual void initialize() = 0;

    /**
     * @brief 处理数据点
     * @param data 当前数据
     * @return 交易信号
     */
    virtual StrategySignal processData(const QVariantMap& data) = 0;

    /**
     * @brief 策略名称
     */
    virtual QString name() const = 0;

    /**
     * @brief 策略描述
     */
    virtual QString description() const = 0;
};

/**
 * @brief 回测引擎
 *
 * 提供策略回测功能：
 * - 策略执行
 * - 性能分析
 * - 报告生成
 */
class BacktestEngine : public QObject {
    Q_OBJECT

public:
    static BacktestEngine* instance();

    /**
     * @brief 设置策略
     */
    void setStrategy(std::shared_ptr<IStrategy> strategy);

    /**
     * @brief 设置回测参数
     */
    void setParameters(double initialCapital,
                       double commissionRate = 0.0003,
                       double slippage = 0.001);

    /**
     * @brief 设置回测数据
     * @param symbol 股票代码
     * @param data K线数据
     */
    void setData(const QString& symbol, const QVector<QVariantMap>& data);

    /**
     * @brief 运行回测
     */
    void run();

    /**
     * @brief 异步运行回测
     */
    void runAsync();

    /**
     * @brief 获取交易记录
     */
    QVector<BacktestTrade> getTrades() const { return m_trades; }

    /**
     * @brief 获取统计结果
     */
    BacktestStats getStats() const { return m_stats; }

    /**
     * @brief 获取权益曲线
     */
    QVector<QPair<QDateTime, double>> getEquityCurve() const { return m_equityCurve; }

    /**
     * @brief 生成报告
     */
    QString generateReport() const;

    /**
     * @brief 导出交易记录
     */
    bool exportTrades(const QString& filePath) const;

signals:
    /**
     * @brief 回测完成
     */
    void backtestCompleted(const BacktestStats& stats);

    /**
     * @brief 回测进度
     */
    void progressChanged(int current, int total);

    /**
     * @brief 交易发生
     */
    void tradeOccurred(const BacktestTrade& trade);

private:
    explicit BacktestEngine(QObject* parent = nullptr);
    ~BacktestEngine() override = default;

    void executeTrade(const StrategySignal& signal, const QDateTime& time, double price);
    void updatePositions(double currentPrice);
    void calculateStats();
    void recordEquity(const QDateTime& time, double equity);

    std::shared_ptr<IStrategy> m_strategy;
    QString m_symbol;
    QVector<QVariantMap> m_data;

    double m_initialCapital = 1000000.0;
    double m_commissionRate = 0.0003;
    double m_slippage = 0.001;
    double m_currentCash = 0.0;

    QVector<BacktestTrade> m_trades;
    QHash<QString, BacktestPosition> m_positions;
    BacktestStats m_stats;
    QVector<QPair<QDateTime, double>> m_equityCurve;
};

#endif // BACKTESTENGINE_H