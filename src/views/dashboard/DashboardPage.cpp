#include "DashboardPage.h"
#include <QHBoxLayout>
#include <QToolBar>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QKeyEvent>
#include <QShortcut>
#include <QTimer>
#include <QMessageBox>
#include <QDebug>


struct DashboardPage::Impl
{
    // UI组件 - 控制栏
    QWidget* m_toolBar = nullptr;
    QWidget* m_statusBar = nullptr;
    TreeMapWidget* m_treeMap = nullptr;

    // 控件指针 - 按初始化顺序排列
    QComboBox* m_marketCombo = nullptr;
    QComboBox* m_industryCombo = nullptr;
    QLineEdit* m_searchEdit = nullptr;
    QPushButton* m_refreshBtn = nullptr;
    QPushButton* m_reviewBtn = nullptr;
    QPushButton* m_playBtn = nullptr;

    // 状态栏标签
    QLabel* m_timeLabel = nullptr;
    QLabel* m_upCountLabel = nullptr;
    QLabel* m_flatCountLabel = nullptr;
    QLabel* m_downCountLabel = nullptr;
    QLabel* m_turnoverLabel = nullptr;
    QLabel* m_selectedLabel = nullptr;
    QLabel* searchLabel = nullptr;

    // 定时器指针
    QTimer* m_reviewTimer = nullptr;
    QTimer* m_clockTimer = nullptr;

    // 数据管理
    QuoteDataManager* m_dataManager = nullptr;

    // 状态变量 - 按初始化顺序排列
    QString m_currentMarket = nullptr;
    bool m_autoPlaying = false;
};

/**
 * @brief 构造函数
 * 初始化仪表盘页面，创建所有UI组件和数据管理器
 * @param parent 父窗口指针
 */
DashboardPage::DashboardPage(QWidget* parent)
    : BasePage(parent)
      , d(std::make_unique<Impl>())
{
    // 设置对象名称，便于样式表定位
    setObjectName("DashboardPage");

    // 设置黑色背景，金融软件标准暗色主题
    setStyleSheet("background-color: #0a0a0a;");

    // 设置焦点策略以接收键盘事件
    setFocusPolicy(Qt::StrongFocus);

    // 初始化UI布局
    initUI();

    // 连接信号槽
    connectSignals();

    // 初始化数据
    initData();
}

/**
 * @brief 析构函数
 * 清理资源，停止定时器
 */
DashboardPage::~DashboardPage()
{
    if (d->m_dataManager)
    {
        d->m_dataManager->stopAutoUpdate();
    }

    if (d->m_reviewTimer && d->m_reviewTimer->isActive())
    {
        d->m_reviewTimer->stop();
    }

    if (d->m_clockTimer && d->m_clockTimer->isActive())
    {
        d->m_clockTimer->stop();
    }
}

QString DashboardPage::pageId() const
{
    // 使用QStringLiteral避免运行时字符串分配
    return QStringLiteral("DashboardPage");
}

void DashboardPage::initializePage()
{
}

/**
 * @brief 初始化UI
 * 创建整体布局：顶部工具栏 + 中央热力图 + 底部状态栏
 */
void DashboardPage::initUI()
{
    // 主布局：垂直排列
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // 1. 顶部控制栏（工具栏）
    initToolBar();
    mainLayout->addWidget(d->m_toolBar);

    // 2. 中央热力图控件
    d->m_treeMap = new TreeMapWidget(this);
    d->m_treeMap->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mainLayout->addWidget(d->m_treeMap, 1); // 占据所有剩余空间

    // 3. 底部信息栏（状态栏）
    initStatusBar();
    mainLayout->addWidget(d->m_statusBar);

    setLayout(mainLayout);
}

/**
 * @brief 初始化工具栏
 * 包含：市场选择、行业筛选、搜索框、刷新按钮、复盘控制
 */
