/**
 * @file TradeExecutionUseCase.h
 * @brief 交易执行用例
 */

#ifndef TRADE_EXECUTION_USECASE_H
#define TRADE_EXECUTION_USECASE_H

#include "UseCaseBase.h"
#include <QString>

namespace WealthPilot {
namespace Application {

class TradeExecutionUseCase : public UseCaseBase
{
    Q_OBJECT

public:
    explicit TradeExecutionUseCase(const QString& orderId, QObject* parent = nullptr);

protected:
    bool doExecute() override;

private:
    QString m_orderId;
};

} // namespace Application
} // namespace WealthPilot

#endif // TRADE_EXECUTION_USECASE_H