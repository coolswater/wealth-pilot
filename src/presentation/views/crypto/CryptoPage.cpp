/**
 * @file CryptoPage.cpp
 * @brief 数字货币页面实现
 */

#include "CryptoPage.h"
#include "infrastructure/config/Tokens.h"
#include "presentation/components/StyleHelper.h"
#include "presentation/delegates/ColorDelegates.h"
#include "shared/utils/Logger.h"

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
    : DataHubPageBase(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
}

CryptoPage::~CryptoPage() = default;

void CryptoPage::initializePage()
{
    if (isInitialized()) return;
    
    // ============================================================
    // 1. 设置 DataHub 订阅
    // ============================================================
    setupDataHubSubscriptions();
    
    // ============================================================
    // 2. 加载示例数据
    // ============================================================
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

void CryptoPage::setupDataHubSubscriptions()
{
    // 订阅加密货币行情
    dataHub().subscribePattern(this, "market:crypto:*",
        [this](const QString& topic, const QVariant& value) {
            Q_UNUSED(topic)
            Q_UNUSED(value)
            // 更新加密货币行情
        });
    
    // 订阅主流加密货币
    QStringList symbols = {"BTC", "ETH", "BNB", "SOL", "XRP"};
    for (const QString& symbol : symbols) {
        dataHub().subscribe(this, QString("market:crypto:%1").arg(symbol),
            [this, symbol](const QVariant& value) {
                Q_UNUSED(symbol)
                Q_UNUSED(value)
                // 更新价格
            });
        m_subscribedSymbols.append(symbol);
    }
    
    LOG_INFO("[CryptoPage] DataHub subscriptions setup complete");
}

void CryptoPage::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 页面头部
    auto* header = StyleHelper::createPageHeader(this, QStringLiteral("数字货币"));
    mainLayout->addWidget(header);

    // 内容区域
    auto* contentWidget = new QWidget(this);
    auto* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(Tokens::Spacing::MD, Tokens::Spacing::MD, Tokens::Spacing::MD,
                                      Tokens::Spacing::MD);
    contentLayout->setSpacing(Tokens::Spacing::SM);

    // 顶部工具栏
    auto* toolbarLayout = new QHBoxLayout();
    auto* refreshBtn = new QPushButton(QStringLiteral("刷新"), this);
    toolbarLayout->addStretch();
    toolbarLayout->addWidget(refreshBtn);
    contentLayout->addLayout(toolbarLayout);

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

    // 设置颜色委托（红涨绿跌）
    d->cryptoListTable->setItemDelegateForColumn(2, new PriceColorDelegate(this));
    d->cryptoListTable->setItemDelegateForColumn(3, new ChangeColorDelegate(this));

    splitter->addWidget(d->cryptoListTable);

    // 右侧：详情面板
    d->detailPanel = new QWidget();
    auto* detailLayout = new QVBoxLayout(d->detailPanel);
    d->priceLabel = new QLabel(QStringLiteral("选择货币查看详情"));
    detailLayout->addWidget(d->priceLabel);
    splitter->addWidget(d->detailPanel);

    splitter->setSizes({400, 400});
    contentLayout->addWidget(splitter);

    mainLayout->addWidget(contentWidget, 1);

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