/////////////////////////////////////////////////////////////////////////
///@file CtpApiLoader.h
///@brief CTP API 动态加载器 - 解决 MinGW 与 MSVC DLL 兼容性问题
/////////////////////////////////////////////////////////////////////////

#ifndef CTPAPILOADER_H
#define CTPAPILOADER_H

#include <QtCore/QLibrary>
#include <QtCore/QObject>
#include <QtCore/QDebug>
#include <functional>
#include <memory>

// CTP 头文件
#include "external/ctp/ThostFtdcMdApi.h"
#include "external/ctp/ThostFtdcTraderApi.h"
#include "shared/utils/Logger.h"

namespace CTP {

/**
 * @brief CTP API 函数指针类型
 */
// 行情 API 函数指针
using CreateFtdcMdApiFunc = CThostFtdcMdApi* (*)(const char*, bool, bool, bool);
using GetMdApiVersionFunc = const char* (*)();

// 交易 API 函数指针
using CreateFtdcTraderApiFunc = CThostFtdcTraderApi* (*)(const char*, bool);
using GetTraderApiVersionFunc = const char* (*)();

/**
 * @brief CTP API 动态加载器
 * @details 解决 MinGW 与 MSVC 编译的 CTP DLL 名称修饰不兼容问题
 */
class CtpApiLoader {
public:
    static CtpApiLoader& instance() {
        static CtpApiLoader loader;
        return loader;
    }

    /**
     * @brief 加载行情 API
     * @return 是否加载成功
     */
    bool loadMarketApi() {
        if (m_mdLibrary.isLoaded()) {
            return true;
        }

        // 尝试加载 DLL
        m_mdLibrary.setFileName("thostmduserapi_se");
        if (!m_mdLibrary.load()) {
            LOG_ERROR(QString("Failed to load thostmduserapi_se.dll: %1").arg(m_mdLibrary.errorString()));
            return false;
        }

        // 解析函数
        m_createMdApi = reinterpret_cast<CreateFtdcMdApiFunc>(
            m_mdLibrary.resolve("?CreateFtdcMdApi@CThostFtdcMdApi@@SAPEAV1@PEBD_N1_N@Z"));

        m_getMdApiVersion = reinterpret_cast<GetMdApiVersionFunc>(
            m_mdLibrary.resolve("?GetApiVersion@CThostFtdcMdApi@@SAPEBDXZ"));

        if (!m_createMdApi) {
            LOG_ERROR(QString("Failed to resolve CreateFtdcMdApi function"));
            return false;
        }

        LOG_INFO(QString("CTP Market API loaded, version: %1").arg((m_getMdApiVersion ? m_getMdApiVersion() : "unknown")));
        return true;
    }

    /**
     * @brief 加载交易 API
     * @return 是否加载成功
     */
    bool loadTraderApi() {
        if (m_traderLibrary.isLoaded()) {
            return true;
        }

        m_traderLibrary.setFileName("thosttraderapi_se");
        if (!m_traderLibrary.load()) {
            LOG_ERROR(QString("Failed to load thosttraderapi_se.dll %1").arg(m_traderLibrary.errorString()));
            return false;
        }

        m_createTraderApi = reinterpret_cast<CreateFtdcTraderApiFunc>(
            m_traderLibrary.resolve("?CreateFtdcTraderApi@CThostFtdcTraderApi@@SAPEAV1@PEBD_N@Z"));

        m_getTraderApiVersion = reinterpret_cast<GetTraderApiVersionFunc>(
            m_traderLibrary.resolve("?GetApiVersion@CThostFtdcTraderApi@@SAPEBDXZ"));

        if (!m_createTraderApi) {
            LOG_ERROR(QString("Failed to resolve CreateFtdcTraderApi function"));
            return false;
        }

        LOG_INFO(QString("CTP Trader API loaded, version: %1").arg((m_getTraderApiVersion ? m_getTraderApiVersion() : "unknown")));
        return true;
    }

    /**
     * @brief 创建行情 API 实例
     */
    CThostFtdcMdApi* createMdApi(const char* pszFlowPath = "",
                                  bool bIsUsingUdp = false,
                                  bool bIsMulticast = false,
                                  bool bIsProductionMode = true) {
        if (!loadMarketApi() || !m_createMdApi) {
            return nullptr;
        }
        return m_createMdApi(pszFlowPath, bIsUsingUdp, bIsMulticast, bIsProductionMode);
    }

    /**
     * @brief 创建交易 API 实例
     */
    CThostFtdcTraderApi* createTraderApi(const char* pszFlowPath = "",
                                          bool bIsProductionMode = true) {
        if (!loadTraderApi() || !m_createTraderApi) {
            return nullptr;
        }
        return m_createTraderApi(pszFlowPath, bIsProductionMode);
    }

    /**
     * @brief 获取行情 API 版本
     */
    const char* getMdApiVersion() {
        if (!loadMarketApi() || !m_getMdApiVersion) {
            return "unknown";
        }
        return m_getMdApiVersion();
    }

    /**
     * @brief 获取交易 API 版本
     */
    const char* getTraderApiVersion() {
        if (!loadTraderApi() || !m_getTraderApiVersion) {
            return "unknown";
        }
        return m_getTraderApiVersion();
    }

private:
    CtpApiLoader() = default;
    ~CtpApiLoader() {
        m_mdLibrary.unload();
        m_traderLibrary.unload();
    }

    QLibrary m_mdLibrary;
    QLibrary m_traderLibrary;

    CreateFtdcMdApiFunc m_createMdApi{nullptr};
    GetMdApiVersionFunc m_getMdApiVersion{nullptr};
    CreateFtdcTraderApiFunc m_createTraderApi{nullptr};
    GetTraderApiVersionFunc m_getTraderApiVersion{nullptr};
};

} // namespace CTP

#endif // CTPAPILOADER_H
