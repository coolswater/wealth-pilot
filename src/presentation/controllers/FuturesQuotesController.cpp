/**
 * @file FuturesQuotesController.cpp
 * @brief 期货行情控制器实现
 * 
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "FuturesQuotesController.h"
#include "data/models/FuturesQuoteModel.h"
#include "data/models/FuturesQuoteItem.h"
#include "core/services/di/ServiceLocator.h"
#include "shared/utils/Logger.h"

#include <QSortFilterProxyModel>
#include <QRegularExpression>

namespace WealthPilot
{
    FuturesQuotesController::FuturesQuotesController(QObject* parent)
        : ControllerBase(parent)
          , m_proxyModel(new QSortFilterProxyModel(this))
          , m_updateTimer(new QTimer(this))
    {
        LOG_DEBUG("FuturesQuotesController created");
    }

    FuturesQuotesController::~FuturesQuotesController()
    {
        cleanup();
        LOG_DEBUG("FuturesQuotesController destroyed");
    }

    void FuturesQuotesController::initialize()
    {
        ControllerBase::initialize();

        // 获取 CTP 服务
        m_ctpService = getService<CTP::CTPService>();

        // 创建模型
        m_model = new FuturesQuoteModel(this);
        m_proxyModel->setSourceModel(m_model);
        m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
        m_proxyModel->setSortRole(Qt::UserRole);

        // 设置定时更新
        m_updateTimer->setInterval(1000); // 1秒更新一次
        connect(m_updateTimer, &QTimer::timeout, this, &FuturesQuotesController::onUpdateTimer);

        // 连接 CTP 服务信号
        setupConnections();

        // 初始状态
        m_connectionStatus = QStringLiteral("未连接");
        m_isConnected = false;

        LOG_INFO("FuturesQuotesController initialized");
    }

    void FuturesQuotesController::cleanup()
    {
        ControllerBase::cleanup();

        // 停止定时器
        if (m_updateTimer)
        {
            m_updateTimer->stop();
        }

        // 断开 CTP 连接
        if (m_ctpService)
        {
            disconnect(m_ctpService, nullptr, this, nullptr);
        }

        // 清空订阅
        m_subscribedContracts.clear();
        m_mainContracts.clear();

        LOG_INFO("FuturesQuotesController cleaned up");
    }

    void FuturesQuotesController::setupConnections()
    {
        if (!m_ctpService)
        {
            LOG_WARNING("CTPService not available");
            return;
        }

        // 连接状态信号
        connect(m_ctpService, &CTP::CTPService::connected,
                this, &FuturesQuotesController::onCtpConnected);
        connect(m_ctpService, &CTP::CTPService::disconnected,
                this, &FuturesQuotesController::onCtpDisconnected);
        connect(m_ctpService, &CTP::CTPService::errorOccurred,
                this, &FuturesQuotesController::onCtpError);

        // 行情数据信号
        connect(m_ctpService, &CTP::CTPService::marketDataReceived,
                this, &FuturesQuotesController::onMarketDataReceived);
        connect(m_ctpService, &CTP::CTPService::batchMarketDataReceived,
                this, &FuturesQuotesController::onBatchMarketDataReceived);
        connect(m_ctpService, &CTP::CTPService::instrumentQueried,
                this, &FuturesQuotesController::onInstrumentQueried);
    }

    // ========== 数据操作 ==========

    void FuturesQuotesController::refreshData()
    {
        beginOperation("刷新期货行情");

        if (m_ctpService && m_isConnected)
        {
            // 请求所有合约行情
            m_ctpService->queryAllInstruments();
        }
        else
        {
            setError("CTP 未连接");
            endOperation("刷新期货行情", false);
        }
    }

    void FuturesQuotesController::subscribeContract(const QString& instrumentId)
    {
        if (!m_ctpService || !m_isConnected)
        {
            setError("CTP 未连接，无法订阅");
            return;
        }

        m_ctpService->subscribeMarketData(instrumentId);
        m_subscribedContracts.insert(instrumentId);

        emit subscriptionChanged(instrumentId, true);
        LOG_INFO(QString("Subscribed: %1").arg(instrumentId));
    }

    void FuturesQuotesController::unsubscribeContract(const QString& instrumentId)
    {
        if (!m_ctpService || !m_isConnected)
        {
            return;
        }

        m_ctpService->unsubscribeMarketData(instrumentId);
        m_subscribedContracts.remove(instrumentId);

        emit subscriptionChanged(instrumentId, false);
        LOG_INFO(QString("Unsubscribed: %1").arg(instrumentId));
    }

    void FuturesQuotesController::subscribeContracts(const QStringList& instrumentIds)
    {
        for (const QString& id : instrumentIds)
        {
            subscribeContract(id);
        }
    }

    void FuturesQuotesController::setFilterMode(int mode)
    {
        m_filterMode = static_cast<FilterMode>(mode);
        applyFilter();

        int visibleCount = m_proxyModel->rowCount();
        emit filterCompleted(visibleCount, m_model->rowCount());

        LOG_DEBUG(QString("Filter mode set: %1, visible=%2").arg(mode).arg(visibleCount));
    }

    void FuturesQuotesController::setExchangeFilter(const QString& exchange)
    {
        m_exchangeFilter = exchange;
        applyFilter();

        LOG_DEBUG(QString("Exchange filter: %1").arg(exchange));
    }

    void FuturesQuotesController::setProductFilter(const QString& product)
    {
        m_productFilter = product;
        applyFilter();

        LOG_DEBUG(QString("Product filter: %1").arg(product));
    }

    void FuturesQuotesController::searchContracts(const QString& keyword)
    {
        m_searchKeyword = keyword.trimmed();
        applyFilter();

        int visibleCount = m_proxyModel->rowCount();
        LOG_DEBUG(QString("Search: %1, results=%2").arg(keyword).arg(visibleCount));
    }

    void FuturesQuotesController::sortByField(int field, Qt::SortOrder order)
    {
        m_proxyModel->sort(field, order);
        LOG_DEBUG(QString("Sort by field %1, order=%2").arg(field).arg(order));
    }

    // ========== 主力合约 ==========

    void FuturesQuotesController::updateMainContracts()
    {
        identifyMainContracts();

        QStringList mainList = m_mainContracts.values();
        emit mainContractsUpdated(mainList);

        LOG_INFO(QString("Main contracts updated: %1").arg(mainList.size()));
    }

    QStringList FuturesQuotesController::getMainContracts() const
    {
        return m_mainContracts.values();
    }

    bool FuturesQuotesController::isMainContract(const QString& instrumentId) const
    {
        return m_mainContracts.contains(instrumentId);
    }

    void FuturesQuotesController::identifyMainContracts()
    {
        // 根据持仓量识别主力合约
        // 简化实现：按品种分组，取持仓量最大的合约

        QHash<QString, QString> productMainContract; // 品种 -> 主力合约
        QHash<QString, qint64> productMaxVolume; // 品种 -> 最大持仓量

        for (int i = 0; i < m_model->rowCount(); ++i)
        {
            FuturesQuoteItem item = m_model->getQuote(i);

            // 解析品种（去掉月份）
            QString product = item.code.left(item.code.length() - 4);

            if (item.openInterest > productMaxVolume[product])
            {
                productMaxVolume[product] = item.openInterest;
                productMainContract[product] = item.code;
            }
        }

        // 更新主力合约集合
        m_mainContracts.clear();
        for (const QString& contract : productMainContract)
        {
            m_mainContracts.insert(contract);
        }
    }

    // ========== 统计信息 ==========

    int FuturesQuotesController::totalCount() const
    {
        return m_model ? m_model->rowCount() : 0;
    }

    int FuturesQuotesController::filteredCount() const
    {
        return m_proxyModel ? m_proxyModel->rowCount() : 0;
    }

    int FuturesQuotesController::subscribedCount() const
    {
        return m_subscribedContracts.size();
    }

    QVariantMap FuturesQuotesController::getMarketStatistics() const
    {
        int upCount = 0;
        int downCount = 0;
        int flatCount = 0;
        qint64 totalVolume = 0;
        qint64 totalOpenInterest = 0;

        for (int i = 0; i < m_model->rowCount(); ++i)
        {
            FuturesQuoteItem item = m_model->getQuote(i);

            if (item.changePercent > 0.01) upCount++;
            else if (item.changePercent < -0.01) downCount++;
            else flatCount++;

            totalVolume += item.volume;
            totalOpenInterest += item.openInterest;
        }

        return {
            {"upCount", upCount},
            {"downCount", downCount},
            {"flatCount", flatCount},
            {"totalVolume", totalVolume},
            {"totalOpenInterest", totalOpenInterest},
            {"total", m_model->rowCount()}
        };
    }

    // ========== 私有方法 ==========

    void FuturesQuotesController::applyFilter()
    {
        // 构建筛选正则表达式
        QString pattern;

        // 交易所筛选
        if (!m_exchangeFilter.isEmpty())
        {
            if (m_exchangeFilter == "SHFE")
            {
                pattern = "^[a-z]+[0-9]+\\.SHFE$";
            }
            else if (m_exchangeFilter == "DCE")
            {
                pattern = "^[a-z]+[0-9]+\\.DCE$";
            }
            else if (m_exchangeFilter == "CZCE")
            {
                pattern = "^[A-Z]+[0-9]+\\.CZCE$";
            }
            else if (m_exchangeFilter == "CFFEX")
            {
                pattern = "^[A-Z]+[0-9]+\\.CFFEX$";
            }
            else if (m_exchangeFilter == "INE")
            {
                pattern = "^[a-z]+[0-9]+\\.INE$";
            }
        }

        // 品种筛选
        if (!m_productFilter.isEmpty())
        {
            pattern = m_productFilter + "[0-9]+";
        }

        // 设置筛选
        if (!pattern.isEmpty())
        {
            m_proxyModel->setFilterRegularExpression(QRegularExpression(pattern));
        }
        else
        {
            m_proxyModel->setFilterRegularExpression(QRegularExpression());
        }

        // 搜索筛选
        if (!m_searchKeyword.isEmpty())
        {
            m_proxyModel->setFilterFixedString(m_searchKeyword);
        }

        // 主力合约筛选
        if (m_filterMode == FilterMode::MainContracts)
        {
            // 需要自定义筛选器
        }

        // 已订阅筛选
        if (m_filterMode == FilterMode::Subscribed)
        {
            // 需要自定义筛选器
        }
    }

    void FuturesQuotesController::updateActivity(const QString& instrumentId, qint64 volume, qint64 turnover)
    {
        qint64 lastVolume = m_lastVolumeCache.value(instrumentId, 0);
        qint64 volumeChange = volume - lastVolume;

        // 活跃度 = 成交量变化 + 成交额权重
        int activityScore = static_cast<int>(volumeChange / 100) + static_cast<int>(turnover / 1000000);

        m_activityScores[instrumentId] = activityScore;
        m_lastVolumeCache[instrumentId] = volume;
    }

    // ========== CTP 回调 ==========

    void FuturesQuotesController::onCtpConnected()
    {
        m_isConnected = true;
        m_connectionStatus = QStringLiteral("已连接");

        emit connectionStatusChanged(m_connectionStatus, true);

        // 启动定时更新
        m_updateTimer->start();

        // 查询合约
        m_ctpService->queryAllInstruments();

        LOG_INFO("CTP connected");
    }

    void FuturesQuotesController::onCtpDisconnected()
    {
        m_isConnected = false;
        m_connectionStatus = QStringLiteral("已断开");

        emit connectionStatusChanged(m_connectionStatus, false);

        // 停止定时更新
        m_updateTimer->stop();

        LOG_INFO("CTP disconnected");
    }

    void FuturesQuotesController::onCtpError(const QString& error)
    {
        setError(QString("CTP 错误: %1").arg(error));
        m_connectionStatus = QStringLiteral("错误");

        emit connectionStatusChanged(m_connectionStatus, false);

        LOG_ERROR(QString("CTP error: %1").arg(error));
    }

    void FuturesQuotesController::onMarketDataReceived(const CTP::MarketData& data)
    {
        if (!m_model) return;

        // 更新模型
        m_model->updateQuote(data);

        // 更新活跃度
        updateActivity(data.instrumentID, data.volume, data.turnover);

        emit contractUpdated(data.instrumentID);
    }

    void FuturesQuotesController::onBatchMarketDataReceived(const QList<CTP::MarketData>& dataList)
    {
        if (!m_model) return;

        // 批量更新
        for (const CTP::MarketData& data : dataList)
        {
            m_model->updateQuote(data);
            updateActivity(data.instrumentID, data.volume, data.turnover);
        }

        emit dataRefreshed(dataList.size());
        endOperation("刷新期货行情", true);

        LOG_DEBUG(QString("Batch data received: %1").arg(dataList.size()));
    }

    void FuturesQuotesController::onInstrumentQueried(const QString& instrumentId,
                                                      const QString& exchangeId,
                                                      const QString& instrumentName,
                                                      double priceTick,
                                                      int volumeMultiple)
    {
        if (!m_model) return;

        // 添加合约到模型
        FuturesQuoteItem item;
        item.code = instrumentId;
        item.name = instrumentName;
        item.exchange = exchangeId;
        item.priceTick = priceTick;
        item.volumeMultiple = volumeMultiple;

        m_model->addQuote(item);

        LOG_DEBUG(QString("Instrument queried: %1 %2").arg(instrumentId, instrumentName));
    }

    void FuturesQuotesController::onUpdateTimer()
    {
        // 定期刷新活跃度排序等
        if (m_filterMode == FilterMode::Active)
        {
            // 按活跃度重新排序
            m_proxyModel->sort(5, Qt::DescendingOrder); // 假设第5列是活跃度
        }
    }
} // namespace WealthPilot