/**
 * @file DataExporter.h
 * @brief 数据导出工具 - 支持CSV格式导出
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef DATAEXPORTER_H
#define DATAEXPORTER_H

#include <QObject>
#include <QString>
#include <QStringList>

/**
 * @brief 数据导出工具类
 */
class DataExporter : public QObject
{
    Q_OBJECT

public:
    static DataExporter& instance();

    /**
     * @brief 导出通用表格数据到CSV
     */
    bool exportToCSV(const QStringList& headers,
                     const QVector<QStringList>& data,
                     const QString& filePath);

signals:
    void exportCompleted(const QString& filePath, bool success);

private:
    DataExporter(QObject* parent = nullptr);
    ~DataExporter() override;
    Q_DISABLE_COPY(DataExporter)
};

#endif // DATAEXPORTER_H
