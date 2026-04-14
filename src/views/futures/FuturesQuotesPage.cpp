/**
 * @file FuturesQuotesPage.cpp
 * @brief 期货行情页面 - CTP实时行情对接实现（含活跃合约过滤）
 * @note 关键特性：
 *   1. 自动查询所有合约并订阅
 *   2. 零拷贝优化：CTP数据直接转换为FuturesQuoteItem
 *   3. 连接状态管理：自动重连、合约订阅、断线恢复
 *   4. 线程安全：所有CTP回调通过QueuedConnection切换到UI线程
 *   5. 【新增】智能合约过滤：自动过滤过期合约、不活跃合约
 *   6. 【新增】主力合约识别与优先显示
 */

#include "FuturesQuotesPage.h"
#include "utils/Logger.h"
#include "models/FuturesQuoteModel.h"
#include "models/FuturesQuoteDelegate.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QHeaderView>
#include <QMessageBox>
#include <QTimer>
#include <QSortFilterProxyModel>
#include <QComboBox>
#include <QDate>
#include <QRegularExpression>
#include <QDateTime>

#include <atomic>
#include <optional>

// C++17 结构化绑定辅助
using CtpConfig = std::tuple<QString, QString, QString, QString, QString>;

// 【新增】活跃度配置常量
namespace ActivityConfig {
    constexpr int MIN_VOLUME = 10;           // 最小成交量阈值
    constexpr int MIN_OPEN_INTEREST = 50;    // 最小持仓量阈值
    constexpr int MIN_DAILY_VOLUME = 100;    // 最小日成交量（用于判断主力）
    constexpr int EXPIRY_MONTHS_AHEAD = 3;   // 显示未来N个月的合约
    constexpr int MAX_DISPLAY_CONTRACTS = 100; // 最大显示合约数
}

// 【新增】自定义代理模型类，支持活跃度过滤
class ActivityFilterProxyModel : public QSortFilterProxyModel {
public:
    explicit ActivityFilterProxyModel(QObject* parent = nullptr)
        : QSortFilterProxyModel(parent), m_filterMode(1) {}

    // 设置过滤模式
    void setActivityFilterMode(int mode) {
        if (m_filterMode != mode) {
            m_filterMode = mode;
            invalidate(); // 使用 invalidate() 替代 invalidateFilter()
        }
    }

    int activityFilterMode() const { return m_filterMode; }

protected:
    // 【新增】重写过滤判断，支持活跃度过滤
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override {
        // 先执行默认的文本过滤
        if (!QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent)) {
            return false;
        }

        // 获取活跃度状态（假设存储在 Qt::UserRole + 1 角色中）
        QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
        int activityStatus = sourceModel()->data(index, Qt::UserRole + 1).toInt();
        bool isMainContract = sourceModel()->data(index, Qt::UserRole + 2).toBool();

        switch (m_filterMode) {
            case 0: // ShowAll
                return true;
            case 1: // ShowActiveOnly
                return activityStatus > 0; // 活跃状态
            case 2: // ShowMainOnly
                return isMainContract;
            case 3: // ShowHighLiquidity
                return activityStatus >= 2; // 高流动性状态
            default:
                return true;
        }
    }

private:
    int m_filterMode; // 0:全部, 1:仅活跃, 2:仅主力, 3:高流动性
};

class FuturesQuotesPage::Impl {
public:
    // UI 组件
    QTableView *m_tableView = nullptr;
    FuturesQuoteModel *m_model = nullptr;
    ActivityFilterProxyModel *m_proxyModel = nullptr;  // 【修改】使用自定义代理模型
    QLabel *m_statusLabel = nullptr;
    QLineEdit *m_contractInput = nullptr;
    QLineEdit *m_filterInput = nullptr;
    QPushButton *m_subscribeBtn = nullptr;
    QComboBox *m_activityFilter = nullptr;

    // CTP客户端（C++17智能指针管理）
    std::unique_ptr<CTP::CTPService> m_CTPService;

    // 状态控制
    std::atomic<bool> m_isCtpConnected{false};
    std::atomic<bool> m_isProcessing{false};
    std::atomic<int> m_tickCount{0};
    std::atomic<int> m_instrumentCount{0};

    // 数据缓存
    QVector<FuturesQuoteItem> m_pendingUpdates;
    QMutex m_pendingMutex;

    // 已订阅合约列表
    QSet<QString> m_subscribedContracts;
    QMutex m_contractMutex;

    // 合约信息缓存
    struct InstrumentInfo {
        QString instrumentId;
        QString exchangeId;
        QString instrumentName;
        double priceTick{};
        int volumeMultiple{};
        QDate expiryDate;
        QString underlyingProduct;
        bool isMainContract = false;
    };
    QMap<QString, InstrumentInfo> m_instruments;
    QMutex m_instrumentsMutex;

    // 活跃度跟踪
    struct ContractActivity {
        qint64 lastUpdateTime = 0;
        int totalVolume = 0;
        int openInterest = 0;
        double turnover = 0.0;
        int updateCount = 0;
        bool isExpired = false;
        bool isActive = false;
    };
    QMap<QString, ContractActivity> m_activityMap;
    QMutex m_activityMutex;

