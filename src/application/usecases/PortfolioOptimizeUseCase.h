/**
 * @file PortfolioOptimizeUseCase.h
 * @brief 组合优化用例
 */

#ifndef PORTFOLIO_OPTIMIZE_USECASE_H
#define PORTFOLIO_OPTIMIZE_USECASE_H

#include "UseCaseBase.h"
#include <QStringList>

namespace WealthPilot {
namespace Application {

class PortfolioOptimizeUseCase : public UseCaseBase
{
    Q_OBJECT

public:
    explicit PortfolioOptimizeUseCase(const QStringList& symbols, QObject* parent = nullptr);

protected:
    bool doExecute() override;

private:
    QStringList m_symbols;
};

} // namespace Application
} // namespace WealthPilot

#endif // PORTFOLIO_OPTIMIZE_USECASE_H