#include "TradeExecutionUseCase.h"
#include "shared/utils/Logger.h"

namespace WealthPilot {
namespace Application {

TradeExecutionUseCase::TradeExecutionUseCase(const QString& orderId, QObject* parent)
    : UseCaseBase(parent), m_orderId(orderId)
{
}

bool TradeExecutionUseCase::doExecute()
{
    LOG_INFO(QString("Executing trade: %1").arg(m_orderId));
    return true;
}

} // namespace Application
} // namespace WealthPilot