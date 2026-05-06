/**
 * @file StockKLinePage.cpp
 * @brief 股票K线图页面实现
 */

#include "StockKLinePage.h"
#include "core/config/Tokens.h"
#include "utils/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QSplitter>
#include <QTimer>

struct StockKLinePage::Impl {
    QWidget* chartWidget = nullptr;
    QLabel* infoLabel = nullptr;
    QTimer* refreshTimer = nullptr;
};

StockKLinePage::StockKLinePage(QWidget* parent)
    : BasePage(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
    setupConnections();
}

StockKLinePage::~StockKLinePage() = default;

void StockKLinePage::setStock(const QString& stockCode, const QString& exchange)
{
    Q_UNUSED(exchange);
    m_stockCode = stockCode;
    
    // 更新显示
    if (m_stockNameLabel) {
        m_stockNameLabel->setText(QStringLiteral("股票: %1").arg(stockCode));
    }
    
    emit stockChanged(stockCode);
    LOG_DEBUG(QString("Stock set: %1").arg(stockCode));
}

QString StockKLinePage::stockCode() const
{
    return m_stockCode;
}

void StockKLinePage::setPeriod(StockKLinePeriod period)
{
    m_period = period;
    if (m_periodCombo) {
        m_periodCombo->setCurrentIndex(static_cast<int>(period));
    }
    emit periodChanged(static_cast<int>(period));
}

void StockKLinePage::setAdjustType(int adjust)
{
    Q_UNUSED(adjust);
    // TODO: 实现复权设置
}

void StockKLinePage::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // 顶部工具栏
    auto* toolbarLayout = new QHBoxLayout();
    toolbarLayout->setSpacing(8);

    // 股票名称标签
    m_stockNameLabel = new QLabel(QStringLiteral("股票: 未选择"), this);
    toolbarLayout->addWidget(m_stockNameLabel);

    toolbarLayout->addSpacing(20);

    // 周期选择
    toolbarLayout->addWidget(new QLabel(QStringLiteral("周期:"), this));
    m_periodCombo = new QComboBox(this);
    m_periodCombo->addItem(QStringLiteral("1分钟"), static_cast<int>(StockKLinePeriod::Min1));
    m_periodCombo->addItem(QStringLiteral("5分钟"), static_cast<int>(StockKLinePeriod::Min5));
    m_periodCombo->addItem(QStringLiteral("15分钟"), static_cast<int>(StockKLinePeriod::Min15));
    m_periodCombo->addItem(QStringLiteral("30分钟"), static_cast<int>(StockKLinePeriod::Min30));
    m_periodCombo->addItem(QStringLiteral("60分钟"), static_cast<int>(StockKLinePeriod::Min60));
    m_periodCombo->addItem(QStringLiteral("日线"), static_cast<int>(StockKLinePeriod::Day));
    m_periodCombo->addItem(QStringLiteral("周线"), static_cast<int>(StockKLinePeriod::Week));
    m_periodCombo->addItem(QStringLiteral("月线"), static_cast<int>(StockKLinePeriod::Month));
    m_periodCombo->setCurrentIndex(static_cast<int>(StockKLinePeriod::Day));
    toolbarLayout->addWidget(m_periodCombo);

    toolbarLayout->addSpacing(10);

    // 复权选择
    toolbarLayout->addWidget(new QLabel(QStringLiteral("复权:"), this));
    m_adjustCombo = new QComboBox(this);
    m_adjustCombo->addItem(QStringLiteral("不复权"), 0);
    m_adjustCombo->addItem(QStringLiteral("前复权"), 1);
    m_adjustCombo->addItem(QStringLiteral("后复权"), 2);
    toolbarLayout->addWidget(m_adjustCombo);

    toolbarLayout->addSpacing(10);

    // 刷新按钮
    m_refreshBtn = new QPushButton(QStringLiteral("刷新"), this);
    m_refreshBtn->setFixedWidth(80);
    toolbarLayout->addWidget(m_refreshBtn);

    toolbarLayout->addStretch();

    mainLayout->addLayout(toolbarLayout);

    // K线图区域（占位）
    d->chartWidget = new QWidget(this);
    d->chartWidget->setStyleSheet("background-color: #0F1419; border: 1px solid #2D3748; border-radius: 4px;");
    mainLayout->addWidget(d->chartWidget, 1);

    // 底部信息栏
    d->infoLabel = new QLabel(QStringLiteral("等待数据..."), this);
    mainLayout->addWidget(d->infoLabel);

    // 设置样式
    setStyleSheet(R"(
        QLabel {
            color: #A0AEC0;
        }
        QComboBox {
            background-color: #1A1F2E;
            color: #E2E8F0;
            border: 1px solid #2D3748;
            border-radius: 4px;
            padding: 4px 8px;
            min-width: 80px;
        }
        QComboBox::drop-down {
            border: none;
        }
        QComboBox::down-arrow {
            image: none;
            border-left: 4px solid transparent;
            border-right: 4px solid transparent;
            border-top: 6px solid #A0AEC0;
            margin-right: 6px;
        }
        QPushButton {
            background-color: #3B82F6;
            color: white;
            border: none;
            border-radius: 4px;
            padding: 6px 12px;
        }
        QPushButton:hover {
            background-color: #2563EB;
        }
    )");
}

void StockKLinePage::setupConnections()
{
    connect(m_periodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &StockKLinePage::onPeriodChanged);
    connect(m_adjustCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &StockKLinePage::onAdjustChanged);
    connect(m_refreshBtn, &QPushButton::clicked, this, &StockKLinePage::onRefresh);
}

void StockKLinePage::onPeriodChanged(int index)
{
    m_period = static_cast<StockKLinePeriod>(index);
    emit periodChanged(index);
    LOG_DEBUG(QString("Period changed: %1").arg(index));
}

void StockKLinePage::onAdjustChanged(int index)
{
    Q_UNUSED(index);
    LOG_DEBUG(QString("Adjust changed: %1").arg(index));
}

void StockKLinePage::onRefresh()
{
    if (m_stockCode.isEmpty()) {
        LOG_WARNING("No stock selected");
        return;
    }
    
    // TODO: 实现数据刷新
    d->infoLabel->setText(QStringLiteral("刷新中..."));
    LOG_DEBUG(QString("Refresh: %1").arg(m_stockCode));
}
