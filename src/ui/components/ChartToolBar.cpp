/**
 * @file ChartToolBar.cpp
 * @brief 图表工具栏组件实现
 */

#include "ChartToolBar.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QComboBox>
#include <QToolButton>
#include <QButtonGroup>
#include <QMenu>
#include <QAction>
#include <QLabel>
#include <QFrame>
#include <QStyle>

// ========== PIMPL 实现 ==========

struct ChartToolBar::Impl {
    // 周期选择
    QComboBox* periodCombo = nullptr;

    // 工具按钮
    QToolButton* adjustmentBtn = nullptr;
    QToolButton* indicatorBtn = nullptr;
    QToolButton* drawToolBtn = nullptr;
    QToolButton* chartTypeBtn = nullptr;

    // 菜单
    QMenu* adjustmentMenu = nullptr;
    QMenu* indicatorMenu = nullptr;
    QMenu* drawToolMenu = nullptr;
    QMenu* chartTypeMenu = nullptr;

    // 当前状态
    KLinePeriod currentPeriod = KLinePeriod::Minute15;
    AdjustmentType currentAdjustment = AdjustmentType::None;

    // 指标状态
    QMap<QString, bool> indicatorStates = {
        {"MA5", true},
        {"MA10", true},
        {"MA20", true},
        {"MA30", false},
        {"MA60", false},
        {"MACD", false},
        {"RSI", false},
        {"KDJ", false},
        {"BOLL", false},
        {"VOL", true}
    };
};

// ========== 构造与析构 ==========

ChartToolBar::ChartToolBar(QWidget *parent)
    : QWidget(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
}

ChartToolBar::~ChartToolBar() = default;

// ========== 状态设置 ==========

void ChartToolBar::setCurrentPeriod(KLinePeriod period)
{
    if (d->currentPeriod != period) {
        d->currentPeriod = period;
        if (d->periodCombo) {
            d->periodCombo->blockSignals(true);
            d->periodCombo->setCurrentIndex(static_cast<int>(period));
            d->periodCombo->blockSignals(false);
        }
    }
}

KLinePeriod ChartToolBar::currentPeriod() const
{
    return d->currentPeriod;
}

void ChartToolBar::setCurrentAdjustment(AdjustmentType type)
{
    d->currentAdjustment = type;
}

AdjustmentType ChartToolBar::currentAdjustment() const
{
    return d->currentAdjustment;
}

void ChartToolBar::setIndicatorEnabled(const QString& indicator, bool enabled)
{
    d->indicatorStates[indicator] = enabled;
    updateIndicatorMenuState();
}

bool ChartToolBar::isIndicatorEnabled(const QString& indicator) const
{
    return d->indicatorStates.value(indicator, false);
}

// ========== 私有槽函数 ==========

void ChartToolBar::onPeriodComboChanged(int index)
{
    d->currentPeriod = static_cast<KLinePeriod>(index);
    emit periodChanged(d->currentPeriod);
}

void ChartToolBar::onAdjustmentMenuTriggered(QAction* action)
{
    QString text = action->text();

    if (text == QStringLiteral("不复权")) {
        d->currentAdjustment = AdjustmentType::None;
    } else if (text == QStringLiteral("前复权")) {
        d->currentAdjustment = AdjustmentType::Front;
    } else if (text == QStringLiteral("后复权")) {
        d->currentAdjustment = AdjustmentType::Back;
    }

    emit adjustmentChanged(d->currentAdjustment);
}

void ChartToolBar::onIndicatorMenuTriggered(QAction* action)
{
    QString indicator = action->text();
    bool enabled = !action->isChecked();
    action->setChecked(enabled);

    d->indicatorStates[indicator] = enabled;
    emit indicatorToggled(indicator, enabled);
}

void ChartToolBar::onDrawToolMenuTriggered(QAction* action)
{
    QString tool = action->text();
    emit drawToolSelected(tool);
}

void ChartToolBar::onChartTypeMenuTriggered(QAction* action)
{
    QString type = action->data().toString();
    emit chartTypeChanged(type);
}

// ========== 私有方法 ==========

void ChartToolBar::setupUI()
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 5, 10, 5);
    layout->setSpacing(8);

    // 周期选择
    setupPeriodSelector();
    layout->addWidget(d->periodCombo);

    // 分隔线
    layout->addWidget(createSeparator());

    // 工具按钮
    setupToolButtons();
    layout->addWidget(d->adjustmentBtn);
    layout->addWidget(createSeparator());
    layout->addWidget(d->indicatorBtn);
    layout->addWidget(createSeparator());
    layout->addWidget(d->drawToolBtn);
    layout->addWidget(createSeparator());
    layout->addWidget(d->chartTypeBtn);

    // 弹性空间
    layout->addStretch();

    // 设置菜单
    setupMenus();
}

