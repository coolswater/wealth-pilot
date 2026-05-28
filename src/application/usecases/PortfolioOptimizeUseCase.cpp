#include "PortfolioOptimizeUseCase.h"
#include "shared/utils/Logger.h"

namespace WealthPilot {
namespace Application {

PortfolioOptimizeUseCase::PortfolioOptimizeUseCase(const QStringList& symbols, QObject* parent)
    : UseCaseBase(parent), m_symbols(symbols)
{
}

bool PortfolioOptimizeUseCase::doExecute()
{
    LOG_INFO(QString("Portfolio optimization for %1 symbols").arg(m_symbols.size()));
    return true;
}

} // namespace Application
} // namespace WealthPilot