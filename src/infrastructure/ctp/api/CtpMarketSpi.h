/////////////////////////////////////////////////////////////////////////
///@file CtpMarketSpi.h
///@brief 行情SPI实现 - 独立线程处理行情数据
/////////////////////////////////////////////////////////////////////////

#ifndef CTPMARKETSPI_H
#define CTPMARKETSPI_H

#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtCore/QScopedPointer>

// CTP SDK 使用未命名参数作为虚函数占位符，禁用警告
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <external/ctp/ThostFtdcUserApiStruct.h>
#include <external/ctp/ThostFtdcMdApi.h>
#pragma GCC diagnostic pop

namespace CTP {

// 前向声明
// class BatchBuffer;
struct MarketData;

/**
 * @brief 行情SPI类
 * @details 继承CTP SPI，内部转换为Qt信号，运行于独立线程
 */
class CtpMarketSpi : public QObject, public CThostFtdcMdSpi {
    Q_OBJECT
public:
    explicit CtpMarketSpi(QObject *parent = nullptr);
    ~CtpMarketSpi() override;

    /**
     * @brief 创建并初始化API（线程安全）
     */
    void createApi(const QString& flowPath, bool isUdp = false,
                   bool isMulticast = false);

    /**
     * @brief 注册前置地址
     */
    void registerFront(const QString& address);

    /**
     * @brief 初始化并连接
     */
    void init();

    /**
     * @brief 释放API
     */
    void release();

    /**
     * @brief 登录请求
     */
    void login(const QString& brokerId, const QString& userId,
               const QString& password);

    /**
     * @brief 订阅行情（批量）
     */
    void subscribeMarketData(const QList<QString>& instruments);

    /**
     * @brief 取消订阅
     */
    void unsubscribeMarketData(const QList<QString>& instruments);

    // CTP SPI回调实现（C++17 override）
    void OnFrontConnected() override;
    void OnFrontDisconnected(int nReason) override;
    void OnHeartBeatWarning(int nTimeLapse) override;
    void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
                        CThostFtdcRspInfoField *pRspInfo,
                        int nRequestID, bool bIsLast) override;
    void OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument,
                            CThostFtdcRspInfoField *pRspInfo,
                            int nRequestID, bool bIsLast) override;
    void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData) override;
    void OnRspError(CThostFtdcRspInfoField *pRspInfo,
                    int nRequestID, bool bIsLast) override;

signals:
    void connected();
    void disconnected(int reason);
    void heartbeatWarning(int timeLapse);
    void loginResult(bool success, const QString& msg);
    void marketDataReceived(const CTP::MarketData& data);
    void error(int requestId, int errorId, const QString& msg);

private:
    class Impl;
    QScopedPointer<Impl> d;

    // C++17 结构化绑定辅助函数
    static MarketData convertDepthMarketData(
        const CThostFtdcDepthMarketDataField& field);
};

/**
 * @brief 行情工作线程
 * @details 确保所有CTP API调用都在同一线程（CTP要求）
 */
class MarketWorker : public QThread {
    Q_OBJECT
public:
    explicit MarketWorker(QObject *parent = nullptr) : QThread(parent) {}

    void run() override {
        // C++17 线程局部存储
        thread_local bool initialized = false;
        if (!initialized) {
            initialized = true;
            exec();  // 进入Qt事件循环
        }
    }
};

} // namespace CTP

#endif
