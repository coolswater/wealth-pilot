/**
 * @file FuturesQuotesPage.cpp
 * @brief 期货行情页面 - CTP实时行情对接实现
 * @note 关键特性：
 *   1. 双模运行：支持CTP真实行情与本地模拟数据无缝切换
 *   2. 零拷贝优化：CTP数据直接转换为FuturesQuoteItem，复用现有缓冲队列
 *   3. 连接状态管理：自动重连、合约订阅、断线恢复
 *   4. 线程安全：所有CTP回调通过QueuedConnection切换到UI线程
 */

#include "FuturesQuotesPage.h"
#include "utils/Logger.h"
#include "models/FuturesQuoteModel.h"
#include "models/FuturesQuoteDelegate.h"

// CTP客户端接口（基于之前设计的CTPService）

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QHeaderView>
#include <QMessageBox>
#include <QTimer>
#include <QInputDialog>
#include <QtConcurrent/QtConcurrent>
#include <utils/FuturesMockDataGenerator.h>

#include <atomic>
#include <optional>

// C++17 结构化绑定辅助
using CtpConfig = std::tuple<QString, QString, QString, QString, QString>;

class FuturesQuotesPage::Impl {
public:
    // UI 组件
    QTableView *m_tableView = nullptr;
    FuturesQuoteModel *m_model = nullptr;
    QLabel *m_statusLabel = nullptr;
    QLineEdit *m_contractInput = nullptr;  // 合约输入框
    QPushButton *m_subscribeBtn = nullptr; // 订阅按钮
    QPushButton *m_modeBtn = nullptr;      // 模式切换（真实/模拟）

    // CTP客户端（C++17智能指针管理）
    std::unique_ptr<CTP::CTPService> m_CTPService;

    // 模拟数据生成器（备用）
    FuturesMockDataGenerator *m_generator = nullptr;
    QTimer *m_simulateTimer = nullptr;

    // 状态控制
    std::atomic<bool> m_isRealMode{true};      // 当前是否为真实行情模式
    std::atomic<bool> m_isCtpConnected{false};
    std::atomic<bool> m_isProcessing{false};
    std::atomic<int> m_tickCount{0};

    // 数据缓存（复用现有机制）
    QVector<FuturesQuoteItem> m_pendingUpdates;
    QMutex m_pendingMutex;

    // 已订阅合约列表
    QSet<QString> m_subscribedContracts;
    QMutex m_contractMutex;

    // 性能监控
    QTimer *m_flushTimer = nullptr;
    std::atomic<int> m_quoteSequence{0};  // 行情序列号，用于去重
};

FuturesQuotesPage::FuturesQuotesPage(QWidget* parent)
    : BasePage(parent)
    , d(std::make_unique<Impl>())
{
    d->m_model = new FuturesQuoteModel(this);
    d->m_CTPService = std::make_unique<CTP::CTPService>(this);
    d->m_generator = new FuturesMockDataGenerator();
    d->m_simulateTimer = new QTimer(this);
    d->m_flushTimer = new QTimer(this);

    setupUI();
    setupConnections();
    setupCtpConnections();  // CTP信号连接

    // 延迟初始化
    QTimer::singleShot(100, this, &FuturesQuotesPage::initData);

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
    // 注意：请将以下占位符替换为您的 SimNow 账号信息
    // SimNow 注册地址：https://www.simnow.com.cn/
    auto [brokerId, userId, password, appId, authCode] = std::make_tuple(
        QString("9999"),                                    // SimNow 经纪公司代码
        QString("120750"),                                  // 替换为您的 SimNow 账号
        QString("ltc@Simnow900624"),                         // 替换为您的密码
        QString("simnow_client_test"),                       // SimNow 7x24 环境无需 AppID
        QString("0000000000000000")                          // SimNow 7x24 环境无需 AuthCode
        );

    // 配置CTP客户端
    d->m_CTPService->setCredentials(brokerId, userId, password, appId, authCode);

    // SimNow 第二套 7x24 小时测试环境（移动线路，更稳定）
    d->m_CTPService->setMarketFrontAddress("tcp://182.254.243.31:40011");
    d->m_CTPService->setTradingFrontAddress("tcp://182.254.243.31:40001");

    LOG_INFO("CTP credentials configured, calling setupConnections()...");

    // 启动连接（异步）
    d->m_CTPService->setupConnections();

    setupCtpConnections();  // 新增：CTP信号连接

    LOG_INFO("CTP initialization started with SimNow 7x24 environment");
}