void DashboardPage::initToolBar()
{
    d->m_toolBar = new QWidget(this);
    d->m_toolBar->setFixedHeight(50);
    d->m_toolBar->setStyleSheet(
        "QWidget { background-color: #1a1a1a; border-bottom: 1px solid #333; }"
        "QPushButton { "
        "   background-color: #2a2a2a; color: #fff; border: 1px solid #444;"
        "   padding: 5px 15px; border-radius: 3px; font-size: 12px;"
        "}"
        "QPushButton:hover { background-color: #3a3a3a; }"
        "QPushButton:checked { background-color: #0066cc; border-color: #0088ff; }"
        "QComboBox { "
        "   background-color: #2a2a2a; color: #fff; border: 1px solid #444;"
        "   padding: 5px; min-width: 100px; border-radius: 3px;"
        "}"
        "QComboBox::drop-down { border: none; width: 20px; }"
        "QComboBox QAbstractItemView { background-color: #2a2a2a; color: #fff; }"
        "QLineEdit { "
        "   background-color: #2a2a2a; color: #fff; border: 1px solid #444;"
        "   padding: 5px; border-radius: 3px; min-width: 150px;"
        "}"
        "QLabel { color: #aaa; font-size: 12px; }"
    );

    auto* toolLayout = new QHBoxLayout(d->m_toolBar);
    toolLayout->setSpacing(15);
    toolLayout->setContentsMargins(15, 5, 15, 5);

    // 标题标签
    auto* titleLabel = new QLabel("行情热力图", d->m_toolBar);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #fff; border: none;");
    toolLayout->addWidget(titleLabel);

    toolLayout->addSpacing(20);

    // 市场选择下拉框
    auto* marketLabel = new QLabel("市场:", d->m_toolBar);
    toolLayout->addWidget(marketLabel);

    d->m_marketCombo = new QComboBox(d->m_toolBar);
    d->m_marketCombo->addItem("A股全图", "all");
    d->m_marketCombo->addItem("上证主板", "sh");
    d->m_marketCombo->addItem("深证主板", "sz");
    d->m_marketCombo->addItem("创业板", "cyb");
    d->m_marketCombo->addItem("科创板", "kcb");
    d->m_marketCombo->addItem("沪深300", "hs300");
    d->m_marketCombo->addItem("中证500", "zz500");
    d->m_marketCombo->addItem("期货全图", "futures");
    d->m_marketCombo->setCurrentIndex(0);
    toolLayout->addWidget(d->m_marketCombo);

    // 分隔线
    toolLayout->addSpacing(10);

    // 行业筛选（预留，实际数据动态加载）
    auto* industryLabel = new QLabel("行业:", d->m_toolBar);
    toolLayout->addWidget(industryLabel);

    d->m_industryCombo = new QComboBox(d->m_toolBar);
    d->m_industryCombo->addItem("全部", "");
    // 行业列表将在数据加载后填充
    d->m_industryCombo->setEnabled(false); // 初始禁用，等待数据
    toolLayout->addWidget(d->m_industryCombo);

    toolLayout->addSpacing(10);

    // 搜索框
    d->searchLabel = new QLabel("搜索:", d->m_toolBar);
    toolLayout->addWidget(d->searchLabel);

    d->m_searchEdit = new QLineEdit(d->m_toolBar);
    d->m_searchEdit->setPlaceholderText("代码/名称...");
    toolLayout->addWidget(d->m_searchEdit);

    // 弹性空间
    toolLayout->addStretch(1);

    // 操作按钮组
    // 刷新按钮
    d->m_refreshBtn = new QPushButton("🔄 刷新", d->m_toolBar);
    d->m_refreshBtn->setToolTip("立即刷新数据 (F5)");
    toolLayout->addWidget(d->m_refreshBtn);

    // 复盘模式开关
    d->m_reviewBtn = new QPushButton("📊 复盘", d->m_toolBar);
    d->m_reviewBtn->setCheckable(true);
    d->m_reviewBtn->setToolTip("开启历史复盘模式 (R)");
    toolLayout->addWidget(d->m_reviewBtn);

    // 播放/暂停按钮（复盘专用）
    d->m_playBtn = new QPushButton("▶ 播放", d->m_toolBar);
    d->m_playBtn->setEnabled(false); // 初始禁用，复盘模式开启后可用
    d->m_playBtn->setToolTip("自动播放复盘 (Space)");
    toolLayout->addWidget(d->m_playBtn);

    // 帮助按钮
    auto* helpBtn = new QPushButton("❓", d->m_toolBar);
    helpBtn->setFixedWidth(30);
    helpBtn->setToolTip("快捷键：←→复盘翻页 | F5刷新 | F11全屏");
    toolLayout->addWidget(helpBtn);

    connect(helpBtn, &QPushButton::clicked, this, &DashboardPage::showHelp);
}

/**
 * @brief 初始化状态栏
 * 显示：当前时间、涨跌统计、成交额、选中项信息
 */
