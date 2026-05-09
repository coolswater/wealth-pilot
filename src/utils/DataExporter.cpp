/**
 * @file DataExporter.cpp
 * @brief 数据导出工具实现
 */

#include "DataExporter.h"
#include "Logger.h"
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>
#include <QStringConverter>

// ========== 构造函数和析构函数 ==========

DataExporter::DataExporter(QObject* parent)
    : QObject(parent)
{
}

DataExporter::~DataExporter() = default;

// ========== 公共方法 ==========

ExportResult DataExporter::exportToFile(const QString& filePath,
                                        const QVector<QVariantMap>& data,
                                        const QVector<ExportColumn>& columns,
                                        ExportFormat format)
{
    ExportResult result;

    if (data.isEmpty())
    {
        result.errorString = "No data to export";
        return result;
    }

    if (columns.isEmpty())
    {
        result.errorString = "No columns defined";
        return result;
    }

    bool success = false;

    switch (format)
    {
    case ExportFormat::CSV:
        success = writeCSV(filePath, data, columns);
        break;
    case ExportFormat::JSON:
        success = writeJSON(filePath, data, columns);
        break;
    case ExportFormat::HTML:
        success = writeHTML(filePath, data, columns);
        break;
    case ExportFormat::Excel:
        // Excel 需要额外的库支持，暂时用 CSV 替代
        success = writeCSV(filePath, data, columns);
        break;
    }

    if (success)
    {
        result.success = true;
        result.filePath = filePath;
        result.rowCount = data.size();
        LOG_INFO(QString("Exported %1 rows to %2").arg(data.size()).arg(filePath));
    }
    else
    {
        result.errorString = "Failed to write file";
    }

    return result;
}

ExportResult DataExporter::exportToCSV(const QString& filePath,
                                       const QVector<QVariantMap>& data,
                                       const QVector<ExportColumn>& columns)
{
    return exportToFile(filePath, data, columns, ExportFormat::CSV);
}

ExportResult DataExporter::exportToJSON(const QString& filePath,
                                        const QVector<QVariantMap>& data,
                                        const QVector<ExportColumn>& columns)
{
    return exportToFile(filePath, data, columns, ExportFormat::JSON);
}

ExportResult DataExporter::exportToHTML(const QString& filePath,
                                        const QVector<QVariantMap>& data,
                                        const QVector<ExportColumn>& columns)
{
    return exportToFile(filePath, data, columns, ExportFormat::HTML);
}

// ========== 工具方法 ==========

QString DataExporter::formatValue(const QVariant& value, const QString& format)
{
    if (value.isNull() || !value.isValid())
    {
        return "";
    }

    if (format.isEmpty())
    {
        return value.toString();
    }

    // 根据格式化字符串处理
    if (format.startsWith("fixed:"))
    {
        int decimals = format.mid(6).toInt();
        return QString::number(value.toDouble(), 'f', decimals);
    }
    else if (format == "percent")
    {
        return QString::number(value.toDouble() * 100, 'f', 2) + "%";
    }
    else if (format == "money")
    {
        double amount = value.toDouble();
        if (qAbs(amount) >= 100000000)
        {
            return QString::number(amount / 100000000, 'f', 2) + "亿";
        }
        else if (qAbs(amount) >= 10000)
        {
            return QString::number(amount / 10000, 'f', 2) + "万";
        }
        else
        {
            return QString::number(amount, 'f', 2);
        }
    }
    else if (format == "datetime")
    {
        return QDateTime::fromMSecsSinceEpoch(value.toLongLong()).toString("yyyy-MM-dd HH:mm:ss");
    }
    else if (format == "date")
    {
        return QDateTime::fromMSecsSinceEpoch(value.toLongLong()).toString("yyyy-MM-dd");
    }
    else if (format == "time")
    {
        return QDateTime::fromMSecsSinceEpoch(value.toLongLong()).toString("HH:mm:ss");
    }

    return value.toString();
}

