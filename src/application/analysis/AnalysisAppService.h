/**
 * @file AnalysisAppService.h
 * @brief 分析应用服务 - 协调各种分析器
 */

#ifndef ANALYSIS_APP_SERVICE_H
#define ANALYSIS_APP_SERVICE_H

#include <QObject>
#include <QVariantMap>
#include "shared/base/Singleton.h"

namespace WealthPilot {
namespace Application {

/**
 * @brief 分析应用服务
 */
class AnalysisAppService : public QObject, public Singleton<AnalysisAppService>
{
    Q_OBJECT
    friend class Singleton<AnalysisAppService>;

public:
    /**
     * @brief 运行技术分析
     */
    void runTechnicalAnalysis(const QString& symbol);

    /**
     * @brief 运行缠论分析
     */
    void runChanLunAnalysis(const QString& symbol);

    /**
     * @brief 获取分析结果
     */
    QVariantMap getAnalysisResult(const QString& symbol);

signals:
    void analysisCompleted(const QString& symbol, const QVariantMap& result);
    void analysisFailed(const QString& symbol, const QString& error);

private:
    AnalysisAppService();
    ~AnalysisAppService();

    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace Application
} // namespace WealthPilot

#endif // ANALYSIS_APP_SERVICE_H