void DashboardPage::initStatusBar()
{
    d->m_statusBar = new QWidget(this);
    d->m_statusBar->setFixedHeight(35);
    d->m_statusBar->setStyleSheet(
        "QWidget { background-color: #1a1a1a; border-top: 1px solid #333; }"
        "QLabel { color: #ccc; font-size: 12px; padding: 0 10px; }"
    );

    auto* statusLayout = new QHBoxLayout(d->m_statusBar);
    statusLayout->setSpacing(10);
    statusLayout->setContentsMargins(15, 5, 15, 5);

    // 当前时间
    d->m_timeLabel = new QLabel("2024-01-01 09:30:00", d->m_statusBar);
    d->m_timeLabel->setStyleSheet("color: #fff; font-weight: bold;");
    statusLayout->addWidget(d->m_timeLabel);

    statusLayout->addStretch(1);

    // 统计信息组
    auto* statsLayout = new QHBoxLayout();
    statsLayout->setSpacing(20);

    // 上涨家数（红色）
    d->m_upCountLabel = new QLabel("上涨: --", d->m_statusBar);
    d->m_upCountLabel->setStyleSheet("color: #ff4d4d;"); // 亮红色
    statsLayout->addWidget(d->m_upCountLabel);

    // 平盘家数（灰色）
    d->m_flatCountLabel = new QLabel("平盘: --", d->m_statusBar);
    d->m_flatCountLabel->setStyleSheet("color: #888;");
    statsLayout->addWidget(d->m_flatCountLabel);

    // 下跌家数（绿色）
    d->m_downCountLabel = new QLabel("下跌: --", d->m_statusBar);
    d->m_downCountLabel->setStyleSheet("color: #00ff00;"); // 亮绿色
    statsLayout->addWidget(d->m_downCountLabel);

    // 分隔线
    auto* sep1 = new QLabel("|", d->m_statusBar);
    sep1->setStyleSheet("color: #444;");
    statsLayout->addWidget(sep1);

    // 总成交额
    d->m_turnoverLabel = new QLabel("成交额: --亿", d->m_statusBar);
    statsLayout->addWidget(d->m_turnoverLabel);

    // 分隔线
    auto* sep2 = new QLabel("|", d->m_statusBar);
    sep2->setStyleSheet("color: #444;");
    statsLayout->addWidget(sep2);

    // 当前选中项
    d->m_selectedLabel = new QLabel("选中: 无", d->m_statusBar);
    d->m_selectedLabel->setStyleSheet("color: #fff;");
    statsLayout->addWidget(d->m_selectedLabel);

    statusLayout->addLayout(statsLayout);

    // 初始化时间更新定时器
    d->m_clockTimer = new QTimer(this);
    connect(d->m_clockTimer, &QTimer::timeout, this, &DashboardPage::updateTimeDisplay);
    d->m_clockTimer->start(1000); // 每秒更新
    updateTimeDisplay();
}

/**
 * @brief 连接信号槽
 * 绑定所有交互事件
 */
void DashboardPage::connectSignals()
{
    // 市场切换
    connect(d->m_marketCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DashboardPage::onMarketChanged);

    // 行业筛选
    connect(d->m_industryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DashboardPage::onIndustryChanged);

    // 搜索框实时过滤
    connect(d->m_searchEdit, &QLineEdit::textChanged,
            this, &DashboardPage::onSearchTextChanged);

    // 搜索框回车确认
    connect(d->m_searchEdit, &QLineEdit::returnPressed,
            this, &DashboardPage::onRefreshClicked);

    // 刷新按钮
    connect(d->m_refreshBtn, &QPushButton::clicked,
            this, &DashboardPage::onRefreshClicked);

    // 复盘模式开关
    connect(d->m_reviewBtn, &QPushButton::toggled,
            this, &DashboardPage::onReviewModeToggled);

    // 播放/暂停
    connect(d->m_playBtn, &QPushButton::clicked,
            this, &DashboardPage::onAutoPlay);

    // 热力图信号
    connect(d->m_treeMap, &TreeMapWidget::itemDoubleClicked,
            this, &DashboardPage::onItemDoubleClicked);

    connect(d->m_treeMap, &TreeMapWidget::itemHovered,
            this, &DashboardPage::onItemHovered);

    connect(d->m_treeMap, &TreeMapWidget::statsChanged,
            this, &DashboardPage::onStatsChanged);

    // 复盘定时器
    d->m_reviewTimer = new QTimer(this);
    connect(d->m_reviewTimer, &QTimer::timeout, this, [this]()
    {
        d->m_treeMap->nextTimeFrame();
    });
}

/**
 * @brief 初始化数据
 * 创建数据管理器并加载初始数据
 */