    // 品种主力合约映射
    QMap<QString, QString> m_mainContracts;
    QMutex m_mainContractMutex;

    // 性能监控
    QTimer *m_flushTimer = nullptr;
    std::atomic<int> m_quoteSequence{0};

    // 页面可见性控制
    std::atomic<bool> m_isVisible{true};
};

FuturesQuotesPage::FuturesQuotesPage(QWidget* parent)
    : BasePage(parent)
    , d(std::make_unique<Impl>())
{
    d->m_model = new FuturesQuoteModel(this);

    // 【新增】初始化示例数据
    initializeSampleData();

    // 【修改】使用自定义代理模型
    d->m_proxyModel = new ActivityFilterProxyModel(this);
    d->m_proxyModel->setSourceModel(d->m_model);
    d->m_proxyModel->setFilterKeyColumn(0);
    d->m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    d->m_proxyModel->setFilterRole(Qt::DisplayRole); // 默认按显示文本过滤

    d->m_CTPService = std::make_unique<CTP::CTPService>(this);
    d->m_flushTimer = new QTimer(this);

    setupUI();
    setupConnections();
    setupCtpConnections();

    LOG_INFO("FuturesQuotesPage created with CTP support and activity filtering");
}

FuturesQuotesPage::~FuturesQuotesPage() = default;

QString FuturesQuotesPage::pageId() const
{
    return QStringLiteral("FuturesQuotesPage");
}

void FuturesQuotesPage::onPageActivated(const QVariantMap &params)
{
    LOG_INFO("FuturesQuotesPage activated");
    d->m_isVisible.store(true);

    if (!d->m_isCtpConnected.load()) {
        LOG_WARNING("CTP not connected, attempting reconnection...");
        if (d->m_CTPService) {
            d->m_CTPService->setupConnections();
        }
    }

    if (d->m_flushTimer && !d->m_flushTimer->isActive()) {
        d->m_flushTimer->start(500);
    }

    updateMainContracts();
}

void FuturesQuotesPage::onPageDeactivated()
{
    LOG_INFO("FuturesQuotesPage deactivated");
    d->m_isVisible.store(false);

    if (d->m_flushTimer) {
        d->m_flushTimer->stop();
    }

    {
        QMutexLocker locker(&d->m_pendingMutex);
        d->m_pendingUpdates.clear();
    }
}

void FuturesQuotesPage::initializePage()
{
    LOG_INFO("initializePage() called - starting CTP initialization");

    auto [brokerId, userId, password, appId, authCode] = std::make_tuple(
        QString("9999"),
        QString("120750"),
        QString("ltc@Simnow900624"),
        QString("simnow_client_test"),
        QString("0000000000000000")
        );

    d->m_CTPService->setCredentials(brokerId, userId, password, appId, authCode);
    // simnow第1套
    // d->m_CTPService->setMarketFrontAddress("tcp://182.254.243.31:30011");
    // d->m_CTPService->setTradingFrontAddress("tcp://182.254.243.31:30001");

    // simnow第2套 7*24小时
    d->m_CTPService->setMarketFrontAddress("tcp://182.254.243.31:40011");
    d->m_CTPService->setTradingFrontAddress("tcp://182.254.243.31:40001");

    LOG_INFO("CTP credentials configured, calling setupConnections()...");

    d->m_CTPService->setupConnections();

    LOG_INFO("CTP initialization started with SimNow environment");
}

