/**
 * @file RiskAppService.h
 * @brief 风控应用服务
 */

#ifndef RISK_APP_SERVICE_H
#define RISK_APP_SERVICE_H

#include <QObject>
#include "shared/base/Singleton.h"

namespace WealthPilot {
namespace Application {

class RiskAppService : public QObject, public Singleton<RiskAppService>
{
    Q_OBJECT
    friend class Singleton<RiskAppService>;

public:
    void checkRisk(const QString& symbol);

signals:
    void riskAlert(const QString& message);

private:
    RiskAppService() = default;
};

} // namespace Application
} // namespace WealthPilot

#endif // RISK_APP_SERVICE_H
