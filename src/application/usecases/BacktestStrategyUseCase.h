/**
 * @file BacktestStrategyUseCase.h
 * @brief 回测策略用例
 */

#ifndef BACKTEST_STRATEGY_USECASE_H
#define BACKTEST_STRATEGY_USECASE_H

#include "UseCaseBase.h"
#include <QString>

namespace WealthPilot {
namespace Application {

class BacktestStrategyUseCase : public UseCaseBase
{
    Q_OBJECT

public:
    explicit BacktestStrategyUseCase(const QString& strategyId, QObject* parent = nullptr);

protected:
    bool doExecute() override;

private:
    QString m_strategyId;
};

} // namespace Application
} // namespace WealthPilot

#endif // BACKTEST_STRATEGY_USECASE_H