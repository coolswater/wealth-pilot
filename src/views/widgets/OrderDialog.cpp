#include "OrderDialog.h"
#include <QMessageBox>
#include <cmath>

OrderDialog::OrderDialog(QWidget *parent)
    : QDialog(parent)
    , m_lastPrice(0.0)
    , m_tickSize(0.01)
    , m_volumeMultiple(1)
    , m_marginRatio(0.1)
    , m_available(0.0)
    , m_margin(0.0)
    , m_frozenMargin(0.0)
    , m_longPosition(0)
    , m_shortPosition(0)
{
    initUI();
    initConnections();
    updateStyles();
}

OrderDialog::~OrderDialog()
{
}

void OrderDialog::initUI()
{
    setWindowTitle("Place Order");
    setMinimumSize(480, 640);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    // ===== Header Section =====
    QFrame *headerFrame = new QFrame(this);
    headerFrame->setObjectName("headerFrame");
    QHBoxLayout *headerLayout = new QHBoxLayout(headerFrame);
    
    m_instrumentLabel = new QLabel("Select Contract", this);
    m_instrumentLabel->setObjectName("instrumentLabel");
    m_instrumentLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #FFFFFF;");
    
    m_priceLabel = new QLabel("--", this);
    m_priceLabel->setObjectName("priceLabel");
    m_priceLabel->setStyleSheet("font-size: 16px; color: #FFFFFF;");
    
    m_changeLabel = new QLabel("--", this);
    m_changeLabel->setObjectName("changeLabel");
    m_changeLabel->setStyleSheet("font-size: 14px;");
    
    headerLayout->addWidget(m_instrumentLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(m_priceLabel);
    headerLayout->addSpacing(10);
    headerLayout->addWidget(m_changeLabel);
    
    mainLayout->addWidget(headerFrame);
    
    // ===== Order Type Section =====
    QGroupBox *orderTypeGroup = new QGroupBox("Order Type", this);
    orderTypeGroup->setObjectName("orderTypeGroup");
    QGridLayout *orderTypeLayout = new QGridLayout(orderTypeGroup);
    orderTypeLayout->setSpacing(10);
    
    // Order type combo
    orderTypeLayout->addWidget(new QLabel("Type:", this), 0, 0);
    m_orderTypeCombo = new QComboBox(this);
    m_orderTypeCombo->addItems({"Market", "Limit", "Stop", "Stop Limit", "Iceberg"});
    orderTypeLayout->addWidget(m_orderTypeCombo, 0, 1);
    
    // Direction combo
    orderTypeLayout->addWidget(new QLabel("Direction:", this), 1, 0);
    m_directionCombo = new QComboBox(this);
    m_directionCombo->addItems({"Buy/Long", "Sell/Short"});
    orderTypeLayout->addWidget(m_directionCombo, 1, 1);
    
    // Open/Close combo
    orderTypeLayout->addWidget(new QLabel("Open/Close:", this), 2, 0);
    m_openCloseCombo = new QComboBox(this);
    m_openCloseCombo->addItems({"Open", "Close", "Close Today", "Close Yesterday"});
    orderTypeLayout->addWidget(m_openCloseCombo, 2, 1);
    
    mainLayout->addWidget(orderTypeGroup);
    
    // ===== Price & Quantity Section =====
    QGroupBox *priceGroup = new QGroupBox("Price & Quantity", this);
    priceGroup->setObjectName("priceGroup");
    QGridLayout *priceLayout = new QGridLayout(priceGroup);
    priceLayout->setSpacing(10);
    
    // Price
    priceLayout->addWidget(new QLabel("Price:", this), 0, 0);
    m_priceSpinBox = new QDoubleSpinBox(this);
    m_priceSpinBox->setDecimals(2);
    m_priceSpinBox->setMinimum(0.0);
    m_priceSpinBox->setMaximum(999999.99);
    m_priceSpinBox->setSingleStep(0.01);
    m_priceSpinBox->setSuffix(" ");
    priceLayout->addWidget(m_priceSpinBox, 0, 1);
    
    // Stop Price (for stop orders)
    priceLayout->addWidget(new QLabel("Trigger Price:", this), 1, 0);
    m_stopPriceSpinBox = new QDoubleSpinBox(this);
    m_stopPriceSpinBox->setDecimals(2);
    m_stopPriceSpinBox->setMinimum(0.0);
    m_stopPriceSpinBox->setMaximum(999999.99);
    m_stopPriceSpinBox->setSingleStep(0.01);
    m_stopPriceSpinBox->setEnabled(false);
    priceLayout->addWidget(m_stopPriceSpinBox, 1, 1);
    
    // Quantity
    priceLayout->addWidget(new QLabel("Quantity:", this), 2, 0);
    m_quantitySpinBox = new QSpinBox(this);
    m_quantitySpinBox->setMinimum(1);
    m_quantitySpinBox->setMaximum(99999);
    m_quantitySpinBox->setValue(1);
    priceLayout->addWidget(m_quantitySpinBox, 2, 1);
    
    // Quick quantity buttons
    QHBoxLayout *quickQtyLayout = new QHBoxLayout();
    for (int qty : {1, 2, 5, 10, 20, 50, 100}) {
        QPushButton *btn = new QPushButton(QString::number(qty), this);
        btn->setFixedSize(40, 28);
        btn->setObjectName("quickQtyBtn");
        connect(btn, &QPushButton::clicked, this, [this, qty]() {
            m_quantitySpinBox->setValue(qty);
        });
        quickQtyLayout->addWidget(btn);
    }
    priceLayout->addLayout(quickQtyLayout, 3, 0, 1, 2);
    
    mainLayout->addWidget(priceGroup);
    
    // ===== Calculation Section =====
    QGroupBox *calcGroup = new QGroupBox("Calculation", this);
    calcGroup->setObjectName("calcGroup");
    QGridLayout *calcLayout = new QGridLayout(calcGroup);
    calcLayout->setSpacing(10);
    
    calcLayout->addWidget(new QLabel("Margin:", this), 0, 0);
    m_marginLabel = new QLabel("0.00", this);
    m_marginLabel->setObjectName("marginLabel");
    calcLayout->addWidget(m_marginLabel, 0, 1);
    
    calcLayout->addWidget(new QLabel("Commission:", this), 1, 0);
    m_commissionLabel = new QLabel("0.00", this);
    m_commissionLabel->setObjectName("commissionLabel");
    calcLayout->addWidget(m_commissionLabel, 1, 1);
    
    calcLayout->addWidget(new QLabel("Total:", this), 2, 0);
    m_totalLabel = new QLabel("0.00", this);
    m_totalLabel->setObjectName("totalLabel");
    m_totalLabel->setStyleSheet("font-weight: bold; color: #FF9500;");
    calcLayout->addWidget(m_totalLabel, 2, 1);
    
    m_calculateBtn = new QPushButton("Calculate", this);
    m_calculateBtn->setObjectName("calculateBtn");
    calcLayout->addWidget(m_calculateBtn, 3, 0, 1, 2);
    
    mainLayout->addWidget(calcGroup);
    
    // ===== Stop Loss & Take Profit Section =====
    QGroupBox *slTpGroup = new QGroupBox("Stop Loss & Take Profit", this);
    slTpGroup->setObjectName("slTpGroup");
    QGridLayout *slTpLayout = new QGridLayout(slTpGroup);
    slTpLayout->setSpacing(10);
    
    // Take Profit
    m_enableTakeProfitCheck = new QCheckBox("Take Profit:", this);
    slTpLayout->addWidget(m_enableTakeProfitCheck, 0, 0);
    m_takeProfitSpinBox = new QDoubleSpinBox(this);
    m_takeProfitSpinBox->setDecimals(2);
    m_takeProfitSpinBox->setMinimum(0.0);
    m_takeProfitSpinBox->setMaximum(999999.99);
    m_takeProfitSpinBox->setEnabled(false);
    slTpLayout->addWidget(m_takeProfitSpinBox, 0, 1);
    
    // Stop Loss
    m_enableStopLossCheck = new QCheckBox("Stop Loss:", this);
    slTpLayout->addWidget(m_enableStopLossCheck, 1, 0);
    m_stopLossSpinBox = new QDoubleSpinBox(this);
    m_stopLossSpinBox->setDecimals(2);
    m_stopLossSpinBox->setMinimum(0.0);
    m_stopLossSpinBox->setMaximum(999999.99);
    m_stopLossSpinBox->setEnabled(false);
    slTpLayout->addWidget(m_stopLossSpinBox, 1, 1);
    
    mainLayout->addWidget(slTpGroup);
    
    // ===== Position & Account Info Section =====
    QGroupBox *infoGroup = new QGroupBox("Position & Account", this);
    infoGroup->setObjectName("infoGroup");
    QGridLayout *infoLayout = new QGridLayout(infoGroup);
    infoLayout->setSpacing(10);
    
    infoLayout->addWidget(new QLabel("Long Position:", this), 0, 0);
    m_longPosLabel = new QLabel("0", this);
    infoLayout->addWidget(m_longPosLabel, 0, 1);
    
    infoLayout->addWidget(new QLabel("Short Position:", this), 1, 0);
    m_shortPosLabel = new QLabel("0", this);
    infoLayout->addWidget(m_shortPosLabel, 1, 1);
    
    infoLayout->addWidget(new QLabel("Available:", this), 2, 0);
    m_availableLabel = new QLabel("0.00", this);
    m_availableLabel->setStyleSheet("color: #34C759;");
    infoLayout->addWidget(m_availableLabel, 2, 1);
    
    mainLayout->addWidget(infoGroup);
    
    // ===== Risk Display Section =====
    QGroupBox *riskGroup = new QGroupBox("Risk Analysis", this);
    riskGroup->setObjectName("riskGroup");
    QGridLayout *riskLayout = new QGridLayout(riskGroup);
    riskLayout->setSpacing(10);
    
    riskLayout->addWidget(new QLabel("Risk Ratio:", this), 0, 0);
    m_riskRatioLabel = new QLabel("0.00%", this);
    riskLayout->addWidget(m_riskRatioLabel, 0, 1);
    
    riskLayout->addWidget(new QLabel("Risk Amount:", this), 1, 0);
    m_riskAmountLabel = new QLabel("0.00", this);
    riskLayout->addWidget(m_riskAmountLabel, 1, 1);
    
    riskLayout->addWidget(new QLabel("Profit Ratio:", this), 2, 0);
    m_profitRatioLabel = new QLabel("0.00%", this);
    riskLayout->addWidget(m_profitRatioLabel, 2, 1);
    
    riskLayout->addWidget(new QLabel("Profit Amount:", this), 3, 0);
    m_profitAmountLabel = new QLabel("0.00", this);
    riskLayout->addWidget(m_profitAmountLabel, 3, 1);
    
    mainLayout->addWidget(riskGroup);
    
    // ===== Buttons Section =====
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);
    
    m_submitBtn = new QPushButton("Submit Order", this);
    m_submitBtn->setObjectName("submitBtn");
    m_submitBtn->setMinimumHeight(40);
    m_submitBtn->setStyleSheet(R"(
        QPushButton#submitBtn {
            background-color: #FF9500;
            color: white;
            font-size: 14px;
            font-weight: bold;
            border-radius: 6px;
        }
        QPushButton#submitBtn:hover {
            background-color: #FF8000;
        }
        QPushButton#submitBtn:pressed {
            background-color: #E68600;
        }
    )");
    
    m_cancelBtn = new QPushButton("Cancel", this);
    m_cancelBtn->setObjectName("cancelBtn");
    m_cancelBtn->setMinimumHeight(40);
    
    m_resetBtn = new QPushButton("Reset", this);
    m_resetBtn->setObjectName("resetBtn");
    m_resetBtn->setMinimumHeight(40);
    
    btnLayout->addWidget(m_resetBtn);
    btnLayout->addWidget(m_cancelBtn);
    btnLayout->addWidget(m_submitBtn);
    
    mainLayout->addLayout(btnLayout);
    mainLayout->addStretch();
}

