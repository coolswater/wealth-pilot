/**
 * @file TradingIntegration.h
 * @brief 交易页面与TradingService集成
 */

#ifndef TRADINGINTEGRATION_H
#define TRADINGINTEGRATION_H

#include <QObject>
#include <QVector>
#include <QVariantMap>
#include "domain/trading/TradingTypes.h"

// Forward declarations in WealthPilot namespace
namespace WealthPilot {
    class AccountPage;
    class TradeHistoryPage;
    class ConditionOrderPage;
}

namespace Trading {

/**
 * @brief 交易页面集成管理器
 * @details 负责连接UI页面与TradingService
 */
class TradingIntegration : public QObject
{
    Q_OBJECT

public:
    static TradingIntegration& instance();

    /**
     * @brief 初始化集成
     */
    void initialize();

    /**
     * @brief 注册账户页面
     */
    void registerAccountPage(WealthPilot::AccountPage* page);

    /**
     * @brief 注册成交记录页面
     */
    void registerTradeHistoryPage(WealthPilot::TradeHistoryPage* page);

    /**
     * @brief 注册条件单页面
     */
    void registerConditionOrderPage(WealthPilot::ConditionOrderPage* page);

    /**
     * @brief 处理下单提交
     */
    void handleOrderSubmit(const QString& instrumentId, OrderType orderType,
                           TradeDirection direction, OpenCloseFlag openClose,
                           int quantity, double price, double stopPrice = 0.0);

signals:
    /**
     * @brief 账户数据更新
     */
    void accountDataUpdated(double balance, double available, double margin,
                            double frozenMargin, double commission, 
                            double closeProfit, double positionProfit);

private slots:
    /**
     * @brief 处理TradingService订单成交
     */
    void onOrderFilled(const QString& orderId, const TradeRecord& trade);

    /**
     * @brief 处理TradingService持仓更新
     */
    void onPositionUpdated(const PositionInfo& position);

    /**
     * @brief 处理TradingService盈亏更新
     */
    void onProfitUpdated(double totalProfit);

private:
    TradingIntegration(QObject* parent = nullptr);
    ~TradingIntegration() override = default;
    Q_DISABLE_COPY(TradingIntegration)

    void connectTradingService();
    void updateAccountPage();

    WealthPilot::AccountPage* m_accountPage = nullptr;
        WealthPilot::TradeHistoryPage* m_tradeHistoryPage = nullptr;
        WealthPilot::ConditionOrderPage* m_conditionOrderPage = nullptr;
};

} // namespace Trading

#endif // TRADINGINTEGRATION_H
