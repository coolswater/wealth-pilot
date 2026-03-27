/**
 * @file FuturesQuotesPage.cpp
 * @brief 期货行情页面实现
 */
#include "FuturesQuotesPage.h"
#include "utils/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QTimer>

#include <services/CTPService.h>

class FuturesQuotesPage::Impl {
public:
    QTableWidget* quoteTable = nullptr;
    QPushButton* connectBtn = nullptr;
    QPushButton* subscribeBtn = nullptr;
    QLineEdit* instrumentInput = nullptr;
    QLabel* statusLabel = nullptr;
    QComboBox* exchangeCombo = nullptr;

    bool isConnected = false;
    QMap<QString, int> instrumentRowMap;  // 合约代码到行号的映射
};

FuturesQuotesPage::FuturesQuotesPage(QWidget* parent)
    : BasePage(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();

    // 连接CTP服务信号
    connect(CTPService::instance(), &CTPService::marketDataReceived,
            this, &FuturesQuotesPage::onMarketDataReceived);
    connect(CTPService::instance(), &CTPService::marketConnected,
            this, &FuturesQuotesPage::onConnectionStateChanged);
    connect(CTPService::instance(), &CTPService::marketDisconnected,
            this, &FuturesQuotesPage::onConnectionStateChanged);

    LOG_INFO("FuturesQuotesPage created");
}

FuturesQuotesPage::~FuturesQuotesPage() = default;

QString FuturesQuotesPage::pageId() const
{
 return QStringLiteral("FuturesQuotesPage");
}

void FuturesQuotesPage::initializePage()
{

}

void FuturesQuotesPage::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(20);

    // 标题
    QLabel* titleLabel = new QLabel("期货行情", this);
    titleLabel->setStyleSheet("font-size: 28px; font-weight: 700; color: white;");
    mainLayout->addWidget(titleLabel);

    // 工具栏
    createToolbar();

    // 行情表格
    createQuoteTable();

    mainLayout->addStretch();
}

void FuturesQuotesPage::createToolbar()
{
    QHBoxLayout* toolbarLayout = new QHBoxLayout();
    toolbarLayout->setSpacing(12);

    // 连接按钮
    d->connectBtn = new QPushButton("连接行情", this);
    d->connectBtn->setFixedSize(100, 36);
    d->connectBtn->setCursor(Qt::PointingHandCursor);
    d->connectBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #10B981;
            color: white;
            border: none;
            border-radius: 8px;
            font-weight: 500;
        }
        QPushButton:hover {
            background-color: #059669;
        }
        QPushButton:disabled {
            background-color: #6B7280;
        }
    )");
    connect(d->connectBtn, &QPushButton::clicked, this, &FuturesQuotesPage::onConnectClicked);
    toolbarLayout->addWidget(d->connectBtn);

    // 交易所选择
    d->exchangeCombo = new QComboBox(this);
    d->exchangeCombo->addItem("上海期货交易所", "SHFE");
    d->exchangeCombo->addItem("大连商品交易所", "DCE");
    d->exchangeCombo->addItem("郑州商品交易所", "ZCE");
    d->exchangeCombo->addItem("中国金融期货交易所", "CFFEX");
    d->exchangeCombo->setFixedWidth(180);
    d->exchangeCombo->setStyleSheet(R"(
        QComboBox {
            background-color: rgba(255, 255, 255, 0.05);
            border: 1px solid rgba(255, 255, 255, 0.1);
            border-radius: 8px;
            padding: 8px 12px;
            color: white;
        }
    )");
    toolbarLayout->addWidget(d->exchangeCombo);

    // 合约代码输入
    d->instrumentInput = new QLineEdit(this);
    d->instrumentInput->setPlaceholderText("输入合约代码，如: rb2505");
    d->instrumentInput->setFixedWidth(200);
    d->instrumentInput->setStyleSheet(R"(
        QLineEdit {
            background-color: rgba(255, 255, 255, 0.05);
            border: 1px solid rgba(255, 255, 255, 0.1);
            border-radius: 8px;
            padding: 8px 12px;
            color: white;
        }
        QLineEdit:focus {
            border-color: #3B82F6;
        }
    )");
    toolbarLayout->addWidget(d->instrumentInput);

    // 订阅按钮
    d->subscribeBtn = new QPushButton("订阅", this);
    d->subscribeBtn->setFixedSize(80, 36);
    d->subscribeBtn->setCursor(Qt::PointingHandCursor);
    d->subscribeBtn->setEnabled(false);
    d->subscribeBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #3B82F6;
            color: white;
            border: none;
            border-radius: 8px;
            font-weight: 500;
        }
        QPushButton:hover {
            background-color: #2563EB;
        }
        QPushButton:disabled {
            background-color: #6B7280;
        }
    )");
    connect(d->subscribeBtn, &QPushButton::clicked, this, &FuturesQuotesPage::onSubscribeClicked);
    toolbarLayout->addWidget(d->subscribeBtn);

    toolbarLayout->addStretch();

    // 状态标签
    d->statusLabel = new QLabel("未连接", this);
    d->statusLabel->setStyleSheet("color: #EF4444; font-size: 14px;");
    toolbarLayout->addWidget(d->statusLabel);

    layout()->addItem(toolbarLayout);
}