void OrderDialog::initConnections()
{
    connect(m_orderTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &OrderDialog::onOrderTypeChanged);
    connect(m_directionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &OrderDialog::onDirectionChanged);
    connect(m_openCloseCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &OrderDialog::onOpenCloseChanged);
    connect(m_quantitySpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &OrderDialog::onQuantityChanged);
    connect(m_priceSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &OrderDialog::onPriceChanged);
    
    connect(m_enableTakeProfitCheck, &QCheckBox::toggled,
            m_takeProfitSpinBox, &QDoubleSpinBox::setEnabled);
    connect(m_enableStopLossCheck, &QCheckBox::toggled,
            m_stopLossSpinBox, &QDoubleSpinBox::setEnabled);
    
    connect(m_calculateBtn, &QPushButton::clicked,
            this, &OrderDialog::onCalculateClicked);
    connect(m_submitBtn, &QPushButton::clicked,
            this, &OrderDialog::onSubmitClicked);
    connect(m_cancelBtn, &QPushButton::clicked,
            this, &OrderDialog::onCancelClicked);
    connect(m_resetBtn, &QPushButton::clicked,
            this, &OrderDialog::onResetClicked);
}

void OrderDialog::updateStyles()
{
    setStyleSheet(R"(
        QDialog {
            background-color: #1E1F24;
        }
        QGroupBox {
            color: #FFFFFF;
            font-weight: bold;
            border: 1px solid #3A3B41;
            border-radius: 6px;
            margin-top: 12px;
            padding-top: 8px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            padding: 0 8px;
            background-color: #1E1F24;
        }
        QLabel {
            color: #FFFFFF;
        }
        QComboBox, QSpinBox, QDoubleSpinBox, QLineEdit {
            background-color: #2C2D33;
            color: #FFFFFF;
            border: 1px solid #3A3B41;
            border-radius: 4px;
            padding: 4px 8px;
            min-height: 28px;
        }
        QComboBox:hover, QSpinBox:hover, QDoubleSpinBox:hover {
            border-color: #FF9500;
        }
        QComboBox::drop-down {
            border: none;
            width: 20px;
        }
        QComboBox::down-arrow {
            image: none;
            border-left: 5px solid transparent;
            border-right: 5px solid transparent;
            border-top: 5px solid #FFFFFF;
            margin-right: 5px;
        }
        QPushButton {
            background-color: #2C2D33;
            color: #FFFFFF;
            border: 1px solid #3A3B41;
            border-radius: 4px;
            padding: 6px 12px;
        }
        QPushButton:hover {
            background-color: #3A3B41;
            border-color: #FF9500;
        }
        QPushButton:pressed {
            background-color: #484A52;
        }
        QPushButton#quickQtyBtn {
            background-color: #3A3B41;
            font-size: 12px;
        }
        QCheckBox {
            color: #FFFFFF;
        }
        QCheckBox::indicator {
            width: 18px;
            height: 18px;
            border-radius: 3px;
            border: 1px solid #3A3B41;
            background-color: #2C2D33;
        }
        QCheckBox::indicator:checked {
            background-color: #FF9500;
            border-color: #FF9500;
        }
    )");
}