QString DataExporter::suggestFileName(const QString& baseName, ExportFormat format)
{
    QString ext;
    switch (format)
    {
    case ExportFormat::CSV: ext = "csv";
        break;
    case ExportFormat::Excel: ext = "xlsx";
        break;
    case ExportFormat::JSON: ext = "json";
        break;
    case ExportFormat::HTML: ext = "html";
        break;
    }

    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    return QString("%1_%2.%3").arg(baseName, timestamp, ext);
}

// ========== 私有方法 ==========

bool DataExporter::writeCSV(const QString& filePath,
                            const QVector<QVariantMap>& data,
                            const QVector<ExportColumn>& columns)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        LOG_ERROR(QString("Failed to open file: %1").arg(filePath));
        return false;
    }

    QTextStream out(&file);
    // Qt6 使用 setEncoding 替代 setCodec
    out.setEncoding(QStringConverter::Utf8);
    out.setGenerateByteOrderMark(true); // 添加 BOM，Excel 识别 UTF-8

    // 写入表头
    QStringList headers;
    for (const auto& col : columns)
    {
        headers << col.header;
    }
    out << headers.join(",") << "\n";

    // 写入数据
    for (const auto& row : data) {
        QStringList values;
        for (const auto& col : columns)
        {
            QString value = formatValue(row.value(col.key), col.format);
            // CSV 转义：包含逗号或引号时用引号包裹
            if (value.contains(',') || value.contains('"') || value.contains('\n'))
            {
                value.replace('"', "\"\"");
                value = QString("\"%1\"").arg(value);
            }
            values << value;
        }
        out << values.join(",") << "\n";
    }

    file.close();
    return true;
}

bool DataExporter::writeJSON(const QString& filePath,
                             const QVector<QVariantMap>& data,
                             const QVector<ExportColumn>& columns)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly))
    {
        LOG_ERROR(QString("Failed to open file: %1").arg(filePath));
        return false;
    }

    QJsonArray jsonArray;

    for (const auto& row : data)
    {
        QJsonObject obj;
        for (const auto& col : columns)
        {
            QString value = formatValue(row.value(col.key), col.format);
            obj[col.header] = value;
        }
        jsonArray.append(obj);
    }

    QJsonDocument doc(jsonArray);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    return true;
}

bool DataExporter::writeHTML(const QString& filePath,
                             const QVector<QVariantMap>& data,
                             const QVector<ExportColumn>& columns)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        LOG_ERROR(QString("Failed to open file: %1").arg(filePath));
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    // HTML 头部
    out << "<!DOCTYPE html>\n";
    out << "<html>\n<head>\n";
    out << "<meta charset=\"UTF-8\">\n";
    out << "<title>Data Export</title>\n";
    out << "<style>\n";
    out << "body { font-family: Arial, sans-serif; margin: 20px; }\n";
    out << "table { border-collapse: collapse; width: 100%; }\n";
    out << "th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }\n";
    out << "th { background-color: #3B82F6; color: white; }\n";
    out << "tr:nth-child(even) { background-color: #f2f2f2; }\n";
    out << "tr:hover { background-color: #ddd; }\n";
    out << "</style>\n";
    out << "</head>\n<body>\n";

    // 表格
    out << "<table>\n";

    // 表头
    out << "<tr>\n";
    for (const auto& col : columns)
    {
        out << QString("<th>%1</th>\n").arg(col.header);
    }
    out << "</tr>\n";

    // 数据行
    for (const auto& row : data)
    {
        out << "<tr>\n";
        for (const auto& col : columns)
        {
            QString value = formatValue(row.value(col.key), col.format);
            out << QString("<td>%1</td>\n").arg(value);
        }
        out << "</tr>\n";
    }

    out << "</table>\n";
    out << "</body>\n</html>\n";

    file.close();
    return true;
}
