/**
 * @file NewsPanelWidget.cpp
 * @brief 新闻资讯面板实现 - 使用属性选择器替代硬编码样式
 */

#include "NewsPanelWidget.h"
#include "core/config/Tokens.h"
#include <QHeaderView>
#include <QDateTime>
#include <QStyle>

using namespace Tokens;

NewsPanelWidget::NewsPanelWidget(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("NewsPanelWidget");
    setupUI();

    // 连接新闻数据源信号
    auto* newsSource = NewsDataSource::instance();
    connect(newsSource, &NewsDataSource::newsUpdated,
            this, &NewsPanelWidget::onNewsUpdated);
    connect(newsSource, &NewsDataSource::socialHeatUpdated,
            this, &NewsPanelWidget::onSocialHeatUpdated);
}

NewsPanelWidget::~NewsPanelWidget()
{
}

void NewsPanelWidget::setSymbol(const QString& symbol)
{
    m_currentSymbol = symbol;
    m_titleLabel->setText(QString(QStringLiteral("新闻资讯 - %1")).arg(symbol));

    // 订阅新闻
    NewsDataSource::instance()->subscribeSymbols({symbol});

    // 立即刷新
    refreshNews();
}

void NewsPanelWidget::refreshNews()
{
    if (m_currentSymbol.isEmpty()) return;

    NewsDataSource::instance()->requestNews(m_currentSymbol, 50);
    NewsDataSource::instance()->requestSocialHeat(m_currentSymbol);
}

void NewsPanelWidget::onNewsUpdated(const QString& symbol, const QVector<NewsItem>& news)
{
    if (symbol != m_currentSymbol) return;

    m_currentNews = news;
    updateNewsList(news);
}

void NewsPanelWidget::onSocialHeatUpdated(const QString& symbol, const SocialHeatData& heat)
{
    if (symbol != m_currentSymbol) return;

    updateSocialHeat(heat);
}

void NewsPanelWidget::onCategoryFilterChanged(int index)
{
    Q_UNUSED(index);

    // 根据分类筛选新闻
    QString category = m_categoryFilter->currentText();
    QVector<NewsItem> filteredNews;

    if (category == QStringLiteral("全部")) {
        filteredNews = m_currentNews;
    } else {
        for (const NewsItem& news : m_currentNews) {
            if (news.category == category) {
                filteredNews.append(news);
            }
        }
    }

    updateNewsList(filteredNews);
}

void NewsPanelWidget::onRefreshClicked()
{
    refreshNews();
}

void NewsPanelWidget::onNewsDoubleClicked(int row, int column)
{
    Q_UNUSED(column);

    if (row >= 0 && row < m_currentNews.size()) {
        emit newsSelected(m_currentNews[row]);
    }
}

void NewsPanelWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(Spacing::MD);
    mainLayout->setContentsMargins(Spacing::MD, Spacing::MD, Spacing::MD, Spacing::MD);

    // 标题栏
    QHBoxLayout* titleLayout = new QHBoxLayout();
    m_titleLabel = new QLabel(QStringLiteral("新闻资讯"));
    m_titleLabel->setObjectName("newsTitle");
    titleLayout->addWidget(m_titleLabel);
    titleLayout->addStretch();

    // 分类筛选
    QLabel* categoryLabel = new QLabel(QStringLiteral("分类:"));
    categoryLabel->setProperty("dataType", "label");
    titleLayout->addWidget(categoryLabel);

    m_categoryFilter = new QComboBox();
    m_categoryFilter->addItem(QStringLiteral("全部"));
    m_categoryFilter->addItem(QStringLiteral("新闻"));
    m_categoryFilter->addItem(QStringLiteral("公告"));
    m_categoryFilter->addItem(QStringLiteral("财报"));
    m_categoryFilter->addItem(QStringLiteral("研报"));
    m_categoryFilter->setFixedWidth(80);
    connect(m_categoryFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &NewsPanelWidget::onCategoryFilterChanged);
    titleLayout->addWidget(m_categoryFilter);

    m_refreshBtn = new QPushButton(QStringLiteral("刷新"));
    m_refreshBtn->setFixedWidth(60);
    m_refreshBtn->setProperty("secondary", true);
    connect(m_refreshBtn, &QPushButton::clicked, this, &NewsPanelWidget::onRefreshClicked);
    titleLayout->addWidget(m_refreshBtn);

    mainLayout->addLayout(titleLayout);

    // 社交热度
    QHBoxLayout* heatLayout = new QHBoxLayout();
    QLabel* heatTitleLabel = new QLabel(QStringLiteral("社交热度:"));
    heatTitleLabel->setProperty("dataType", "label");
    heatLayout->addWidget(heatTitleLabel);

    m_socialHeatLabel = new QLabel(QStringLiteral("--"));
    m_socialHeatLabel->setObjectName("heatLabel");
    heatLayout->addWidget(m_socialHeatLabel);
    heatLayout->addStretch();
    mainLayout->addLayout(heatLayout);

    // 新闻列表
    m_newsTable = new QTableWidget();
    m_newsTable->setObjectName("newsTable");
    m_newsTable->setColumnCount(6);
    m_newsTable->setHorizontalHeaderLabels({
        QStringLiteral("时间"),
        QStringLiteral("分类"),
        QStringLiteral("标题"),
        QStringLiteral("情感"),
        QStringLiteral("影响"),
        QStringLiteral("阅读")
    });
    m_newsTable->horizontalHeader()->setStretchLastSection(true);
    m_newsTable->setColumnWidth(0, 80);
    m_newsTable->setColumnWidth(1, 60);
    m_newsTable->setColumnWidth(3, 60);
    m_newsTable->setColumnWidth(4, 60);
    m_newsTable->setColumnWidth(5, 60);
    m_newsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_newsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_newsTable->setAlternatingRowColors(true);
    m_newsTable->verticalHeader()->setVisible(false);
    connect(m_newsTable, &QTableWidget::cellDoubleClicked,
            this, &NewsPanelWidget::onNewsDoubleClicked);
    mainLayout->addWidget(m_newsTable, 1);

    // 提示信息
    QLabel* tipLabel = new QLabel(QStringLiteral("双击查看新闻详情"));
    tipLabel->setProperty("dataType", "label");
    mainLayout->addWidget(tipLabel);
}

