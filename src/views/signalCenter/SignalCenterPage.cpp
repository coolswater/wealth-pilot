#include "SignalCenterPage.h"
#include "core/config/Tokens.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QGridLayout>
#include <QButtonGroup>
#include <QResizeEvent>
#include <QScrollArea>
#include <QFrame>

#include <ui/components/CardWidget.h>

#include <QTimer>

struct SignalCenterPage::Impl {
    QVBoxLayout* mainLayout = nullptr;
    QHBoxLayout* headerLayout = nullptr;
    QButtonGroup* buttonGroup = nullptr;
    QWidget* gridContainer = nullptr;      // 网格容器
    QGridLayout* gridLayout = nullptr;
    QList<CardWidget*> cards;              // 保存卡片引用
    int currentColumns = 5;                // 当前列数

    // 布局参数
    static constexpr int CARD_WIDTH = 280;     // 卡片固定宽度
    static constexpr int CARD_MAX_HEIGHT = 180;
    static constexpr int SPACING = 12;         // 卡片间距
    static constexpr int MARGIN = 16;          // 边距
};

SignalCenterPage::SignalCenterPage(QWidget *parent)
    : BasePage(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
}

SignalCenterPage::~SignalCenterPage() = default;

QString SignalCenterPage::pageId() const
{
    return QStringLiteral("SignalCenterPage");
}

void SignalCenterPage::initializePage()
{
}

void SignalCenterPage::setupUI()
{
    d->mainLayout = new QVBoxLayout(this);
    d->mainLayout->setContentsMargins(Impl::MARGIN, Impl::MARGIN, Impl::MARGIN, Impl::MARGIN);
    d->mainLayout->setSpacing(16);

    // 头部区域
    QFrame* headerFrame = new QFrame(this);
    headerFrame->setStyleSheet(QString("QFrame { background-color: %1; border-radius: 6px; }")
        .arg(Tokens::Colors::BgElevated));
    
    d->headerLayout = new QHBoxLayout(headerFrame);
    d->headerLayout->setContentsMargins(16, 12, 16, 12);
    d->headerLayout->setSpacing(8);
    
    d->buttonGroup = new QButtonGroup(this);
    d->buttonGroup->setExclusive(true);

    // 我的订阅按钮
    QPushButton* mySignalsBtn = new QPushButton(QStringLiteral("我的订阅"), this);
    d->buttonGroup->addButton(mySignalsBtn);
    mySignalsBtn->setCheckable(true);
    mySignalsBtn->setChecked(true);
    mySignalsBtn->setFixedHeight(32);
    mySignalsBtn->setCursor(Qt::PointingHandCursor);
    mySignalsBtn->setStyleSheet(QString(R"(
        QPushButton {
            background-color: transparent;
            color: %1;
            border: none;
            border-radius: 4px;
            padding: 6px 16px;
            font-size: 13px;
        }
        QPushButton:checked {
            background-color: %2;
            color: %3;
        }
        QPushButton:hover:!checked {
            background-color: rgba(59, 130, 246, 0.1);
        }
    )").arg(Tokens::Colors::TextSecondary, 
            Tokens::Colors::PrimaryLight, 
            Tokens::Colors::Primary));

    // 推荐信号按钮
    QPushButton* recommendBtn = new QPushButton(QStringLiteral("推荐信号"), this);
    d->buttonGroup->addButton(recommendBtn);
    recommendBtn->setCheckable(true);
    recommendBtn->setFixedHeight(32);
    recommendBtn->setCursor(Qt::PointingHandCursor);
    recommendBtn->setStyleSheet(mySignalsBtn->styleSheet());

    // 排行榜按钮
    QPushButton* rankBtn = new QPushButton(QStringLiteral("排行榜"), this);
    d->buttonGroup->addButton(rankBtn);
    rankBtn->setCheckable(true);
    rankBtn->setFixedHeight(32);
    rankBtn->setCursor(Qt::PointingHandCursor);
    rankBtn->setStyleSheet(mySignalsBtn->styleSheet());

    d->headerLayout->addWidget(mySignalsBtn);
    d->headerLayout->addWidget(recommendBtn);
    d->headerLayout->addWidget(rankBtn);
    d->headerLayout->addStretch();
    
    // 刷新按钮
    QPushButton* refreshBtn = new QPushButton(QStringLiteral("刷新"), this);
    refreshBtn->setFixedSize(60, 32);
    refreshBtn->setCursor(Qt::PointingHandCursor);
    refreshBtn->setStyleSheet(QString(R"(
        QPushButton {
            background-color: %1;
            color: %2;
            border: none;
            border-radius: 4px;
            font-size: 12px;
        }
        QPushButton:hover {
            background-color: %3;
        }
    )").arg(Tokens::Colors::BgSurface, Tokens::Colors::TextSecondary, Tokens::Colors::Border));
    d->headerLayout->addWidget(refreshBtn);
    
    d->mainLayout->addWidget(headerFrame);

    // 创建网格容器
    d->gridContainer = new QWidget(this);
    d->gridLayout = new QGridLayout(d->gridContainer);
    d->gridLayout->setSpacing(Impl::SPACING);
    d->gridLayout->setContentsMargins(0, 0, 0, 0);
    d->gridLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    // 创建卡片数据
    QStringList names = {QStringLiteral("量化策略A"), QStringLiteral("趋势跟踪B"), 
                         QStringLiteral("价值投资C"), QStringLiteral("短线精灵D"),
                         QStringLiteral("均值回归E"), QStringLiteral("动量策略F"), 
                         QStringLiteral("套利策略G")};
    QList<double> returns = {156.8, 89.3, 45.2, 234.5, 67.5, 123.4, 89.0};
    QList<int> winRates = {78, 65, 82, 71, 69, 75, 80};
    QList<int> followers = {2340, 1890, 3200, 1560, 2100, 1800, 2900};

    // 创建所有卡片
    for (int i = 0; i < names.size(); ++i) {
        CardWidget* card = createCard(names[i], returns[i], winRates[i], followers[i]);
        d->cards.append(card);
        d->gridLayout->addWidget(card, 0, i);
    }

    d->mainLayout->addWidget(d->gridContainer);
    d->mainLayout->addStretch();

    // 初始布局
    QTimer::singleShot(0, this, [this]() { updateGridLayout(); });
}

