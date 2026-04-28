/**
 * @file StatusBarWidget.cpp
 * @brief Status Bar Widget Implementation
 */

#include "StatusBarWidget.h"
#include "core/config/Tokens.h"

#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QStatusBar>
#include <QTimer>
#include <QLineEdit>
#include <QTableWidget>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QPoint>

#include <ctp/service/CTPService.h>

#include "AIAssistantPanelWidget.h"
#include "NetworkIndicator.h"
#include "utils/Logger.h"


struct StatusBarWidget::Impl {
    QHBoxLayout * layout = nullptr;
    QLabel * aiStatusLabel = nullptr;
    QLabel * ctpStatusLabel = nullptr;
    QLabel * versionLabel = nullptr;
    QLabel * timeLabel = nullptr;
    QLabel * latencyLabel = nullptr;

    AIAssistantPanelWidget* aiPanel = nullptr;

    NetworkIndicator* networkIndicator = nullptr;

    QTimer *timer = nullptr;
    
    // 搜索相关
    QLineEdit* searchEdit = nullptr;
    QTableWidget* searchResultPopup = nullptr;
    QWidget* searchPopupWidget = nullptr;
};

StatusBarWidget::StatusBarWidget(QWidget *parent)
    : BaseWidget(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
}

StatusBarWidget::~StatusBarWidget()
{
    if (d->timer) {
        d->timer->stop();
    }
}

void StatusBarWidget::setupUI()
{
    d->layout = new QHBoxLayout(this);
    d->layout->setContentsMargins(10, 0, 10, 0);
    d->layout->setSpacing(15);

    // Version label
    d->versionLabel = new QLabel("v2.0.0", this);
    d->versionLabel->setObjectName("versionLabel");
    d->layout->addWidget(d->versionLabel);

    d->layout->addStretch(1);

    // AI status
    d->aiStatusLabel = new QLabel("AI: Ready", this);
    d->aiStatusLabel->setObjectName("aiStatus");
    d->layout->addWidget(d->aiStatusLabel);

    // CTP status
    d->ctpStatusLabel = new QLabel("CTP: Disconnected", this);
    d->ctpStatusLabel->setObjectName("ctpStatus");
    d->layout->addWidget(d->ctpStatusLabel);

    // Network indicator
    d->networkIndicator = new NetworkIndicator(this);
    d->networkIndicator->setCheckInterval(3000);
    d->networkIndicator->startMonitoring();
    d->networkIndicator->setIndicatorSize(20, 13);

    d->networkIndicator->setColorScheme(NetworkIndicator::ExcellentSignal, QColor(0, 255, 0));
    d->layout->addWidget(d->networkIndicator);

    // 搜索框
    setupSearch();

    // Latency label
    d->latencyLabel = new QLabel("Checking...");
    d->latencyLabel->setStyleSheet("QLabel { color: #888; font-size: 12px; }");
    d->layout->addWidget(d->latencyLabel);

    // Time display
    d->timeLabel = new QLabel(this);
    d->timeLabel->setObjectName("timeLabel");
    d->layout->addWidget(d->timeLabel);

    // Timer for time update
    d->timer = new QTimer(this);
    QObject::connect(d->timer, &QTimer::timeout, this, &StatusBarWidget::updateTime);
    d->timer->start(1000);

    updateTime();
}

void StatusBarWidget::updateTime()
{
    QDateTime now = QDateTime::currentDateTime();
    d->timeLabel->setText(now.toString("yyyy-MM-dd hh:mm:ss"));
}

void StatusBarWidget::setAIStatus(const QString& status)
{
    d->aiStatusLabel->setText(QString("AI: %1").arg(status));
}

void StatusBarWidget::setCTPStatus(const QString& status)
{
    d->ctpStatusLabel->setText(QString("CTP: %1").arg(status));
}

void StatusBarWidget::setLatency(const QString& latency)
{
    d->latencyLabel->setText(latency);
}

void StatusBarWidget::setVersion(const QString& version)
{
    d->versionLabel->setText(version);
}

void StatusBarWidget::setupSearch()
{
    // 搜索框
    d->searchEdit = new QLineEdit(this);
    d->searchEdit->setPlaceholderText(QStringLiteral("搜索股票"));
    d->searchEdit->setFixedWidth(120);
    d->searchEdit->setFixedHeight(24);
    d->searchEdit->setStyleSheet(R"(
        QLineEdit {
            background: #2d3748;
            color: #ffffff;
            border: none;
            padding: 0 8px;
            font-size: 12px;
            border-radius: 4px;
        }
        QLineEdit::placeholder {
            color: #6b7280;
        }
    )");
    connect(d->searchEdit, &QLineEdit::textChanged, this, &StatusBarWidget::onSearchTextChanged);
    d->layout->addWidget(d->searchEdit);
}

