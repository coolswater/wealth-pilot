/**
 * @file FundPage.cpp
 * @brief 基金页面实现 - 基金行情展示与分析
 *
 * @details 实现功能：
 * - 基金列表展示与管理
 * - 基金详情查看
 * - 基金搜索筛选
 * - 自选基金管理
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "FundPage.h"
#include "ui/components/KLineChart.h"
#include "core/config/Tokens.h"
#include "market/FundDataSource.h"
#include "utils/Logger.h"
#include "market/FavoritesManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QTableWidget>
#include <QHeaderView>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QHeaderView>
#include <QSplitter>
#include <QGroupBox>
#include <QProgressBar>
#include <QRandomGenerator>

// ========== PIMPL实现 ==========

struct FundPage::Impl {
    // 工具栏组件
    QComboBox* fundTypeCombo = nullptr;     ///< 基金类型下拉框
    QLineEdit* searchEdit = nullptr;        ///< 搜索框
    QPushButton* refreshBtn = nullptr;      ///< 刷新按钮
    QPushButton* addToWatchlistBtn = nullptr; ///< 加自选按钮
    
    // 基金列表
    QTableWidget* fundListTable = nullptr;  ///< 基金列表表格
    
    // 详情面板
    QWidget* detailPanel = nullptr;         ///< 详情面板
    QLabel* fundNameLabel = nullptr;        ///< 基金名称标签
    QLabel* navLabel = nullptr;             ///< 净值标签
    QLabel* accNavLabel = nullptr;          ///< 累计净值标签
    QLabel* lastPriceLabel = nullptr;       ///< 最新价标签
    QLabel* changeLabel = nullptr;          ///< 涨跌幅标签
    QLabel* scaleLabel = nullptr;           ///< 规模标签
    QLabel* managerLabel = nullptr;         ///< 基金经理标签
    QLabel* companyLabel = nullptr;         ///< 基金公司标签
    QTableWidget* holdingTable = nullptr;   ///< 持仓表格
    KLineChart* klineChart = nullptr;       ///< K线图（场内基金）
    
    // 当前选中基金
    QString currentFundCode;                ///< 当前基金代码
    FundQuote currentFund;                  ///< 当前基金数据
    
    // 数据缓存
    QVector<FundQuote> fundCache;           ///< 基金数据缓存
};

// ========== 构造与析构 ==========

FundPage::FundPage(QWidget *parent)
    : BasePage(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
}

FundPage::~FundPage() = default;

// ========== 初始化 ==========

void FundPage::initializePage()
{
    loadFundList();
}

void FundPage::refresh()
{
    loadFundList();
    if (!d->currentFundCode.isEmpty()) {
        loadFundHolding(d->currentFundCode);
        loadFundKLine(d->currentFundCode);
    }
}

void FundPage::setFund(const QString& code)
{
    d->currentFundCode = code;
    // 查找基金数据
    for (const auto& fund : d->fundCache) {
        if (fund.code == code) {
            updateFundDetail(fund);
            break;
        }
    }
}

// ========== UI初始化 ==========

void FundPage::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // 初始化工具栏
    initToolBar();
    
    // 主内容区域（左右分割）
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setStyleSheet(QString("QSplitter::handle { background: %1; width: 1px; }").arg(Tokens::Colors::Border));
    
    // 左侧：基金列表
    auto* listWidget = new QWidget();
    listWidget->setStyleSheet(QString("QWidget { background: %1; }").arg(Tokens::Colors::BgSurface));
    auto* listLayout = new QVBoxLayout(listWidget);
    listLayout->setContentsMargins(0, 0, 0, 0);
    
    initFundList();
    listLayout->addWidget(d->fundListTable);
    splitter->addWidget(listWidget);
    
    // 右侧：详情面板
    initDetailPanel();
    splitter->addWidget(d->detailPanel);
    
    // 设置分割比例
    splitter->setSizes({400, 600});
    
    mainLayout->addWidget(splitter, 1);
    
    // 初始化连接
    initConnections();
}

void FundPage::initToolBar()
{
    auto* toolbar = new QWidget(this);
    toolbar->setFixedHeight(40);
    toolbar->setStyleSheet(QString("QWidget { background: %1; }").arg(Tokens::Colors::BgSurface));
    
    auto* layout = new QHBoxLayout(toolbar);
    layout->setContentsMargins(12, 6, 12, 6);
    layout->setSpacing(8);
    
    // 基金类型下拉框
    d->fundTypeCombo = new QComboBox();
    d->fundTypeCombo->addItems({
        QStringLiteral("全部基金"),
        QStringLiteral("ETF基金"),
        QStringLiteral("LOF基金"),
        QStringLiteral("开放式基金"),
        QStringLiteral("货币基金"),
        QStringLiteral("债券基金")
    });
    d->fundTypeCombo->setFixedSize(120, 26);
    d->fundTypeCombo->setStyleSheet(QString(R"(
        QComboBox {
            background: %1;
            color: %2;
            border: none;
            padding: 0 8px;
            font-size: 12px;
            border-radius: 4px;
        }
        QComboBox::drop-down { border: none; width: 16px; }
        QComboBox QAbstractItemView {
            background: %1;
            color: %3;
            selection-background-color: %4;
        }
    )").arg(Tokens::Colors::BgElevated, Tokens::Colors::TextSecondary, Tokens::Colors::TextPrimary, Tokens::Colors::BgHover));
    layout->addWidget(d->fundTypeCombo);
    
    // 搜索框
    d->searchEdit = new QLineEdit();
    d->searchEdit->setPlaceholderText(QStringLiteral("搜索基金代码/名称"));
    d->searchEdit->setFixedSize(180, 26);
    d->searchEdit->setStyleSheet(QString(R"(
        QLineEdit {
            background: %1;
            color: %2;
            border: none;
            padding: 0 10px;
            font-size: 12px;
            border-radius: 4px;
        }
        QLineEdit::placeholder { color: %3; }
    )").arg(Tokens::Colors::BgElevated, Tokens::Colors::TextPrimary, Tokens::Colors::TextTertiary));
    layout->addWidget(d->searchEdit);
    
    layout->addStretch();
    
    // 加自选按钮
    d->addToWatchlistBtn = new QPushButton(QStringLiteral("加自选"));
    d->addToWatchlistBtn->setFixedSize(70, 26);
    d->addToWatchlistBtn->setStyleSheet(QString(R"(
        QPushButton {
            background: %1;
            color: %2;
            border: none;
            font-size: 12px;
            border-radius: 4px;
        }
        QPushButton:hover { background: %3; }
    )").arg(Tokens::Colors::BgElevated, Tokens::Colors::TextPrimary, Tokens::Colors::BgHover));
    layout->addWidget(d->addToWatchlistBtn);
    
    // 刷新按钮
    d->refreshBtn = new QPushButton(QStringLiteral("刷新"));
    d->refreshBtn->setFixedSize(60, 26);
    d->refreshBtn->setStyleSheet(QString(R"(
        QPushButton {
            background: %1;
            color: %2;
            border: none;
            font-size: 12px;
            border-radius: 4px;
        }
        QPushButton:hover { background: %3; }
    )").arg(Tokens::Colors::BgElevated, Tokens::Colors::TextPrimary, Tokens::Colors::BgHover));
    layout->addWidget(d->refreshBtn);
    
    auto* mainLayout = qobject_cast<QVBoxLayout*>(this->layout());
    mainLayout->addWidget(toolbar);
}

void FundPage::initFundList()
{
    d->fundListTable = new QTableWidget();
    d->fundListTable->setColumnCount(8);
    d->fundListTable->setHorizontalHeaderLabels({
        QStringLiteral("代码"),
        QStringLiteral("名称"),
        QStringLiteral("类型"),
        QStringLiteral("最新价"),
        QStringLiteral("涨跌幅"),
        QStringLiteral("净值"),
        QStringLiteral("累计净值"),
        QStringLiteral("规模(亿)")
    });
    
    // 表格样式
    d->fundListTable->setStyleSheet(QString(R"(
        QTableWidget {
            background: %1;
            color: %2;
            border: none;
            gridline-color: %3;
            font-size: 12px;
        }
        QTableWidget::item {
            padding: 4px;
        }
        QTableWidget::item:selected {
            background: %4;
        }
        QHeaderView::section {
            background: %5;
            color: %6;
            border: none;
            padding: 6px;
            font-size: 11px;
        }
    )").arg(Tokens::Colors::BgSurface, Tokens::Colors::TextPrimary, Tokens::Colors::Border, Tokens::Colors::BgElevated, Tokens::Colors::BgBase, Tokens::Colors::TextTertiary));
    
    d->fundListTable->horizontalHeader()->setStretchLastSection(true);
    d->fundListTable->verticalHeader()->setVisible(false);
    d->fundListTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    d->fundListTable->setSelectionMode(QAbstractItemView::SingleSelection);
    d->fundListTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

void FundPage::initDetailPanel()
{
    d->detailPanel = new QWidget();
    d->detailPanel->setStyleSheet(QString("QWidget { background: %1; }").arg(Tokens::Colors::BgSurface));
    
    auto* layout = new QVBoxLayout(d->detailPanel);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(12);
    
    // 基金基本信息
    auto* infoGroup = new QGroupBox(QStringLiteral("基金信息"));
    infoGroup->setStyleSheet(QString(R"(
        QGroupBox {
            color: %1;
            font-size: 13px;
            font-weight: bold;
            border: 1px solid %2;
            border-radius: 4px;
            margin-top: 8px;
            padding-top: 8px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
        }
    )").arg(Tokens::Colors::TextPrimary, Tokens::Colors::Border));
    
    auto* infoLayout = new QGridLayout(infoGroup);
    infoLayout->setSpacing(8);
    
    // 基金名称
    d->fundNameLabel = new QLabel(QStringLiteral("--"));
    d->fundNameLabel->setStyleSheet(QString("font-size: 16px; font-weight: bold; color: %1;").arg(Tokens::Colors::TextPrimary));
    infoLayout->addWidget(d->fundNameLabel, 0, 0, 1, 4);
    
    // 净值信息
    int row = 1;
    auto createInfoRow = [&](const QString& label, QLabel*& valueLabel) {
        auto* lbl = new QLabel(label);
        lbl->setStyleSheet(QString("color: %1; font-size: 12px;").arg(Tokens::Colors::TextTertiary));
        infoLayout->addWidget(lbl, row, 0);
        valueLabel = new QLabel(QStringLiteral("--"));
        valueLabel->setStyleSheet(QString("color: %1; font-size: 12px;").arg(Tokens::Colors::TextPrimary));
        infoLayout->addWidget(valueLabel, row, 1);
        row++;
    };
    
    createInfoRow(QStringLiteral("单位净值:"), d->navLabel);
    createInfoRow(QStringLiteral("累计净值:"), d->accNavLabel);
    createInfoRow(QStringLiteral("最新价:"), d->lastPriceLabel);
    createInfoRow(QStringLiteral("涨跌幅:"), d->changeLabel);
    createInfoRow(QStringLiteral("基金规模:"), d->scaleLabel);
    createInfoRow(QStringLiteral("基金经理:"), d->managerLabel);
    createInfoRow(QStringLiteral("基金公司:"), d->companyLabel);
    
    layout->addWidget(infoGroup);
    
    // 持仓明细
    auto* holdingGroup = new QGroupBox(QStringLiteral("持仓明细"));
    holdingGroup->setStyleSheet(infoGroup->styleSheet());
    
    auto* holdingLayout = new QVBoxLayout(holdingGroup);
    
    d->holdingTable = new QTableWidget();
    d->holdingTable->setColumnCount(5);
    d->holdingTable->setHorizontalHeaderLabels({
        QStringLiteral("股票代码"),
        QStringLiteral("股票名称"),
        QStringLiteral("持仓比例"),
        QStringLiteral("持仓市值"),
        QStringLiteral("涨跌幅")
    });
    d->holdingTable->setStyleSheet(d->fundListTable->styleSheet());
    d->holdingTable->horizontalHeader()->setStretchLastSection(true);
    d->holdingTable->verticalHeader()->setVisible(false);
    d->holdingTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    holdingLayout->addWidget(d->holdingTable);
    
    layout->addWidget(holdingGroup);
    
    // K线图（场内基金）
    auto* klineGroup = new QGroupBox(QStringLiteral("K线图"));
    klineGroup->setStyleSheet(infoGroup->styleSheet());
    
    auto* klineLayout = new QVBoxLayout(klineGroup);
    
    d->klineChart = new KLineChart();
    d->klineChart->setMinimumHeight(200);
    klineLayout->addWidget(d->klineChart);
    
    layout->addWidget(klineGroup);
}

void FundPage::initConnections()
{
    // 基金类型切换
    connect(d->fundTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FundPage::onFundTypeChanged);
    
    // 搜索
    connect(d->searchEdit, &QLineEdit::textChanged,
            this, &FundPage::onSearchTextChanged);
    
    // 基金列表点击
    connect(d->fundListTable, &QTableWidget::cellClicked,
            this, &FundPage::onFundListClicked);
    
    // 加自选
    connect(d->addToWatchlistBtn, &QPushButton::clicked,
            this, &FundPage::onAddToWatchlist);
    
    // 刷新
    connect(d->refreshBtn, &QPushButton::clicked,
            this, &FundPage::onRefreshData);
}

// ========== 数据加载 ==========

void FundPage::loadFundList()
{
    d->fundCache.clear();
    
    // 使用真实数据源获取基金列表
    FundDataSource::instance()->requestFundList(FundType::ETF, "changePercent", 20, [this](const QVector<FundQuote>& quotes) {
        d->fundCache = quotes;
        
        // 更新表格
        d->fundListTable->setRowCount(d->fundCache.size());
        
        for (int i = 0; i < d->fundCache.size(); ++i) {
            const auto& fund = d->fundCache[i];
            
            d->fundListTable->setItem(i, 0, new QTableWidgetItem(fund.code));
            d->fundListTable->setItem(i, 1, new QTableWidgetItem(fund.name));
            d->fundListTable->setItem(i, 2, new QTableWidgetItem(formatFundType(fund.type)));
            
            // 最新价（场内基金才有）
            auto* priceItem = new QTableWidgetItem(fund.lastPrice > 0 ? QString::number(fund.lastPrice, 'f', 3) : "--");
            d->fundListTable->setItem(i, 3, priceItem);
            
            // 涨跌幅
            auto* changeItem = new QTableWidgetItem(QString::number(fund.changePercent, 'f', 2) + "%");
            // 中国市场：红涨绿跌
            if (fund.changePercent > 0) {
                changeItem->setForeground(QColor(Tokens::Colors::Danger));  // 上涨红色
            } else if (fund.changePercent < 0) {
                changeItem->setForeground(QColor(Tokens::Colors::Success));  // 下跌绿色
            }
            d->fundListTable->setItem(i, 4, changeItem);
            
            // 净值
            d->fundListTable->setItem(i, 5, new QTableWidgetItem(QString::number(fund.nav, 'f', 4)));
            
            // 累计净值
            d->fundListTable->setItem(i, 6, new QTableWidgetItem(QString::number(fund.accNav, 'f', 4)));
            
            // 规模
            d->fundListTable->setItem(i, 7, new QTableWidgetItem(QString::number(fund.scale, 'f', 1)));
        }
        
        // 默认选中第一个
        if (!d->fundCache.isEmpty()) {
            updateFundDetail(d->fundCache[0]);
        }
    });
    
    // 启动自动刷新
    FundDataSource::instance()->startAutoRefresh(60000);
}

void FundPage::updateFundDetail(const FundQuote& quote)
{
    d->currentFund = quote;
    
    // 更新基本信息
    d->fundNameLabel->setText(QString("%1(%2)").arg(quote.name, quote.code));
    d->navLabel->setText(QString::number(quote.nav, 'f', 4));
    d->accNavLabel->setText(QString::number(quote.accNav, 'f', 4));
    
    // 最新价（场内基金）
    if (quote.lastPrice > 0) {
        d->lastPriceLabel->setText(QString::number(quote.lastPrice, 'f', 3));
    } else {
        d->lastPriceLabel->setText(QStringLiteral("--"));
    }
    
    // 涨跌幅
    QString changeText = QString::number(quote.changePercent, 'f', 2) + "%";
    if (quote.changePercent > 0) {
        d->changeLabel->setText("+" + changeText);
        d->changeLabel->setStyleSheet(QString("color: %1; font-size: 12px;").arg(Tokens::Colors::Danger));
    } else if (quote.changePercent < 0) {
        d->changeLabel->setText(changeText);
        d->changeLabel->setStyleSheet(QString("color: %1; font-size: 12px;").arg(Tokens::Colors::Success));
    } else {
        d->changeLabel->setText(changeText);
        d->changeLabel->setStyleSheet(QString("color: %1; font-size: 12px;").arg(Tokens::Colors::TextTertiary));
    }
    
    // 规模
    d->scaleLabel->setText(QString::number(quote.scale, 'f', 1) + QStringLiteral(" 亿"));
    
    // 基金经理
    d->managerLabel->setText(quote.manager);
    
    // 基金公司
    d->companyLabel->setText(quote.company);
}

void FundPage::loadFundHolding(const QString& code)
{
    Q_UNUSED(code)
    
    // 模拟持仓数据
    QVector<FundHolding> holdings = {
        {"600519", QStringLiteral("贵州茅台"), 8.5, 450, 850000, 1.2},
        {"000858", QStringLiteral("五粮液"), 6.2, 320, 560000, 0.8},
        {"000333", QStringLiteral("美的集团"), 5.1, 280, 420000, -0.5},
        {"600036", QStringLiteral("招商银行"), 4.8, 250, 380000, 0.3},
        {"601318", QStringLiteral("中国平安"), 4.2, 220, 350000, -0.2}
    };
    
    d->holdingTable->setRowCount(holdings.size());
    
    for (int i = 0; i < holdings.size(); ++i) {
        const auto& h = holdings[i];
        
        d->holdingTable->setItem(i, 0, new QTableWidgetItem(h.stockCode));
        d->holdingTable->setItem(i, 1, new QTableWidgetItem(h.stockName));
        d->holdingTable->setItem(i, 2, new QTableWidgetItem(QString::number(h.ratio, 'f', 1) + "%"));
        d->holdingTable->setItem(i, 3, new QTableWidgetItem(QString::number(h.value / 10000, 'f', 1) + QStringLiteral("万")));
        
        auto* changeItem = new QTableWidgetItem(QString::number(h.change, 'f', 2) + "%");
        if (h.change > 0) {
            changeItem->setForeground(QColor(Tokens::Colors::Success));
        } else if (h.change < 0) {
            changeItem->setForeground(QColor(Tokens::Colors::Danger));
        }
        d->holdingTable->setItem(i, 4, changeItem);
    }
}

void FundPage::loadFundKLine(const QString& code)
{
    Q_UNUSED(code)
    
    // 场内基金才显示K线
    if (d->currentFund.type != FundType::ETF && d->currentFund.type != FundType::LOF) {
        d->klineChart->setVisible(false);
        return;
    }
    
    d->klineChart->setVisible(true);
    
    // 生成模拟K线数据
    QVector<KLineData> klineData;
    QDateTime baseTime = QDateTime::currentDateTime().addDays(-100);
    
    double basePrice = d->currentFund.lastPrice > 0 ? d->currentFund.lastPrice : d->currentFund.nav;
    
    for (int i = 0; i < 100; ++i) {
        KLineData kline;
        kline.time = baseTime.addDays(i);
        
        double volatility = 0.02;
        double change = (QRandomGenerator::global()->bounded(100) - 50) / 100.0 * volatility;
        
        kline.open = basePrice * (1 + change);
        kline.close = kline.open * (1 + (QRandomGenerator::global()->bounded(100) - 50) / 100.0 * volatility);
        kline.high = qMax(kline.open, kline.close) * (1 + QRandomGenerator::global()->bounded(50) / 100.0 * volatility);
        kline.low = qMin(kline.open, kline.close) * (1 - QRandomGenerator::global()->bounded(50) / 100.0 * volatility);
        kline.volume = QRandomGenerator::global()->bounded(100000, 500000);
        kline.turnover = kline.volume * (kline.open + kline.close) / 2;
        
        klineData.append(kline);
        basePrice = kline.close;
    }
    
    d->klineChart->setData(klineData);
}

// ========== 槽函数 ==========

void FundPage::onFundTypeChanged(int index)
{
    // 根据类型筛选基金列表
    // 0: 全部, 1: ETF, 2: LOF, 3: 开放式, 4: 货币, 5: 债券
    FundType selectedType = static_cast<FundType>(index - 1); // -1 表示全部
    
    for (int i = 0; i < d->fundListTable->rowCount(); ++i) {
        bool shouldHide = false;
        
        if (index > 0 && i < d->fundCache.size()) {
            // 根据选择的类型过滤
            FundType fundType = d->fundCache[i].type;
            shouldHide = (fundType != selectedType);
        }
        
        d->fundListTable->setRowHidden(i, shouldHide);
    }
}

void FundPage::onSearchTextChanged(const QString& text)
{
    // 搜索过滤
    for (int i = 0; i < d->fundListTable->rowCount(); ++i) {
        QString code = d->fundListTable->item(i, 0)->text();
        QString name = d->fundListTable->item(i, 1)->text();
        
        bool match = text.isEmpty() || 
                     code.contains(text, Qt::CaseInsensitive) ||
                     name.contains(text, Qt::CaseInsensitive);
        
        d->fundListTable->setRowHidden(i, !match);
    }
}

void FundPage::onFundListClicked(int row, int column)
{
    Q_UNUSED(column)
    
    if (row >= 0 && row < d->fundCache.size()) {
        const auto& fund = d->fundCache[row];
        d->currentFundCode = fund.code;
        updateFundDetail(fund);
        loadFundHolding(fund.code);
        loadFundKLine(fund.code);
        
        emit fundSelected(fund.code, fund.name);
    }
}

void FundPage::onAddToWatchlist()
{
    if (d->currentFundCode.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先选择一个基金"));
        return;
    }
    
    // 添加到自选
    if (FavoritesManager::instance()->addFavorite(d->currentFundCode)) {
        // 查找基金名称
        QString fundName;
        for (const auto& fund : d->fundCache) {
            if (fund.code == d->currentFundCode) {
                fundName = fund.name;
                break;
            }
        }
        QMessageBox::information(this, QStringLiteral("成功"), 
            QStringLiteral("已将 %1 (%2) 添加到自选").arg(fundName, d->currentFundCode));
        LOG_INFO(QString("Added fund to watchlist: %1").arg(d->currentFundCode));
    } else {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("该基金已在自选列表中"));
    }
}

void FundPage::onRefreshData()
{
    refresh();
}

// ========== 辅助函数 ==========

QString FundPage::formatFundType(FundType type)
{
    switch (type) {
        case FundType::ETF: return QStringLiteral("ETF");
        case FundType::LOF: return QStringLiteral("LOF");
        case FundType::OpenEnd: return QStringLiteral("开放式");
        case FundType::ClosedEnd: return QStringLiteral("封闭式");
        case FundType::Money: return QStringLiteral("货币");
        case FundType::Bond: return QStringLiteral("债券");
        default: return QStringLiteral("未知");
    }
}
