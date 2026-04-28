/**
 * @file DataExporter.h
 * @brief 数据导出工具 - 支持多种格式导出
 *
 * @details 功能：
 * - 导出K线数据（CSV、Excel）
 * - 导出交易记录
 * - 导出回测报告（PDF）
 * - 导出预警记录
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef DATAEXPORTER_H
#define DATAEXPORTER_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QDateTime>

// 前向声明
struct KLineData;
struct TradeRecord;
struct BacktestResult;
struct AlertRecord;

/**
 * @brief 导出格式枚举
 */
enum class ExportFormat {
    CSV,        ///< CSV格式
    Excel,      ///< Excel格式
    PDF,        ///< PDF格式
    JSON        ///< JSON格式
};

/**
 * @brief 数据导出工具类
 */
class DataExporter : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 获取单例实例
     */
    static DataExporter& instance();

    /**
     * @brief 导出K线数据
     * @param data K线数据
     * @param filePath 文件路径
     * @param format 导出格式
     * @return 是否成功
     */
    bool exportKLineData(const QVector<KLineData>& data, 
                         const QString& filePath,
                         ExportFormat format = ExportFormat::CSV);

    /**
     * @brief 导出交易记录
     * @param trades 交易记录
     * @param filePath 文件路径
     * @param format 导出格式
     * @return 是否成功
     */
    bool exportTradeRecords(const QVector<TradeRecord>& trades,
                            const QString& filePath,
                            ExportFormat format = ExportFormat::CSV);

    /**
     * @brief 导出回测报告
     * @param result 回测结果
     * @param trades 交易记录
     * @param filePath 文件路径
     * @return 是否成功
     */
    bool exportBacktestReport(const BacktestResult& result,
                              const QVector<TradeRecord>& trades,
                              const QString& filePath);

    /**
     * @brief 导出预警记录
     * @param alerts 预警记录
     * @param filePath 文件路径
     * @param format 导出格式
     * @return 是否成功
     */
    bool exportAlertRecords(const QVector<AlertRecord>& alerts,
                            const QString& filePath,
                            ExportFormat format = ExportFormat::CSV);

    /**
     * @brief 导出通用表格数据
     * @param headers 表头
     * @param data 数据（每行是一个字符串列表）
     * @param filePath 文件路径
     * @param format 导出格式
     * @return 是否成功
     */
    bool exportTableData(const QStringList& headers,
                         const QVector<QStringList>& data,
                         const QString& filePath,
                         ExportFormat format = ExportFormat::CSV);

signals:
    /**
     * @brief 导出完成信号
     */
    void exportCompleted(const QString& filePath, bool success);

    /**
     * @brief 导出进度信号
     */
    void exportProgress(int current, int total);

private:
    DataExporter(QObject* parent = nullptr);
    ~DataExporter() override;
    Q_DISABLE_COPY(DataExporter)

    // 内部导出方法
    bool exportToCSV(const QStringList& headers, 
                     const QVector<QStringList>& data,
                     const QString& filePath);
    
    bool exportToExcel(const QStringList& headers,
                       const QVector<QStringList>& data,
                       const QString& filePath);
    
    bool exportToPDF(const QString& title,
                     const QStringList& headers,
                     const QVector<QStringList>& data,
                     const QString& filePath);
    
    bool exportToJSON(const QStringList& headers,
                      const QVector<QStringList>& data,
                      const QString& filePath);
};

#endif // DATAEXPORTER_H