void DashboardPage::initData()
{
    d->m_dataManager = new QuoteDataManager(this);

    // 连接数据更新信号
    connect(d->m_dataManager, &QuoteDataManager::dataUpdated,
            this, &DashboardPage::onDataUpdated);

    connect(d->m_dataManager, &QuoteDataManager::itemUpdated,
            this, [this](const StockQuoteItem& item)
            {
                // 单个项目更新时，如果正在显示该项目，可以刷新UI
                Q_UNUSED(item)
                // 实际应用中可以优化为局部刷新
            });

    // 加载初始数据（A股全图，500只模拟股票）
    loadMarketData("all");

    // 启动自动更新（8秒间隔，与52etf.site保持一致）
    d->m_dataManager->startAutoUpdate(8000);
}

/**
 * @brief 市场切换处理
 * 根据选择的市场加载对应数据
 */
void DashboardPage::onMarketChanged(int index) const
{
    QString market = d->m_marketCombo->currentData().toString();
    d->m_currentMarket = market;

    loadMarketData(market);

    // 更新行业下拉框状态（期货没有行业概念）
    bool isFutures = (market == "futures");
    d->m_industryCombo->setEnabled(!isFutures);
    if (isFutures)
    {
        d->m_industryCombo->setCurrentIndex(0);
    }
}

/**
 * @brief 行业筛选处理
 */
void DashboardPage::onIndustryChanged(int index) const
{
    Q_UNUSED(index)
    if (const QString industry = d->m_industryCombo->currentData().toString(); industry.isEmpty())
    {
        d->m_treeMap->filterByMarket(d->m_currentMarket);
    }
    else
    {
        d->m_treeMap->filterByIndustry(industry);
    }
}

/**
 * @brief 搜索文本变化处理
 * 实时过滤显示
 */
void DashboardPage::onSearchTextChanged(const QString& text) const
{
    if (text.isEmpty())
    {
        // 空搜索恢复市场筛选
        d->m_treeMap->filterByMarket(d->m_currentMarket);
    }
    else if (text.length() >= 2)
    {
        // 至少2个字符才开始搜索，避免频繁刷新
        d->m_treeMap->search(text);
    }
}

/**
 * @brief 刷新按钮点击
 * 立即重新加载数据
 */
void DashboardPage::onRefreshClicked() const
{
    loadMarketData(d->m_currentMarket);

    // 显示刷新提示
    d->m_refreshBtn->setText("🔄 刷新中...");
    QTimer::singleShot(500, this, [this]()
    {
        d->m_refreshBtn->setText("🔄 刷新");
    });
}

/**
 * @brief 加载市场数据
 * 根据市场类型调用数据管理器生成或获取数据
 * @param market 市场标识符
 */
void DashboardPage::loadMarketData(const QString& market) const
{
    if (!d->m_dataManager) return;

    if (market == "futures")
    {
        // 生成期货数据（12个主要品种）
        d->m_dataManager->generateMockFuturesData();
    }
    else
    {
        // 生成股票模拟数据
        int count = 500;
        if (market == "hs300") count = 300;
        else if (market == "zz500") count = 500;
        else if (market == "cyb" || market == "kcb") count = 200;

        d->m_dataManager->generateMockData(count);
    }
}

/**
 * @brief 数据更新处理
 * 将新数据传递给热力图控件
 */
void DashboardPage::onDataUpdated() const
{
    if (!d->m_dataManager) return;

    const QVector<StockQuoteItem> items = d->m_dataManager->getAllItems();
    d->m_treeMap->setData(items);

    // 更新行业下拉框（基于当前数据的行业分布）
    updateIndustryCombo(items);
}

/**
 * @brief 更新行业下拉框选项
 * 从数据中提取行业列表
 */
void DashboardPage::updateIndustryCombo(const QVector<StockQuoteItem>& items) const
{
    QSet<QString> industries;
    for (const auto& item : items)
    {
        if (!item.industry.isEmpty())
        {
            industries.insert(item.industry);
        }
    }

    QStringList list = industries.values();
    list.sort();

    // 保存当前选择
    const QString current = d->m_industryCombo->currentData().toString();

    d->m_industryCombo->clear();
    d->m_industryCombo->addItem("全部", "");

    for (const QString& ind : list)
    {
        d->m_industryCombo->addItem(ind, ind);
    }

    // 恢复选择
    int index = d->m_industryCombo->findData(current);
    if (index >= 0)
    {
        d->m_industryCombo->setCurrentIndex(index);
    }
}

