/**
 * @file ReportGenerator.cpp
 * @brief 报表生成器实现
 */

#include "ReportGenerator.h"
#include "ExcelExporter.h"
#include "data/DataStorage.h"
#include "data/DataStorageService.h"
#include "services/trading/TradingService.h"
#include "domain/trading/PositionManager.h"
#include "shared/utils/Logger.h"

#include <QMutexLocker>
#include <QFile>
#include <QTextStream>
#include <QMap>
#include <QtMath>

struct ReportGenerator::Impl {
    mutable QMutex mutex;
};

ReportGenerator& ReportGenerator::instance()
{
    static ReportGenerator instance;
    return instance;
}

ReportGenerator::ReportGenerator(QObject *parent)
    : QObject(parent)
    , d(std::make_unique<Impl>())
{
    LOG_DEBUG("ReportGenerator created");
}

ReportGenerator::~ReportGenerator()
{
    LOG_DEBUG("ReportGenerator destroyed");
}

ReportData ReportGenerator::generateDailyReport(const QDate &date)
{
    QDateTime start(date, QTime(0, 0, 0));
    QDateTime end(date, QTime(23, 59, 59));

    ReportData report;
    report.title = QString("日报表 - %1").arg(date.toString("yyyy-MM-dd"));
    report.startTime = start;
    report.endTime = end;

    // 获取交易记录
    report.tradeDetails = DataStorage::instance().loadTradeRecords(start, end);

    calculateStatistics(report);
    calculateRiskMetrics(report);
    groupByInstrument(report);

    LOG_INFO(QString("Generated daily report for %1, trades: %2")
        .arg(date.toString("yyyy-MM-dd")).arg(report.totalTrades));

    emit reportGenerated(report);
    return report;
}

ReportData ReportGenerator::generateWeeklyReport(const QDate &weekStart)
{
    // 计算周一
    QDate monday = weekStart.addDays(-(weekStart.dayOfWeek() - 1));
    QDateTime start(monday, QTime(0, 0, 0));
    QDateTime end(monday.addDays(6), QTime(23, 59, 59));

    ReportData report;
    report.title = QString("周报表 - %1 至 %2")
        .arg(start.toString("yyyy-MM-dd"))
        .arg(end.toString("yyyy-MM-dd"));
    report.startTime = start;
    report.endTime = end;

    report.tradeDetails = DataStorage::instance().loadTradeRecords(start, end);

    calculateStatistics(report);
    calculateRiskMetrics(report);
    groupByInstrument(report);

    LOG_INFO(QString("Generated weekly report, trades: %1").arg(report.totalTrades));

    emit reportGenerated(report);
    return report;
}

ReportData ReportGenerator::generateMonthlyReport(int year, int month)
{
    QDateTime start(QDate(year, month, 1), QTime(0, 0, 0));
    QDateTime end(QDate(year, month, 1).addMonths(1).addDays(-1), QTime(23, 59, 59));

    ReportData report;
    report.title = QString("月报表 - %1年%2月").arg(year).arg(month);
    report.startTime = start;
    report.endTime = end;

    report.tradeDetails = DataStorage::instance().loadTradeRecords(start, end);

    calculateStatistics(report);
    calculateRiskMetrics(report);
    groupByInstrument(report);

    LOG_INFO(QString("Generated monthly report, trades: %1").arg(report.totalTrades));

    emit reportGenerated(report);
    return report;
}

ReportData ReportGenerator::generateCustomReport(const QDateTime &start, const QDateTime &end)
{
    ReportData report;
    report.title = QString("自定义报表 - %1 至 %2")
        .arg(start.toString("yyyy-MM-dd"))
        .arg(end.toString("yyyy-MM-dd"));
    report.startTime = start;
    report.endTime = end;

    report.tradeDetails = DataStorage::instance().loadTradeRecords(start, end);

    calculateStatistics(report);
    calculateRiskMetrics(report);
    groupByInstrument(report);

    LOG_INFO(QString("Generated custom report, trades: %1").arg(report.totalTrades));

    emit reportGenerated(report);
    return report;
}

void ReportGenerator::calculateStatistics(ReportData &report)
{
    report.totalTrades = report.tradeDetails.size();
    if (report.totalTrades == 0) {
        return;
    }

    double totalWin = 0.0;
    double totalLoss = 0.0;
    double maxProfit = 0.0;
    double maxLoss = 0.0;
    double totalCommission = 0.0;
    double totalTurnover = 0.0;
    int winCount = 0;
    int lossCount = 0;

    for (const auto &trade : report.tradeDetails) {
        double profit = trade["profit"].toDouble();
        double commission = trade["commission"].toDouble();
        double turnover = trade["turnover"].toDouble();

        totalCommission += commission;
        totalTurnover += turnover;

        if (profit > 0) {
            totalWin += profit;
            winCount++;
            if (profit > maxProfit) {
                maxProfit = profit;
            }
        } else if (profit < 0) {
            totalLoss += qAbs(profit);
            lossCount++;
            if (qAbs(profit) > maxLoss) {
                maxLoss = qAbs(profit);
            }
        }
    }

    report.winTrades = winCount;
    report.lossTrades = lossCount;
    report.winRate = report.totalTrades > 0
        ? static_cast<double>(winCount) / report.totalTrades * 100.0
        : 0.0;

    report.totalProfit = totalWin;
    report.totalLoss = totalLoss;
    report.netProfit = totalWin - totalLoss - totalCommission;
    report.maxProfit = maxProfit;
    report.maxLoss = maxLoss;
    report.totalCommission = totalCommission;
    report.totalTurnover = totalTurnover;

    // 盈亏比
    report.profitFactor = totalLoss > 0 ? totalWin / totalLoss : 0.0;
}

