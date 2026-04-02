/**
 * @file FuturesQuotesPage.cpp
 * @brief 期货行情页面 - CTP实时行情对接实现
 * @note 关键特性：
 *   1. 自动查询所有合约并订阅
 *   2. 零拷贝优化：CTP数据直接转换为FuturesQuoteItem
 *   3. 连接状态管理：自动重连、合约订阅、断线恢复
 *   4. 线程安全：所有CTP回调通过QueuedConnection切换到UI线程
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

#include <atomic>
#include <optional>

// C++17 结构化绑定辅助
using CtpConfig = std::tuple<QString, QString, QString, QString, QString>;

class FuturesQuotesPage::Impl {
public:
    // UI 组件
    QTableView *m_tableView = nullptr;
    FuturesQuoteModel *m_model = nullptr;
    QSortFilterProxyModel *m_proxyModel = nullptr;  // 筛选代理
    QLabel *m_statusLabel = nullptr;
    QLineEdit *m_contractInput = nullptr;  // 合约输入框
    QLineEdit *m_filterInput = nullptr;    // 筛选输入框
    QPushButton *m_subscribeBtn = nullptr; // 订阅按钮

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
        double priceTick;
        int volumeMultiple;
    };
    QMap<QString, InstrumentInfo> m_instruments;

    // 性能监控
    QTimer *m_flushTimer = nullptr;
    std::atomic<int> m_quoteSequence{0};
};

FuturesQuotesPage::FuturesQuotesPage(QWidget* parent)
    : BasePage(parent)
    , d(std::make_unique<Impl>())
{
    d->m_model = new FuturesQuoteModel(this);
    d->m_proxyModel = new QSortFilterProxyModel(this);
    d->m_proxyModel->setSourceModel(d->m_model);
    d->m_proxyModel->setFilterKeyColumn(0);  // 按合约代码筛选
    d->m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    d->m_CTPService = std::make_unique<CTP::CTPService>(this);
    d->m_flushTimer = new QTimer(this);

    setupUI();
    setupConnections();
    setupCtpConnections();

    LOG_INFO("FuturesQuotesPage created with CTP support");
}

FuturesQuotesPage::~FuturesQuotesPage() = default;

QString FuturesQuotesPage::pageId() const
{
    return QStringLiteral("FuturesQuotesPage");
}

/**
 * @brief 设置CTP客户端连接（生产级配置）
 */
void FuturesQuotesPage::initializePage()
{
    LOG_INFO("initializePage() called - starting CTP initialization");

    // C++17 结构化绑定配置
    auto [brokerId, userId, password, appId, authCode] = std::make_tuple(
        QString("9999"),
        QString("120750"),
        QString("ltc@Simnow900624"),
        QString("simnow_client_test"),
        QString("0000000000000000")
        );

    // 配置CTP客户端
    d->m_CTPService->setCredentials(brokerId, userId, password, appId, authCode);

    // SimNow 第一套 测试环境
    d->m_CTPService->setMarketFrontAddress("tcp://182.254.243.31:30011");
    d->m_CTPService->setTradingFrontAddress("tcp://182.254.243.31:30001");

    // // SimNow 第二套 7x24 小时测试环境
    // d->m_CTPService->setMarketFrontAddress("tcp://180.168.146.187:10211");
    // d->m_CTPService->setTradingFrontAddress("tcp://180.168.146.187:10201");

    LOG_INFO("CTP credentials configured, calling setupConnections()...");

    // 启动连接（异步）
    d->m_CTPService->setupConnections();

    LOG_INFO("CTP initialization started with SimNow 7x24 environment");
}

/**
 * @brief 设置CTP信号连接
 */
