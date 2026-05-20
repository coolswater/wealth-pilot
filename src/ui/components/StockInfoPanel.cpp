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
#include <QAbstractItemView>

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
    setStyleSheet(QString("background-color: %1;").arg(Tokens::Colors::BgBase));

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ========== 顶部：股票信息 ==========
    auto* headerWidget = new QWidget(this);
    headerWidget->setStyleSheet(QString("background-color: %1;").arg(Tokens::Colors::BgElevated));
    auto* headerLayout = new QVBoxLayout(headerWidget);
    headerLayout->setContentsMargins(12, 12, 12, 12);
    headerLayout->setSpacing(4);

    // 股票名称
    m_stockNameLabel = new QLabel("--", headerWidget);
    m_stockNameLabel->setStyleSheet(QString("color: %1; font-size: 16px; font-weight: bold;").arg(Tokens::Colors::TextPrimary));
    headerLayout->addWidget(m_stockNameLabel);

    // 价格
    m_priceLabel = new QLabel("--", headerWidget);
    m_priceLabel->setStyleSheet(QString("color: %1; font-size: 24px; font-weight: bold;").arg(Tokens::Colors::TextPrimary));
    headerLayout->addWidget(m_priceLabel);

    // 涨跌
    m_changeLabel = new QLabel("--", headerWidget);
    m_changeLabel->setStyleSheet(QString("color: %1; font-size: 14px;").arg(Tokens::Colors::TextSecondary));
    headerLayout->addWidget(m_changeLabel);

    mainLayout->addWidget(headerWidget);

    // ========== 五档盘口 ==========
    auto* orderBookWidget = new QWidget(this);
    orderBookWidget->setStyleSheet(QString("background-color: %1; border-top: 1px solid %2; border-bottom: 1px solid %2;")
        .arg(Tokens::Colors::BgElevated, Tokens::Colors::Border));
    auto* orderBookLayout = new QVBoxLayout(orderBookWidget);
    orderBookLayout->setContentsMargins(8, 8, 8, 8);
    orderBookLayout->setSpacing(2);

    // 五档标题
    auto* orderBookTitle = new QLabel(QStringLiteral("五档盘口"), orderBookWidget);
    orderBookTitle->setStyleSheet(QString("color: %1; font-size: 12px; font-weight: bold; padding-bottom: 4px;").arg(Tokens::Colors::TextSecondary));
    orderBookLayout->addWidget(orderBookTitle);

    // 五档表格（卖5-卖1，买1-买5）
    for (int i = 4; i >= 0; --i) {
        auto* rowLayout = new QHBoxLayout();
        rowLayout->setSpacing(4);

        // 卖盘标签
        auto* askLabel = new QLabel(QStringLiteral("卖%1").arg(i+1), orderBookWidget);
        askLabel->setStyleSheet(QString("color: %1; font-size: 11px; width: 30px;").arg(Tokens::Colors::TextSecondary));
        askLabel->setFixedWidth(30);
        rowLayout->addWidget(askLabel);

        // 卖盘价格
        m_askLabels[i] = new QLabel("--", orderBookWidget);
        m_askLabels[i]->setStyleSheet(QString("color: %1; font-size: 12px; font-weight: bold;").arg(Tokens::Colors::Danger));
        m_askLabels[i]->setAlignment(Qt::AlignCenter);
        rowLayout->addWidget(m_askLabels[i], 1);

        // 买盘价格
        m_bidLabels[i] = new QLabel("--", orderBookWidget);
        m_bidLabels[i]->setStyleSheet(QString("color: %1; font-size: 12px; font-weight: bold;").arg(Tokens::Colors::Success));
        m_bidLabels[i]->setAlignment(Qt::AlignCenter);
        rowLayout->addWidget(m_bidLabels[i], 1);

        // 买盘标签
        auto* bidLabel = new QLabel(QStringLiteral("买%1").arg(i+1), orderBookWidget);
        bidLabel->setStyleSheet(QString("color: %1; font-size: 11px; width: 30px;").arg(Tokens::Colors::TextSecondary));
        bidLabel->setFixedWidth(30);
        rowLayout->addWidget(bidLabel);

        orderBookLayout->addLayout(rowLayout);
    }

    mainLayout->addWidget(orderBookWidget);

    // ========== 详细行情 ==========
    auto* detailWidget = new QWidget(this);
    detailWidget->setStyleSheet(QString("background-color: %1; border-bottom: 1px solid %2;")
        .arg(Tokens::Colors::BgElevated, Tokens::Colors::Border));
    auto* detailLayout = new QGridLayout(detailWidget);
    detailLayout->setContentsMargins(8, 8, 8, 8);
    detailLayout->setSpacing(4);

    // 详细行情数据
    QStringList detailLabels = {
        QStringLiteral("今开"), QStringLiteral("最高"),
        QStringLiteral("昨收"), QStringLiteral("最低"),
        QStringLiteral("成交量"), QStringLiteral("成交额"),
        QStringLiteral("换手率"), QStringLiteral("量比")
    };

    for (int i = 0; i < detailLabels.size(); ++i) {
        auto* label = new QLabel(detailLabels[i], detailWidget);
        label->setStyleSheet(QString("color: %1; font-size: 11px;").arg(Tokens::Colors::TextSecondary));
        detailLayout->addWidget(label, i / 2, (i % 2) * 2);

        auto* value = new QLabel("--", detailWidget);
        value->setStyleSheet(QString("color: %1; font-size: 11px;").arg(Tokens::Colors::TextPrimary));
        detailLayout->addWidget(value, i / 2, (i % 2) * 2 + 1);
    }

    mainLayout->addWidget(detailWidget);

    // ========== 成交明细 ==========
    auto* tickWidget = new QWidget(this);
    tickWidget->setStyleSheet(QString("background-color: %1;").arg(Tokens::Colors::BgElevated));
    auto* tickLayout = new QVBoxLayout(tickWidget);
    tickLayout->setContentsMargins(8, 8, 8, 8);
    tickLayout->setSpacing(4);

    // 成交明细标题
    auto* tickTitle = new QLabel(QStringLiteral("成交明细"), tickWidget);
    tickTitle->setStyleSheet(QString("color: %1; font-size: 12px; font-weight: bold;").arg(Tokens::Colors::TextSecondary));
    tickLayout->addWidget(tickTitle);

    // 成交明细表格
    m_tickTable = new QTableWidget(tickWidget);
    m_tickTable->setColumnCount(3);
    m_tickTable->setHorizontalHeaderLabels({QStringLiteral("时间"), QStringLiteral("价格"), QStringLiteral("数量")});
    m_tickTable->setStyleSheet(QString(R"(
        QTableWidget {
            background-color: transparent;
            border: none;
            gridline-color: %1;
        }
        QTableWidget::item {
            padding: 2px;
            font-size: 11px;
        }
        QHeaderView::section {
            background-color: transparent;
            color: %2;
            border: none;
            padding: 4px;
            font-size: 11px;
        }
    )").arg(Tokens::Colors::Border, Tokens::Colors::TextSecondary));
    m_tickTable->horizontalHeader()->setStretchLastSection(true);
    m_tickTable->verticalHeader()->setVisible(false);
    m_tickTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tickTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tickLayout->addWidget(m_tickTable, 1);

    mainLayout->addWidget(tickWidget, 1);

    // 隐藏不需要的控件
    m_statusLabel = new QLabel(this);
    m_statusLabel->hide();
    m_orderRatioLabel = new QLabel(this);
    m_orderRatioLabel->hide();
    m_detailTable = new QTableWidget(this);
    m_detailTable->hide();
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
    QString changeText;
    if (quote.changePercent >= 0) {
        changeText = QString("+%1 (+%2%)")
            .arg(QString::number(quote.changeAmount, 'f', 2))
            .arg(QString::number(quote.changePercent, 'f', 2));
        m_changeLabel->setStyleSheet(QString("color: %1; font-size: 14px;").arg(Tokens::Colors::Danger));
    } else {
        changeText = QString("%1 (%2%)")
            .arg(QString::number(quote.changeAmount, 'f', 2))
            .arg(QString::number(quote.changePercent, 'f', 2));
        m_changeLabel->setStyleSheet(QString("color: %1; font-size: 14px;").arg(Tokens::Colors::Success));
    }
    m_changeLabel->setText(changeText);

    // 更新五档（只显示价格）
    for (int i = 0; i < 5; ++i) {
        m_bidLabels[i]->setText(QString::number(quote.bidPrice[i], 'f', 2));
        m_askLabels[i]->setText(QString::number(quote.askPrice[i], 'f', 2));
    }
}

