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
    splitter->setStyleSheet("QSplitter::handle { background: #2a2a2a; height: 1px; }");
    
    // 上部：预警规则
    auto* topWidget = new QWidget();
    topWidget->setStyleSheet("QWidget { background: #0a0a0a; }");
    auto* topLayout = new QVBoxLayout(topWidget);
    topLayout->setContentsMargins(8, 8, 8, 8);
    
    auto* ruleGroup = new QGroupBox(QStringLiteral("预警规则"));
    ruleGroup->setStyleSheet(R"(
        QGroupBox {
            color: #ffffff;
            font-size: 13px;
            font-weight: bold;
            border: 1px solid #2a2a2a;
            border-radius: 4px;
            margin-top: 8px;
        }
        QGroupBox::title { subcontrol-origin: margin; left: 10px; }
    )");
    
    auto* ruleLayout = new QVBoxLayout(ruleGroup);
    initAlertList();
    ruleLayout->addWidget(d->alertTable);
    topLayout->addWidget(ruleGroup);
    
    splitter->addWidget(topWidget);
    
    // 下部：预警历史
    auto* bottomWidget = new QWidget();
    bottomWidget->setStyleSheet("QWidget { background: #0a0a0a; }");
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
    toolbar->setStyleSheet("QWidget { background: #0a0a0a; }");
    
    auto* layout = new QHBoxLayout(toolbar);
    layout->setContentsMargins(12, 6, 12, 6);
    layout->setSpacing(8);
    
    // 添加预警按钮
    d->addBtn = new QPushButton(QStringLiteral("添加预警"));
    d->addBtn->setFixedSize(80, 26);
    d->addBtn->setStyleSheet(R"(
        QPushButton {
            background: #00D4AA;
            color: #ffffff;
            border: none;
            font-size: 12px;
            border-radius: 4px;
        }
        QPushButton:hover { background: #00B894; }
    )");
    layout->addWidget(d->addBtn);
    
    // 删除按钮
    d->deleteBtn = new QPushButton(QStringLiteral("删除"));
    d->deleteBtn->setFixedSize(60, 26);
    d->deleteBtn->setStyleSheet(R"(
        QPushButton {
            background: #FF3366;
            color: #ffffff;
            border: none;
            font-size: 12px;
            border-radius: 4px;
        }
        QPushButton:hover { background: #E91E63; }
    )");
    layout->addWidget(d->deleteBtn);
    
    // 启用/禁用按钮
    d->toggleBtn = new QPushButton(QStringLiteral("启用/禁用"));
    d->toggleBtn->setFixedSize(80, 26);
    d->toggleBtn->setStyleSheet(R"(
        QPushButton {
            background: #2a2a2a;
            color: #ffffff;
            border: none;
            font-size: 12px;
            border-radius: 4px;
        }
        QPushButton:hover { background: #3a3a3a; }
    )");
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
    
    d->alertTable->setStyleSheet(R"(
        QTableWidget {
            background: #0a0a0a;
            color: #ffffff;
            border: none;
            gridline-color: #1a1a1a;
            font-size: 12px;
        }
        QTableWidget::item:selected { background: #2a2a2a; }
        QHeaderView::section {
            background: #0d0d0d;
            color: #888888;
            border: none;
            padding: 6px;
            font-size: 11px;
        }
    )");
    
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
    d->alertRules.append({QUuid::createUuid().toString(), "600519", QStringLiteral("贵州茅台"), AlertType::PriceAbove, 1800.0, AlertStatus::Active, QDateTime::currentDateTime()});
    d->alertRules.append({QUuid::createUuid().toString(), "000858", QStringLiteral("五粮液"), AlertType::ChangeBelow, -5.0, AlertStatus::Active, QDateTime::currentDateTime()});
    d->alertRules.append({QUuid::createUuid().toString(), "000001", QStringLiteral("平安银行"), AlertType::VolumeAbove, 10000000, AlertStatus::Triggered, QDateTime::currentDateTime().addDays(-1), QDateTime::currentDateTime()});
    
    d->alertTable->setRowCount(d->alertRules.size());
    
    for (int i = 0; i < d->alertRules.size(); ++i) {
        const auto& rule = d->alertRules[i];
        
        d->alertTable->setItem(i, 0, new QTableWidgetItem(rule.symbol));
        d->alertTable->setItem(i, 1, new QTableWidgetItem(rule.name));
        d->alertTable->setItem(i, 2, new QTableWidgetItem(formatAlertType(rule.type)));
        d->alertTable->setItem(i, 3, new QTableWidgetItem(QString::number(rule.threshold)));
        
        auto* statusItem = new QTableWidgetItem(formatAlertStatus(rule.status));
        if (rule.status == AlertStatus::Active) {
            statusItem->setForeground(QColor("#00D4AA"));
        } else if (rule.status == AlertStatus::Triggered) {
            statusItem->setForeground(QColor("#FF3366"));
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
    d->alertHistory.append({QUuid::createUuid().toString(), "600519", QStringLiteral("贵州茅台"), AlertType::PriceAbove, 1800.0, 1805.5, QDateTime::currentDateTime().addHours(-2), QStringLiteral("价格突破1800元"), true});
    d->alertHistory.append({QUuid::createUuid().toString(), "000858", QStringLiteral("五粮液"), AlertType::ChangeBelow, -5.0, -6.2, QDateTime::currentDateTime().addDays(-1), QStringLiteral("跌幅超过5%"), true});
    
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
    LOG_INFO(QString("Alert rule added: %1 %2").arg(rule.symbol, formatAlertType(rule.type)));
}

void AlertCenterPage::removeAlertRule(const QString& id)
{
    for (int i = 0; i < d->alertRules.size(); ++i) {
        if (d->alertRules[i].id == id) {
            d->alertRules.removeAt(i);
            break;
        }
    }
    loadAlertRules();
}

void AlertCenterPage::toggleAlertRule(const QString& id)
{
    for (auto& rule : d->alertRules) {
        if (rule.id == id) {
            if (rule.status == AlertStatus::Active) {
                rule.status = AlertStatus::Disabled;
            } else if (rule.status == AlertStatus::Disabled) {
                rule.status = AlertStatus::Active;
            }
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
    dialog->setStyleSheet("QDialog { background: #0a0a0a; } QLabel { color: #ffffff; }");
    
    auto* layout = new QFormLayout(dialog);
    
    auto* symbolEdit = new QLineEdit();
    symbolEdit->setStyleSheet("background: #1a1a1a; color: #ffffff; padding: 4px;");
    layout->addRow(QStringLiteral("标的代码:"), symbolEdit);
    
    auto* typeCombo = new QComboBox();
    typeCombo->addItems({QStringLiteral("价格高于"), QStringLiteral("价格低于"), QStringLiteral("涨幅高于"), QStringLiteral("跌幅高于")});
    typeCombo->setStyleSheet("background: #1a1a1a; color: #ffffff; padding: 4px;");
    layout->addRow(QStringLiteral("预警类型:"), typeCombo);
    
    auto* thresholdSpin = new QDoubleSpinBox();
    thresholdSpin->setRange(-100, 100000);
    thresholdSpin->setStyleSheet("background: #1a1a1a; color: #ffffff; padding: 4px;");
    layout->addRow(QStringLiteral("阈值:"), thresholdSpin);
    
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttonBox->setStyleSheet("QPushButton { background: #2a2a2a; color: #ffffff; padding: 6px 16px; }");
    layout->addRow(buttonBox);
    
    connect(buttonBox, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    
    if (dialog->exec() == QDialog::Accepted) {
        AlertRule rule;
        rule.id = QUuid::createUuid().toString();
        rule.symbol = symbolEdit->text();
        rule.type = static_cast<AlertType>(typeCombo->currentIndex());
        rule.threshold = thresholdSpin->value();
        rule.status = AlertStatus::Active;
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
        d->currentAlertId = d->alertRules[row].id;
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

QString AlertCenterPage::formatAlertType(AlertType type)
{
    switch (type) {
        case AlertType::PriceAbove: return QStringLiteral("价格高于");
        case AlertType::PriceBelow: return QStringLiteral("价格低于");
        case AlertType::ChangeAbove: return QStringLiteral("涨幅高于");
        case AlertType::ChangeBelow: return QStringLiteral("跌幅高于");
        case AlertType::VolumeAbove: return QStringLiteral("成交量高于");
        case AlertType::TurnoverAbove: return QStringLiteral("换手率高于");
        default: return QStringLiteral("未知");
    }
}

QString AlertCenterPage::formatAlertStatus(AlertStatus status)
{
    switch (status) {
        case AlertStatus::Active: return QStringLiteral("激活中");
        case AlertStatus::Triggered: return QStringLiteral("已触发");
        case AlertStatus::Disabled: return QStringLiteral("已禁用");
        default: return QStringLiteral("未知");
    }
}
