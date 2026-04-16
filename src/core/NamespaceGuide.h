/**
 * @file NamespaceGuide.h
 * @brief 命名空间使用指南
 * @author WealthPilot Team
 * @version 2.0.0
 * 
 * @details 定义项目的命名空间结构：
 * 
 * WealthPilot/                    # 顶层命名空间
 * ├── Core/                       # 核心框架
 * │   ├── Singleton
 * │   ├── ServiceLocator
 * │   ├── BasePage
 * │   └── ErrorCode
 * │
 * ├── Models/                     # 数据模型
 * │   ├── FuturesQuoteItem
 * │   ├── StockQuoteItem
 * │   └── MarketData
 * │
 * ├── Views/                      # UI视图
 * │   ├── Widgets/
 * │   └── Pages/
 * │
 * ├── Services/                   # 服务层
 * │   ├── CTP/
 * │   └── AI/
 * │
 * ├── Network/                    # 网络层
 * │   ├── NetworkManager
 * │   └── NetworkCache
 * │
 * └── Utils/                      # 工具类
 *     ├── Logger
 *     └── Result
 */

#ifndef WEALTHPILOT_CORE_NAMESPACEGUIDE_H
#define WEALTHPILOT_CORE_NAMESPACEGUIDE_H

/**
 * @brief 命名空间使用规则
 * 
 * 1. 所有代码必须在 WealthPilot 或其子命名空间中
 * 2. 子命名空间按模块划分
 * 3. 头文件中使用完整命名空间
 * 4. cpp 文件中可以使用 using 缩短
 * 
 * @example
 * @code
 * // .h 文件
 * namespace WealthPilot::Core {
 *     class MyService : public Singleton<MyService> { ... };
 * }
 * 
 * // .cpp 文件
 * namespace WealthPilot::Core {
 *     using namespace Utils; // 使用其他子命名空间
 *     
 *     void MyService::doSomething() { ... }
 * }
 * @endcode
 */

// ========== 命名空间别名 ==========

namespace WealthPilot {
    // 核心模块
    namespace Core {}
    
    // 数据模型
    namespace Models {}
    
    // UI视图
    namespace Views {}
    
    // 服务层
    namespace Services {}
    
    // 网络层
    namespace Network {}
    
    // 工具类
    namespace Utils {}
    
    // 插件
    namespace Plugins {}
}

// ========== 常用别名 ==========
// 可以在 cpp 文件中使用这些别名简化代码

// namespace WP = WealthPilot;  // 在需要时使用

#endif // WEALTHPILOT_CORE_NAMESPACEGUIDE_H
