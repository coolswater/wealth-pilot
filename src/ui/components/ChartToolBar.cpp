/**
 * @file ChartToolBar.cpp
 * @brief 图表工具栏组件实现
 */

#include "ChartToolBar.h"
#include "ChartStyles.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QComboBox>
#include <QToolButton>
#include <QButtonGroup>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QLabel>
#include <QFrame>
#include <QStyle>

// ========== PIMPL 实现 ==========

struct ChartToolBar::Impl {
    // 周期选择
    QComboBox* periodCombo = nullptr;

    // 工具按钮
    QToolButton* adjustmentBtn = nullptr;
    QToolButton* mainIndicatorBtn = nullptr;    // 主图指标按钮
    QToolButton* subIndicatorBtn = nullptr;     // 副图指标按钮
    QToolButton* drawToolBtn = nullptr;
    QToolButton* chartTypeBtn = nullptr;

    // 菜单
    QMenu* adjustmentMenu = nullptr;
    QMenu* mainIndicatorMenu = nullptr;         // 主图指标菜单
    QMenu* subIndicatorMenu = nullptr;          // 副图指标菜单
    QMenu* drawToolMenu = nullptr;
    QMenu* chartTypeMenu = nullptr;

    // 当前状态
    KLinePeriod currentPeriod = KLinePeriod::Minute15;
    AdjustmentType currentAdjustment = AdjustmentType::None;

    // 主图指标状态（单选）
    QString currentMainIndicator = "MA";  // 当前主图指标
    
    // 副图指标状态（单选）
    QString currentSubIndicator = "None";  // 当前副图指标
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

void ChartToolBar::setCurrentMainIndicator(const QString& indicator)
{
    d->currentMainIndicator = indicator;
    // 更新菜单选中状态
    if (d->mainIndicatorMenu) {
        for (QAction* action : d->mainIndicatorMenu->actions()) {
            action->setChecked(action->data().toString() == indicator);
        }
    }
}

QString ChartToolBar::currentMainIndicator() const
{
    return d->currentMainIndicator;
}

void ChartToolBar::setCurrentSubIndicator(const QString& indicator)
{
    d->currentSubIndicator = indicator;
    // 更新菜单选中状态
    if (d->subIndicatorMenu) {
        for (QAction* action : d->subIndicatorMenu->actions()) {
            action->setChecked(action->data().toString() == indicator);
        }
    }
}

QString ChartToolBar::currentSubIndicator() const
{
    return d->currentSubIndicator;
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

void ChartToolBar::onMainIndicatorMenuTriggered(QAction* action)
{
    QString indicator = action->data().toString();
    d->currentMainIndicator = indicator;
    
    // 更新菜单选中状态（单选）
    for (QAction* a : d->mainIndicatorMenu->actions()) {
        a->setChecked(a == action);
    }
    
    emit mainIndicatorChanged(indicator);
}

void ChartToolBar::onSubIndicatorMenuTriggered(QAction* action)
{
    QString indicator = action->data().toString();
    d->currentSubIndicator = indicator;
    
    // 更新菜单选中状态（单选）
    for (QAction* a : d->subIndicatorMenu->actions()) {
        a->setChecked(a == action);
    }
    
    emit subIndicatorChanged(indicator);
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
    layout->setContentsMargins(16, 8, 16, 8);
    layout->setSpacing(12);

    // 周期选择
    setupPeriodSelector();
    layout->addWidget(d->periodCombo);

    // 分隔线
    layout->addWidget(createSeparator());

    // 工具按钮
    setupToolButtons();
    layout->addWidget(d->adjustmentBtn);
    layout->addWidget(createSeparator());
    layout->addWidget(d->mainIndicatorBtn);
    layout->addWidget(createSeparator());
    layout->addWidget(d->subIndicatorBtn);
    layout->addWidget(createSeparator());
    layout->addWidget(d->drawToolBtn);
    layout->addWidget(createSeparator());
    layout->addWidget(d->chartTypeBtn);

    // 弹性空间
    layout->addStretch();

    // 设置菜单
    setupMenus();
    
    // 应用统一样式
    setStyleSheet(ChartStyles::StyleSheets::chartToolBarStyle());
    setFixedHeight(ChartStyles::Sizes::ToolBarHeight);
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

    // 主图指标按钮
    d->mainIndicatorBtn = new QToolButton(this);
    d->mainIndicatorBtn->setText(QStringLiteral("主图"));
    d->mainIndicatorBtn->setPopupMode(QToolButton::InstantPopup);
    d->mainIndicatorBtn->setToolButtonStyle(Qt::ToolButtonTextOnly);
    d->mainIndicatorBtn->setFixedWidth(60);

    // 副图指标按钮
    d->subIndicatorBtn = new QToolButton(this);
    d->subIndicatorBtn->setText(QStringLiteral("副图"));
    d->subIndicatorBtn->setPopupMode(QToolButton::InstantPopup);
    d->subIndicatorBtn->setToolButtonStyle(Qt::ToolButtonTextOnly);
    d->subIndicatorBtn->setFixedWidth(60);

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

    // 主图指标菜单（单选）
    d->mainIndicatorMenu = new QMenu(this);
    QActionGroup* mainGroup = new QActionGroup(this);
    mainGroup->setExclusive(true);
    
    QStringList mainIndicators = {"MA", "EMA", "BOLL"};
    for (const QString& indicator : mainIndicators) {
        QAction* action = d->mainIndicatorMenu->addAction(indicator);
        action->setCheckable(true);
        action->setData(indicator);
        action->setChecked(indicator == d->currentMainIndicator);
        mainGroup->addAction(action);
    }
    d->mainIndicatorBtn->setMenu(d->mainIndicatorMenu);

    connect(d->mainIndicatorMenu, &QMenu::triggered,
            this, &ChartToolBar::onMainIndicatorMenuTriggered);

    // 副图指标菜单（单选）
    d->subIndicatorMenu = new QMenu(this);
    QActionGroup* subGroup = new QActionGroup(this);
    subGroup->setExclusive(true);
    
    QStringList subIndicators = {"None", "MACD", "KDJ", "RSI"};
    for (const QString& indicator : subIndicators) {
        QAction* action = d->subIndicatorMenu->addAction(indicator);
        action->setCheckable(true);
        action->setData(indicator);
        action->setChecked(indicator == d->currentSubIndicator);
        subGroup->addAction(action);
    }
    d->subIndicatorBtn->setMenu(d->subIndicatorMenu);

    connect(d->subIndicatorMenu, &QMenu::triggered,
            this, &ChartToolBar::onSubIndicatorMenuTriggered);

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
