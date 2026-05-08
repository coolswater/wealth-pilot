/**
 * @file ConditionOrderPage.cpp
 * @brief 条件单页面实现
 */

#include "ConditionOrderPage.h"
#include "core/config/Tokens.h"
#include "ui/components/PageStyles.h"
#include "trading/ConditionOrderEngine.h"
#include "utils/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>

namespace WealthPilot {

ConditionOrderPage::ConditionOrderPage(QWidget* parent)
    : BasePage(parent)
    , m_table(new QTableWidget(this))
    , m_addBtn(new QPushButton(QStringLiteral("添加"), this))
    , m_editBtn(new QPushButton(QStringLiteral("修改"), this))
    , m_deleteBtn(new QPushButton(QStringLiteral("删除"), this))
    , m_refreshBtn(new QPushButton(QStringLiteral("刷新"), this))
{
    setupUI();
}

ConditionOrderPage::~ConditionOrderPage() = default;

void ConditionOrderPage::initializePage()
{
    if (isInitialized()) return;
    
    // 加载示例数据
    updateTable();
    setInitialized(true);
    LOG_DEBUG("ConditionOrderPage initialized");
}

void ConditionOrderPage::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(Tokens::Spacing::MD, Tokens::Spacing::MD, Tokens::Spacing::MD, Tokens::Spacing::MD);
    mainLayout->setSpacing(Tokens::Spacing::SM);

    // 顶部工具栏
    auto* toolbarLayout = new QHBoxLayout();
    toolbarLayout->setSpacing(8);

    toolbarLayout->addWidget(m_addBtn);
    toolbarLayout->addWidget(m_editBtn);
    toolbarLayout->addWidget(m_deleteBtn);
    toolbarLayout->addStretch();
    toolbarLayout->addWidget(m_refreshBtn);

    mainLayout->addLayout(toolbarLayout);

    // 表格
    m_table->setColumnCount(7);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("订单ID"),
        QStringLiteral("合约"),
        QStringLiteral("条件类型"),
        QStringLiteral("触发价格"),
        QStringLiteral("委托价格"),
        QStringLiteral("数量"),
        QStringLiteral("状态")
    });
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->setVisible(false);

    mainLayout->addWidget(m_table);

    // 连接信号
    connect(m_addBtn, &QPushButton::clicked, this, &ConditionOrderPage::onAddClicked);
    connect(m_editBtn, &QPushButton::clicked, this, &ConditionOrderPage::onEditClicked);
    connect(m_deleteBtn, &QPushButton::clicked, this, &ConditionOrderPage::onDeleteClicked);
    connect(m_refreshBtn, &QPushButton::clicked, this, &ConditionOrderPage::onRefreshClicked);

    // 设置样式
    m_table->setStyleSheet(QString(R"(
        QTableWidget {
            background-color: %1;
            alternate-background-color: %2;
            gridline-color: %3;
            border: 1px solid %3;
            border-radius: 4px;
        }
        QTableWidget::item {
            padding: 4px;
        }
        QHeaderView::section {
            background-color: %2;
            color: %4;
            padding: 6px;
            border: none;
            border-bottom: 1px solid %3;
            font-weight: bold;
        }
    )").arg(Tokens::Colors::BgSurface, Tokens::Colors::BgBase, Tokens::Colors::Border, Tokens::Colors::TextSecondary));

    m_addBtn->setStyleSheet(PageStyles::primaryButton());
    m_editBtn->setStyleSheet(PageStyles::secondaryButton());
    m_deleteBtn->setStyleSheet(PageStyles::secondaryButton());
    m_refreshBtn->setStyleSheet(PageStyles::secondaryButton());
}

void ConditionOrderPage::updateTable()
{
    m_table->setRowCount(0);
    
    // 实现数据加载
    // TODO: 实际应用中应该从 ConditionOrderEngine 获取数据
    // 由于类型不匹配，暂时使用演示数据
    
    // 添加演示数据
    QVector<ConditionOrder> demoOrders;
    
    ConditionOrder order1;
    order1.orderId = QStringLiteral("CO001");
    order1.instrumentId = QStringLiteral("sh600000");
    order1.conditionType = ConditionType::StopLoss;
    order1.triggerPrice = 10.50;
    order1.orderPrice = 10.50;
    order1.quantity = 1000;
    order1.status = ConditionOrderStatus::Pending;
    demoOrders.append(order1);
    
    ConditionOrder order2;
    order2.orderId = QStringLiteral("CO002");
    order2.instrumentId = QStringLiteral("sz000001");
    order2.conditionType = ConditionType::TakeProfit;
    order2.triggerPrice = 15.00;
    order2.orderPrice = 15.00;
    order2.quantity = 500;
    order2.status = ConditionOrderStatus::Pending;
    demoOrders.append(order2);
    
    m_table->setRowCount(demoOrders.size());
    
    m_table->setRowCount(demoOrders.size());
    
    for (int i = 0; i < demoOrders.size(); ++i) {
        const auto& order = demoOrders[i];
        
        // 订单ID
        m_table->setItem(i, 0, new QTableWidgetItem(order.orderId));
        
        // 合约代码
        m_table->setItem(i, 1, new QTableWidgetItem(order.instrumentId));
        
        // 条件类型
        QString typeStr;
        switch (order.conditionType) {
            case ConditionType::StopLoss: typeStr = QStringLiteral("止损"); break;
            case ConditionType::TakeProfit: typeStr = QStringLiteral("止盈"); break;
            case ConditionType::TrailingStop: typeStr = QStringLiteral("跟踪止损"); break;
            case ConditionType::PriceTrigger: typeStr = QStringLiteral("价格触发"); break;
            case ConditionType::TimeTrigger: typeStr = QStringLiteral("时间触发"); break;
        }
        m_table->setItem(i, 2, new QTableWidgetItem(typeStr));
        
        // 触发价格
        m_table->setItem(i, 3, new QTableWidgetItem(QString::number(order.triggerPrice, 'f', 2)));
        
        // 委托价格
        m_table->setItem(i, 4, new QTableWidgetItem(QString::number(order.orderPrice, 'f', 2)));
        
        // 数量
        m_table->setItem(i, 5, new QTableWidgetItem(QString::number(order.quantity)));
        
        // 状态
        QString statusStr;
        switch (order.status) {
            case ConditionOrderStatus::Pending: statusStr = QStringLiteral("待触发"); break;
            case ConditionOrderStatus::Triggered: statusStr = QStringLiteral("已触发"); break;
            case ConditionOrderStatus::Cancelled: statusStr = QStringLiteral("已取消"); break;
            case ConditionOrderStatus::Expired: statusStr = QStringLiteral("已过期"); break;
        }
        m_table->setItem(i, 6, new QTableWidgetItem(statusStr));
    }
    
    LOG_DEBUG(QString("Condition order table updated: %1 rows").arg(demoOrders.size()));
}

void ConditionOrderPage::onAddClicked()
{
    QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("添加条件单功能待实现"));
    LOG_DEBUG("Add condition order clicked");
}

void ConditionOrderPage::onEditClicked()
{
    QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("修改条件单功能待实现"));
    LOG_DEBUG("Edit condition order clicked");
}

void ConditionOrderPage::onDeleteClicked()
{
    QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("删除条件单功能待实现"));
    LOG_DEBUG("Delete condition order clicked");
}

void ConditionOrderPage::onRefreshClicked()
{
    initializePage();
    LOG_DEBUG("ConditionOrderPage refreshed");
}

} // namespace WealthPilot