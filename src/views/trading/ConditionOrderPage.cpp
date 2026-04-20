#include "ConditionOrderPage.h"
#include "ui/components/PageStyles.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QDialog>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QUuid>

ConditionOrderPage::ConditionOrderPage(QWidget *parent)
    : BasePage(parent)
{
}

ConditionOrderPage::~ConditionOrderPage()
{
}

void ConditionOrderPage::initializePage()
{
    initUI();
    initConnections();
}

void ConditionOrderPage::onPageActivated(const QVariantMap &params)
{
    Q_UNUSED(params);
}

void ConditionOrderPage::initUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(24, 16, 24, 16);
    
    // Header
    QHBoxLayout *headerLayout = new QHBoxLayout();
    QLabel *titleLabel = new QLabel("条件单管理", this);
    titleLabel->setStyleSheet(PageStyles::titleText());
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    mainLayout->addLayout(headerLayout);
    
    // Summary Cards
    QHBoxLayout *summaryLayout = new QHBoxLayout();
    summaryLayout->setSpacing(12);
    
    auto createSummaryCard = [](const QString &title, const QString &color) -> QFrame* {
        QFrame *frame = new QFrame();
        frame->setStyleSheet(PageStyles::statCard(color));
        QHBoxLayout *layout = new QHBoxLayout(frame);
        layout->setContentsMargins(8, 8, 8, 8);
        QLabel *tLabel = new QLabel(title, frame);
        tLabel->setStyleSheet(PageStyles::labelText());
        QLabel *vLabel = new QLabel("0", frame);
        vLabel->setStyleSheet(PageStyles::valueText(color));
        layout->addWidget(tLabel);
        layout->addStretch();
        layout->addWidget(vLabel);
        return frame;
    };
    
    QFrame *totalCard = createSummaryCard("总单数", PageStyles::primaryColor());
    m_totalCountLabel = totalCard->findChildren<QLabel*>().last();
    summaryLayout->addWidget(totalCard);
    
    QFrame *pendingCard = createSummaryCard("待触发", PageStyles::warningColor());
    m_pendingCountLabel = pendingCard->findChildren<QLabel*>().last();
    summaryLayout->addWidget(pendingCard);
    
    QFrame *triggeredCard = createSummaryCard("已触发", PageStyles::successColor());
    m_triggeredCountLabel = triggeredCard->findChildren<QLabel*>().last();
    summaryLayout->addWidget(triggeredCard);
    
    mainLayout->addLayout(summaryLayout);
    
    // Filter Bar
    QHBoxLayout *filterLayout = new QHBoxLayout();
    
    filterLayout->addWidget(new QLabel("状态:", this));
    m_statusFilterCombo = new QComboBox(this);
    m_statusFilterCombo->addItems({"全部", "待触发", "已触发", "已撤销", "已过期"});
    m_statusFilterCombo->setStyleSheet(PageStyles::comboBox());
    filterLayout->addWidget(m_statusFilterCombo);
    
    filterLayout->addSpacing(20);
    filterLayout->addWidget(new QLabel("类型:", this));
    m_typeFilterCombo = new QComboBox(this);
    m_typeFilterCombo->addItems({"全部", "止损", "止盈", "跟踪止损", "价格触发", "时间触发"});
    m_typeFilterCombo->setStyleSheet(PageStyles::comboBox());
    filterLayout->addWidget(m_typeFilterCombo);
    
    filterLayout->addStretch();
    
    m_addBtn = new QPushButton("添加", this);
    m_editBtn = new QPushButton("编辑", this);
    m_deleteBtn = new QPushButton("删除", this);
    m_cancelBtn = new QPushButton("撤销", this);
    m_refreshBtn = new QPushButton("刷新", this);
    
    QString btnStyle = PageStyles::secondaryButton();
    m_addBtn->setStyleSheet(btnStyle);
    m_editBtn->setStyleSheet(btnStyle);
    m_deleteBtn->setStyleSheet(btnStyle);
    m_cancelBtn->setStyleSheet(btnStyle);
    m_refreshBtn->setStyleSheet(btnStyle);
    
    m_editBtn->setEnabled(false);
    m_deleteBtn->setEnabled(false);
    m_cancelBtn->setEnabled(false);
    
    filterLayout->addWidget(m_addBtn);
    filterLayout->addWidget(m_editBtn);
    filterLayout->addWidget(m_deleteBtn);
    filterLayout->addWidget(m_cancelBtn);
    filterLayout->addWidget(m_refreshBtn);
    mainLayout->addLayout(filterLayout);
    
    // Order Table
    m_orderTable = new QTableWidget(this);
    m_orderTable->setColumnCount(9);
    m_orderTable->setHorizontalHeaderLabels({"单号", "合约", "类型", "触发价", "委托价", "数量", "方向", "状态", "创建时间"});
    m_orderTable->horizontalHeader()->setStretchLastSection(true);
    m_orderTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_orderTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_orderTable->setAlternatingRowColors(true);
    m_orderTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_orderTable->verticalHeader()->setVisible(false);
    m_orderTable->setStyleSheet(PageStyles::table());
    mainLayout->addWidget(m_orderTable, 1);
}