void FuturesQuotesPage::setupCtpConnections()
{
    // 1. 连接状态监控
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

    // 2. 行情登录成功 - 可以开始订阅行情
    connect(d->m_CTPService.get(), &CTP::CTPService::marketLoginFinished, this,
            [this](bool success, const QString& msg) {
                if (success) {
                    LOG_INFO("Market login successful");
                    updateConnectionStatus("行情登录成功", "#4CAF50");
                } else {
                    LOG_ERROR(QString("Market login failed: %1").arg(msg));
                }
            });

    // 3. 交易登录成功 - 先确认结算单，再查询合约
    connect(d->m_CTPService.get(), &CTP::CTPService::tradingLoginFinished, this,
            [this](bool success, const QString& msg) {
                if (success) {
                    LOG_INFO("Trading login successful, confirming settlement...");

                    if (d->m_statusLabel) {
                        d->m_statusLabel->setText("交易登录成功，正在确认结算单...");
                    }

                    // 先确认结算单
                    d->m_CTPService->confirmSettlement();
                } else {
                    LOG_ERROR(QString("Trading login failed: %1").arg(msg));
                    QMessageBox::warning(this, "交易登录失败",
                                         QString("CTP交易登录失败: %1").arg(msg));
                }
            });

    // 3.1 结算单确认成功 - 查询合约
    connect(d->m_CTPService.get(), &CTP::CTPService::settlementConfirmed, this,
            [this](bool success, const QString& msg) {
                if (success) {
                    LOG_INFO("Settlement confirmed, querying instruments...");

                    if (d->m_statusLabel) {
                        d->m_statusLabel->setText("结算单已确认，正在查询合约...");
                    }

                    // 延迟查询，避免流控
                    QTimer::singleShot(500, this, [this]() {
                        d->m_CTPService->queryInstruments();
                    });
                } else {
                    LOG_ERROR(QString("Settlement confirmation failed: %1").arg(msg));
                    // 即使失败也尝试查询
                    QTimer::singleShot(1000, this, [this]() {
                        d->m_CTPService->queryInstruments();
                    });
                }
            });

    // 4. 合约查询回调
    connect(d->m_CTPService.get(), &CTP::CTPService::instrumentQueried,
            this, &FuturesQuotesPage::onInstrumentQueried,
            Qt::QueuedConnection);

    connect(d->m_CTPService.get(), &CTP::CTPService::instrumentQueryFinished,
            this, &FuturesQuotesPage::onInstrumentQueryFinished,
            Qt::QueuedConnection);

    // 5. 批量行情接收
    connect(d->m_CTPService.get(), &CTP::CTPService::marketDataBatchReceived,
            this, &FuturesQuotesPage::onCtpBatchMarketData,
            Qt::QueuedConnection);

    // 6. 单个行情（备用）
    connect(d->m_CTPService.get(), &CTP::CTPService::marketDataReceived,
            this, &FuturesQuotesPage::onCtpSingleMarketData,
            Qt::QueuedConnection);

    // 7. 错误处理
    connect(d->m_CTPService.get(), &CTP::CTPService::errorOccurred, this,
            [this](int reqId, int errorId, const QString& errorMsg) {
                LOG_ERROR(QString("CTP Error [%1] Request:%2 - %3")
                              .arg(errorId).arg(reqId).arg(errorMsg));
            });

    LOG_INFO("FuturesQuotesPage setupCtpConnections");
}

/**
 * @brief 合约查询回调
 */
void FuturesQuotesPage::onInstrumentQueried(const QString& instrumentId, const QString& exchangeId,
                                            const QString& instrumentName, double priceTick, int volumeMultiple)
{
    // 缓存合约信息
    Impl::InstrumentInfo info;
    info.instrumentId = instrumentId;
    info.exchangeId = exchangeId;
    info.instrumentName = instrumentName;
    info.priceTick = priceTick;
    info.volumeMultiple = volumeMultiple;

    d->m_instruments[instrumentId] = info;

    // 添加到订阅列表（排除期权等非主流品种）
    if (exchangeId == "SHFE" || exchangeId == "DCE" || exchangeId == "CZCE" ||
        exchangeId == "CFFEX" || exchangeId == "INE" || exchangeId == "GFEX") {
        d->m_subscribedContracts.insert(instrumentId);
    }

    int count = d->m_instrumentCount.fetch_add(1) + 1;

    // 每100个更新一次状态
    if (count % 100 == 0 && d->m_statusLabel) {
        d->m_statusLabel->setText(QString("已查询 %1 个合约...").arg(count));
    }
}

/**
 * @brief 合约查询完成回调
 */
void FuturesQuotesPage::onInstrumentQueryFinished(int totalCount)
{
    int instrumentCount = d->m_instrumentCount.load();
    LOG_INFO(QString("Instrument query finished, total: %1, subscribed: %2")
             .arg(instrumentCount).arg(d->m_subscribedContracts.size()));

    if (d->m_statusLabel) {
        d->m_statusLabel->setText(QString("查询完成，共 %1 个合约，订阅 %2 个")
                                  .arg(instrumentCount).arg(d->m_subscribedContracts.size()));
    }

    // 批量订阅行情（分批订阅，避免一次性订阅过多）
    QList<QString> contracts = d->m_subscribedContracts.values();
    const int batchSize = 100;  // 每批100个

    for (int i = 0; i < contracts.size(); i += batchSize) {
        QList<QString> batch;
        int endIdx = qMin(i + batchSize, static_cast<int>(contracts.size()));
        for (int j = i; j < endIdx; ++j) {
            batch.append(contracts[j]);
        }

        // 延迟订阅，避免流控
        QTimer::singleShot(i / batchSize * 1000, this, [this, batch]() {
            subscribeContracts(batch);
        });
    }
}

