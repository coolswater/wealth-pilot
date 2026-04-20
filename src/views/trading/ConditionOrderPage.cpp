#include "ConditionOrderPage.h"
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
    updateStyles();
}

void ConditionOrderPage::onPageActivated(const QVariantMap &params)
{
    Q_UNUSED(params);
    onRefreshClicked();
}

void ConditionOrderPage::initUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(24, 16, 24, 16);
    
    // Header
    QHBoxLayout *headerLayout = new QHBoxLayout();
    QLabel *titleLabel = new QLabel("Condition Orders", this);
    titleLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #FFFFFF;");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    mainLayout->addLayout(headerLayout);
    
    // Summary Cards
    QHBoxLayout *summaryLayout = new QHBoxLayout();
    summaryLayout->setSpacing(12);
    
    auto createSummaryCard = [](const QString &title, const QString &color) -> QFrame* {
        QFrame *frame = new QFrame();
        frame->setStyleSheet(QString("background-color: #2C2D33; border-radius: 8px; border-left: 4px solid %1; padding: 12px 16px;").arg(color));
        QHBoxLayout *layout = new QHBoxLayout(frame);
        layout->setContentsMargins(8, 8, 8, 8);
        QLabel *tLabel = new QLabel(title, frame);
        tLabel->setStyleSheet("color: #8E8E93; font-size: 12px;");
        QLabel *vLabel = new QLabel("0", frame);
        vLabel->setStyleSheet(QString("color: %1; font-size: 20px; font-weight: bold;").arg(color));
        layout->addWidget(tLabel);
        layout->addStretch();
        layout->addWidget(vLabel);
        return frame;
    };
    
    QFrame *totalCard = createSummaryCard("Total Orders", "#FFFFFF");
    m_totalCountLabel = totalCard->findChildren<QLabel*>().last();
    summaryLayout->addWidget(totalCard);
    
    QFrame *pendingCard = createSummaryCard("Pending", "#FF9500");
    m_pendingCountLabel = pendingCard->findChildren<QLabel*>().last();
    summaryLayout->addWidget(pendingCard);
    
    QFrame *triggeredCard = createSummaryCard("Triggered", "#34C759");
    m_triggeredCountLabel = triggeredCard->findChildren<QLabel*>().last();
    summaryLayout->addWidget(triggeredCard);
    
    mainLayout->addLayout(summaryLayout);
    
    // Filter Bar
    QHBoxLayout *filterLayout = new QHBoxLayout();
    filterLayout->addWidget(new QLabel("Status:", this));
    m_statusFilterCombo = new QComboBox(this);
    m_statusFilterCombo->addItems({"All", "Pending", "Triggered", "Cancelled", "Expired"});
    filterLayout->addWidget(m_statusFilterCombo);
    filterLayout->addSpacing(20);
    filterLayout->addWidget(new QLabel("Type:", this));
    m_typeFilterCombo = new QComboBox(this);
    m_typeFilterCombo->addItems({"All", "Stop Loss", "Take Profit", "Trailing Stop", "Price Trigger", "Time Trigger"});
    filterLayout->addWidget(m_typeFilterCombo);
    filterLayout->addStretch();
    
    m_addBtn = new QPushButton("Add", this);
    m_editBtn = new QPushButton("Edit", this);
    m_deleteBtn = new QPushButton("Delete", this);
    m_cancelBtn = new QPushButton("Cancel Order", this);
    m_refreshBtn = new QPushButton("Refresh", this);
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
    m_orderTable->setHorizontalHeaderLabels({"Order ID", "Contract", "Type", "Trigger Price", "Order Price", "Quantity", "Direction", "Status", "Create Time"});
    m_orderTable->horizontalHeader()->setStretchLastSection(true);
    m_orderTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_orderTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_orderTable->setAlternatingRowColors(true);
    m_orderTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_orderTable->verticalHeader()->setVisible(false);
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

