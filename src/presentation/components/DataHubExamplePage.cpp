/**
 * @file DataHubExamplePage.cpp
 * @brief DataHub 示例页面实现
 */

#include "DataHubExamplePage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QDebug>

namespace WealthPilot {
namespace UI {

DataHubExamplePage::DataHubExamplePage(QWidget* parent)
    : DataHubPageBase(parent)
    , m_titleLabel(new QLabel(this))
    , m_quoteTable(new QTableWidget(this))
    , m_refreshBtn(new QPushButton(QStringLiteral("刷新数据"), this))
    , m_subscribeBtn(new QPushButton(QStringLiteral("订阅股票"), this))
{
    setupUI();
}

DataHubExamplePage::~DataHubExamplePage()
{
}

void DataHubExamplePage::initializePage()
{
    // 默认订阅几只股票
    m_subscribedSymbols = {"sh600000", "sh600519", "sz000001", "sz000002"};

    // 设置 DataHub 订阅
    setupDataHubSubscriptions();

    // 请求初始数据
    QStringList topics;
    for (const auto& symbol : m_subscribedSymbols) {
        topics << QString("market:quote:%1").arg(symbol);
    }
    requestData(topics, true);
}

void DataHubExamplePage::setupUI()
{
    auto* layout = new QVBoxLayout(this);

    // 标题
    m_titleLabel->setText(QStringLiteral("DataHub 示例 - 股票行情订阅"));
    m_titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; padding: 10px;");
    layout->addWidget(m_titleLabel);

    // 表格
    m_quoteTable->setColumnCount(8);
    m_quoteTable->setHorizontalHeaderLabels({
        QStringLiteral("代码"),
        QStringLiteral("名称"),
        QStringLiteral("最新价"),
        QStringLiteral("涨跌幅"),
        QStringLiteral("成交量"),
        QStringLiteral("最高"),
        QStringLiteral("最低"),
        QStringLiteral("更新时间")
    });
    m_quoteTable->horizontalHeader()->setStretchLastSection(true);
    m_quoteTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(m_quoteTable);

    // 按钮
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addWidget(m_refreshBtn);
    btnLayout->addWidget(m_subscribeBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    // 连接信号
    connect(m_refreshBtn, &QPushButton::clicked, this, &DataHubExamplePage::onRefreshClicked);
    connect(m_subscribeBtn, &QPushButton::clicked, this, &DataHubExamplePage::onSubscribeNewSymbol);
}

void DataHubExamplePage::setupDataHubSubscriptions()
{
    qDebug() << "[DataHubExamplePage] Setting up DataHub subscriptions...";

    // 方法1: 使用模式订阅（订阅所有 market:quote:* 的更新）
    // 适合需要监听多只股票的场景
    dataHub().subscribePattern(this, "market:quote:*",
        [this](const QString& topic, const QVariant& value) {
            // 解析 topic 获取 symbol
            auto parts = topic.split(':');
            if (parts.size() >= 3) {
                QString symbol = parts[2];
                if (value.canConvert<StockQuote>()) {
                    updateQuoteDisplay(symbol, value.value<StockQuote>());
                }
            }
        });

    // 方法2: 使用 subscribeQuote 便捷方法
    // 适合订阅特定股票的场景
    for (const auto& symbol : m_subscribedSymbols) {
        subscribeQuote(symbol, [this, symbol](const StockQuote& quote) {
            updateQuoteDisplay(symbol, quote);
        });
    }

    qDebug() << "[DataHubExamplePage] Subscribed to" << m_subscribedSymbols.size() << "symbols";
}

void DataHubExamplePage::updateQuoteDisplay(const QString& symbol, const StockQuote& quote)
{
    // 查找或创建行
    int row = -1;
    for (int i = 0; i < m_quoteTable->rowCount(); ++i) {
        if (m_quoteTable->item(i, 0)->text() == symbol) {
            row = i;
            break;
        }
    }

    if (row < 0) {
        row = m_quoteTable->rowCount();
        m_quoteTable->insertRow(row);
    }

    // 更新数据
    m_quoteTable->setItem(row, 0, new QTableWidgetItem(quote.symbol));
    m_quoteTable->setItem(row, 1, new QTableWidgetItem(quote.name));
    m_quoteTable->setItem(row, 2, new QTableWidgetItem(QString::number(quote.lastPrice, 'f', 2)));

    // 涨跌幅（带颜色）
    auto* changeItem = new QTableWidgetItem(QString::number(quote.changePercent, 'f', 2) + "%");
    if (quote.changePercent > 0) {
        changeItem->setForeground(QColor(255, 0, 0));  // 红涨
    } else if (quote.changePercent < 0) {
        changeItem->setForeground(QColor(0, 128, 0));  // 绿跌
    }
    m_quoteTable->setItem(row, 3, changeItem);

    m_quoteTable->setItem(row, 4, new QTableWidgetItem(QString::number(quote.volume)));
    m_quoteTable->setItem(row, 5, new QTableWidgetItem(QString::number(quote.highPrice, 'f', 2)));
    m_quoteTable->setItem(row, 6, new QTableWidgetItem(QString::number(quote.lowPrice, 'f', 2)));
    m_quoteTable->setItem(row, 7, new QTableWidgetItem(quote.updateTime.toString("hh:mm:ss")));

    qDebug() << "[DataHubExamplePage] Updated quote for" << symbol << "price:" << quote.lastPrice;
}

void DataHubExamplePage::updateSnapshotDisplay(const QString& symbol, const MarketSnapshot& snapshot)
{
    Q_UNUSED(symbol)
    Q_UNUSED(snapshot)
    // 可以在这里更新快照显示
}

void DataHubExamplePage::onRefreshClicked()
{
    // 强制刷新所有订阅的数据
    QStringList topics;
    for (const auto& symbol : m_subscribedSymbols) {
        topics << QString("market:quote:%1").arg(symbol);
    }
    requestData(topics, true);  // force = true

    qDebug() << "[DataHubExamplePage] Refresh requested for" << topics.size() << "topics";
}

void DataHubExamplePage::onSubscribeNewSymbol()
{
    bool ok;
    QString symbol = QInputDialog::getText(this, 
        QStringLiteral("订阅股票"),
        QStringLiteral("请输入股票代码（如 sh600000）:"),
        QLineEdit::Normal, "", &ok);

    if (ok && !symbol.isEmpty() && !m_subscribedSymbols.contains(symbol)) {
        m_subscribedSymbols.append(symbol);

        // 订阅新股票
        subscribeQuote(symbol, [this, symbol](const StockQuote& quote) {
            updateQuoteDisplay(symbol, quote);
        });

        // 立即请求数据
        requestData(QString("market:quote:%1").arg(symbol), true);

        qDebug() << "[DataHubExamplePage] Subscribed to new symbol:" << symbol;
    }
}

} // namespace UI
} // namespace WealthPilot