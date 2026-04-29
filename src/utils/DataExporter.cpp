/**
 * @file DataExporter.cpp
 * @brief 数据导出工具实现
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "DataExporter.h"
#include "utils/Logger.h"

#include <QFile>
#include <QTextStream>

DataExporter::DataExporter(QObject* parent)
    : QObject(parent)
{
}

DataExporter::~DataExporter() = default;

DataExporter& DataExporter::instance()
{
    static DataExporter instance;
    return instance;
}

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
    
    // 写入UTF-8 BOM（可选，帮助Excel识别编码）
    stream << "\xEF\xBB\xBF";
    
    // 写入表头
    stream << headers.join(",") << "\n";
    
    // 写入数据
    for (const auto& row : data) {
        stream << row.join(",") << "\n";
    }
    
    file.close();
    
    LOG_INFO(QString("CSV export completed: %1").arg(filePath));
    emit exportCompleted(filePath, true);
    return true;
}
