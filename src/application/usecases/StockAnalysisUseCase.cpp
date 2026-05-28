#include "StockAnalysisUseCase.h"
#include "shared/utils/Logger.h"

namespace WealthPilot {
namespace Application {

StockAnalysisUseCase::StockAnalysisUseCase(const QString& symbol, QObject* parent)
    : UseCaseBase(parent), m_symbol(symbol)
{
}

bool StockAnalysisUseCase::doExecute()
{
    LOG_INFO(QString("Executing StockAnalysisUseCase for %1").arg(m_symbol));
    // TODO: 实现分析逻辑
    return true;
}

} // namespace Application
} // namespace WealthPilot
