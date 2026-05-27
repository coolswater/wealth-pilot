/**
 * @file TradingViewModel.cpp
 * @brief 交易 ViewModel 实现
 * 
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "TradingViewModel.h"
#include "core/trading/TradingService.h"
#include "core/trading/RiskController.h"
#include "shared/utils/Logger.h"

#include <QMessageBox>

namespace WealthPilot
{
    TradingViewModel::TradingViewModel(QObject* parent)
        : ViewModelBase(parent)
    {
        setupCommands();
        LOG_DEBUG("TradingViewModel created");
    }

    TradingViewModel::~TradingViewModel()
    {
        cleanup();
        LOG_DEBUG("TradingViewModel destroyed");
    }

    void TradingViewModel::initialize()
    {
        ViewModelBase::initialize();

        // 获取服务
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
                    this, &TradingViewModel::onPositionUpdated);
            connect(m_tradingService, &TradingService::orderSubmitted,
                    this, &TradingViewModel::onOrderSubmitted);
            connect(m_tradingService, &TradingService::orderFilled,
                    this, &TradingViewModel::onOrderFilled);
            connect(m_tradingService, &TradingService::orderRejected,
                    this, &TradingViewModel::onOrderRejected);
            connect(m_tradingService, &TradingService::profitUpdated,
                    this, [this](double profit)
                    {
                        m_totalProfit = profit;
                        emit profitChanged();
                    });
        }

        // 连接风控信号
        if (m_riskController)
        {
            connect(m_riskController, &RiskController::riskWarning,
                    this, &TradingViewModel::riskWarning);
        }

        LOG_INFO("TradingViewModel initialized");
    }

    void TradingViewModel::cleanup()
    {
        ViewModelBase::cleanup();

        // 断开服务连接
        if (m_tradingService)
        {
            disconnect(m_tradingService, nullptr, this, nullptr);
        }
        if (m_riskController)
        {
            disconnect(m_riskController, nullptr, this, nullptr);
        }

        LOG_INFO("TradingViewModel cleaned up");
    }

    void TradingViewModel::setupCommands()
    {
        // 买入开仓命令
        m_buyOpenCommand = createCommand(
            [this]()
            {
                executeBuyOpen();
                return QVariant();
            },
            [this]() { return canBuyOpen(); }
        );

        // 卖出开仓命令
        m_sellOpenCommand = createCommand(
            [this]()
            {
                executeSellOpen();
                return QVariant();
            },
            [this]() { return canSellOpen(); }
        );

        // 买入平仓命令
        m_buyCloseCommand = createCommand(
            [this]()
            {
                executeBuyClose();
                return QVariant();
            },
            [this]() { return canBuyClose(); }
        );

        // 卖出平仓命令
        m_sellCloseCommand = createCommand(
            [this]()
            {
                executeSellClose();
                return QVariant();
            },
            [this]() { return canSellClose(); }
        );

        // 刷新命令
        m_refreshCommand = createCommand(
            [this]()
            {
                executeRefresh();
                return QVariant();
            },
            [this]() { return true; }
        );
    }

    // ========== 合约设置 ==========

    void TradingViewModel::setInstrument(const QString& instrumentId,
                                         const QString& instrumentName,
                                         const QString& exchange)
    {
        if (m_instrumentId == instrumentId)
        {
            return;
        }

        m_instrumentId = instrumentId;
        m_instrumentName = instrumentName;
        m_exchange = exchange;

        // 重置行情数据
        m_currentPrice = 0.0;
        m_priceChange = 0.0;
        m_priceChangePercent = 0.0;
        m_preClosePrice = 0.0;
        m_openPrice = 0.0;
        m_highPrice = 0.0;
        m_lowPrice = 0.0;
        m_volume = 0;
        m_turnover = 0.0;

        // 重置持仓数据
        m_longPosition = 0;
        m_shortPosition = 0;
        m_longAvgPrice = 0.0;
        m_shortAvgPrice = 0.0;
        m_longProfit = 0.0;
        m_shortProfit = 0.0;

        // 更新命令状态
        m_buyOpenCommand->updateCanExecute();
        m_sellOpenCommand->updateCanExecute();
        m_buyCloseCommand->updateCanExecute();
        m_sellCloseCommand->updateCanExecute();

        emit instrumentChanged();
        emit priceChanged();
        emit positionChanged();

        LOG_INFO(QString("Instrument set: %1").arg(instrumentId));
    }

    // ========== 下单参数设置 ==========

    void TradingViewModel::setOrderPrice(double price)
    {
        if (m_orderPrice != price)
        {
            m_orderPrice = price;
            emit orderParamsChanged();
        }
    }

    void TradingViewModel::setOrderVolume(int volume)
    {
        if (m_orderVolume != volume)
        {
            m_orderVolume = volume;
            emit orderParamsChanged();
        }
    }

    void TradingViewModel::setOrderType(int type)
    {
        if (m_orderType != type)
        {
            m_orderType = type;
            emit orderParamsChanged();
        }
    }

    // ========== 计算方法 ==========

    double TradingViewModel::calculateMargin(int volume, double price) const
    {
        return volume * price * m_volumeMultiple * m_marginRatio;
    }

    double TradingViewModel::calculateCommission(int volume, double price) const
    {
        // 假设手续费率为万分之三
        double commissionRate = 0.0003;
        return volume * price * m_volumeMultiple * commissionRate;
    }

    double TradingViewModel::calculateProfit(int volume, double openPrice, double closePrice,
                                             PositionDirection direction) const
    {
        double profit = 0.0;
        if (direction == PositionDirection::Long)
        {
            profit = (closePrice - openPrice) * volume * m_volumeMultiple;
        }
        else
        {
            profit = (openPrice - closePrice) * volume * m_volumeMultiple;
        }
        return profit;
    }

    double TradingViewModel::riskLevel() const
    {
        if (m_totalAsset <= 0)
        {
            return 0.0;
        }
        return m_frozenMargin / m_totalAsset;
    }

    // ========== 交易执行 ==========

    void TradingViewModel::executeBuyOpen()
    {
        if (!canBuyOpen())
        {
            setError("Cannot execute buy open");
            return;
        }

        setStatus(QString("买入开仓 %1 手 @ %2").arg(m_orderVolume).arg(m_orderPrice));

        // 构建订单请求
        OrderRequest request;
        request.instrumentId = m_instrumentId;
        request.direction = TradeDirection::Buy;
        request.openClose = OpenCloseFlag::Open;
        request.volume = m_orderVolume;
        request.price = m_orderPrice;
        request.orderType = static_cast<OrderType>(m_orderType);

        // 风控检查
        if (m_riskController)
        {
            auto result = m_riskController->checkOrder(request);
            if (!result.passed)
            {
                setError(result.message);
                emit riskWarning(result.message);
                return;
            }
        }

        // 提交订单
        if (m_tradingService)
        {
            QString orderId = m_tradingService->submitOrder(request);

            emit orderSubmitted(orderId, "买入开仓");
            LOG_INFO(QString("Buy open order submitted: %1").arg(orderId));
        }
    }

    void TradingViewModel::executeSellOpen()
    {
        if (!canSellOpen())
        {
            setError("Cannot execute sell open");
            return;
        }

        setStatus(QString("卖出开仓 %1 手 @ %2").arg(m_orderVolume).arg(m_orderPrice));

        // 构建订单请求
        OrderRequest request;
        request.instrumentId = m_instrumentId;
        request.direction = TradeDirection::Sell;
        request.openClose = OpenCloseFlag::Open;
        request.volume = m_orderVolume;
        request.price = m_orderPrice;
        request.orderType = static_cast<OrderType>(m_orderType);

        // 风控检查
        if (m_riskController)
        {
            auto result = m_riskController->checkOrder(request);
            if (!result.passed)
            {
                setError(result.message);
                emit riskWarning(result.message);
                return;
            }
        }

        // 提交订单
        if (m_tradingService)
        {
            QString orderId = m_tradingService->submitOrder(request);

            emit orderSubmitted(orderId, "卖出开仓");
            LOG_INFO(QString("Sell open order submitted: %1").arg(orderId));
        }
    }

    void TradingViewModel::executeBuyClose()
    {
        if (!canBuyClose())
        {
            setError("Cannot execute buy close - no short position");
            return;
        }

        setStatus(QString("买入平仓 %1 手 @ %2").arg(m_orderVolume).arg(m_orderPrice));

        // 构建订单请求
        OrderRequest request;
        request.instrumentId = m_instrumentId;
        request.direction = TradeDirection::Buy;
        request.openClose = OpenCloseFlag::Close;
        request.volume = qMin(m_orderVolume, m_shortPosition);
        request.price = m_orderPrice;
        request.orderType = static_cast<OrderType>(m_orderType);

        // 提交订单
        if (m_tradingService)
        {
            QString orderId = m_tradingService->submitOrder(request);

            emit orderSubmitted(orderId, "买入平仓");
            LOG_INFO(QString("Buy close order submitted: %1").arg(orderId));
        }
    }

    void TradingViewModel::executeSellClose()
    {
        if (!canSellClose())
        {
            setError("Cannot execute sell close - no long position");
            return;
        }

        setStatus(QString("卖出平仓 %1 手 @ %2").arg(m_orderVolume).arg(m_orderPrice));

        // 构建订单请求
        OrderRequest request;
        request.instrumentId = m_instrumentId;
        request.direction = TradeDirection::Sell;
        request.openClose = OpenCloseFlag::Close;
        request.volume = qMin(m_orderVolume, m_longPosition);
        request.price = m_orderPrice;
        request.orderType = static_cast<OrderType>(m_orderType);

        // 提交订单
        if (m_tradingService)
        {
            QString orderId = m_tradingService->submitOrder(request);

            emit orderSubmitted(orderId, "卖出平仓");
            LOG_INFO(QString("Sell close order submitted: %1").arg(orderId));
        }
    }

    void TradingViewModel::executeRefresh()
    {
        setStatus("刷新数据...");

        // TradingService 暂无 refreshData 方法
        // TODO: 实现刷新逻辑

        clearStatus();
        LOG_DEBUG("Data refreshed");
    }

    // ========== 可执行检查 ==========

    bool TradingViewModel::canTrade() const
    {
        return !m_instrumentId.isEmpty() && m_currentPrice > 0 && m_tradingService;
    }

    bool TradingViewModel::canBuyOpen() const
    {
        return canTrade() && m_availableFund > calculateMargin(m_orderVolume, m_orderPrice);
    }

    bool TradingViewModel::canSellOpen() const
    {
        return canTrade() && m_availableFund > calculateMargin(m_orderVolume, m_orderPrice);
    }

    bool TradingViewModel::canBuyClose() const
    {
        return canTrade() && m_shortPosition > 0;
    }

    bool TradingViewModel::canSellClose() const
    {
        return canTrade() && m_longPosition > 0;
    }

    // ========== 信号处理 ==========

    void TradingViewModel::onPositionUpdated(const PositionInfo& position)
    {
        if (position.instrumentId != m_instrumentId)
        {
            return;
        }

        // 根据方向设置持仓
        if (position.direction == PositionDirection::Long)
        {
            m_longPosition = position.volume;
            m_longAvgPrice = position.avgPrice;
            m_longProfit = position.profit;
        }
        else if (position.direction == PositionDirection::Short)
        {
            m_shortPosition = position.volume;
            m_shortAvgPrice = position.avgPrice;
            m_shortProfit = position.profit;
        }

        updateProfit();
        emit positionChanged();

        // 更新命令状态
        m_buyCloseCommand->updateCanExecute();
        m_sellCloseCommand->updateCanExecute();
    }

    void TradingViewModel::onOrderSubmitted(const QString& orderId)
    {
        LOG_INFO(QString("Order submitted: %1").arg(orderId));
    }

    void TradingViewModel::onOrderFilled(const QString& orderId, const TradeRecord& /*trade*/)
    {
        LOG_INFO(QString("Order filled: %1").arg(orderId));
        clearStatus();
    }

    void TradingViewModel::onOrderRejected(const QString& orderId, const QString& reason)
    {
        setError(QString("订单被拒绝: %1").arg(reason));
        emit orderRejected(reason);
        LOG_WARNING(QString("Order rejected: %1 - %2").arg(orderId, reason));
    }

    void TradingViewModel::updateProfit()
    {
        if (m_longPosition > 0 && m_longAvgPrice > 0)
        {
            m_longProfit = calculateProfit(m_longPosition, m_longAvgPrice, m_currentPrice, PositionDirection::Long);
        }
        else
        {
            m_longProfit = 0.0;
        }

        if (m_shortPosition > 0 && m_shortAvgPrice > 0)
        {
            m_shortProfit = calculateProfit(m_shortPosition, m_shortAvgPrice, m_currentPrice, PositionDirection::Short);
        }
        else
        {
            m_shortProfit = 0.0;
        }

        emit positionChanged();
    }
} // namespace WealthPilot