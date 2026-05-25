/**
 * @file TradingPagesRegistration.cpp
 * @brief 交易页面注册 - 将交易相关页面注册到页面工厂
 */

#include "account/AccountPage.h"
#include "core/trading/TradeHistoryPage.h"
#include "core/trading/ConditionOrderPage.h"
#include "core/navigation/PageFactoryRegistry.h"
#include "shared/utils/Logger.h"

namespace WealthPilot {
namespace Trading {

/**
 * @brief 注册交易页面
 */
void registerPages()
{
    LOG_INFO("Registering trading pages...");
    
    // 注册账户资金页面
    PageFactoryRegistry::instance()->registerPage<AccountPage>(
        "account",
        QStringLiteral("账户资金")
    );
    
    // 注册成交记录页面
    PageFactoryRegistry::instance()->registerPage<TradeHistoryPage>(
        "tradeHistory",
        QStringLiteral("成交记录")
    );
    
    // 注册条件单管理页面
    PageFactoryRegistry::instance()->registerPage<ConditionOrderPage>(
        "conditionOrder",
        QStringLiteral("条件单")
    );
    
    LOG_INFO("Trading pages registered successfully");
}

/**
 * @brief 注销交易页面
 */
void unregisterPages()
{
    LOG_INFO("Unregistering trading pages...");
    
    PageFactoryRegistry::instance()->unregisterPage("account");
    PageFactoryRegistry::instance()->unregisterPage("tradeHistory");
    PageFactoryRegistry::instance()->unregisterPage("conditionOrder");
    
    LOG_INFO("Trading pages unregistered successfully");
}

} // namespace Trading
} // namespace WealthPilot