CardWidget* SignalCenterPage::createCard(const QString& name, double returnRate,
                                         int winRate, int followers)
{
    CardWidget* card = new CardWidget(name, this);
    card->setFixedSize(Impl::CARD_WIDTH, Impl::CARD_MAX_HEIGHT);
    card->setStyleSheet(QString(R"(
        CardWidget {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 6px;
        }
    )").arg(Tokens::Colors::BgElevated, Tokens::Colors::Border));

    QWidget* content = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(content);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);

    // 收益率行
    QHBoxLayout* returnLayout = new QHBoxLayout();
    QLabel* returnLabel = new QLabel(QStringLiteral("累计收益"));
    returnLabel->setStyleSheet(QString("color: %1; font-size: 11px;")
        .arg(Tokens::Colors::TextTertiary));
    returnLayout->addWidget(returnLabel);
    returnLayout->addStretch();
    
    QLabel* returnValue = new QLabel(QString("+%1%").arg(returnRate, 0, 'f', 1));
    returnValue->setStyleSheet(QString("color: %1; font-size: 20px; font-weight: bold; font-family: 'Consolas', monospace;")
        .arg(Tokens::Colors::Success));
    returnLayout->addWidget(returnValue);
    layout->addLayout(returnLayout);

    // 胜率行
    QHBoxLayout* winLayout = new QHBoxLayout();
    QLabel* winLabel = new QLabel(QStringLiteral("胜率"));
    winLabel->setStyleSheet(QString("color: %1; font-size: 11px;")
        .arg(Tokens::Colors::TextTertiary));
    winLayout->addWidget(winLabel);
    winLayout->addStretch();
    
    QLabel* winValue = new QLabel(QString("%1%").arg(winRate));
    winValue->setStyleSheet(QString("color: %1; font-size: 13px; font-weight: 600;")
        .arg(Tokens::Colors::TextPrimary));
    winLayout->addWidget(winValue);
    layout->addLayout(winLayout);

    // 订阅人数行
    QHBoxLayout* followLayout = new QHBoxLayout();
    QLabel* followLabel = new QLabel(QStringLiteral("订阅人数"));
    followLabel->setStyleSheet(QString("color: %1; font-size: 11px;")
        .arg(Tokens::Colors::TextTertiary));
    followLayout->addWidget(followLabel);
    followLayout->addStretch();
    
    QLabel* followValue = new QLabel(QString::number(followers));
    followValue->setStyleSheet(QString("color: %1; font-size: 13px;")
        .arg(Tokens::Colors::TextSecondary));
    followLayout->addWidget(followValue);
    layout->addLayout(followLayout);

    layout->addStretch();

    // 订阅按钮
    QPushButton* subBtn = new QPushButton(QStringLiteral("订阅 ¥99/月"));
    subBtn->setFixedHeight(32);
    subBtn->setCursor(Qt::PointingHandCursor);
    subBtn->setStyleSheet(QString(R"(
        QPushButton {
            background-color: %1;
            color: white;
            border: none;
            border-radius: 4px;
            font-weight: 600;
            font-size: 12px;
        }
        QPushButton:hover {
            background-color: %2;
        }
        QPushButton:pressed {
            background-color: %3;
        }
    )").arg(Tokens::Colors::Primary, Tokens::Colors::PrimaryHover, Tokens::Colors::PrimaryDark));
    layout->addWidget(subBtn);

    card->setContent(content);
    return card;
}

void SignalCenterPage::resizeEvent(QResizeEvent* event)
{
    BasePage::resizeEvent(event);
    updateGridLayout();
}

void SignalCenterPage::updateGridLayout()
{
    if (d->cards.isEmpty()) return;

    // 计算可用宽度（减去边距）
    int availableWidth = width() - Impl::MARGIN * 2;

    // 计算每行可容纳的列数
    int columns = qMax(1, (availableWidth + Impl::SPACING) / (Impl::CARD_WIDTH + Impl::SPACING));

    // 限制最大列数
    columns = qMin(columns, d->cards.size());

    // 如果列数没变，不需要重排
    if (columns == d->currentColumns && d->gridLayout->count() == d->cards.size()) {
        return;
    }
    d->currentColumns = columns;

    // 重新排列卡片
    for (CardWidget* card : d->cards) {
        d->gridLayout->removeWidget(card);
    }

    // 重新添加
    for (int i = 0; i < d->cards.size(); ++i) {
        int row = i / columns;
        int col = i % columns;
        d->gridLayout->addWidget(d->cards[i], row, col);
    }

    // 设置列拉伸因子
    for (int c = 0; c < columns; ++c) {
        d->gridLayout->setColumnStretch(c, 0);
    }
    d->gridLayout->setColumnStretch(columns, 1);
}