void ConditionOrderPage::initConnections()
{
    connect(m_addBtn, &QPushButton::clicked, this, &ConditionOrderPage::onAddClicked);
    connect(m_editBtn, &QPushButton::clicked, this, &ConditionOrderPage::onEditClicked);
    connect(m_deleteBtn, &QPushButton::clicked, this, &ConditionOrderPage::onDeleteClicked);
    connect(m_cancelBtn, &QPushButton::clicked, this, &ConditionOrderPage::onCancelClicked);
    connect(m_refreshBtn, &QPushButton::clicked, this, &ConditionOrderPage::onRefreshClicked);
    connect(m_statusFilterCombo, &QComboBox::currentTextChanged, this, &ConditionOrderPage::onFilterChanged);
    connect(m_typeFilterCombo, &QComboBox::currentTextChanged, this, &ConditionOrderPage::onFilterChanged);
    connect(m_orderTable, &QTableWidget::itemSelectionChanged, this, &ConditionOrderPage::onSelectionChanged);
    connect(m_orderTable, &QTableWidget::doubleClicked, this, &ConditionOrderPage::onEditClicked);
}

void ConditionOrderPage::addConditionOrder(const ConditionOrder &order)
{
    m_orders.append(order);
    updateTable();
    updateSummary();
}

void ConditionOrderPage::setConditionOrders(const QVector<ConditionOrder> &orders)
{
    m_orders = orders;
    updateTable();
    updateSummary();
}

ConditionOrderPage::ConditionOrder ConditionOrderPage::getSelectedOrder() const
{
    int row = m_orderTable->currentRow();
    if (row < 0) return ConditionOrder();
    QString orderId = m_orderTable->item(row, 0)->text();
    for (const auto &o : m_orders) {
        if (o.orderId == orderId) return o;
    }
    return ConditionOrder();
}

