/**
 * @file TradingViewModel.h
 * @brief 交易 ViewModel - MVVM 架构示例
 * 
 * @details 提供交易面板的数据绑定和命令：
 * - 实时行情数据
 * - 持仓信息
 * - 交易命令（买入/卖出开仓/平仓）
 * - 账户资金
 * 
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef TRADINGVIEWMODEL_H
#define TRADINGVIEWMODEL_H

#include "ViewModelBase.h"
#include "trading/TradingTypes.h"
#include <QTimer>

// 前向声明
class TradingService;
class RiskController;

namespace WealthPilot
{
    /**
 * @brief 交易 ViewModel
 *
 * @details 用于 QML 交易面板：
 * @code
 * // TradingPanel.qml
 * TradingViewModel {
 *     id: viewModel
 * }
 *
 * Text { text: viewModel.currentPrice.toFixed(2) }
 * Button {
 *     text: "买入开仓"
 *     enabled: viewModel.buyOpenCommand.canExecute
 *     onClicked: viewModel.buyOpenCommand.execute()
 * }
 * @endcode
 */
    class TradingViewModel : public ViewModelBase
    {
        Q_OBJECT

        // ========== 合约信息 ==========

        /// 合约代码
        Q_PROPERTY(QString instrumentId READ instrumentId NOTIFY instrumentChanged)
        /// 合约名称
        Q_PROPERTY(QString instrumentName READ instrumentName NOTIFY instrumentChanged)
        /// 合约交易所
        Q_PROPERTY(QString exchange READ exchange NOTIFY instrumentChanged)

        // ========== 行情数据 ==========

        /// 最新价
        Q_PROPERTY(double currentPrice READ currentPrice NOTIFY priceChanged)
        /// 涨跌额
        Q_PROPERTY(double priceChange READ priceChange NOTIFY priceChanged)
        /// 涨跌幅 (%)
        Q_PROPERTY(double priceChangePercent READ priceChangePercent NOTIFY priceChanged)
        /// 昨收价
        Q_PROPERTY(double preClosePrice READ preClosePrice NOTIFY priceChanged)
        /// 开盘价
        Q_PROPERTY(double openPrice READ openPrice NOTIFY priceChanged)
        /// 最高价
        Q_PROPERTY(double highPrice READ highPrice NOTIFY priceChanged)
        /// 最低价
        Q_PROPERTY(double lowPrice READ lowPrice NOTIFY priceChanged)
        /// 成交量
        Q_PROPERTY(qint64 volume READ volume NOTIFY volumeChanged)
        /// 成交额
        Q_PROPERTY(double turnover READ turnover NOTIFY volumeChanged)

        // ========== 持仓信息 ==========

        /// 多头持仓
        Q_PROPERTY(int longPosition READ longPosition NOTIFY positionChanged)
        /// 空头持仓
        Q_PROPERTY(int shortPosition READ shortPosition NOTIFY positionChanged)
        /// 多头均价
        Q_PROPERTY(double longAvgPrice READ longAvgPrice NOTIFY positionChanged)
        /// 空头均价
        Q_PROPERTY(double shortAvgPrice READ shortAvgPrice NOTIFY positionChanged)
        /// 多头盈亏
        Q_PROPERTY(double longProfit READ longProfit NOTIFY positionChanged)
        /// 空头盈亏
        Q_PROPERTY(double shortProfit READ shortProfit NOTIFY positionChanged)
        /// 总盈亏
        Q_PROPERTY(double totalProfit READ totalProfit NOTIFY positionChanged)

        // ========== 账户信息 ==========

        /// 可用资金
        Q_PROPERTY(double availableFund READ availableFund NOTIFY fundChanged)
        /// 总资产
        Q_PROPERTY(double totalAsset READ totalAsset NOTIFY fundChanged)
        /// 冻结保证金
        Q_PROPERTY(double frozenMargin READ frozenMargin NOTIFY fundChanged)
        /// 风险度
        Q_PROPERTY(double riskLevel READ riskLevel NOTIFY fundChanged)

        // ========== 交易参数 ==========

        /// 最小变动价位
        Q_PROPERTY(double tickSize READ tickSize NOTIFY instrumentChanged)
        /// 合约乘数
        Q_PROPERTY(int volumeMultiple READ volumeMultiple NOTIFY instrumentChanged)
        /// 保证金比例
        Q_PROPERTY(double marginRatio READ marginRatio NOTIFY instrumentChanged)
        /// 涨停价
        Q_PROPERTY(double limitUp READ limitUp NOTIFY instrumentChanged)
        /// 跌停价
        Q_PROPERTY(double limitDown READ limitDown NOTIFY instrumentChanged)

        // ========== 交易命令 ==========

        /// 买入开仓命令
        Q_PROPERTY(Command* buyOpenCommand READ buyOpenCommand CONSTANT)
        /// 卖出开仓命令
        Q_PROPERTY(Command* sellOpenCommand READ sellOpenCommand CONSTANT)
        /// 买入平仓命令
        Q_PROPERTY(Command* buyCloseCommand READ buyCloseCommand CONSTANT)
        /// 卖出平仓命令
        Q_PROPERTY(Command* sellCloseCommand READ sellCloseCommand CONSTANT)
        /// 刷新命令
        Q_PROPERTY(Command* refreshCommand READ refreshCommand CONSTANT)

        // ========== 下单参数 ==========

        /// 下单价格
        Q_PROPERTY(double orderPrice READ orderPrice WRITE setOrderPrice NOTIFY orderParamsChanged)
        /// 下单数量
        Q_PROPERTY(int orderVolume READ orderVolume WRITE setOrderVolume NOTIFY orderParamsChanged)
        /// 下单类型
        Q_PROPERTY(int orderType READ orderType WRITE setOrderType NOTIFY orderParamsChanged)

    public:
        explicit TradingViewModel(QObject* parent = nullptr);
        ~TradingViewModel() override;

        // ========== 初始化 ==========

        void initialize() override;
        void cleanup() override;

        // ========== 属性访问 ==========

        // 合约信息
        QString instrumentId() const { return m_instrumentId; }
        QString instrumentName() const { return m_instrumentName; }
        QString exchange() const { return m_exchange; }

        // 行情数据
        double currentPrice() const { return m_currentPrice; }
        double priceChange() const { return m_priceChange; }
        double priceChangePercent() const { return m_priceChangePercent; }
        double preClosePrice() const { return m_preClosePrice; }
        double openPrice() const { return m_openPrice; }
        double highPrice() const { return m_highPrice; }
        double lowPrice() const { return m_lowPrice; }
        qint64 volume() const { return m_volume; }
        double turnover() const { return m_turnover; }

        // 持仓信息
        int longPosition() const { return m_longPosition; }
        int shortPosition() const { return m_shortPosition; }
        double longAvgPrice() const { return m_longAvgPrice; }
        double shortAvgPrice() const { return m_shortAvgPrice; }
        double longProfit() const { return m_longProfit; }
        double shortProfit() const { return m_shortProfit; }
        double totalProfit() const { return m_longProfit + m_shortProfit; }

        // 账户信息
        double availableFund() const { return m_availableFund; }
        double totalAsset() const { return m_totalAsset; }
        double frozenMargin() const { return m_frozenMargin; }
        double riskLevel() const;

        // 交易参数
        double tickSize() const { return m_tickSize; }
        int volumeMultiple() const { return m_volumeMultiple; }
        double marginRatio() const { return m_marginRatio; }
        double limitUp() const { return m_limitUp; }
        double limitDown() const { return m_limitDown; }

        // 命令访问
        Command* buyOpenCommand() { return m_buyOpenCommand; }
        Command* sellOpenCommand() { return m_sellOpenCommand; }
        Command* buyCloseCommand() { return m_buyCloseCommand; }
        Command* sellCloseCommand() { return m_sellCloseCommand; }
        Command* refreshCommand() { return m_refreshCommand; }

        // 下单参数
        double orderPrice() const { return m_orderPrice; }
        int orderVolume() const { return m_orderVolume; }
        int orderType() const { return m_orderType; }

        // ========== 公共方法 ==========

        /**
     * @brief 设置当前合约
     */
    Q_INVOKABLE void setInstrument(const QString& instrumentId,
                                   const QString& instrumentName = QString(),
                                   const QString& exchange = QString());

        /**
     * @brief 设置下单价格
     */
        void setOrderPrice(double price);

        /**
     * @brief 设置下单数量
     */
        void setOrderVolume(int volume);

        /**
     * @brief 设置下单类型
     */
        void setOrderType(int type);

        /**
     * @brief 计算保证金
     */
    Q_INVOKABLE double calculateMargin(int volume, double price) const;

        /**
     * @brief 计算手续费
     */
    Q_INVOKABLE double calculateCommission(int volume, double price) const;

        /**
     * @brief 计算盈亏
     */
    Q_INVOKABLE double calculateProfit(int volume, double openPrice, double closePrice,
                                       PositionDirection direction) const;

        signals :

        void instrumentChanged();
        void priceChanged();
        void volumeChanged();
        void positionChanged();
        void profitChanged();
        void fundChanged();
        void orderParamsChanged();

        /**
     * @brief 订单提交成功
     */
        void orderSubmitted(const QString& orderId, const QString& operation);

        /**
     * @brief 订单被拒绝
     */
        void orderRejected(const QString& reason);

        /**
     * @brief 风控警告
     */
        void riskWarning(const QString& message);

    private
        slots :
        // 服务信号处理

        void onPositionUpdated(const PositionInfo& position);
        void onOrderSubmitted(const QString& orderId);
        void onOrderFilled(const QString& orderId, const TradeRecord& trade);
        void onOrderRejected(const QString& orderId, const QString& reason);

    private:
        // 初始化命令
        void setupCommands();

        // 执行交易
        void executeBuyOpen();
        void executeSellOpen();
        void executeBuyClose();
        void executeSellClose();
        void executeRefresh();

        // 检查是否可交易
        bool canTrade() const;
        bool canBuyOpen() const;
        bool canSellOpen() const;
        bool canBuyClose() const;
        bool canSellClose() const;

        // 更新盈亏
        void updateProfit();

        // ========== 合约信息 ==========
        QString m_instrumentId;
        QString m_instrumentName;
        QString m_exchange;

        // ========== 行情数据 ==========
        double m_currentPrice = 0.0;
        double m_priceChange = 0.0;
        double m_priceChangePercent = 0.0;
        double m_preClosePrice = 0.0;
        double m_openPrice = 0.0;
        double m_highPrice = 0.0;
        double m_lowPrice = 0.0;
        qint64 m_volume = 0;
        double m_turnover = 0.0;

        // ========== 持仓信息 ==========
        int m_longPosition = 0;
        int m_shortPosition = 0;
        double m_longAvgPrice = 0.0;
        double m_shortAvgPrice = 0.0;
        double m_longProfit = 0.0;
        double m_shortProfit = 0.0;
        double m_totalProfit = 0.0;

        // ========== 账户信息 ==========
        double m_availableFund = 0.0;
        double m_totalAsset = 0.0;
        double m_frozenMargin = 0.0;

        // ========== 交易参数 ==========
        double m_tickSize = 0.01;
        int m_volumeMultiple = 1;
        double m_marginRatio = 0.1;
        double m_limitUp = 0.0;
        double m_limitDown = 0.0;

        // ========== 下单参数 ==========
        double m_orderPrice = 0.0;
        int m_orderVolume = 1;
        int m_orderType = 0; // 0=限价, 1=市价, 2=对手价

        // ========== 命令 ==========
        Command* m_buyOpenCommand = nullptr;
        Command* m_sellOpenCommand = nullptr;
        Command* m_buyCloseCommand = nullptr;
        Command* m_sellCloseCommand = nullptr;
        Command* m_refreshCommand = nullptr;

        // ========== 服务引用 ==========
        TradingService* m_tradingService = nullptr;
        RiskController* m_riskController = nullptr;
    };
} // namespace WealthPilot

#endif // TRADINGVIEWMODEL_H
