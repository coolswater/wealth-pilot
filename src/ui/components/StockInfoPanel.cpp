/**
 * @file StockInfoPanel.cpp
 * @brief 股票信息面板实现
 */

#include "StockInfoPanel.h"
#include "core/config/Tokens.h"
#include "ui/components/StyleHelper.h"
#include "core/cache/CacheManager.h"
#include "data/DataStorageService.h"
#include "utils/Logger.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QTimer>
#include <QVariantMap>
#include <QVariantList>

using namespace Tokens;

StockInfoPanel::StockInfoPanel(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("StockInfoPanel");
    setupUI();
    LOG_DEBUG("StockInfoPanel created");
}

StockInfoPanel::~StockInfoPanel() = default;

void StockInfoPanel::setupUI()
{
    setFixedWidth(280);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(Spacing::SM, Spacing::SM, Spacing::SM, Spacing::SM);
    mainLayout->setSpacing(Spacing::SM);

    // 第一行：股票名称和价格
    auto* headerLayout = new QVBoxLayout();
    m_stockNameLabel = new QLabel("--", this);
    m_stockNameLabel->setObjectName("stockNameLabel");
    m_stockNameLabel->setProperty("dataType", "title");
    headerLayout->addWidget(m_stockNameLabel);

    m_priceLabel = new QLabel("--", this);
    m_priceLabel->setObjectName("priceLabel");
    m_priceLabel->setProperty("dataType", "price");
    headerLayout->addWidget(m_priceLabel);

    m_changeLabel = new QLabel("--", this);
    m_changeLabel->setObjectName("changeLabel");
    m_changeLabel->setProperty("dataType", "change");
    headerLayout->addWidget(m_changeLabel);
    mainLayout->addLayout(headerLayout);

    // 第二行：交易状态
    m_statusLabel = new QLabel(QStringLiteral("交易状态: --"), this);
    m_statusLabel->setProperty("dataType", "status");
    mainLayout->addWidget(m_statusLabel);

    // 第三行：委比委差
    m_orderRatioLabel = new QLabel(QStringLiteral("委比: --  委差: --"), this);
    m_orderRatioLabel->setProperty("dataType", "label");
    mainLayout->addWidget(m_orderRatioLabel);

    // 第四行：五档盘口
    auto* orderBookLayout = new QGridLayout();
    for (int i = 0; i < 5; ++i) {
        m_bidLabels[i] = new QLabel(QStringLiteral("买%1: --").arg(5-i), this);
        m_bidLabels[i]->setObjectName("bidLabel");
        m_bidLabels[i]->setProperty("orderType", "bid");
        StyleHelper::refreshStyle(m_bidLabels[i]);

        m_askLabels[i] = new QLabel(QStringLiteral("卖%1: --").arg(i+1), this);
        m_askLabels[i]->setObjectName("askLabel");
        m_askLabels[i]->setProperty("orderType", "ask");
        StyleHelper::refreshStyle(m_askLabels[i]);

        orderBookLayout->addWidget(m_bidLabels[i], i, 0);
        orderBookLayout->addWidget(m_askLabels[i], i, 1);
    }
    mainLayout->addLayout(orderBookLayout);

    // 第五行：详细行情
    m_detailTable = new QTableWidget(14, 2, this);
    m_detailTable->setObjectName("detailTable");
    m_detailTable->horizontalHeader()->setVisible(false);
    m_detailTable->verticalHeader()->setVisible(false);
    m_detailTable->setShowGrid(false);
    mainLayout->addWidget(m_detailTable);

    // 第六行：成交明细
    m_tickTable = new QTableWidget(this);
    m_tickTable->setObjectName("tickTable");
    m_tickTable->setColumnCount(3);
    m_tickTable->setHorizontalHeaderLabels({QStringLiteral("时间"), QStringLiteral("价格"), QStringLiteral("数量")});
    mainLayout->addWidget(m_tickTable, 1);
}

void StockInfoPanel::setStock(const QString& stockCode, const QString& stockName)
{
    m_stockCode = stockCode;
    m_stockNameLabel->setText(stockName.isEmpty() ? stockCode : stockName);
    clearData();
    
    if (!m_dataSource) {
        m_dataSource = new StockDataSource(StockDataSource::Source::Sina, this);
        connect(m_dataSource, &StockDataSource::quotesReceived,
                this, [this](const QVector<StockQuote>& quotes) {
                    if (!quotes.isEmpty()) {
                        onQuoteReceived(quotes.first().symbol, quotes.first());
                    }
                });
    }
    
    // 使用三层缓存加载数据
    loadQuoteWithFallback();
    
    emit stockChanged(stockCode);
}

