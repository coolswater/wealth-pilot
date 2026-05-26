/**
 * @file DashboardPageRefactored.cpp
 * @brief 金融行情综合看板页面实现 - 重构版
 *
 * @details 重构要点：
 * 1. 组件化拆分，主文件只负责组装
 * 2. 统一使用 DataHub 数据调度
 * 3. 移除独立 QTimer
 */

#include "DashboardPageRefactored.h"
#include "components/IndexPanel.h"
#include "components/RankGridPanel.h"
#include "components/InfoPanel.h"
#include "DashboardTypes.h"

#include "infrastructure/config/Tokens.h"
#include "presentation/styles/ThemeManager.h"
#include "data/market/StockDataSource.h"
#include "data/DataStorageService.h"
#include "core/services/cache/CacheManager.h"
#include "shared/utils/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QLabel>
#include <QTimer>

namespace WealthPilot {

// ============================================================================
// PIMPL 实现
// ============================================================================

struct DashboardPage::Impl {
    // 组件
    IndexPanel* indexPanel = nullptr;           ///< 指数面板
    RankGridPanel* rankGridPanel = nullptr;     ///< 六宫格排行榜
    InfoPanel* infoPanel = nullptr;             ///< 底部信息面板

    // UI 元素
    QLabel* timeLabel = nullptr;
    QLabel* statusLabel = nullptr;

    // 数据源（移除独立定时器，使用 DataHub）
    StockDataSource* indexDataSource = nullptr;
    StockDataSource* rankDataSource = nullptr;
    StockDataSource* watchlistDataSource = nullptr;

    // 预设数据
    QStringList indexSymbols = {
        "sh000001", "sz399001", "sz399006", "sh000688",
        "sh000016", "sh000300", "bj899050"
    };

    QStringList hotStockSymbols = {
        "sh600519", "sh601318", "sz000858", "sz000001", "sh600036",
        "sz002594", "sz300750", "sh601012", "sz000333", "sh600900",
        "sz002415", "sh601888", "sz000002", "sh600276", "sz002304",
        "sh601166", "sz000651", "sh601398", "sz002352", "sh600030",
        "sz000725", "sh601288", "sz002475", "sh600000", "sz000063"
    };

    QStringList watchlistSymbols = {
        "sh600519", "sh601318", "sz000858", "sz000001", "sh600036",
        "sz002594", "sz300750", "sh601012", "sz000333", "sh600900"
    };

    // 数据缓存
    QVector<IndexData> indexData;
};

// ============================================================================
// DashboardPage 实现
// ============================================================================

DashboardPage::DashboardPage(QWidget* parent)
    : DataHubPageBase(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
    setObjectName("DashboardPage");
}

DashboardPage::~DashboardPage() = default;

void DashboardPage::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 1. 头部工具栏
    setupHeader();

    // 2. 主分割器
    auto* mainSplitter = new QSplitter(Qt::Vertical, this);
    mainSplitter->setHandleWidth(1);
    mainSplitter->setChildrenCollapsible(false);
    mainSplitter->setStyleSheet(
        QString("QSplitter::handle { background-color: %1; }")
            .arg(Tokens::Colors::Border));

    // 3. 指数面板
    d->indexPanel = new IndexPanel(this);
    mainSplitter->addWidget(d->indexPanel);

    // 4. 六宫格排行榜
    d->rankGridPanel = new RankGridPanel(this);
    mainSplitter->addWidget(d->rankGridPanel);

    // 5. 底部信息面板
    d->infoPanel = new InfoPanel(this);
    mainSplitter->addWidget(d->infoPanel);

    // 设置分割比例
    mainSplitter->setSizes({120, 350, 280});

    mainLayout->addWidget(mainSplitter, 1);
}

