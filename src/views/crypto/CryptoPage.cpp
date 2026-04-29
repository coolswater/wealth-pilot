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
#include "market/CryptoDataSource.h"
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

void CryptoPage::initializePage()
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
    
    // 使用真实数据源获取排行榜
    CryptoDataSource::instance()->requestTopList(20, [this](const QVector<CryptoQuote>& quotes) {
        d->cryptoCache = quotes;
        
        d->cryptoListTable->setRowCount(d->cryptoCache.size());
        
        for (int i = 0; i < d->cryptoCache.size(); ++i) {
            const auto& crypto = d->cryptoCache[i];
            
            d->cryptoListTable->setItem(i, 0, new QTableWidgetItem(QString::number(crypto.rank)));
            d->cryptoListTable->setItem(i, 1, new QTableWidgetItem(crypto.name));
            d->cryptoListTable->setItem(i, 2, new QTableWidgetItem(crypto.symbol));
            d->cryptoListTable->setItem(i, 3, new QTableWidgetItem(QString::number(crypto.price, 'f', 2)));
            d->cryptoListTable->setItem(i, 4, new QTableWidgetItem(QString::number(crypto.priceCny, 'f', 2)));
            
            auto* changeItem = new QTableWidgetItem(QString::number(crypto.change24h, 'f', 2) + "%");
            changeItem->setForeground(crypto.change24h >= 0 ? QColor(Tokens::Colors::Success) : QColor(Tokens::Colors::Danger));
            d->cryptoListTable->setItem(i, 5, changeItem);
            
            d->cryptoListTable->setItem(i, 6, new QTableWidgetItem(QString::number(crypto.marketCap / 100000000, 'f', 1)));
            d->cryptoListTable->setItem(i, 7, new QTableWidgetItem(QString::number(crypto.volume24h / 100000000, 'f', 1) + QStringLiteral("亿")));
        }
        
        // 默认选中第一个
        if (!d->cryptoCache.isEmpty()) {
            updateCryptoDetail(d->cryptoCache[0]);
        }
    });
    
    // 启动自动刷新
    CryptoDataSource::instance()->startAutoRefresh(60000);
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
