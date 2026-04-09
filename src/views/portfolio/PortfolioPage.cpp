#include "PortfolioPage.h"
#include <QGraphicsDropShadowEffect>
#include <QLineEdit>
#include <QSplitter>
#include <QComboBox>
#include <QtConcurrent>
#include <QThread>

#include "views/widgets/AssetCardWidget.h"
#include "views/widgets/AssetPieChart.h"

struct PortfolioPage::Impl
{
    QVBoxLayout* mainLayout = nullptr;

    // UI 组件
    QLineEdit* m_searchEdit = nullptr;

    // 统计卡片
    AssetCardWidget* m_totalAssetCard = nullptr;
    AssetCardWidget* m_dailyPnLCard = nullptr;
    AssetCardWidget* m_returnCard = nullptr;
    AssetCardWidget* m_riskCard = nullptr;

    // 图表
    AssetPieChart* m_pieChart = nullptr;
    NetValueChart* m_lineChart = nullptr;

    // 定时器
    QTimer* m_realtimeTimer = nullptr;

    // 性能优化：使用智能指针管理资源
    // std::unique_ptr<QThreadPool> m_threadPool;

    // 数据缓存
    struct AssetData
    {
        double totalAssets = 0.0;
        double dailyPnL = 0.0;
        double returnRate = 0.0;
        double riskLevel = 0.0;
    } m_cachedData;

    // 性能优化：预分配布局容量避免重新分配
    static constexpr int LayoutSpacing = 10;
    static constexpr int ContentsMargins = 20;
};

PortfolioPage::PortfolioPage(QWidget* parent)
    : BasePage(parent)
      , d(std::make_unique<Impl>())
{
    setupUI();
    // 构造函数保持轻量，所有UI延迟到initializePage构建
    setObjectName("PortfolioPage");

    // 实时数据更新定时器（每5秒）
    d->m_realtimeTimer = new QTimer(this);
    d->m_realtimeTimer->setInterval(5000);
    connect(d->m_realtimeTimer, &QTimer::timeout, this, &PortfolioPage::updateRealTimeData);
}

PortfolioPage::~PortfolioPage() = default;

QString PortfolioPage::pageId() const
{
    // 使用QStringLiteral避免运行时字符串分配
    return QStringLiteral("PortfolioPage");
}

void PortfolioPage::initializePage()
{
    if (isInitialized()) return; // 防止重复初始化

    setupAnimations();
    connectSignals();

    setInitialized(true);
    emit pageStatusChanged(QStringLiteral("initialized"));
}

void PortfolioPage::setupUI()
{
    // 1. 主布局配置 - 零边距融入父容器
    d->mainLayout = new QVBoxLayout(this);
    d->mainLayout->setContentsMargins(
        Impl::ContentsMargins,
        Impl::ContentsMargins,
        Impl::ContentsMargins,
        Impl::ContentsMargins
    );
    d->mainLayout->setSpacing(Impl::LayoutSpacing);
    d->mainLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    // 1. 顶部标题栏
    createHeader();
    // 2. 统计卡片区域
    createCardsSection();
    auto* cardsContainer = new QWidget(this);
    auto* cardsLayout = new QHBoxLayout(cardsContainer);
    cardsLayout->setSpacing(16);
    cardsLayout->addWidget(d->m_totalAssetCard);
    cardsLayout->addWidget(d->m_dailyPnLCard);
    cardsLayout->addWidget(d->m_returnCard);
    cardsLayout->addWidget(d->m_riskCard);
    d->mainLayout->addWidget(cardsContainer);

    // 3. 图表区域（使用Splitter允许调整大小）
    createChartsSection();
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(d->m_pieChart);
    splitter->addWidget(d->m_lineChart);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    d->mainLayout->addWidget(splitter, 1);

    // 初始化样式
    setupAnimations();
}

void PortfolioPage::createHeader()
{
    auto* header = new QWidget(this);
    auto* layout = new QHBoxLayout(header);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addStretch();

    // 搜索框
    d->m_searchEdit = new QLineEdit(this);
    d->m_searchEdit->setPlaceholderText("搜索股票、基金、期货...");
    d->m_searchEdit->setProperty("search", "true");
    d->m_searchEdit->setFixedWidth(300);
    d->m_searchEdit->setClearButtonEnabled(true);
    connect(d->m_searchEdit, &QLineEdit::textChanged, this, &PortfolioPage::onSearch);
    layout->addWidget(d->m_searchEdit);

    // 时间范围选择
    auto timeRange = new QComboBox(this);
    timeRange->addItems({"1周", "1月", "3月", "1年", "今年", "全部"});
    timeRange->setCurrentIndex(1); // 默认1月
    connect(timeRange, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PortfolioPage::onTimeRangeChanged);
    layout->addWidget(timeRange);

    d->mainLayout->addWidget(header); // 添加整个头部
}