/**
 * @brief CTP批量行情处理
 */
void FuturesQuotesPage::onCtpBatchMarketData(const QList<CTP::MarketData>& dataList)
{
    if (dataList.isEmpty()) return;

    // 跳帧保护
    if (d->m_pendingUpdates.size() > 1000) {
        LOG_WARNING("Quote buffer overflow, dropping old data");
        QMutexLocker locker(&d->m_pendingMutex);
        d->m_pendingUpdates.remove(0, dataList.size());
    }

    QVector<FuturesQuoteItem> updates;
    updates.reserve(dataList.size());

    for (const auto& ctpData : dataList) {
        FuturesQuoteItem item;
        item.contractName = ctpData.instrumentId;
        item.lastPrice = ctpData.lastPrice;
        item.bidPrice = ctpData.bidPrice1;
        item.askPrice = ctpData.askPrice1;
        item.bidVolume = ctpData.bidVolume1;
        item.askVolume = ctpData.askVolume1;
        item.volume = ctpData.volume;
        item.openInterest = static_cast<int>(ctpData.openInterest);
        item.preSettlementPrice = ctpData.preSettlementPrice;

        item.change = ctpData.lastPrice - ctpData.preSettlementPrice;
        item.changePercent = (ctpData.preSettlementPrice > 0) ?
                                 ((ctpData.lastPrice - ctpData.preSettlementPrice) / ctpData.preSettlementPrice * 100) : 0.0;

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

/**
 * @brief CTP单条行情处理
 */
void FuturesQuotesPage::onCtpSingleMarketData(const CTP::MarketData& data)
{
    FuturesQuoteItem item;
    item.contractName = data.instrumentId;
    item.lastPrice = data.lastPrice;
    item.bidPrice = data.bidPrice1;
    item.askPrice = data.askPrice1;
    item.bidVolume = data.bidVolume1;
    item.askVolume = data.askVolume1;
    item.volume = data.volume;
    item.openInterest = static_cast<int>(data.openInterest);
    item.preSettlementPrice = data.preSettlementPrice;

    item.change = data.lastPrice - data.preSettlementPrice;
    item.changePercent = (data.preSettlementPrice > 0) ?
                             ((data.lastPrice - data.preSettlementPrice) / data.preSettlementPrice * 100) : 0.0;

    {
        QMutexLocker locker(&d->m_pendingMutex);
        d->m_pendingUpdates.append(std::move(item));
    }
}

/**
 * @brief 订阅合约（UI交互）
 */
void FuturesQuotesPage::onSubscribeContract()
{
    QString contract = d->m_contractInput->text().trimmed().toUpper();
    if (contract.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入合约代码（如：rb2505）");
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

/**
 * @brief 实际执行订阅
 */
void FuturesQuotesPage::subscribeContracts(const QList<QString>& contracts)
{
    if (!d->m_CTPService || contracts.isEmpty()) return;

    d->m_CTPService->subscribeMarketData(contracts, true);

    LOG_INFO(QString("Subscribed contracts: %1").arg(contracts.size()));

    if (d->m_statusLabel) {
        d->m_statusLabel->setText(
            QString("已订阅 %1 个合约 | 接收 %2 笔行情")
                .arg(d->m_subscribedContracts.size())
                .arg(d->m_tickCount.load())
            );
    }
}

/**
 * @brief 更新连接状态显示
 */
void FuturesQuotesPage::updateConnectionStatus(const QString& text, const QString& color)
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

/**
 * @brief 行点击处理
 */
void FuturesQuotesPage::onRowClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;

    // 映射到源模型
    QModelIndex sourceIndex = d->m_proxyModel->mapToSource(index);
    auto item = d->m_model->itemAt(sourceIndex.row());
    if (!item) return;

    d->m_tableView->selectRow(index.row());

    LOG_INFO(QString("Selected contract: %1 @ %2")
                 .arg(item->contractName)
                 .arg(item->lastPrice));
}

/**
 * @brief 批量刷新UI
 */
void FuturesQuotesPage::flushPendingUpdates()
{
    if (!d->m_model) return;

    if (d->m_pendingUpdates.isEmpty()) return;

    QVector<FuturesQuoteItem> updates;
    {
        QMutexLocker locker(&d->m_pendingMutex);
        if (d->m_pendingUpdates.isEmpty()) return;
        updates = std::move(d->m_pendingUpdates);
        d->m_pendingUpdates.clear();
    }

    d->m_tickCount.fetch_add(updates.size());

    if (updates.size() == 1) {
        d->m_model->updateQuote(updates.first());
    } else {
        d->m_model->updateQuotes(updates);
    }

    // 更新状态栏
    int seq = d->m_quoteSequence.fetch_add(updates.size()) + updates.size();
    if (seq % 100 == 0 && d->m_statusLabel) {
        d->m_statusLabel->setText(
            QString("实盘连接正常 | 累计接收 %1 笔 | 订阅 %2 个合约")
                .arg(seq)
                .arg(d->m_subscribedContracts.size())
            );
    }
}

void FuturesQuotesPage::setupUI()
{
    QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(layout());
    if (!mainLayout) {
        mainLayout = new QVBoxLayout(this);
        mainLayout->setSpacing(0);
        mainLayout->setContentsMargins(10, 10, 10, 10);
    }

    // 工具栏
    auto *toolbarLayout = new QHBoxLayout;

    auto *refreshBtn = new QPushButton("刷新");
    refreshBtn->setProperty("ghost", true);

    d->m_contractInput = new QLineEdit();
    d->m_contractInput->setPlaceholderText("输入合约代码 (如: rb2505)");
    d->m_contractInput->setMaximumWidth(150);

    d->m_subscribeBtn = new QPushButton("订阅");
    d->m_subscribeBtn->setObjectName("subscribeBtn");
    d->m_subscribeBtn->setProperty("primary", true);

    auto *filterLabel = new QLabel("筛选：");
    filterLabel->setProperty("secondary", true);
    d->m_filterInput = new QLineEdit();
    d->m_filterInput->setPlaceholderText("输入合约代码筛选...");
    d->m_filterInput->setMaximumWidth(120);

    toolbarLayout->addWidget(refreshBtn);
    toolbarLayout->addSpacing(10);
    toolbarLayout->addWidget(d->m_contractInput);
    toolbarLayout->addWidget(d->m_subscribeBtn);
    toolbarLayout->addStretch();
    toolbarLayout->addWidget(filterLabel);
    toolbarLayout->addWidget(d->m_filterInput);

    mainLayout->addLayout(toolbarLayout);

    // 表格视图
    d->m_tableView = new QTableView(this);
    d->m_tableView->setModel(d->m_proxyModel);  // 使用代理模型

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

    d->m_tableView->setColumnWidth(0, 80);
    d->m_tableView->setColumnWidth(1, 100);
    d->m_tableView->setColumnWidth(2, 80);
    d->m_tableView->setColumnWidth(9, 80);
    d->m_tableView->setColumnWidth(10, 80);

    mainLayout->addWidget(d->m_tableView);

    // 状态栏 - 样式由QSS管理
    d->m_statusLabel = new QLabel("正在连接CTP...", this);
    d->m_statusLabel->setObjectName("statusLabel");
    mainLayout->addWidget(d->m_statusLabel);

    // 信号连接
    connect(refreshBtn, &QPushButton::clicked, this, [this]() {
        d->m_model->clear();
        LOG_INFO("Quotes cleared");
    });

    connect(d->m_subscribeBtn, &QPushButton::clicked, this, &FuturesQuotesPage::onSubscribeContract);

    // 筛选功能
    connect(d->m_filterInput, &QLineEdit::textChanged, this, [this](const QString& text) {
        d->m_proxyModel->setFilterFixedString(text.toUpper());
    });
}

void FuturesQuotesPage::setupConnections()
{
    Q_ASSERT(d->m_tableView && d->m_model);

    connect(d->m_tableView, &QTableView::clicked, this, &FuturesQuotesPage::onRowClicked);
    connect(d->m_tableView, &QTableView::doubleClicked, this, &FuturesQuotesPage::onRowClicked);

    // UI刷新定时器（500ms合并刷新）
    connect(d->m_flushTimer, &QTimer::timeout, this, &FuturesQuotesPage::flushPendingUpdates);
    d->m_flushTimer->start(500);
}