void NewsPanelWidget::updateNewsList(const QVector<NewsItem>& news)
{
    m_newsTable->setRowCount(0);

    for (const NewsItem& item : news) {
        int row = m_newsTable->rowCount();
        m_newsTable->insertRow(row);

        // 时间
        m_newsTable->setItem(row, 0, new QTableWidgetItem(
            item.publishTime.toString(QStringLiteral("MM-dd hh:mm"))));

        // 分类
        m_newsTable->setItem(row, 1, new QTableWidgetItem(item.category));

        // 标题
        QTableWidgetItem* titleItem = new QTableWidgetItem(item.title);
        if (item.isImportant) {
            titleItem->setForeground(QColor(Colors::Primary));
            titleItem->setFont(QFont(titleItem->font().family(), -1, QFont::Bold));
        }
        m_newsTable->setItem(row, 2, titleItem);

        // 情感
        QString sentimentText = sentimentToText(item.sentiment.sentiment);
        QTableWidgetItem* sentimentItem = new QTableWidgetItem(sentimentText);
        sentimentItem->setForeground(QColor(sentimentToColor(item.sentiment.sentiment)));
        m_newsTable->setItem(row, 3, sentimentItem);

        // 影响力
        QString impactText;
        if (item.sentiment.impactScore > 5.0) {
            impactText = QStringLiteral("高");
        } else if (item.sentiment.impactScore > 2.0) {
            impactText = QStringLiteral("中");
        } else if (item.sentiment.impactScore > -2.0) {
            impactText = QStringLiteral("低");
        } else {
            impactText = QStringLiteral("高");
        }
        m_newsTable->setItem(row, 4, new QTableWidgetItem(impactText));

        // 阅读量
        QString readText;
        if (item.readCount >= 10000) {
            readText = QString(QStringLiteral("%1万"))
                .arg(item.readCount / 10000.0, 0, 'f', 1);
        } else {
            readText = QString::number(item.readCount);
        }
        m_newsTable->setItem(row, 5, new QTableWidgetItem(readText));
    }
}

void NewsPanelWidget::updateSocialHeat(const SocialHeatData& heat)
{
    QString text = QString(QStringLiteral("%1提及 (较昨日%2%3%)"))
        .arg(heat.mentionCount)
        .arg(heat.changePercent >= 0 ? QStringLiteral("+") : QString())
        .arg(heat.changePercent, 0, 'f', 1);

    m_socialHeatLabel->setText(text);

    // 使用属性选择器设置热度状态
    QString status;
    if (heat.changePercent > 20) {
        status = "hot";
    } else if (heat.changePercent > 0) {
        status = "warm";
    } else if (heat.changePercent < -20) {
        status = "cold";
    } else {
        status = "normal";
    }
    m_socialHeatLabel->setProperty("heatStatus", status);
    m_socialHeatLabel->style()->unpolish(m_socialHeatLabel);
    m_socialHeatLabel->style()->polish(m_socialHeatLabel);
}

QString NewsPanelWidget::sentimentToColor(SentimentType sentiment) const
{
    switch (sentiment) {
    case SentimentType::Positive: return Colors::Success;
    case SentimentType::Negative: return Colors::Danger;
    case SentimentType::Neutral: return Colors::TextSecondary;
    default: return Colors::TextSecondary;
    }
}

QString NewsPanelWidget::sentimentToText(SentimentType sentiment) const
{
    switch (sentiment) {
    case SentimentType::Positive: return QStringLiteral("正面");
    case SentimentType::Negative: return QStringLiteral("负面");
    case SentimentType::Neutral: return QStringLiteral("中性");
    default: return QStringLiteral("未知");
    }
}