/**
 * @file PageTemplate.cpp
 * @brief 页面模板系统实现
 */

#include "PageTemplate.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGraphicsDropShadowEffect>
#include <QProgressBar>
#include <QStandardItemModel>
#include <QHeaderView>

namespace WealthPilot {
namespace UI {

// 颜色常量
namespace Colors {
    const QString BgSurface = "#1E1F24";
    const QString BgElevated = "#2C2D33";
    const QString Border = "#3A3B42";
    const QString TextPrimary = "#FFFFFF";
    const QString TextSecondary = "#8E8E93";
    const QString TextTertiary = "#636366";
    const QString Primary = "#007AFF";
}

QFrame* PageTemplate::createPageHeader(QWidget* parent, const QString& title,
                                        bool showSearch, bool showRefresh)
{
    auto* header = new QFrame(parent);
    header->setObjectName("pageHeader");
    header->setFixedHeight(56);
    header->setStyleSheet(QString("QFrame#pageHeader { background-color: %1; border-bottom: 1px solid %2; }")
                          .arg(Colors::BgSurface, Colors::Border));

    auto* layout = new QHBoxLayout(header);
    layout->setContentsMargins(20, 0, 20, 0);
    layout->setSpacing(16);

    auto* titleLabel = new QLabel(title, header);
    titleLabel->setStyleSheet(QString("color: %1; font-size: 18px; font-weight: bold;").arg(Colors::TextPrimary));
    layout->addWidget(titleLabel);
    layout->addStretch();

    if (showSearch) {
        auto* searchEdit = new QLineEdit(header);
        searchEdit->setPlaceholderText(QStringLiteral("搜索..."));
        searchEdit->setFixedWidth(240);
        searchEdit->setFixedHeight(36);
        searchEdit->setStyleSheet(QString("QLineEdit { background-color: %1; border: 1px solid %2; border-radius: 8px; padding: 0 12px; color: %3; }")
                                  .arg(Colors::BgElevated, Colors::Border, Colors::TextPrimary));
        layout->addWidget(searchEdit);
    }

    if (showRefresh) {
        auto* refreshBtn = new QPushButton(QStringLiteral("刷新"), header);
        refreshBtn->setFixedSize(80, 36);
        refreshBtn->setStyleSheet(standardButtonStyleSheet());
        layout->addWidget(refreshBtn);
    }

    return header;
}

QFrame* PageTemplate::createSummaryCard(QWidget* parent, const QString& title,
                                         const QString& value, const QString& detail)
{
    auto* card = new QFrame(parent);
    card->setObjectName("summaryCard");
    card->setFixedSize(200, 100);
    applyCardStyle(card);

    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(8);

    auto* titleLabel = new QLabel(title, card);
    titleLabel->setStyleSheet(QString("color: %1; font-size: 13px;").arg(Colors::TextSecondary));
    layout->addWidget(titleLabel);

    auto* valueLabel = new QLabel(value, card);
    valueLabel->setObjectName("cardValue");
    valueLabel->setStyleSheet(QString("color: %1; font-size: 24px; font-weight: bold;").arg(Colors::TextPrimary));
    layout->addWidget(valueLabel);

    if (!detail.isEmpty()) {
        auto* detailLabel = new QLabel(detail, card);
        detailLabel->setStyleSheet(QString("color: %1; font-size: 12px;").arg(Colors::TextTertiary));
        layout->addWidget(detailLabel);
    }

    layout->addStretch();
    return card;
}

QTableView* PageTemplate::createStandardTable(QWidget* parent, const QStringList& headers)
{
    auto* table = new QTableView(parent);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setAlternatingRowColors(true);
    table->setSortingEnabled(true);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setStretchLastSection(true);
    applyTableStyle(table);

    if (!headers.isEmpty()) {
        auto* model = new QStandardItemModel(table);
        model->setHorizontalHeaderLabels(headers);
        table->setModel(model);
    }

    return table;
}

QTabWidget* PageTemplate::createStandardTabs(QWidget* parent, const QStringList& tabs)
{
    auto* tabWidget = new QTabWidget(parent);
    tabWidget->setStyleSheet(QString(
        "QTabWidget::pane { border: 1px solid %1; border-radius: 8px; background-color: %2; }"
        "QTabBar::tab { background-color: transparent; color: %3; padding: 8px 16px; border: none; }"
        "QTabBar::tab:selected { color: %4; border-bottom: 2px solid %4; }")
        .arg(Colors::Border, Colors::BgElevated, Colors::TextSecondary, Colors::Primary));

    for (const QString& tabName : tabs) {
        tabWidget->addTab(new QWidget(tabWidget), tabName);
    }

    return tabWidget;
}

QFrame* PageTemplate::createEmptyState(QWidget* parent, const QString& message, const QString& icon)
{
    auto* container = new QFrame(parent);
    auto* layout = new QVBoxLayout(container);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(16);

    if (!icon.isEmpty()) {
        auto* iconLabel = new QLabel(icon, container);
        iconLabel->setStyleSheet("font-size: 48px;");
        iconLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(iconLabel);
    }

    auto* msgLabel = new QLabel(message, container);
    msgLabel->setStyleSheet(QString("color: %1; font-size: 14px;").arg(Colors::TextTertiary));
    msgLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(msgLabel);

    return container;
}

void PageTemplate::applyCardStyle(QFrame* card, const CardStyle& style)
{
    card->setStyleSheet(QString("QFrame#summaryCard { background-color: %1; border: 1px solid %2; border-radius: %3px; }")
                        .arg(style.backgroundColor, style.borderColor).arg(style.borderRadius));

    auto* shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(8);
    shadow->setColor(QColor(0, 0, 0, 50));
    shadow->setOffset(0, 2);
    card->setGraphicsEffect(shadow);
}

void PageTemplate::applyTableStyle(QTableView* table, const TableStyle& style)
{
    table->setStyleSheet(QString(
        "QTableView { background-color: %1; border: none; gridline-color: %2; selection-background-color: %3; }"
        "QTableView::item { padding: 8px; color: %4; }"
        "QHeaderView::section { background-color: %5; color: %6; padding: 8px; border: none; }")
        .arg(style.rowBackground, style.gridLineColor, Colors::Primary, style.rowTextColor,
             style.headerBackground, style.headerTextColor));
}

QString PageTemplate::standardButtonStyleSheet()
{
    return QString(
        "QPushButton { background-color: %1; color: %2; border: none; border-radius: 6px; padding: 8px 16px; }"
        "QPushButton:hover { background-color: %3; }"
        "QPushButton:pressed { background-color: %4; }")
        .arg(Colors::Primary, Colors::TextPrimary, "#0066CC", "#0055AA");
}

QString PageTemplate::standardCardStyleSheet()
{
    return QString("QFrame { background-color: %1; border: 1px solid %2; border-radius: 8px; }")
           .arg(Colors::BgElevated, Colors::Border);
}

QString PageTemplate::standardTableStyleSheet()
{
    return QString("QTableView { background-color: %1; border: none; gridline-color: %2; }")
           .arg(Colors::BgElevated, Colors::Border);
}

// 其他方法的简化实现
QFrame* PageTemplate::createPageHeaderWithNav(QWidget* parent, const QString& title, const QStringList& navItems)
{
    auto* header = new QFrame(parent);
    header->setFixedHeight(80);
    auto* layout = new QVBoxLayout(header);
    layout->setContentsMargins(20, 12, 20, 12);

    auto* titleLabel = new QLabel(title, header);
    titleLabel->setStyleSheet(QString("color: %1; font-size: 18px; font-weight: bold;").arg(Colors::TextPrimary));
    layout->addWidget(titleLabel);

    auto* navLayout = new QHBoxLayout();
    for (const QString& item : navItems) {
        auto* btn = new QPushButton(item, header);
        btn->setCheckable(true);
        btn->setFixedHeight(28);
        navLayout->addWidget(btn);
    }
    navLayout->addStretch();
    layout->addLayout(navLayout);

    return header;
}

QFrame* PageTemplate::createSummaryCardRow(QWidget* parent, const QVector<QPair<QString, QString>>& cards)
{
    auto* container = new QFrame(parent);
    auto* layout = new QHBoxLayout(container);
    layout->setSpacing(16);

    for (const auto& card : cards) {
        layout->addWidget(createSummaryCard(container, card.first, card.second));
    }
    layout->addStretch();

    return container;
}

QFrame* PageTemplate::createSummaryCardGrid(QWidget* parent, const QVector<QPair<QString, QString>>& cards, int columns)
{
    auto* container = new QFrame(parent);
    auto* layout = new QGridLayout(container);
    layout->setSpacing(16);

    int row = 0, col = 0;
    for (const auto& card : cards) {
        layout->addWidget(createSummaryCard(container, card.first, card.second), row, col);
        if (++col >= columns) { col = 0; row++; }
    }

    return container;
}

QFrame* PageTemplate::createTableWithSearch(QWidget* parent, const QStringList& headers, const QString& placeholder)
{
    auto* container = new QFrame(parent);
    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* searchEdit = new QLineEdit(container);
    searchEdit->setPlaceholderText(placeholder);
    layout->addWidget(searchEdit);

    layout->addWidget(createStandardTable(container, headers));

    return container;
}

QSplitter* PageTemplate::createVerticalSplitter(QWidget* parent)
{
    auto* splitter = new QSplitter(Qt::Vertical, parent);
    splitter->setHandleWidth(1);
    return splitter;
}

QSplitter* PageTemplate::createHorizontalSplitter(QWidget* parent)
{
    auto* splitter = new QSplitter(Qt::Horizontal, parent);
    splitter->setHandleWidth(1);
    return splitter;
}

QFrame* PageTemplate::createChartContainer(QWidget* parent, const QString& title)
{
    auto* container = new QFrame(parent);
    container->setStyleSheet(QString("QFrame { background-color: %1; border: 1px solid %2; border-radius: 8px; }")
                             .arg(Colors::BgElevated, Colors::Border));

    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(16, 12, 16, 12);

    if (!title.isEmpty()) {
        auto* titleLabel = new QLabel(title, container);
        titleLabel->setStyleSheet(QString("color: %1; font-size: 14px; font-weight: bold;").arg(Colors::TextPrimary));
        layout->addWidget(titleLabel);
    }

    layout->addWidget(new QFrame(container), 1);
    return container;
}

QScrollArea* PageTemplate::createScrollArea(QWidget* parent)
{
    auto* scrollArea = new QScrollArea(parent);
    scrollArea->setWidgetResizable(true);
    return scrollArea;
}

QFrame* PageTemplate::createButtonRow(QWidget* parent, const QVector<QPair<QString, std::function<void()>>>& buttons)
{
    auto* container = new QFrame(parent);
    auto* layout = new QHBoxLayout(container);
    layout->setSpacing(8);

    for (const auto& btn : buttons) {
        auto* button = new QPushButton(btn.first, container);
        button->setFixedHeight(36);
        button->setStyleSheet(standardButtonStyleSheet());
        if (btn.second) { QObject::connect(button, &QPushButton::clicked, btn.second); }
        layout->addWidget(button);
    }
    layout->addStretch();

    return container;
}

QFrame* PageTemplate::createLoadingState(QWidget* parent, const QString& message)
{
    auto* container = new QFrame(parent);
    auto* layout = new QVBoxLayout(container);
    layout->setAlignment(Qt::AlignCenter);

    auto* progressBar = new QProgressBar(container);
    progressBar->setRange(0, 0);
    progressBar->setFixedWidth(200);
    progressBar->setFixedHeight(4);
    layout->addWidget(progressBar, 0, Qt::AlignCenter);

    auto* msgLabel = new QLabel(message, container);
    msgLabel->setStyleSheet(QString("color: %1; font-size: 14px;").arg(Colors::TextTertiary));
    layout->addWidget(msgLabel);

    return container;
}

} // namespace UI
} // namespace WealthPilot