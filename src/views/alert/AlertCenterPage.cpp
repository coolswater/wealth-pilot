/**
 * @file AlertCenterPage.cpp
 * @brief 预警中心页面实现 - 价格预警与消息通知
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "AlertCenterPage.h"
#include "core/config/Tokens.h"
#include "utils/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QHeaderView>
#include <QSplitter>
#include <QGroupBox>
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QUuid>

// ========== PIMPL实现 ==========

struct AlertCenterPage::Impl {
    // 工具栏
    QPushButton* addBtn = nullptr;
    QPushButton* deleteBtn = nullptr;
    QPushButton* toggleBtn = nullptr;
    QPushButton* refreshBtn = nullptr;
    QPushButton* clearHistoryBtn = nullptr;
    
    // 预警规则列表
    QTableWidget* alertTable = nullptr;
    
    // 预警历史
    QTableWidget* historyTable = nullptr;
    
    // 数据
    QVector<AlertRule> alertRules;
    QVector<AlertRecord> alertHistory;
    
    // 当前选中
    QString currentAlertId;
};

// ========== 构造与析构 ==========

AlertCenterPage::AlertCenterPage(QWidget *parent)
    : BasePage(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
}

AlertCenterPage::~AlertCenterPage() = default;

// ========== 初始化 ==========

void AlertCenterPage::initializePage()
{
    loadAlertRules();
    loadAlertHistory();
}

void AlertCenterPage::refresh()
{
    loadAlertRules();
    loadAlertHistory();
}

// ========== UI初始化 ==========

void AlertCenterPage::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    initToolBar();
    
    // 主内容区域
    auto* splitter = new QSplitter(Qt::Vertical, this);
    splitter->setStyleSheet(QString("QSplitter::handle { background: %1; height: 1px; }").arg(Tokens::Colors::Border));
    
    // 上部：预警规则
    auto* topWidget = new QWidget();
    topWidget->setStyleSheet(QString("QWidget { background: %1; }").arg(Tokens::Colors::BgSurface));
    auto* topLayout = new QVBoxLayout(topWidget);
    topLayout->setContentsMargins(8, 8, 8, 8);
    
    auto* ruleGroup = new QGroupBox(QStringLiteral("预警规则"));
    ruleGroup->setStyleSheet(QString(R"(
        QGroupBox {
            color: %1;
            font-size: 13px;
            font-weight: bold;
            border: 1px solid %2;
            border-radius: 4px;
            margin-top: 8px;
        }
        QGroupBox::title { subcontrol-origin: margin; left: 10px; }
    )").arg(Tokens::Colors::TextPrimary, Tokens::Colors::Border));
    
    auto* ruleLayout = new QVBoxLayout(ruleGroup);
    initAlertList();
    ruleLayout->addWidget(d->alertTable);
    topLayout->addWidget(ruleGroup);
    
    splitter->addWidget(topWidget);
    
    // 下部：预警历史
    auto* bottomWidget = new QWidget();
    bottomWidget->setStyleSheet(QString("QWidget { background: %1; }").arg(Tokens::Colors::BgSurface));
    auto* bottomLayout = new QVBoxLayout(bottomWidget);
    bottomLayout->setContentsMargins(8, 8, 8, 8);
    
    auto* historyGroup = new QGroupBox(QStringLiteral("预警历史"));
    historyGroup->setStyleSheet(ruleGroup->styleSheet());
    
    auto* historyLayout = new QVBoxLayout(historyGroup);
    initHistoryList();
    historyLayout->addWidget(d->historyTable);
    bottomLayout->addWidget(historyGroup);
    
    splitter->addWidget(bottomWidget);
    splitter->setSizes({300, 300});
    
    mainLayout->addWidget(splitter, 1);
    
    initConnections();
}

void AlertCenterPage::initToolBar()
{
    auto* toolbar = new QWidget(this);
    toolbar->setFixedHeight(40);
    toolbar->setStyleSheet(QString("QWidget { background: %1; }").arg(Tokens::Colors::BgSurface));
    
    auto* layout = new QHBoxLayout(toolbar);
    layout->setContentsMargins(12, 6, 12, 6);
    layout->setSpacing(8);
    
    // 添加预警按钮
    d->addBtn = new QPushButton(QStringLiteral("添加预警"));
    d->addBtn->setFixedSize(80, 26);
    d->addBtn->setStyleSheet(QString(R"(
        QPushButton {
            background: %1;
            color: %2;
            border: none;
            font-size: 12px;
            border-radius: 4px;
        }
        QPushButton:hover { background: %3; }
    )").arg(Tokens::Colors::Success, Tokens::Colors::TextPrimary, Tokens::Colors::SuccessLight));
    layout->addWidget(d->addBtn);
    
    // 删除按钮
    d->deleteBtn = new QPushButton(QStringLiteral("删除"));
    d->deleteBtn->setFixedSize(60, 26);
    d->deleteBtn->setStyleSheet(QString(R"(
        QPushButton {
            background: %1;
            color: %2;
            border: none;
            font-size: 12px;
            border-radius: 4px;
        }
        QPushButton:hover { background: %3; }
    )").arg(Tokens::Colors::Danger, Tokens::Colors::TextPrimary, Tokens::Colors::DangerLight));
    layout->addWidget(d->deleteBtn);
    
    // 启用/禁用按钮
    d->toggleBtn = new QPushButton(QStringLiteral("启用/禁用"));
    d->toggleBtn->setFixedSize(80, 26);
    d->toggleBtn->setStyleSheet(QString(R"(
        QPushButton {
            background: %1;
            color: %2;
            border: none;
            font-size: 12px;
            border-radius: 4px;
        }
        QPushButton:hover { background: %3; }
    )").arg(Tokens::Colors::BgElevated, Tokens::Colors::TextPrimary, Tokens::Colors::BgHover));
    layout->addWidget(d->toggleBtn);
    
    layout->addStretch();
    
    // 刷新按钮
    d->refreshBtn = new QPushButton(QStringLiteral("刷新"));
    d->refreshBtn->setFixedSize(60, 26);
    d->refreshBtn->setStyleSheet(d->toggleBtn->styleSheet());
    layout->addWidget(d->refreshBtn);
    
    // 清空历史按钮
    d->clearHistoryBtn = new QPushButton(QStringLiteral("清空历史"));
    d->clearHistoryBtn->setFixedSize(70, 26);
    d->clearHistoryBtn->setStyleSheet(d->toggleBtn->styleSheet());
    layout->addWidget(d->clearHistoryBtn);
    
    auto* mainLayout = qobject_cast<QVBoxLayout*>(this->layout());
    mainLayout->addWidget(toolbar);
}

void AlertCenterPage::initAlertList()
{
    d->alertTable = new QTableWidget();
    d->alertTable->setColumnCount(7);
    d->alertTable->setHorizontalHeaderLabels({
        QStringLiteral("标的代码"),
        QStringLiteral("标的名称"),
        QStringLiteral("预警类型"),
        QStringLiteral("阈值"),
        QStringLiteral("状态"),
        QStringLiteral("创建时间"),
        QStringLiteral("触发时间")
    });
    
    d->alertTable->setStyleSheet(QString(R"(
        QTableWidget {
            background: %1;
            color: %2;
            border: none;
            gridline-color: %3;
            font-size: 12px;
        }
        QTableWidget::item:selected { background: %4; }
        QHeaderView::section {
            background: %5;
            color: %6;
            border: none;
            padding: 6px;
            font-size: 11px;
        }
    )").arg(Tokens::Colors::BgSurface, Tokens::Colors::TextPrimary, Tokens::Colors::Border, Tokens::Colors::BgElevated, Tokens::Colors::BgBase, Tokens::Colors::TextTertiary));
    
    d->alertTable->horizontalHeader()->setStretchLastSection(true);
    d->alertTable->verticalHeader()->setVisible(false);
    d->alertTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    d->alertTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

void AlertCenterPage::initHistoryList()
{
    d->historyTable = new QTableWidget();
    d->historyTable->setColumnCount(6);
    d->historyTable->setHorizontalHeaderLabels({
        QStringLiteral("标的代码"),
        QStringLiteral("标的名称"),
        QStringLiteral("预警类型"),
        QStringLiteral("阈值"),
        QStringLiteral("实际值"),
        QStringLiteral("触发时间")
    });
    
    d->historyTable->setStyleSheet(d->alertTable->styleSheet());
    d->historyTable->horizontalHeader()->setStretchLastSection(true);
    d->historyTable->verticalHeader()->setVisible(false);
    d->historyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

void AlertCenterPage::initConnections()
{
    connect(d->addBtn, &QPushButton::clicked, this, &AlertCenterPage::onAddAlert);
    connect(d->deleteBtn, &QPushButton::clicked, this, &AlertCenterPage::onDeleteAlert);
    connect(d->toggleBtn, &QPushButton::clicked, this, &AlertCenterPage::onToggleAlert);
    connect(d->refreshBtn, &QPushButton::clicked, this, &AlertCenterPage::onRefreshData);
    connect(d->clearHistoryBtn, &QPushButton::clicked, this, &AlertCenterPage::onClearHistory);
    connect(d->alertTable, &QTableWidget::cellClicked, this, &AlertCenterPage::onAlertListClicked);
}

// ========== 数据加载 ==========

void AlertCenterPage::loadAlertRules()
{
    d->alertRules.clear();
    
    // 模拟数据
    AlertRule r1; r1.ruleId = QUuid::createUuid().toString(); r1.instrumentId = "600519"; r1.type = AlertType::PriceAbove; r1.threshold = 1800.0; r1.isActive = true; r1.createTime = QDateTime::currentDateTime();
    d->alertRules.append(r1);
    
    AlertRule r2; r2.ruleId = QUuid::createUuid().toString(); r2.instrumentId = "000858"; r2.type = AlertType::ChangePercentBelow; r2.threshold = -5.0; r2.isActive = true; r2.createTime = QDateTime::currentDateTime();
    d->alertRules.append(r2);
    
    AlertRule r3; r3.ruleId = QUuid::createUuid().toString(); r3.instrumentId = "000001"; r3.type = AlertType::MarginWarning; r3.threshold = 10000000; r3.isActive = false; r3.isTriggered = true; r3.createTime = QDateTime::currentDateTime().addDays(-1); r3.triggerTime = QDateTime::currentDateTime();
    d->alertRules.append(r3);
    
    d->alertTable->setRowCount(d->alertRules.size());
    
    for (int i = 0; i < d->alertRules.size(); ++i) {
        const auto& rule = d->alertRules[i];
        
        d->alertTable->setItem(i, 0, new QTableWidgetItem(rule.instrumentId));
        d->alertTable->setItem(i, 1, new QTableWidgetItem(rule.instrumentId));
        d->alertTable->setItem(i, 2, new QTableWidgetItem(formatAlertType(rule.type)));
        d->alertTable->setItem(i, 3, new QTableWidgetItem(QString::number(rule.threshold)));
        
        QString statusText = rule.isActive ? QStringLiteral("激活中") : (rule.isTriggered ? QStringLiteral("已触发") : QStringLiteral("已禁用"));
        auto* statusItem = new QTableWidgetItem(statusText);
        if (rule.isActive) {
            statusItem->setForeground(QColor(Tokens::Colors::Success));
        } else if (rule.isTriggered) {
            statusItem->setForeground(QColor(Tokens::Colors::Danger));
        }
        d->alertTable->setItem(i, 4, statusItem);
        
        d->alertTable->setItem(i, 5, new QTableWidgetItem(rule.createTime.toString("yyyy-MM-dd hh:mm")));
        d->alertTable->setItem(i, 6, new QTableWidgetItem(rule.triggerTime.isValid() ? rule.triggerTime.toString("yyyy-MM-dd hh:mm") : "--"));
    }
}

void AlertCenterPage::loadAlertHistory()
{
    d->alertHistory.clear();
    
    // 模拟数据
    AlertRecord rec1;
    rec1.symbol = "600519"; rec1.name = QStringLiteral("贵州茅台"); rec1.type = AlertType::PriceAbove;
    rec1.threshold = 1800.0; rec1.actualValue = 1805.5; rec1.triggerTime = QDateTime::currentDateTime().addSecs(-2*3600);
    rec1.message = QStringLiteral("价格突破1800元");
    d->alertHistory.append(rec1);
    
    AlertRecord rec2;
    rec2.symbol = "000858"; rec2.name = QStringLiteral("五粮液"); rec2.type = AlertType::ChangePercentBelow;
    rec2.threshold = -5.0; rec2.actualValue = -6.2; rec2.triggerTime = QDateTime::currentDateTime().addDays(-1);
    rec2.message = QStringLiteral("跌幅超过5%");
    d->alertHistory.append(rec2);
    
    d->historyTable->setRowCount(d->alertHistory.size());
    
    for (int i = 0; i < d->alertHistory.size(); ++i) {
        const auto& record = d->alertHistory[i];
        
        d->historyTable->setItem(i, 0, new QTableWidgetItem(record.symbol));
        d->historyTable->setItem(i, 1, new QTableWidgetItem(record.name));
        d->historyTable->setItem(i, 2, new QTableWidgetItem(formatAlertType(record.type)));
        d->historyTable->setItem(i, 3, new QTableWidgetItem(QString::number(record.threshold)));
        d->historyTable->setItem(i, 4, new QTableWidgetItem(QString::number(record.actualValue)));
        d->historyTable->setItem(i, 5, new QTableWidgetItem(record.triggerTime.toString("yyyy-MM-dd hh:mm")));
    }
}

void AlertCenterPage::addAlertRule(const AlertRule& rule)
{
    d->alertRules.append(rule);
    loadAlertRules();
    LOG_INFO(QString("Alert rule added: %1 %2").arg(rule.instrumentId, formatAlertType(rule.type)));
}

void AlertCenterPage::removeAlertRule(const QString& ruleId)
{
    for (int i = 0; i < d->alertRules.size(); ++i) {
        if (d->alertRules[i].ruleId == ruleId) {
            d->alertRules.removeAt(i);
            break;
        }
    }
    loadAlertRules();
}

void AlertCenterPage::toggleAlertRule(const QString& ruleId)
{
    for (auto& rule : d->alertRules) {
        if (rule.ruleId == ruleId) {
            rule.isActive = !rule.isActive;
            break;
        }
    }
    loadAlertRules();
}

// ========== 槽函数 ==========

void AlertCenterPage::onAddAlert()
{
    auto* dialog = new QDialog(this);
    dialog->setWindowTitle(QStringLiteral("添加预警"));
    dialog->setFixedSize(350, 250);
    dialog->setStyleSheet(QString("QDialog { background: %1; } QLabel { color: %2; }").arg(Tokens::Colors::BgSurface, Tokens::Colors::TextPrimary));
    
    auto* layout = new QFormLayout(dialog);
    
    auto* symbolEdit = new QLineEdit();
    symbolEdit->setStyleSheet(QString("background: %1; color: %2; padding: 4px;").arg(Tokens::Colors::BgElevated, Tokens::Colors::TextPrimary));
    layout->addRow(QStringLiteral("标的代码:"), symbolEdit);
    
    auto* typeCombo = new QComboBox();
    typeCombo->addItems({QStringLiteral("价格高于"), QStringLiteral("价格低于"), QStringLiteral("涨幅高于"), QStringLiteral("跌幅高于")});
    typeCombo->setStyleSheet(QString("background: %1; color: %2; padding: 4px;").arg(Tokens::Colors::BgElevated, Tokens::Colors::TextPrimary));
    layout->addRow(QStringLiteral("预警类型:"), typeCombo);
    
    auto* thresholdSpin = new QDoubleSpinBox();
    thresholdSpin->setRange(-100, 100000);
    thresholdSpin->setStyleSheet(QString("background: %1; color: %2; padding: 4px;").arg(Tokens::Colors::BgElevated, Tokens::Colors::TextPrimary));
    layout->addRow(QStringLiteral("阈值:"), thresholdSpin);
    
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttonBox->setStyleSheet(QString("QPushButton { background: %1; color: %2; padding: 6px 16px; }").arg(Tokens::Colors::BgElevated, Tokens::Colors::TextPrimary));
    layout->addRow(buttonBox);
    
    connect(buttonBox, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    
    if (dialog->exec() == QDialog::Accepted) {
        AlertRule rule;
        rule.ruleId = QUuid::createUuid().toString();
        rule.instrumentId = symbolEdit->text();
        rule.type = static_cast<AlertType>(typeCombo->currentIndex());
        rule.threshold = thresholdSpin->value();
        rule.isActive = true;
        rule.createTime = QDateTime::currentDateTime();
        addAlertRule(rule);
    }
    
    dialog->deleteLater();
}

void AlertCenterPage::onDeleteAlert()
{
    if (d->currentAlertId.isEmpty()) {
        return;
    }
    removeAlertRule(d->currentAlertId);
}

void AlertCenterPage::onToggleAlert()
{
    if (d->currentAlertId.isEmpty()) {
        return;
    }
    toggleAlertRule(d->currentAlertId);
}

void AlertCenterPage::onAlertListClicked(int row, int column)
{
    Q_UNUSED(column)
    if (row >= 0 && row < d->alertRules.size()) {
        d->currentAlertId = d->alertRules[row].ruleId;
    }
}

void AlertCenterPage::onRefreshData()
{
    refresh();
}

void AlertCenterPage::onClearHistory()
{
    d->alertHistory.clear();
    loadAlertHistory();
}

// ========== 辅助函数 ==========

QString AlertCenterPage::formatAlertType(AlertType type) const
{
    switch (type) {
        case AlertType::PriceAbove: return QStringLiteral("价格高于");
        case AlertType::PriceBelow: return QStringLiteral("价格低于");
        case AlertType::ChangePercentAbove: return QStringLiteral("涨幅高于");
        case AlertType::ChangePercentBelow: return QStringLiteral("跌幅高于");
        case AlertType::ProfitAbove: return QStringLiteral("盈利高于");
        case AlertType::LossAbove: return QStringLiteral("亏损高于");
        case AlertType::MarginWarning: return QStringLiteral("保证金预警");
        case AlertType::DrawdownWarning: return QStringLiteral("回撤预警");
        default: return QStringLiteral("未知");
    }
}

QString AlertCenterPage::formatAlertStatus(AlertStatus status) const
{
    switch (status) {
        case AlertStatus::Active: return QStringLiteral("激活中");
        case AlertStatus::Triggered: return QStringLiteral("已触发");
        case AlertStatus::Disabled: return QStringLiteral("已禁用");
        default: return QStringLiteral("未知");
    }
}