void OrderDialog::setContract(const QString &instrumentId, 
                               const QString &instrumentName,
                               double lastPrice,
                               double tickSize,
                               int volumeMultiple,
                               double marginRatio)
{
    m_instrumentId = instrumentId;
    m_instrumentName = instrumentName;
    m_lastPrice = lastPrice;
    m_tickSize = tickSize;
    m_volumeMultiple = volumeMultiple;
    m_marginRatio = marginRatio;
    
    m_instrumentLabel->setText(QString("%1 %2").arg(instrumentId, instrumentName));
    m_priceLabel->setText(QString::number(lastPrice, 'f', 2));
    
    // Set price spinbox step
    m_priceSpinBox->setSingleStep(tickSize);
    m_stopPriceSpinBox->setSingleStep(tickSize);
    m_takeProfitSpinBox->setSingleStep(tickSize);
    m_stopLossSpinBox->setSingleStep(tickSize);
    
    // Set default price
    if (lastPrice > 0) {
        m_priceSpinBox->setValue(lastPrice);
    }
    
    updateCalculations();
}

void OrderDialog::setAccount(double available, double margin, double frozenMargin)
{
    m_available = available;
    m_margin = margin;
    m_frozenMargin = frozenMargin;
    
    m_availableLabel->setText(QString::number(available, 'f', 2));
    updateCalculations();
}

