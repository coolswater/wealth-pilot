/**
 * @file NewsPage.cpp
 * @brief 新闻资讯页面实现 - 对接华尔街见闻API
 */

#include "NewsPage.h"
#include "core/config/Tokens.h"
#include "utils/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QTextBrowser>
#include <QComboBox>
#include <QTimer>
#include <QDateTime>
#include <QScrollArea>
#include <QFrame>
#include <QDesktopServices>
#include <QUrl>

using namespace Tokens::Colors;

// ============================================================================
// NewsPage::Impl
// ============================================================================

class NewsPage::Impl {
public:
    // UI 组件
    QListWidget* newsList = nullptr;
    QTextBrowser* contentBrowser = nullptr;
    QLabel* titleLabel = nullptr;
    QLabel* sourceLabel = nullptr;
    QLabel* timeLabel = nullptr;
    QComboBox* categoryCombo = nullptr;
    QLabel* statusLabel = nullptr;
    QPushButton* openUrlBtn = nullptr;
    
    // 数据源
    NewsDataSource* dataSource = nullptr;
    
    // 数据
    QVector<NewsItem> allNews;
    NewsItem currentNews;
    
    // 定时刷新
    QTimer* refreshTimer = nullptr;
    
    // 当前筛选
    QString currentCategory;
};

// ============================================================================
// NewsPage 实现
// ============================================================================

NewsPage::NewsPage(QWidget* parent)
    : BasePage(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
    setupConnections();
}

NewsPage::~NewsPage()
{
    if (d->refreshTimer) {
        d->refreshTimer->stop();
    }
}

QString NewsPage::pageId() const
{
    return QStringLiteral("NewsPage");
}

void NewsPage::initializePage()
{
    if (isInitialized()) return;
    
    // 初始化数据源
    d->dataSource = new NewsDataSource(this);
    connect(d->dataSource, &NewsDataSource::newsReceived,
            this, &NewsPage::onNewsReceived);
    
    // 请求初始数据
    requestNews();
    
    // 启动定时刷新（5分钟）
    d->refreshTimer = new QTimer(this);
    connect(d->refreshTimer, &QTimer::timeout, this, &NewsPage::onAutoRefresh);
    d->refreshTimer->start(300000);
    
    setInitialized(true);
    LOG_INFO("NewsPage initialized with WallStCN API");
}

void NewsPage::onPageActivated(const QVariantMap& params)
{
    Q_UNUSED(params);
    if (d->refreshTimer) {
        d->refreshTimer->start(300000);
    }
}

// ============================================================================
// UI 设置
// ============================================================================