/**
 * @brief 复盘下一帧
 * 调用 TreeMapWidget 的 public 方法，不直接访问 protected 事件
 */
void DashboardPage::nextReviewFrame() const
{
    if (!d->m_treeMap) return;

    if (d->m_reviewBtn && d->m_reviewBtn->isChecked())
    {
        // 复盘模式：时间前进
        d->m_treeMap->nextTimeFrame();
    }
    else
    {
        // 普通模式：通过发送事件让 TreeMapWidget 处理导航
        auto event = new QKeyEvent(QKeyEvent::KeyPress, Qt::Key_Right, Qt::NoModifier);
        QApplication::sendEvent(d->m_treeMap, event); // 修正：直接使用 m_treeMap，不是 d->m_treeMap
        delete event;
    }
}

/**
 * @brief 复盘上一帧
 */
void DashboardPage::prevReviewFrame() const
{
    if (!d->m_treeMap) return;

    if (d->m_reviewBtn && d->m_reviewBtn->isChecked())
    {
        d->m_treeMap->prevTimeFrame();
    }
    else
    {
        // 普通模式：选择上一个
        auto* event = new QKeyEvent(QKeyEvent::KeyPress, Qt::Key_Left, Qt::NoModifier);
        QApplication::sendEvent(d->m_treeMap, event);
        delete event;
    }
}

/**
 * @brief 切换自动播放
 */
void DashboardPage::toggleAutoPlay() const
{
    if (!d->m_reviewBtn || !d->m_reviewBtn->isChecked()) return;

    d->m_autoPlaying = !d->m_autoPlaying;

    if (d->m_playBtn)
    {
        d->m_playBtn->setText(d->m_autoPlaying ? "⏸ 暂停" : "▶ 播放");
    }

    if (d->m_autoPlaying)
    {
        if (d->m_reviewTimer) d->m_reviewTimer->start(1000);
    }
    else
    {
        if (d->m_reviewTimer) d->m_reviewTimer->stop();
    }
}

/**
 * @brief 复盘模式切换
 * @param enabled true开启复盘，false关闭
 */
void DashboardPage::onReviewModeToggled(const bool enabled)
{
    d->m_treeMap->setReviewMode(enabled);
    d->m_playBtn->setEnabled(enabled);

    if (enabled)
    {
        d->m_reviewBtn->setText("📊 复盘中");
        d->m_dataManager->stopAutoUpdate(); // 复盘时暂停实时更新

        // 加载历史数据（模拟）
        QMessageBox::information(this, "复盘模式",
                                 "复盘模式已开启。\n使用 ← → 方向键浏览历史数据，空格键播放/暂停。");
    }
    else
    {
        d->m_reviewBtn->setText("📊 复盘");
        d->m_autoPlaying = false;
        d->m_playBtn->setText("▶ 播放");
        d->m_reviewTimer->stop();
        d->m_dataManager->startAutoUpdate(8000); // 恢复实时更新
    }
}

/**
 * @brief 自动播放复盘
 * 开启/停止定时自动翻页
 */
void DashboardPage::onAutoPlay() const
{
    toggleAutoPlay();
}

/**
 * @brief 双击项目处理
 * 打开详细K线图（预留接口）
 */
void DashboardPage::onItemDoubleClicked(const StockQuoteItem& item)
{
    // 这里可以弹出K线对话框或发送信号给主窗口打开详情页
    QMessageBox::information(this, "详情",
                             QString("股票: %1 %2\n现价: %3\n涨跌: %4%\n市值: %5亿")
                             .arg(item.code, item.name)
                             .arg(item.price, 0, 'f', 2)
                             .arg(item.changePercent, 0, 'f', 2)
                             .arg(item.marketCap, 0, 'f', 2));
}

/**
 * @brief 鼠标悬停处理
 * 更新状态栏选中信息
 */
void DashboardPage::onItemHovered(const StockQuoteItem& item) const
{
    d->m_selectedLabel->setText(
        QString("选中: %1 %2 | %3%")
        .arg(item.code, item.name)
        .arg(item.changePercent, 0, 'f', 2)
    );
}

/**
 * @brief 统计数据更新
 * 来自TreeMapWidget的统计信号，更新底部状态栏
 */
void DashboardPage::onStatsChanged(int up, int flat, int down, double turnover) const
{
    d->m_upCountLabel->setText(QString("上涨: %1").arg(up));
    d->m_flatCountLabel->setText(QString("平盘: %1").arg(flat));
    d->m_downCountLabel->setText(QString("下跌: %1").arg(down));
    d->m_turnoverLabel->setText(QString("成交额: %1亿").arg(turnover, 0, 'f', 2));
}

