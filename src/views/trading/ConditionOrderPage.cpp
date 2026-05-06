/**
 * @file ConditionOrderPage.cpp
 * @brief 条件单页面实现
 */

#include "ConditionOrderPage.h"
#include "core/config/Tokens.h"
#include "ui/components/PageStyles.h"
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
    // TODO: 实现数据加载
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