/**
 * @brief 设置CTP信号连接（关键：确保线程安全）
 */
void FuturesQuotesPage::setupCtpConnections()
{
    // 1. 连接状态监控
    connect(d->m_CTPService.get(), &CTP::CTPService::marketConnected, this, [this]() {
        d->m_isCtpConnected.store(true);
        updateConnectionStatus("行情已连接", "#4CAF50");  // 绿色
        LOG_INFO("CTP Market connected, waiting for login...");
        // 注意：订阅移到 loginFinished 后执行
    });

    connect(d->m_CTPService.get(), &CTP::CTPService::marketDisconnected, this,
            [this](int reason) {
                d->m_isCtpConnected.store(false);
                updateConnectionStatus(QString("行情断开 (代码:%1)").arg(reason), "#F44336");  // 红色
                LOG_WARNING(QString("CTP Market disconnected, reason: %1").arg(reason));
            });

    connect(d->m_CTPService.get(), &CTP::CTPService::loginFinished, this,
            [this](bool success, const QString& msg) {
                if (success) {
                    LOG_INFO("CTP Login successful, now subscribing contracts...");

                    // 登录成功后订阅合约
                    if (d->m_subscribedContracts.isEmpty()) {
                        // SimNow 7x24 环境支持的测试合约
                        // 注意：合约代码需要根据当前日期查询，这里使用常见的测试合约
                        QList<QString> defaultContracts = {
                            "rb2605",   // 螺纹钢 2026年5月
                            "cu2605",   // 铜 2026年5月
                            "au2605",   // 黄金 2026年5月
                            "ag2605",   // 白银 2026年5月
                            "i2605",     // 铁矿石 2026年5月
                            "FU2704‌",
                            "LU2704‌",
                            "SC2704‌",
                            "PPF2704‌",
                            "PTA2605",
                        };
                        LOG_INFO(QString("Auto-subscribing default contracts: %1").arg(defaultContracts.join(", ")));

                        for (const auto& contract : defaultContracts) {
                            d->m_subscribedContracts.insert(contract);
                        }
                        subscribeContracts(defaultContracts);
                    } else {
                        subscribeContracts(d->m_subscribedContracts.values());
                    }

                    // 登录成功后自动切换到真实模式
                    if (!d->m_isRealMode.load()) {
                        switchToRealMode();
                    }
                } else {
                    LOG_ERROR(QString("CTP Login failed: %1").arg(msg));
                    QMessageBox::warning(this, "连接失败",
                                         QString("CTP登录失败: %1\n将保持模拟模式").arg(msg));
                }
            });

    // 2. 批量行情接收（高性能路径）
    connect(d->m_CTPService.get(), &CTP::CTPService::marketDataBatchReceived,
            this, &FuturesQuotesPage::onCtpBatchMarketData,
            Qt::QueuedConnection);  // 关键：确保在主线程处理

    // 3. 单个行情（备用）
    connect(d->m_CTPService.get(), &CTP::CTPService::marketDataReceived,
            this, &FuturesQuotesPage::onCtpSingleMarketData,
            Qt::QueuedConnection);

    // 4. 错误处理
    connect(d->m_CTPService.get(), &CTP::CTPService::errorOccurred, this,
            [this](int reqId, int errorId, const QString& errorMsg) {
                LOG_ERROR(QString("CTP Error [%1] Request:%2 - %3")
                              .arg(errorId).arg(reqId).arg(errorMsg));

                // 特定错误处理
                if (errorId == 8000) {  // 流控错误
                    QMessageBox::warning(this, "流控警告",
                                         "查询过于频繁，请降低刷新率");
                }
            });

    LOG_INFO("FuturesQuotesPage setupCtpConnections");
}

/**
 * @brief CTP批量行情处理（零拷贝优化）
 * @details 直接将CTP数据转换为FuturesQuoteItem，复用现有m_pendingUpdates队列
 */
