/**
 * @file StockQuotesPage.cpp
 * @brief 股票行情页面实现 - 使用 Controller 模式
 * @author WealthPilot Team
 * @version 2.0.0 - MVVM 重构
 */

#include "StockQuotesPage.h"
#include "ui/components/StyleHelper.h"
#include "ui/styles/ButtonStyles.h"
#include "utils/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QProgressDialog>

using namespace Tokens;

namespace WealthPilot {

// ============================================================================
// StockQuotesPage 实现
// ============================================================================

StockQuotesPage::StockQuotesPage(QWidget* parent)
    : BasePage(parent)
{
    setupUI();
    setupController();
    setupConnections();

    LOG_DEBUG("StockQuotesPage created (MVVM)");
}

StockQuotesPage::~StockQuotesPage()
{
    LOG_DEBUG("StockQuotesPage destroyed");
}

void StockQuotesPage::initializePage()
{
    if (isInitialized()) {
        return;
    }

    // 初始化 Controller
    if (m_controller)
    {
        m_controller->initialize();
        m_controller->refreshData();
    }

    setInitialized(true);
    LOG_INFO("StockQuotesPage initialized");
}

// ============================================================================
// UI 初始化
// ============================================================================

void StockQuotesPage::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(24, 16, 24, 16);

    // ========== 标题栏 ==========
    QHBoxLayout* headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(12);
    
    QLabel* titleLabel = new QLabel(QStringLiteral("股票行情"), this);
    StyleHelper::setTitleLabel(titleLabel);
    headerLayout->addWidget(titleLabel);
    
    headerLayout->addStretch();

    // 数量标签
    m_countLabel = new QLabel(QStringLiteral("共 0 只股票"), this);
    StyleHelper::setLabelText(m_countLabel);
    headerLayout->addWidget(m_countLabel);

    mainLayout->addLayout(headerLayout);

    // ========== 工具栏 ==========
    QHBoxLayout* toolbarLayout = new QHBoxLayout();
    toolbarLayout->setSpacing(8);

    // 搜索框
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(QStringLiteral("搜索股票代码或名称"));
    m_searchEdit->setObjectName(QStringLiteral("searchEdit"));
    m_searchEdit->setFixedWidth(220);
    toolbarLayout->addWidget(m_searchEdit);

    // 筛选下拉框
    m_filterCombo = new QComboBox(this);
    m_filterCombo->addItem(QStringLiteral("全部"), QStringLiteral("all"));
    m_filterCombo->addItem(QStringLiteral("沪A"), QStringLiteral("sh"));
    m_filterCombo->addItem(QStringLiteral("深A"), QStringLiteral("sz"));
    m_filterCombo->addItem(QStringLiteral("创业板"), QStringLiteral("cyb"));
    m_filterCombo->addItem(QStringLiteral("科创板"), QStringLiteral("kcb"));
    m_filterCombo->setObjectName(QStringLiteral("filterCombo"));
    m_filterCombo->setFixedWidth(100);
    toolbarLayout->addWidget(m_filterCombo);

    toolbarLayout->addStretch();

    // 导出按钮
    m_exportBtn = new QPushButton(QStringLiteral("导出"), this);
    ButtonStyles::setExport(m_exportBtn);
    m_exportBtn->setFixedWidth(80);
    toolbarLayout->addWidget(m_exportBtn);

    // 刷新按钮
    m_refreshBtn = new QPushButton(QStringLiteral("刷新"), this);
    ButtonStyles::setRefresh(m_refreshBtn);
    m_refreshBtn->setFixedWidth(80);
    toolbarLayout->addWidget(m_refreshBtn);

    mainLayout->addLayout(toolbarLayout);

    // ========== 表格视图 ==========
    m_tableView = new QTableView(this);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->setSortingEnabled(true);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->verticalHeader()->setVisible(false);
    m_tableView->setObjectName(QStringLiteral("stockTableView"));

    // 设置列宽 (使用数字索引)
    m_tableView->setColumnWidth(0, 80); // 代码
    m_tableView->setColumnWidth(1, 120); // 名称
    m_tableView->setColumnWidth(2, 90); // 现价
    m_tableView->setColumnWidth(3, 90); // 涨跌额
    m_tableView->setColumnWidth(4, 90); // 涨跌幅
    m_tableView->setColumnWidth(5, 100); // 成交量
    m_tableView->setColumnWidth(6, 100); // 成交额
    m_tableView->setColumnWidth(7, 80); // 最高
    m_tableView->setColumnWidth(8, 80); // 最低

    mainLayout->addWidget(m_tableView);

    // ========== 状态栏 ==========
    QHBoxLayout* statusLayout = new QHBoxLayout();
    statusLayout->setSpacing(12);

    m_statusLabel = new QLabel(QStringLiteral("就绪"), this);
    StyleHelper::setLabelText(m_statusLabel);
    statusLayout->addWidget(m_statusLabel);

    statusLayout->addStretch();

    mainLayout->addLayout(statusLayout);
}

void StockQuotesPage::setupController()
{
    // 创建 Controller
    m_controller = new StockQuotesController(this);

    // 设置模型到表格
    m_tableView->setModel(m_controller->proxyModel());

    LOG_DEBUG("StockQuotesController created and bound to view");
}

