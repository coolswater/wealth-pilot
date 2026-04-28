/**
 * @file DataExporter.cpp
 * @brief 数据导出工具实现 - 支持多种格式导出
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "DataExporter.h"
#include "core/types/MarketTypes.h"
#include "utils/Logger.h"

#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QProgressDialog>
#include <QApplication>

// ========== 构造与析构 ==========

DataExporter::DataExporter(QObject* parent)
    : QObject(parent)
{
}

DataExporter::~DataExporter() = default;

// ========== 单例 ==========

DataExporter& DataExporter::instance()
{
    static DataExporter instance;
    return instance;
}

// ========== K线数据导出 ==========

bool DataExporter::exportKLineData(const QVector<KLineData>& data,
                                   const QString& filePath,
                                   ExportFormat format)
{
    // 构建表头
    QStringList headers = {
        QStringLiteral("日期"),
        QStringLiteral("开盘价"),
        QStringLiteral("最高价"),
        QStringLiteral("最低价"),
        QStringLiteral("收盘价"),
        QStringLiteral("成交量"),
        QStringLiteral("成交额")
    };
    
    // 构建数据
    QVector<QStringList> rows;
    for (const auto& kline : data) {
        QStringList row;
        row << kline.time.toString("yyyy-MM-dd");
        row << QString::number(kline.open, 'f', 2);
        row << QString::number(kline.high, 'f', 2);
        row << QString::number(kline.low, 'f', 2);
        row << QString::number(kline.close, 'f', 2);
        row << QString::number(kline.volume);
        row << QString::number(kline.turnover, 'f', 2);
        rows.append(row);
    }
    
    return exportTableData(headers, rows, filePath, format);
}

// ========== 交易记录导出 ==========

bool DataExporter::exportTradeRecords(const QVector<TradeRecord>& trades,
                                      const QString& filePath,
                                      ExportFormat format)
{
    QStringList headers = {
        QStringLiteral("时间"),
        QStringLiteral("方向"),
        QStringLiteral("价格"),
        QStringLiteral("数量"),
        QStringLiteral("盈亏"),
        QStringLiteral("累计盈亏")
    };
    
    QVector<QStringList> rows;
    for (const auto& trade : trades) {
        QStringList row;
        row << trade.time.toString("yyyy-MM-dd hh:mm:ss");
        row << trade.action;
        row << QString::number(trade.price, 'f', 2);
        row << QString::number(trade.volume);
        row << QString::number(trade.profit, 'f', 2);
        row << QString::number(trade.cumProfit, 'f', 2);
        rows.append(row);
    }
    
    return exportTableData(headers, rows, filePath, format);
}

// ========== 回测报告导出 ==========

bool DataExporter::exportBacktestReport(const BacktestResult& result,
                                        const QVector<TradeRecord>& trades,
                                        const QString& filePath)
{
    // 构建报告标题和内容
    QString title = QStringLiteral("策略回测报告");
    
    QStringList headers = {
        QStringLiteral("指标"),
        QStringLiteral("数值")
    };
    
    QVector<QStringList> rows;
    rows.append({QStringLiteral("总收益率"), QString::number(result.totalReturn, 'f', 2) + "%"});
    rows.append({QStringLiteral("年化收益率"), QString::number(result.annualReturn, 'f', 2) + "%"});
    rows.append({QStringLiteral("最大回撤"), QString::number(result.maxDrawdown, 'f', 2) + "%"});
    rows.append({QStringLiteral("夏普比率"), QString::number(result.sharpeRatio, 'f', 2)});
    rows.append({QStringLiteral("胜率"), QString::number(result.winRate, 'f', 1) + "%"});
    rows.append({QStringLiteral("盈亏比"), QString::number(result.profitFactor, 'f', 2)});
    rows.append({QStringLiteral("总交易次数"), QString::number(result.totalTrades)});
    rows.append({QStringLiteral("盈利次数"), QString::number(result.winTrades)});
    rows.append({QStringLiteral("亏损次数"), QString::number(result.lossTrades)});
    rows.append({QStringLiteral("单笔最大盈利"), QString::number(result.maxProfit, 'f', 2)});
    rows.append({QStringLiteral("单笔最大亏损"), QString::number(result.maxLoss, 'f', 2)});
    
    // 导出PDF
    return exportToPDF(title, headers, rows, filePath);
}

// ========== 预警记录导出 ==========

bool DataExporter::exportAlertRecords(const QVector<AlertRecord>& alerts,
                                      const QString& filePath,
                                      ExportFormat format)
{
    QStringList headers = {
        QStringLiteral("标的代码"),
        QStringLiteral("标的名称"),
        QStringLiteral("预警类型"),
        QStringLiteral("阈值"),
        QStringLiteral("实际值"),
        QStringLiteral("触发时间"),
        QStringLiteral("消息")
    };
    
    QVector<QStringList> rows;
    for (const auto& alert : alerts) {
        QStringList row;
        row << alert.symbol;
        row << alert.name;
        // row << formatAlertType(alert.type);
        row << QString::number(alert.threshold);
        row << QString::number(alert.actualValue);
        row << alert.triggerTime.toString("yyyy-MM-dd hh:mm:ss");
        row << alert.message;
        rows.append(row);
    }
    
    return exportTableData(headers, rows, filePath, format);
}

// ========== 通用表格数据导出 ==========

bool DataExporter::exportTableData(const QStringList& headers,
                                   const QVector<QStringList>& data,
                                   const QString& filePath,
                                   ExportFormat format)
{
    bool success = false;
    
    switch (format) {
        case ExportFormat::CSV:
            success = exportToCSV(headers, data, filePath);
            break;
        case ExportFormat::Excel:
            success = exportToExcel(headers, data, filePath);
            break;
        case ExportFormat::PDF:
            success = exportToPDF(QStringLiteral("数据导出"), headers, data, filePath);
            break;
        case ExportFormat::JSON:
            success = exportToJSON(headers, data, filePath);
            break;
    }
    
    emit exportCompleted(filePath, success);
    return success;
}

// ========== CSV导出 ==========

bool DataExporter::exportToCSV(const QStringList& headers,
                               const QVector<QStringList>& data,
                               const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        LOG_ERROR(QString("Failed to open file: %1").arg(filePath));
        return false;
    }
    
    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    
    // 写入表头
    stream << headers.join(",") << "\n";
    
    // 写入数据
    for (int i = 0; i < data.size(); ++i) {
        stream << data[i].join(",") << "\n";
        emit exportProgress(i + 1, data.size());
    }
    
    file.close();
    LOG_INFO(QString("CSV export completed: %1").arg(filePath));
    return true;
}

// ========== Excel导出 ==========

bool DataExporter::exportToExcel(const QStringList& headers,
                                 const QVector<QStringList>& data,
                                 const QString& filePath)
{
    // 简化实现：使用CSV格式，但以.xlsx扩展名保存
    // 完整实现需要使用QXlsx库
    
    QString csvPath = filePath;
    csvPath.replace(".xlsx", ".csv");
    
    LOG_INFO(QStringLiteral("Excel export: using CSV format as fallback"));
    return exportToCSV(headers, data, csvPath);
}

// ========== PDF导出 ==========

bool DataExporter::exportToPDF(const QString& title,
                               const QStringList& headers,
                               const QVector<QStringList>& data,
                               const QString& filePath)
{
    // 简化实现：导出为文本文件
    // 完整实现需要使用QPdfWriter或第三方库
    
    QString txtPath = filePath;
    txtPath.replace(".pdf", ".txt");
    
    QFile file(txtPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        LOG_ERROR(QString("Failed to open file: %1").arg(txtPath));
        return false;
    }
    
    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    
    // 写入标题
    stream << "========================================\n";
    stream << title << "\n";
    stream << "========================================\n\n";
    
    // 写入表头
    stream << headers.join("\t") << "\n";
    stream << "----------------------------------------\n";
    
    // 写入数据
    for (const auto& row : data) {
        stream << row.join("\t") << "\n";
    }
    
    file.close();
    LOG_INFO(QString("PDF export (text fallback) completed: %1").arg(txtPath));
    return true;
}

// ========== JSON导出 ==========

bool DataExporter::exportToJSON(const QStringList& headers,
                                const QVector<QStringList>& data,
                                const QString& filePath)
{
    QJsonArray jsonArray;
    
    for (const auto& row : data) {
        QJsonObject obj;
        for (int i = 0; i < headers.size() && i < row.size(); ++i) {
            obj[headers[i]] = row[i];
        }
        jsonArray.append(obj);
    }
    
    QJsonDocument doc(jsonArray);
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        LOG_ERROR(QString("Failed to open file: %1").arg(filePath));
        return false;
    }
    
    file.write(doc.toJson());
    file.close();
    
    LOG_INFO(QString("JSON export completed: %1").arg(filePath));
    return true;
}
