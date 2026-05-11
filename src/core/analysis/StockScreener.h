/**
 * @file StockScreener.h
 * @brief 股票筛选器 - AI 智能选股
 *
 * @details 功能：
 * - 基于技术指标筛选
 * - 基于基本面筛选
 * - 自定义筛选条件
 * - 筛选结果排序
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef STOCKSCREENER_H
#define STOCKSCREENER_H

#include <QObject>
#include <QVector>
#include <QString>
#include <QVariant>
#include <functional>

/**
 * @brief 筛选条件
 */
struct ScreenerCondition {
    QString field;          ///< 字段名
    QString op;             ///< 操作符 (>, <, =, >=, <=, between, contains)
    QVariant value;         ///< 值
    QVariant value2;        ///< 第二个值（用于 between）
    double weight = 1.0;    ///< 权重
    bool enabled = true;    ///< 是否启用
};

/**
 * @brief 筛选结果
 */
struct ScreenerResult {
    QString symbol;         ///< 股票代码
    QString name;           ///< 股票名称
    double score = 0.0;     ///< 综合得分
    int matchCount = 0;     ///< 匹配条件数
    QVariantMap details;    ///< 详细数据
};

/**
 * @brief 股票筛选器
 *
 * 提供智能选股功能：
 * - 多条件筛选
 * - 条件组合
 * - 结果排序
 * - 筛选模板
 */
class StockScreener : public QObject {
    Q_OBJECT

public:
    static StockScreener* instance();

    /**
     * @brief 添加筛选条件
     */
    void addCondition(const ScreenerCondition& condition);

    /**
     * @brief 添加筛选条件（简化版）
     */
    void addCondition(const QString& field, const QString& op,
                     const QVariant& value, double weight = 1.0);

    /**
     * @brief 清除所有条件
     */
    void clearConditions();

    /**
     * @brief 设置筛选范围
     * @param symbols 股票代码列表
     */
    void setScope(const QStringList& symbols);

    /**
     * @brief 执行筛选
     */
    void execute();

    /**
     * @brief 异步执行筛选
     */
    void executeAsync();

    /**
     * @brief 获取结果
     */
    QVector<ScreenerResult> getResults() const { return m_results; }

    /**
     * @brief 获取符合条件的股票数量
     */
    int getMatchCount() const { return m_results.size(); }

    /**
     * @brief 保存筛选模板
     */
    void saveTemplate(const QString& name);

    /**
     * @brief 加载筛选模板
     */
    bool loadTemplate(const QString& name);

    /**
     * @brief 获取模板列表
     */
    QStringList getTemplates() const;

    // ========== 预设筛选策略 ==========

    /**
     * @brief 突破策略
     */
    void setupBreakoutStrategy();

    /**
     * @brief 超跌策略
     */
    void setupOversoldStrategy();

    /**
     * @brief 强势策略
     */
    void setupMomentumStrategy();

    /**
     * @brief 价值策略
     */
    void setupValueStrategy();

    /**
     * @brief 成长策略
     */
    void setupGrowthStrategy();

signals:
    /**
     * @brief 筛选完成
     */
    void screeningCompleted(const QVector<ScreenerResult>& results);

    /**
     * @brief 筛选进度
     */
    void progressChanged(int current, int total);

private:
    explicit StockScreener(QObject* parent = nullptr);
    ~StockScreener() override = default;

    bool evaluateCondition(const QString& symbol, const ScreenerCondition& condition);
    double calculateScore(const QString& symbol, const QVector<ScreenerCondition>& matchedConditions);
    QVariantMap getStockData(const QString& symbol);

    QVector<ScreenerCondition> m_conditions;
    QStringList m_scope;
    QVector<ScreenerResult> m_results;
};

#endif // STOCKSCREENER_H
