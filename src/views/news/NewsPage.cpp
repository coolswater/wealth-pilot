/**
 * @file NewsPage.cpp
 * @brief 新闻资讯页面实现 - 垂直滚动卡片列表设计
 */

#include "NewsPage.h"
#include "core/config/Tokens.h"
#include "market/NewsDataSource.h"
#include "ui/styles/ButtonStyles.h"
#include "utils/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QFrame>
#include <QDialog>
#include <QDialogButtonBox>
#include <QTextBrowser>
#include <QMouseEvent>
#include <QGraphicsDropShadowEffect>
#include <QDateTime>
#include <QRegularExpression>

// ============================================================================
// 颜色常量 - 使用 Tokens 主题令牌
// ============================================================================
namespace {
    // 使用 Tokens 中定义的颜色
    const QString COLOR_BG_GLOBAL = Tokens::Colors::BgBase;        // 全局背景
    const QString COLOR_BG_CARD = Tokens::Colors::BgElevated;          // 卡片背景
    const QString COLOR_TEXT_TITLE = Tokens::Colors::TextPrimary;       // 标题文字
    const QString COLOR_TEXT_META = Tokens::Colors::TextSecondary;        // 元数据文字
    const QString COLOR_TEXT_SUMMARY = Tokens::Colors::TextPrimary;     // 摘要文字
    const QString COLOR_HIGHLIGHT = Tokens::Colors::Warning;        // 强调色（数字）
    const QString COLOR_CATEGORY_ACTIVE = Tokens::Colors::Primary;  // 分类标签高亮
    const QString COLOR_SEPARATOR = Tokens::Colors::Border;      // 分隔线
    const QString COLOR_HOVER_BG = Tokens::Colors::BgHover; // 悬停背景
}

// ============================================================================
// NewsPage::Impl
// ============================================================================

class NewsPage::Impl {
public:
    QWidget* container = nullptr;
    QVBoxLayout* containerLayout = nullptr;
    QScrollArea* scrollArea = nullptr;
    QWidget* scrollContent = nullptr;
    QVBoxLayout* cardsLayout = nullptr;
    
    QVector<NewsCardData> allNews;
    QVector<NewsCardWidget*> cards;
    
    QString currentCategory = QStringLiteral("全部");
    NewsDataSource* dataSource = nullptr;
};

// ============================================================================
// NewsCardWidget 实现
// ============================================================================

NewsCardWidget::NewsCardWidget(const NewsCardData& data, QWidget* parent)
    : QFrame(parent)
    , m_data(data)
{
    setupUI();
}

void NewsCardWidget::setLastCard(bool isLast)
{
    if (m_separator) {
        m_separator->setVisible(!isLast);
    }
}