void StatusBarWidget::onSearchTextChanged(const QString& text)
{
    // 如果搜索框为空，隐藏弹窗
    if (text.isEmpty()) {
        if (d->searchPopupWidget) {
            d->searchPopupWidget->hide();
        }
        return;
    }
    
    // 创建搜索结果弹窗
    if (!d->searchPopupWidget) {
        d->searchPopupWidget = new QWidget(this);
        d->searchPopupWidget->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
        d->searchPopupWidget->setStyleSheet("QWidget { background: #1a1f2e; border: 1px solid #2d3748; }");
        
        auto* popupLayout = new QVBoxLayout(d->searchPopupWidget);
        popupLayout->setContentsMargins(0, 0, 0, 0);
        popupLayout->setSpacing(0);
        
        d->searchResultPopup = new QTableWidget(d->searchPopupWidget);
        d->searchResultPopup->setColumnCount(3);
        d->searchResultPopup->setHorizontalHeaderLabels({QStringLiteral("代码"), QStringLiteral("名称"), QStringLiteral("板块")});
        d->searchResultPopup->horizontalHeader()->setStretchLastSection(true);
        d->searchResultPopup->setEditTriggers(QAbstractItemView::NoEditTriggers);
        d->searchResultPopup->setSelectionBehavior(QAbstractItemView::SelectRows);
        d->searchResultPopup->setSelectionMode(QAbstractItemView::SingleSelection);
        d->searchResultPopup->verticalHeader()->setVisible(false);
        d->searchResultPopup->setShowGrid(false);
        d->searchResultPopup->setStyleSheet(R"(
            QTableWidget {
                background: #1a1f2e;
                color: #ffffff;
                border: none;
                font-size: 12px;
            }
            QTableWidget::item {
                padding: 6px 8px;
            }
            QTableWidget::item:selected {
                background: #3b82f6;
            }
            QHeaderView::section {
                background: #2d3748;
                color: #6b7280;
                border: none;
                padding: 6px;
                font-size: 11px;
            }
        )");
        d->searchResultPopup->setMinimumWidth(300);
        d->searchResultPopup->setMinimumHeight(200);
        
        connect(d->searchResultPopup, &QTableWidget::cellClicked, this, &StatusBarWidget::onSearchResultClicked);
        
        popupLayout->addWidget(d->searchResultPopup);
    }
    
    // 模糊匹配股票（模拟数据）
    QVector<QVector<QString>> results;
    
    // 模拟股票数据库
    static const QVector<QVector<QString>> stockDatabase = {
        {"300085", QStringLiteral("银之杰"), QStringLiteral("软件服务")},
        {"300059", QStringLiteral("东方财富"), QStringLiteral("互联网金融")},
        {"000001", QStringLiteral("平安银行"), QStringLiteral("银行")},
        {"000002", QStringLiteral("万科A"), QStringLiteral("房地产")},
        {"600000", QStringLiteral("浦发银行"), QStringLiteral("银行")},
        {"600036", QStringLiteral("招商银行"), QStringLiteral("银行")},
        {"600519", QStringLiteral("贵州茅台"), QStringLiteral("白酒")},
        {"000858", QStringLiteral("五粮液"), QStringLiteral("白酒")},
        {"002415", QStringLiteral("海康威视"), QStringLiteral("安防")},
        {"300750", QStringLiteral("宁德时代"), QStringLiteral("新能源")},
        {"601318", QStringLiteral("中国平安"), QStringLiteral("保险")},
        {"000333", QStringLiteral("美的集团"), QStringLiteral("家电")},
        {"002594", QStringLiteral("比亚迪"), QStringLiteral("新能源汽车")},
        {"600900", QStringLiteral("长江电力"), QStringLiteral("电力")},
        {"601012", QStringLiteral("隆基绿能"), QStringLiteral("光伏")},
    };
    
    // 模糊匹配
    for (const auto& stock : stockDatabase) {
        if (stock[0].contains(text, Qt::CaseInsensitive) || 
            stock[1].contains(text, Qt::CaseInsensitive)) {
            results.append(stock);
        }
    }
    
    // 更新搜索结果表格
    d->searchResultPopup->setRowCount(results.size());
    for (int i = 0; i < results.size(); ++i) {
        for (int j = 0; j < 3; ++j) {
            auto* item = new QTableWidgetItem(results[i][j]);
            if (j == 0) {
                item->setForeground(QColor("#3b82f6"));  // 代码蓝色
            }
            d->searchResultPopup->setItem(i, j, item);
        }
    }
    
    // 显示弹窗在搜索框下方（右下角）
    if (results.size() > 0) {
        QPoint pos = d->searchEdit->mapToGlobal(QPoint(0, d->searchEdit->height()));
        // 调整位置到右下角
        pos.setX(pos.x() - 160);  // 向左偏移
        d->searchPopupWidget->move(pos);
        d->searchPopupWidget->show();
        d->searchPopupWidget->setFocus();
    } else {
        d->searchPopupWidget->hide();
    }
}

void StatusBarWidget::onSearchResultClicked(int row, int column)
{
    if (!d->searchResultPopup || row < 0) return;
    
    // 获取选中的股票代码和名称
    QString code = d->searchResultPopup->item(row, 0)->text();
    QString name = d->searchResultPopup->item(row, 1)->text();
    
    // 确定交易所
    QString exchange;
    if (code.startsWith("6")) {
        exchange = "SH";
    } else if (code.startsWith("0") || code.startsWith("3")) {
        exchange = "SZ";
    } else {
        exchange = "SZ";
    }
    
    // 发送股票选择信号
    emit stockSelected(code, name, exchange);
    
    // 清空搜索框并隐藏弹窗
    d->searchEdit->clear();
    d->searchPopupWidget->hide();
    
    LOG_INFO(QString("Stock selected from statusbar: %1 (%2)").arg(name, code));
}