void StockQuotesPage::setupConnections()
{
    // ========== UI 信号连接 ==========

    // 搜索框文本改变
    connect(m_searchEdit, &QLineEdit::textChanged,
            this, &StockQuotesPage::onSearchChanged);
    
    // 筛选条件改变
    connect(m_filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &StockQuotesPage::onFilterChanged);
    
    // 刷新按钮点击
    connect(m_refreshBtn, &QPushButton::clicked,
            this, &StockQuotesPage::onRefreshData);

    // 导出按钮点击
    connect(m_exportBtn, &QPushButton::clicked, this, [this]()
    {
        QString filePath = QFileDialog::getSaveFileName(
            this,
            QStringLiteral("导出CSV"),
            QString(),
            QStringLiteral("CSV文件 (*.csv)")
        );

        if (!filePath.isEmpty())
        {
            m_controller->exportToCSV(filePath);
        }
    });

    // 表格行双击
    connect(m_tableView, &QTableView::doubleClicked,
            this, &StockQuotesPage::onRowDoubleClicked);

    // ========== Controller 信号连接 ==========

    // 数据刷新完成
    connect(m_controller, &StockQuotesController::dataRefreshed,
            this, &StockQuotesPage::onDataRefreshed);

    // 数据筛选完成
    connect(m_controller, &StockQuotesController::dataFiltered,
            this, &StockQuotesPage::onDataFiltered);

    // 搜索完成
    connect(m_controller, &StockQuotesController::searchCompleted,
            this, &StockQuotesPage::onSearchCompleted);

    // 数据加载状态
    connect(m_controller, &StockQuotesController::dataLoading,
            this, &StockQuotesPage::onDataLoading);

    // 导出完成
    connect(m_controller, &StockQuotesController::exportCompleted,
            this, &StockQuotesPage::onExportCompleted);

    // 导出失败
    connect(m_controller, &StockQuotesController::exportFailed,
            this, [this](const QString& error)
            {
                QMessageBox::warning(this, QStringLiteral("导出失败"), error);
            });

    // 错误发生
    connect(m_controller, &ControllerBase::errorOccurred,
            this, &StockQuotesPage::onErrorOccurred);

    // 状态变化
    connect(m_controller, &ControllerBase::stateChanged,
            this, [this](const QString& key, const QVariant& value) {
                Q_UNUSED(key)
                Q_UNUSED(value)
                updateStatus();
            });
}

// ============================================================================
// UI 事件处理
// ============================================================================

void StockQuotesPage::onSearchChanged(const QString& text)
{
    if (m_controller)
    {
        m_controller->searchData(text);
    }
}

void StockQuotesPage::onFilterChanged(int index)
{
    if (m_controller)
    {
        QString market = m_filterCombo->itemData(index).toString();
        m_controller->filterByMarket(market);
    }
}

void StockQuotesPage::onRefreshData()
{
    if (m_controller)
    {
        m_controller->refreshData();
    }
}

void StockQuotesPage::onRowDoubleClicked(const QModelIndex& index)
{
    if (!index.isValid() || !m_controller)
    {
        return;
    }
    
    // 获取源模型索引
    QModelIndex sourceIndex = m_controller->proxyModel()->mapToSource(index);

    if (sourceIndex.row() >= 0)
    {
        // 获取股票代码和名称
        QString symbol = m_controller->proxyModel()->data(m_controller->proxyModel()->index(sourceIndex.row(), 0)).
                                       toString();
        QString name = m_controller->proxyModel()->data(m_controller->proxyModel()->index(sourceIndex.row(), 1)).
                                     toString();
        emit navigateToKLinePage(symbol, name);

        LOG_DEBUG(QString("Navigate to K-Line: %1 %2").arg(symbol, name));
    }
}

// ============================================================================
// Controller 信号处理
// ============================================================================

void StockQuotesPage::onDataRefreshed(int count)
{
    m_countLabel->setText(QStringLiteral("共 %1 只股票").arg(count));
    m_statusLabel->setText(QStringLiteral("已加载 %1 条数据").arg(count));

    LOG_INFO(QString("Data refreshed: %1 quotes").arg(count));
}

void StockQuotesPage::onDataFiltered(int visibleCount, int totalCount)
{
    m_countLabel->setText(QStringLiteral("显示 %1 / 共 %2 只股票")
                          .arg(visibleCount).arg(totalCount));

    LOG_DEBUG(QString("Data filtered: %1/%2").arg(visibleCount).arg(totalCount));
}

void StockQuotesPage::onSearchCompleted(int resultCount, const QString& keyword)
{
    Q_UNUSED(keyword)

    int totalCount = m_controller->totalCount();
    m_countLabel->setText(QStringLiteral("搜索结果: %1 / 共 %2 只股票")
                          .arg(resultCount).arg(totalCount));

    if (resultCount == 0)
    {
        m_statusLabel->setText(QStringLiteral("未找到匹配结果"));
    }
    else
    {
        m_statusLabel->setText(QStringLiteral("找到 %1 条匹配结果").arg(resultCount));
    }
}

void StockQuotesPage::onDataLoading(bool loading)
{
    m_refreshBtn->setEnabled(!loading);
    m_tableView->setEnabled(!loading);

    if (loading)
    {
        m_statusLabel->setText(QStringLiteral("加载中..."));
    }
}

void StockQuotesPage::onExportCompleted(const QString& filePath)
{
    m_statusLabel->setText(QStringLiteral("已导出到: %1").arg(filePath));

    QMessageBox::information(this, QStringLiteral("导出成功"),
                             QStringLiteral("数据已成功导出到:\n%1").arg(filePath));
}

void StockQuotesPage::onErrorOccurred(const QString& error)
{
    m_statusLabel->setText(QStringLiteral("错误: %1").arg(error));

    LOG_ERROR(QString("StockQuotesPage error: %1").arg(error));
}

void StockQuotesPage::updateStatus()
{
    // 可以根据 Controller 状态更新 UI
}

} // namespace WealthPilot