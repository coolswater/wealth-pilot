/**
 * @file SignalCenterPage.cpp
 * @brief 信号中心页面实现 - 参考资讯页面样式优化
 */

#include "SignalCenterPage.h"
#include "core/config/Tokens.h"
#include "utils/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QMouseEvent>

using namespace Tokens;

// ============================================================================
// 颜色常量
// ============================================================================
namespace {
    const QString COLOR_BG_GLOBAL = Colors::BgBase;
    const QString COLOR_BG_CARD = Colors::BgElevated;
    const QString COLOR_TEXT_TITLE = Colors::TextPrimary;
    const QString COLOR_TEXT_META = Colors::TextSecondary;
    const QString COLOR_TEXT_VALUE = Colors::TextPrimary;
    const QString COLOR_SUCCESS = Colors::Success;
    const QString COLOR_DANGER = Colors::Danger;
    const QString COLOR_PRIMARY = Colors::Primary;
    const QString COLOR_SEPARATOR = Colors::Border;
    const QString COLOR_HOVER_BG = Colors::BgHover;
}

// ============================================================================
// SignalCardWidget - 信号卡片组件
// ============================================================================
class SignalCardWidget : public QFrame
{
    Q_OBJECT

public:
    struct Data {
        QString id;
        QString name;
        double returnRate = 0.0;
        int winRate = 0;
        int followers = 0;
        double price = 0.0;
        QString strategy;
        QString description;
    };

    explicit SignalCardWidget(const Data& data, QWidget* parent = nullptr);
    
    void setLastCard(bool isLast);

signals:
    void clicked();
    void subscribeClicked(const Data& data);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    void setupUI();

    Data m_data;
    QLabel* m_nameLabel = nullptr;
    QLabel* m_returnLabel = nullptr;
    QLabel* m_winRateLabel = nullptr;
    QLabel* m_followersLabel = nullptr;
    QPushButton* m_subscribeBtn = nullptr;
    QFrame* m_separator = nullptr;
};

SignalCardWidget::SignalCardWidget(const Data& data, QWidget* parent)
    : QFrame(parent)
    , m_data(data)
{
    setupUI();
}

void SignalCardWidget::setLastCard(bool isLast)
{
    if (m_separator) {
        m_separator->setVisible(!isLast);
    }
}

