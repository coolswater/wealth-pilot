/**
 * @file PortfolioAppService.h
 * @brief 投资组合应用服务
 */

#ifndef PORTFOLIO_APP_SERVICE_H
#define PORTFOLIO_APP_SERVICE_H

#include <QObject>
#include "shared/base/Singleton.h"

namespace WealthPilot {
namespace Application {

class PortfolioAppService : public QObject, public Singleton<PortfolioAppService>
{
    Q_OBJECT
    friend class Singleton<PortfolioAppService>;

public:
    void optimizePortfolio(const QStringList& symbols);

signals:
    void optimizationCompleted();

private:
    PortfolioAppService() = default;
};

} // namespace Application
} // namespace WealthPilot

#endif // PORTFOLIO_APP_SERVICE_H
