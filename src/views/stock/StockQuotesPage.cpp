#include "StockQuotesPage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QComboBox>
#include <QLineEdit>
#include <QRandomGenerator>

#include <ui/components/CardWidget.h>

struct StockQuotesPage::Impl {
    QTableWidget* table = nullptr;
    QComboBox* filterCombo = nullptr;
};

StockQuotesPage::StockQuotesPage(QWidget *parent)
    : BasePage(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
}

StockQuotesPage::~StockQuotesPage() = default;

QString StockQuotesPage::pageId() const
{
    return QStringLiteral("StockQuotesPage");
}

void StockQuotesPage::initializePage()
{

}

void StockQuotesPage::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(24);

    // 头部
    QHBoxLayout* headerLayout = new QHBoxLayout();

    QLabel* titleLabel = new QLabel("行情中心", this);
    titleLabel->setStyleSheet("font-size: 28px; font-weight: 700; color: white;");
    headerLayout->addWidget(titleLabel);

    headerLayout->addStretch();

    // 搜索框
    QLineEdit* searchEdit = new QLineEdit(this);
    searchEdit->setPlaceholderText("搜索股票代码或名称...");
    searchEdit->setFixedWidth(280);
    searchEdit->setFixedHeight(40);
    searchEdit->setStyleSheet(R"(
        QLineEdit {
            background-color: rgba(255, 255, 255, 0.05);
            border: 1px solid rgba(255, 255, 255, 0.1);
            border-radius: 20px;
            padding: 0 16px;
            color: white;
        }
    )");
    headerLayout->addWidget(searchEdit);

    mainLayout->addLayout(headerLayout);

    // 筛选栏
    QHBoxLayout* filterLayout = new QHBoxLayout();

    QStringList tabs = {"全部", "板块", "概念", "地域", "自选"};
    for (const QString& tab : tabs) {
        QPushButton* btn = new QPushButton(tab, this);
        btn->setCheckable(true);
        btn->setFixedHeight(36);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(R"(
            QPushButton {
                background-color: transparent;
                color: #9CA3AF;
                border: none;
                border-bottom: 2px solid transparent;
                padding: 0 20px;
                font-size: 14px;
            }
            QPushButton:checked {
                color: #3B82F6;
                border-bottom: 2px solid #3B82F6;
            }
        )");
        filterLayout->addWidget(btn);
    }

    filterLayout->addStretch();

    d->filterCombo = new QComboBox(this);
    d->filterCombo->addItems({"全部行业", "科技", "金融", "消费", "医药", "能源"});
    d->filterCombo->setFixedWidth(140);
    d->filterCombo->setStyleSheet(R"(
        QComboBox {
            background-color: rgba(255, 255, 255, 0.05);
            border: 1px solid rgba(255, 255, 255, 0.1);
            border-radius: 6px;
            padding: 8px 12px;
            color: white;
        }
    )");
    filterLayout->addWidget(d->filterCombo);

    mainLayout->addLayout(filterLayout);

    // 数据表格
    CardWidget* tableCard = new CardWidget("", this);

    d->table = new QTableWidget(this);
    d->table->setColumnCount(7);
    d->table->setHorizontalHeaderLabels({"名称", "最新价", "涨跌幅", "涨跌额", "成交量", "成交额", "操作"});
    d->table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    d->table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    d->table->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Fixed);
    d->table->setColumnWidth(0, 150);
    d->table->setColumnWidth(6, 100);
    d->table->setSelectionBehavior(QAbstractItemView::SelectRows);
    d->table->setAlternatingRowColors(true);
    d->table->verticalHeader()->setVisible(false);
    d->table->setStyleSheet(R"(
        QTableWidget {
            background-color: transparent;
            border: none;
            gridline-color: rgba(255, 255, 255, 0.05);
        }
        QTableWidget::item {
            padding: 12px;
            border-bottom: 1px solid rgba(255, 255, 255, 0.05);
            color: white;
        }
        QTableWidget::item:selected {
            background-color: rgba(59, 130, 246, 0.2);
        }
    )");

    // 添加示例数据
    QStringList names = {"贵州茅台", "比亚迪", "宁德时代", "中国平安", "招商银行"};
    QList<double> prices = {1850.00, 245.60, 198.50, 45.80, 38.90};
    QList<double> changes = {1.21, -0.85, 2.34, 0.52, -0.23};

    d->table->setRowCount(names.size());
    for (int i = 0; i < names.size(); ++i) {
        d->table->setItem(i, 0, new QTableWidgetItem(names[i]));
        d->table->setItem(i, 1, new QTableWidgetItem(QString::number(prices[i], 'f', 2)));

        QString changeText = QString("%1%").arg(changes[i], 0, 'f', 2);
        QTableWidgetItem* changeItem = new QTableWidgetItem(changeText);
        changeItem->setForeground(changes[i] >= 0 ? QColor("#10B981") : QColor("#EF4444"));
        d->table->setItem(i, 2, changeItem);

        double changeAmount = prices[i] * changes[i] / 100;
        QTableWidgetItem* amountItem = new QTableWidgetItem(QString::number(changeAmount, 'f', 2));
        amountItem->setForeground(changes[i] >= 0 ? QColor("#10B981") : QColor("#EF4444"));
        d->table->setItem(i, 3, amountItem);

        d->table->setItem(i, 4, new QTableWidgetItem(QString("%1万").arg(QRandomGenerator::global()->bounded(10, 110)))); // 10 到 109
        d->table->setItem(i, 5, new QTableWidgetItem(QString("%1亿").arg(QRandomGenerator::global()->bounded(5, 55))));  // 5 到 54

        QPushButton* actionBtn = new QPushButton("查看");
        actionBtn->setFixedSize(60, 28);
        actionBtn->setStyleSheet(R"(
            QPushButton {
                background-color: rgba(59, 130, 246, 0.2);
                color: #3B82F6;
                border: none;
                border-radius: 4px;
                font-size: 12px;
            }
        )");
        d->table->setCellWidget(i, 6, actionBtn);
    }

    tableCard->setContent(d->table);
    mainLayout->addWidget(tableCard);
}