void ReportGenerator::calculateRiskMetrics(ReportData &report)
{
    if (report.tradeDetails.isEmpty()) {
        return;
    }

    // 计算最大回撤
    double equity = 0.0;
    double peak = 0.0;
    double maxDrawdown = 0.0;
    QVector<double> returns;

    for (const auto &trade : report.tradeDetails) {
        double profit = trade["profit"].toDouble() - trade["commission"].toDouble();
        equity += profit;

        if (equity > peak) {
            peak = equity;
        }

        double drawdown = peak > 0 ? (peak - equity) / peak : 0.0;
        if (drawdown > maxDrawdown) {
            maxDrawdown = drawdown;
        }

        returns.append(profit);
    }

    report.maxDrawdown = maxDrawdown * 100.0;

    // 计算夏普比率（简化版）
    if (returns.size() > 1) {
        double meanReturn = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();

        double variance = 0.0;
        for (double r : returns) {
            variance += qPow(r - meanReturn, 2);
        }
        variance /= (returns.size() - 1);

        double stdDev = qSqrt(variance);
        report.sharpeRatio = stdDev > 0 ? meanReturn / stdDev * qSqrt(252.0) : 0.0;  // 年化
    }
}

void ReportGenerator::groupByInstrument(ReportData &report)
{
    QMap<QString, QVariantMap> instrumentStats;

    for (const auto &trade : report.tradeDetails) {
        QString instrumentId = trade["instrumentId"].toString();

        QVariantMap stats = instrumentStats.value(instrumentId);
        if (stats.isEmpty()) {
            stats["instrumentId"] = instrumentId;
            stats["trades"] = 0;
            stats["profit"] = 0.0;
            stats["commission"] = 0.0;
        }

        stats["trades"] = stats["trades"].toInt() + 1;
        stats["profit"] = stats["profit"].toDouble() + trade["profit"].toDouble();
        stats["commission"] = stats["commission"].toDouble() + trade["commission"].toDouble();

        instrumentStats.insert(instrumentId, stats);
    }

    report.instrumentBreakdown = QVariantList(instrumentStats.values().begin(), instrumentStats.values().end());
}

bool ReportGenerator::exportReport(const ReportData &report, const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit reportError(QString("Cannot open file: %1").arg(filePath));
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    out << "========================================\n";
    out << report.title << "\n";
    out << "========================================\n\n";

    out << "时间范围: " << report.startTime.toString("yyyy-MM-dd hh:mm")
        << " 至 " << report.endTime.toString("yyyy-MM-dd hh:mm") << "\n\n";

    out << "【交易统计】\n";
    out << QString("总交易次数: %1\n").arg(report.totalTrades);
    out << QString("盈利次数: %1\n").arg(report.winTrades);
    out << QString("亏损次数: %1\n").arg(report.lossTrades);
    out << QString("胜率: %1%\n").arg(report.winRate, 0, 'f', 2);
    out << "\n";

    out << "【盈亏分析】\n";
    out << QString("总盈利: %1\n").arg(report.totalProfit, 0, 'f', 2);
    out << QString("总亏损: %1\n").arg(report.totalLoss, 0, 'f', 2);
    out << QString("净盈亏: %1\n").arg(report.netProfit, 0, 'f', 2);
    out << QString("最大单笔盈利: %1\n").arg(report.maxProfit, 0, 'f', 2);
    out << QString("最大单笔亏损: %1\n").arg(report.maxLoss, 0, 'f', 2);
    out << QString("盈亏比: %1\n").arg(report.profitFactor, 0, 'f', 2);
    out << "\n";

    out << "【费用统计】\n";
    out << QString("总手续费: %1\n").arg(report.totalCommission, 0, 'f', 2);
    out << QString("总成交额: %1\n").arg(report.totalTurnover, 0, 'f', 2);
    out << "\n";

    out << "【风险指标】\n";
    out << QString("最大回撤: %1%\n").arg(report.maxDrawdown, 0, 'f', 2);
    out << QString("夏普比率: %1\n").arg(report.sharpeRatio, 0, 'f', 2);
    out << "\n";

    out << "========================================\n";
    out << "生成时间: " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "\n";

    file.close();

    LOG_INFO(QString("Report exported to: %1").arg(filePath));
    return true;
}

QVector<QPair<QDateTime, double>> ReportGenerator::getEquityCurve(const QDateTime &start,
                                                                    const QDateTime &end,
                                                                    int intervalMinutes)
{
    QVector<QPair<QDateTime, double>> curve;

    auto trades = DataStorage::instance().loadTradeRecords(start, end);
    if (trades.isEmpty()) {
        return curve;
    }

    // 按时间排序
    std::sort(trades.begin(), trades.end(), [](const QVariantMap &a, const QVariantMap &b) {
        return a["time"].toDateTime() < b["time"].toDateTime();
    });

    double equity = 0.0;
    QDateTime lastTime = start;

    for (const auto &trade : trades) {
        QDateTime tradeTime = trade["time"].toDateTime();
        double profit = trade["profit"].toDouble() - trade["commission"].toDouble();

        // 如果间隔超过指定分钟数，插入中间点
        while (lastTime.secsTo(tradeTime) > intervalMinutes * 60) {
            lastTime = lastTime.addSecs(intervalMinutes * 60);
            curve.append(qMakePair(lastTime, equity));
        }

        equity += profit;
        curve.append(qMakePair(tradeTime, equity));
        lastTime = tradeTime;
    }

    return curve;
}
