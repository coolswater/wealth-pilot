#include "OrderDialog.h"
#include "presentation/components/StyleHelper.h"
#include "core/config/Tokens.h"
#include <QMessageBox>
#include <cmath>

OrderDialog::OrderDialog(QWidget *parent)
    : QDialog(parent)
    , m_lastPrice(0.0), m_tickSize(0.01), m_volumeMultiple(1), m_marginRatio(0.1)
    , m_available(0.0), m_margin(0.0), m_frozenMargin(0.0)
    , m_longPosition(0), m_shortPosition(0)
{
    initUI();
    initConnections();
}

OrderDialog::~OrderDialog()
{
}

void OrderDialog::initUI()
{
    setWindowTitle("下单");
    setMinimumSize(480, 640);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    

    
    // Header
    QFrame *headerFrame = new QFrame(this);
    QHBoxLayout *headerLayout = new QHBoxLayout(headerFrame);
    
    m_instrumentLabel = new QLabel("选择合约", this);

    
    m_priceLabel = new QLabel("--", this);

    
    m_changeLabel = new QLabel("--", this);

    
    headerLayout->addWidget(m_instrumentLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(m_priceLabel);
    headerLayout->addSpacing(10);
    headerLayout->addWidget(m_changeLabel);
    mainLayout->addWidget(headerFrame);
    
    // Order Type Group
    QGroupBox *orderTypeGroup = new QGroupBox("订单类型", this);

    QGridLayout *orderTypeLayout = new QGridLayout(orderTypeGroup);
    orderTypeLayout->setSpacing(10);
    
    orderTypeLayout->addWidget(new QLabel("类型:", this), 0, 0);
    m_orderTypeCombo = new QComboBox(this);
    m_orderTypeCombo->addItems({"市价", "限价", "止损", "止损限价", "冰山"});

    orderTypeLayout->addWidget(m_orderTypeCombo, 0, 1);
    
    orderTypeLayout->addWidget(new QLabel("方向:", this), 1, 0);
    m_directionCombo = new QComboBox(this);
    m_directionCombo->addItems({"买入/多头", "卖出/空头"});

    orderTypeLayout->addWidget(m_directionCombo, 1, 1);
    
    orderTypeLayout->addWidget(new QLabel("开平:", this), 2, 0);
    m_openCloseCombo = new QComboBox(this);
    m_openCloseCombo->addItems({"开仓", "平仓", "平今", "平昨"});

    orderTypeLayout->addWidget(m_openCloseCombo, 2, 1);
    
    mainLayout->addWidget(orderTypeGroup);
    
    // Price & Quantity Group
    QGroupBox *priceGroup = new QGroupBox("价格与数量", this);

    QGridLayout *priceLayout = new QGridLayout(priceGroup);
    priceLayout->setSpacing(10);
    
    priceLayout->addWidget(new QLabel("价格:", this), 0, 0);
    m_priceSpinBox = new QDoubleSpinBox(this);
    m_priceSpinBox->setDecimals(2);
    m_priceSpinBox->setMinimum(0.0);
    m_priceSpinBox->setMaximum(999999.99);
    m_priceSpinBox->setSingleStep(0.01);

    priceLayout->addWidget(m_priceSpinBox, 0, 1);
    
    priceLayout->addWidget(new QLabel("触发价:", this), 1, 0);
    m_stopPriceSpinBox = new QDoubleSpinBox(this);
    m_stopPriceSpinBox->setDecimals(2);
    m_stopPriceSpinBox->setMinimum(0.0);
    m_stopPriceSpinBox->setMaximum(999999.99);
    m_stopPriceSpinBox->setSingleStep(0.01);
    m_stopPriceSpinBox->setEnabled(false);

    priceLayout->addWidget(m_stopPriceSpinBox, 1, 1);
    
    priceLayout->addWidget(new QLabel("数量:", this), 2, 0);
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
        StyleHelper::setSecondaryButton(btn);
        connect(btn, &QPushButton::clicked, this, [this, qty]() { m_quantitySpinBox->setValue(qty); });
        quickQtyLayout->addWidget(btn);
    }
    priceLayout->addLayout(quickQtyLayout, 3, 0, 1, 2);
    mainLayout->addWidget(priceGroup);
    
    // Calculation Group
    QGroupBox *calcGroup = new QGroupBox("计算", this);

    QGridLayout *calcLayout = new QGridLayout(calcGroup);
    calcLayout->setSpacing(10);
    
    calcLayout->addWidget(new QLabel("保证金:", this), 0, 0);
    m_marginLabel = new QLabel("0.00", this);

    calcLayout->addWidget(m_marginLabel, 0, 1);
    
    calcLayout->addWidget(new QLabel("手续费:", this), 1, 0);
    m_commissionLabel = new QLabel("0.00", this);

    calcLayout->addWidget(m_commissionLabel, 1, 1);
    
    calcLayout->addWidget(new QLabel("合计:", this), 2, 0);
    m_totalLabel = new QLabel("0.00", this);

    calcLayout->addWidget(m_totalLabel, 2, 1);
    
    m_calculateBtn = new QPushButton("计算", this);
    StyleHelper::setSecondaryButton(m_calculateBtn);
    calcLayout->addWidget(m_calculateBtn, 3, 0, 1, 2);
    mainLayout->addWidget(calcGroup);
    
    // Stop Loss & Take Profit Group
    QGroupBox *slTpGroup = new QGroupBox("止盈止损", this);

    QGridLayout *slTpLayout = new QGridLayout(slTpGroup);
    slTpLayout->setSpacing(10);
    
    m_enableTakeProfitCheck = new QCheckBox("止盈:", this);

    slTpLayout->addWidget(m_enableTakeProfitCheck, 0, 0);
    m_takeProfitSpinBox = new QDoubleSpinBox(this);
    m_takeProfitSpinBox->setDecimals(2);
    m_takeProfitSpinBox->setMinimum(0.0);
    m_takeProfitSpinBox->setMaximum(999999.99);
    m_takeProfitSpinBox->setEnabled(false);

    slTpLayout->addWidget(m_takeProfitSpinBox, 0, 1);
    
    m_enableStopLossCheck = new QCheckBox("止损:", this);

    slTpLayout->addWidget(m_enableStopLossCheck, 1, 0);
    m_stopLossSpinBox = new QDoubleSpinBox(this);
    m_stopLossSpinBox->setDecimals(2);
    m_stopLossSpinBox->setMinimum(0.0);
    m_stopLossSpinBox->setMaximum(999999.99);
    m_stopLossSpinBox->setEnabled(false);

    slTpLayout->addWidget(m_stopLossSpinBox, 1, 1);
    mainLayout->addWidget(slTpGroup);
    
    // Position & Account Info Group
    QGroupBox *infoGroup = new QGroupBox("持仓与账户", this);

    QGridLayout *infoLayout = new QGridLayout(infoGroup);
    infoLayout->setSpacing(10);
    
    infoLayout->addWidget(new QLabel("多头持仓:", this), 0, 0);
    m_longPosLabel = new QLabel("0", this);

    infoLayout->addWidget(m_longPosLabel, 0, 1);
    
    infoLayout->addWidget(new QLabel("空头持仓:", this), 1, 0);
    m_shortPosLabel = new QLabel("0", this);

    infoLayout->addWidget(m_shortPosLabel, 1, 1);
    
    infoLayout->addWidget(new QLabel("可用资金:", this), 2, 0);
    m_availableLabel = new QLabel("0.00", this);

    infoLayout->addWidget(m_availableLabel, 2, 1);
    mainLayout->addWidget(infoGroup);
    
    // Risk Display Group
    QGroupBox *riskGroup = new QGroupBox("风险分析", this);

    QGridLayout *riskLayout = new QGridLayout(riskGroup);
    riskLayout->setSpacing(10);
    
    riskLayout->addWidget(new QLabel("风险比例:", this), 0, 0);
    m_riskRatioLabel = new QLabel("0.00%", this);

    riskLayout->addWidget(m_riskRatioLabel, 0, 1);
    
    riskLayout->addWidget(new QLabel("风险金额:", this), 1, 0);
    m_riskAmountLabel = new QLabel("0.00", this);

    riskLayout->addWidget(m_riskAmountLabel, 1, 1);
    
    riskLayout->addWidget(new QLabel("盈利比例:", this), 2, 0);
    m_profitRatioLabel = new QLabel("0.00%", this);

    riskLayout->addWidget(m_profitRatioLabel, 2, 1);
    
    riskLayout->addWidget(new QLabel("盈利金额:", this), 3, 0);
    m_profitAmountLabel = new QLabel("0.00", this);

    riskLayout->addWidget(m_profitAmountLabel, 3, 1);
    mainLayout->addWidget(riskGroup);
    
    // Buttons
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);
    
    m_submitBtn = new QPushButton("提交订单", this);
    m_submitBtn->setMinimumHeight(40);
    StyleHelper::setPrimaryButton(m_submitBtn);
    
    m_cancelBtn = new QPushButton("取消", this);
    m_cancelBtn->setMinimumHeight(40);
    StyleHelper::setSecondaryButton(m_cancelBtn);
    
    m_resetBtn = new QPushButton("重置", this);
    m_resetBtn->setMinimumHeight(40);
    StyleHelper::setSecondaryButton(m_resetBtn);
    
    btnLayout->addWidget(m_resetBtn);
    btnLayout->addWidget(m_cancelBtn);
    btnLayout->addWidget(m_submitBtn);
    mainLayout->addLayout(btnLayout);
    mainLayout->addStretch();
    

}