void SignalCardWidget::setupUI()
{
    setStyleSheet(QString(R"(
        SignalCardWidget {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 8px;
        }
    )").arg(COLOR_BG_CARD, COLOR_SEPARATOR));
    
    auto* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(8);
    shadow->setColor(QColor(0, 0, 0, 30));
    shadow->setOffset(0, 2);
    setGraphicsEffect(shadow);
    
    setMinimumHeight(160);
    setCursor(Qt::PointingHandCursor);
    
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 12);
    mainLayout->setSpacing(10);
    
    // 标题行
    auto* titleRow = new QHBoxLayout();
    titleRow->setSpacing(8);
    
    m_nameLabel = new QLabel(m_data.name);
    m_nameLabel->setStyleSheet(QString(
        "QLabel { color: %1; font-size: 15px; font-weight: bold; }"
    ).arg(COLOR_TEXT_TITLE));
    titleRow->addWidget(m_nameLabel, 1);
    
    if (!m_data.strategy.isEmpty()) {
        QLabel* strategyLabel = new QLabel(m_data.strategy);
        strategyLabel->setStyleSheet(QString(
            "QLabel { color: %1; font-size: 11px; background: %2; padding: 2px 8px; border-radius: 3px; }"
        ).arg(COLOR_PRIMARY, Colors::PrimaryLight));
        titleRow->addWidget(strategyLabel);
    }
    
    mainLayout->addLayout(titleRow);
    
    // 收益率行
    auto* returnRow = new QHBoxLayout();
    returnRow->setSpacing(8);
    
    QLabel* returnTitle = new QLabel(QStringLiteral("累计收益"));
    returnTitle->setStyleSheet(QString("QLabel { color: %1; font-size: 12px; }").arg(COLOR_TEXT_META));
    returnRow->addWidget(returnTitle);
    returnRow->addStretch();
    
    QString returnText = m_data.returnRate >= 0 
        ? QString("+%1%").arg(m_data.returnRate, 0, 'f', 1)
        : QString("%1%").arg(m_data.returnRate, 0, 'f', 1);
    m_returnLabel = new QLabel(returnText);
    m_returnLabel->setStyleSheet(QString(
        "QLabel { color: %1; font-size: 22px; font-weight: bold; font-family: 'Consolas', monospace; }"
    ).arg(m_data.returnRate >= 0 ? COLOR_SUCCESS : COLOR_DANGER));
    returnRow->addWidget(m_returnLabel);
    mainLayout->addLayout(returnRow);
    
    // 胜率和订阅人数行
    auto* statsRow = new QHBoxLayout();
    statsRow->setSpacing(16);
    
    QLabel* winRateTitle = new QLabel(QStringLiteral("胜率"));
    winRateTitle->setStyleSheet(QString("QLabel { color: %1; font-size: 12px; }").arg(COLOR_TEXT_META));
    statsRow->addWidget(winRateTitle);
    
    m_winRateLabel = new QLabel(QString("%1%").arg(m_data.winRate));
    m_winRateLabel->setStyleSheet(QString(
        "QLabel { color: %1; font-size: 13px; font-weight: 600; }"
    ).arg(COLOR_TEXT_VALUE));
    statsRow->addWidget(m_winRateLabel);
    
    statsRow->addSpacing(16);
    
    QLabel* followersTitle = new QLabel(QStringLiteral("订阅"));
    followersTitle->setStyleSheet(QString("QLabel { color: %1; font-size: 12px; }").arg(COLOR_TEXT_META));
    statsRow->addWidget(followersTitle);
    
    m_followersLabel = new QLabel(QString::number(m_data.followers));
    m_followersLabel->setStyleSheet(QString(
        "QLabel { color: %1; font-size: 13px; }"
    ).arg(COLOR_TEXT_VALUE));
    statsRow->addWidget(m_followersLabel);
    
    statsRow->addStretch();
    
    m_subscribeBtn = new QPushButton(QStringLiteral("订阅"));
    m_subscribeBtn->setStyleSheet(QString(R"(
        QPushButton {
            background-color: %1;
            color: white;
            border: none;
            border-radius: 4px;
            padding: 4px 12px;
            font-size: 12px;
            font-weight: 600;
        }
        QPushButton:hover {
            background-color: %2;
        }
    )").arg(COLOR_PRIMARY, Colors::PrimaryHover));
    m_subscribeBtn->setCursor(Qt::PointingHandCursor);
    connect(m_subscribeBtn, &QPushButton::clicked, this, [this]() {
        emit subscribeClicked(m_data);
    });
    statsRow->addWidget(m_subscribeBtn);
    
    mainLayout->addLayout(statsRow);
    
    m_separator = new QFrame();
    m_separator->setFrameShape(QFrame::HLine);
    m_separator->setStyleSheet(QString(
        "QFrame { background-color: %1; border: none; max-height: 1px; }"
    ).arg(COLOR_SEPARATOR));
    mainLayout->addWidget(m_separator);
}

void SignalCardWidget::mousePressEvent(QMouseEvent* event)
{
    QFrame::mousePressEvent(event);
    if (event->button() == Qt::LeftButton) {
        emit clicked();
    }
}

void SignalCardWidget::enterEvent(QEnterEvent* event)
{
    QFrame::enterEvent(event);
    setStyleSheet(QString(R"(
        SignalCardWidget {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 8px;
        }
    )").arg(COLOR_BG_CARD, COLOR_PRIMARY));
}

void SignalCardWidget::leaveEvent(QEvent* event)
{
    QFrame::leaveEvent(event);
    setStyleSheet(QString(R"(
        SignalCardWidget {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 8px;
        }
    )").arg(COLOR_BG_CARD, COLOR_SEPARATOR));
}

// ============================================================================
// SignalCenterPage 实现
// ============================================================================
namespace WealthPilot {

struct SignalCenterPage::Impl {
    QScrollArea* scrollArea = nullptr;
    QWidget* scrollContent = nullptr;
    QVBoxLayout* cardsLayout = nullptr;
    
    QVector<SignalCardWidget::Data> allSignals;
    QVector<SignalCardWidget*> cards;
    
    QString currentCategory = QStringLiteral("我的订阅");
};

SignalCenterPage::SignalCenterPage(QWidget *parent)
    : BasePage(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
    loadDemoData();
}

SignalCenterPage::~SignalCenterPage() = default;

QString SignalCenterPage::pageId() const
{
    return QStringLiteral("SignalCenterPage");
}

void SignalCenterPage::initializePage()
{
    LOG_INFO("SignalCenterPage initialized");
}

void SignalCenterPage::setupUI()
{
    setStyleSheet(QString("QWidget { background-color: %1; }").arg(COLOR_BG_GLOBAL));
    
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    setupCategoryBar();
    setupScrollArea();
}

void SignalCenterPage::setupCategoryBar()
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
        QStringLiteral("我的订阅"),
        QStringLiteral("推荐信号"),
        QStringLiteral("排行榜"),
        QStringLiteral("最新上线")
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
        )").arg(COLOR_PRIMARY);
        
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
        )").arg(COLOR_TEXT_META, COLOR_SEPARATOR, COLOR_HOVER_BG, COLOR_PRIMARY);
        
        btn->setStyleSheet(btn->isChecked() ? activeStyle : normalStyle);
        
        connect(btn, &QPushButton::clicked, this, [this, btn, cat, activeStyle, normalStyle]() {
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
    
    QPushButton* refreshBtn = new QPushButton(QStringLiteral("刷新"));
    refreshBtn->setFixedHeight(32);
    refreshBtn->setFixedWidth(60);
    refreshBtn->setCursor(Qt::PointingHandCursor);
    refreshBtn->setStyleSheet(QString(R"(
        QPushButton {
            background-color: transparent;
            color: %1;
            border: 1px solid %2;
            border-radius: 4px;
            font-size: 12px;
        }
        QPushButton:hover {
            background-color: %3;
            border-color: %4;
        }
    )").arg(COLOR_TEXT_META, COLOR_SEPARATOR, COLOR_HOVER_BG, COLOR_PRIMARY));
    connect(refreshBtn, &QPushButton::clicked, this, [this]() {
        loadDemoData();
        LOG_INFO("Signal data refreshed");
    });
    barLayout->addWidget(refreshBtn);
    
    auto* mainLayout = qobject_cast<QVBoxLayout*>(QWidget::layout());
    if (mainLayout) {
        mainLayout->insertWidget(0, categoryBar);
    }
}

void SignalCenterPage::setupScrollArea()
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
    
    auto* mainLayout = qobject_cast<QVBoxLayout*>(QWidget::layout());
    if (mainLayout) {
        mainLayout->addWidget(d->scrollArea);
    }
}

void SignalCenterPage::loadDemoData()
{
    d->allSignals = {
        {QStringLiteral("s1"), QStringLiteral("量化策略A"), 156.8, 78, 2340, 99.0, QStringLiteral("量化"), QStringLiteral("多因子选股策略")},
        {QStringLiteral("s2"), QStringLiteral("趋势跟踪B"), 89.3, 65, 1890, 79.0, QStringLiteral("趋势"), QStringLiteral("动量突破策略")},
        {QStringLiteral("s3"), QStringLiteral("价值投资C"), 45.2, 82, 3200, 59.0, QStringLiteral("价值"), QStringLiteral("低估值选股策略")},
        {QStringLiteral("s4"), QStringLiteral("短线精灵D"), 234.5, 71, 1560, 129.0, QStringLiteral("短线"), QStringLiteral("日内高频交易")},
        {QStringLiteral("s5"), QStringLiteral("均值回归E"), 67.5, 69, 2100, 89.0, QStringLiteral("量化"), QStringLiteral("统计套利策略")},
        {QStringLiteral("s6"), QStringLiteral("动量策略F"), 123.4, 75, 1800, 109.0, QStringLiteral("趋势"), QStringLiteral("行业轮动策略")},
        {QStringLiteral("s7"), QStringLiteral("套利策略G"), 89.0, 80, 2900, 69.0, QStringLiteral("套利"), QStringLiteral("期现套利策略")},
        {QStringLiteral("s8"), QStringLiteral("成长精选H"), 178.6, 76, 2450, 119.0, QStringLiteral("成长"), QStringLiteral("高成长选股策略")}
    };
    
    updateCards();
}

void SignalCenterPage::updateCards(const QString& filter)
{
    for (auto* card : d->cards) {
        d->cardsLayout->removeWidget(card);
        card->deleteLater();
    }
    d->cards.clear();
    
    for (const auto& signal : d->allSignals) {
        Q_UNUSED(filter);
        
        auto* card = new SignalCardWidget(signal);
        connect(card, &SignalCardWidget::clicked, this, &SignalCenterPage::onCardClicked);
        connect(card, &SignalCardWidget::subscribeClicked, this, [this](const SignalCardWidget::Data& data) {
            SignalCardData cardData;
            cardData.id = data.id;
            cardData.name = data.name;
            cardData.returnRate = data.returnRate;
            cardData.winRate = data.winRate;
            cardData.followers = data.followers;
            cardData.price = data.price;
            cardData.strategy = data.strategy;
            cardData.description = data.description;
            onSubscribeClicked(cardData);
        });
        
        d->cardsLayout->insertWidget(d->cardsLayout->count() - 1, card);
        d->cards.append(card);
    }
    
    if (!d->cards.isEmpty()) {
        d->cards.last()->setLastCard(true);
    }
}

void SignalCenterPage::onCategoryClicked(const QString& category)
{
    d->currentCategory = category;
    updateCards(category);
    LOG_INFO(QString("Category changed to: %1").arg(category));
}

void SignalCenterPage::onCardClicked()
{
    auto* card = qobject_cast<SignalCardWidget*>(sender());
    if (card) {
        LOG_INFO("Signal card clicked");
    }
}

void SignalCenterPage::onSubscribeClicked(const SignalCardData& data)
{
    LOG_INFO(QString("Subscribe clicked: %1").arg(data.name));
}

} // namespace WealthPilot

#include "SignalCenterPage.moc"
