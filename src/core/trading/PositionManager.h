/**
 * @file PositionManager.h
 * @brief 持仓管理器 - 统一管理所有持仓
 *
 * @details 功能：
 * - 实时持仓监控
 * - 盈亏计算
 * - 保证金管理
 * - 持仓风险分析
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef POSITIONMANAGER_H
#define POSITIONMANAGER_H

#include <QObject>
#include <QHash>
#include <QMap>
#include <QMutex>
#include <memory>
#include "TradingTypes.h"

/**
 * @brief 持仓管理器
 * @details 单例模式，统一管理所有持仓
 * 
 * @example
 * @code
 * // 获取持仓
 * QVector<PositionInfo> positions = PositionManager::instance().getPositions();
 * 
 * // 获取总盈亏
 * double totalProfit = PositionManager::instance().getTotalProfit();
 * @endcode
 */
class PositionManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 获取单例实例
     */
    static PositionManager& instance();

    /**
     * @brief 初始化持仓管理器
     */
    bool initialize();

    /**
     * @brief 关闭持仓管理器
     */
    void shutdown();

    // ========== 持仓操作 ==========

    /**
     * @brief 更新持仓信息（来自CTP推送）
     * @param position 持仓信息
     */
    void updatePosition(const PositionInfo& position);

    /**
     * @brief 删除持仓
     * @param instrumentId 合约代码
     * @param direction 持仓方向
     */
    void removePosition(const QString& instrumentId, PositionDirection direction);

    /**
     * @brief 清空所有持仓
     */
    void clearPositions();

    /**
     * @brief 更新持仓价格（行情更新时调用）
     * @param instrumentId 合约代码
     * @param lastPrice 最新价
     */
    void updatePositionPrice(const QString& instrumentId, double lastPrice);

    // ========== 查询接口 ==========

    /**
     * @brief 获取所有持仓
     * @return 持仓列表
     */
    QVector<PositionInfo> getPositions() const;

    /**
     * @brief 获取指定合约的持仓
     * @param instrumentId 合约代码
     * @return 持仓列表（多空可能都有）
     */
    QVector<PositionInfo> getPositions(const QString& instrumentId) const;

    /**
     * @brief 获取指定方向的持仓
     * @param instrumentId 合约代码
     * @param direction 持仓方向
     * @return 持仓信息，不存在返回空
     */
    std::optional<PositionInfo> getPosition(const QString& instrumentId, 
                                             PositionDirection direction) const;

    /**
     * @brief 判断是否有持仓
     * @param instrumentId 合约代码
     * @return 是否有持仓
     */
    bool hasPosition(const QString& instrumentId) const;

    /**
     * @brief 获取持仓数量
     * @param instrumentId 合约代码
     * @param direction 持仓方向
     * @return 持仓数量
     */
    int getPositionVolume(const QString& instrumentId, PositionDirection direction) const;

    /**
     * @brief 获取可平仓数量
     * @param instrumentId 合约代码
     * @param direction 持仓方向
     * @return 可平仓数量
     */
    int getCloseableVolume(const QString& instrumentId, PositionDirection direction) const;

    // ========== 盈亏计算 ==========

    /**
     * @brief 获取总浮动盈亏
     */
    double getTotalProfit() const;

    /**
     * @brief 获取今日盈亏
     */
    double getTodayProfit() const;

    /**
     * @brief 获取已实现盈亏
     */
    double getRealizedProfit() const;

    /**
     * @brief 获取指定合约的盈亏
     * @param instrumentId 合约代码
     */
    double getInstrumentProfit(const QString& instrumentId) const;

    /**
     * @brief 获取盈亏比例
     */
    double getProfitRatio() const;

    // ========== 保证金计算 ==========

    /**
     * @brief 获取总占用保证金
     */
    double getTotalMargin() const;

    /**
     * @brief 获取总冻结保证金
     */
    double getTotalFrozenMargin() const;

    /**
     * @brief 获取保证金比例
     */
    double getMarginRatio() const;

    // ========== 风险分析 ==========

    /**
     * @brief 持仓风险统计
     */
    struct PositionRisk {
        double totalProfit = 0.0;       ///< 总盈亏
        double maxProfit = 0.0;         ///< 最大盈利
        double maxLoss = 0.0;           ///< 最大亏损
        double profitRatio = 0.0;       ///< 盈利比例
        int profitCount = 0;            ///< 盈利持仓数
        int lossCount = 0;              ///< 亏损持仓数
        int flatCount = 0;              ///< 持平持仓数
        double maxDrawdown = 0.0;       ///< 最大回撤
        double riskLevel = 0.0;         ///< 风险度
    };
    PositionRisk calculateRisk() const;

    /**
     * @brief 获取持仓汇总
     */
    struct PositionSummary {
        int totalPositions = 0;         ///< 总持仓数
        int longPositions = 0;          ///< 多头持仓数
        int shortPositions = 0;         ///< 空头持仓数
        double totalVolume = 0.0;       ///< 总持仓量
        double totalValue = 0.0;        ///< 总市值
        double totalMargin = 0.0;       ///< 总保证金
        double totalProfit = 0.0;       ///< 总盈亏
        double todayProfit = 0.0;       ///< 今日盈亏
        double realizedProfit = 0.0;    ///< 已实现盈亏
    };
    PositionSummary getSummary() const;

signals:
    /**
     * @brief 持仓更新
     * @param position 持仓信息
     */
    void positionUpdated(const PositionInfo& position);

    /**
     * @brief 持仓删除
     * @param instrumentId 合约代码
     * @param direction 持仓方向
     */
    void positionRemoved(const QString& instrumentId, PositionDirection direction);

    /**
     * @brief 盈亏更新
     * @param totalProfit 总盈亏
     */
    void profitUpdated(double totalProfit);

    /**
     * @brief 风险预警
     * @param riskType 风险类型
     * @param message 预警信息
     */
    void riskAlert(const QString& riskType, const QString& message);

private:
    // 私有构造函数（单例）
    PositionManager(QObject* parent = nullptr);
    ~PositionManager() override;
    Q_DISABLE_COPY(PositionManager)

    // 生成持仓键
    QString makePositionKey(const QString& instrumentId, PositionDirection direction) const;

    // 计算持仓盈亏
    void calculatePositionProfit(PositionInfo& position, double lastPrice);

    // PIMPL 实现
    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // POSITIONMANAGER_H