void OrderDialog::initConnections()
{
    connect(m_orderTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &OrderDialog::onOrderTypeChanged);
    connect(m_directionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &OrderDialog::onDirectionChanged);
    connect(m_openCloseCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &OrderDialog::onOpenCloseChanged);
    connect(m_quantitySpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &OrderDialog::onQuantityChanged);
    connect(m_priceSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &OrderDialog::onPriceChanged);
    
    connect(m_enableTakeProfitCheck, &QCheckBox::toggled, m_takeProfitSpinBox, &QDoubleSpinBox::setEnabled);
    connect(m_enableStopLossCheck, &QCheckBox::toggled, m_stopLossSpinBox, &QDoubleSpinBox::setEnabled);
    
    connect(m_calculateBtn, &QPushButton::clicked, this, &OrderDialog::onCalculateClicked);
    connect(m_submitBtn, &QPushButton::clicked, this, &OrderDialog::onSubmitClicked);
    connect(m_cancelBtn, &QPushButton::clicked, this, &OrderDialog::onCancelClicked);
    connect(m_resetBtn, &QPushButton::clicked, this, &OrderDialog::onResetClicked);
}

void OrderDialog::setContract(const QString &instrumentId, const QString &instrumentName,
                               double lastPrice, double tickSize, int volumeMultiple, double marginRatio)
{
    m_instrumentId = instrumentId;
    m_instrumentName = instrumentName;
    m_lastPrice = lastPrice;
    m_tickSize = tickSize;
    m_volumeMultiple = volumeMultiple;
    m_marginRatio = marginRatio;
    
    m_instrumentLabel->setText(QString("%1 %2").arg(instrumentId, instrumentName));
    m_priceLabel->setText(QString::number(lastPrice, 'f', 2));
    
    m_priceSpinBox->setSingleStep(tickSize);
    m_stopPriceSpinBox->setSingleStep(tickSize);
    m_takeProfitSpinBox->setSingleStep(tickSize);
    m_stopLossSpinBox->setSingleStep(tickSize);
    
    if (lastPrice > 0) m_priceSpinBox->setValue(lastPrice);
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
    m_priceSpinBox->setEnabled(type != OrderType::Market && type != OrderType::Stop);
    m_stopPriceSpinBox->setEnabled(type == OrderType::Stop || type == OrderType::StopLimit);
    updateCalculations();
}

