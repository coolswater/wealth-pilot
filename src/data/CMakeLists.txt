# ==================== Data 模块 CMakeLists.txt ====================
# 数据模块：数据存储、市场数据、数据模型

add_library(Data STATIC)

# ==================== DataHub 数据中心 ====================
target_sources(Data PRIVATE
        datahub/DataHub.h
        datahub/DataHub.cpp
        datahub/MarketDataProducer.h
        datahub/MarketDataProducer.cpp
        datahub/DataHubBootstrap.h
        datahub/DataHubBootstrap.cpp
        datahub/DataHubMonitor.h
        datahub/DataHubMonitor.cpp
)

# ==================== Managers 数据管理 ====================
target_sources(Data PRIVATE
        managers/DataSourceManager.h
        managers/DataSourceManager.cpp
)

# ==================== Market 市场数据 ====================
target_sources(Data PRIVATE
        market/QuoteDataManager.h
        market/QuoteDataManager.cpp
        market/FavoritesManager.h
        market/FavoritesManager.cpp
        market/MarketDataHub.h
        market/MarketDataHub.cpp
        market/StockDataSource.h
        market/MarketDataWebSocket.cpp
        market/StockDataSource.cpp
        market/ForexDataSource.h
        market/ForexDataSource.cpp
        market/CryptoDataSource.h
        market/CryptoDataSource.cpp
        market/FundDataSource.h
        market/FundDataSource.cpp
        market/NewsDataSource.h
        market/NewsDataSource.cpp
)

# ==================== Models 数据模型 ====================
target_sources(Data PRIVATE
        models/FuturesQuoteItem.h
        models/FuturesQuoteModel.h
        models/FuturesQuoteModel.cpp
        models/FuturesQuoteDelegate.h
        models/FuturesQuoteDelegate.cpp
        models/StockQuoteItem.h
        models/StockQuoteItem.cpp
        models/QuoteModelBase.h
        models/QuoteModelBase.cpp
)

# ==================== 数据存储 ====================
target_sources(Data PRIVATE
        DataStorage.h
        DataStorage.cpp
        DataStorageService.h
        DataStorageService.cpp
        MarketDataStorage.h
        MarketDataStorage.cpp
)

# ==================== 包含目录 ====================
target_include_directories(Data PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
)

# ==================== 链接库 ====================
target_link_libraries(Data PUBLIC
        Qt6::Core
        Qt6::Widgets
        Qt6::Sql
        Qt6::Network
        Qt6::Charts
        Qt6::WebSockets
        WealthPilot::Shared
)

# ==================== 编译选项 ====================
if (MSVC)
    target_compile_options(Data PRIVATE /W4)
else ()
    target_compile_options(Data PRIVATE -Wall -Wextra -Wpedantic)
endif ()

# 添加别名，允许其他模块使用 WealthPilot::Data
add_library(WealthPilot::Data ALIAS Data)