void FuturesQuotesPage::setupCtpConnections()
{
    connect(d->m_CTPService.get(), &CTP::CTPService::marketConnected, this, [this]() {
        d->m_isCtpConnected.store(true);
        updateConnectionStatus("行情已连接", "#4CAF50");
        LOG_INFO("CTP Market connected, waiting for login...");
    });

    connect(d->m_CTPService.get(), &CTP::CTPService::marketDisconnected, this,
            [this](int reason) {
                d->m_isCtpConnected.store(false);
                updateConnectionStatus(QString("行情断开 (代码:%1)").arg(reason), "#F44336");
                LOG_WARNING(QString("CTP Market disconnected, reason: %1").arg(reason));
            });

    connect(d->m_CTPService.get(), &CTP::CTPService::marketLoginFinished, this,
            [this](const bool success, const QString& msg) {
                if (success) {
                    LOG_INFO("Market login successful");
                    updateConnectionStatus("行情登录成功", "#4CAF50");
                } else {
                    LOG_ERROR(QString("Market login failed: %1").arg(msg));
                }
            });

    connect(d->m_CTPService.get(), &CTP::CTPService::tradingLoginFinished, this,
            [this](const bool success, const QString& msg) {
                if (success) {
                    LOG_INFO("Trading login successful, confirming settlement...");

                    if (d->m_statusLabel) {
                        d->m_statusLabel->setText("交易登录成功，正在确认结算单...");
                    }

                    d->m_CTPService->confirmSettlement();
                } else {
                    LOG_ERROR(QString("Trading login failed: %1").arg(msg));
                    QMessageBox::warning(this, "交易登录失败",
                                         QString("CTP交易登录失败: %1").arg(msg));
                }
            });

    connect(d->m_CTPService.get(), &CTP::CTPService::settlementConfirmed, this,
            [this](const bool success, const QString& msg) {
                if (success) {
                    LOG_INFO("Settlement confirmed, querying instruments...");

                    if (d->m_statusLabel) {
                        d->m_statusLabel->setText("结算单已确认，正在查询合约...");
                    }

                    QTimer::singleShot(500, this, [this]() {
                        d->m_CTPService->queryInstruments();
                    });
                } else {
                    LOG_ERROR(QString("Settlement confirmation failed: %1").arg(msg));
                    QTimer::singleShot(1000, this, [this]() {
                        d->m_CTPService->queryInstruments();
                    });
                }
            });

    connect(d->m_CTPService.get(), &CTP::CTPService::instrumentQueried,
            this, &FuturesQuotesPage::onInstrumentQueried,
            Qt::QueuedConnection);

    connect(d->m_CTPService.get(), &CTP::CTPService::instrumentQueryFinished,
            this, &FuturesQuotesPage::onInstrumentQueryFinished,
            Qt::QueuedConnection);

    connect(d->m_CTPService.get(), &CTP::CTPService::marketDataBatchReceived,
            this, &FuturesQuotesPage::onCtpBatchMarketData,
            Qt::QueuedConnection);

    connect(d->m_CTPService.get(), &CTP::CTPService::marketDataReceived,
            this, &FuturesQuotesPage::onCtpSingleMarketData,
            Qt::QueuedConnection);

    connect(d->m_CTPService.get(), &CTP::CTPService::errorOccurred, this,
            [this](const int reqId, int errorId, const QString& errorMsg) {
                LOG_ERROR(QString("CTP Error [%1] Request:%2 - %3")
                              .arg(errorId).arg(reqId).arg(errorMsg));
            });

    LOG_INFO("FuturesQuotesPage setupCtpConnections");
}

void FuturesQuotesPage::onInstrumentQueried(const QString& instrumentId, const QString& exchangeId,
                                            const QString& instrumentName, const double priceTick,
                                            const int volumeMultiple)
{
    auto [productCode, expiryDate, isStandard] = parseContractCode(instrumentId);

    QDate currentDate = QDate::currentDate();
    bool isExpired = expiryDate.isValid() && expiryDate < currentDate;
    bool isTooFar = expiryDate.isValid() &&
                    expiryDate > currentDate.addMonths(ActivityConfig::EXPIRY_MONTHS_AHEAD);

    if (!isExpired && !isTooFar && isStandard) {
        Impl::InstrumentInfo info;
        info.instrumentId = instrumentId;
        info.exchangeId = exchangeId;
        info.instrumentName = instrumentName;
        info.priceTick = priceTick;
        info.volumeMultiple = volumeMultiple;
        info.expiryDate = expiryDate;
        info.underlyingProduct = productCode;

        {
            QMutexLocker locker(&d->m_instrumentsMutex);
            d->m_instruments[instrumentId] = info;
        }

        if (exchangeId == "SHFE" || exchangeId == "DCE" || exchangeId == "CZCE" ||
            exchangeId == "CFFEX" || exchangeId == "INE" || exchangeId == "GFEX") {

            {
                QMutexLocker activityLocker(&d->m_activityMutex);
                Impl::ContractActivity activity;
                activity.isExpired = false;
                activity.isActive = true;
                d->m_activityMap[instrumentId] = activity;
            }

            {
                QMutexLocker contractLocker(&d->m_contractMutex);
                d->m_subscribedContracts.insert(instrumentId);
            }

            LOG_DEBUG(QString("Added valid contract: %1 (%2)").arg(instrumentId).arg(exchangeId));
        }

        int count = d->m_instrumentCount.fetch_add(1) + 1;
        if (count % 100 == 0 && d->m_statusLabel) {
            d->m_statusLabel->setText(QString("已查询 %1 个有效合约...").arg(count));
        }
    } else {
        if (isExpired) {
            LOG_DEBUG(QString("过滤过期合约: %1 (到期: %2)")
                      .arg(instrumentId).arg(expiryDate.toString("yyyy-MM")));
        }
        if (isTooFar) {
            LOG_DEBUG(QString("过滤未来合约: %1 (到期: %2)")
                      .arg(instrumentId).arg(expiryDate.toString("yyyy-MM")));
        }
        if (!isStandard) {
            LOG_DEBUG(QString("过滤非标准合约: %1").arg(instrumentId));
        }
    }
}