void OrderDialog::onDirectionChanged(int) { updateCalculations(); updateRiskDisplay(); }
void OrderDialog::onOpenCloseChanged(int) { updatePositionInfo(); updateCalculations(); }
void OrderDialog::onQuantityChanged(int) { updateCalculations(); updateRiskDisplay(); }
void OrderDialog::onPriceChanged(double) { updateCalculations(); updateRiskDisplay(); }
void OrderDialog::onCalculateClicked() { updateCalculations(); updateRiskDisplay(); }

void OrderDialog::onSubmitClicked()
{
    QString errorMsg;
    if (!validateOrder(errorMsg)) { QMessageBox::warning(this, "验证失败", errorMsg); return; }
    
    OrderParams params = getOrderParams();
    emit orderSubmitted(params);
    
    QMessageBox::information(this, "订单已提交",
        QString("合约: %1\n数量: %2\n价格: %3")
        .arg(params.instrumentId).arg(params.quantity).arg(params.price, 0, 'f', 2));
    accept();
}

void OrderDialog::onCancelClicked() { emit orderCancelled(); reject(); }
void OrderDialog::onResetClicked() { clearInputs(); }

void OrderDialog::updateCalculations()
{
    if (m_lastPrice <= 0) return;
    
    double price = m_priceSpinBox->value() > 0 ? m_priceSpinBox->value() : m_lastPrice;
    int quantity = m_quantitySpinBox->value();
    
    double margin = price * quantity * m_volumeMultiple * m_marginRatio;
    m_marginLabel->setText(QString::number(margin, 'f', 2));
    
    double commission = price * quantity * m_volumeMultiple * 0.0001;
    m_commissionLabel->setText(QString::number(commission, 'f', 2));
    
    double total = margin + commission;
    m_totalLabel->setText(QString::number(total, 'f', 2));

    // 使用属性选择器
    m_totalLabel->setProperty("status", total > m_available ? "danger" : "success");
    StyleHelper::refreshStyle(m_totalLabel);
}