void StockInfoPanel::updateTickData(const QVector<TickData>& ticks)
{
    m_tickTable->setRowCount(ticks.size());
    for (int i = 0; i < ticks.size(); ++i) {
        const auto& tick = ticks[i];
        
        // 时间
        auto* timeItem = new QTableWidgetItem(tick.time.toString("hh:mm:ss"));
        timeItem->setTextAlignment(Qt::AlignCenter);
        timeItem->setForeground(QColor(Tokens::Colors::TextSecondary));
        m_tickTable->setItem(i, 0, timeItem);
        
        // 价格（根据买卖方向显示颜色）
        auto* priceItem = new QTableWidgetItem(QString::number(tick.price, 'f', 2));
        priceItem->setTextAlignment(Qt::AlignCenter);
        if (tick.direction == WealthPilot::TradeDirection::Buy) {
            priceItem->setForeground(QColor(Tokens::Colors::Danger));
        } else if (tick.direction == WealthPilot::TradeDirection::Sell) {
            priceItem->setForeground(QColor(Tokens::Colors::Success));
        }
        m_tickTable->setItem(i, 1, priceItem);
        
        // 数量
        auto* volumeItem = new QTableWidgetItem(QString::number(tick.volume));
        volumeItem->setTextAlignment(Qt::AlignCenter);
        volumeItem->setForeground(QColor(Tokens::Colors::TextPrimary));
        m_tickTable->setItem(i, 2, volumeItem);
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