void FuturesQuotesPage::createQuoteTable()
{
    d->quoteTable = new QTableWidget(this);
    d->quoteTable->setColumnCount(10);
    d->quoteTable->setHorizontalHeaderLabels({
        "合约", "最新价", "涨跌", "涨跌幅", "买一价", "买一量",
        "卖一价", "卖一量", "成交量", "持仓量"
    });

    // 设置表格样式
    d->quoteTable->setStyleSheet(R"(
        QTableWidget {
            background-color: rgba(255, 255, 255, 0.03);
            border: 1px solid rgba(255, 255, 255, 0.1);
            border-radius: 12px;
            gridline-color: rgba(255, 255, 255, 0.05);
        }
        QTableWidget::item {
            padding: 12px;
            color: white;
        }
        QHeaderView::section {
            background-color: rgba(255, 255, 255, 0.05);
            color: #9CA3AF;
            padding: 12px;
            border: none;
            font-weight: 500;
        }
    )");

    d->quoteTable->horizontalHeader()->setStretchLastSection(true);
    d->quoteTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    d->quoteTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    d->quoteTable->setAlternatingRowColors(true);
    d->quoteTable->verticalHeader()->setVisible(false);

    connect(d->quoteTable, &QTableWidget::cellClicked,
            this, &FuturesQuotesPage::onQuoteItemClicked);

    layout()->addWidget(d->quoteTable);
}

void FuturesQuotesPage::onConnectClicked()
{
    if (!d->isConnected) {
        // 连接行情服务器
        if (CTPService::instance()->connectMarket()) {
            LOG_INFO("Market connection initiated");
        } else {
            QMessageBox::warning(this, "连接失败", "无法连接到行情服务器");
        }
    } else {
        // 断开连接
        CTPService::instance()->disconnect();
    }
}

void FuturesQuotesPage::onSubscribeClicked()
{
    QString instrument = d->instrumentInput->text().trimmed();
    if (instrument.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入合约代码");
        return;
    }

    // 订阅合约
    CTPService::instance()->subscribeMarketData({instrument});
    d->instrumentInput->clear();

    LOG_INFO(QString("Subscribed to instrument: %1").arg(instrument));
}

void FuturesQuotesPage::onMarketDataReceived(const FuturesQuote& quote)
{
    updateQuoteDisplay(quote);
}

void FuturesQuotesPage::updateQuoteDisplay(const FuturesQuote& quote)
{

    LOG_INFO(QString("Subscribed to instrument: %1").arg(quote.lastPrice));
    int row;

    // 检查是否已存在该合约
    if (d->instrumentRowMap.contains(quote.instrumentId)) {
        row = d->instrumentRowMap[quote.instrumentId];
    } else {
        // 添加新行
        row = d->quoteTable->rowCount();
        d->quoteTable->insertRow(row);
        d->instrumentRowMap[quote.instrumentId] = row;
    }

    // 计算涨跌
    double change = quote.lastPrice - quote.preSettlementPrice;
    double changePercent = (change / quote.preSettlementPrice) * 100;

    // 设置单元格数据
    d->quoteTable->setItem(row, 0, new QTableWidgetItem(quote.instrumentId));
    d->quoteTable->setItem(row, 1, new QTableWidgetItem(QString::number(quote.lastPrice, 'f', 2)));

    QTableWidgetItem* changeItem = new QTableWidgetItem(QString::number(change, 'f', 2));
    changeItem->setForeground(change >= 0 ? QColor("#10B981") : QColor("#EF4444"));
    d->quoteTable->setItem(row, 2, changeItem);

    QTableWidgetItem* changePercentItem = new QTableWidgetItem(QString("%1%").arg(changePercent, 0, 'f', 2));
    changePercentItem->setForeground(change >= 0 ? QColor("#10B981") : QColor("#EF4444"));
    d->quoteTable->setItem(row, 3, changePercentItem);

    d->quoteTable->setItem(row, 4, new QTableWidgetItem(QString::number(quote.bidPrice1, 'f', 2)));
    d->quoteTable->setItem(row, 5, new QTableWidgetItem(QString::number(quote.bidVolume1)));
    d->quoteTable->setItem(row, 6, new QTableWidgetItem(QString::number(quote.askPrice1, 'f', 2)));
    d->quoteTable->setItem(row, 7, new QTableWidgetItem(QString::number(quote.askVolume1)));
    d->quoteTable->setItem(row, 8, new QTableWidgetItem(QString::number(quote.volume)));
    d->quoteTable->setItem(row, 9, new QTableWidgetItem(QString::number(quote.openInterest)));
}

void FuturesQuotesPage::onConnectionStateChanged()
{
    bool connected = CTPService::instance()->isMarketConnected();
    setConnectionStatus(connected);
}

void FuturesQuotesPage::setConnectionStatus(bool connected)
{
    d->isConnected = connected;

    if (connected) {
        d->connectBtn->setText("断开");
        d->connectBtn->setStyleSheet(R"(
            QPushButton {
                background-color: #EF4444;
                color: white;
                border: none;
                border-radius: 8px;
                font-weight: 500;
            }
            QPushButton:hover {
                background-color: #DC2626;
            }
        )");
        d->statusLabel->setText("已连接");
        d->statusLabel->setStyleSheet("color: #10B981; font-size: 14px;");
        d->subscribeBtn->setEnabled(true);
    } else {
        d->connectBtn->setText("连接行情");
        d->connectBtn->setStyleSheet(R"(
            QPushButton {
                background-color: #10B981;
                color: white;
                border: none;
                border-radius: 8px;
                font-weight: 500;
            }
            QPushButton:hover {
                background-color: #059669;
            }
        )");
        d->statusLabel->setText("未连接");
        d->statusLabel->setStyleSheet("color: #EF4444; font-size: 14px;");
        d->subscribeBtn->setEnabled(false);
    }
}

void FuturesQuotesPage::onQuoteItemClicked(int row, int column)
{
    Q_UNUSED(column)

    QString instrument = d->quoteTable->item(row, 0)->text();
    LOG_INFO(QString("Selected instrument: %1").arg(instrument));

    // TODO: 显示合约详情或打开交易对话框
}