void ConditionOrderPage::onAddClicked()
{
    QDialog dialog(this);
    dialog.setWindowTitle("添加条件单");
    dialog.setMinimumWidth(400);
    QFormLayout *form = new QFormLayout(&dialog);
    
    QComboBox *typeCombo = new QComboBox(&dialog);
    typeCombo->addItems({"止损", "止盈", "跟踪止损", "价格触发", "时间触发"});
    typeCombo->setStyleSheet(PageStyles::comboBox());
    form->addRow("类型:", typeCombo);
    
    QLineEdit *instrumentEdit = new QLineEdit(&dialog);
    instrumentEdit->setPlaceholderText("如: au2506");
    instrumentEdit->setStyleSheet(PageStyles::inputField());
    form->addRow("合约:", instrumentEdit);
    
    QDoubleSpinBox *triggerPriceSpin = new QDoubleSpinBox(&dialog);
    triggerPriceSpin->setDecimals(2);
    triggerPriceSpin->setMaximum(999999.99);
    triggerPriceSpin->setStyleSheet(PageStyles::inputField());
    form->addRow("触发价:", triggerPriceSpin);
    
    QDoubleSpinBox *orderPriceSpin = new QDoubleSpinBox(&dialog);
    orderPriceSpin->setDecimals(2);
    orderPriceSpin->setMaximum(999999.99);
    orderPriceSpin->setSpecialValueText("市价");
    orderPriceSpin->setStyleSheet(PageStyles::inputField());
    form->addRow("委托价:", orderPriceSpin);
    
    QSpinBox *quantitySpin = new QSpinBox(&dialog);
    quantitySpin->setMinimum(1);
    quantitySpin->setMaximum(99999);
    quantitySpin->setStyleSheet(PageStyles::inputField());
    form->addRow("数量:", quantitySpin);
    
    QComboBox *directionCombo = new QComboBox(&dialog);
    directionCombo->addItems({"买入/开仓", "卖出/开仓", "买入/平仓", "卖出/平仓"});
    directionCombo->setStyleSheet(PageStyles::comboBox());
    form->addRow("方向:", directionCombo);
    
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    form->addRow(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    
    if (dialog.exec() == QDialog::Accepted) {
        ConditionOrder order;
        order.orderId = QUuid::createUuid().toString(QUuid::WithoutBraces).left(12);
        order.instrumentId = instrumentEdit->text();
        order.conditionType = static_cast<ConditionType>(typeCombo->currentIndex());
        order.triggerPrice = triggerPriceSpin->value();
        order.orderPrice = orderPriceSpin->value();
        order.quantity = quantitySpin->value();
        int dirIdx = directionCombo->currentIndex();
        order.isBuy = (dirIdx == 0 || dirIdx == 2);
        order.isOpen = (dirIdx == 0 || dirIdx == 1);
        order.status = ConditionOrderStatus::Pending;
        order.createTime = QDateTime::currentDateTime();
        addConditionOrder(order);
        emit conditionOrderAdded(order);
    }
}

void ConditionOrderPage::onEditClicked()
{
    ConditionOrder order = getSelectedOrder();
    if (order.orderId.isEmpty()) { QMessageBox::warning(this, "提示", "请选择订单"); return; }
    if (order.status != ConditionOrderStatus::Pending) { QMessageBox::warning(this, "提示", "只能编辑待触发订单"); return; }
    
    QDialog dialog(this);
    dialog.setWindowTitle("编辑条件单");
    QFormLayout *form = new QFormLayout(&dialog);
    
    QDoubleSpinBox *triggerPriceSpin = new QDoubleSpinBox(&dialog);
    triggerPriceSpin->setDecimals(2);
    triggerPriceSpin->setMaximum(999999.99);
    triggerPriceSpin->setValue(order.triggerPrice);
    triggerPriceSpin->setStyleSheet(PageStyles::inputField());
    form->addRow("触发价:", triggerPriceSpin);
    
    QDoubleSpinBox *orderPriceSpin = new QDoubleSpinBox(&dialog);
    orderPriceSpin->setDecimals(2);
    orderPriceSpin->setMaximum(999999.99);
    orderPriceSpin->setValue(order.orderPrice);
    orderPriceSpin->setStyleSheet(PageStyles::inputField());
    form->addRow("委托价:", orderPriceSpin);
    
    QSpinBox *quantitySpin = new QSpinBox(&dialog);
    quantitySpin->setMinimum(1);
    quantitySpin->setValue(order.quantity);
    quantitySpin->setStyleSheet(PageStyles::inputField());
    form->addRow("数量:", quantitySpin);
    
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    form->addRow(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    
    if (dialog.exec() == QDialog::Accepted) {
        order.triggerPrice = triggerPriceSpin->value();
        order.orderPrice = orderPriceSpin->value();
        order.quantity = quantitySpin->value();
        for (int i = 0; i < m_orders.size(); ++i) {
            if (m_orders[i].orderId == order.orderId) { m_orders[i] = order; break; }
        }
        updateTable();
    }
}

void ConditionOrderPage::onDeleteClicked()
{
    ConditionOrder order = getSelectedOrder();
    if (order.orderId.isEmpty()) { QMessageBox::warning(this, "提示", "请选择订单"); return; }
    if (QMessageBox::question(this, "确认", QString("删除订单 %1?").arg(order.orderId)) == QMessageBox::Yes) {
        QString orderId = order.orderId;
        for (int i = 0; i < m_orders.size(); ++i) {
            if (m_orders[i].orderId == orderId) { m_orders.removeAt(i); break; }
        }
        updateTable();
        updateSummary();
        emit conditionOrderRemoved(orderId);
    }
}

void ConditionOrderPage::onCancelClicked()
{
    ConditionOrder order = getSelectedOrder();
    if (order.orderId.isEmpty()) { QMessageBox::warning(this, "提示", "请选择订单"); return; }
    if (order.status != ConditionOrderStatus::Pending) { QMessageBox::warning(this, "提示", "只能撤销待触发订单"); return; }
    if (QMessageBox::question(this, "确认", QString("撤销订单 %1?").arg(order.orderId)) == QMessageBox::Yes) {
        for (int i = 0; i < m_orders.size(); ++i) {
            if (m_orders[i].orderId == order.orderId) {
                m_orders[i].status = ConditionOrderStatus::Cancelled;
                break;
            }
        }
        updateTable();
        updateSummary();
        emit conditionOrderCancelled(order.orderId);
    }
}

void ConditionOrderPage::onRefreshClicked() { emit requestRefresh(); }
void ConditionOrderPage::onFilterChanged() { updateTable(); }
void ConditionOrderPage::onSelectionChanged()
{
    bool hasSelection = m_orderTable->currentRow() >= 0;
    m_editBtn->setEnabled(hasSelection);
    m_deleteBtn->setEnabled(hasSelection);
    m_cancelBtn->setEnabled(hasSelection);
}

void ConditionOrderPage::updateTable()
{
    QString statusFilter = m_statusFilterCombo->currentText();
    QString typeFilter = m_typeFilterCombo->currentText();
    
    QVector<ConditionOrder> filtered;
    for (const auto &o : m_orders) {
        if (statusFilter != "全部" && statusToString(o.status) != statusFilter) continue;
        if (typeFilter != "全部" && conditionTypeToString(o.conditionType) != typeFilter) continue;
        filtered.append(o);
    }
    
    m_orderTable->setRowCount(filtered.size());
    for (int i = 0; i < filtered.size(); ++i) {
        const auto &o = filtered[i];
        m_orderTable->setItem(i, 0, new QTableWidgetItem(o.orderId));
        m_orderTable->setItem(i, 1, new QTableWidgetItem(o.instrumentId));
        m_orderTable->setItem(i, 2, new QTableWidgetItem(conditionTypeToString(o.conditionType)));
        m_orderTable->setItem(i, 3, new QTableWidgetItem(QString::number(o.triggerPrice, 'f', 2)));
        m_orderTable->setItem(i, 4, new QTableWidgetItem(o.orderPrice > 0 ? QString::number(o.orderPrice, 'f', 2) : "市价"));
        m_orderTable->setItem(i, 5, new QTableWidgetItem(QString::number(o.quantity)));
        m_orderTable->setItem(i, 6, new QTableWidgetItem(QString("%1/%2").arg(o.isBuy ? "买" : "卖").arg(o.isOpen ? "开" : "平")));
        
        auto *statusItem = new QTableWidgetItem(statusToString(o.status));
        statusItem->setForeground(QBrush(statusColor(o.status)));
        m_orderTable->setItem(i, 7, statusItem);
        m_orderTable->setItem(i, 8, new QTableWidgetItem(o.createTime.toString("yyyy-MM-dd HH:mm:ss")));
    }
}

void ConditionOrderPage::updateSummary()
{
    int total = m_orders.size();
    int pending = 0, triggered = 0;
    for (const auto &o : m_orders) {
        if (o.status == ConditionOrderStatus::Pending) pending++;
        else if (o.status == ConditionOrderStatus::Triggered) triggered++;
    }
    m_totalCountLabel->setText(QString::number(total));
    m_pendingCountLabel->setText(QString::number(pending));
    m_triggeredCountLabel->setText(QString::number(triggered));
}

QString ConditionOrderPage::conditionTypeToString(ConditionType type) const
{
    switch (type) {
        case ConditionType::StopLoss: return "止损";
        case ConditionType::TakeProfit: return "止盈";
        case ConditionType::TrailingStop: return "跟踪止损";
        case ConditionType::PriceTrigger: return "价格触发";
        case ConditionType::TimeTrigger: return "时间触发";
        default: return "未知";
    }
}

QString ConditionOrderPage::statusToString(ConditionOrderStatus status) const
{
    switch (status) {
        case ConditionOrderStatus::Pending: return "待触发";
        case ConditionOrderStatus::Triggered: return "已触发";
        case ConditionOrderStatus::Cancelled: return "已撤销";
        case ConditionOrderStatus::Expired: return "已过期";
        default: return "未知";
    }
}

QColor ConditionOrderPage::statusColor(ConditionOrderStatus status) const
{
    switch (status) {
        case ConditionOrderStatus::Pending: return QColor(PageStyles::warningColor());
        case ConditionOrderStatus::Triggered: return QColor(PageStyles::successColor());
        case ConditionOrderStatus::Cancelled: return QColor(PageStyles::flatColor());
        case ConditionOrderStatus::Expired: return QColor(PageStyles::errorColor());
        default: return QColor(PageStyles::primaryColor());
    }
}