void OrderDialog::setCurrentPosition(int longPos, int shortPos)
{
    m_longPosition = longPos;
    m_shortPosition = shortPos;
    
    m_longPosLabel->setText(QString::number(longPos));
    m_shortPosLabel->setText(QString::number(shortPos));
    
    updatePositionInfo();
}

void OrderDialog::onOrderTypeChanged(int index)
{
    OrderType type = static_cast<OrderType>(index);
    
    // Enable/disable price inputs based on order type
    switch (type) {
    case OrderType::Market:
        m_priceSpinBox->setEnabled(false);
        m_stopPriceSpinBox->setEnabled(false);
        break;
    case OrderType::Limit:
        m_priceSpinBox->setEnabled(true);
        m_stopPriceSpinBox->setEnabled(false);
        break;
    case OrderType::Stop:
        m_priceSpinBox->setEnabled(false);
        m_stopPriceSpinBox->setEnabled(true);
        break;
    case OrderType::StopLimit:
        m_priceSpinBox->setEnabled(true);
        m_stopPriceSpinBox->setEnabled(true);
        break;
    case OrderType::Iceberg:
        m_priceSpinBox->setEnabled(true);
        m_stopPriceSpinBox->setEnabled(false);
        break;
    }
    
    updateCalculations();
}

void OrderDialog::onDirectionChanged(int index)
{
    Q_UNUSED(index);
    updateCalculations();
    updateRiskDisplay();
}

