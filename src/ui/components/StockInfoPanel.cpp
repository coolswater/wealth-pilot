/**
 * @file StockInfoPanel.cpp
 * @brief 股票信息面板实现
 */

#include "StockInfoPanel.h"
#include "utils/Logger.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>

StockInfoPanel::StockInfoPanel(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    LOG_DEBUG("StockInfoPanel created");
}

StockInfoPanel::~StockInfoPanel() = default;

void StockInfoPanel::setupUI()
{
    setFixedWidth(280);
    setStyleSheet("background-color: #1E1E1E; color: #FFFFFF;");

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // 第一行：股票名称和价格
    auto* headerLayout = new QVBoxLayout();
    m_stockNameLabel = new QLabel("--", this);
    m_stockNameLabel->setStyleSheet("font-size: 16px; font-weight: bold;");
    m_priceLabel = new QLabel("--", this);
    m_priceLabel->setStyleSheet("font-size: 24px; font-weight: bold;");
    m_changeLabel = new QLabel("--", this);
    m_changeLabel->setStyleSheet("font-size: 14px;");
    headerLayout->addWidget(m_stockNameLabel);
    headerLayout->addWidget(m_priceLabel);
    headerLayout->addWidget(m_changeLabel);
    mainLayout->addLayout(headerLayout);

    // 第二行：交易状态
    m_statusLabel = new QLabel(QStringLiteral("交易状态: --"), this);
    m_statusLabel->setStyleSheet("font-size: 12px;");
    mainLayout->addWidget(m_statusLabel);

    // 第三行：委比委差
    m_orderRatioLabel = new QLabel(QStringLiteral("委比: --  委差: --"), this);
    m_orderRatioLabel->setStyleSheet("font-size: 12px;");
    mainLayout->addWidget(m_orderRatioLabel);

    // 第四行：五档盘口
    auto* orderBookLayout = new QGridLayout();
    for (int i = 0; i < 5; ++i) {
        m_bidLabels[i] = new QLabel(QStringLiteral("买%1: --").arg(5-i), this);
        m_bidLabels[i]->setStyleSheet("font-size: 11px; color: #00AA00;");
        m_askLabels[i] = new QLabel(QStringLiteral("卖%1: --").arg(i+1), this);
        m_askLabels[i]->setStyleSheet("font-size: 11px; color: #FF0000;");
        orderBookLayout->addWidget(m_bidLabels[i], i, 0);
        orderBookLayout->addWidget(m_askLabels[i], i, 1);
    }
    mainLayout->addLayout(orderBookLayout);

    // 第五行：详细行情
    m_detailTable = new QTableWidget(14, 2, this);
    m_detailTable->setStyleSheet("QTableWidget { background-color: #2A2A2A; color: #FFFFFF; }");
    m_detailTable->horizontalHeader()->setVisible(false);
    m_detailTable->verticalHeader()->setVisible(false);
    m_detailTable->setShowGrid(false);
    mainLayout->addWidget(m_detailTable);

    // 第六行：成交明细
    m_tickTable = new QTableWidget(this);
    m_tickTable->setStyleSheet("QTableWidget { background-color: #2A2A2A; color: #FFFFFF; }");
    m_tickTable->setColumnCount(3);
    m_tickTable->setHorizontalHeaderLabels({QStringLiteral("时间"), QStringLiteral("价格"), QStringLiteral("数量")});
    mainLayout->addWidget(m_tickTable, 1);
}

void StockInfoPanel::setStock(const QString& stockCode, const QString& stockName)
{
    m_stockCode = stockCode;
    m_stockNameLabel->setText(stockName.isEmpty() ? stockCode : stockName);
    clearData();
    emit stockChanged(stockCode);
}

void StockInfoPanel::updateQuote(const StockQuote& quote)
{
    m_currentQuote = quote;

    // 更新价格
    m_priceLabel->setText(QString::number(quote.lastPrice, 'f', 2));
    updatePriceLabel(m_priceLabel, quote.lastPrice, quote.preClose);

    // 更新涨跌
    QString changeText = QString("%1 (%2%)")
        .arg(QString::number(quote.changeAmount, 'f', 2))
        .arg(QString::number(quote.changePercent, 'f', 2));
    m_changeLabel->setText(changeText);
    updatePriceLabel(m_changeLabel, quote.changeAmount);

    // 更新五档
    for (int i = 0; i < 5; ++i) {
        m_bidLabels[i]->setText(QStringLiteral("买%1: %2").arg(5-i)
            .arg(QString::number(quote.bidPrice[i], 'f', 2)));
        m_askLabels[i]->setText(QStringLiteral("卖%1: %2").arg(i+1)
            .arg(QString::number(quote.askPrice[i], 'f', 2)));
    }

    // 更新委比
    m_orderRatioLabel->setText(QStringLiteral("委比: %1%  委差: %2")
        .arg(QString::number(quote.orderRatio, 'f', 2))
        .arg(QString::number(quote.orderDiff)));
}

void StockInfoPanel::updateTickData(const QVector<TickData>& ticks)
{
    m_tickTable->setRowCount(ticks.size());
    for (int i = 0; i < ticks.size(); ++i) {
        const auto& tick = ticks[i];
        m_tickTable->setItem(i, 0, new QTableWidgetItem(tick.time.toString("hh:mm:ss")));
        m_tickTable->setItem(i, 1, new QTableWidgetItem(QString::number(tick.price, 'f', 2)));
        m_tickTable->setItem(i, 2, new QTableWidgetItem(QString::number(tick.volume)));
    }
}

void StockInfoPanel::clearData()
{
    m_priceLabel->setText("--");
    m_changeLabel->setText("--");
    m_statusLabel->setText(QStringLiteral("交易状态: --"));
    m_orderRatioLabel->setText(QStringLiteral("委比: --  委差: --"));
    for (int i = 0; i < 5; ++i) {
        m_bidLabels[i]->setText(QStringLiteral("买%1: --").arg(5-i));
        m_askLabels[i]->setText(QStringLiteral("卖%1: --").arg(i+1));
    }
    m_tickTable->setRowCount(0);
}

void StockInfoPanel::updatePriceLabel(QLabel* label, double price, double prevPrice)
{
    QString color;
    if (prevPrice > 0) {
        color = (price > prevPrice) ? COLOR_GREEN : (price < prevPrice ? COLOR_RED : COLOR_GRAY);
    } else {
        color = (price > 0) ? COLOR_GREEN : (price < 0 ? COLOR_RED : COLOR_GRAY);
    }
    label->setStyleSheet(QString("color: %1;").arg(color));
}

QString StockInfoPanel::formatVolume(qint64 volume) const
{
    if (volume >= 100000000) return QString::number(volume / 100000000.0, 'f', 2) + QStringLiteral("亿");
    if (volume >= 10000) return QString::number(volume / 10000.0, 'f', 2) + QStringLiteral("万");
    return QString::number(volume);
}

QString StockInfoPanel::formatAmount(double amount) const
{
    if (amount >= 100000000) return QString::number(amount / 100000000.0, 'f', 2) + QStringLiteral("亿");
    if (amount >= 10000) return QString::number(amount / 10000.0, 'f', 2) + QStringLiteral("万");
    return QString::number(amount, 'f', 2);
}
