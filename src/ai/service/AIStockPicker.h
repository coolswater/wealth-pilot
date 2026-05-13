/**
 * @file AIStockPicker.h
 * @brief AI 智能选股服务
 *
 * @details 功能：
 * - 因子选股
 * - AI 辅助选股
 * - 相似股票推荐
 * - 概念选股
 * - 选股策略回测
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef AISTOCKPICKER_H
#define AISTOCKPICKER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QMap>
#include <QList>
#include <QDateTime>
#include <functional>

namespace WealthPilot
{
    /**
 * @brief 选股因子枚举
 */
    enum class StockFactor
    {
        PE, ///< 市盈率
        PB, ///< 市净率
        ROE, ///< 净资产收益率
        RevenueGrowth, ///< 营收增长率
        ProfitGrowth, ///< 利润增长率
        DebtRatio, ///< 资产负债率
        TurnoverRate, ///< 换手率
        MarketCap, ///< 市值
        DividendYield, ///< 股息率
        Momentum, ///< 动量因子
        Volatility ///< 波动率
    };

    /**
 * @brief 选股条件
 */
    struct ScreeningCondition
    {
        StockFactor factor; ///< 因子
        double minValue = 0; ///< 最小值
        double maxValue = 1e9; ///< 最大值
        double weight = 1.0; ///< 权重
        bool enabled = true; ///< 是否启用
    };

    /**
 * @brief 选股结果
 */
    struct StockPickResult
    {
        QString stockCode; ///< 股票代码
        QString stockName; ///< 股票名称
        double score = 0; ///< 综合得分
        int rank = 0; ///< 排名
        QString reason; ///< 选股理由
        QDateTime pickedAt; ///< 选股时间

        static StockPickResult fromJson(const QJsonObject& json);
        QJsonObject toJson() const;
    };

    /**
 * @brief 选股策略
 */
    struct StockPickingStrategy
    {
        QString id;
        QString name;
        QString description;
        QList<ScreeningCondition> conditions;
        int maxResults = 50;
        bool useAI = true;
        QDateTime createdAt;
        QDateTime updatedAt;

        static StockPickingStrategy fromJson(const QJsonObject& json);
        QJsonObject toJson() const;
    };

    /**
 * @brief AI 智能选股服务
 */
    class AIStockPicker : public QObject
    {
        Q_OBJECT

    public:
        explicit AIStockPicker(QObject* parent = nullptr);
        ~AIStockPicker() override;

        // 因子选股
        QList<StockPickResult> screenByFactors(
            const QList<ScreeningCondition>& conditions,
            int maxResults = 50);

        QList<StockPickResult> quickScreen(
            const QString& strategyType,
            int maxResults = 50);

        // AI 选股
        void screenByNaturalLanguage(
            const QString& description,
            std::function<void(const QList<StockPickResult> &)> callback);

        // 相似股票
        QList<StockPickResult> findSimilarStocks(
            const QString& stockCode,
            int maxResults = 10);

        // 概念选股
        QList<StockPickResult> screenByConcept(
            const QString& conceptName,
            int maxResults = 20);

        QStringList getHotConcepts() const;

        // 策略管理
        QString createStrategy(const QString& name,
                               const QList<ScreeningCondition>& conditions);
        QList<StockPickingStrategy> listStrategies() const;

        // 工具方法
        static QString getFactorName(StockFactor factor);
        QStringList getPresetStrategies() const;

        signals :

        void screeningCompleted(const QList<StockPickResult>& results);
        void errorOccurred(const QString& error);

    private:
        void initializePresetStrategies();
        double calculateFactorScore(const QString& stockCode,
                                    const QList<ScreeningCondition>& conditions);
        double getStockFactorValue(const QString& stockCode, StockFactor factor);

    private:
        QMap<QString, StockPickingStrategy> m_strategies;
        QString m_storagePath;
    };
} // namespace WealthPilot

#endif // AISTOCKPICKER_H