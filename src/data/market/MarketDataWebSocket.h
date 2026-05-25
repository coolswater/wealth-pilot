/**
 * @file MarketDataWebSocket.h
 * @brief 实时行情 WebSocket 推送接口
 *
 * @details 实现功能：
 * - WebSocket 连接管理
 * - 行情订阅/取消订阅
 * - 实时 Tick 数据推送
 * - 自动重连机制
 * - 心跳保活
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef MARKETDATAWEBSOCKET_H
#define MARKETDATAWEBSOCKET_H

#include <QObject>
#include <QWebSocket>
#include <QTimer>
#include <QMap>
#include <QSet>
#include <memory>

/**
 * @brief 实时行情 Tick 数据
 */
struct MarketTick
{
    QString symbol; ///< 证券代码
    QString name; ///< 证券名称
    double price = 0.0; ///< 最新价
    double open = 0.0; ///< 开盘价
    double high = 0.0; ///< 最高价
    double low = 0.0; ///< 最低价
    double preClose = 0.0; ///< 昨收价
    qint64 volume = 0; ///< 成交量
    double amount = 0.0; ///< 成交额
    double bidPrice1 = 0.0; ///< 买一价
    qint64 bidVolume1 = 0; ///< 买一量
    double askPrice1 = 0.0; ///< 卖一价
    qint64 askVolume1 = 0; ///< 卖一量
    qint64 timestamp = 0; ///< 时间戳
};

/**
 * @brief 行情订阅类型
 */
enum class MarketSubscribeType
{
    Tick = 0x01, ///< Tick 行情
    KLine = 0x02, ///< K线行情
    Depth = 0x04, ///< 盘口深度
    All = Tick | KLine | Depth
};

/**
 * @brief 实时行情 WebSocket 类
 */
class MarketDataWebSocket : public QObject
{
    Q_OBJECT

public:
    explicit MarketDataWebSocket(QObject* parent = nullptr);
    ~MarketDataWebSocket() override;

    // 连接管理
    void connectToServer(const QString& url);
    void disconnect();
    bool isConnected() const;

    // 订阅管理
    void subscribe(const QString& symbol, MarketSubscribeType type = MarketSubscribeType::Tick);
    void unsubscribe(const QString& symbol);
    void unsubscribeAll();
    QSet<QString> subscribedSymbols() const;

    // 配置
    void setReconnectInterval(int ms); ///< 设置重连间隔
    void setHeartbeatInterval(int ms); ///< 设置心跳间隔
    void setAutoReconnect(bool enabled); ///< 设置自动重连

    signals :
    /**
     * @brief 连接成功信号
     */

    void connected();

    /**
     * @brief 连接断开信号
     */
    void disconnected();

    /**
     * @brief 连接错误信号
     */
    void connectionError(const QString& errorString);

    /**
     * @brief 收到 Tick 数据信号
     */
    void tickReceived(const MarketTick& tick);

    /**
     * @brief 订阅成功信号
     */
    void subscribeSuccess(const QString& symbol);

    /**
     * @brief 取消订阅成功信号
     */
    void unsubscribeSuccess(const QString& symbol);

private
    slots :

    void onConnected();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError error);
    void onTextMessageReceived(const QString& message);
    void onHeartbeat();
    void onReconnect();

private:
    void sendHeartbeat();
    void resubscribeAll();
    void parseTickData(const QJsonObject& json);
    void parseKLineData(const QJsonObject& json);
    void parseDepthData(const QJsonObject& json);

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // MARKETDATAWEBSOCKET_H