void NewsCardWidget::setupUI()
{
    setStyleSheet(QString(R"(
        NewsCardWidget {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 8px;
        }
    )").arg(COLOR_BG_CARD, COLOR_SEPARATOR));
    
    // 添加阴影效果
    auto* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(8);
    shadow->setColor(QColor(0, 0, 0, 30));
    shadow->setOffset(0, 2);
    setGraphicsEffect(shadow);
    
    setMinimumHeight(140);
    setCursor(Qt::PointingHandCursor);
    
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 12);
    mainLayout->setSpacing(10);
    
    // 标题行
    auto* titleRow = new QHBoxLayout();
    titleRow->setSpacing(8);
    
    m_titleLabel = new QLabel(m_data.title);
    m_titleLabel->setStyleSheet(QString(
        "QLabel { color: %1; font-size: 15px; font-weight: bold; }"
    ).arg(COLOR_TEXT_TITLE));
    m_titleLabel->setWordWrap(true);
    titleRow->addWidget(m_titleLabel, 1);
    
    m_timeLabel = new QLabel(formatTime(m_data.publishTime));
    m_timeLabel->setStyleSheet(QString(
        "QLabel { color: %1; font-size: 12px; }"
    ).arg(COLOR_TEXT_META));
    m_timeLabel->setAlignment(Qt::AlignTop | Qt::AlignRight);
    titleRow->addWidget(m_timeLabel);
    
    mainLayout->addLayout(titleRow);
    
    // 元数据行
    auto* metaRow = new QHBoxLayout();
    metaRow->setSpacing(8);
    
    QString sourceText = m_data.source;
    if (!m_data.category.isEmpty()) {
        sourceText += QString(" · %1").arg(m_data.category);
    }
    m_sourceLabel = new QLabel(sourceText);
    m_sourceLabel->setStyleSheet(QString(
        "QLabel { color: %1; font-size: 12px; }"
    ).arg(COLOR_TEXT_META));
    metaRow->addWidget(m_sourceLabel, 1);
    
    m_detailBtn = new QPushButton(QStringLiteral("详情"));
    ButtonStyles::setInfo(m_detailBtn);
    m_detailBtn->setCursor(Qt::PointingHandCursor);
    connect(m_detailBtn, &QPushButton::clicked, this, [this]() {
        emit detailRequested(m_data);
    });
    metaRow->addWidget(m_detailBtn);
    
    mainLayout->addLayout(metaRow);
    
    // 摘要
    m_summaryLabel = new QLabel();
    QString summaryText = highlightNumbersInText(m_data.summary, m_data.highlightNumbers);
    m_summaryLabel->setTextFormat(Qt::RichText);
    m_summaryLabel->setText(summaryText);
    m_summaryLabel->setStyleSheet(QString(
        "QLabel { color: %1; font-size: 13px; line-height: 1.5; }"
    ).arg(COLOR_TEXT_SUMMARY));
    m_summaryLabel->setWordWrap(true);
    m_summaryLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    mainLayout->addWidget(m_summaryLabel);
    
    // 分隔线
    m_separator = new QFrame();
    m_separator->setFrameShape(QFrame::HLine);
    m_separator->setStyleSheet(QString(
        "QFrame { background-color: %1; border: none; max-height: 1px; }"
    ).arg(COLOR_SEPARATOR));
    mainLayout->addWidget(m_separator);
}

QString NewsCardWidget::formatTime(const QDateTime& time) const
{
    qint64 secs = time.secsTo(QDateTime::currentDateTime());
    
    if (secs < 60) {
        return QStringLiteral("刚刚");
    } else if (secs < 3600) {
        return QString(QStringLiteral("%1分钟前")).arg(secs / 60);
    } else if (secs < 86400) {
        return QString(QStringLiteral("%1小时前")).arg(secs / 3600);
    } else {
        return time.toString("MM-dd HH:mm");
    }
}

QString NewsCardWidget::highlightNumbersInText(const QString& text, const QStringList& numbers) const
{
    QString result = text;
    for (const auto& num : numbers) {
        // 使用富文本标记高亮数字
        result.replace(num, QString("<span style='color: %1; font-weight: bold;'>%2</span>")
            .arg(COLOR_HIGHLIGHT, num));
    }
    return result;
}

void NewsCardWidget::mousePressEvent(QMouseEvent* event)
{
    QFrame::mousePressEvent(event);
    if (event->button() == Qt::LeftButton) {
        emit clicked();
    }
}

