#include "SignalCenterPage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QGridLayout>
#include <QButtonGroup>
#include <QResizeEvent>
#include <QScrollArea>

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
    static constexpr int CARD_WIDTH = 300;     // 卡片固定宽度
    static constexpr int CARD_MAX_HEIGHT = 200;
    static constexpr int SPACING = 20;         // 卡片间距
    static constexpr int MARGIN = 24;          // 边距
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
    d->mainLayout->setContentsMargins(24, 24, 24, 24);
    d->mainLayout->setSpacing(24);

    // 头部
    d->headerLayout = new QHBoxLayout();
    d->buttonGroup = new QButtonGroup(this);
    d->buttonGroup->setExclusive(true);

    QPushButton* mySignalsBtn = new QPushButton("我的订阅", this);
    d->buttonGroup->addButton(mySignalsBtn);
    mySignalsBtn->setCheckable(true);
    mySignalsBtn->setChecked(true);
    mySignalsBtn->setStyleSheet(R"(
        QPushButton {
            background-color: transparent;
            color: #9CA3AF;
            border: none;
            border-radius: 8px;
            padding: 10px 20px;
            font-weight: 500;
        }
        QPushButton:checked {
            background-color: rgba(59, 130, 246, 0.2);
            color: #3B82F6;
        }
        QPushButton:hover {
            background-color: rgba(59, 130, 246, 0.1);
        }
    )");

    d->headerLayout->addWidget(mySignalsBtn);
    d->headerLayout->addStretch();
    d->mainLayout->addLayout(d->headerLayout);

    // 创建网格容器（放在ScrollArea中更合理）
    d->gridContainer = new QWidget(this);
    d->gridLayout = new QGridLayout(d->gridContainer);
    d->gridLayout->setSpacing(Impl::SPACING);
    d->gridLayout->setContentsMargins(0, 0, 0, 0);
    d->gridLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    // 创建卡片数据
    QStringList names = {"量化策略A", "趋势跟踪B", "价值投资C", "短线精灵D",
                         "均值回归E", "动量策略F", "套利策略G"};
    QList<double> returns = {156.8, 89.3, 45.2, 234.5, 67.5, 123.4, 89.0};
    QList<int> winRates = {78, 65, 82, 71, 69, 75, 80};
    QList<int> followers = {2340, 1890, 3200, 1560, 2100, 1800, 2900};

    // 创建所有卡片
    for (int i = 0; i < names.size(); ++i) {
        CardWidget* card = createCard(names[i], returns[i], winRates[i], followers[i]);
        d->cards.append(card);
        d->gridLayout->addWidget(card, 0, i); // 先全部放在第一行，resize时会重排
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

    QWidget* content = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(content);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    // 收益率
    QHBoxLayout* returnLayout = new QHBoxLayout();
    QLabel* returnLabel = new QLabel("累计收益");
    returnLabel->setStyleSheet("color: #9CA3AF; font-size: 12px;");
    returnLayout->addWidget(returnLabel);
    returnLayout->addStretch();
    QLabel* returnValue = new QLabel(QString("+%1%").arg(returnRate, 0, 'f', 1));
    returnValue->setStyleSheet("color: #10B981; font-size: 24px; font-weight: 700;");
    returnLayout->addWidget(returnValue);
    layout->addLayout(returnLayout);

    // 胜率
    QHBoxLayout* winLayout = new QHBoxLayout();
    QLabel* winLabel = new QLabel("胜率");
    winLabel->setStyleSheet("color: #9CA3AF; font-size: 12px;");
    winLayout->addWidget(winLabel);
    winLayout->addStretch();
    QLabel* winValue = new QLabel(QString("%1%").arg(winRate));
    winValue->setStyleSheet("color: white; font-weight: 600;");
    winLayout->addWidget(winValue);
    layout->addLayout(winLayout);

    // 订阅人数
    QHBoxLayout* followLayout = new QHBoxLayout();
    QLabel* followLabel = new QLabel("订阅人数");
    followLabel->setStyleSheet("color: #9CA3AF; font-size: 12px;");
    followLayout->addWidget(followLabel);
    followLayout->addStretch();
    QLabel* followValue = new QLabel(QString::number(followers));
    followValue->setStyleSheet("color: white;");
    followLayout->addWidget(followValue);
    layout->addLayout(followLayout);

    layout->addStretch();

    // 订阅按钮
    QPushButton* subBtn = new QPushButton("订阅 ¥99/月");
    subBtn->setFixedHeight(36);
    subBtn->setCursor(Qt::PointingHandCursor);
    subBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #3B82F6;
            color: white;
            border: none;
            border-radius: 8px;
            font-weight: 600;
            font-size: 13px;
        }
        QPushButton:hover {
            background-color: #2563EB;
        }
        QPushButton:pressed {
            background-color: #1D4ED8;
        }
    )");
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
    // 公式：列数 = 可用宽度 / (卡片宽度 + 间距)
    int columns = qMax(1, (availableWidth + Impl::SPACING) / (Impl::CARD_WIDTH + Impl::SPACING));

    // 限制最大列数（可选，防止超宽屏过于稀疏）
    columns = qMin(columns, d->cards.size());

    // 如果列数没变，不需要重排
    if (columns == d->currentColumns && d->gridLayout->count() == d->cards.size()) {
        return;
    }
    d->currentColumns = columns;

    // 重新排列卡片
    // 先移除但不删除
    for (CardWidget* card : d->cards) {
        d->gridLayout->removeWidget(card);
    }

    // 重新添加
    for (int i = 0; i < d->cards.size(); ++i) {
        int row = i / columns;
        int col = i % columns;
        d->gridLayout->addWidget(d->cards[i], row, col);
    }

    // 设置列拉伸因子，让卡片左对齐
    for (int c = 0; c < columns; ++c) {
        d->gridLayout->setColumnStretch(c, 0);
    }
    d->gridLayout->setColumnStretch(columns, 1); // 最后一列拉伸填充剩余空间
}
