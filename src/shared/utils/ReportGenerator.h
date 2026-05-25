/**
 * @file ReportGenerator.h
 * @brief 报表生成器 - 生成交易报表和统计分析
 *
 * @details 功能：
 * - 日报表：当日交易汇总
 * - 周报表：本周交易分析
 * - 月报表：月度交易统计
 * - 自定义报表：指定时间段
 * - 资金曲线图数据生成
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef REPORTGENERATOR_H
#define REPORTGENERATOR_H

#include <QObject>
#include <QDateTime>
#include <QVector>
#include <QVariantMap>
#include <QPair>

/**
 * @brief 报表类型
 */
enum class ReportType {
    Daily,      ///< 日报表
    Weekly,     ///< 周报表
    Monthly,    ///< 月报表
    Custom      ///< 自定义时间范围
};

/**
 * @brief 报表数据结构
 */
struct ReportData {
    QString title;
    QDateTime startTime;
    QDateTime endTime;

    // 交易统计
    int totalTrades = 0;
    int winTrades = 0;
    int lossTrades = 0;
    double winRate = 0.0;

    double totalProfit = 0.0;
    double totalLoss = 0.0;
    double netProfit = 0.0;
    double maxProfit = 0.0;
    double maxLoss = 0.0;

    double totalCommission = 0.0;
    double totalTurnover = 0.0;

    // 风险指标
    double maxDrawdown = 0.0;
    double sharpeRatio = 0.0;
    double profitFactor = 0.0;

    // 资金曲线数据
    QVector<QPair<QDateTime, double>> equityCurve;

    // 详细数据
    QVector<QVariantMap> tradeDetails;
    QVariantList instrumentBreakdown;  // 按合约分类统计
};

/**
 * @brief 报表生成器
 */
class ReportGenerator : public QObject
{
    Q_OBJECT

public:
    static ReportGenerator& instance();

    /**
     * @brief 生成日报表
     */
    ReportData generateDailyReport(const QDate &date = QDate::currentDate());

    /**
     * @brief 生成周报表
     */
    ReportData generateWeeklyReport(const QDate &weekStart = QDate::currentDate());

    /**
     * @brief 生成月报表
     */
    ReportData generateMonthlyReport(int year, int month);

    /**
     * @brief 生成自定义报表
     */
    ReportData generateCustomReport(const QDateTime &start, const QDateTime &end);

    /**
     * @brief 导出报表到文件
     */
    bool exportReport(const ReportData &report, const QString &filePath);

    /**
     * @brief 获取资金曲线数据
     */
    QVector<QPair<QDateTime, double>> getEquityCurve(const QDateTime &start,
                                                      const QDateTime &end,
                                                      int intervalMinutes = 60);

signals:
    void reportGenerated(const ReportData &report);
    void reportError(const QString &error);

private:
    ReportGenerator(QObject *parent = nullptr);
    ~ReportGenerator() override;
    Q_DISABLE_COPY(ReportGenerator)

    void calculateStatistics(ReportData &report);
    void calculateRiskMetrics(ReportData &report);
    void groupByInstrument(ReportData &report);

    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // REPORTGENERATOR_H