void FuturesQuotesPage::onInstrumentQueryFinished(int totalCount)
{
    int instrumentCount = d->m_instrumentCount.load();
    int subscribedCount = 0;
    {
        QMutexLocker locker(&d->m_contractMutex);
        subscribedCount = d->m_subscribedContracts.size();
    }

    LOG_INFO(QString("Instrument query finished, total: %1, valid: %2, subscribed: %3")
             .arg(totalCount).arg(instrumentCount).arg(subscribedCount));

    if (d->m_statusLabel) {
        d->m_statusLabel->setText(QString("查询完成，共 %1 个有效合约，订阅 %2 个")
                                  .arg(instrumentCount).arg(subscribedCount));
    }

    LOG_INFO("Starting to identify main contracts and subscribe");
    identifyMainContracts();
    subscribeContractsByPriority();
}

std::tuple<QString, QDate, bool> FuturesQuotesPage::parseContractCode(const QString& contractId)
{
    QRegularExpression re("^(?:[a-zA-Z]+\\d?)(\\d{4})$");
    QRegularExpressionMatch match = re.match(contractId);

    if (match.hasMatch()) {
        QString product = match.captured(1).toLower();
        QString yearMonth = match.captured(2);

        const int year = 2000 + yearMonth.left(2).toInt();
        int month = yearMonth.right(2).toInt();

        QDate expiryDate(year, month, 1);
        expiryDate = expiryDate.addMonths(1).addDays(-1);

        return {product, expiryDate, true};
    }

    return {contractId, QDate(), false};
}

void FuturesQuotesPage::identifyMainContracts() const
{
    QMutexLocker locker(&d->m_instrumentsMutex);

    QMap<QString, QList<QString>> productContracts;

    for (auto it = d->m_instruments.begin(); it != d->m_instruments.end(); ++it) {
        const auto& info = it.value();
        productContracts[info.underlyingProduct].append(info.instrumentId);
    }

    QMutexLocker mainLocker(&d->m_mainContractMutex);
    d->m_mainContracts.clear();

    QDate currentDate = QDate::currentDate();

    for (auto it = productContracts.begin(); it != productContracts.end(); ++it) {
        const QString& product = it.key();
        const QList<QString>& contracts = it.value();

        QString mainContract;
        QDate nearestDate;

        for (const QString& contractId : contracts) {
            const auto& info = d->m_instruments[contractId];
            if (info.expiryDate >= currentDate) {
                if (!nearestDate.isValid() || info.expiryDate < nearestDate) {
                    nearestDate = info.expiryDate;
                    mainContract = contractId;
                }
            }
        }

        if (!mainContract.isEmpty()) {
            d->m_mainContracts[product] = mainContract;
            d->m_instruments[mainContract].isMainContract = true;
            LOG_INFO(QString("主力合约识别: %1 -> %2").arg(product).arg(mainContract));
        }
    }
}

void FuturesQuotesPage::subscribeContractsByPriority()
{
    QMutexLocker locker(&d->m_contractMutex);

    QList<QString> priorityContracts;
    QSet<QString> mainContractSet;

    {
        QMutexLocker mainLocker(&d->m_mainContractMutex);
        for (auto it = d->m_mainContracts.begin(); it != d->m_mainContracts.end(); ++it) {
            priorityContracts.append(it.value());
            mainContractSet.insert(it.value());
        }
    }

    {
        QMutexLocker instLocker(&d->m_instrumentsMutex);
        QList<QPair<QDate, QString>> datedContracts;

        for (auto it = d->m_instruments.begin(); it != d->m_instruments.end(); ++it) {
            if (!mainContractSet.contains(it.key())) {
                datedContracts.append({it.value().expiryDate, it.key()});
            }
        }

        std::sort(datedContracts.begin(), datedContracts.end(),
                  [](const auto& a, const auto& b) { return a.first < b.first; });

        for (const auto& pair : datedContracts) {
            priorityContracts.append(pair.second);
        }
    }

    int subscribeCount = qMin(priorityContracts.size(), ActivityConfig::MAX_DISPLAY_CONTRACTS);
    QList<QString> toSubscribe = priorityContracts.mid(0, subscribeCount);

    LOG_INFO(QString("Priority subscription: %1 main contracts, %2 total contracts, subscribing %3")
             .arg(mainContractSet.size())
             .arg(priorityContracts.size())
             .arg(subscribeCount));

    subscribeContractsInBatches(toSubscribe);

    LOG_INFO(QString("Priority subscription: %1 main + %2 others, total: %3, subscribed: %4")
             .arg(mainContractSet.size())
             .arg(subscribeCount - mainContractSet.size())
             .arg(subscribeCount)
             .arg(d->m_subscribedContracts.size()));
}

void FuturesQuotesPage::subscribeContractsInBatches(const QList<QString>& contracts) const
{
    if (!d->m_CTPService || contracts.isEmpty()) return;

    const int batchSize = 50;

    for (int i = 0; i < contracts.size(); i += batchSize) {
        QList<QString> batch = contracts.mid(i, batchSize);
        int delayMs = (i / batchSize) * 200;

        QTimer::singleShot(delayMs, this, [this, batch]() {
            subscribeContracts(batch);
        });
    }
}