void StockInfoPanel::updateQuote(const StockQuote& quote)
{
    m_currentQuote = quote;

    // 更新价格
    m_priceLabel->setText(QString::number(quote.lastPrice, 'f', 2));
    updatePriceLabel(m_priceLabel, quote.lastPrice, quote.preClose);

    // 更新涨跌
    QString changeText = QString("%1 (%2%)")
        .arg(QString::number(quote.changeAmount, 'f', 2))
        .arg(QString::number(quote.changePercent, 'f', 2));
    m_changeLabel->setText(changeText);
    updatePriceLabel(m_changeLabel, quote.changeAmount);

    // 更新五档
    for (int i = 0; i < 5; ++i) {
        m_bidLabels[i]->setText(QStringLiteral("买%1: %2").arg(5-i)
            .arg(QString::number(quote.bidPrice[i], 'f', 2)));
        m_askLabels[i]->setText(QStringLiteral("卖%1: %2").arg(i+1)
            .arg(QString::number(quote.askPrice[i], 'f', 2)));
    }

    // 更新委比
    m_orderRatioLabel->setText(QStringLiteral("委比: %1%  委差: %2")
        .arg(QString::number(quote.orderRatio, 'f', 2))
        .arg(QString::number(quote.orderDiff)));
}

void StockInfoPanel::updateTickData(const QVector<TickData>& ticks)
{
    m_tickTable->setRowCount(ticks.size());
    for (int i = 0; i < ticks.size(); ++i) {
        const auto& tick = ticks[i];
        m_tickTable->setItem(i, 0, new QTableWidgetItem(tick.time.toString("hh:mm:ss")));
        m_tickTable->setItem(i, 1, new QTableWidgetItem(QString::number(tick.price, 'f', 2)));
        m_tickTable->setItem(i, 2, new QTableWidgetItem(QString::number(tick.volume)));
    }
}

void StockInfoPanel::clearData()
{
    m_priceLabel->setText("--");
    m_changeLabel->setText("--");
    m_statusLabel->setText(QStringLiteral("交易状态: --"));
    m_orderRatioLabel->setText(QStringLiteral("委比: --  委差: --"));
    for (int i = 0; i < 5; ++i) {
        m_bidLabels[i]->setText(QStringLiteral("买%1: --").arg(5-i));
        m_askLabels[i]->setText(QStringLiteral("卖%1: --").arg(i+1));
    }
    m_tickTable->setRowCount(0);
}

void StockInfoPanel::updatePriceLabel(QLabel* label, double price, double prevPrice)
{
    QString status;
    if (prevPrice > 0) {
        status = (price > prevPrice) ? "up" : (price < prevPrice ? "down" : "flat");
    } else {
        status = (price > 0) ? "up" : (price < 0 ? "down" : "flat");
    }
    label->setProperty("trend", status);
    StyleHelper::refreshStyle(label);
}

QString StockInfoPanel::formatVolume(qint64 volume) const
{
    if (volume >= 100000000) return QString::number(volume / 100000000.0, 'f', 2) + QStringLiteral("亿");
    if (volume >= 10000) return QString::number(volume / 10000.0, 'f', 2) + QStringLiteral("万");
    return QString::number(volume);
}

QString StockInfoPanel::formatAmount(double amount) const
{
    if (amount >= 100000000) return QString::number(amount / 100000000.0, 'f', 2) + QStringLiteral("亿");
    if (amount >= 10000) return QString::number(amount / 10000.0, 'f', 2) + QStringLiteral("万");
    return QString::number(amount, 'f', 2);
}

// ============================================================================
// 三层缓存机制实现
// ============================================================================

void StockInfoPanel::loadQuoteWithFallback()
{
    if (m_stockCode.isEmpty()) return;
    
    LOG_INFO(QString("Loading quote with fallback for %1").arg(m_stockCode));
    
    // 1. 尝试从缓存加载
    if (loadQuoteFromCache()) {
        LOG_INFO("Quote loaded from cache");
        // 缓存命中，后台更新数据
        QTimer::singleShot(100, this, [this]() {
            loadQuoteFromNetwork();
        });
        return;
    }
    
    // 2. 缓存未命中，尝试从数据库加载
    if (loadQuoteFromDatabase()) {
        LOG_INFO("Quote loaded from database");
        // 数据库命中，后台更新数据
        QTimer::singleShot(100, this, [this]() {
            loadQuoteFromNetwork();
        });
        return;
    }
    
    // 3. 都未命中，从网络获取
    loadQuoteFromNetwork();
}

bool StockInfoPanel::loadQuoteFromCache()
{
    auto* cache = CacheManager::instance();
    QString key = quoteCacheKey();
    
    if (!cache->contains(key)) {
        return false;
    }
    
    QVariant data = cache->get(key);
    if (!data.isValid()) {
        return false;
    }
    
    // 解析缓存数据
    QVariantMap map = data.toMap();
    StockQuote quote;
    quote.symbol = map["symbol"].toString();
    quote.name = map["name"].toString();
    quote.lastPrice = map["lastPrice"].toDouble();
    quote.openPrice = map["openPrice"].toDouble();
    quote.highPrice = map["highPrice"].toDouble();
    quote.lowPrice = map["lowPrice"].toDouble();
    quote.preClose = map["preClose"].toDouble();
    quote.volume = map["volume"].toLongLong();
    quote.turnover = map["turnover"].toDouble();
    quote.changeAmount = map["changeAmount"].toDouble();
    quote.changePercent = map["changePercent"].toDouble();
    quote.orderRatio = map["orderRatio"].toDouble();
    quote.orderDiff = map["orderDiff"].toLongLong();
    
    // 解析五档盘口
    QVariantList bidPrices = map["bidPrices"].toList();
    QVariantList bidVolumes = map["bidVolumes"].toList();
    QVariantList askPrices = map["askPrices"].toList();
    QVariantList askVolumes = map["askVolumes"].toList();
    
    for (int i = 0; i < 5 && i < bidPrices.size(); ++i) {
        quote.bidPrice[i] = bidPrices[i].toDouble();
        quote.bidVolume[i] = bidVolumes[i].toLongLong();
        quote.askPrice[i] = askPrices[i].toDouble();
        quote.askVolume[i] = askVolumes[i].toLongLong();
    }
    
    if (quote.isValid()) {
        updateQuote(quote);
        return true;
    }
    
    return false;
}

