/**
 * @file CryptoPage.cpp
 * @brief 数字货币页面实现
 */

#include "CryptoPage.h"
#include "core/config/Tokens.h"
#include "ui/components/StyleHelper.h"
#include "utils/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QHeaderView>
#include <QSplitter>

namespace WealthPilot {

struct CryptoPage::Impl {
    QTableWidget* cryptoListTable = nullptr;
    QWidget* detailPanel = nullptr;
    QLabel* priceLabel = nullptr;
    QString currentSymbol;
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
    if (isInitialized()) return;
    
    // 加载示例数据
    d->cryptoListTable->setRowCount(5);
    
    QStringList symbols = {"BTC", "ETH", "BNB", "SOL", "XRP"};
    QStringList names = {"Bitcoin", "Ethereum", "BNB", "Solana", "XRP"};
    QList<double> prices = {42500.0, 2250.0, 310.0, 95.0, 0.52};
    
    for (int i = 0; i < 5; ++i) {
        d->cryptoListTable->setItem(i, 0, new QTableWidgetItem(symbols[i]));
        d->cryptoListTable->setItem(i, 1, new QTableWidgetItem(names[i]));
        d->cryptoListTable->setItem(i, 2, new QTableWidgetItem(QString::number(prices[i], 'f', 2)));
        d->cryptoListTable->setItem(i, 3, new QTableWidgetItem("+2.5%"));
    }
    
    setInitialized(true);
    LOG_DEBUG("CryptoPage initialized");
}

void CryptoPage::refresh()
{
    initializePage();
}

void CryptoPage::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(Tokens::Spacing::MD, Tokens::Spacing::MD, Tokens::Spacing::MD, Tokens::Spacing::MD);
    mainLayout->setSpacing(Tokens::Spacing::SM);

    // 顶部工具栏
    auto* toolbarLayout = new QHBoxLayout();
    auto* refreshBtn = new QPushButton(QStringLiteral("刷新"), this);
    toolbarLayout->addStretch();
    toolbarLayout->addWidget(refreshBtn);
    mainLayout->addLayout(toolbarLayout);

    // 分割器
    auto* splitter = new QSplitter(Qt::Horizontal, this);

    // 左侧：货币列表
    d->cryptoListTable = new QTableWidget();
    d->cryptoListTable->setColumnCount(4);
    d->cryptoListTable->setHorizontalHeaderLabels({
        QStringLiteral("代码"),
        QStringLiteral("名称"),
        QStringLiteral("价格"),
        QStringLiteral("涨跌幅")
    });
    d->cryptoListTable->horizontalHeader()->setStretchLastSection(true);
    d->cryptoListTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    d->cryptoListTable->setAlternatingRowColors(true);
    d->cryptoListTable->verticalHeader()->setVisible(false);
    splitter->addWidget(d->cryptoListTable);

    // 右侧：详情面板
    d->detailPanel = new QWidget();
    auto* detailLayout = new QVBoxLayout(d->detailPanel);
    d->priceLabel = new QLabel(QStringLiteral("选择货币查看详情"));
    detailLayout->addWidget(d->priceLabel);
    splitter->addWidget(d->detailPanel);

    splitter->setSizes({400, 400});
    mainLayout->addWidget(splitter);

    // 连接信号
    connect(refreshBtn, &QPushButton::clicked, this, &CryptoPage::onRefreshData);
    connect(d->cryptoListTable, &QTableWidget::cellClicked, this, &CryptoPage::onCryptoListClicked);

    // 设置样式（使用 StyleHelper）
    StyleHelper::setPrimaryButton(refreshBtn);
    StyleHelper::setLabelText(d->priceLabel);
}

void CryptoPage::onCryptoListClicked(int row, int column)
{
    Q_UNUSED(column);
    if (row >= 0) {
        auto* symbolItem = d->cryptoListTable->item(row, 0);
        auto* priceItem = d->cryptoListTable->item(row, 2);
        if (symbolItem && priceItem) {
            d->currentSymbol = symbolItem->text();
            d->priceLabel->setText(QStringLiteral("%1: $%2").arg(d->currentSymbol, priceItem->text()));
            emit cryptoSelected(d->currentSymbol, priceItem->text().toDouble());
        }
    }
}

void CryptoPage::onRefreshData()
{
    refresh();
    LOG_DEBUG("CryptoPage refreshed");
}

} // namespace WealthPilot