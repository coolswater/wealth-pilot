/**
 * @file ExcelExporter.cpp
 * @brief Excel导出器实现
 */

#include "ExcelExporter.h"
#include "Logger.h"

#include <QFile>
#include <QTextStream>
#include <QTextCodec>
#include <QDateTime>

ExcelExporter::ExcelExporter(QObject *parent)
    : QObject(parent)
{
    LOG_DEBUG("ExcelExporter created");
}

ExcelExporter::~ExcelExporter()
{
    LOG_DEBUG("ExcelExporter destroyed");
}

bool ExcelExporter::exportToCsv(QAbstractItemModel *model, const QString &filePath,
                                 bool includeHeaders)
{
    if (!model || filePath.isEmpty()) {
        emit exportError("Invalid model or file path");
        return false;
    }

    QStringList lines;
    int rowCount = model->rowCount();
    int colCount = model->columnCount();

    // 添加表头
    if (includeHeaders) {
        QStringList headers;
        for (int col = 0; col < colCount; ++col) {
            headers << escapeCsvField(model->headerData(col, Qt::Horizontal).toString());
        }
        lines << headers.join(m_separator);
    }

    // 添加数据
    for (int row = 0; row < rowCount; ++row) {
        QStringList rowData;
        for (int col = 0; col < colCount; ++col) {
            QModelIndex index = model->index(row, col);
            rowData << escapeCsvField(model->data(index).toString());
        }
        lines << rowData.join(m_separator);

        emit exportProgress(row + 1, rowCount);
    }

    QString content = lines.join("\n");
    bool success = writeToFile(content, filePath);

    emit exportFinished(filePath, success);
    return success;
}

bool ExcelExporter::exportToCsv(const QStringList &headers,
                                 const QVector<QStringList> &data,
                                 const QString &filePath)
{
    if (filePath.isEmpty()) {
        emit exportError("Invalid file path");
        return false;
    }

    QStringList lines;

    // 添加表头
    if (!headers.isEmpty()) {
        QStringList escapedHeaders;
        for (const QString &h : headers) {
            escapedHeaders << escapeCsvField(h);
        }
        lines << escapedHeaders.join(m_separator);
    }

    // 添加数据
    int total = data.size();
    for (int i = 0; i < total; ++i) {
        QStringList escapedRow;
        for (const QString &field : data[i]) {
            escapedRow << escapeCsvField(field);
        }
        lines << escapedRow.join(m_separator);

        emit exportProgress(i + 1, total);
    }

    QString content = lines.join("\n");
    bool success = writeToFile(content, filePath);

    emit exportFinished(filePath, success);
    return success;
}

bool ExcelExporter::exportTradeRecords(const QVector<QVariantMap> &trades,
                                        const QString &filePath)
{
    if (trades.isEmpty()) {
        emit exportError("No trade records to export");
        return false;
    }

    QStringList headers;
    headers << "成交ID" << "订单ID" << "合约代码" << "买卖方向"
            << "开平标志" << "成交价格" << "成交数量" << "成交金额"
            << "成交时间" << "手续费";

    QVector<QStringList> data;
    for (const auto &trade : trades) {
        QStringList row;
        row << trade["tradeId"].toString();
        row << trade["orderId"].toString();
        row << trade["instrumentId"].toString();
        row << trade["direction"].toString();
        row << trade["offsetFlag"].toString();
        row << QString::number(trade["price"].toDouble(), 'f', 2);
        row << QString::number(trade["volume"].toInt());
        row << QString::number(trade["turnover"].toDouble(), 'f', 2);
        row << trade["tradeTime"].toDateTime().toString("yyyy-MM-dd hh:mm:ss");
        row << QString::number(trade["commission"].toDouble(), 'f', 2);
        data << row;
    }

    return exportToCsv(headers, data, filePath);
}

bool ExcelExporter::exportPositions(const QVector<QVariantMap> &positions,
                                     const QString &filePath)
{
    if (positions.isEmpty()) {
        emit exportError("No positions to export");
        return false;
    }

    QStringList headers;
    headers << "合约代码" << "合约名称" << "多持仓" << "空持仓"
            << "多头成本" << "空头成本" << "今多" << "今空"
            << "多头盈亏" << "空头盈亏" << "总盈亏"
            << "保证金" << "更新时间";

    QVector<QStringList> data;
    for (const auto &pos : positions) {
        QStringList row;
        row << pos["instrumentId"].toString();
        row << pos["instrumentName"].toString();
        row << QString::number(pos["longPosition"].toInt());
        row << QString::number(pos["shortPosition"].toInt());
        row << QString::number(pos["longCost"].toDouble(), 'f', 2);
        row << QString::number(pos["shortCost"].toDouble(), 'f', 2);
        row << QString::number(pos["longToday"].toInt());
        row << QString::number(pos["shortToday"].toInt());
        row << QString::number(pos["longProfit"].toDouble(), 'f', 2);
        row << QString::number(pos["shortProfit"].toDouble(), 'f', 2);
        row << QString::number(pos["totalProfit"].toDouble(), 'f', 2);
        row << QString::number(pos["margin"].toDouble(), 'f', 2);
        row << pos["updateTime"].toDateTime().toString("yyyy-MM-dd hh:mm:ss");
        data << row;
    }

    return exportToCsv(headers, data, filePath);
}

bool ExcelExporter::exportAccountInfo(const QVariantMap &accountInfo,
                                       const QString &filePath)
{
    if (accountInfo.isEmpty()) {
        emit exportError("No account info to export");
        return false;
    }

    QStringList headers;
    headers << "项目" << "金额";

    QVector<QStringList> data;
    data << QStringList{"总资产", QString::number(accountInfo["balance"].toDouble(), 'f', 2)};
    data << QStringList{"可用资金", QString::number(accountInfo["available"].toDouble(), 'f', 2)};
    data << QStringList{"保证金占用", QString::number(accountInfo["margin"].toDouble(), 'f', 2)};
    data << QStringList{"冻结保证金", QString::number(accountInfo["frozenMargin"].toDouble(), 'f', 2)};
    data << QStringList{"浮动盈亏", QString::number(accountInfo["floatingProfit"].toDouble(), 'f', 2)};
    data << QStringList{"当日盈亏", QString::number(accountInfo["todayProfit"].toDouble(), 'f', 2)};
    data << QStringList{"导出时间", QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")};

    return exportToCsv(headers, data, filePath);
}

QString ExcelExporter::escapeCsvField(const QString &field) const
{
    // 如果包含分隔符、引号或换行，需要用引号包围
    if (field.contains(m_separator) || field.contains("\"") || field.contains("\n")) {
        QString escaped = field;
        escaped.replace("\"", "\"\"");  // 引号转义
        return QString("\"%1\"").arg(escaped);
    }
    return field;
}

bool ExcelExporter::writeToFile(const QString &content, const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QString error = QString("Cannot open file: %1").arg(filePath);
        LOG_ERROR(error);
        emit exportError(error);
        return false;
    }

    QTextStream out(&file);

    // 设置编码
    if (m_encoding.toUpper() == "UTF-8") {
        out.setEncoding(QStringConverter::Utf8);
        // 添加BOM以便Excel正确识别UTF-8
        file.write("\xEF\xBB\xBF");
    } else {
        out.setEncoding(QStringConverter::Latin1);
    }

    out << content;
    file.close();

    LOG_INFO(QString("Exported to: %1").arg(filePath));
    return true;
}
