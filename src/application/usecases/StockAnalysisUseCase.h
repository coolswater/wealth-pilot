/**
 * @file StockAnalysisUseCase.h
 * @brief 股票分析用例
 */

#ifndef STOCK_ANALYSIS_USECASE_H
#define STOCK_ANALYSIS_USECASE_H

#include "UseCaseBase.h"
#include <QString>

namespace WealthPilot {
namespace Application {

class StockAnalysisUseCase : public UseCaseBase
{
    Q_OBJECT

public:
    explicit StockAnalysisUseCase(const QString& symbol, QObject* parent = nullptr);

protected:
    bool doExecute() override;

private:
    QString m_symbol;
};

} // namespace Application
} // namespace WealthPilot

#endif // STOCK_ANALYSIS_USECASE_H