void OrderDialog::onOpenCloseChanged(int index)
{
    Q_UNUSED(index);
    updatePositionInfo();
    updateCalculations();
}

void OrderDialog::onQuantityChanged(int value)
{
    Q_UNUSED(value);
    updateCalculations();
    updateRiskDisplay();
}

void OrderDialog::onPriceChanged(double value)
{
    Q_UNUSED(value);
    updateCalculations();
    updateRiskDisplay();
}

void OrderDialog::onCalculateClicked()
{
    updateCalculations();
    updateRiskDisplay();
}

void OrderDialog::onSubmitClicked()
{
    QString errorMsg;
    if (!validateOrder(errorMsg)) {
        QMessageBox::warning(this, "Validation Error", errorMsg);
        return;
    }
    
    OrderParams params = getOrderParams();
    emit orderSubmitted(params);
    
    QMessageBox::information(this, "Order Submitted", 
        QString("Order submitted successfully!\n\n"
                "Contract: %1\n"
                "Type: %2\n"
                "Direction: %3\n"
                "Quantity: %4\n"
                "Price: %5")
        .arg(params.instrumentId)
        .arg(static_cast<int>(params.orderType))
        .arg(static_cast<int>(params.direction))
        .arg(params.quantity)
        .arg(params.price, 0, 'f', 2));
    
    accept();
}

void OrderDialog::onCancelClicked()
{
    emit orderCancelled();
    reject();
}

void OrderDialog::onResetClicked()
{
    clearInputs();
}