void DashboardPage::setupHeader()
{
    ThemeColors theme = ThemeManager::instance()->currentTheme();

    auto* header = new QFrame(this);
    header->setFixedHeight(48);
    header->setStyleSheet(QString("background-color: %1; border-bottom: 1px solid %2;")
        .arg(theme.bgElevated, theme.border));

    auto* layout = new QHBoxLayout(header);
    layout->setContentsMargins(16, 0, 16, 0);
    layout->setSpacing(16);

    // 标题
    auto* titleLabel = new QLabel(QStringLiteral("行情看板"), header);
    titleLabel->setStyleSheet(QString("font-size: 18px; font-weight: bold; color: %1;")
        .arg(theme.textPrimary));
    layout->addWidget(titleLabel);

    layout->addStretch();

    // 时间显示
    d->timeLabel = new QLabel(header);
    d->timeLabel->setStyleSheet(QString("color: %1; font-size: 13px;")
        .arg(theme.textSecondary));
    layout->addWidget(d->timeLabel);

    layout->addSpacing(20);

    // 状态信息
    d->statusLabel = new QLabel(header);
    d->statusLabel->setStyleSheet(QString("color: %1; font-size: 13px;")
        .arg(theme.textTertiary));
    layout->addWidget(d->statusLabel);
}

void DashboardPage::initializePage()
{
    if (isInitialized()) return;

    // 1. 设置 DataHub 订阅
    setupDataHubSubscriptions();

    // 2. 初始化数据存储服务
    if (!DataStorageService::instance()->isInitialized()) {
        DataStorageService::instance()->initialize();
    }

    CacheManager::instance()->initialize();

    // 3. 设置信号连接
    setupConnections();

    // 4. 加载数据
    loadDataWithFallback();

    setInitialized(true);
    LOG_DEBUG("DashboardPage initialized (refactored version)");
}

void DashboardPage::setupDataHubSubscriptions()
{
    // 订阅指数数据 - 使用 DataHub 统一调度
    for (const QString& symbol : d->indexSymbols) {
        subscribeQuote(symbol, [this, symbol](const StockQuote& quote) {
            IndexData data;
            data.code = quote.symbol;
            data.name = quote.name;
            data.current = quote.lastPrice;
            data.change = quote.changeAmount;
            data.changePercent = quote.changePercent;
            d->indexData.append(data);
        });
    }

    // 订阅排行榜模式
    dataHub().subscribePattern(this, "market:rank:*",
        [this](const QString& topic, const QVariant& value) {
            Q_UNUSED(topic)
            onRankDataReceived(value);
        });

    // 订阅自选股
    dataHub().subscribePattern(this, "watchlist:*",
        [this](const QString& topic, const QVariant& value) {
            Q_UNUSED(topic)
            onWatchlistDataReceived(value);
        });

    LOG_INFO("[DashboardPage] DataHub subscriptions setup complete");
}

void DashboardPage::setupConnections()
{
    // 组件信号转发
    connect(d->rankGridPanel, &RankGridPanel::stockDoubleClicked,
            this, &DashboardPage::navigateToStockKLine);

    connect(d->infoPanel, &InfoPanel::stockDoubleClicked,
            this, &DashboardPage::navigateToStockKLine);

    // 主题切换
    ThemeManager::instance()->registerThemeChangeListener(this, [this]() {
        updateTheme();
    });
}

void DashboardPage::loadDataWithFallback()
{
    // 优先级：缓存 -> 数据库 -> 网络
    if (loadFromCache()) {
        LOG_DEBUG("Data loaded from cache");
        return;
    }

    if (loadFromDatabase()) {
        LOG_DEBUG("Data loaded from database");
        return;
    }

    loadFromNetwork();
}

bool DashboardPage::loadFromCache()
{
    auto* cache = CacheManager::instance();
    bool loaded = false;

    // 加载指数缓存
    if (cache->contains("dashboard_index_data")) {
        auto indexVariant = cache->get("dashboard_index_data");
        if (indexVariant.canConvert<QVector<IndexData>>()) {
            d->indexData = indexVariant.value<QVector<IndexData>>();
            d->indexPanel->setData(d->indexData);
            loaded = true;
        }
    }

    return loaded;
}

bool DashboardPage::loadFromDatabase()
{
    auto* storage = DataStorageService::instance();
    if (!storage->hasLocalData()) {
        return false;
    }

    // 加载指数历史数据
    auto indexHistory = storage->getLatestIndexData();
    if (!indexHistory.isEmpty()) {
        d->indexData.clear();
        for (const auto& hist : indexHistory) {
            IndexData data;
            data.code = hist.code;
            data.name = hist.name;
            data.current = hist.closePrice;
            data.change = hist.closePrice * hist.changePercent / 100.0;
            data.changePercent = hist.changePercent;
            d->indexData.append(data);
        }
        d->indexPanel->setData(d->indexData);
    }

    return !d->indexData.isEmpty();
}