void FuturesQuotesPage::onCtpBatchMarketData(const QList<CTP::MarketData>& dataList)
{
    if (!d->m_isVisible.load() || dataList.isEmpty()) return;

    LOG_INFO(QString("Received %1 market data items").arg(dataList.size()));

    if (d->m_pendingUpdates.size() > 1000) {
        LOG_WARNING("Quote buffer overflow, dropping old data");
        QMutexLocker locker(&d->m_pendingMutex);
        d->m_pendingUpdates.remove(0, dataList.size());
    }

    QVector<FuturesQuoteItem> updates;
    updates.reserve(dataList.size());

    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

    for (const auto& ctpData : dataList) {
        QString contractId = ctpData.InstrumentID;

        bool isActive = updateContractActivity(contractId, ctpData, currentTime);

        if (!shouldDisplayContract(contractId, ctpData)) {
            LOG_DEBUG(QString("Skipping contract %1 (filtered out)").arg(contractId));
            continue;
        }

        FuturesQuoteItem item;
        item.contractName = contractId;
        item.lastPrice = ctpData.lastPrice;
        item.bidPrice = ctpData.BidPrice1;
        item.askPrice = ctpData.AskPrice1;
        item.bidVolume = ctpData.BidVolume1;
        item.askVolume = ctpData.AskVolume1;
        item.volume = ctpData.Volume;
        item.openPrice = ctpData.OpenPrice;
        item.highPrice = ctpData.HighestPrice;
        item.openInterest = static_cast<int>(ctpData.OpenInterest);
        item.preSettlementPrice = ctpData.preSettlementPrice;
        item.change = ctpData.lastPrice - ctpData.preSettlementPrice;
        item.changePercent = (ctpData.preSettlementPrice > 0) ?
                                 ((ctpData.lastPrice - ctpData.preSettlementPrice) / ctpData.preSettlementPrice * 100) : 0.0;

        {
            QMutexLocker locker(&d->m_instrumentsMutex);
            if (d->m_instruments.contains(contractId)) {
                item.isMainContract = d->m_instruments[contractId].isMainContract;
            }
        }

        // 【修改】活跃度状态：0=不活跃, 1=活跃, 2=高流动性
        int activityLevel = 0;
        if (ctpData.Volume > 100 && ctpData.OpenInterest > 500) {
            activityLevel = 2; // 高流动性
        } else if (isActive) {
            activityLevel = 1; // 活跃
        }
        item.activityStatus = activityLevel;

        updates.append(item);
    }

    {
        QMutexLocker locker(&d->m_pendingMutex);
        for (auto& update : updates) {
            d->m_pendingUpdates.append(std::move(update));
        }
    }

    if (updates.size() > 50) {
        QMetaObject::invokeMethod(this, &FuturesQuotesPage::flushPendingUpdates,
                                  Qt::QueuedConnection);
    }
}

bool FuturesQuotesPage::updateContractActivity(const QString& contractId,
                                                const CTP::MarketData& data,
                                                qint64 currentTime)
{
    QMutexLocker locker(&d->m_activityMutex);

    auto& activity = d->m_activityMap[contractId];
    activity.lastUpdateTime = currentTime;
    activity.totalVolume = data.Volume;
    activity.openInterest = static_cast<int>(data.OpenInterest);
    activity.updateCount++;

    bool hasVolume = data.Volume > 0;
    bool hasOpenInterest = data.OpenInterest > ActivityConfig::MIN_OPEN_INTEREST;
    bool hasBidAsk = data.BidVolume1 > 0 && data.AskVolume1 > 0;

    activity.isActive = hasVolume || hasOpenInterest || hasBidAsk;

    updateMainContractByVolume(contractId, data.Volume);

    return activity.isActive;
}

void FuturesQuotesPage::updateMainContractByVolume(const QString& contractId, int volume) const
{
    auto [productCode, expiryDate, isStandard] = parseContractCode(contractId);
    if (!isStandard) return;

    QMutexLocker mainLocker(&d->m_mainContractMutex);
    QMutexLocker instLocker(&d->m_instrumentsMutex);

    QString currentMain = d->m_mainContracts.value(productCode);

    if (contractId == currentMain) {
        return;
    }

    if (volume > ActivityConfig::MIN_DAILY_VOLUME && !currentMain.isEmpty()) {
        // 简化处理：实际应该比较一段时间内的累计成交量
    }
}

