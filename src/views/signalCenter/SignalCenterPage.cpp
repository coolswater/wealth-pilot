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
#include "ui/styles/ButtonStyles.h"
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
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QFontMetrics>

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
// SortButton - 可点击排序按钮
// ============================================================================
class SortButton : public QPushButton
{
    Q_OBJECT

public:
    explicit SortButton(const QString& text, const QString& sortKey, QWidget* parent = nullptr)
        : QPushButton(parent)
        , m_baseText(text)
        , m_sortKey(sortKey)
        , m_ascending(false)
        , m_active(false)
    {
        setCursor(Qt::PointingHandCursor);
        setFixedHeight(28);
        updateStyle();
        connect(this, &QPushButton::clicked, this, [this]() {
            toggleOrder();
            emit sortClicked(m_sortKey, m_ascending);
        });
    }

    QString sortKey() const { return m_sortKey; }
    bool isAscending() const { return m_ascending; }

    void toggleOrder() {
        m_ascending = !m_ascending;
        updateStyle();
    }

    void setActive(bool active) {
        m_active = active;
        updateStyle();
    }

signals:
    void sortClicked(const QString& sortKey, bool ascending);

private:
    void updateStyle() {
        QString icon = m_ascending ? QStringLiteral(" ↑") : QStringLiteral(" ↓");
        setText(m_baseText + icon);

        if (m_active) {
            setStyleSheet(QString(R"(
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
        } else {
            setStyleSheet(QString(R"(
                QPushButton {
                    background-color: transparent;
                    color: %1;
                    border: 1px solid %2;
                    border-radius: 4px;
                    padding: 4px 12px;
                    font-size: 12px;
                }
                QPushButton:hover {
                    background-color: %3;
                    border-color: %4;
                }
            )").arg(COLOR_TEXT_META, COLOR_SEPARATOR, COLOR_HOVER_BG, COLOR_PRIMARY));
        }
    }

    QString m_baseText;
    QString m_sortKey;
    bool m_ascending = false;
    bool m_active = false;
};

// ============================================================================
// SignalMiniCard - 小型信号卡片组件
// ============================================================================
class SignalMiniCard : public QFrame
{
    Q_OBJECT

public:
    explicit SignalMiniCard(const SignalCardData& data, bool showSubscribed = false, QWidget* parent = nullptr);

signals:
    void clicked();
    void subscribeClicked(const SignalCardData& data);
    void unsubscribeClicked(const SignalCardData& data);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    void setupUI();

    SignalCardData m_data;
    bool m_showSubscribed = false;
    QLabel* m_nameLabel = nullptr;
    QLabel* m_returnLabel = nullptr;
    QLabel* m_winRateLabel = nullptr;
    QLabel* m_followersLabel = nullptr;
    QPushButton* m_subscribeBtn = nullptr;
};

SignalMiniCard::SignalMiniCard(const SignalCardData& data, bool showSubscribed, QWidget* parent)
    : QFrame(parent)
    , m_data(data)
    , m_showSubscribed(showSubscribed)
{
    setupUI();
}

void SignalMiniCard::setupUI()
{
    setStyleSheet(QString(R"(
        SignalMiniCard {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 10px;
        }
    )").arg(COLOR_BG_CARD, COLOR_SEPARATOR));

    auto* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(12);
    shadow->setColor(QColor(0, 0, 0, 40));
    shadow->setOffset(0, 4);
    setGraphicsEffect(shadow);

    setFixedSize(240, 160);
    setCursor(Qt::PointingHandCursor);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 14, 16, 12);
    mainLayout->setSpacing(8);

    // 标题行：名称 + 策略标签
    auto* titleRow = new QHBoxLayout();
    titleRow->setSpacing(8);

    m_nameLabel = new QLabel(m_data.name);
    m_nameLabel->setStyleSheet(QString(
        "QLabel { color: %1; font-size: 14px; font-weight: bold; }"
    ).arg(COLOR_TEXT_TITLE));
    m_nameLabel->setWordWrap(false);
    titleRow->addWidget(m_nameLabel, 1);

    if (!m_data.strategy.isEmpty()) {
        QLabel* strategyLabel = new QLabel(m_data.strategy);
        strategyLabel->setStyleSheet(QString(
            "QLabel { color: %1; font-size: 10px; background: %2; padding: 2px 8px; border-radius: 4px; font-weight: 500; }"
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
        "QLabel { color: %1; font-size: 26px; font-weight: bold; font-family: 'DIN Alternate', 'Consolas', monospace; }"
    ).arg(m_data.returnRate >= 0 ? COLOR_SUCCESS : COLOR_DANGER));
    mainLayout->addWidget(m_returnLabel);

    mainLayout->addStretch();

    // 底部统计行
    auto* statsRow = new QHBoxLayout();
    statsRow->setSpacing(16);

    // 胜率
    auto* winRateBox = new QVBoxLayout();
    winRateBox->setSpacing(2);
    QLabel* winRateTitle = new QLabel(QStringLiteral("胜率"));
    winRateTitle->setStyleSheet(QString("QLabel { color: %1; font-size: 11px; }").arg(COLOR_TEXT_META));
    winRateBox->addWidget(winRateTitle);
    m_winRateLabel = new QLabel(QString("%1%").arg(m_data.winRate));
    m_winRateLabel->setStyleSheet(QString("QLabel { color: %1; font-size: 14px; font-weight: 600; }").arg(COLOR_TEXT_VALUE));
    winRateBox->addWidget(m_winRateLabel);
    statsRow->addLayout(winRateBox);

    // 订阅人数
    auto* followersBox = new QVBoxLayout();
    followersBox->setSpacing(2);
    QLabel* followersTitle = new QLabel(QStringLiteral("订阅"));
    followersTitle->setStyleSheet(QString("QLabel { color: %1; font-size: 11px; }").arg(COLOR_TEXT_META));
    followersBox->addWidget(followersTitle);
    m_followersLabel = new QLabel(QString::number(m_data.followers));
    m_followersLabel->setStyleSheet(QString("QLabel { color: %1; font-size: 14px; font-weight: 600; }").arg(COLOR_TEXT_VALUE));
    followersBox->addWidget(m_followersLabel);
    statsRow->addLayout(followersBox);

    statsRow->addStretch();

    // 订阅按钮
    m_subscribeBtn = new QPushButton();
    m_subscribeBtn->setCursor(Qt::PointingHandCursor);
    m_subscribeBtn->setFixedSize(56, 28);

    if (m_showSubscribed || m_data.subscribed) {
        m_subscribeBtn->setText(QStringLiteral("已订阅"));
        m_subscribeBtn->setStyleSheet(QString(R"(
            QPushButton {
                background-color: %1;
                color: white;
                border: none;
                border-radius: 6px;
                font-size: 12px;
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
                border-radius: 6px;
                font-size: 12px;
                font-weight: 600;
            }
            QPushButton:hover {
                background-color: %2;
            }
        )").arg(COLOR_PRIMARY, Colors::PrimaryHover));
    }

    connect(m_subscribeBtn, &QPushButton::clicked, this, [this]() {
        if (m_showSubscribed || m_data.subscribed) {
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
            border: 2px solid %2;
            border-radius: 10px;
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
            border-radius: 10px;
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
    QLabel* countLabel = nullptr;

    // 排序按钮
    SortButton* returnSortBtn = nullptr;
    SortButton* winRateSortBtn = nullptr;
    SortButton* followersSortBtn = nullptr;

    QVector<SignalCardData> allSignals;
    QVector<SignalCardData> subscribedSignals;
    QVector<SignalMiniCard*> cards;

    QString currentCategory = QStringLiteral("我的订阅");
    int cardsPerRow = 4;
    QString currentSort = QStringLiteral("return");
    bool sortAscending = false;

    bool initDatabase();
    void loadSubscriptions();
    void saveSubscription(const SignalCardData& data);
    void removeSubscription(const QString& signalId);
    bool isSubscribed(const QString& signalId);
};

bool SignalCenterPage::Impl::initDatabase()
{
    QString connectionName = QStringLiteral("wealthpilot_signals");
    if (!QSqlDatabase::contains(connectionName)) {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
        db.setDatabaseName(QStringLiteral("wealthpilot.db"));
        if (!db.open()) {
            LOG_ERROR("Failed to open database");
            return false;
        }
    }

    QSqlDatabase db = QSqlDatabase::database(connectionName);
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

    QString connectionName = QStringLiteral("wealthpilot_signals");
    QSqlDatabase db = QSqlDatabase::database(connectionName);
    if (!db.isOpen()) return;

    QSqlQuery query(db);
    query.exec(QStringLiteral("SELECT id, name, return_rate, win_rate, followers, price, strategy, description FROM signal_subscriptions ORDER BY subscribe_time DESC"));

    while (query.next()) {
        SignalCardData data;
        data.id = query.value(0).toString();
        data.name = query.value(1).toString();
        data.returnRate = query.value(2).toDouble();
        data.winRate = query.value(3).toInt();
        data.followers = query.value(4).toInt();
        data.price = query.value(5).toDouble();
        data.strategy = query.value(6).toString();
        data.description = query.value(7).toString();
        data.subscribed = true;
        subscribedSignals.append(data);
    }
}

void SignalCenterPage::Impl::saveSubscription(const SignalCardData& data)
{
    QString connectionName = QStringLiteral("wealthpilot_signals");
    QSqlDatabase db = QSqlDatabase::database(connectionName);
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
    QString connectionName = QStringLiteral("wealthpilot_signals");
    QSqlDatabase db = QSqlDatabase::database(connectionName);
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
    : DataHubPageBase(parent)
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
    // ============================================================
    // 1. 设置 DataHub 订阅
    // ============================================================
    setupDataHubSubscriptions();
    
    // ============================================================
    // 2. 加载订阅数据
    // ============================================================
    d->loadSubscriptions();
    updateCards();
    
    LOG_INFO("SignalCenterPage initialized with DataHub");
}

void SignalCenterPage::setupDataHubSubscriptions()
{
    // 订阅信号列表
    dataHub().subscribePattern(this, "signal:*",
        [this](const QString& topic, const QVariant& value) {
            Q_UNUSED(value)
            if (topic == "signal:list") {
                // 更新信号列表
                updateCards();
            } else if (topic.startsWith("signal:subscribed")) {
                // 更新我的订阅
                d->loadSubscriptions();
            }
        });
    
    // 订阅信号更新
    dataHub().subscribe(this, "signal:updates",
        [this](const QString&, const QVariant& value) {
            Q_UNUSED(value)
            // 更新信号状态
            updateCards();
        });
    
    LOG_INFO("[SignalCenterPage] DataHub subscriptions setup complete");
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
    categoryBar->setFixedHeight(52);

    auto* barLayout = new QHBoxLayout(categoryBar);
    barLayout->setContentsMargins(20, 10, 20, 10);
    barLayout->setSpacing(4);

    // 页面标题
    auto* titleLabel = new QLabel(QStringLiteral("信号中心"));
    titleLabel->setStyleSheet(QString("font-size: 18px; font-weight: bold; color: %1;")
        .arg(COLOR_TEXT_TITLE));
    barLayout->addWidget(titleLabel);
    barLayout->addSpacing(20);

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
                border-radius: 6px;
                padding: 6px 20px;
                font-size: 13px;
                font-weight: 600;
            }
        )").arg(COLOR_PRIMARY);

        QString normalStyle = QString(R"(
            QPushButton {
                background-color: transparent;
                color: %1;
                border: none;
                border-radius: 6px;
                padding: 6px 20px;
                font-size: 13px;
            }
            QPushButton:hover {
                background-color: %2;
            }
        )").arg(COLOR_TEXT_META, COLOR_HOVER_BG);

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
    toolBar->setFixedHeight(48);

    auto* barLayout = new QHBoxLayout(toolBar);
    barLayout->setContentsMargins(20, 8, 20, 8);
    barLayout->setSpacing(12);

    // 数量标签
    d->countLabel = new QLabel(QStringLiteral("共 0 个信号"));
    d->countLabel->setStyleSheet(QString("QLabel { color: %1; font-size: 13px; }").arg(COLOR_TEXT_META));
    barLayout->addWidget(d->countLabel);

    barLayout->addStretch();

    // 排序标签
    QLabel* sortLabel = new QLabel(QStringLiteral("排序:"));
    sortLabel->setStyleSheet(QString("QLabel { color: %1; font-size: 12px; }").arg(COLOR_TEXT_META));
    barLayout->addWidget(sortLabel);

    // 收益率排序按钮
    d->returnSortBtn = new SortButton(QStringLiteral("收益率"), QStringLiteral("return"));
    d->returnSortBtn->setActive(true);
    connect(d->returnSortBtn, &SortButton::sortClicked, this, [this](const QString& sortKey, bool ascending) {
        onSortChanged(sortKey, ascending);
    });
    barLayout->addWidget(d->returnSortBtn);

    // 胜率排序按钮
    d->winRateSortBtn = new SortButton(QStringLiteral("胜率"), QStringLiteral("winrate"));
    connect(d->winRateSortBtn, &SortButton::sortClicked, this, [this](const QString& sortKey, bool ascending) {
        onSortChanged(sortKey, ascending);
    });
    barLayout->addWidget(d->winRateSortBtn);

    // 订阅数排序按钮
    d->followersSortBtn = new SortButton(QStringLiteral("订阅数"), QStringLiteral("followers"));
    connect(d->followersSortBtn, &SortButton::sortClicked, this, [this](const QString& sortKey, bool ascending) {
        onSortChanged(sortKey, ascending);
    });
    barLayout->addWidget(d->followersSortBtn);

    barLayout->addSpacing(8);

    // 刷新按钮
    QPushButton* refreshBtn = new QPushButton(QStringLiteral("刷新"));
    refreshBtn->setFixedHeight(28);
    refreshBtn->setFixedWidth(60);
    ButtonStyles::setRefresh(refreshBtn);
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
    contentLayout->setContentsMargins(20, 20, 20, 20);
    contentLayout->setSpacing(0);

    // 网格容器
    auto* gridContainer = new QWidget();
    d->cardsGrid = new QGridLayout(gridContainer);
    d->cardsGrid->setContentsMargins(0, 0, 0, 0);
    d->cardsGrid->setSpacing(16);
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
        sig.subscribed = d->isSubscribed(sig.id);
    }

    updateCards();
}

QVector<SignalCardData> SignalCenterPage::getFilteredSignals()
{
    QVector<SignalCardData> result;

    if (d->currentCategory == QStringLiteral("我的订阅")) {
        result = d->subscribedSignals;
    } else if (d->currentCategory == QStringLiteral("推荐信号")) {
        for (const auto& sig : d->allSignals) {
            if (!d->isSubscribed(sig.id)) {
                result.append(sig);
            }
        }
    } else if (d->currentCategory == QStringLiteral("排行榜")) {
        result = d->allSignals;
    } else if (d->currentCategory == QStringLiteral("最新上线")) {
        for (int i = d->allSignals.size() - 1; i >= std::max(0, (int)d->allSignals.size() - 6); --i) {
            result.append(d->allSignals[i]);
        }
    }

    return result;
}

void SignalCenterPage::sortSignals(QVector<SignalCardData>& signalList)
{
    std::sort(signalList.begin(), signalList.end(), [this](const SignalCardData& a, const SignalCardData& b) {
        bool result = false;
        if (d->currentSort == QStringLiteral("return")) {
            result = a.returnRate > b.returnRate;
        } else if (d->currentSort == QStringLiteral("winrate")) {
            result = a.winRate > b.winRate;
        } else if (d->currentSort == QStringLiteral("followers")) {
            result = a.followers > b.followers;
        }
        return d->sortAscending ? !result : result;
    });
}

void SignalCenterPage::onSortChanged(const QString& sortKey, bool ascending)
{
    d->currentSort = sortKey;
    d->sortAscending = ascending;

    // 更新所有按钮状态
    if (sortKey == QStringLiteral("return")) {
        d->returnSortBtn->setActive(true);
        d->winRateSortBtn->setActive(false);
        d->followersSortBtn->setActive(false);
    } else if (sortKey == QStringLiteral("winrate")) {
        d->returnSortBtn->setActive(false);
        d->winRateSortBtn->setActive(true);
        d->followersSortBtn->setActive(false);
    } else if (sortKey == QStringLiteral("followers")) {
        d->returnSortBtn->setActive(false);
        d->winRateSortBtn->setActive(false);
        d->followersSortBtn->setActive(true);
    }

    updateCards();
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
    auto signalList = getFilteredSignals();

    // 排序
    sortSignals(signalList);

    // 更新数量标签
    d->countLabel->setText(QStringLiteral("共 %1 个信号").arg(signalList.size()));

    // 添加新卡片到网格
    int row = 0;
    int col = 0;
    bool showSubscribed = (d->currentCategory == QStringLiteral("我的订阅"));

    for (const auto& signal : signalList) {
        auto* card = new SignalMiniCard(signal, showSubscribed);
        connect(card, &SignalMiniCard::clicked, this, &SignalCenterPage::onCardClicked);
        connect(card, &SignalMiniCard::subscribeClicked, this, [this](const SignalCardData& data) {
            onSubscribeClicked(data);
        });
        connect(card, &SignalMiniCard::unsubscribeClicked, this, [this](const SignalCardData& data) {
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

void SignalCenterPage::onSubscribeClicked(const SignalCardData& data)
{
    d->saveSubscription(data);

    SignalCardData subscribedData = data;
    subscribedData.subscribed = true;
    d->subscribedSignals.append(subscribedData);

    for (auto& sig : d->allSignals) {
        if (sig.id == data.id) {
            sig.subscribed = true;
            break;
        }
    }

    updateCards();

    QMessageBox::information(this, QStringLiteral("订阅成功"),
        QStringLiteral("已成功订阅信号：%1").arg(data.name));

    LOG_INFO(QString("Subscribed: %1").arg(data.name));
}

void SignalCenterPage::onUnsubscribeClicked(const SignalCardData& data)
{
    d->removeSubscription(data.id);

    for (int i = 0; i < d->subscribedSignals.size(); ++i) {
        if (d->subscribedSignals[i].id == data.id) {
            d->subscribedSignals.removeAt(i);
            break;
        }
    }

    for (auto& sig : d->allSignals) {
        if (sig.id == data.id) {
            sig.subscribed = false;
            break;
        }
    }

    updateCards();

    LOG_INFO(QString("Unsubscribed: %1").arg(data.name));
}

} // namespace WealthPilot

#include "SignalCenterPage.moc"