void PortfolioPage::createCardsSection()
{
    // 总资产卡片
    d->m_totalAssetCard = new AssetCardWidget(this);
    d->m_totalAssetCard->setTitle("总资产");
    d->m_totalAssetCard->setValue("¥1,245,680", true);
    d->m_totalAssetCard->setSubValue("较昨日 +¥28,450");
    d->m_totalAssetCard->setTrend(2.34);
    d->m_totalAssetCard->setTheme(AssetCardWidget::Primary);

    // 今日盈亏卡片
    d->m_dailyPnLCard = new AssetCardWidget(this);
    d->m_dailyPnLCard->setTitle("今日盈亏");
    d->m_dailyPnLCard->setValue("+¥28,450", true);
    d->m_dailyPnLCard->setSubValue("股票 +¥18,200 | 期货 +¥10,250");
    d->m_dailyPnLCard->setTrend(5.2);
    d->m_dailyPnLCard->setTheme(AssetCardWidget::Success);

    // 持仓收益率卡片
    d->m_returnCard = new AssetCardWidget(this);
    d->m_returnCard->setTitle("持仓收益率");
    d->m_returnCard->setValue("+15.8%", true);
    d->m_returnCard->setSubValue("沪深300同期 +10.6%");
    d->m_returnCard->setTrend(0.0, "跑赢大盘");
    d->m_returnCard->setTheme(AssetCardWidget::Success);

    // 期货风险度卡片
    d->m_riskCard = new AssetCardWidget(this);
    d->m_riskCard->setTitle("期货风险度");
    d->m_riskCard->setValue("中等", false);
    d->m_riskCard->setSubValue("保证金占用 ¥210,000");
    d->m_riskCard->setTrend(-35.0);
    d->m_riskCard->setTheme(AssetCardWidget::Warning);
}

void PortfolioPage::createChartsSection()
{
    // 资产配置饼图
    d->m_pieChart = new AssetPieChart(this);

    QVector<std::pair<QString, double>> assets = {
        {"股票", 45.0},
        {"期货", 25.0},
        {"基金", 20.0},
        {"现金", 10.0}
    };
    d->m_pieChart->setData(assets);

    // 净值走势图
    d->m_lineChart = new NetValueChart(this);

    // 生成模拟数据
    QVector<TimeValuePoint> values;
    QVector<TimeValuePoint> benchmark;
    const QDateTime now = QDateTime::currentDateTime();
    for (int i = 30; i >= 0; --i)
    {
        QDateTime date = now.addDays(-i);
        double val = 10.0 + (30 - i) * 0.2 + QRandomGenerator::global()->bounded(2.0);
        double bench = 10.0 + (30 - i) * 0.15 + QRandomGenerator::global()->bounded(1.0);
        values.append({date, val});
        benchmark.append({date, bench});
    }
    d->m_lineChart->setData(values);
    d->m_lineChart->setBenchmarkData(benchmark);
}

void PortfolioPage::setupAnimations()
{
    // 卡片入场动画
    auto* group = new QSequentialAnimationGroup(this);
    AssetCardWidget* cards[] = {d->m_totalAssetCard, d->m_dailyPnLCard, d->m_returnCard, d->m_riskCard};
    for (int i = 0; i < 4; ++i)
    {
        auto* anim = new QPropertyAnimation(cards[i], "pos");
        anim->setDuration(600);
        anim->setStartValue(QPoint(cards[i]->x(), cards[i]->y() + 50));
        anim->setEndValue(cards[i]->pos());
        anim->setEasingCurve(QEasingCurve::OutBack);

        auto* seq = new QSequentialAnimationGroup(this);
        seq->addPause(i * 100); // 延迟 i*100 ms
        seq->addAnimation(anim);
        group->addAnimation(seq);
    }
    group->start();
}

void PortfolioPage::connectSignals()
{
}

void PortfolioPage::refreshData()
{
    // 异步数据加载避免阻塞UI
    auto future = QtConcurrent::run([this]()
    {
        QThread::msleep(500);
        QMetaObject::invokeMethod(this, [this]()
        {
            setupDemoData();
        }, Qt::QueuedConnection);
    });
    // 如果不需要等待或取消，可以不做任何事，但变量存在即可消除警告
    Q_UNUSED(future); // 或者直接 (void)future;
}

void PortfolioPage::setupDemoData() const
{
    // 更新卡片数据（带动画）
    d->m_totalAssetCard->setValue("¥1,245,680", true);
    d->m_dailyPnLCard->setValue("+¥28,450", true);
    d->m_returnCard->setValue("+15.8%", true);
}

void PortfolioPage::onSearch(const QString& text)
{
    if (text.isEmpty()) return;

    // 实现搜索逻辑
    qDebug() << "Searching for:" << text;
}

void PortfolioPage::onTimeRangeChanged(int index) const
{
    // 根据时间范围重新加载图表数据
    const int days[] = {7, 30, 90, 365, 365, 0};
    int day = days[index];

    auto future = QtConcurrent::run([this, day]()
    {
        QThread::msleep(300);
        QMetaObject::invokeMethod(d->m_lineChart, []()
        {
            // 更新图表
        }, Qt::QueuedConnection);
    });
    Q_UNUSED(future);
}

void PortfolioPage::updateRealTimeData() const
{
    // 模拟实时价格更新
    static double lastPrice = 1245680.0;
    double change = QRandomGenerator::global()->bounded(1000.0) - 500.0;
    lastPrice += change;

    QString value = QString("¥%1").arg(lastPrice, 0, 'f', 0);
    d->m_totalAssetCard->setValue(value, false);
}

void PortfolioPage::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    // 响应式布局调整
}

void PortfolioPage::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    d->m_realtimeTimer->start();
}

void PortfolioPage::hideEvent(QHideEvent* event)
{
    QWidget::hideEvent(event);
    d->m_realtimeTimer->stop();
}