bool FuturesQuotesPage::shouldDisplayContract(const QString& contractId, const CTP::MarketData& data) const
{
    // 【修改】添加后备逻辑：如果没有任何合约，显示所有数据
    bool hasAnyContract = false;
    {
        QMutexLocker locker(&d->m_instrumentsMutex);
        hasAnyContract = !d->m_instruments.isEmpty();
    }

    if (!hasAnyContract) {
        LOG_INFO("No contracts available, showing all data as fallback");
        return true;
    }

    // 【修改】直接从代理模型获取当前过滤模式，避免atomic的语法问题
    switch (int filterMode = d->m_proxyModel->activityFilterMode()) {
        case 0: // ShowAll
            return true;

        case 1: { // ShowActiveOnly
            bool hasVolume = data.Volume > 0;
            bool hasOI = data.OpenInterest > ActivityConfig::MIN_OPEN_INTEREST;
            bool hasLiquidity = data.BidVolume1 > 0 && data.AskVolume1 > 0;
            bool result = hasVolume || hasOI || hasLiquidity;
            if (!result) {
                LOG_DEBUG(QString("Contract %1 filtered out (inactive): volume=%2, OI=%3, bid=%4, ask=%5")
                          .arg(contractId).arg(data.Volume).arg(data.OpenInterest)
                          .arg(data.BidVolume1).arg(data.AskVolume1));
            }
            return result || !hasAnyContract;
        }

        case 2: { // ShowMainOnly
            QMutexLocker locker(&d->m_mainContractMutex);
            bool result = d->m_mainContracts.values().contains(contractId);
            if (!result) {
                LOG_DEBUG(QString("Contract %1 is not a main contract").arg(contractId));
            }
            return result || !hasAnyContract;
        }

        case 3: { // ShowHighLiquidity
            bool result = data.Volume > 100 && data.OpenInterest > 500;
            if (!result) {
                LOG_DEBUG(QString("Contract %1 filtered out (low liquidity): volume=%2, OI=%3")
                          .arg(contractId).arg(data.Volume).arg(data.OpenInterest));
            }
            return result || !hasAnyContract;
        }
        default: ;
    }

    return true;
}

void FuturesQuotesPage::onCtpSingleMarketData(const CTP::MarketData& data)
{
    if (!d->m_isVisible.load()) return;

    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    bool isActive = updateContractActivity(data.InstrumentID, data, currentTime);

    if (!shouldDisplayContract(data.InstrumentID, data)) {
        return;
    }

    FuturesQuoteItem item;
    item.contractName = data.InstrumentID;
    item.lastPrice = data.lastPrice;
    item.bidPrice = data.BidPrice1;
    item.askPrice = data.AskPrice1;
    item.bidVolume = data.BidVolume1;
    item.askVolume = data.AskVolume1;
    item.volume = data.Volume;
    item.openInterest = static_cast<int>(data.OpenInterest);
    item.preSettlementPrice = data.preSettlementPrice;
    item.change = data.lastPrice - data.preSettlementPrice;
    item.changePercent = (data.preSettlementPrice > 0) ?
                             ((data.lastPrice - data.preSettlementPrice) / data.preSettlementPrice * 100) : 0.0;

    int activityLevel = 0;
    if (data.Volume > 100 && data.OpenInterest > 500) {
        activityLevel = 2;
    } else if (isActive) {
        activityLevel = 1;
    }
    item.activityStatus = activityLevel;

    {
        QMutexLocker locker(&d->m_pendingMutex);
        d->m_pendingUpdates.append(std::move(item));
    }
}

void FuturesQuotesPage::updateMainContracts()
{
    identifyMainContracts();

    // 【修改】如果当前是仅显示主力模式，刷新代理模型
    if (d->m_proxyModel->activityFilterMode() == 2) {
        QMetaObject::invokeMethod(this, [this]() {
            d->m_proxyModel->invalidate(); // 使用 invalidate() 替代
        }, Qt::QueuedConnection);
    }
}

// 【新增】设置活跃度过滤模式的公有接口
void FuturesQuotesPage::setActivityFilterMode(const int mode) const
{
    if (mode >= 0 && mode <= 3) {
        d->m_proxyModel->setActivityFilterMode(mode);
        LOG_INFO(QString("Activity filter mode changed to: %1").arg(mode));
    }
}

void FuturesQuotesPage::onSubscribeContract()
{
    QString contract = d->m_contractInput->text().trimmed().toUpper();
    if (contract.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入合约代码（如：rb2505）");
        return;
    }

    auto [product, expiry, isValid] = parseContractCode(contract);
    if (isValid && expiry.isValid() && expiry < QDate::currentDate()) {
        QMessageBox::warning(this, "过期合约",
                             QString("合约 %1 已过期（到期日：%2）")
                             .arg(contract).arg(expiry.toString("yyyy-MM-dd")));
        return;
    }

    {
        QMutexLocker locker(&d->m_contractMutex);
        d->m_subscribedContracts.insert(contract);
    }

    if (d->m_isCtpConnected.load()) {
        subscribeContracts({contract});
    } else {
        QMessageBox::information(this, "提示",
                                 QString("合约 %1 已加入订阅列表，连接成功后自动订阅").arg(contract));
    }

    d->m_contractInput->clear();
}

void FuturesQuotesPage::subscribeContracts(const QList<QString>& contracts) const
{
    if (!d->m_CTPService || contracts.isEmpty()) return;

    d->m_CTPService->subscribeMarketData(contracts, true);

    LOG_INFO(QString("Subscribed contracts: %1").arg(contracts.size()));

    if (d->m_statusLabel) {
        int activeCount = 0;
        {
            QMutexLocker locker(&d->m_activityMutex);
            for (const auto& contract : contracts) {
                if (d->m_activityMap.value(contract).isActive) {
                    activeCount++;
                }
            }
        }

        d->m_statusLabel->setText(
            QString("已订阅 %1 个合约 | 活跃 %2 个 | 接收 %3 笔行情")
                .arg(contracts.size())
                .arg(activeCount)
                .arg(d->m_tickCount.load())
            );
    }
}

