/**
 * @file OrderViewModel.h
 * @brief 订单 ViewModel - MVVM 架构
 * 
 * @details 提供订单对话框的数据绑定和命令：
 * - 合约信息
 * - 下单参数（价格、数量、类型）
 * - 计算结果（保证金、手续费）
 * - 止损止盈设置
 * - 风控检查
 * 
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef ORDERVIEWMODEL_H
#define ORDERVIEWMODEL_H

#include "ViewModelBase.h"
#include "domain/trading/TradingTypes.h"
#include "domain/trading/TradingService.h"
#include "domain/trading/RiskController.h"
#include <QTimer>

namespace WealthPilot
{
    /**
 * @brief 订单 ViewModel
 *
 * @details 用于 QML 订单对话框：
 * @code
 * // OrderDialog.qml
 * OrderViewModel {
 *     id: viewModel
 * }
 *
 * TextField {
 *     text: viewModel.orderPrice
 *     onTextChanged: viewModel.setOrderPrice(text)
 * }
 *
 * Button {
 *     text: "提交订单"
 *     enabled: viewModel.submitCommand.canExecute
 *     onClicked: viewModel.submitCommand.execute()
 * }
 * @endcode
 */
    class OrderViewModel : public ViewModelBase
    {
        Q_OBJECT

        // ========== 合约信息 ==========

        /// 合约代码
        Q_PROPERTY(QString instrumentId READ instrumentId NOTIFY instrumentChanged)
        /// 合约名称
        Q_PROPERTY(QString instrumentName READ instrumentName NOTIFY instrumentChanged)
        /// 最新价
        Q_PROPERTY(double lastPrice READ lastPrice NOTIFY priceChanged)
        /// 涨跌额
        Q_PROPERTY(double priceChange READ priceChange NOTIFY priceChanged)
        /// 涨跌幅 (%)
        Q_PROPERTY(double priceChangePercent READ priceChangePercent NOTIFY priceChanged)
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

        // ========== 下单参数 ==========

        /// 订单类型 (0:限价, 1:市价, 2:对手价, 3:止损)
        Q_PROPERTY(int orderType READ orderType WRITE setOrderType NOTIFY orderParamsChanged)
        /// 交易方向 (0:买入, 1:卖出)
        Q_PROPERTY(int direction READ direction WRITE setDirection NOTIFY orderParamsChanged)
        /// 开平标志 (0:开仓, 1:平仓)
        Q_PROPERTY(int openClose READ openClose WRITE setOpenClose NOTIFY orderParamsChanged)
        /// 下单价格
        Q_PROPERTY(double orderPrice READ orderPrice WRITE setOrderPrice NOTIFY orderParamsChanged)
        /// 下单数量
        Q_PROPERTY(int orderVolume READ orderVolume WRITE setOrderVolume NOTIFY orderParamsChanged)
        /// 止损价格（止损单）
        Q_PROPERTY(double stopPrice READ stopPrice WRITE setStopPrice NOTIFY orderParamsChanged)

        // ========== 计算结果 ==========

        /// 需要保证金
        Q_PROPERTY(double requiredMargin READ requiredMargin NOTIFY calculationChanged)
        /// 预估手续费
        Q_PROPERTY(double estimatedCommission READ estimatedCommission NOTIFY calculationChanged)
        /// 总资金需求
        Q_PROPERTY(double totalRequirement READ totalRequirement NOTIFY calculationChanged)
        /// 可用资金
        Q_PROPERTY(double availableFund READ availableFund NOTIFY accountChanged)
        /// 资金是否足够
        Q_PROPERTY(bool fundSufficient READ fundSufficient NOTIFY calculationChanged)

        // ========== 持仓信息 ==========

        /// 多头持仓
        Q_PROPERTY(int longPosition READ longPosition NOTIFY positionChanged)
        /// 空头持仓
        Q_PROPERTY(int shortPosition READ shortPosition NOTIFY positionChanged)
        /// 可平多头
        Q_PROPERTY(int closeableLong READ closeableLong NOTIFY positionChanged)
        /// 可平空头
        Q_PROPERTY(int closeableShort READ closeableShort NOTIFY positionChanged)

        // ========== 止损止盈 ==========

        /// 是否启用止盈
        Q_PROPERTY(bool enableTakeProfit READ enableTakeProfit WRITE setEnableTakeProfit NOTIFY stopProfitChanged)
        /// 止盈价格
        Q_PROPERTY(double takeProfitPrice READ takeProfitPrice WRITE setTakeProfitPrice NOTIFY stopProfitChanged)
        /// 是否启用止损
        Q_PROPERTY(bool enableStopLoss READ enableStopLoss WRITE setEnableStopLoss NOTIFY stopProfitChanged)
        /// 止损价格
        Q_PROPERTY(double stopLossPrice READ stopLossPrice WRITE setStopLossPrice NOTIFY stopProfitChanged)

        // ========== 风控显示 ==========

        /// 风险比例
        Q_PROPERTY(double riskRatio READ riskRatio NOTIFY calculationChanged)
        /// 风险金额
        Q_PROPERTY(double riskAmount READ riskAmount NOTIFY calculationChanged)
        /// 盈利比例
        Q_PROPERTY(double profitRatio READ profitRatio NOTIFY calculationChanged)
        /// 盈利金额
        Q_PROPERTY(double profitAmount READ profitAmount NOTIFY calculationChanged)

        // ========== 命令 ==========

        /// 计算命令
        Q_PROPERTY(Command* calculateCommand READ calculateCommand CONSTANT)
        /// 提交订单命令
        Q_PROPERTY(Command* submitCommand READ submitCommand CONSTANT)
        /// 重置命令
        Q_PROPERTY(Command* resetCommand READ resetCommand CONSTANT)

        // ========== 验证状态 ==========

        /// 是否可提交
        Q_PROPERTY(bool canSubmit READ canSubmit NOTIFY validationChanged)
        /// 验证错误信息
        Q_PROPERTY(QString validationError READ validationError NOTIFY validationChanged)

    public:
        explicit OrderViewModel(QObject* parent = nullptr);
        ~OrderViewModel() override;

        // ========== 初始化 ==========

        void initialize() override;
        void cleanup() override;

        // ========== 属性访问 ==========

        // 合约信息
        QString instrumentId() const { return m_instrumentId; }
        QString instrumentName() const { return m_instrumentName; }
        double lastPrice() const { return m_lastPrice; }
        double priceChange() const { return m_priceChange; }
        double priceChangePercent() const { return m_priceChangePercent; }
        double tickSize() const { return m_tickSize; }
        int volumeMultiple() const { return m_volumeMultiple; }
        double marginRatio() const { return m_marginRatio; }
        double limitUp() const { return m_limitUp; }
        double limitDown() const { return m_limitDown; }

        // 下单参数
        int orderType() const { return m_orderType; }
        int direction() const { return m_direction; }
        int openClose() const { return m_openClose; }
        double orderPrice() const { return m_orderPrice; }
        int orderVolume() const { return m_orderVolume; }
        double stopPrice() const { return m_stopPrice; }

        // 计算结果
        double requiredMargin() const { return m_requiredMargin; }
        double estimatedCommission() const { return m_estimatedCommission; }
        double totalRequirement() const { return m_totalRequirement; }
        double availableFund() const { return m_availableFund; }
        bool fundSufficient() const { return m_availableFund >= m_totalRequirement; }

        // 持仓信息
        int longPosition() const { return m_longPosition; }
        int shortPosition() const { return m_shortPosition; }
        int closeableLong() const { return m_longPosition; }
        int closeableShort() const { return m_shortPosition; }

        // 止损止盈
        bool enableTakeProfit() const { return m_enableTakeProfit; }
        double takeProfitPrice() const { return m_takeProfitPrice; }
        bool enableStopLoss() const { return m_enableStopLoss; }
        double stopLossPrice() const { return m_stopLossPrice; }

        // 风控显示
        double riskRatio() const { return m_riskRatio; }
        double riskAmount() const { return m_riskAmount; }
        double profitRatio() const { return m_profitRatio; }
        double profitAmount() const { return m_profitAmount; }

        // 命令
        Command* calculateCommand() { return m_calculateCommand; }
        Command* submitCommand() { return m_submitCommand; }
        Command* resetCommand() { return m_resetCommand; }

        // 验证状态
        bool canSubmit() const { return m_canSubmit; }
        QString validationError() const { return m_validationError; }

        // ========== 公共方法 ==========

        /**
     * @brief 设置合约信息
     */
    Q_INVOKABLE void setInstrument(const QString& instrumentId,
                                   const QString& instrumentName,
                                   double lastPrice,
                                   double tickSize,
                                   int volumeMultiple,
                                   double marginRatio,
                                   double limitUp = 0.0,
                                   double limitDown = 0.0);

        /**
     * @brief 设置账户信息
     */
    Q_INVOKABLE void setAccount(double available, double margin, double frozen);

        /**
     * @brief 设置持仓信息
     */
    Q_INVOKABLE void setPosition(int longPos, int shortPos);

        /**
     * @brief 设置下单价格
     */
        void setOrderPrice(double price);

        /**
     * @brief 设置下单数量
     */
        void setOrderVolume(int volume);

        /**
     * @brief 设置订单类型
     */
        void setOrderType(int type);

        /**
     * @brief 设置交易方向
     */
        void setDirection(int direction);

        /**
     * @brief 设置开平标志
     */
        void setOpenClose(int openClose);

        /**
     * @brief 设置止损价格
     */
        void setStopPrice(double price);

        /**
     * @brief 设置止盈启用
     */
        void setEnableTakeProfit(bool enable);

        /**
     * @brief 设置止盈价格
     */
        void setTakeProfitPrice(double price);

        /**
     * @brief 设置止损启用
     */
        void setEnableStopLoss(bool enable);

        /**
     * @brief 设置止损价格
     */
        void setStopLossPrice(double price);

        /**
     * @brief 获取订单参数
     */
        struct OrderParams
        {
            QString instrumentId;
            OrderType orderType;
            PositionDirection direction;
            OpenCloseFlag openClose;
            int quantity;
            double price;
            double stopPrice;
            double takeProfitPrice;
            double stopLossPrice;
            bool enableTakeProfit;
            bool enableStopLoss;
        };

        OrderParams getOrderParams() const;

        signals :

        void instrumentChanged();
        void priceChanged();
        void orderParamsChanged();
        void calculationChanged();
        void accountChanged();
        void positionChanged();
        void stopProfitChanged();
        void validationChanged();

        /**
     * @brief 订单提交成功
     */
        void orderSubmitted(const QString& orderId);

        /**
     * @brief 订单提交失败
     */
        void orderRejected(const QString& reason);

        /**
     * @brief 风控警告
     */
        void riskWarning(const QString& message);

    private
        slots :

        void onPositionUpdated(const PositionInfo& position);

    private:
        void setupCommands();
        void executeCalculate();
        void executeSubmit();
        void executeReset();

        void updateCalculations();
        void updateRiskDisplay();
        void validateOrder();
        bool checkRiskControl();

        // ========== 合约信息 ==========
        QString m_instrumentId;
        QString m_instrumentName;
        double m_lastPrice = 0.0;
        double m_priceChange = 0.0;
        double m_priceChangePercent = 0.0;
        double m_tickSize = 0.01;
        int m_volumeMultiple = 1;
        double m_marginRatio = 0.1;
        double m_limitUp = 0.0;
        double m_limitDown = 0.0;

        // ========== 下单参数 ==========
        int m_orderType = 0; // 限价
        int m_direction = 0; // 买入
        int m_openClose = 0; // 开仓
        double m_orderPrice = 0.0;
        int m_orderVolume = 1;
        double m_stopPrice = 0.0;

        // ========== 计算结果 ==========
        double m_requiredMargin = 0.0;
        double m_estimatedCommission = 0.0;
        double m_totalRequirement = 0.0;
        double m_availableFund = 0.0;

        // ========== 持仓信息 ==========
        int m_longPosition = 0;
        int m_shortPosition = 0;

        // ========== 止损止盈 ==========
        bool m_enableTakeProfit = false;
        double m_takeProfitPrice = 0.0;
        bool m_enableStopLoss = false;
        double m_stopLossPrice = 0.0;

        // ========== 风控显示 ==========
        double m_riskRatio = 0.0;
        double m_riskAmount = 0.0;
        double m_profitRatio = 0.0;
        double m_profitAmount = 0.0;

        // ========== 命令 ==========
        Command* m_calculateCommand = nullptr;
        Command* m_submitCommand = nullptr;
        Command* m_resetCommand = nullptr;

        // ========== 验证状态 ==========
        bool m_canSubmit = false;
        QString m_validationError;

        // ========== 服务引用 ==========
        TradingService* m_tradingService = nullptr;
        RiskController* m_riskController = nullptr;
    };
} // namespace WealthPilot

#endif // ORDERVIEWMODEL_H