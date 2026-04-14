/**
 * @file FuturesPagesRegistration.cpp
 * @brief 期货页面注册 - 将期货相关页面注册到页面工厂
 *
 * @details 注册页面：
 * - 期货K线详情页
 * - 期货行情列表页
 * - 期货交易下单页
 * - 期货持仓查询页
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#include "FuturesKLinePage.h"
#include "core/PageFactoryRegistry.h"
#include "utils/Logger.h"

namespace WealthPilot {
namespace Futures {

/**
 * @brief 注册期货页面
 */
void registerPages()
{
    LOG_INFO("Registering futures pages...");
    
    // 注册期货K线详情页
    PageFactoryRegistry::instance()->registerPage<FuturesKLinePage>(
        "FuturesKLine",
        "期货K线详情页"
    );
    
    LOG_INFO("Futures pages registered successfully");
}

/**
 * @brief 注销期货页面
 */
void unregisterPages()
{
    LOG_INFO("Unregistering futures pages...");
    
    PageFactoryRegistry::instance()->unregisterPage("FuturesKLine");
    
    LOG_INFO("Futures pages unregistered successfully");
}

} // namespace Futures
} // namespace WealthPilot
