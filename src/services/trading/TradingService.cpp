/**
 * @file TradingService.cpp
 * @brief 交易服务实现
 */

#include "TradingService.h"
#include "shared/utils/Logger.h"

namespace WealthPilot {
namespace Services {

struct TradingService::Impl {
    bool initialized = false;
};

TradingService::TradingService()
    : d(std::make_unique<Impl>())
{
}

TradingService::~TradingService()
{
    shutdown();
}

bool TradingService::initialize()
{
    if (d->initialized) return true;

    LOG_INFO("TradingService initializing...");
    d->initialized = true;
    emit serviceReady();
    return true;
}

void TradingService::shutdown()
{
    if (!d->initialized) return;

    LOG_INFO("TradingService shutting down...");
    d->initialized = false;
}

} // namespace Services
} // namespace WealthPilot
