/**
 * @file ExcelExporter.h
 * @brief Excel导出器 - 数据导出到Excel文件
 *
 * @details 功能：
 * - 导出表格数据到Excel
 * - 支持CSV格式（无需第三方库）
 * - 支持自定义样式
 * - 支持多工作表
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef EXCELEXPORTER_H
#define EXCELEXPORTER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QVariant>
#include <QAbstractItemModel>

/**
 * @brief Excel导出器
 */
class ExcelExporter : public QObject
{
    Q_OBJECT

public:
    explicit ExcelExporter(QObject *parent = nullptr);
    ~ExcelExporter() override;

    /**
     * @brief 导出模型数据到CSV
     * @param model 数据模型
     * @param filePath 文件路径
     * @param includeHeaders 是否包含表头
     * @return 是否成功
     */
    bool exportToCsv(QAbstractItemModel *model, const QString &filePath,
                     bool includeHeaders = true);

    /**
     * @brief 导出自定义数据到CSV
     * @param headers 表头
     * @param data 数据（每行一个QStringList）
     * @param filePath 文件路径
     * @return 是否成功
     */
    bool exportToCsv(const QStringList &headers,
                     const QVector<QStringList> &data,
                     const QString &filePath);

    /**
     * @brief 导出交易记录
     * @param trades 交易记录数据
     * @param filePath 文件路径
     * @return 是否成功
     */
    bool exportTradeRecords(const QVector<QVariantMap> &trades,
                            const QString &filePath);

    /**
     * @brief 导出持仓数据
     * @param positions 持仓数据
     * @param filePath 文件路径
     * @return 是否成功
     */
    bool exportPositions(const QVector<QVariantMap> &positions,
                         const QString &filePath);

    /**
     * @brief 导出账户资金
     * @param accountInfo 账户信息
     * @param filePath 文件路径
     * @return 是否成功
     */
    bool exportAccountInfo(const QVariantMap &accountInfo,
                           const QString &filePath);

    /**
     * @brief 设置CSV分隔符
     */
    void setSeparator(const QString &sep) { m_separator = sep; }

    /**
     * @brief 设置编码
     */
    void setEncoding(const QString &encoding) { m_encoding = encoding; }

signals:
    void exportProgress(int current, int total);
    void exportFinished(const QString &filePath, bool success);
    void exportError(const QString &error);

private:
    QString escapeCsvField(const QString &field) const;
    bool writeToFile(const QString &content, const QString &filePath);

    QString m_separator = ",";
    QString m_encoding = "UTF-8";
};

#endif // EXCELEXPORTER_H
