/**
 * @file OrderViewModel.cpp
 * @brief 订单 ViewModel 实现
 * 
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "OrderViewModel.h"
#include "trading/TradingService.h"
#include "trading/RiskController.h"
#include "utils/Logger.h"

namespace WealthPilot
{
    OrderViewModel::OrderViewModel(QObject* parent)
        : ViewModelBase(parent)
    {
        setupCommands();
        LOG_DEBUG("OrderViewModel created");
    }

    OrderViewModel::~OrderViewModel()
    {
        cleanup();
        LOG_DEBUG("OrderViewModel destroyed");
    }

    void OrderViewModel::initialize()
    {
        ViewModelBase::initialize();

        m_tradingService = &TradingService::instance();
        m_riskController = getService<RiskController>();

        if (!m_tradingService)
        {
            setError("TradingService not available");
            return;
        }

        // 连接服务信号
        if (m_tradingService)
        {
            connect(m_tradingService, &TradingService::positionUpdated,
                    this, &OrderViewModel::onPositionUpdated);
        }

        LOG_INFO("OrderViewModel initialized");
    }

    void OrderViewModel::cleanup()
    {
        ViewModelBase::cleanup();

        if (m_tradingService)
        {
            disconnect(m_tradingService, nullptr, this, nullptr);
        }

        LOG_INFO("OrderViewModel cleaned up");
    }

    void OrderViewModel::setupCommands()
    {
        // 计算命令
        m_calculateCommand = createCommand(
            [this]()
            {
                executeCalculate();
                return QVariant();
            },
            [this]() { return !m_instrumentId.isEmpty(); }
        );

        // 提交订单命令
        m_submitCommand = createCommand(
            [this]()
            {
                executeSubmit();
                return QVariant();
            },
            [this]() { return m_canSubmit; }
        );

        // 重置命令
        m_resetCommand = createCommand(
            [this]()
            {
                executeReset();
                return QVariant();
            },
            [this]() { return true; }
        );
    }

    // ========== 合约设置 ==========

    void OrderViewModel::setInstrument(const QString& instrumentId,
                                       const QString& instrumentName,
                                       double lastPrice,
                                       double tickSize,
                                       int volumeMultiple,
                                       double marginRatio,
                                       double limitUp,
                                       double limitDown)
    {
        m_instrumentId = instrumentId;
        m_instrumentName = instrumentName;
        m_lastPrice = lastPrice;
        m_tickSize = tickSize;
        m_volumeMultiple = volumeMultiple;
        m_marginRatio = marginRatio;
        m_limitUp = limitUp;
        m_limitDown = limitDown;

        // 默认价格设为最新价
        m_orderPrice = lastPrice;

        // 更新计算
        updateCalculations();
        validateOrder();

        emit instrumentChanged();
        emit priceChanged();
        emit orderParamsChanged();

        m_calculateCommand->updateCanExecute();

        LOG_DEBUG(QString("Instrument set: %1, price=%2")
                  .arg(instrumentId).arg(lastPrice));
    }

    void OrderViewModel::setAccount(double available, double margin, double frozen)
    {
        Q_UNUSED(margin)
        Q_UNUSED(frozen)

        m_availableFund = available;
        emit accountChanged();

        validateOrder();
    }

    void OrderViewModel::setPosition(int longPos, int shortPos)
    {
        m_longPosition = longPos;
        m_shortPosition = shortPos;

        emit positionChanged();

        validateOrder();
    }

    // ========== 参数设置 ==========

    void OrderViewModel::setOrderPrice(double price)
    {
        if (qAbs(m_orderPrice - price) > 0.0001)
        {
            m_orderPrice = price;
            updateCalculations();
            validateOrder();
            emit orderParamsChanged();
        }
    }

    void OrderViewModel::setOrderVolume(int volume)
    {
        if (m_orderVolume != volume && volume > 0)
        {
            m_orderVolume = volume;
            updateCalculations();
            validateOrder();
            emit orderParamsChanged();
        }
    }

    void OrderViewModel::setOrderType(int type)
    {
        if (m_orderType != type)
        {
            m_orderType = type;
            validateOrder();
            emit orderParamsChanged();
        }
    }

    void OrderViewModel::setDirection(int direction)
    {
        if (m_direction != direction)
        {
            m_direction = direction;
            validateOrder();
            emit orderParamsChanged();
        }
    }

    void OrderViewModel::setOpenClose(int openClose)
    {
        if (m_openClose != openClose)
        {
            m_openClose = openClose;
            validateOrder();
            emit orderParamsChanged();
        }
    }

    void OrderViewModel::setStopPrice(double price)
    {
        if (qAbs(m_stopPrice - price) > 0.0001)
        {
            m_stopPrice = price;
            emit orderParamsChanged();
        }
    }

    void OrderViewModel::setEnableTakeProfit(bool enable)
    {
        if (m_enableTakeProfit != enable)
        {
            m_enableTakeProfit = enable;
            emit stopProfitChanged();
        }
    }

    void OrderViewModel::setTakeProfitPrice(double price)
    {
        if (qAbs(m_takeProfitPrice - price) > 0.0001)
        {
            m_takeProfitPrice = price;
            emit stopProfitChanged();
        }
    }

    void OrderViewModel::setEnableStopLoss(bool enable)
    {
        if (m_enableStopLoss != enable)
        {
            m_enableStopLoss = enable;
            emit stopProfitChanged();
        }
    }

    void OrderViewModel::setStopLossPrice(double price)
    {
        if (qAbs(m_stopLossPrice - price) > 0.0001)
        {
            m_stopLossPrice = price;
            emit stopProfitChanged();
        }
    }

    // ========== 订单参数 ==========

    OrderViewModel::OrderParams OrderViewModel::getOrderParams() const
    {
        OrderParams params;
        params.instrumentId = m_instrumentId;
        params.orderType = static_cast<OrderType>(m_orderType);
        params.direction = static_cast<PositionDirection>(m_direction);
        params.openClose = static_cast<OpenCloseFlag>(m_openClose);
        params.quantity = m_orderVolume;
        params.price = m_orderPrice;
        params.stopPrice = m_stopPrice;
        params.takeProfitPrice = m_takeProfitPrice;
        params.stopLossPrice = m_stopLossPrice;
        params.enableTakeProfit = m_enableTakeProfit;
        params.enableStopLoss = m_enableStopLoss;
        return params;
    }

    // ========== 命令执行 ==========

    void OrderViewModel::executeCalculate()
    {
        updateCalculations();
        updateRiskDisplay();
        validateOrder();

        setStatus("计算完成");
        LOG_DEBUG("Order calculated");
    }

    void OrderViewModel::executeSubmit()
    {
        if (!m_canSubmit)
        {
            setError(m_validationError);
            return;
        }

        setStatus("提交订单...");

        // 风控检查
        if (!checkRiskControl())
        {
            return;
        }

        // 提交订单
        if (m_tradingService)
        {
            OrderRequest request;
            request.instrumentId = m_instrumentId;
            request.direction = static_cast<TradeDirection>(m_direction);
            request.openClose = static_cast<OpenCloseFlag>(m_openClose);
            request.orderType = static_cast<OrderType>(m_orderType);
            request.price = m_orderPrice;
            request.volume = m_orderVolume;
            request.stopPrice = m_stopPrice;

            QString orderId = m_tradingService->submitOrder(request);

            if (!orderId.isEmpty())
            {
                emit orderSubmitted(orderId);
                setStatus(QString("订单已提交: %1").arg(orderId));
                LOG_INFO(QString("Order submitted: %1").arg(orderId));
            }
            else
            {
                setError("订单提交失败");
                emit orderRejected("订单提交失败");
            }
        }
    }

    void OrderViewModel::executeReset()
    {
        m_orderPrice = m_lastPrice;
        m_orderVolume = 1;
        m_orderType = 0;
        m_stopPrice = 0.0;
        m_enableTakeProfit = false;
        m_takeProfitPrice = 0.0;
        m_enableStopLoss = false;
        m_stopLossPrice = 0.0;

        clearError();
        clearStatus();

        updateCalculations();
        validateOrder();

        emit orderParamsChanged();
        emit stopProfitChanged();

        LOG_DEBUG("Order reset");
    }

    // ========== 计算更新 ==========

    void OrderViewModel::updateCalculations()
    {
        // 计算保证金
        m_requiredMargin = m_orderVolume * m_orderPrice * m_volumeMultiple * m_marginRatio;

        // 计算手续费（假设万分之三）
        double commissionRate = 0.0003;
        m_estimatedCommission = m_orderVolume * m_orderPrice * m_volumeMultiple * commissionRate;

        // 总资金需求
        m_totalRequirement = m_requiredMargin + m_estimatedCommission;

        emit calculationChanged();
    }

    void OrderViewModel::updateRiskDisplay()
    {
        if (m_orderPrice <= 0 || m_orderVolume <= 0)
        {
            m_riskRatio = 0.0;
            m_riskAmount = 0.0;
            m_profitRatio = 0.0;
            m_profitAmount = 0.0;
            emit calculationChanged();
            return;
        }

        // 计算止损风险
        if (m_enableStopLoss&& m_stopLossPrice
        >
        0
        )
        {
            double priceDiff = 0.0;
            if (m_direction == 0)
            {
                // 买入
                priceDiff = m_orderPrice - m_stopLossPrice;
            }
            else
            {
                // 卖出
                priceDiff = m_stopLossPrice - m_orderPrice;
            }
            m_riskAmount = qAbs(priceDiff * m_orderVolume * m_volumeMultiple);
            m_riskRatio = m_riskAmount / (m_orderPrice * m_orderVolume * m_volumeMultiple) * 100;
        }
        else
        {
            m_riskAmount = 0.0;
            m_riskRatio = 0.0;
        }

        // 计算止盈盈利
        if (m_enableTakeProfit&& m_takeProfitPrice
        >
        0
        )
        {
            double priceDiff = 0.0;
            if (m_direction == 0)
            {
                // 买入
                priceDiff = m_takeProfitPrice - m_orderPrice;
            }
            else
            {
                // 卖出
                priceDiff = m_orderPrice - m_takeProfitPrice;
            }
            m_profitAmount = qAbs(priceDiff * m_orderVolume * m_volumeMultiple);
            m_profitRatio = m_profitAmount / (m_orderPrice * m_orderVolume * m_volumeMultiple) * 100;
        }
        else
        {
            m_profitAmount = 0.0;
            m_profitRatio = 0.0;
        }

        emit calculationChanged();
    }

    void OrderViewModel::validateOrder()
    {
        m_validationError.clear();
        m_canSubmit = true;

        // 检查合约
        if (m_instrumentId.isEmpty())
        {
            m_validationError = "请先选择合约";
            m_canSubmit = false;
            emit validationChanged();
            return;
        }

        // 检查价格
        if (m_orderPrice <= 0)
        {
            m_validationError = "请输入有效价格";
            m_canSubmit = false;
            emit validationChanged();
            return;
        }

        // 检查数量
        if (m_orderVolume <= 0)
        {
            m_validationError = "请输入有效数量";
            m_canSubmit = false;
            emit validationChanged();
            return;
        }

        // 检查资金（仅开仓）
        if (m_openClose == 0)
        {
            // 开仓
            if (m_availableFund < m_totalRequirement)
            {
                m_validationError = QString("资金不足，需要 %1，可用 %2")
                                    .arg(m_totalRequirement, 0, 'f', 2)
                                    .arg(m_availableFund, 0, 'f', 2);
                m_canSubmit = false;
            }
        }

        // 检查持仓（仅平仓）
        if (m_openClose == 1)
        {
            // 平仓
            if (m_direction == 0 && m_orderVolume > m_shortPosition)
            {
                // 买入平仓
                m_validationError = QString("空头持仓不足，当前 %1 手").arg(m_shortPosition);
                m_canSubmit = false;
            }
            else if (m_direction == 1 && m_orderVolume > m_longPosition)
            {
                // 卖出平仓
                m_validationError = QString("多头持仓不足，当前 %1 手").arg(m_longPosition);
                m_canSubmit = false;
            }
        }

        // 检查涨跌停
        if (m_limitUp > 0 && m_orderPrice > m_limitUp)
        {
            m_validationError = QString("价格超过涨停价 %1").arg(m_limitUp, 0, 'f', 2);
            m_canSubmit = false;
        }
        if (m_limitDown > 0 && m_orderPrice < m_limitDown)
        {
            m_validationError = QString("价格低于跌停价 %1").arg(m_limitDown, 0, 'f', 2);
            m_canSubmit = false;
        }

        emit validationChanged();
        m_submitCommand->updateCanExecute();
    }

    bool OrderViewModel::checkRiskControl()
    {
        if (m_riskController)
        {
            OrderRequest request;
            request.instrumentId = m_instrumentId;
            request.direction = static_cast<TradeDirection>(m_direction);
            request.openClose = static_cast<OpenCloseFlag>(m_openClose);
            request.volume = m_orderVolume;
            request.price = m_orderPrice;
            request.orderType = static_cast<OrderType>(m_orderType);

            auto result = m_riskController->checkOrder(request);
            if (!result.passed)
            {
                setError(result.message);
                emit riskWarning(result.message);
                return false;
            }
        }
        return true;
    }

    // ========== 信号处理 ==========

    void OrderViewModel::onPositionUpdated(const PositionInfo& position)
    {
        if (position.instrumentId == m_instrumentId)
        {
            // 根据方向设置持仓
            if (position.direction == PositionDirection::Long)
            {
                m_longPosition = position.volume;
            }
            else if (position.direction == PositionDirection::Short)
            {
                m_shortPosition = position.volume;
            }
            emit positionChanged();
            validateOrder();
        }
    }
} // namespace WealthPilot