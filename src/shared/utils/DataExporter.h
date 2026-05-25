/**
 * @file DataExporter.h
 * @brief 数据导出工具类
 *
 * @details 实现功能：
 * - 导出为 CSV 格式
 * - 导出为 Excel 格式（需要 QtXlsx）
 * - 导出为 JSON 格式
 * - 批量数据导出
 * - 自定义列选择
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef DATAEXPORTER_H
#define DATAEXPORTER_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QVariantMap>
#include <memory>

/**
 * @brief 导出格式
 */
enum class ExportFormat
{
    CSV, ///< CSV 格式
    Excel, ///< Excel 格式
    JSON, ///< JSON 格式
    HTML ///< HTML 表格格式
};

/**
 * @brief 导出列配置
 */
struct ExportColumn
{
    QString key; ///< 数据键名
    QString header; ///< 表头名称
    QString format; ///< 格式化字符串（可选）
    int width = 100; ///< 列宽（Excel用）
};

/**
 * @brief 导出结果
 */
struct ExportResult
{
    bool success = false;
    QString filePath;
    QString errorString;
    int rowCount = 0;
};

/**
 * @brief 数据导出器
 */
class DataExporter : public QObject
{
    Q_OBJECT

public:
    explicit DataExporter(QObject* parent = nullptr);
    ~DataExporter() override;

    // 导出数据
    ExportResult exportToFile(const QString& filePath,
                              const QVector<QVariantMap>& data,
                              const QVector<ExportColumn>& columns,
                              ExportFormat format = ExportFormat::CSV);

    // 便捷导出方法
    ExportResult exportToCSV(const QString& filePath,
                             const QVector<QVariantMap>& data,
                             const QVector<ExportColumn>& columns);

    ExportResult exportToJSON(const QString& filePath,
                              const QVector<QVariantMap>& data,
                              const QVector<ExportColumn>& columns);

    ExportResult exportToHTML(const QString& filePath,
                              const QVector<QVariantMap>& data,
                              const QVector<ExportColumn>& columns);

    // 工具方法
    static QString formatValue(const QVariant& value, const QString& format);
    static QString suggestFileName(const QString& baseName, ExportFormat format);

private:
    bool writeCSV(const QString& filePath,
                  const QVector<QVariantMap>& data,
                  const QVector<ExportColumn>& columns);

    bool writeJSON(const QString& filePath,
                   const QVector<QVariantMap>& data,
                   const QVector<ExportColumn>& columns);

    bool writeHTML(const QString& filePath,
                   const QVector<QVariantMap>& data,
                   const QVector<ExportColumn>& columns);
};

#endif // DATAEXPORTER_H