void ConditionOrderPage::updateStyles()
{
    setStyleSheet(R"(
        QWidget { background-color: #1E1F24; }
        QTableWidget { background-color: #2C2D33; color: #FFFFFF; gridline-color: #3A3B41; border: 1px solid #3A3B41; border-radius: 6px; }
        QTableWidget::item { padding: 6px; }
        QTableWidget::item:selected { background-color: #3A3B41; }
        QHeaderView::section { background-color: #2C2D33; color: #8E8E93; padding: 8px; border: none; border-bottom: 1px solid #3A3B41; font-weight: bold; }
        QPushButton { background-color: #2C2D33; color: #FFFFFF; border: 1px solid #3A3B41; border-radius: 4px; padding: 6px 16px; }
        QPushButton:hover { background-color: #3A3B41; border-color: #FF9500; }
        QPushButton:disabled { color: #8E8E93; background-color: #252629; }
        QComboBox { background-color: #2C2D33; color: #FFFFFF; border: 1px solid #3A3B41; border-radius: 4px; padding: 4px 8px; min-width: 100px; }
        QLabel { color: #FFFFFF; }
    )");
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
    for (const auto &order : m_orders) {
        if (order.orderId == orderId) return order;
    }
    return ConditionOrder();
}

void ConditionOrderPage::onAddClicked()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Add Condition Order");
    dialog.setMinimumWidth(400);
    QFormLayout *form = new QFormLayout(&dialog);
    
    QComboBox *typeCombo = new QComboBox(&dialog);
    typeCombo->addItems({"Stop Loss", "Take Profit", "Trailing Stop", "Price Trigger", "Time Trigger"});
    form->addRow("Type:", typeCombo);
    
    QLineEdit *instrumentEdit = new QLineEdit(&dialog);
    instrumentEdit->setPlaceholderText("e.g., au2506");
    form->addRow("Contract:", instrumentEdit);
    
    QDoubleSpinBox *triggerPriceSpin = new QDoubleSpinBox(&dialog);
    triggerPriceSpin->setDecimals(2);
    triggerPriceSpin->setMaximum(999999.99);
    form->addRow("Trigger Price:", triggerPriceSpin);
    
    QDoubleSpinBox *orderPriceSpin = new QDoubleSpinBox(&dialog);
    orderPriceSpin->setDecimals(2);
    orderPriceSpin->setMaximum(999999.99);
    orderPriceSpin->setSpecialValueText("Market");
    form->addRow("Order Price:", orderPriceSpin);
    
    QSpinBox *quantitySpin = new QSpinBox(&dialog);
    quantitySpin->setMinimum(1);
    quantitySpin->setMaximum(99999);
    form->addRow("Quantity:", quantitySpin);
    
    QComboBox *directionCombo = new QComboBox(&dialog);
    directionCombo->addItems({"Buy/Open", "Sell/Open", "Buy/Close", "Sell/Close"});
    form->addRow("Direction:", directionCombo);
    
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
    if (order.orderId.isEmpty()) { QMessageBox::warning(this, "Warning", "Please select an order."); return; }
    if (order.status != ConditionOrderStatus::Pending) { QMessageBox::warning(this, "Warning", "Only pending orders can be edited."); return; }
    
    QDialog dialog(this);
    dialog.setWindowTitle("Edit Condition Order");
    QFormLayout *form = new QFormLayout(&dialog);
    
    QDoubleSpinBox *triggerPriceSpin = new QDoubleSpinBox(&dialog);
    triggerPriceSpin->setDecimals(2);
    triggerPriceSpin->setMaximum(999999.99);
    triggerPriceSpin->setValue(order.triggerPrice);
    form->addRow("Trigger Price:", triggerPriceSpin);
    
    QDoubleSpinBox *orderPriceSpin = new QDoubleSpinBox(&dialog);
    orderPriceSpin->setDecimals(2);
    orderPriceSpin->setMaximum(999999.99);
    orderPriceSpin->setValue(order.orderPrice);
    form->addRow("Order Price:", orderPriceSpin);
    
    QSpinBox *quantitySpin = new QSpinBox(&dialog);
    quantitySpin->setMinimum(1);
    quantitySpin->setValue(order.quantity);
    form->addRow("Quantity:", quantitySpin);
    
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
    if (order.orderId.isEmpty()) { QMessageBox::warning(this, "Warning", "Please select an order."); return; }
    if (QMessageBox::question(this, "Confirm", QString("Delete order %1?").arg(order.orderId)) == QMessageBox::Yes) {
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
    if (order.orderId.isEmpty()) { QMessageBox::warning(this, "Warning", "Please select an order."); return; }
    if (order.status != ConditionOrderStatus::Pending) { QMessageBox::warning(this, "Warning", "Only pending orders can be cancelled."); return; }
    if (QMessageBox::question(this, "Confirm", QString("Cancel order %1?").arg(order.orderId)) == QMessageBox::Yes) {
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
    for (const auto &order : m_orders) {
        if (statusFilter != "All" && statusToString(order.status) != statusFilter) continue;
        if (typeFilter != "All" && conditionTypeToString(order.conditionType) != typeFilter) continue;
        filtered.append(order);
    }
    
    m_orderTable->setRowCount(filtered.size());
    for (int i = 0; i < filtered.size(); ++i) {
        const auto &order = filtered[i];
        m_orderTable->setItem(i, 0, new QTableWidgetItem(order.orderId));
        m_orderTable->setItem(i, 1, new QTableWidgetItem(order.instrumentId));
        m_orderTable->setItem(i, 2, new QTableWidgetItem(conditionTypeToString(order.conditionType)));
        m_orderTable->setItem(i, 3, new QTableWidgetItem(QString::number(order.triggerPrice, 'f', 2)));
        m_orderTable->setItem(i, 4, new QTableWidgetItem(order.orderPrice > 0 ? QString::number(order.orderPrice, 'f', 2) : "Market"));
        m_orderTable->setItem(i, 5, new QTableWidgetItem(QString::number(order.quantity)));
        m_orderTable->setItem(i, 6, new QTableWidgetItem(QString("%1/%2").arg(order.isBuy ? "Buy" : "Sell").arg(order.isOpen ? "Open" : "Close")));
        auto *statusItem = new QTableWidgetItem(statusToString(order.status));
        statusItem->setForeground(QBrush(statusColor(order.status)));
        m_orderTable->setItem(i, 7, statusItem);
        m_orderTable->setItem(i, 8, new QTableWidgetItem(order.createTime.toString("yyyy-MM-dd HH:mm:ss")));
    }
}

void ConditionOrderPage::updateSummary()
{
    int total = m_orders.size();
    int pending = 0, triggered = 0;
    for (const auto &order : m_orders) {
        if (order.status == ConditionOrderStatus::Pending) pending++;
        else if (order.status == ConditionOrderStatus::Triggered) triggered++;
    }
    m_totalCountLabel->setText(QString::number(total));
    m_pendingCountLabel->setText(QString::number(pending));
    m_triggeredCountLabel->setText(QString::number(triggered));
}

QString ConditionOrderPage::conditionTypeToString(ConditionType type) const
{
    switch (type) {
        case ConditionType::StopLoss: return "Stop Loss";
        case ConditionType::TakeProfit: return "Take Profit";
        case ConditionType::TrailingStop: return "Trailing Stop";
        case ConditionType::PriceTrigger: return "Price Trigger";
        case ConditionType::TimeTrigger: return "Time Trigger";
        default: return "Unknown";
    }
}

QString ConditionOrderPage::statusToString(ConditionOrderStatus status) const
{
    switch (status) {
        case ConditionOrderStatus::Pending: return "Pending";
        case ConditionOrderStatus::Triggered: return "Triggered";
        case ConditionOrderStatus::Cancelled: return "Cancelled";
        case ConditionOrderStatus::Expired: return "Expired";
        default: return "Unknown";
    }
}

QColor ConditionOrderPage::statusColor(ConditionOrderStatus status) const
{
    switch (status) {
        case ConditionOrderStatus::Pending: return QColor("#FF9500");
        case ConditionOrderStatus::Triggered: return QColor("#34C759");
        case ConditionOrderStatus::Cancelled: return QColor("#8E8E93");
        case ConditionOrderStatus::Expired: return QColor("#FF2D55");
        default: return QColor("#FFFFFF");
    }
}