void FuturesQuotesPage::updateConnectionStatus(const QString& text, const QString& color) const
{
    if (!d->m_statusLabel) return;

    QMetaObject::invokeMethod(d->m_statusLabel, [this, text, color]() {
        d->m_statusLabel->setText(text);
        d->m_statusLabel->setStyleSheet(
            QString("color: %1; padding: 5px; border-top: 1px solid #ddd; font-weight: bold;")
                .arg(color)
            );
    }, Qt::QueuedConnection);
}

void FuturesQuotesPage::onRowClicked(const QModelIndex &index) const
{
    if (!index.isValid()) return;

    QModelIndex sourceIndex = d->m_proxyModel->mapToSource(index);
    auto item = d->m_model->itemAt(sourceIndex.row());
    if (!item) return;

    d->m_tableView->selectRow(index.row());

    LOG_INFO(QString("Selected contract: %1 @ %2")
                 .arg(item->contractName)
                 .arg(item->lastPrice));
}

void FuturesQuotesPage::flushPendingUpdates() const
{
    if (!d->m_isVisible.load()) return;

    if (!d->m_model) return;

    if (d->m_pendingUpdates.isEmpty()) {
        LOG_DEBUG("No pending updates to flush");
        return;
    }

    const int maxBatchSize = 200;

    QVector<FuturesQuoteItem> updates;
    {
        QMutexLocker locker(&d->m_pendingMutex);
        if (d->m_pendingUpdates.isEmpty()) return;

        int takeCount = qMin(maxBatchSize, d->m_pendingUpdates.size());
        updates = d->m_pendingUpdates.mid(0, takeCount);
        d->m_pendingUpdates.remove(0, takeCount);
    }

    d->m_tickCount.fetch_add(updates.size());

    if (updates.size() == 1) {
        d->m_model->updateQuote(updates.first());
    } else {
        d->m_model->updateQuotes(updates);
    }

    int seq = d->m_quoteSequence.fetch_add(updates.size()) + updates.size();
    if (seq % 100 == 0 && d->m_statusLabel) {
        int activeCount = 0;
        int mainCount = 0;

        {
            QMutexLocker locker(&d->m_activityMutex);
            for (auto it = d->m_activityMap.begin(); it != d->m_activityMap.end(); ++it) {
                if (it.value().isActive) activeCount++;
            }
        }

        {
            QMutexLocker mainLocker(&d->m_mainContractMutex);
            mainCount = d->m_mainContracts.size();
        }

        d->m_statusLabel->setText(
            QString("连接正常 | 累计 %1 笔 | 活跃 %2 | 主力 %3 | 订阅 %4 个")
                .arg(seq)
                .arg(activeCount)
                .arg(mainCount)
                .arg(d->m_subscribedContracts.size())
            );
    }

    LOG_DEBUG(QString("Flushed %1 updates, model now has %2 rows")
             .arg(updates.size()).arg(d->m_model->rowCount()));
}

