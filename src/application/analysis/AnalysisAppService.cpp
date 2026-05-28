/**
 * @file AnalysisAppService.cpp
 */

#include "AnalysisAppService.h"
#include "shared/utils/Logger.h"

namespace WealthPilot {
namespace Application {

struct AnalysisAppService::Impl {
    QMap<QString, QVariantMap> results;
};

AnalysisAppService::AnalysisAppService()
    : d(std::make_unique<Impl>())
{
    LOG_INFO("AnalysisAppService initialized");
}

AnalysisAppService::~AnalysisAppService() = default;

void AnalysisAppService::runTechnicalAnalysis(const QString& symbol)
{
    LOG_DEBUG(QString("Run technical analysis: %1").arg(symbol));
    // TODO: 调用 Domain 层分析器
}

void AnalysisAppService::runChanLunAnalysis(const QString& symbol)
{
    LOG_DEBUG(QString("Run ChanLun analysis: %1").arg(symbol));
    // TODO: 调用缠论分析器
}

QVariantMap AnalysisAppService::getAnalysisResult(const QString& symbol)
{
    return d->results.value(symbol);
}

} // namespace Application
} // namespace WealthPilot