void DashboardPage::loadFromNetwork()
{
    LOG_INFO("Loading data from network...");

    // 初始化数据源
    if (!d->indexDataSource) {
        d->indexDataSource = new StockDataSource(StockDataSource::Source::Sina, this);
        d->rankDataSource = new StockDataSource(StockDataSource::Source::Sina, this);
        d->watchlistDataSource = new StockDataSource(StockDataSource::Source::Sina, this);

        // 连接信号 - 使用 DataHub 发布数据
        connect(d->indexDataSource, &StockDataSource::quotesReceived,
                this, [this](const QVector<StockQuote>& quotes) {
                    // 发布到 DataHub
                    for (const auto& q : quotes) {
                        dataHub().publish(
                            QString("market:quote:%1").arg(q.symbol),
                            QVariant::fromValue(q));
                    }
                    saveToCache();
                    saveToDatabase();
                });
    }

    // 请求初始数据
    d->indexDataSource->requestQuotes(d->indexSymbols);
    d->rankDataSource->requestQuotes(d->hotStockSymbols);
    d->watchlistDataSource->requestQuotes(d->watchlistSymbols);

    // 注意：不再使用 startAutoRefresh，改为 DataHub 统一调度
    // DataHub 会根据订阅情况自动刷新

    d->statusLabel->setText(QStringLiteral("正在从网络获取数据..."));
}

void DashboardPage::refreshData()
{
    // 通过 DataHub 请求刷新
    for (const QString& symbol : d->indexSymbols) {
        dataHub().requestRefresh(QString("market:quote:%1").arg(symbol));
    }
}

void DashboardPage::saveToCache()
{
    auto* cache = CacheManager::instance();
    if (!d->indexData.isEmpty()) {
        cache->set("dashboard_index_data", QVariant::fromValue(d->indexData), 300);
    }
}

void DashboardPage::saveToDatabase()
{
    auto* storage = DataStorageService::instance();
    // 保存行情缓存
    for (const auto& data : d->indexData) {
        IndexHistoryData hist;
        hist.code = data.code;
        hist.name = data.name;
        hist.closePrice = data.current;
        hist.changePercent = data.changePercent;
        hist.volume = static_cast<long long>(data.volume);
        hist.amount = data.amount;
        storage->saveIndexHistory(hist);
    }
}

void DashboardPage::updateTheme()
{
    ThemeColors theme = ThemeManager::instance()->currentTheme();
    setStyleSheet(QString("background-color: %1;").arg(theme.bgPrimary));

    d->timeLabel->setStyleSheet(QString("color: %1; font-size: 13px;")
        .arg(theme.textSecondary));
    d->statusLabel->setStyleSheet(QString("color: %1; font-size: 13px;")
        .arg(theme.textTertiary));
}

// ============================================================================
// DataHub 数据回调
// ============================================================================

void DashboardPage::onIndexDataReceived(const QVariant& data)
{
    if (data.canConvert<StockQuote>()) {
        auto quote = data.value<StockQuote>();
        IndexData idx;
        idx.code = quote.symbol;
        idx.name = quote.name;
        idx.current = quote.lastPrice;
        idx.change = quote.changeAmount;
        idx.changePercent = quote.changePercent;

        d->indexPanel->updateIndex(quote.symbol, idx);
    }
}

void DashboardPage::onRankDataReceived(const QVariant& data)
{
    // 处理排行榜数据
    Q_UNUSED(data)
}

void DashboardPage::onWatchlistDataReceived(const QVariant& data)
{
    // 处理自选股数据
    Q_UNUSED(data)
}

void DashboardPage::onNewsDataReceived(const QVariant& data)
{
    // 处理新闻数据
    Q_UNUSED(data)
}

void DashboardPage::onMoneyFlowDataReceived(const QVariant& data)
{
    // 处理资金流向数据
    Q_UNUSED(data)
}

} // namespace WealthPilot