void FuturesQuotesPage::setupUI()
{
    auto* mainLayout = qobject_cast<QVBoxLayout*>(layout());
    if (!mainLayout) {
        mainLayout = new QVBoxLayout(this);
        mainLayout->setSpacing(0);
        mainLayout->setContentsMargins(10, 10, 10, 10);
    }

    auto *toolbarLayout = new QHBoxLayout;

    auto *refreshBtn = new QPushButton("刷新");
    refreshBtn->setProperty("ghost", true);

    d->m_contractInput = new QLineEdit();
    d->m_contractInput->setPlaceholderText("输入合约代码 (如: rb2505)");
    d->m_contractInput->setMaximumWidth(150);

    d->m_subscribeBtn = new QPushButton("订阅");
    d->m_subscribeBtn->setObjectName("subscribeBtn");
    d->m_subscribeBtn->setProperty("primary", true);

    d->m_activityFilter = new QComboBox(this);
    d->m_activityFilter->addItem("显示全部", 0);
    d->m_activityFilter->addItem("仅活跃合约", 1);
    d->m_activityFilter->addItem("仅主力合约", 2);
    d->m_activityFilter->addItem("高流动性", 3);
    d->m_activityFilter->setCurrentIndex(1);
    d->m_activityFilter->setMaximumWidth(120);

    auto *filterLabel = new QLabel("筛选：");
    filterLabel->setProperty("secondary", true);
    d->m_filterInput = new QLineEdit();
    d->m_filterInput->setPlaceholderText("输入合约代码筛选...");
    d->m_filterInput->setMaximumWidth(120);

    toolbarLayout->addWidget(refreshBtn);
    toolbarLayout->addSpacing(10);
    toolbarLayout->addWidget(d->m_contractInput);
    toolbarLayout->addWidget(d->m_subscribeBtn);
    toolbarLayout->addSpacing(10);
    toolbarLayout->addWidget(new QLabel("显示:"));
    toolbarLayout->addWidget(d->m_activityFilter);
    toolbarLayout->addStretch();
    toolbarLayout->addWidget(filterLabel);
    toolbarLayout->addWidget(d->m_filterInput);

    mainLayout->addLayout(toolbarLayout);

    d->m_tableView = new QTableView(this);
    d->m_tableView->setModel(d->m_proxyModel);

    d->m_tableView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    d->m_tableView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    d->m_tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    d->m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    d->m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    d->m_tableView->setSortingEnabled(false);
    d->m_tableView->setAlternatingRowColors(true);
    d->m_tableView->setShowGrid(false);
    d->m_tableView->verticalHeader()->setVisible(false);
    d->m_tableView->horizontalHeader()->setStretchLastSection(true);
    d->m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    d->m_tableView->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    d->m_tableView->setItemDelegate(new FuturesQuoteDelegate(this));

    d->m_tableView->setColumnWidth(0, 50);
    d->m_tableView->setColumnWidth(1, 80);
    d->m_tableView->setColumnWidth(2, 80);

    mainLayout->addWidget(d->m_tableView);

    d->m_statusLabel = new QLabel("正在连接CTP...", this);
    d->m_statusLabel->setObjectName("statusLabel");
    mainLayout->addWidget(d->m_statusLabel);

    connect(refreshBtn, &QPushButton::clicked, this, [this]() {
        d->m_model->clear();
        LOG_INFO("Quotes cleared");
    });

    connect(d->m_subscribeBtn, &QPushButton::clicked, this, &FuturesQuotesPage::onSubscribeContract);

    connect(d->m_filterInput, &QLineEdit::textChanged, this, [this](const QString& text) {
        d->m_proxyModel->setFilterFixedString(text.toUpper());
    });

    // 【修改】使用公有方法设置过滤模式
    connect(d->m_activityFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
                const int mode = d->m_activityFilter->currentData().toInt();
                setActivityFilterMode(mode);
            });
}

void FuturesQuotesPage::setupConnections()
{
    Q_ASSERT(d->m_tableView && d->m_model);

    connect(d->m_tableView, &QTableView::clicked, this, &FuturesQuotesPage::onRowClicked);
    connect(d->m_tableView, &QTableView::doubleClicked, this, &FuturesQuotesPage::onRowClicked);

    connect(d->m_flushTimer, &QTimer::timeout, this, &FuturesQuotesPage::flushPendingUpdates);
    d->m_flushTimer->start(500);

    auto* mainContractTimer = new QTimer(this);
    connect(mainContractTimer, &QTimer::timeout, this, &FuturesQuotesPage::updateMainContracts);
    mainContractTimer->start(5 * 60 * 1000);
}

void FuturesQuotesPage::initializeSampleData() const
{
    LOG_INFO("Initializing sample data for FuturesQuotesPage");

    // 创建一些示例期货合约数据
    QVector<FuturesQuoteItem> sampleData;
    
    // 示例合约数据
    QVector<std::tuple<QString, double, double, int, int, double, double>> sampleContracts = {
        {"cu2504", 71230, 71250, 150, 200, 71000, 71500},  // 沪铜
        {"rb2505", 4320, 4325, 80, 100, 4300, 4350},      // 螺纹钢
        {"au2506", 580, 582, 30, 40, 575, 585},          // 沪金
        {"ag2505", 7800, 7820, 50, 60, 7750, 7850},       // 沪银
        {"zn2505", 23800, 23850, 120, 150, 23700, 23900}, // 锌
        {"ni2505", 158000, 158500, 5, 8, 157000, 159000},// 镍
        {"sn2505", 228000, 228500, 3, 5, 227000, 229000},// 锡
        {"al2505", 20400, 20450, 100, 120, 20300, 20500},// 铝
        {"pb2505", 20800, 20850, 40, 50, 20700, 20900},  // 铅
        {"zcecf2505", 6800, 6820, 200, 250, 6750, 6850}  // 玉米淀粉
    };

    for (const auto& contract : sampleContracts) {
        const auto& [code, lastPrice, change, volume, openInterest, bidPrice, askPrice] = contract;
        
        FuturesQuoteItem item;
        item.contractName = code;
        item.lastPrice = lastPrice;
        item.change = change;
        item.changePercent = (change / lastPrice) * 100;
        item.volume = volume;
        item.openInterest = openInterest;
        item.bidPrice = bidPrice;
        item.askPrice = askPrice;
        item.isMainContract = true;  // 标记为活跃合约
        item.activityStatus = 2;     // 高流动性状态

        sampleData.append(item);
    }

    // 将示例数据添加到模型中
    if (d->m_model) {
        d->m_model->setQuotes(sampleData);
        LOG_INFO(QString("Added %1 sample contracts to model").arg(sampleData.size()));
    }
}