void OrderDialog::updateCalculations()
{
    if (m_lastPrice <= 0) return;
    
    double price = m_priceSpinBox->value();
    if (price <= 0) price = m_lastPrice;
    
    int quantity = m_quantitySpinBox->value();
    
    // Calculate margin
    double margin = price * quantity * m_volumeMultiple * m_marginRatio;
    m_marginLabel->setText(QString::number(margin, 'f', 2));
    
    // Estimate commission (simplified: 0.01% of contract value)
    double contractValue = price * quantity * m_volumeMultiple;
    double commission = contractValue * 0.0001;
    m_commissionLabel->setText(QString::number(commission, 'f', 2));
    
    // Total required
    double total = margin + commission;
    m_totalLabel->setText(QString::number(total, 'f', 2));
    
    // Check if sufficient
    if (total > m_available) {
        m_totalLabel->setStyleSheet("font-weight: bold; color: #FF3B30;");
    } else {
        m_totalLabel->setStyleSheet("font-weight: bold; color: #34C759;");
    }
}

void OrderDialog::updateRiskDisplay()
{
    if (m_lastPrice <= 0) return;
    
    double price = m_priceSpinBox->value();
    if (price <= 0) price = m_lastPrice;
    
    int quantity = m_quantitySpinBox->value();
    
    // Calculate risk if stop loss is enabled
    if (m_enableStopLossCheck->isChecked()) {
        double stopLoss = m_stopLossSpinBox->value();
        double riskPoints = 0;
        
        PositionDirection dir = static_cast<PositionDirection>(m_directionCombo->currentIndex());
        if (dir == PositionDirection::Long) {
            riskPoints = price - stopLoss;
        } else {
            riskPoints = stopLoss - price;
        }
        
        double riskAmount = riskPoints * quantity * m_volumeMultiple;
        double riskRatio = (riskAmount / m_available) * 100.0;
        
        m_riskAmountLabel->setText(QString::number(std::abs(riskAmount), 'f', 2));
        m_riskRatioLabel->setText(QString::number(std::abs(riskRatio), 'f', 2) + "%");
        
        // Color based on risk level
        if (std::abs(riskRatio) > 10.0) {
            m_riskRatioLabel->setStyleSheet("color: #FF3B30;");
        } else if (std::abs(riskRatio) > 5.0) {
            m_riskRatioLabel->setStyleSheet("color: #FF9500;");
        } else {
            m_riskRatioLabel->setStyleSheet("color: #34C759;");
        }
    } else {
        m_riskAmountLabel->setText("--");
        m_riskRatioLabel->setText("--");
        m_riskRatioLabel->setStyleSheet("color: #8E8E93;");
    }
    
    // Calculate profit if take profit is enabled
    if (m_enableTakeProfitCheck->isChecked()) {
        double takeProfit = m_takeProfitSpinBox->value();
        double profitPoints = 0;
        
        PositionDirection dir = static_cast<PositionDirection>(m_directionCombo->currentIndex());
        if (dir == PositionDirection::Long) {
            profitPoints = takeProfit - price;
        } else {
            profitPoints = price - takeProfit;
        }
        
        double profitAmount = profitPoints * quantity * m_volumeMultiple;
        double profitRatio = (profitAmount / m_available) * 100.0;
        
        m_profitAmountLabel->setText(QString::number(std::abs(profitAmount), 'f', 2));
        m_profitRatioLabel->setText(QString::number(std::abs(profitRatio), 'f', 2) + "%");
    } else {
        m_profitAmountLabel->setText("--");
        m_profitRatioLabel->setText("--");
    }
}

void OrderDialog::updatePositionInfo()
{
    OpenCloseFlag ocFlag = static_cast<OpenCloseFlag>(m_openCloseCombo->currentIndex());
    
    // Update available close quantity based on position
    if (ocFlag != OpenCloseFlag::Open) {
        PositionDirection dir = static_cast<PositionDirection>(m_directionCombo->currentIndex());
        int maxCloseQty = 0;
        
        if (dir == PositionDirection::Long) {
            // Selling to close long position
            maxCloseQty = m_longPosition;
        } else {
            // Buying to close short position
            maxCloseQty = m_shortPosition;
        }
        
        m_quantitySpinBox->setMaximum(std::max(1, maxCloseQty));
    } else {
        m_quantitySpinBox->setMaximum(99999);
    }
}

