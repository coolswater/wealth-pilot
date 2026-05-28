#include "BacktestStrategyUseCase.h"
#include "shared/utils/Logger.h"

namespace WealthPilot {
namespace Application {

BacktestStrategyUseCase::BacktestStrategyUseCase(const QString& strategyId, QObject* parent)
    : UseCaseBase(parent), m_strategyId(strategyId)
{
}

bool BacktestStrategyUseCase::doExecute()
{
    LOG_INFO(QString("Backtesting strategy: %1").arg(m_strategyId));
    return true;
}

} // namespace Application
} // namespace WealthPilot