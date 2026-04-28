/**
 * @file CryptoPage.cpp
 * @brief 数字货币页面实现 - 加密货币行情展示与分析
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "CryptoPage.h"
#include "ui/components/KLineChart.h"
#include "core/config/Tokens.h"
#include "utils/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QHeaderView>
#include <QSplitter>
#include <QGroupBox>

struct CryptoPage::Impl {
    QTableWidget* cryptoListTable = nullptr;
    QLabel* nameLabel = nullptr;
    QLabel* priceLabel = nullptr;
    QLabel* changeLabel = nullptr;
    QLabel* volumeLabel = nullptr;
    QLabel* marketCapLabel = nullptr;
    QLabel* highLabel = nullptr;
    QLabel* lowLabel = nullptr;
    KLineChart* klineChart = nullptr;
    
    QString currentSymbol;
    CryptoQuote currentQuote;
    QVector<CryptoQuote> cryptoCache;
};

CryptoPage::CryptoPage(QWidget *parent)
    : BasePage(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
}

CryptoPage::~CryptoPage() = default;

void CryptoPage::initialize()
{
    loadCryptoList();
}

void CryptoPage::refresh()
{
    loadCryptoList();
    if (!d->currentSymbol.isEmpty()) {
        loadCryptoKLine(d->currentSymbol);
    }
}

void CryptoPage::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // 工具栏
    auto* toolbar = new QWidget(this);
    toolbar->setFixedHeight(40);
    toolbar->setStyleSheet("QWidget { background: #0a0a0a; }");
    auto* toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(12, 6, 12, 6);
    
    auto* refreshBtn = new QPushButton(QStringLiteral("刷新"));
    refreshBtn->setFixedSize(60, 26);
    refreshBtn->setStyleSheet("QPushButton { background: #2a2a2a; color: #ffffff; border: none; border-radius: 4px; } QPushButton:hover { background: #3a3a3a; }");
    connect(refreshBtn, &QPushButton::clicked, this, &CryptoPage::onRefreshData);
    toolbarLayout->addWidget(refreshBtn);
    toolbarLayout->addStretch();
    
    mainLayout->addWidget(toolbar);
    
    // 主内容
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setStyleSheet("QSplitter::handle { background: #2a2a2a; width: 1px; }");
    
    initCryptoList();
    splitter->addWidget(d->cryptoListTable);
    
    initDetailPanel();
    splitter->addWidget(d->klineChart);
    
    splitter->setSizes({400, 500});
    mainLayout->addWidget(splitter, 1);
    
    initConnections();
}

void CryptoPage::initCryptoList()
{
    d->cryptoListTable = new QTableWidget();
    d->cryptoListTable->setColumnCount(8);
    d->cryptoListTable->setHorizontalHeaderLabels({
        QStringLiteral("排名"),
        QStringLiteral("名称"),
        QStringLiteral("符号"),
        QStringLiteral("价格(USD)"),
        QStringLiteral("价格(CNY)"),
        QStringLiteral("24h涨跌"),
        QStringLiteral("市值(亿)"),
        QStringLiteral("24h成交额")
    });
    
    d->cryptoListTable->setStyleSheet(R"(
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
    
    d->cryptoListTable->horizontalHeader()->setStretchLastSection(true);
    d->cryptoListTable->verticalHeader()->setVisible(false);
    d->cryptoListTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    d->cryptoListTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

void CryptoPage::initDetailPanel()
{
    d->klineChart = new KLineChart();
    d->klineChart->setMinimumHeight(400);
}

void CryptoPage::initConnections()
{
    connect(d->cryptoListTable, &QTableWidget::cellClicked,
            this, &CryptoPage::onCryptoListClicked);
}

void CryptoPage::loadCryptoList()
{
    d->cryptoCache.clear();
    
    // 模拟数据
    d->cryptoCache.append({{"BTC", "Bitcoin", 67890.50, 492345.67, 2.35, 28500000000, 1340000000000, 69500.00, 66200.00, 1}});
    d->cryptoCache.append({{"ETH", "Ethereum", 3456.78, 25056.23, 1.89, 15600000000, 415000000000, 3520.00, 3380.00, 2}});
    d->cryptoCache.append({{"BNB", "BNB", 567.89, 4116.23, -0.56, 1200000000, 87000000000, 580.00, 555.00, 3}});
    d->cryptoCache.append({{"SOL", "Solana", 145.67, 1056.23, 5.23, 2300000000, 67000000000, 150.00, 138.00, 4}});
    d->cryptoCache.append({{"XRP", "XRP", 0.5234, 3.79, -1.23, 1500000000, 29000000000, 0.5400, 0.5100, 5}});
    d->cryptoCache.append({{"ADA", "Cardano", 0.4567, 3.31, 0.89, 450000000, 16000000000, 0.4700, 0.4400, 6}});
    d->cryptoCache.append({{"DOGE", "Dogecoin", 0.1234, 0.89, 3.45, 890000000, 18000000000, 0.1300, 0.1180, 7}});
    d->cryptoCache.append({{"DOT", "Polkadot", 7.89, 57.18, -0.78, 320000000, 10000000000, 8.20, 7.50, 8}});
    d->cryptoCache.append({{"MATIC", "Polygon", 0.7890, 5.72, 2.12, 280000000, 7800000000, 0.8200, 0.7600, 9}});
    d->cryptoCache.append({{"LINK", "Chainlink", 14.56, 105.56, 1.56, 450000000, 8600000000, 15.00, 14.00, 10}});
    
    d->cryptoListTable->setRowCount(d->cryptoCache.size());
    
    for (int i = 0; i < d->cryptoCache.size(); ++i) {
        const auto& crypto = d->cryptoCache[i];
        
        d->cryptoListTable->setItem(i, 0, new QTableWidgetItem(QString::number(crypto.rank)));
        d->cryptoListTable->setItem(i, 1, new QTableWidgetItem(crypto.name));
        d->cryptoListTable->setItem(i, 2, new QTableWidgetItem(crypto.symbol));
        d->cryptoListTable->setItem(i, 3, new QTableWidgetItem(QString::number(crypto.price, 'f', 2)));
        d->cryptoListTable->setItem(i, 4, new QTableWidgetItem(QString::number(crypto.priceCny, 'f', 2)));
        
        auto* changeItem = new QTableWidgetItem(QString::number(crypto.change24h, 'f', 2) + "%");
        changeItem->setForeground(crypto.change24h >= 0 ? QColor("#00D4AA") : QColor("#FF3366"));
        d->cryptoListTable->setItem(i, 5, changeItem);
        
        d->cryptoListTable->setItem(i, 6, new QTableWidgetItem(QString::number(crypto.marketCap / 100000000, 'f', 1)));
        d->cryptoListTable->setItem(i, 7, new QTableWidgetItem(QString::number(crypto.volume24h / 100000000, 'f', 1) + QStringLiteral("亿")));
    }
}

void CryptoPage::updateCryptoDetail(const CryptoQuote& quote)
{
    d->currentQuote = quote;
    // 更新详情...
}

void CryptoPage::loadCryptoKLine(const QString& symbol)
{
    Q_UNUSED(symbol)
    // 加载K线数据...
}

void CryptoPage::onCryptoListClicked(int row, int column)
{
    Q_UNUSED(column)
    if (row >= 0 && row < d->cryptoCache.size()) {
        const auto& crypto = d->cryptoCache[row];
        d->currentSymbol = crypto.symbol;
        updateCryptoDetail(crypto);
        loadCryptoKLine(crypto.symbol);
        emit cryptoSelected(crypto.symbol, crypto.price);
    }
}

void CryptoPage::onRefreshData()
{
    refresh();
}