void OrderDialog::updateRiskDisplay()
{
    if (m_lastPrice <= 0) return;
    
    double price = m_priceSpinBox->value() > 0 ? m_priceSpinBox->value() : m_lastPrice;
    int quantity = m_quantitySpinBox->value();
    
    if (m_enableStopLossCheck->isChecked()) {
        double stopLoss = m_stopLossSpinBox->value();
        double riskPoints = (m_directionCombo->currentIndex() == 0) ? (price - stopLoss) : (stopLoss - price);
        double riskAmount = std::abs(riskPoints * quantity * m_volumeMultiple);
        double riskRatio = m_available > 0 ? (riskAmount / m_available * 100.0) : 0;
        
        m_riskAmountLabel->setText(QString::number(riskAmount, 'f', 2));
        m_riskRatioLabel->setText(QString::number(riskRatio, 'f', 2) + "%");

        // 使用属性选择器
        QString riskStatus = riskRatio > 10 ? "danger" : (riskRatio > 5 ? "warning" : "success");
        m_riskRatioLabel->setProperty("status", riskStatus);
        StyleHelper::refreshStyle(m_riskRatioLabel);
    }
    else {
        m_riskAmountLabel->setText("--");
        m_riskRatioLabel->setText("--");
    }
    
    if (m_enableTakeProfitCheck->isChecked()) {
        double takeProfit = m_takeProfitSpinBox->value();
        double profitPoints = (m_directionCombo->currentIndex() == 0) ? (takeProfit - price) : (price - takeProfit);
        double profitAmount = std::abs(profitPoints * quantity * m_volumeMultiple);
        double profitRatio = m_available > 0 ? (profitAmount / m_available * 100.0) : 0;
        
        m_profitAmountLabel->setText(QString::number(profitAmount, 'f', 2));
        m_profitRatioLabel->setText(QString::number(profitRatio, 'f', 2) + "%");
    } else {
        m_profitAmountLabel->setText("--");
        m_profitRatioLabel->setText("--");
    }
}

void OrderDialog::updatePositionInfo()
{
    OpenCloseFlag ocFlag = static_cast<OpenCloseFlag>(m_openCloseCombo->currentIndex());
    if (ocFlag != OpenCloseFlag::Open) {
        int maxCloseQty = (m_directionCombo->currentIndex() == 0) ? m_longPosition : m_shortPosition;
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
    if (m_lastPrice > 0) m_priceSpinBox->setValue(m_lastPrice);
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
    if (m_instrumentId.isEmpty()) { errorMsg = "请先选择合约"; return false; }
    
    OrderType type = static_cast<OrderType>(m_orderTypeCombo->currentIndex());
    if ((type == OrderType::Limit || type == OrderType::StopLimit || type == OrderType::Iceberg) && m_priceSpinBox->value() <= 0) {
        errorMsg = "请输入有效价格"; return false;
    }
    if ((type == OrderType::Stop || type == OrderType::StopLimit) && m_stopPriceSpinBox->value() <= 0) {
        errorMsg = "请输入有效触发价"; return false;
    }
    if (m_quantitySpinBox->value() <= 0) { errorMsg = "请输入有效数量"; return false; }
    
    double price = m_priceSpinBox->value() > 0 ? m_priceSpinBox->value() : m_lastPrice;
    double margin = price * m_quantitySpinBox->value() * m_volumeMultiple * m_marginRatio;
    if (margin > m_available) {
        errorMsg = QString("保证金不足。需要: %1, 可用: %2").arg(margin, 0, 'f', 2).arg(m_available, 0, 'f', 2);
        return false;
    }
    
    return true;
}