void NewsPage::setupUI()
{
    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // 左侧：新闻列表
    auto* leftPanel = new QWidget();
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(8);

    // 工具栏
    auto* toolbarLayout = new QHBoxLayout();
    
    auto* refreshBtn = new QPushButton(QStringLiteral("刷新"));
    refreshBtn->setProperty("ghost", true);
    
    d->categoryCombo = new QComboBox();
    d->categoryCombo->addItem(QStringLiteral("全球"), static_cast<int>(NewsDataSource::Channel::Global));
    d->categoryCombo->addItem(QStringLiteral("A股"), static_cast<int>(NewsDataSource::Channel::AShares));
    d->categoryCombo->addItem(QStringLiteral("美股"), static_cast<int>(NewsDataSource::Channel::USStocks));
    d->categoryCombo->addItem(QStringLiteral("外汇"), static_cast<int>(NewsDataSource::Channel::Forex));
    d->categoryCombo->addItem(QStringLiteral("商品"), static_cast<int>(NewsDataSource::Channel::Commodities));
    d->categoryCombo->setMaximumWidth(100);
    
    toolbarLayout->addWidget(refreshBtn);
    toolbarLayout->addSpacing(10);
    toolbarLayout->addWidget(new QLabel(QStringLiteral("频道:")));
    toolbarLayout->addWidget(d->categoryCombo);
    toolbarLayout->addStretch();
    
    leftLayout->addLayout(toolbarLayout);

    // 新闻列表
    d->newsList = new QListWidget();
    d->newsList->setStyleSheet(QString(R"(
        QListWidget {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 8px;
            padding: 8px;
        }
        QListWidget::item {
            padding: 12px 8px;
            border-bottom: 1px solid %2;
            color: %3;
        }
        QListWidget::item:selected {
            background-color: rgba(59, 130, 246, 0.2);
            color: %4;
        }
        QListWidget::item:hover {
            background-color: rgba(255, 255, 255, 0.05);
        }
    )").arg(BgElevated, Border, TextPrimary, Primary));
    
    leftLayout->addWidget(d->newsList);

    // 状态栏
    d->statusLabel = new QLabel(QStringLiteral("正在加载新闻..."));
    d->statusLabel->setStyleSheet(QString("color: %1; font-size: 12px;").arg(TextSecondary));
    leftLayout->addWidget(d->statusLabel);

    leftPanel->setFixedWidth(400);
    mainLayout->addWidget(leftPanel);

    // 右侧：新闻详情
    auto* rightPanel = new QWidget();
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(16, 16, 16, 16);
    rightLayout->setSpacing(12);

    // 标题
    d->titleLabel = new QLabel(QStringLiteral("选择新闻查看详情"));
    d->titleLabel->setStyleSheet(QString("font-size: 20px; font-weight: 600; color: %1;").arg(TextPrimary));
    d->titleLabel->setWordWrap(true);
    rightLayout->addWidget(d->titleLabel);

    // 元信息
    auto* metaLayout = new QHBoxLayout();
    d->sourceLabel = new QLabel();
    d->sourceLabel->setStyleSheet(QString("color: %1; font-size: 12px;").arg(TextSecondary));
    d->timeLabel = new QLabel();
    d->timeLabel->setStyleSheet(QString("color: %1; font-size: 12px;").arg(TextSecondary));
    
    d->openUrlBtn = new QPushButton(QStringLiteral("查看原文"));
    d->openUrlBtn->setProperty("ghost", true);
    d->openUrlBtn->setVisible(false);
    
    metaLayout->addWidget(d->sourceLabel);
    metaLayout->addSpacing(16);
    metaLayout->addWidget(d->timeLabel);
    metaLayout->addStretch();
    metaLayout->addWidget(d->openUrlBtn);
    rightLayout->addLayout(metaLayout);

    // 分隔线
    auto* separator = new QFrame();
    separator->setFrameShape(QFrame::HLine);
    separator->setStyleSheet(QString("background-color: %1; max-height: 1px;").arg(Border));
    rightLayout->addWidget(separator);

    // 内容
    d->contentBrowser = new QTextBrowser();
    d->contentBrowser->setStyleSheet(QString(R"(
        QTextBrowser {
            background-color: transparent;
            border: none;
            color: %1;
            font-size: 14px;
            line-height: 1.8;
        }
    )").arg(TextPrimary));
    d->contentBrowser->setOpenExternalLinks(true);
    rightLayout->addWidget(d->contentBrowser, 1);

    mainLayout->addWidget(rightPanel, 1);

    // 连接按钮
    connect(refreshBtn, &QPushButton::clicked, this, &NewsPage::onRefreshNews);
    connect(d->openUrlBtn, &QPushButton::clicked, this, [this]() {
        if (!d->currentNews.url.isEmpty()) {
            QDesktopServices::openUrl(QUrl(d->currentNews.url));
        }
    });
}

void NewsPage::setupConnections()
{
    connect(d->newsList, &QListWidget::itemClicked, 
            this, &NewsPage::onNewsSelected);
    connect(d->categoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &NewsPage::onFilterChanged);
}

// ============================================================================
// 数据处理
// ============================================================================

void NewsPage::requestNews()
{
    if (d->dataSource) {
        int channelIndex = d->categoryCombo->currentIndex();
        int channelData = d->categoryCombo->itemData(channelIndex).toInt();
        auto channel = static_cast<NewsDataSource::Channel>(channelData);
        d->dataSource->requestNews(channel, 30);
        d->statusLabel->setText(QStringLiteral("正在加载新闻..."));
    }
}

void NewsPage::onNewsReceived(const QVector<NewsItem>& news)
{
    d->allNews = news;
    updateNewsList();
    
    d->statusLabel->setText(QString(QStringLiteral("华尔街见闻 · %1 条新闻 · %2"))
        .arg(news.size())
        .arg(QDateTime::currentDateTime().toString("HH:mm:ss")));
}

void NewsPage::updateNewsList(const QString& category)
{
    Q_UNUSED(category);
    d->newsList->clear();
    
    for (const auto& news : d->allNews) {
        auto* item = new QListWidgetItem();
        
        // 显示格式：标题
        QString displayText = news.title;
        if (!news.author.isEmpty()) {
            displayText = QString("%1 [%2]").arg(news.title, news.author);
        }
        
        item->setText(displayText);
        item->setData(Qt::UserRole, news.id);
        item->setToolTip(QString("%1\n来源: %2\n时间: %3")
            .arg(news.title, news.source, news.publishTime.toString("yyyy-MM-dd HH:mm")));
        
        d->newsList->addItem(item);
    }
}

void NewsPage::displayNewsDetail(const NewsItem& news)
{
    d->titleLabel->setText(news.title);
    d->sourceLabel->setText(QString(QStringLiteral("来源: %1")).arg(news.source));
    d->timeLabel->setText(news.publishTime.toString("yyyy-MM-dd HH:mm"));
    
    QString content = news.content;
    
    // 添加作者信息
    if (!news.author.isEmpty()) {
        content = QString(QStringLiteral("作者: %1\n\n%2")).arg(news.author, content);
    }
    
    d->contentBrowser->setText(content);
    
    // 显示原文链接按钮
    d->openUrlBtn->setVisible(!news.url.isEmpty());
}

// ============================================================================
// 槽函数
// ============================================================================

void NewsPage::onNewsSelected(QListWidgetItem* item)
{
    QString newsId = item->data(Qt::UserRole).toString();
    
    for (const auto& news : d->allNews) {
        if (news.id == newsId) {
            d->currentNews = news;
            displayNewsDetail(news);
            break;
        }
    }
}

void NewsPage::onFilterChanged(int index)
{
    Q_UNUSED(index);
    requestNews();
}

void NewsPage::onRefreshNews()
{
    requestNews();
    LOG_INFO("News refreshed");
}

void NewsPage::onAutoRefresh()
{
    requestNews();
    LOG_DEBUG("News auto-refreshed");
}
