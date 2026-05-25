/**
 * @file Types.h
 * @brief 统一类型导出头文件
 * @details 包含所有类型定义，方便其他文件引用
 * @author WealthPilot Team
 * @date 2026-01-01
 */

#pragma once

// 市场数据类型
#include "MarketTypes.h"

// 交易相关类型
#include "TradingTypes.h"

// 分析相关类型
#include "AnalysisTypes.h"

// 新闻相关类型
#include "NewsTypes.h.h"

/**
 * @namespace WealthPilot
 * @brief WealthPilot 命名空间
 * 
 * 包含所有公共类型定义和工具函数
 */

/**
 * @brief 类型定义使用说明
 * 
 * 1. 所有公共类型定义在 WealthPilot 命名空间中
 * 2. 使用 `#include "shared/types/Types.h"` 包含所有类型
 * 3. 或按需包含单个类型文件
 * 
 * 示例:
 * @code
 * #include "shared/types/Types.h"
 * 
 * WealthPilot::Quote quote;
 * quote.symbol = "600519";
 * quote.price = 1800.0;
 * @endcode
 */