/**
 * @brief 更新时间显示
 * 每秒更新状态栏时间
 */
void DashboardPage::updateTimeDisplay() const
{
    QDateTime now = QDateTime::currentDateTime();
    QString timeStr = now.toString("yyyy-MM-dd hh:mm:ss");

    // 判断交易时间（A股 9:30-11:30, 13:00-15:00）
    int hour = now.time().hour();
    int minute = now.time().minute();
    bool isTrading = ((hour == 9 && minute >= 30) || (hour == 10) ||
        (hour == 11 && minute <= 30) ||
        (hour >= 13 && hour < 15) ||
        (hour == 15 && minute == 0));

    if (isTrading)
    {
        timeStr += " (交易中)";
        d->m_timeLabel->setStyleSheet("color: #00ff00; font-weight: bold;");
    }
    else
    {
        timeStr += " (休市)";
        d->m_timeLabel->setStyleSheet("color: #888; font-weight: bold;");
    }

    d->m_timeLabel->setText(timeStr);
}

/**
 * @brief 显示帮助信息
 */
void DashboardPage::showHelp()
{
    QString help = R"(快捷键说明：
• F5          - 立即刷新数据
• F11         - 全屏切换
• R           - 切换复盘模式
• Space       - 复盘播放/暂停
• ← / →       - 复盘时间后退/前进
• ↑ / ↓       - 选择上一个/下一个股票
• ESC         - 取消选中

操作提示：
• 色块面积代表流通市值大小
• 颜色红涨绿跌（A股风格）
• 双击色块查看详细行情
• 滚轮调整最小显示市值
• 8秒自动刷新数据)";

    QMessageBox::information(this, "帮助", help);
}

/**
 * @brief 键盘事件处理
 * 实现快捷键功能
 */
void DashboardPage::keyPressEvent(QKeyEvent* event)
{
    // 先处理 DashboardPage 级别的快捷键
    switch (event->key())
    {
    case Qt::Key_F5:
        onRefreshClicked();
        event->accept();
        return;

    case Qt::Key_F11:
        // 通知主窗口切换全屏，或自行处理
        event->ignore(); // 传递给父级处理
        return;

    case Qt::Key_R:
        if (d->m_reviewBtn) d->m_reviewBtn->toggle();
        event->accept();
        return;

    case Qt::Key_Space:
        if (d->m_reviewBtn && d->m_reviewBtn->isChecked())
        {
            toggleAutoPlay();
        }
        event->accept();
        return;

    case Qt::Key_Right:
    case Qt::Key_Up:
        // 复盘前进或选择下一个
        nextReviewFrame();
        event->accept();
        return;

    case Qt::Key_Left:
    case Qt::Key_Down:
        // 复盘后退或选择上一个
        prevReviewFrame();
        event->accept();
        return;

    case Qt::Key_Escape:
        // 取消选中
        if (d->m_treeMap)
        {
            // 调用 public 方法清除选择，而不是 protected 的 keyPressEvent
            // 假设 TreeMapWidget 有 clearSelection 或类似方法
            // 或者通过 selectItem 空字符串实现
            d->m_treeMap->selectItem("");
        }
        event->accept();
        return;

    default:
        QWidget::keyPressEvent(event); // 或 BasePage::keyPressEvent(event)
    }
}

void DashboardPage::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event); // 或 BasePage::resizeEvent(event)
}

/**
 * @brief 设置全屏模式
 * 由主窗口调用，调整UI元素显示
 * @param fullscreen 是否全屏
 */
void DashboardPage::setFullScreen(const bool fullscreen) const
{
    if (fullscreen)
    {
        d->m_toolBar->setVisible(false);
        d->m_statusBar->setVisible(false);
    }
    else
    {
        d->m_toolBar->setVisible(true);
        d->m_statusBar->setVisible(true);
    }
}

/**
 * @brief 公开接口：手动刷新
 * 供外部调用强制刷新数据
 */
void DashboardPage::refreshData() const
{
    onRefreshClicked();
}

/**
 * @brief 公开接口：设置市场
 * 供外部程序控制当前显示的市场
 * @param market 市场代码
 */
void DashboardPage::setMarket(const QString& market) const
{
    int index = d->m_marketCombo->findData(market);
    if (index >= 0)
    {
        d->m_marketCombo->setCurrentIndex(index);
    }
}