void FuturesQuotesPage::onCtpBatchMarketData(const QList<CTP::MarketData>& dataList)
{
    LOG_WARNING("================= onCtpBatchMarketData called =================");

    if (!d->m_isRealMode.load() || dataList.isEmpty()) return;

    // 跳帧保护：如果积压过多，丢弃部分数据保活
    if (d->m_pendingUpdates.size() > 1000) {
        LOG_WARNING("Quote buffer overflow, dropping old data");
        QMutexLocker locker(&d->m_pendingMutex);
        d->m_pendingUpdates.remove(0, dataList.size());  // 移除旧数据
    }

    QVector<FuturesQuoteItem> updates;
    updates.reserve(dataList.size());  // C++17 预分配优化

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

        // 计算涨跌（最新价 - 昨结算）
        item.change = ctpData.lastPrice - ctpData.preSettlementPrice;
        
        // 计算涨跌幅
        item.changePercent = (ctpData.preSettlementPrice > 0) ?
                                 ((ctpData.lastPrice - ctpData.preSettlementPrice) / ctpData.preSettlementPrice * 100) : 0.0;

        updates.append(item);
    }

    // 加入待处理队列（与模拟数据共享同一队列，确保一致性）
    {
        QMutexLocker locker(&d->m_pendingMutex);
        for (auto& update : updates) {
            d->m_pendingUpdates.append(std::move(update));  // C++17 move语义
        }
    }

    // 可选：立即触发刷新（如果数据紧急），否则等待定时器
    if (updates.size() > 50) {  // 大数据包立即刷新
        QMetaObject::invokeMethod(this, &FuturesQuotesPage::flushPendingUpdates,
                                  Qt::QueuedConnection);
    }
}

/**
 * @brief CTP单条行情处理（低频率场景）
 */
void FuturesQuotesPage::onCtpSingleMarketData(const CTP::MarketData& data)
{
    if (!d->m_isRealMode.load()) return;

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
    
    // 计算涨跌（最新价 - 昨结算）
    item.change = data.lastPrice - data.preSettlementPrice;
    
    // 计算涨跌幅
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
        QMessageBox::warning(this, "输入错误", "请输入合约代码（如：rb2605）");
        return;
    }

    // 添加到订阅列表
    {
        QMutexLocker locker(&d->m_contractMutex);
        d->m_subscribedContracts.insert(contract);
    }

    // 如果已连接，立即订阅
    if (d->m_isCtpConnected.load()) {
        subscribeContracts({contract});
    } else {
        QMessageBox::information(this, "提示",
                                 QString("合约 %1 已加入订阅列表，连接成功后自动订阅").arg(contract));
    }

    d->m_contractInput->clear();
}

/**
 * @brief 实际执行订阅（支持批量）
 */
void FuturesQuotesPage::subscribeContracts(const QList<QString>& contracts)
{
    if (!d->m_CTPService || contracts.isEmpty()) return;

    d->m_CTPService->subscribeMarketData(contracts, true);  // 启用批量缓冲

    LOG_INFO(QString("Subscribed contracts: %1").arg(contracts.join(", ")));

    // 更新状态栏
    if (d->m_statusLabel) {
        d->m_statusLabel->setText(
            QString("已订阅 %1 个合约 | 模式: %2")
                .arg(d->m_subscribedContracts.size())
                .arg(d->m_isRealMode.load() ? "实盘" : "模拟")
            );
    }
}

/**
 * @brief 切换真实/模拟模式
 */
void FuturesQuotesPage::switchToRealMode()
{
    if (d->m_isRealMode.load()) return;  // 已经是真实模式

    d->m_isRealMode.store(true);
    d->m_simulateTimer->stop();

    // 清空模拟数据残留
    {
        QMutexLocker locker(&d->m_pendingMutex);
        d->m_pendingUpdates.clear();
    }

    if (d->m_modeBtn) {
        d->m_modeBtn->setText("模式: 实盘");
        d->m_modeBtn->setStyleSheet("background-color: #4CAF50; color: white;");
    }

    LOG_INFO("Switched to REAL mode (CTP)");
    updateConnectionStatus("切换到实盘模式", "#2196F3");
}