void NewsCardWidget::enterEvent(QEnterEvent* event)
{
    QFrame::enterEvent(event);
    setStyleSheet(QString(R"(
        NewsCardWidget {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 8px;
        }
    )").arg(COLOR_BG_CARD, COLOR_CATEGORY_ACTIVE));
}

void NewsCardWidget::leaveEvent(QEvent* event)
{
    QFrame::leaveEvent(event);
    setStyleSheet(QString(R"(
        NewsCardWidget {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 8px;
        }
    )").arg(COLOR_BG_CARD, COLOR_SEPARATOR));
}

// ============================================================================
// NewsPage 实现
// ============================================================================

NewsPage::NewsPage(QWidget* parent)
    : BasePage(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
    loadDemoData();
}

NewsPage::~NewsPage()
{
}

QString NewsPage::pageId() const
{
    return QStringLiteral("NewsPage");
}

void NewsPage::initializePage()
{
    if (isInitialized()) return;
    
    // 初始化数据源
    d->dataSource = NewsDataSource::instance();
    connect(d->dataSource, &NewsDataSource::newsUpdated,
            this, &NewsPage::onNewsReceived);
    
    setInitialized(true);
    LOG_INFO("NewsPage initialized");
}

void NewsPage::setupUI()
{
    // 全局背景
    setStyleSheet(QString("QWidget { background-color: %1; }").arg(COLOR_BG_GLOBAL));
    
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // 分类栏
    setupCategoryBar();
    
    // 滚动区域
    setupScrollArea();
}

void NewsPage::setupCategoryBar()
{
    auto* categoryBar = new QFrame();
    categoryBar->setStyleSheet(QString(
        "QFrame { background-color: %1; border-bottom: 1px solid %2; }"
    ).arg(COLOR_BG_CARD, COLOR_SEPARATOR));
    categoryBar->setFixedHeight(48);
    
    auto* barLayout = new QHBoxLayout(categoryBar);
    barLayout->setContentsMargins(16, 8, 16, 8);
    barLayout->setSpacing(8);
    
    QStringList categories = {
        QStringLiteral("全部"),
        QStringLiteral("新闻"),
        QStringLiteral("能源"),
        QStringLiteral("金融")
    };
    
    for (const auto& cat : categories) {
        auto* btn = new QPushButton(cat);
        btn->setCheckable(true);
        btn->setChecked(cat == d->currentCategory);
        btn->setFixedHeight(32);
        btn->setCursor(Qt::PointingHandCursor);
        
        QString activeStyle = QString(R"(
            QPushButton {
                background-color: %1;
                color: white;
                border: none;
                border-radius: 4px;
                padding: 4px 16px;
                font-size: 13px;
            }
        )").arg(COLOR_CATEGORY_ACTIVE);
        
        QString normalStyle = QString(R"(
            QPushButton {
                background-color: transparent;
                color: %1;
                border: 1px solid %2;
                border-radius: 4px;
                padding: 4px 16px;
                font-size: 13px;
            }
            QPushButton:hover {
                background-color: %3;
                border-color: %4;
            }
        )").arg(COLOR_TEXT_META, COLOR_SEPARATOR, COLOR_HOVER_BG, COLOR_CATEGORY_ACTIVE);
        
        btn->setStyleSheet(btn->isChecked() ? activeStyle : normalStyle);
        
        connect(btn, &QPushButton::clicked, this, [this, btn, cat, activeStyle, normalStyle]() {
            // 更新按钮样式
            auto* bar = qobject_cast<QFrame*>(btn->parent());
            if (bar) {
                for (auto* child : bar->findChildren<QPushButton*>()) {
                    bool isActive = child == btn;
                    child->setChecked(isActive);
                    child->setStyleSheet(isActive ? activeStyle : normalStyle);
                }
            }
            
            onCategoryClicked(cat);
        });
        
        barLayout->addWidget(btn);
    }
    
    barLayout->addStretch();
    
    // 添加到主布局
    auto* mainLayout = qobject_cast<QVBoxLayout*>(QWidget::layout());
    if (mainLayout) {
        mainLayout->insertWidget(0, categoryBar);
    }
}

void NewsPage::setupScrollArea()
{
    d->scrollArea = new QScrollArea();
    d->scrollArea->setWidgetResizable(true);
    d->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    d->scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    d->scrollArea->setStyleSheet(QString(
        "QScrollArea { background-color: %1; border: none; }"
        "QScrollBar:vertical { width: 8px; background-color: transparent; }"
        "QScrollBar::handle:vertical { background-color: %2; border-radius: 4px; min-height: 40px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
    ).arg(COLOR_BG_GLOBAL, Tokens::Colors::Border));
    
    d->scrollContent = new QWidget();
    d->scrollContent->setStyleSheet(QString("QWidget { background-color: %1; }").arg(COLOR_BG_GLOBAL));
    
    d->cardsLayout = new QVBoxLayout(d->scrollContent);
    d->cardsLayout->setContentsMargins(16, 16, 16, 16);
    d->cardsLayout->setSpacing(12);
    d->cardsLayout->addStretch();
    
    d->scrollArea->setWidget(d->scrollContent);
    
    // 添加到主布局
    auto* mainLayout = qobject_cast<QVBoxLayout*>(QWidget::layout());
    if (mainLayout) {
        mainLayout->addWidget(d->scrollArea);
    }
}

void NewsPage::loadDemoData()
{
    QDateTime now = QDateTime::currentDateTime();
    
    d->allNews = {
        {
            QStringLiteral("n1"),
            QStringLiteral("伊朗外交部：将采取一切必要措施回应以色列袭击"),
            QStringLiteral("伊朗外交部发言人表示，伊朗将采取一切必要措施回应以色列对伊朗驻叙利亚领事馆的袭击。伊朗外交部发言人卡纳尼在新闻发布会上表示，伊朗保留采取对等回应的权利，并呼吁国际社会谴责以色列的侵略行为。"),
            QStringLiteral("新华社"),
            QStringLiteral("新闻"),
            now.addSecs(-7200),
            {},
            QStringLiteral("伊朗外交部发言人卡纳尼在新闻发布会上表示，伊朗保留采取对等回应的权利，并呼吁国际社会谴责以色列的侵略行为。伊朗将采取一切必要措施回应以色列对伊朗驻叙利亚领事馆的袭击。")
        },
        {
            QStringLiteral("n2"),
            QStringLiteral("俄罗斯天然气工业股份公司：2024年对华天然气出口增长20.09%"),
            QStringLiteral("俄罗斯天然气工业股份公司（Gazprom）发布声明称，2024年对华天然气出口量达到1.93亿立方米，同比增长20.09%。公司表示，将继续扩大与中国在能源领域的合作，预计2025年出口量将进一步增长。"),
            QStringLiteral("路透社"),
            QStringLiteral("能源"),
            now.addSecs(-14400),
            {QStringLiteral("20.09%"), QStringLiteral("1.93亿立方米")},
            QStringLiteral("俄罗斯天然气工业股份公司（Gazprom）发布声明称，2024年对华天然气出口量达到1.93亿立方米，同比增长20.09%。公司表示，将继续扩大与中国在能源领域的合作，预计2025年出口量将进一步增长。双方正在讨论新的天然气管道项目。")
        },
        {
            QStringLiteral("n3"),
            QStringLiteral("金力永磁：一季度净利润同比增长35.2%，新能源汽车业务表现亮眼"),
            QStringLiteral("金力永磁(300748)发布2026年一季报，公司实现营业收入15.68亿元，同比增长28.3%；净利润2.15亿元，同比增长35.2%。新能源汽车领域收入占比提升至45%，成为公司最大收入来源。"),
            QStringLiteral("证券时报"),
            QStringLiteral("金融"),
            now.addSecs(-28800),
            {QStringLiteral("35.2%"), QStringLiteral("15.68亿元"), QStringLiteral("2.15亿元"), QStringLiteral("28.3%"), QStringLiteral("45%")},
            QStringLiteral("金力永磁(300748)发布2026年一季报，公司实现营业收入15.68亿元，同比增长28.3%；净利润2.15亿元，同比增长35.2%。新能源汽车领域收入占比提升至45%，成为公司最大收入来源。公司表示，将继续加大在新能源汽车、风电等领域的研发投入。")
        },
        {
            QStringLiteral("n4"),
            QStringLiteral("中际旭创：2025年度权益分派实施公告，每10股派发现金红利8.5元"),
            QStringLiteral("中际旭创(300308)发布2025年度权益分派实施公告，公司2025年度权益分派方案为：以公司现有总股本8.02亿股为基数，向全体股东每10股派发现金红利8.5元（含税），合计派发现金红利6.82亿元。股权登记日为2026年4月25日，除权除息日为2026年4月26日。"),
            QStringLiteral("深交所"),
            QStringLiteral("金融"),
            now.addSecs(-43200),
            {QStringLiteral("8.5元"), QStringLiteral("8.02亿股"), QStringLiteral("6.82亿元")},
            QStringLiteral("中际旭创(300308)发布2025年度权益分派实施公告，公司2025年度权益分派方案为：以公司现有总股本8.02亿股为基数，向全体股东每10股派发现金红利8.5元（含税），合计派发现金红利6.82亿元。股权登记日为2026年4月25日，除权除息日为2026年4月26日。本次权益分派对象为截至2026年4月25日下午深圳证券交易所收市后，在中国证券登记结算有限责任公司深圳分公司登记在册的公司全体股东。")
        }
    };
    
    updateCards();
}

void NewsPage::updateCards(const QString& filter)
{
    // 清除现有卡片
    for (auto* card : d->cards) {
        d->cardsLayout->removeWidget(card);
        card->deleteLater();
    }
    d->cards.clear();
    
    // 添加新卡片
    int index = 0;
    for (const auto& news : d->allNews) {
        // 应用分类过滤
        if (!filter.isEmpty() && filter != QStringLiteral("全部")) {
            if (news.category != filter) {
                continue;
            }
        }
        
        auto* card = new NewsCardWidget(news);
        connect(card, &NewsCardWidget::clicked, this, &NewsPage::onCardClicked);
        connect(card, &NewsCardWidget::detailRequested, this, &NewsPage::onDetailRequested);
        
        // 插入到 stretch 之前
        d->cardsLayout->insertWidget(d->cardsLayout->count() - 1, card);
        d->cards.append(card);
        
        ++index;
    }
    
    // 设置最后一张卡片不显示分隔线
    if (!d->cards.isEmpty()) {
        d->cards.last()->setLastCard(true);
    }
}

void NewsPage::onCategoryClicked(const QString& category)
{
    d->currentCategory = category;
    updateCards(category);
}

void NewsPage::onCardClicked()
{
    auto* card = qobject_cast<NewsCardWidget*>(sender());
    if (card) {
        // 可以在这里添加点击效果
    }
}

void NewsPage::onDetailRequested(const NewsCardData& data)
{
    showDetailDialog(data);
}

void NewsPage::onNewsReceived(const QString& symbol, const QVector<NewsItem>& news)
{
    d->allNews.clear();
    
    for (const auto& item : news) {
        NewsCardData data;
        data.id = item.id;
        data.title = item.title;
        data.summary = item.content.left(200);
        data.fullContent = item.content;
        data.source = item.source;
        data.publishTime = item.publishTime;
        d->allNews.append(data);
    }
    
    updateCards(d->currentCategory);
}

void NewsPage::showDetailDialog(const NewsCardData& data)
{
    auto* dialog = new QDialog(this);
    dialog->setWindowTitle(QStringLiteral("新闻详情"));
    dialog->setMinimumSize(500, 400);
    dialog->setStyleSheet(QString(R"(
        QDialog {
            background-color: %1;
        }
        QLabel {
            color: %2;
        }
    )").arg(COLOR_BG_CARD, COLOR_TEXT_TITLE));
    
    auto* layout = new QVBoxLayout(dialog);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(16);
    
    // 标题
    auto* titleLabel = new QLabel(data.title);
    titleLabel->setStyleSheet(QString(
        "QLabel { font-size: 16px; font-weight: bold; color: %1; }"
    ).arg(COLOR_TEXT_TITLE));
    titleLabel->setWordWrap(true);
    layout->addWidget(titleLabel);
    
    // 元数据
    QString metaText = QString("%1 · %2 · %3")
        .arg(data.source, data.category, data.publishTime.toString("yyyy-MM-dd HH:mm"));
    auto* metaLabel = new QLabel(metaText);
    metaLabel->setStyleSheet(QString(
        "QLabel { font-size: 12px; color: %1; }"
    ).arg(COLOR_TEXT_META));
    layout->addWidget(metaLabel);
    
    // 内容
    auto* contentBrowser = new QTextBrowser();
    contentBrowser->setStyleSheet(QString(R"(
        QTextBrowser {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 4px;
            padding: 12px;
            font-size: 13px;
            color: %3;
        }
    )").arg(COLOR_BG_GLOBAL, COLOR_SEPARATOR, COLOR_TEXT_SUMMARY));
    contentBrowser->setPlainText(data.fullContent.isEmpty() ? data.summary : data.fullContent);
    contentBrowser->setOpenExternalLinks(true);
    layout->addWidget(contentBrowser);
    
    // 按钮
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    buttonBox->setStyleSheet(QString(
        "QPushButton { padding: 8px 24px; border-radius: 4px; background-color: %1; color: white; border: none; }"
        "QPushButton:hover { background-color: %2; }"
    ).arg(COLOR_CATEGORY_ACTIVE, Tokens::Colors::PrimaryHover));
    connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::close);
    layout->addWidget(buttonBox);
    
    dialog->exec();
    dialog->deleteLater();
}
