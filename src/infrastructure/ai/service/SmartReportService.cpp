/**
 * @file SmartReportService.cpp
 * @brief 智能报告服务实现
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "SmartReportService.h"
#include "AIService.h"
#include "shared/utils/Logger.h"

#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QUuid>
#include <QDateTime>

namespace WealthPilot
{
    // ============================================================================
    // 数据结构实现
    // ============================================================================

    ReportSection ReportSection::fromJson(const QJsonObject& json)
    {
        ReportSection section;
        section.id = json["id"].toString();
        section.title = json["title"].toString();
        section.content = json["content"].toString();
        section.order = json["order"].toInt();
        section.type = json["type"].toString();
        return section;
    }

    QJsonObject ReportSection::toJson() const
    {
        QJsonObject json;
        json["id"] = id;
        json["title"] = title;
        json["content"] = content;
        json["order"] = order;
        json["type"] = type;
        return json;
    }

    ReportTemplate ReportTemplate::fromJson(const QJsonObject& json)
    {
        ReportTemplate tmpl;
        tmpl.id = json["id"].toString();
        tmpl.name = json["name"].toString();
        tmpl.type = static_cast<ReportType>(json["type"].toInt());
        tmpl.description = json["description"].toString();
        tmpl.isDefault = json["isDefault"].toBool();
        tmpl.createdAt = QDateTime::fromString(json["createdAt"].toString(), Qt::ISODate);
        tmpl.updatedAt = QDateTime::fromString(json["updatedAt"].toString(), Qt::ISODate);

        QJsonArray sectionsArray = json["sections"].toArray();
        for (const auto& s : sectionsArray)
        {
            tmpl.sections.append(ReportSection::fromJson(s.toObject()));
        }

        QJsonArray dataArray = json["requiredData"].toArray();
        for (const auto& d : dataArray)
        {
            tmpl.requiredData.append(d.toString());
        }

        return tmpl;
    }

    QJsonObject ReportTemplate::toJson() const
    {
        QJsonObject json;
        json["id"] = id;
        json["name"] = name;
        json["type"] = static_cast<int>(type);
        json["description"] = description;
        json["isDefault"] = isDefault;
        json["createdAt"] = createdAt.toString(Qt::ISODate);
        json["updatedAt"] = updatedAt.toString(Qt::ISODate);

        QJsonArray sectionsArray;
        for (const auto& s : sections)
        {
            sectionsArray.append(s.toJson());
        }
        json["sections"] = sectionsArray;

        QJsonArray dataArray;
        for (const auto& d : requiredData)
        {
            dataArray.append(d);
        }
        json["requiredData"] = dataArray;

        return json;
    }

    ReportParams ReportParams::fromJson(const QJsonObject& json)
    {
        ReportParams params;
        params.stockCode = json["stockCode"].toString();
        params.startDate = QDateTime::fromString(json["startDate"].toString(), Qt::ISODate);
        params.endDate = QDateTime::fromString(json["endDate"].toString(), Qt::ISODate);

        QJsonArray stocksArray = json["portfolioStocks"].toArray();
        for (const auto& s : stocksArray)
        {
            params.portfolioStocks.append(s.toString());
        }

        QJsonObject customObj = json["customParams"].toObject();
        for (auto it = customObj.begin(); it != customObj.end(); ++it)
        {
            params.customParams[it.key()] = it.value().toVariant();
        }

        return params;
    }

    QJsonObject ReportParams::toJson() const
    {
        QJsonObject json;
        json["stockCode"] = stockCode;
        json["startDate"] = startDate.toString(Qt::ISODate);
        json["endDate"] = endDate.toString(Qt::ISODate);

        QJsonArray stocksArray;
        for (const auto& s : portfolioStocks)
        {
            stocksArray.append(s);
        }
        json["portfolioStocks"] = stocksArray;

        QJsonObject customObj;
        for (auto it = customParams.begin(); it != customParams.end(); ++it)
        {
            customObj[it.key()] = QJsonValue::fromVariant(it.value());
        }
        json["customParams"] = customObj;

        return json;
    }

    Report Report::fromJson(const QJsonObject& json)
    {
        Report report;
        report.id = json["id"].toString();
        report.title = json["title"].toString();
        report.type = static_cast<ReportType>(json["type"].toInt());
        report.templateId = json["templateId"].toString();
        report.params = ReportParams::fromJson(json["params"].toObject());
        report.summary = json["summary"].toString();
        report.confidence = json["confidence"].toDouble();
        report.generatedAt = QDateTime::fromString(json["generatedAt"].toString(), Qt::ISODate);
        report.author = json["author"].toString();

        QJsonArray sectionsArray = json["sections"].toArray();
        for (const auto& s : sectionsArray)
        {
            report.sections.append(ReportSection::fromJson(s.toObject()));
        }

        QJsonArray pointsArray = json["keyPoints"].toArray();
        for (const auto& p : pointsArray)
        {
            report.keyPoints.append(p.toString());
        }

        QJsonArray recsArray = json["recommendations"].toArray();
        for (const auto& r : recsArray)
        {
            report.recommendations.append(r.toString());
        }

        return report;
    }

    QJsonObject Report::toJson() const
    {
        QJsonObject json;
        json["id"] = id;
        json["title"] = title;
        json["type"] = static_cast<int>(type);
        json["templateId"] = templateId;
        json["params"] = params.toJson();
        json["summary"] = summary;
        json["confidence"] = confidence;
        json["generatedAt"] = generatedAt.toString(Qt::ISODate);
        json["author"] = author;

        QJsonArray sectionsArray;
        for (const auto& s : sections)
        {
            sectionsArray.append(s.toJson());
        }
        json["sections"] = sectionsArray;

        QJsonArray pointsArray;
        for (const auto& p : keyPoints)
        {
            pointsArray.append(p);
        }
        json["keyPoints"] = pointsArray;

        QJsonArray recsArray;
        for (const auto& r : recommendations)
        {
            recsArray.append(r);
        }
        json["recommendations"] = recsArray;

        return json;
    }

    // ============================================================================
    // SmartReportService 实现
    // ============================================================================

    SmartReportService::SmartReportService(QObject* parent)
        : QObject(parent)
    {
        QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        m_storagePath = appDataPath + "/reports";
        QDir dir(m_storagePath);
        if (!dir.exists())
        {
            dir.mkpath(".");
        }

        initializeDefaultTemplates();
        LOG_DEBUG("SmartReportService created");
    }

    SmartReportService::~SmartReportService()
    {
        LOG_DEBUG("SmartReportService destroyed");
    }

    QList<ReportTemplate> SmartReportService::getTemplates() const
    {
        return m_templates.values();
    }

    ReportTemplate SmartReportService::getTemplate(ReportType type) const
    {
        for (const auto& tmpl : m_templates)
        {
            if (tmpl.type == type && tmpl.isDefault)
            {
                return tmpl;
            }
        }
        return ReportTemplate();
    }

    ReportTemplate SmartReportService::getTemplateById(const QString& templateId) const
    {
        return m_templates.value(templateId);
    }

    QString SmartReportService::createTemplate(const ReportTemplate& tmpl)
    {
        QString id = tmpl.id.isEmpty() ? generateReportId() : tmpl.id;
        ReportTemplate newTmpl = tmpl;
        newTmpl.id = id;
        newTmpl.createdAt = QDateTime::currentDateTime();
        newTmpl.updatedAt = newTmpl.createdAt;

        m_templates[id] = newTmpl;
        LOG_INFO("Created report template: " + newTmpl.name);
        return id;
    }

    bool SmartReportService::updateTemplate(const ReportTemplate& tmpl)
    {
        if (!m_templates.contains(tmpl.id))
        {
            return false;
        }

        ReportTemplate updated = tmpl;
        updated.updatedAt = QDateTime::currentDateTime();
        m_templates[tmpl.id] = updated;
        return true;
    }

    bool SmartReportService::deleteTemplate(const QString& templateId)
    {
        if (m_templates.contains(templateId) && !m_templates[templateId].isDefault)
        {
            m_templates.remove(templateId);
            return true;
        }
        return false;
    }

    void SmartReportService::generateReport(ReportType type,
                                            const ReportParams& params,
                                            std::function<void(const Report &)> callback)
    {
        ReportTemplate tmpl = getTemplate(type);

        // 分析数据
        analyzeDataForReport(type, params, [this, tmpl, params, callback](const QJsonObject& data)
        {
            // 生成章节
            generateSectionsWithAI(
                tmpl, data, [this, tmpl, params, data, callback](const QList<ReportSection>& sections)
                {
                    Report report;
                    report.id = generateReportId();
                    report.title = tmpl.name + QStringLiteral(" - ") + QDateTime::currentDateTime().toString(
                        "yyyy-MM-dd");
                    report.type = tmpl.type;
                    report.templateId = tmpl.id;
                    report.params = params;
                    report.sections = sections;
                    report.summary = data["summary"].toString();
                    report.confidence = data["confidence"].toDouble(0.7);
                    report.generatedAt = QDateTime::currentDateTime();
                    report.author = QStringLiteral("AI");

                    // 提取关键点
                    QJsonArray pointsArray = data["keyPoints"].toArray();
                    for (const auto& p : pointsArray)
                    {
                        report.keyPoints.append(p.toString());
                    }

                    // 提取建议
                    QJsonArray recsArray = data["recommendations"].toArray();
                    for (const auto& r : recsArray)
                    {
                        report.recommendations.append(r.toString());
                    }

                    saveReport(report);
                    emit reportGenerated(report);
                    callback(report);
                });
        });
    }

    void SmartReportService::generateStockAnalysis(const QString& stockCode,
                                                   std::function<void(const Report &)> callback)
    {
        ReportParams params;
        params.stockCode = stockCode;
        generateReport(ReportType::StockAnalysis, params, callback);
    }

    void SmartReportService::generatePortfolioReview(const QStringList& stocks,
                                                     std::function<void(const Report &)> callback)
    {
        ReportParams params;
        params.portfolioStocks = stocks;
        generateReport(ReportType::PortfolioReview, params, callback);
    }

    void SmartReportService::generateMarketOutlook(std::function < void(const Report &) > callback)
    {
        ReportParams params;
        generateReport(ReportType::MarketOutlook, params, callback);
    }

    void SmartReportService::generateDailyBrief(std::function < void(const Report &) > callback)
    {
        ReportParams params;
        generateReport(ReportType::DailyBrief, params, callback);
    }

    QList<Report> SmartReportService::listReports() const
    {
        return m_reports.values();
    }

    Report SmartReportService::getReport(const QString& reportId) const
    {
        return m_reports.value(reportId);
    }

    bool SmartReportService::deleteReport(const QString& reportId)
    {
        if (m_reports.remove(reportId) > 0)
        {
            QString filePath = m_storagePath + "/" + reportId + ".json";
            QFile::remove(filePath);
            return true;
        }
        return false;
    }

    QString SmartReportService::exportToText(const Report& report) const
    {
        QString text;
        text += QStringLiteral("========================================\n");
        text += report.title + "\n";
        text += QStringLiteral("生成时间: ") + report.generatedAt.toString("yyyy-MM-dd hh:mm:ss") + "\n";
        text += QStringLiteral("========================================\n\n");

        if (!report.summary.isEmpty())
        {
            text += QStringLiteral("【摘要】\n") + report.summary + "\n\n";
        }

        for (const auto& section : report.sections)
        {
            text += QStringLiteral("【") + section.title + QStringLiteral("】\n");
            text += section.content + "\n\n";
        }

        if (!report.keyPoints.isEmpty())
        {
            text += QStringLiteral("【关键要点】\n");
            for (const auto& point : report.keyPoints)
            {
                text += "• " + point + "\n";
            }
            text += "\n";
        }

        if (!report.recommendations.isEmpty())
        {
            text += QStringLiteral("【投资建议】\n");
            for (const auto& rec : report.recommendations)
            {
                text += "• " + rec + "\n";
            }
        }

        return text;
    }

    QString SmartReportService::exportToMarkdown(const Report& report) const
    {
        QString md;
        md += "# " + report.title + "\n\n";
        md += "**生成时间**: " + report.generatedAt.toString("yyyy-MM-dd hh:mm:ss") + "\n\n";

        if (!report.summary.isEmpty())
        {
            md += "## 摘要\n\n" + report.summary + "\n\n";
        }

        for (const auto& section : report.sections)
        {
            md += "## " + section.title + "\n\n" + section.content + "\n\n";
        }

        if (!report.keyPoints.isEmpty())
        {
            md += "## 关键要点\n\n";
            for (const auto& point : report.keyPoints)
            {
                md += "- " + point + "\n";
            }
            md += "\n";
        }

        if (!report.recommendations.isEmpty())
        {
            md += "## 投资建议\n\n";
            for (const auto& rec : report.recommendations)
            {
                md += "- " + rec + "\n";
            }
        }

        return md;
    }

    QString SmartReportService::exportToHtml(const Report& report) const
    {
        QString html;
        html = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\">";
        html += "<title>" + report.title + "</title>";
        html += "<style>body{font-family:Arial,sans-serif;margin:40px;} ";
        html += "h1{color:#333;} h2{color:#666;border-bottom:1px solid #eee;padding-bottom:10px;} ";
        html += "ul{line-height:1.8;}</style></head><body>";

        html += "<h1>" + report.title + "</h1>";
        html += "<p><strong>生成时间</strong>: " + report.generatedAt.toString("yyyy-MM-dd hh:mm:ss") + "</p>";

        if (!report.summary.isEmpty())
        {
            html += "<h2>摘要</h2><p>" + report.summary + "</p>";
        }

        for (const auto& section : report.sections)
        {
            html += "<h2>" + section.title + "</h2>";
            html += "<p>" + section.content.toHtmlEscaped().replace("\n", "<br>") + "</p>";
        }

        if (!report.keyPoints.isEmpty())
        {
            html += "<h2>关键要点</h2><ul>";
            for (const auto& point : report.keyPoints)
            {
                html += "<li>" + point + "</li>";
            }
            html += "</ul>";
        }

        if (!report.recommendations.isEmpty())
        {
            html += "<h2>投资建议</h2><ul>";
            for (const auto& rec : report.recommendations)
            {
                html += "<li>" + rec + "</li>";
            }
            html += "</ul>";
        }

        html += "</body></html>";
        return html;
    }

    bool SmartReportService::exportToFile(const Report& report,
                                          const QString& filePath,
                                          const QString& format)
    {
        QString content;

        if (format == "html")
        {
            content = exportToHtml(report);
        }
        else if (format == "text")
        {
            content = exportToText(report);
        }
        else
        {
            content = exportToMarkdown(report);
        }

        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            LOG_ERROR("Failed to open file for writing: " + filePath);
            return false;
        }

        file.write(content.toUtf8());
        file.close();

        LOG_INFO("Exported report to: " + filePath);
        emit reportExported(filePath);
        return true;
    }

    QString SmartReportService::getReportTypeName(ReportType type)
    {
        switch (type)
        {
        case ReportType::DailyBrief: return QStringLiteral("日报");
        case ReportType::WeeklySummary: return QStringLiteral("周报");
        case ReportType::MonthlyReview: return QStringLiteral("月报");
        case ReportType::StockAnalysis: return QStringLiteral("个股分析");
        case ReportType::PortfolioReview: return QStringLiteral("组合回顾");
        case ReportType::MarketOutlook: return QStringLiteral("市场展望");
        case ReportType::StrategyReport: return QStringLiteral("策略分析");
        case ReportType::RiskReport: return QStringLiteral("风险评估");
        case ReportType::Custom: return QStringLiteral("自定义");
        default: return QStringLiteral("未知");
        }
    }

    QString SmartReportService::getReportTypeDescription(ReportType type)
    {
        switch (type)
        {
        case ReportType::DailyBrief: return QStringLiteral("每日市场概况和投资机会");
        case ReportType::WeeklySummary: return QStringLiteral("本周市场总结和下周展望");
        case ReportType::MonthlyReview: return QStringLiteral("本月投资回顾和收益分析");
        case ReportType::StockAnalysis: return QStringLiteral("个股深度分析和投资建议");
        case ReportType::PortfolioReview: return QStringLiteral("投资组合回顾和优化建议");
        case ReportType::MarketOutlook: return QStringLiteral("市场走势展望和投资策略");
        case ReportType::StrategyReport: return QStringLiteral("投资策略分析和效果评估");
        case ReportType::RiskReport: return QStringLiteral("投资风险评估和建议");
        case ReportType::Custom: return QStringLiteral("用户自定义报告");
        default: return QStringLiteral("");
        }
    }

    void SmartReportService::initializeDefaultTemplates()
    {
        // 个股分析模板
        ReportTemplate stockAnalysis;
        stockAnalysis.id = "default_stock_analysis";
        stockAnalysis.name = QStringLiteral("个股分析报告");
        stockAnalysis.type = ReportType::StockAnalysis;
        stockAnalysis.description = QStringLiteral("对单只股票进行全面分析");
        stockAnalysis.isDefault = true;
        stockAnalysis.requiredData = {"price", "financial", "technical", "news"};
        stockAnalysis.sections = {
            {"s1", QStringLiteral("公司概况"), "", 1, "text"},
            {"s2", QStringLiteral("财务分析"), "", 2, "text"},
            {"s3", QStringLiteral("技术分析"), "", 3, "text"},
            {"s4", QStringLiteral("估值分析"), "", 4, "text"},
            {"s5", QStringLiteral("投资建议"), "", 5, "text"}
        };
        m_templates[stockAnalysis.id] = stockAnalysis;

        // 组合回顾模板
        ReportTemplate portfolioReview;
        portfolioReview.id = "default_portfolio_review";
        portfolioReview.name = QStringLiteral("组合回顾报告");
        portfolioReview.type = ReportType::PortfolioReview;
        portfolioReview.description = QStringLiteral("投资组合表现回顾");
        portfolioReview.isDefault = true;
        portfolioReview.requiredData = {"positions", "returns", "risk"};
        portfolioReview.sections = {
            {"s1", QStringLiteral("组合概况"), "", 1, "text"},
            {"s2", QStringLiteral("收益分析"), "", 2, "text"},
            {"s3", QStringLiteral("风险分析"), "", 3, "text"},
            {"s4", QStringLiteral("持仓分析"), "", 4, "text"},
            {"s5", QStringLiteral("优化建议"), "", 5, "text"}
        };
        m_templates[portfolioReview.id] = portfolioReview;

        // 市场展望模板
        ReportTemplate marketOutlook;
        marketOutlook.id = "default_market_outlook";
        marketOutlook.name = QStringLiteral("市场展望报告");
        marketOutlook.type = ReportType::MarketOutlook;
        marketOutlook.description = QStringLiteral("市场走势展望和投资策略");
        marketOutlook.isDefault = true;
        marketOutlook.requiredData = {"index", "sector", "sentiment"};
        marketOutlook.sections = {
            {"s1", QStringLiteral("大盘分析"), "", 1, "text"},
            {"s2", QStringLiteral("板块轮动"), "", 2, "text"},
            {"s3", QStringLiteral("资金流向"), "", 3, "text"},
            {"s4", QStringLiteral("市场情绪"), "", 4, "text"},
            {"s5", QStringLiteral("投资策略"), "", 5, "text"}
        };
        m_templates[marketOutlook.id] = marketOutlook;

        // 日报模板
        ReportTemplate dailyBrief;
        dailyBrief.id = "default_daily_brief";
        dailyBrief.name = QStringLiteral("每日简报");
        dailyBrief.type = ReportType::DailyBrief;
        dailyBrief.description = QStringLiteral("每日市场概况");
        dailyBrief.isDefault = true;
        dailyBrief.requiredData = {"market", "hot_stocks", "news"};
        dailyBrief.sections = {
            {"s1", QStringLiteral("市场概况"), "", 1, "text"},
            {"s2", QStringLiteral("热门股票"), "", 2, "text"},
            {"s3", QStringLiteral("重要新闻"), "", 3, "text"},
            {"s4", QStringLiteral("投资机会"), "", 4, "text"}
        };
        m_templates[dailyBrief.id] = dailyBrief;
    }

    void SmartReportService::analyzeDataForReport(ReportType type,
                                                  const ReportParams& params,
                                                  std::function<void(const QJsonObject &)> callback)
    {
        QString prompt;

        switch (type)
        {
        case ReportType::StockAnalysis:
            prompt = QString(QStringLiteral(
                "请分析股票 %1 并生成投资分析报告：\n\n"
                "请包含以下内容：\n"
                "1. 公司概况和行业地位\n"
                "2. 财务状况分析\n"
                "3. 技术面分析\n"
                "4. 估值分析\n"
                "5. 投资建议\n\n"
                "请给出关键要点和具体建议。"
            )).arg(params.stockCode);
            break;

        case ReportType::PortfolioReview:
            prompt = QString(QStringLiteral(
                "请分析投资组合 %1 并生成回顾报告：\n\n"
                "请包含以下内容：\n"
                "1. 组合整体表现\n"
                "2. 各持仓分析\n"
                "3. 风险评估\n"
                "4. 优化建议\n\n"
                "请给出关键要点和具体建议。"
            )).arg(params.portfolioStocks.join(", "));
            break;

        case ReportType::MarketOutlook:
            prompt = QStringLiteral(
                "请生成 A 股市场展望报告：\n\n"
                "请包含以下内容：\n"
                "1. 大盘走势分析\n"
                "2. 板块轮动预测\n"
                "3. 资金流向分析\n"
                "4. 市场情绪判断\n"
                "5. 投资策略建议\n\n"
                "请给出关键要点和具体建议。"
            );
            break;

        case ReportType::DailyBrief:
            prompt = QStringLiteral(
                "请生成今日市场简报：\n\n"
                "请包含以下内容：\n"
                "1. 今日市场概况\n"
                "2. 热门股票和板块\n"
                "3. 重要财经新闻\n"
                "4. 明日投资机会\n\n"
                "请给出关键要点和具体建议。"
            );
            break;

        default:
            prompt = QStringLiteral("请生成投资分析报告，给出关键要点和投资建议。");
        }

        AIService::instance()->chat(prompt, [callback](Result<QString> result)
        {
            if (result.isError())
            {
                callback(QJsonObject());
                return;
            }

            QJsonObject data;
            data["summary"] = result.value();
            data["confidence"] = 0.75;

            callback(data);
        });
    }

    void SmartReportService::generateSectionsWithAI(const ReportTemplate& tmpl,
                                                    const QJsonObject& data,
                                                    std::function<void(const QList<ReportSection> &)> callback)
    {
        QList<ReportSection> sections;

        // 使用模板的章节结构
        for (const auto& section : tmpl.sections)
        {
            ReportSection newSection = section;
            newSection.id = generateReportId();

            // 如果有数据，填充内容
            if (data.contains("summary"))
            {
                newSection.content = data["summary"].toString();
            }

            sections.append(newSection);
        }

        callback(sections);
    }

    QString SmartReportService::generateReportId() const
    {
        return QUuid::createUuid().toString(QUuid::WithoutBraces);
    }

    void SmartReportService::saveReport(const Report& report)
    {
        m_reports[report.id] = report;

        QString filePath = m_storagePath + "/" + report.id + ".json";
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly))
        {
            QJsonDocument doc(report.toJson());
            file.write(doc.toJson());
            file.close();
        }
    }
} // namespace WealthPilot