bool StockInfoPanel::loadQuoteFromDatabase()
{
    // 从数据库加载行情数据
    if (m_stockCode.isEmpty()) return false;
    
    auto* dbService = DataStorageService::instance();
    if (!dbService) return false;
    
    CachedQuoteData cachedData = dbService->getQuoteCache(m_stockCode);
    if (cachedData.symbol.isEmpty() || cachedData.lastPrice <= 0) {
        return false;
    }
    
    // 转换为StockQuote
    m_currentQuote.symbol = cachedData.symbol;
    m_currentQuote.name = cachedData.name;
    m_currentQuote.lastPrice = cachedData.lastPrice;
    
    LOG_INFO(QString("Quote loaded from database for %1: price=%2")
        .arg(m_stockCode).arg(cachedData.lastPrice));
    
    // 更新显示
    updateQuote(m_currentQuote);
    return true;
}

void StockInfoPanel::loadQuoteFromNetwork()
{
    if (!m_dataSource || m_stockCode.isEmpty()) return;
    
    LOG_INFO(QString("Loading quote from network for %1").arg(m_stockCode));
    m_dataSource->requestQuotes({m_stockCode});
}

void StockInfoPanel::saveQuoteToCache()
{
    if (!m_currentQuote.isValid()) return;
    
    auto* cache = CacheManager::instance();
    QString key = quoteCacheKey();
    
    // 序列化行情数据
    QVariantMap map;
    map["symbol"] = m_currentQuote.symbol;
    map["name"] = m_currentQuote.name;
    map["lastPrice"] = m_currentQuote.lastPrice;
    map["openPrice"] = m_currentQuote.openPrice;
    map["highPrice"] = m_currentQuote.highPrice;
    map["lowPrice"] = m_currentQuote.lowPrice;
    map["preClose"] = m_currentQuote.preClose;
    map["volume"] = m_currentQuote.volume;
    map["turnover"] = m_currentQuote.turnover;
    map["changeAmount"] = m_currentQuote.changeAmount;
    map["changePercent"] = m_currentQuote.changePercent;
    map["orderRatio"] = m_currentQuote.orderRatio;
    map["orderDiff"] = m_currentQuote.orderDiff;
    
    // 序列化五档盘口
    QVariantList bidPrices, bidVolumes, askPrices, askVolumes;
    for (int i = 0; i < 5; ++i) {
        bidPrices.append(m_currentQuote.bidPrice[i]);
        bidVolumes.append(m_currentQuote.bidVolume[i]);
        askPrices.append(m_currentQuote.askPrice[i]);
        askVolumes.append(m_currentQuote.askVolume[i]);
    }
    map["bidPrices"] = bidPrices;
    map["bidVolumes"] = bidVolumes;
    map["askPrices"] = askPrices;
    map["askVolumes"] = askVolumes;
    
    // 缓存5分钟
    cache->set(key, map, 300);
    
    LOG_DEBUG(QString("Quote saved to cache: %1").arg(key));
}

void StockInfoPanel::saveQuoteToDatabase()
{
    // 保存到数据库
    if (!m_currentQuote.isValid()) return;
    
    auto* dbService = DataStorageService::instance();
    if (!dbService) return;
    
    // 转换为CachedQuoteData
    CachedQuoteData data;
    data.symbol = m_currentQuote.symbol;
    data.name = m_currentQuote.name;
    data.lastPrice = m_currentQuote.lastPrice;
    
    if (dbService->saveQuoteCache(m_stockCode, data)) {
        LOG_DEBUG(QString("Quote saved to database: %1").arg(m_stockCode));
    } else {
        LOG_WARNING(QString("Failed to save quote to database: %1").arg(m_stockCode));
    }
}

QString StockInfoPanel::quoteCacheKey() const
{
    return QString("quote_%1").arg(m_stockCode);
}

void StockInfoPanel::onQuoteReceived(const QString& symbol, const StockQuote& quote)
{
    if (symbol != m_stockCode) return;
    
    m_currentQuote = quote;
    updateQuote(quote);
    
    // 保存到缓存和数据库
    saveQuoteToCache();
    saveQuoteToDatabase();
    
    LOG_DEBUG(QString("Quote received and cached: %1").arg(symbol));
}
