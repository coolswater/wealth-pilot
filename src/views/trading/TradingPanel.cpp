/**
 * @file TradingPanel.cpp
 * @brief 交易面板实现
 */

#include "TradingPanel.h"
#include "../widgets/OrderDialog.h"
#include "trading/TradingService.h"
#include "ui/components/PageStyles.h"
#include "utils/Logger.h"

#include <QMessageBox>
#include <QHeaderView>

TradingPanel::TradingPanel(QWidget *parent)
    : QWidget(parent)
    , m_orderDialog(new OrderDialog(this))
{
    initUI();
    initConnections();
}

TradingPanel::~TradingPanel()
{
}

void TradingPanel::initUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(16, 16, 16, 16);

    // 合约信息区域
    QGroupBox *infoGroup = new QGroupBox("合约信息", this);
    infoGroup->setStyleSheet(PageStyles::groupBox());
    QHBoxLayout *infoLayout = new QHBoxLayout(infoGroup);

    m_instrumentLabel = new QLabel("选择合约", this);
    m_instrumentLabel->setStyleSheet(PageStyles::titleText());

    m_priceLabel = new QLabel("--", this);
    m_priceLabel->setStyleSheet(PageStyles::valueText());

    m_availableLabel = new QLabel("可用: --", this);
    m_availableLabel->setStyleSheet(PageStyles::labelText());

    infoLayout->addWidget(m_instrumentLabel);
    infoLayout->addStretch();
    infoLayout->addWidget(m_priceLabel);
    infoLayout->addSpacing(20);
    infoLayout->addWidget(m_availableLabel);
    mainLayout->addWidget(infoGroup);

    // 快捷下单按钮区域
    QGroupBox *orderGroup = new QGroupBox("快捷下单", this);
    orderGroup->setStyleSheet(PageStyles::groupBox());
    QGridLayout *orderLayout = new QGridLayout(orderGroup);
    orderLayout->setSpacing(10);

    m_buyOpenBtn = new QPushButton("买入开仓", this);
    m_buyOpenBtn->setMinimumHeight(50);
    m_buyOpenBtn->setStyleSheet(QString(R"(
        QPushButton {
            background-color: %1;
            color: white;
            border: none;
            border-radius: 6px;
            font-weight: bold;
            font-size: 14px;
        }
        QPushButton:hover {
            background-color: %2;
        }
    )").arg("#EF4444", "#DC2626"));

    m_sellOpenBtn = new QPushButton("卖出开仓", this);
    m_sellOpenBtn->setMinimumHeight(50);
    m_sellOpenBtn->setStyleSheet(QString(R"(
        QPushButton {
            background-color: %1;
            color: white;
            border: none;
            border-radius: 6px;
            font-weight: bold;
            font-size: 14px;
        }
        QPushButton:hover {
            background-color: %2;
        }
    )").arg("#22C55E", "#16A34A"));

    m_buyCloseBtn = new QPushButton("买入平仓", this);
    m_buyCloseBtn->setMinimumHeight(50);
    m_buyCloseBtn->setStyleSheet(PageStyles::secondaryButton());

    m_sellCloseBtn = new QPushButton("卖出平仓", this);
    m_sellCloseBtn->setMinimumHeight(50);
    m_sellCloseBtn->setStyleSheet(PageStyles::secondaryButton());

    orderLayout->addWidget(m_buyOpenBtn, 0, 0);
    orderLayout->addWidget(m_sellOpenBtn, 0, 1);
    orderLayout->addWidget(m_buyCloseBtn, 1, 0);
    orderLayout->addWidget(m_sellCloseBtn, 1, 1);
    mainLayout->addWidget(orderGroup);

    // 委托列表区域
    QGroupBox *listGroup = new QGroupBox("当日委托", this);
    listGroup->setStyleSheet(PageStyles::groupBox());
    QVBoxLayout *listLayout = new QVBoxLayout(listGroup);

    m_orderTable = new QTableWidget(0, 6, this);
    m_orderTable->setHorizontalHeaderLabels({"合约", "方向", "价格", "数量", "成交", "状态"});
    m_orderTable->horizontalHeader()->setStretchLastSection(true);
    m_orderTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_orderTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_orderTable->setStyleSheet(PageStyles::table());
    listLayout->addWidget(m_orderTable);

    m_statusLabel = new QLabel("就绪", this);
    m_statusLabel->setStyleSheet(PageStyles::labelText());
    listLayout->addWidget(m_statusLabel);

    mainLayout->addWidget(listGroup);

    // 设置页面背景
    setStyleSheet(PageStyles::pageBackground());
}

void TradingPanel::initConnections()
{
    // 按钮信号
    connect(m_buyOpenBtn, &QPushButton::clicked, this, [this]() {
        showOrderDialog(PositionDirection::Long, OpenCloseFlag::Open);
    });
    connect(m_sellOpenBtn, &QPushButton::clicked, this, [this]() {
        showOrderDialog(PositionDirection::Short, OpenCloseFlag::Open);
    });
    connect(m_buyCloseBtn, &QPushButton::clicked, this, [this]() {
        showOrderDialog(PositionDirection::Long, OpenCloseFlag::Close);
    });
    connect(m_sellCloseBtn, &QPushButton::clicked, this, [this]() {
        showOrderDialog(PositionDirection::Short, OpenCloseFlag::Close);
    });

    // 订单对话框信号
    connect(m_orderDialog, &OrderDialog::orderSubmitted,
            this, &TradingPanel::onOrderDialogSubmitted);
    connect(m_orderDialog, &OrderDialog::orderCancelled,
            this, &TradingPanel::onOrderDialogCancelled);

    // 交易服务信号
    connect(&TradingService::instance(), &TradingService::orderSubmitted,
            this, &TradingPanel::onTradingServiceOrderSubmitted);
    connect(&TradingService::instance(), &TradingService::orderFilled,
            this, &TradingPanel::onTradingServiceOrderFilled);
    connect(&TradingService::instance(), &TradingService::orderRejected,
            this, &TradingPanel::onTradingServiceOrderRejected);
    connect(&TradingService::instance(), &TradingService::tradingLog,
            this, &TradingPanel::onTradingServiceTradingLog);
}

void TradingPanel::setCurrentInstrument(const QString &instrumentId,
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

    // 更新订单对话框
    m_orderDialog->setContract(instrumentId, instrumentName, lastPrice,
                               tickSize, volumeMultiple, marginRatio);

    updatePositionInfo();
    updateAccountInfo();
}

void TradingPanel::refresh()
{
    updateAccountInfo();
    updatePositionInfo();
    updateOrderTable();
}

void TradingPanel::updateAccountInfo()
{
    auto accountInfo = TradingService::instance().getAccountInfo();
    m_availableLabel->setText(QString("可用: %1").arg(accountInfo.available, 0, 'f', 2));
    m_orderDialog->setAccount(accountInfo.available, accountInfo.margin, accountInfo.frozenMargin);
}

void TradingPanel::updatePositionInfo()
{
    auto longPos = TradingService::instance().getPosition(m_instrumentId, PositionDirection::Long);
    auto shortPos = TradingService::instance().getPosition(m_instrumentId, PositionDirection::Short);

    m_longPosition = longPos ? longPos->volume : 0;
    m_shortPosition = shortPos ? shortPos->volume : 0;

    m_orderDialog->setCurrentPosition(m_longPosition, m_shortPosition);

    // 更新平仓按钮状态
    m_buyCloseBtn->setEnabled(m_shortPosition > 0);
    m_sellCloseBtn->setEnabled(m_longPosition > 0);
}

void TradingPanel::updateOrderTable()
{
    auto orders = TradingService::instance().getActiveOrders();
    m_orderTable->setRowCount(orders.size());

    for (int i = 0; i < orders.size(); ++i) {
        const auto &order = orders[i];

        m_orderTable->setItem(i, 0, new QTableWidgetItem(order.instrumentId));
        m_orderTable->setItem(i, 1, new QTableWidgetItem(
            TradingUtils::directionToString(order.direction)));
        m_orderTable->setItem(i, 2, new QTableWidgetItem(
            QString::number(order.price, 'f', 2)));
        m_orderTable->setItem(i, 3, new QTableWidgetItem(
            QString::number(order.volume)));
        m_orderTable->setItem(i, 4, new QTableWidgetItem(
            QString::number(order.filledVolume)));
        m_orderTable->setItem(i, 5, new QTableWidgetItem(
            TradingUtils::statusToString(order.status)));
    }
}

void TradingPanel::showOrderDialog(PositionDirection direction, OpenCloseFlag openClose)
{
    if (m_instrumentId.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择合约");
        return;
    }

    // 设置方向
    m_orderDialog->setContract(m_instrumentId, m_instrumentName, m_lastPrice,
                               m_tickSize, m_volumeMultiple, m_marginRatio);

    m_orderDialog->show();
    m_orderDialog->raise();
    m_orderDialog->activateWindow();
}

void TradingPanel::onBuyOpenClicked()
{
    showOrderDialog(PositionDirection::Long, OpenCloseFlag::Open);
}

void TradingPanel::onSellOpenClicked()
{
    showOrderDialog(PositionDirection::Short, OpenCloseFlag::Open);
}

void TradingPanel::onBuyCloseClicked()
{
    showOrderDialog(PositionDirection::Long, OpenCloseFlag::Close);
}

void TradingPanel::onSellCloseClicked()
{
    showOrderDialog(PositionDirection::Short, OpenCloseFlag::Close);
}

void TradingPanel::onOrderDialogSubmitted(const OrderDialog::OrderParams &params)
{
    // 转换为 OrderRequest
    OrderRequest request;
    request.instrumentId = params.instrumentId;
    request.direction = params.direction == PositionDirection::Long
        ? TradeDirection::Buy : TradeDirection::Sell;
    request.openClose = params.openClose;
    request.orderType = params.orderType;
    request.price = params.price;
    request.volume = params.quantity;
    request.stopPrice = params.stopPrice;

    // 提交订单
    QString orderId = TradingService::instance().submitOrder(request);

    if (!orderId.isEmpty()) {
        m_statusLabel->setText(QString("订单已提交: %1").arg(orderId));
        emit orderSubmitted(orderId);
    } else {
        m_statusLabel->setText("订单提交失败");
    }
}

void TradingPanel::onOrderDialogCancelled()
{
    m_statusLabel->setText("已取消下单");
}

void TradingPanel::onTradingServiceOrderSubmitted(const QString &orderId)
{
    LOG_INFO(QString("Order submitted: %1").arg(orderId));
    updateOrderTable();
}

void TradingPanel::onTradingServiceOrderFilled(const QString &orderId)
{
    LOG_INFO(QString("Order filled: %1").arg(orderId));
    m_statusLabel->setText(QString("订单成交: %1").arg(orderId));
    updateOrderTable();
    updatePositionInfo();
}

void TradingPanel::onTradingServiceOrderRejected(const QString &orderId, const QString &reason)
{
    LOG_WARNING(QString("Order rejected: %1, reason: %2").arg(orderId, reason));
    m_statusLabel->setText(QString("订单被拒绝: %1").arg(reason));
    updateOrderTable();
}

void TradingPanel::onTradingServiceTradingLog(const QString &log, int level)
{
    Q_UNUSED(level)
    LOG_DEBUG(QString("Trading log: %1").arg(log));
}