void ChartToolBar::setupPeriodSelector()
{
    d->periodCombo = new QComboBox(this);
    d->periodCombo->addItem(QStringLiteral("分时"));
    d->periodCombo->addItem(QStringLiteral("1分"));
    d->periodCombo->addItem(QStringLiteral("5分"));
    d->periodCombo->addItem(QStringLiteral("15分"));
    d->periodCombo->addItem(QStringLiteral("30分"));
    d->periodCombo->addItem(QStringLiteral("60分"));
    d->periodCombo->addItem(QStringLiteral("日线"));
    d->periodCombo->addItem(QStringLiteral("周线"));
    d->periodCombo->addItem(QStringLiteral("月线"));

    // 默认选择15分钟
    d->periodCombo->setCurrentIndex(static_cast<int>(KLinePeriod::Minute15));

    // 设置固定宽度
    d->periodCombo->setFixedWidth(80);

    // 连接信号
    connect(d->periodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ChartToolBar::onPeriodComboChanged);
}

void ChartToolBar::setupToolButtons()
{
    // 复权按钮
    d->adjustmentBtn = new QToolButton(this);
    d->adjustmentBtn->setText(QStringLiteral("复权"));
    d->adjustmentBtn->setPopupMode(QToolButton::InstantPopup);
    d->adjustmentBtn->setToolButtonStyle(Qt::ToolButtonTextOnly);
    d->adjustmentBtn->setFixedWidth(60);

    // 指标按钮
    d->indicatorBtn = new QToolButton(this);
    d->indicatorBtn->setText(QStringLiteral("指标"));
    d->indicatorBtn->setPopupMode(QToolButton::InstantPopup);
    d->indicatorBtn->setToolButtonStyle(Qt::ToolButtonTextOnly);
    d->indicatorBtn->setFixedWidth(60);

    // 画线按钮
    d->drawToolBtn = new QToolButton(this);
    d->drawToolBtn->setText(QStringLiteral("画线"));
    d->drawToolBtn->setPopupMode(QToolButton::InstantPopup);
    d->drawToolBtn->setToolButtonStyle(Qt::ToolButtonTextOnly);
    d->drawToolBtn->setFixedWidth(60);

    // 图表类型按钮
    d->chartTypeBtn = new QToolButton(this);
    d->chartTypeBtn->setText(QStringLiteral("K线"));
    d->chartTypeBtn->setPopupMode(QToolButton::InstantPopup);
    d->chartTypeBtn->setToolButtonStyle(Qt::ToolButtonTextOnly);
    d->chartTypeBtn->setFixedWidth(60);
}

void ChartToolBar::setupMenus()
{
    // 复权菜单
    d->adjustmentMenu = new QMenu(this);
    d->adjustmentMenu->addAction(QStringLiteral("不复权"));
    d->adjustmentMenu->addAction(QStringLiteral("前复权"));
    d->adjustmentMenu->addAction(QStringLiteral("后复权"));
    d->adjustmentBtn->setMenu(d->adjustmentMenu);

    connect(d->adjustmentMenu, &QMenu::triggered,
            this, &ChartToolBar::onAdjustmentMenuTriggered);

    // 指标菜单
    d->indicatorMenu = new QMenu(this);
    for (auto it = d->indicatorStates.begin(); it != d->indicatorStates.end(); ++it) {
        QAction* action = d->indicatorMenu->addAction(it.key());
        action->setCheckable(true);
        action->setChecked(it.value());
    }
    d->indicatorBtn->setMenu(d->indicatorMenu);

    connect(d->indicatorMenu, &QMenu::triggered,
            this, &ChartToolBar::onIndicatorMenuTriggered);

    // 画线菜单
    d->drawToolMenu = new QMenu(this);
    d->drawToolMenu->addAction(QStringLiteral("趋势线"));
    d->drawToolMenu->addAction(QStringLiteral("水平线"));
    d->drawToolMenu->addAction(QStringLiteral("平行线"));
    d->drawToolMenu->addAction(QStringLiteral("黄金分割"));
    d->drawToolMenu->addSeparator();
    d->drawToolMenu->addAction(QStringLiteral("清除全部"));
    d->drawToolBtn->setMenu(d->drawToolMenu);

    connect(d->drawToolMenu, &QMenu::triggered,
            this, &ChartToolBar::onDrawToolMenuTriggered);

    // 图表类型菜单
    d->chartTypeMenu = new QMenu(this);
    QAction* klineAction = d->chartTypeMenu->addAction(QStringLiteral("K线图"));
    klineAction->setData("kline");
    QAction* timelineAction = d->chartTypeMenu->addAction(QStringLiteral("分时图"));
    timelineAction->setData("timeline");
    d->chartTypeBtn->setMenu(d->chartTypeMenu);

    connect(d->chartTypeMenu, &QMenu::triggered,
            this, &ChartToolBar::onChartTypeMenuTriggered);
}

QFrame* ChartToolBar::createSeparator()
{
    QFrame* separator = new QFrame(this);
    separator->setFrameShape(QFrame::VLine);
    separator->setFrameShadow(QFrame::Sunken);
    separator->setFixedWidth(2);
    return separator;
}

void ChartToolBar::updateIndicatorMenuState()
{
    if (!d->indicatorMenu) {
        return;
    }

    for (QAction* action : d->indicatorMenu->actions()) {
        QString indicator = action->text();
        action->setChecked(d->indicatorStates.value(indicator, false));
    }
}
