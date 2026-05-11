/**
 * @file SignalCenterPage.cpp
 * @brief 信号中心页面实现 - 小卡片网格布局
 *
 * @details 功能：
 * - 我的订阅、推荐信号、排行榜、最新上线四个分类
 * - 列表排序功能（胜率、订阅数、收益率）
 * - 订阅/取消订阅功能
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
#include <QGridLayout>
#include <QComboBox>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>

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
// SignalMiniCard - 小型信号卡片组件
// ============================================================================
class SignalMiniCard : public QFrame
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
        bool isSubscribed = false;  // 是否已订阅
    };

    explicit SignalMiniCard(const Data& data, bool showSubscribed = false, QWidget* parent = nullptr);

    void updateSubscribeButton();

signals:
    void clicked();
    void subscribeClicked(const Data& data);
    void unsubscribeClicked(const Data& data);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    void setupUI();

    Data m_data;
    bool m_showSubscribed = false;  // 是否显示"已订阅"状态
    QLabel* m_nameLabel = nullptr;
    QLabel* m_returnLabel = nullptr;
    QLabel* m_winRateLabel = nullptr;
    QLabel* m_followersLabel = nullptr;
    QPushButton* m_subscribeBtn = nullptr;
};

SignalMiniCard::SignalMiniCard(const Data& data, bool showSubscribed, QWidget* parent)
    : QFrame(parent)
    , m_data(data)
    , m_showSubscribed(showSubscribed)
{
    setupUI();
}

void SignalMiniCard::updateSubscribeButton()
{
    if (m_showSubscribed || m_data.isSubscribed) {
        m_subscribeBtn->setText(QStringLiteral("已订阅"));
        m_subscribeBtn->setStyleSheet(QString(R"(
            QPushButton {
                background-color: %1;
                color: white;
                border: none;
                border-radius: 3px;
                padding: 3px 10px;
                font-size: 11px;
                font-weight: 600;
            }
            QPushButton:hover {
                background-color: %2;
            }
        )").arg(Colors::TextSecondary, Colors::Danger));
    } else {
        m_subscribeBtn->setText(QStringLiteral("订阅"));
        m_subscribeBtn->setStyleSheet(QString(R"(
            QPushButton {
                background-color: %1;
                color: white;
                border: none;
                border-radius: 3px;
                padding: 3px 10px;
                font-size: 11px;
                font-weight: 600;
            }
            QPushButton:hover {
                background-color: %2;
            }
        )").arg(COLOR_PRIMARY, Colors::PrimaryHover));
    }
}

void SignalMiniCard::setupUI()
{
    setStyleSheet(QString(R"(
        SignalMiniCard {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 8px;
        }
    )").arg(COLOR_BG_CARD, COLOR_SEPARATOR));

    auto* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(6);
    shadow->setColor(QColor(0, 0, 0, 25));
    shadow->setOffset(0, 2);
    setGraphicsEffect(shadow);

    setFixedSize(220, 140);
    setCursor(Qt::PointingHandCursor);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 10);
    mainLayout->setSpacing(6);

    // 标题行：名称 + 策略标签
    auto* titleRow = new QHBoxLayout();
    titleRow->setSpacing(6);

    m_nameLabel = new QLabel(m_data.name);
    m_nameLabel->setStyleSheet(QString(
        "QLabel { color: %1; font-size: 13px; font-weight: bold; }"
    ).arg(COLOR_TEXT_TITLE));
    m_nameLabel->setWordWrap(false);
    titleRow->addWidget(m_nameLabel, 1);

    if (!m_data.strategy.isEmpty()) {
        QLabel* strategyLabel = new QLabel(m_data.strategy);
        strategyLabel->setStyleSheet(QString(
            "QLabel { color: %1; font-size: 10px; background: %2; padding: 1px 6px; border-radius: 2px; }"
        ).arg(COLOR_PRIMARY, Colors::PrimaryLight));
        titleRow->addWidget(strategyLabel);
    }

    mainLayout->addLayout(titleRow);

    // 收益率（突出显示）
    QString returnText = m_data.returnRate >= 0
        ? QString("+%1%").arg(m_data.returnRate, 0, 'f', 1)
        : QString("%1%").arg(m_data.returnRate, 0, 'f', 1);
    m_returnLabel = new QLabel(returnText);
    m_returnLabel->setStyleSheet(QString(
        "QLabel { color: %1; font-size: 20px; font-weight: bold; font-family: 'Consolas', monospace; }"
    ).arg(m_data.returnRate >= 0 ? COLOR_SUCCESS : COLOR_DANGER));
    mainLayout->addWidget(m_returnLabel);

    mainLayout->addStretch();

    // 底部统计行
    auto* statsRow = new QHBoxLayout();
    statsRow->setSpacing(12);

    // 胜率
    auto* winRateLayout = new QVBoxLayout();
    winRateLayout->setSpacing(2);
    QLabel* winRateTitle = new QLabel(QStringLiteral("胜率"));
    winRateTitle->setStyleSheet(QString("QLabel { color: %1; font-size: 10px; }").arg(COLOR_TEXT_META));
    winRateLayout->addWidget(winRateTitle);
    m_winRateLabel = new QLabel(QString("%1%").arg(m_data.winRate));
    m_winRateLabel->setStyleSheet(QString("QLabel { color: %1; font-size: 12px; font-weight: 600; }").arg(COLOR_TEXT_VALUE));
    winRateLayout->addWidget(m_winRateLabel);
    statsRow->addLayout(winRateLayout);

    // 订阅人数
    auto* followersLayout = new QVBoxLayout();
    followersLayout->setSpacing(2);
    QLabel* followersTitle = new QLabel(QStringLiteral("订阅"));
    followersTitle->setStyleSheet(QString("QLabel { color: %1; font-size: 10px; }").arg(COLOR_TEXT_META));
    followersLayout->addWidget(followersTitle);
    m_followersLabel = new QLabel(QString::number(m_data.followers));
    m_followersLabel->setStyleSheet(QString("QLabel { color: %1; font-size: 12px; }").arg(COLOR_TEXT_VALUE));
    followersLayout->addWidget(m_followersLabel);
    statsRow->addLayout(followersLayout);

    statsRow->addStretch();

    // 订阅按钮
    m_subscribeBtn = new QPushButton();
    m_subscribeBtn->setCursor(Qt::PointingHandCursor);
    m_subscribeBtn->setFixedSize(50, 24);
    updateSubscribeButton();

    connect(m_subscribeBtn, &QPushButton::clicked, this, [this]() {
        if (m_showSubscribed || m_data.isSubscribed) {
            emit unsubscribeClicked(m_data);
        } else {
            emit subscribeClicked(m_data);
        }
    });
    statsRow->addWidget(m_subscribeBtn);

    mainLayout->addLayout(statsRow);
}

void SignalMiniCard::mousePressEvent(QMouseEvent* event)
{
    QFrame::mousePressEvent(event);
    if (event->button() == Qt::LeftButton && !m_subscribeBtn->geometry().contains(event->pos())) {
        emit clicked();
    }
}

void SignalMiniCard::enterEvent(QEnterEvent* event)
{
    QFrame::enterEvent(event);
    setStyleSheet(QString(R"(
        SignalMiniCard {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 8px;
        }
    )").arg(COLOR_BG_CARD, COLOR_PRIMARY));
}

void SignalMiniCard::leaveEvent(QEvent* event)
{
    QFrame::leaveEvent(event);
    setStyleSheet(QString(R"(
        SignalMiniCard {
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
    QGridLayout* cardsGrid = nullptr;
    QComboBox* sortCombo = nullptr;
    QLabel* countLabel = nullptr;

    QVector<SignalMiniCard::Data> allSignals;
    QVector<SignalMiniCard::Data> subscribedSignals;  // 已订阅的信号
    QVector<SignalMiniCard*> cards;

    QString currentCategory = QStringLiteral("我的订阅");
    int cardsPerRow = 4;
    QString currentSort = QStringLiteral("return_desc");

    // 数据库操作
    bool initDatabase();
    void loadSubscriptions();
    void saveSubscription(const SignalMiniCard::Data& data);
    void removeSubscription(const QString& signalId);
    bool isSubscribed(const QString& signalId);
};

bool SignalCenterPage::Impl::initDatabase()
{
    if (!QSqlDatabase::hasDatabase(QStringLiteral("wealthpilot"))) {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("wealthpilot"));
        db.setDatabaseName(QStringLiteral("wealthpilot.db"));
        if (!db.open()) {
            LOG_ERROR("Failed to open database");
            return false;
        }
    }

    QSqlDatabase db = QSqlDatabase::database(QStringLiteral("wealthpilot"));
    QSqlQuery query(db);
    query.exec(QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS signal_subscriptions (
            id TEXT PRIMARY KEY,
            name TEXT NOT NULL,
            return_rate REAL,
            win_rate INTEGER,
            followers INTEGER,
            price REAL,
            strategy TEXT,
            description TEXT,
            subscribe_time DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )"));

    return true;
}

void SignalCenterPage::Impl::loadSubscriptions()
{
    subscribedSignals.clear();

    QSqlDatabase db = QSqlDatabase::database(QStringLiteral("wealthpilot"));
    if (!db.isOpen()) return;

    QSqlQuery query(db);
    query.exec(QStringLiteral("SELECT id, name, return_rate, win_rate, followers, price, strategy, description FROM signal_subscriptions ORDER BY subscribe_time DESC"));

    while (query.next()) {
        SignalMiniCard::Data data;
        data.id = query.value(0).toString();
        data.name = query.value(1).toString();
        data.returnRate = query.value(2).toDouble();
        data.winRate = query.value(3).toInt();
        data.followers = query.value(4).toInt();
        data.price = query.value(5).toDouble();
        data.strategy = query.value(6).toString();
        data.description = query.value(7).toString();
        data.isSubscribed = true;
        subscribedSignals.append(data);
    }
}

void SignalCenterPage::Impl::saveSubscription(const SignalMiniCard::Data& data)
{
    QSqlDatabase db = QSqlDatabase::database(QStringLiteral("wealthpilot"));
    if (!db.isOpen()) return;

    QSqlQuery query(db);
    query.prepare(QStringLiteral(R"(
        INSERT OR REPLACE INTO signal_subscriptions (id, name, return_rate, win_rate, followers, price, strategy, description)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
    )"));
    query.addBindValue(data.id);
    query.addBindValue(data.name);
    query.addBindValue(data.returnRate);
    query.addBindValue(data.winRate);
    query.addBindValue(data.followers);
    query.addBindValue(data.price);
    query.addBindValue(data.strategy);
    query.addBindValue(data.description);
    query.exec();

    LOG_INFO(QString("Subscription saved: %1").arg(data.name));
}

void SignalCenterPage::Impl::removeSubscription(const QString& signalId)
{
    QSqlDatabase db = QSqlDatabase::database(QStringLiteral("wealthpilot"));
    if (!db.isOpen()) return;

    QSqlQuery query(db);
    query.prepare(QStringLiteral("DELETE FROM signal_subscriptions WHERE id = ?"));
    query.addBindValue(signalId);
    query.exec();

    LOG_INFO(QString("Subscription removed: %1").arg(signalId));
}

bool SignalCenterPage::Impl::isSubscribed(const QString& signalId)
{
    for (const auto& sig : subscribedSignals) {
        if (sig.id == signalId) return true;
    }
    return false;
}

SignalCenterPage::SignalCenterPage(QWidget *parent)
    : BasePage(parent)
    , d(std::make_unique<Impl>())
{
    d->initDatabase();
    d->loadSubscriptions();
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
    d->loadSubscriptions();
    updateCards();
    LOG_INFO("SignalCenterPage initialized");
}

void SignalCenterPage::setupUI()
{
    setStyleSheet(QString("QWidget { background-color: %1; }").arg(COLOR_BG_GLOBAL));

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    setupCategoryBar();
    setupToolBar();
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
        btn->setProperty("category", cat);

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

        connect(btn, &QPushButton::clicked, this, [this, btn, activeStyle, normalStyle]() {
            auto* bar = qobject_cast<QFrame*>(btn->parent());
            if (bar) {
                for (auto* child : bar->findChildren<QPushButton*>()) {
                    bool isActive = child == btn;
                    child->setChecked(isActive);
                    child->setStyleSheet(isActive ? activeStyle : normalStyle);
                }
            }
            onCategoryClicked(btn->property("category").toString());
        });

        barLayout->addWidget(btn);
    }

    barLayout->addStretch();

    auto* mainLayout = qobject_cast<QVBoxLayout*>(QWidget::layout());
    if (mainLayout) {
        mainLayout->insertWidget(0, categoryBar);
    }
}

void SignalCenterPage::setupToolBar()
{
    auto* toolBar = new QFrame();
    toolBar->setStyleSheet(QString(
        "QFrame { background-color: %1; border-bottom: 1px solid %2; }"
    ).arg(COLOR_BG_GLOBAL, COLOR_SEPARATOR));
    toolBar->setFixedHeight(40);

    auto* barLayout = new QHBoxLayout(toolBar);
    barLayout->setContentsMargins(16, 6, 16, 6);
    barLayout->setSpacing(12);

    // 数量标签
    d->countLabel = new QLabel(QStringLiteral("共 0 个信号"));
    d->countLabel->setStyleSheet(QString("QLabel { color: %1; font-size: 12px; }").arg(COLOR_TEXT_META));
    barLayout->addWidget(d->countLabel);

    barLayout->addStretch();

    // 排序标签
    QLabel* sortLabel = new QLabel(QStringLiteral("排序:"));
    sortLabel->setStyleSheet(QString("QLabel { color: %1; font-size: 12px; }").arg(COLOR_TEXT_META));
    barLayout->addWidget(sortLabel);

    // 排序下拉框
    d->sortCombo = new QComboBox();
    d->sortCombo->addItem(QStringLiteral("收益率 ↓"), QStringLiteral("return_desc"));
    d->sortCombo->addItem(QStringLiteral("收益率 ↑"), QStringLiteral("return_asc"));
    d->sortCombo->addItem(QStringLiteral("胜率 ↓"), QStringLiteral("winrate_desc"));
    d->sortCombo->addItem(QStringLiteral("胜率 ↑"), QStringLiteral("winrate_asc"));
    d->sortCombo->addItem(QStringLiteral("订阅数 ↓"), QStringLiteral("followers_desc"));
    d->sortCombo->addItem(QStringLiteral("订阅数 ↑"), QStringLiteral("followers_asc"));
    d->sortCombo->setStyleSheet(QString(R"(
        QComboBox {
            background-color: %1;
            color: %2;
            border: 1px solid %3;
            border-radius: 4px;
            padding: 4px 8px;
            font-size: 12px;
            min-width: 100px;
        }
        QComboBox:hover {
            border-color: %4;
        }
        QComboBox::drop-down {
            border: none;
            width: 20px;
        }
        QComboBox QAbstractItemView {
            background-color: %1;
            color: %2;
            border: 1px solid %3;
            selection-background-color: %5;
        }
    )").arg(COLOR_BG_CARD, COLOR_TEXT_TITLE, COLOR_SEPARATOR, COLOR_PRIMARY, COLOR_HOVER_BG));
    connect(d->sortCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        d->currentSort = d->sortCombo->itemData(index).toString();
        updateCards();
    });
    barLayout->addWidget(d->sortCombo);

    // 刷新按钮
    QPushButton* refreshBtn = new QPushButton(QStringLiteral("刷新"));
    refreshBtn->setFixedHeight(28);
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
        d->loadSubscriptions();
        loadDemoData();
        LOG_INFO("Signal data refreshed");
    });
    barLayout->addWidget(refreshBtn);

    auto* mainLayout = qobject_cast<QVBoxLayout*>(QWidget::layout());
    if (mainLayout) {
        mainLayout->insertWidget(1, toolBar);
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

    auto* contentLayout = new QVBoxLayout(d->scrollContent);
    contentLayout->setContentsMargins(16, 16, 16, 16);
    contentLayout->setSpacing(0);

    // 网格容器
    auto* gridContainer = new QWidget();
    d->cardsGrid = new QGridLayout(gridContainer);
    d->cardsGrid->setContentsMargins(0, 0, 0, 0);
    d->cardsGrid->setSpacing(12);
    d->cardsGrid->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    contentLayout->addWidget(gridContainer);
    contentLayout->addStretch();

    d->scrollArea->setWidget(d->scrollContent);

    auto* mainLayout = qobject_cast<QVBoxLayout*>(QWidget::layout());
    if (mainLayout) {
        mainLayout->addWidget(d->scrollArea);
    }
}

void SignalCenterPage::loadDemoData()
{
    // 所有可用信号
    d->allSignals = {
        {QStringLiteral("s1"), QStringLiteral("量化策略A"), 156.8, 78, 2340, 99.0, QStringLiteral("量化"), QStringLiteral("多因子选股策略")},
        {QStringLiteral("s2"), QStringLiteral("趋势跟踪B"), 89.3, 65, 1890, 79.0, QStringLiteral("趋势"), QStringLiteral("动量突破策略")},
        {QStringLiteral("s3"), QStringLiteral("价值投资C"), 45.2, 82, 3200, 59.0, QStringLiteral("价值"), QStringLiteral("低估值选股策略")},
        {QStringLiteral("s4"), QStringLiteral("短线精灵D"), 234.5, 71, 1560, 129.0, QStringLiteral("短线"), QStringLiteral("日内高频交易")},
        {QStringLiteral("s5"), QStringLiteral("均值回归E"), 67.5, 69, 2100, 89.0, QStringLiteral("量化"), QStringLiteral("统计套利策略")},
        {QStringLiteral("s6"), QStringLiteral("动量策略F"), 123.4, 75, 1800, 109.0, QStringLiteral("趋势"), QStringLiteral("行业轮动策略")},
        {QStringLiteral("s7"), QStringLiteral("套利策略G"), 89.0, 80, 2900, 69.0, QStringLiteral("套利"), QStringLiteral("期现套利策略")},
        {QStringLiteral("s8"), QStringLiteral("成长精选H"), 178.6, 76, 2450, 119.0, QStringLiteral("成长"), QStringLiteral("高成长选股策略")},
        {QStringLiteral("s9"), QStringLiteral("指数增强I"), 56.3, 73, 1980, 89.0, QStringLiteral("量化"), QStringLiteral("沪深300增强")},
        {QStringLiteral("s10"), QStringLiteral("对冲套利J"), 34.8, 85, 2650, 149.0, QStringLiteral("套利"), QStringLiteral("Alpha对冲策略")},
        {QStringLiteral("s11"), QStringLiteral("事件驱动K"), 112.5, 68, 1420, 99.0, QStringLiteral("事件"), QStringLiteral("并购重组套利")},
        {QStringLiteral("s12"), QStringLiteral("技术分析L"), 78.9, 62, 1780, 69.0, QStringLiteral("技术"), QStringLiteral("K线形态策略")}
    };

    // 更新订阅状态
    for (auto& sig : d->allSignals) {
        sig.isSubscribed = d->isSubscribed(sig.id);
    }

    updateCards();
}

QVector<SignalMiniCard::Data> SignalCenterPage::getFilteredSignals()
{
    QVector<SignalMiniCard::Data> result;

    if (d->currentCategory == QStringLiteral("我的订阅")) {
        result = d->subscribedSignals;
    } else if (d->currentCategory == QStringLiteral("推荐信号")) {
        // 推荐未订阅的高收益信号
        for (const auto& sig : d->allSignals) {
            if (!d->isSubscribed(sig.id)) {
                result.append(sig);
            }
        }
    } else if (d->currentCategory == QStringLiteral("排行榜")) {
        // 按收益率排序的所有信号
        result = d->allSignals;
    } else if (d->currentCategory == QStringLiteral("最新上线")) {
        // 模拟最新上线（取后6个）
        for (int i = d->allSignals.size() - 1; i >= std::max(0, (int)d->allSignals.size() - 6); --i) {
            result.append(d->allSignals[i]);
        }
    }

    return result;
}

void SignalCenterPage::sortSignals(QVector<SignalMiniCard::Data>& signals)
{
    std::sort(signals.begin(), signals.end(), [this](const SignalMiniCard::Data& a, const SignalMiniCard::Data& b) {
        if (d->currentSort == QStringLiteral("return_desc")) {
            return a.returnRate > b.returnRate;
        } else if (d->currentSort == QStringLiteral("return_asc")) {
            return a.returnRate < b.returnRate;
        } else if (d->currentSort == QStringLiteral("winrate_desc")) {
            return a.winRate > b.winRate;
        } else if (d->currentSort == QStringLiteral("winrate_asc")) {
            return a.winRate < b.winRate;
        } else if (d->currentSort == QStringLiteral("followers_desc")) {
            return a.followers > b.followers;
        } else if (d->currentSort == QStringLiteral("followers_asc")) {
            return a.followers < b.followers;
        }
        return a.returnRate > b.returnRate;
    });
}

void SignalCenterPage::updateCards()
{
    // 清除现有卡片
    for (auto* card : d->cards) {
        d->cardsGrid->removeWidget(card);
        card->deleteLater();
    }
    d->cards.clear();

    // 获取过滤后的信号
    auto signals = getFilteredSignals();

    // 排序
    sortSignals(signals);

    // 更新数量标签
    d->countLabel->setText(QStringLiteral("共 %1 个信号").arg(signals.size()));

    // 添加新卡片到网格
    int row = 0;
    int col = 0;
    bool showSubscribed = (d->currentCategory == QStringLiteral("我的订阅"));

    for (const auto& signal : signals) {
        auto* card = new SignalMiniCard(signal, showSubscribed);
        connect(card, &SignalMiniCard::clicked, this, &SignalCenterPage::onCardClicked);
        connect(card, &SignalMiniCard::subscribeClicked, this, [this](const SignalMiniCard::Data& data) {
            onSubscribeClicked(data);
        });
        connect(card, &SignalMiniCard::unsubscribeClicked, this, [this](const SignalMiniCard::Data& data) {
            onUnsubscribeClicked(data);
        });

        d->cardsGrid->addWidget(card, row, col, Qt::AlignTop | Qt::AlignLeft);
        d->cards.append(card);

        col++;
        if (col >= d->cardsPerRow) {
            col = 0;
            row++;
        }
    }
}

void SignalCenterPage::onCategoryClicked(const QString& category)
{
    d->currentCategory = category;
    updateCards();
    LOG_INFO(QString("Category changed to: %1").arg(category));
}

void SignalCenterPage::onCardClicked()
{
    auto* card = qobject_cast<SignalMiniCard*>(sender());
    if (card) {
        LOG_INFO("Signal card clicked");
    }
}

void SignalCenterPage::onSubscribeClicked(const SignalMiniCard::Data& data)
{
    // 保存到数据库
    d->saveSubscription(data);

    // 更新订阅列表
    SignalMiniCard::Data subscribedData = data;
    subscribedData.isSubscribed = true;
    d->subscribedSignals.append(subscribedData);

    // 更新所有信号中的订阅状态
    for (auto& sig : d->allSignals) {
        if (sig.id == data.id) {
            sig.isSubscribed = true;
            break;
        }
    }

    // 刷新显示
    updateCards();

    QMessageBox::information(this, QStringLiteral("订阅成功"),
        QStringLiteral("已成功订阅信号：%1").arg(data.name));

    LOG_INFO(QString("Subscribed: %1").arg(data.name));
}

void SignalCenterPage::onUnsubscribeClicked(const SignalMiniCard::Data& data)
{
    // 从数据库删除
    d->removeSubscription(data.id);

    // 更新订阅列表
    for (int i = 0; i < d->subscribedSignals.size(); ++i) {
        if (d->subscribedSignals[i].id == data.id) {
            d->subscribedSignals.removeAt(i);
            break;
        }
    }

    // 更新所有信号中的订阅状态
    for (auto& sig : d->allSignals) {
        if (sig.id == data.id) {
            sig.isSubscribed = false;
            break;
        }
    }

    // 刷新显示
    updateCards();

    LOG_INFO(QString("Unsubscribed: %1").arg(data.name));
}

} // namespace WealthPilot

#include "SignalCenterPage.moc"