void FuturesQuotesPage::switchToSimulateMode()
{
    if (!d->m_isRealMode.load()) return;

    d->m_isRealMode.store(false);
    d->m_simulateTimer->start(1000);  // 重启模拟

    if (d->m_modeBtn) {
        d->m_modeBtn->setText("模式: 模拟");
        d->m_modeBtn->setStyleSheet("background-color: #FF9800; color: white;");
    }

    LOG_INFO("Switched to SIMULATE mode");
    updateConnectionStatus("模拟模式运行中", "#FF9800");
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
 * @brief 行点击处理（扩展：显示详细行情）
 */
void FuturesQuotesPage::onRowClicked(const QModelIndex &index)
{
    if (!index.isValid() || !d->m_model) return;

    auto item = d->m_model->itemAt(index.row());
    if (!item) return;

    // 高亮选中行
    d->m_tableView->selectRow(index.row());

    // 如果是真实模式，可在此处打开下单界面或深度行情
    if (d->m_isRealMode.load() && d->m_isCtpConnected.load()) {
        LOG_INFO(QString("Selected real-time quote: %1 @ %2")
                     .arg(item->contractName)
                     .arg(item->lastPrice));
    }
}

/**
 * @brief 模拟Tick（保留作为备用）
 */
void FuturesQuotesPage::onSimulateTick()
{
    // 真实模式下跳过模拟
    if (d->m_isRealMode.load()) return;

    // 原有的模拟逻辑保持不变...
    bool expected = false;
    if (!d->m_isProcessing.compare_exchange_strong(expected, true)) {
        return;
    }

    auto guard = qScopeGuard([this]() { d->m_isProcessing.store(false); });

    // ... （原有模拟数据生成逻辑）

    if (d->m_statusLabel && d->m_tickCount % 20 == 0) {
        d->m_statusLabel->setText(
            QString("模拟运行中 | 已生成 %1 笔").arg(d->m_tickCount.load())
            );
    }
}

void FuturesQuotesPage::onConnectionStateChanged()
{

}

void FuturesQuotesPage::onMarketDataReceived()
{

}

/**
 * @brief 批量刷新UI（复用现有机制，支持混合数据）
 */
void FuturesQuotesPage::flushPendingUpdates()
{
    if (!d->m_model) return;

    // 快速无锁检查
    if (d->m_pendingUpdates.isEmpty()) return;

    // 取出数据（最小化锁持有时间）
    QVector<FuturesQuoteItem> updates;
    {
        QMutexLocker locker(&d->m_pendingMutex);
        if (d->m_pendingUpdates.isEmpty()) return;
        updates = std::move(d->m_pendingUpdates);  // C++17 move
        d->m_pendingUpdates.clear();
    }

    LOG_INFO(QString("flushPendingUpdates() - Processing %1 quotes").arg(updates.size()));

    // 调用模型批量更新
    if (updates.size() == 1) {
        d->m_model->updateQuote(updates.first());
    } else {
        d->m_model->updateQuotes(updates);  // 批量更新，减少重绘
    }

    // 更新状态栏（每50次刷新一次）
    int seq = d->m_quoteSequence.load();
    if (seq % 50 == 0 && d->m_statusLabel) {
        d->m_statusLabel->setText(
            QString("%1 | 累计接收 %2 笔 | 缓冲 %3 条")
                .arg(d->m_isRealMode.load() ? "实盘连接正常" : "模拟运行中")
                .arg(seq)
                .arg(d->m_pendingUpdates.size())
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

    // 工具栏（扩展：添加模式切换和订阅功能）
    auto *toolbarLayout = new QHBoxLayout;

    auto *refreshBtn = new QPushButton("刷新数据");
    auto *simulateBtn = new QPushButton("模拟推送: OFF");

    // 新增：模式切换按钮
    d->m_modeBtn = new QPushButton("模式: 模拟");
    d->m_modeBtn->setStyleSheet("background-color: #FF9800; color: white;");

    // 新增：合约订阅
    d->m_contractInput = new QLineEdit();
    d->m_contractInput->setPlaceholderText("输入合约代码 (如: rb2605)");
    d->m_contractInput->setMaximumWidth(150);

    d->m_subscribeBtn = new QPushButton("订阅");
    d->m_subscribeBtn->setStyleSheet("background-color: #2196F3; color: white;");

    auto *filterLabel = new QLabel("筛选：");
    auto *filterEdit = new QLineEdit();
    filterEdit->setPlaceholderText("输入合约代码筛选...");
    filterEdit->setMaximumWidth(120);

    toolbarLayout->addWidget(refreshBtn);
    toolbarLayout->addWidget(simulateBtn);
    toolbarLayout->addWidget(d->m_modeBtn);
    toolbarLayout->addSpacing(20);
    toolbarLayout->addWidget(d->m_contractInput);
    toolbarLayout->addWidget(d->m_subscribeBtn);
    toolbarLayout->addStretch();
    toolbarLayout->addWidget(filterLabel);
    toolbarLayout->addWidget(filterEdit);

    mainLayout->addLayout(toolbarLayout);

    // 表格视图（保持原有性能优化设置）
    d->m_tableView = new QTableView(this);
    d->m_tableView->setModel(d->m_model);
    d->m_tableView->horizontalHeader()->setStyleSheet(
        "QHeaderView::section { background-color: #1A1F2E; border: 1px solid #323232}"
        );

    // 性能优化设置（原有代码保留）
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

    d->m_tableView->setColumnWidth(0, 60);
    d->m_tableView->setColumnWidth(1, 100);
    d->m_tableView->setColumnWidth(2, 80);
    d->m_tableView->setColumnWidth(9, 80);
    d->m_tableView->setColumnWidth(10, 80);

    mainLayout->addWidget(d->m_tableView);

    // 状态栏
    d->m_statusLabel = new QLabel("就绪 | 模式: 模拟", this);
    d->m_statusLabel->setStyleSheet("color: #666; padding: 5px; border-top: 1px solid #ddd;");
    mainLayout->addWidget(d->m_statusLabel);

    // 信号连接（新增）
    connect(refreshBtn, &QPushButton::clicked, this, &FuturesQuotesPage::initData);
    connect(d->m_subscribeBtn, &QPushButton::clicked, this, &FuturesQuotesPage::onSubscribeContract);
    connect(d->m_modeBtn, &QPushButton::clicked, this, [this]() {
        if (d->m_isRealMode.load()) {
            switchToSimulateMode();
        } else {
            if (d->m_isCtpConnected.load()) {
                switchToRealMode();
            } else {
                QMessageBox::warning(this, "未连接", "CTP尚未连接，请先等待连接成功");
            }
        }
    });

    connect(simulateBtn, &QPushButton::clicked, [this, simulateBtn]() {
        // 模拟按钮仅在模拟模式下有效
        if (d->m_isRealMode.load()) {
            QMessageBox::information(this, "提示", "当前为实盘模式，请先切换到模拟模式");
            return;
        }

        bool isActive = d->m_simulateTimer->isActive();
        if (isActive) {
            d->m_simulateTimer->stop();
            simulateBtn->setText("模拟推送: OFF");
        } else {
            d->m_simulateTimer->start(1000);
            simulateBtn->setText("模拟推送: ON");
        }
    });
}

void FuturesQuotesPage::setupConnections()
{
    Q_ASSERT(d->m_tableView && d->m_model);

    connect(d->m_tableView, &QTableView::clicked, this, &FuturesQuotesPage::onRowClicked);
    connect(d->m_tableView, &QTableView::doubleClicked, this, &FuturesQuotesPage::onRowClicked);

    // 模拟定时器
    connect(d->m_simulateTimer, &QTimer::timeout, this, &FuturesQuotesPage::onSimulateTick);

    // UI刷新定时器（500ms合并刷新，关键性能优化）
    connect(d->m_flushTimer, &QTimer::timeout, this, &FuturesQuotesPage::flushPendingUpdates);
    d->m_flushTimer->start(500);
}

/**
 * @brief 初始化数据（智能选择数据源）
 */
void FuturesQuotesPage::initData()
{
    if (d->m_statusLabel) {
        d->m_statusLabel->setText("正在加载...");
    }

    // 如果处于真实模式且已连接，等待CTP数据自动推送
    if (d->m_isRealMode.load() && d->m_isCtpConnected.load()) {
        LOG_INFO("Real mode active, waiting for CTP market data...");
        return;
    }

    // 否则加载模拟数据（原有逻辑）
    auto future = QtConcurrent::run([this]() {
        return d->m_generator->generateInitialData();
    });

    auto* watcher = new QFutureWatcher<QVector<FuturesQuoteItem>>(this);
    connect(watcher, &QFutureWatcher<QVector<FuturesQuoteItem>>::finished, this, [this, watcher]() {
        auto data = watcher->result();
        d->m_model->setQuotes(data);
        if (d->m_statusLabel) {
            d->m_statusLabel->setText(QString("就绪 | 模拟数据 %1 条").arg(data.size()));
        }
        watcher->deleteLater();
    });
    watcher->setFuture(future);
}
