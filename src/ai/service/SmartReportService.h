/**
 * @file SmartReportService.h
 * @brief 智能报告服务
 *
 * @details 功能：
 * - 报告模板管理
 * - 数据分析引擎
 * - 报告自动生成
 * - 报告导出（PDF/Word/HTML）
 * - 多种报告类型支持
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef SMARTREPORTSERVICE_H
#define SMARTREPORTSERVICE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>
#include <QMap>
#include <QList>
#include <QDateTime>
#include <functional>

namespace WealthPilot
{
    /**
 * @brief 报告类型枚举
 */
    enum class ReportType
    {
        DailyBrief, ///< 日报 - 每日市场概况
        WeeklySummary, ///< 周报 - 本周市场总结
        MonthlyReview, ///< 月报 - 本月投资回顾
        StockAnalysis, ///< 个股分析报告
        PortfolioReview, ///< 组合回顾报告
        MarketOutlook, ///< 市场展望报告
        StrategyReport, ///< 策略分析报告
        RiskReport, ///< 风险评估报告
        Custom ///< 自定义报告
    };

    /**
 * @brief 报告章节
 */
    struct ReportSection
    {
        QString id; ///< 章节 ID
        QString title; ///< 章节标题
        QString content; ///< 章节内容
        int order = 0; ///< 排序
        QString type; ///< 章节类型（text/chart/table）

        static ReportSection fromJson(const QJsonObject& json);
        QJsonObject toJson() const;
    };

    /**
 * @brief 报告模板
 */
    struct ReportTemplate
    {
        QString id; ///< 模板 ID
        QString name; ///< 模板名称
        ReportType type; ///< 报告类型
        QString description; ///< 模板描述
        QList<ReportSection> sections; ///< 章节列表
        QStringList requiredData; ///< 所需数据
        bool isDefault = false; ///< 是否默认模板
        QDateTime createdAt; ///< 创建时间
        QDateTime updatedAt; ///< 更新时间

        static ReportTemplate fromJson(const QJsonObject& json);
        QJsonObject toJson() const;
    };

    /**
 * @brief 报告参数
 */
    struct ReportParams
    {
        QString stockCode; ///< 股票代码（个股分析）
        QStringList portfolioStocks; ///< 组合股票列表
        QDateTime startDate; ///< 开始日期
        QDateTime endDate; ///< 结束日期
        QMap<QString, QVariant> customParams; ///< 自定义参数

        static ReportParams fromJson(const QJsonObject& json);
        QJsonObject toJson() const;
    };

    /**
 * @brief 报告结构
 */
    struct Report
    {
        QString id; ///< 报告 ID
        QString title; ///< 报告标题
        ReportType type; ///< 报告类型
        QString templateId; ///< 模板 ID
        ReportParams params; ///< 报告参数
        QList<ReportSection> sections; ///< 章节列表
        QString summary; ///< 报告摘要
        QStringList keyPoints; ///< 关键要点
        QStringList recommendations; ///< 投资建议
        double confidence = 0; ///< 置信度
        QDateTime generatedAt; ///< 生成时间
        QString author; ///< 作者（AI 或用户）

        static Report fromJson(const QJsonObject& json);
        QJsonObject toJson() const;
    };

    /**
 * @brief 智能报告服务
 *
 * @details 提供智能报告生成功能：
 * - 报告模板管理
 * - 数据分析引擎
 * - 报告自动生成
 * - 报告导出
 */
    class SmartReportService : public QObject
    {
        Q_OBJECT

    public:
        explicit SmartReportService(QObject* parent = nullptr);
        ~SmartReportService() override;

        // ========== 模板管理 ==========

        /**
     * @brief 获取报告模板列表
     */
        QList<ReportTemplate> getTemplates() const;

        /**
     * @brief 获取指定类型的模板
     */
        ReportTemplate getTemplate(ReportType type) const;

        /**
     * @brief 获取模板
     */
        ReportTemplate getTemplateById(const QString& templateId) const;

        /**
     * @brief 创建自定义模板
     */
        QString createTemplate(const ReportTemplate& tmpl);

        /**
     * @brief 更新模板
     */
        bool updateTemplate(const ReportTemplate& tmpl);

        /**
     * @brief 删除模板
     */
        bool deleteTemplate(const QString& templateId);

        // ========== 报告生成 ==========

        /**
     * @brief 生成报告
     */
        void generateReport(ReportType type,
                            const ReportParams& params,
                            std::function<void(const Report&)> callback);

        /**
     * @brief 生成个股分析报告
     */
        void generateStockAnalysis(const QString& stockCode,
                                   std::function<void(const Report&)> callback);

        /**
     * @brief 生成组合回顾报告
     */
        void generatePortfolioReview(const QStringList& stocks,
                                     std::function<void(const Report&)> callback);

        /**
     * @brief 生成市场展望报告
     */
        void generateMarketOutlook(std::function<void(const Report&)> callback);

        /**
     * @brief 生成日报
     */
        void generateDailyBrief(std::function<void(const Report&)> callback);

        // ========== 报告管理 ==========

        /**
     * @brief 获取报告列表
     */
        QList<Report> listReports() const;

        /**
     * @brief 获取报告
     */
        Report getReport(const QString& reportId) const;

        /**
     * @brief 删除报告
     */
        bool deleteReport(const QString& reportId);

        // ========== 报告导出 ==========

        /**
     * @brief 导出报告为文本
     */
        QString exportToText(const Report& report) const;

        /**
     * @brief 导出报告为 Markdown
     */
        QString exportToMarkdown(const Report& report) const;

        /**
     * @brief 导出报告为 HTML
     */
        QString exportToHtml(const Report& report) const;

        /**
     * @brief 导出报告到文件
     */
        bool exportToFile(const Report& report,
                          const QString& filePath,
                          const QString& format = "markdown");

        // ========== 工具方法 ==========

        static QString getReportTypeName(ReportType type);
        static QString getReportTypeDescription(ReportType type);

        signals :

        void reportGenerated(const Report& report);
        void reportExported(const QString& filePath);
        void errorOccurred(const QString& error);

    private:
        void initializeDefaultTemplates();
        void analyzeDataForReport(ReportType type,
                                  const ReportParams& params,
                                  std::function<void(const QJsonObject &)> callback);
        void generateSectionsWithAI(const ReportTemplate& tmpl,
                                    const QJsonObject& data,
                                    std::function<void(const QList<ReportSection> &)> callback);
        QString generateReportId() const;
        void saveReport(const Report& report);

    private:
        QMap<QString, ReportTemplate> m_templates;
        QMap<QString, Report> m_reports;
        QString m_storagePath;
    };
} // namespace WealthPilot

#endif // SMARTREPORTSERVICE_H