void OrderDialog::clearInputs()
{
    m_orderTypeCombo->setCurrentIndex(0);
    m_directionCombo->setCurrentIndex(0);
    m_openCloseCombo->setCurrentIndex(0);
    m_quantitySpinBox->setValue(1);
    
    if (m_lastPrice > 0) {
        m_priceSpinBox->setValue(m_lastPrice);
    }
    
    m_stopPriceSpinBox->setValue(0);
    m_enableTakeProfitCheck->setChecked(false);
    m_enableStopLossCheck->setChecked(false);
    m_takeProfitSpinBox->setValue(0);
    m_stopLossSpinBox->setValue(0);
    
    updateCalculations();
}

OrderDialog::OrderParams OrderDialog::getOrderParams() const
{
    OrderParams params;
    params.instrumentId = m_instrumentId;
    params.orderType = static_cast<OrderType>(m_orderTypeCombo->currentIndex());
    params.direction = static_cast<PositionDirection>(m_directionCombo->currentIndex());
    params.openClose = static_cast<OpenCloseFlag>(m_openCloseCombo->currentIndex());
    params.quantity = m_quantitySpinBox->value();
    params.price = m_priceSpinBox->value();
    params.stopPrice = m_stopPriceSpinBox->value();
    params.takeProfitPrice = m_takeProfitSpinBox->value();
    params.stopLossPrice = m_stopLossSpinBox->value();
    params.enableTakeProfit = m_enableTakeProfitCheck->isChecked();
    params.enableStopLoss = m_enableStopLossCheck->isChecked();
    
    return params;
}

bool OrderDialog::validateOrder(QString &errorMsg)
{
    if (m_instrumentId.isEmpty()) {
        errorMsg = "Please select a contract first.";
        return false;
    }
    
    OrderType type = static_cast<OrderType>(m_orderTypeCombo->currentIndex());
    
    // Validate price for limit orders
    if (type == OrderType::Limit || type == OrderType::StopLimit || type == OrderType::Iceberg) {
        if (m_priceSpinBox->value() <= 0) {
            errorMsg = "Please enter a valid price.";
            return false;
        }
    }
    
    // Validate stop price for stop orders
    if (type == OrderType::Stop || type == OrderType::StopLimit) {
        if (m_stopPriceSpinBox->value() <= 0) {
            errorMsg = "Please enter a valid trigger price.";
            return false;
        }
    }
    
    // Validate quantity
    if (m_quantitySpinBox->value() <= 0) {
        errorMsg = "Please enter a valid quantity.";
        return false;
    }
    
    // Validate margin requirement
    double price = m_priceSpinBox->value();
    if (price <= 0) price = m_lastPrice;
    double margin = price * m_quantitySpinBox->value() * m_volumeMultiple * m_marginRatio;
    
    if (margin > m_available) {
        errorMsg = QString("Insufficient margin. Required: %1, Available: %2")
                   .arg(QString::number(margin, 'f', 2))
                   .arg(QString::number(m_available, 'f', 2));
        return false;
    }
    
    // Validate close position
    OpenCloseFlag ocFlag = static_cast<OpenCloseFlag>(m_openCloseCombo->currentIndex());
    if (ocFlag != OpenCloseFlag::Open) {
        PositionDirection dir = static_cast<PositionDirection>(m_directionCombo->currentIndex());
        int qty = m_quantitySpinBox->value();
        
        if (dir == PositionDirection::Long && qty > m_longPosition) {
            errorMsg = QString("Cannot close more than current long position (%1).")
                       .arg(m_longPosition);
            return false;
        }
        if (dir == PositionDirection::Short && qty > m_shortPosition) {
            errorMsg = QString("Cannot close more than current short position (%1).")
                       .arg(m_shortPosition);
            return false;
        }
    }
    
    return true;
}
