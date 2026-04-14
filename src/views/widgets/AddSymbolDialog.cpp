/**
 * @file AddSymbolDialog.cpp
 * @brief 添加自选对话框实现
 */

#include "AddSymbolDialog.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>

AddSymbolDialog::AddSymbolDialog(const QStringList &availableSymbols,
                                 const QStringList &existingSymbols,
                                 QWidget *parent)
    : QDialog(parent)
    , m_allSymbols(availableSymbols)
    , m_existingSymbols(existingSymbols)
{
    setupUI();
    setWindowTitle(tr("添加自选"));
    setMinimumSize(400, 500);
}

void AddSymbolDialog::setupUI()
{
    auto *layout = new QVBoxLayout(this);

    // 搜索框
    auto *searchLayout = new QHBoxLayout();
    searchLayout->addWidget(new QLabel(tr("搜索:")));
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("输入交易对，如 BTC-USDT"));
    searchLayout->addWidget(m_searchEdit);
    layout->addLayout(searchLayout);

    // 分类筛选
    auto *categoryLayout = new QHBoxLayout();
    categoryLayout->addWidget(new QLabel(tr("分类:")));
    m_categoryCombo = new QComboBox(this);
    m_categoryCombo->addItem(tr("全部"));
    m_categoryCombo->addItem(tr("USDT交易对"));
    m_categoryCombo->addItem(tr("USD交易对"));
    m_categoryCombo->addItem(tr("加密货币"));
    categoryLayout->addWidget(m_categoryCombo);
    layout->addLayout(categoryLayout);

    // 交易对列表
    m_symbolList = new QListWidget(this);
    m_symbolList->setSelectionMode(QAbstractItemView::MultiSelection);
    m_symbolList->setAlternatingRowColors(true);
    layout->addWidget(m_symbolList);

    // 按钮
    auto *buttonLayout = new QHBoxLayout();
    m_addButton = new QPushButton(tr("添加选中"), this);
    m_addButton->setEnabled(false);
    m_cancelButton = new QPushButton(tr("取消"), this);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_addButton);
    buttonLayout->addWidget(m_cancelButton);
    layout->addLayout(buttonLayout);

    // 连接信号
    connect(m_searchEdit, &QLineEdit::textChanged,
            this, &AddSymbolDialog::onSearchTextChanged);
    connect(m_symbolList, &QListWidget::itemSelectionChanged, [this]() {
        m_addButton->setEnabled(!m_symbolList->selectedItems().isEmpty());
    });
    connect(m_symbolList, &QListWidget::itemDoubleClicked,
            this, &AddSymbolDialog::onSymbolSelected);
    connect(m_addButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_categoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AddSymbolDialog::onCategoryChanged);

    // 初始化搜索防抖定时器
    m_searchTimer = new QTimer(this);
    m_searchTimer->setSingleShot(true);
    m_searchTimer->setInterval(300);
    connect(m_searchTimer, &QTimer::timeout, this, &AddSymbolDialog::updateSearchResults);

    // 初始加载所有交易对
    filterSymbols("");
}

void AddSymbolDialog::onSearchTextChanged(const QString &text) const
{
    // 防抖处理，避免频繁搜索
    m_searchTimer->stop();
    m_searchTimer->start();
}

void AddSymbolDialog::updateSearchResults()
{
    filterSymbols(m_searchEdit->text());
}

void AddSymbolDialog::filterSymbols(const QString &filter)
{
    m_symbolList->clear();

    QString upperFilter = filter.toUpper();

    for (const QString &symbol : m_allSymbols) {
        // 如果已在自选列表中，跳过
        if (m_existingSymbols.contains(symbol)) {
            continue;
        }

        // 搜索过滤
        if (!upperFilter.isEmpty() && !symbol.contains(upperFilter)) {
            continue;
        }

        // 分类过滤
        int categoryIndex = m_categoryCombo->currentIndex();
        if (categoryIndex == 1 && !symbol.endsWith("-USDT")) continue;  // USDT交易对
        if (categoryIndex == 2 && !symbol.endsWith("-USD")) continue;   // USD交易对
        if (categoryIndex == 3 && (symbol.contains("-USDT") || symbol.contains("-USD"))) continue;

        auto *item = new QListWidgetItem(symbol);

        // 高亮显示热门交易对
        if (symbol == "BTC-USDT" || symbol == "ETH-USDT") {
            item->setBackground(QColor(60, 60, 60));
            item->setForeground(Qt::yellow);
        }

        m_symbolList->addItem(item);
    }

    // 显示结果数量
    setWindowTitle(tr("添加自选 - 找到 %1 个交易对").arg(m_symbolList->count()));
}

void AddSymbolDialog::onSymbolSelected(const QListWidgetItem *item)
{
    if (!item) return;

    QString symbol = item->text();
    if (!m_selectedSymbols.contains(symbol)) {
        m_selectedSymbols.append(symbol);
    }
    accept();
}

void AddSymbolDialog::onCategoryChanged(int index)
{
    Q_UNUSED(index)
    filterSymbols(m_searchEdit->text());
}

QStringList AddSymbolDialog::selectedSymbols() const
{
    QStringList result;
    for (QListWidgetItem *item : m_symbolList->selectedItems()) {
        result.append(item->text());
    }
    return result;
}