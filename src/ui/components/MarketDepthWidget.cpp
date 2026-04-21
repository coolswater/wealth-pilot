/**
 * @file MarketDepthWidget.cpp
 * @brief 盘口信息组件实现
 */

#include "MarketDepthWidget.h"
#include "core/config/Tokens.h"
#include "plugins/ICTPPlugin.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QFrame>
#include <QMouseEvent>

// ========== PIMPL 实现 ==========

struct MarketDepthWidget::Impl {
    // 合约信息
    QString instrumentId;
    QString instrumentName;

    // 头部标签
    QLabel* instrumentLabel = nullptr;
    QLabel* priceLabel = nullptr;
    QLabel* changeLabel = nullptr;
    QLabel* changePercentLabel = nullptr;

    // 买卖盘标签（一档）
    QLabel* bidPriceLabel = nullptr;
    QLabel* bidVolumeLabel = nullptr;
    QLabel* askPriceLabel = nullptr;
    QLabel* askVolumeLabel = nullptr;

    // 统计标签
    QLabel* openLabel = nullptr;
    QLabel* highLabel = nullptr;
    QLabel* lowLabel = nullptr;
    QLabel* volumeLabel = nullptr;
    QLabel* openInterestLabel = nullptr;

    // 昨收价（用于计算涨跌）
    double preClosePrice = 0.0;

    // 颜色定义
    QColor upColor{"#EF4444"};      // 上涨红色
    QColor downColor{"#10B981"};    // 下跌绿色
    QColor flatColor{"#9CA3AF"};    // 平盘灰色
};

// ========== 构造与析构 ==========

MarketDepthWidget::MarketDepthWidget(QWidget *parent)
    : QWidget(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
    setObjectName("MarketDepthWidget");
}

MarketDepthWidget::~MarketDepthWidget() = default;

// ========== 公共接口 ==========

void MarketDepthWidget::setInstrument(const QString& instrumentId, const QString& instrumentName)
{
    d->instrumentId = instrumentId;
    d->instrumentName = instrumentName;

    if (d->instrumentLabel) {
        QString display = instrumentName.isEmpty() ? instrumentId :
                         QString("%1 (%2)").arg(instrumentName, instrumentId);
        d->instrumentLabel->setText(display);
    }
}

void MarketDepthWidget::updateQuote(const MarketData& quote)
{
    d->preClosePrice = quote.preSettlementPrice;

    // 更新最新价
    if (d->priceLabel) {
        d->priceLabel->setText(formatPrice(quote.lastPrice));
        d->priceLabel->setStyleSheet(QString("font-size: 24px; font-weight: bold; color: %1;")
            .arg(quote.lastPrice >= quote.preSettlementPrice ? d->upColor.name() : d->downColor.name()));
    }

    // 更新涨跌
    if (d->changeLabel && d->changePercentLabel) {
        double change = quote.lastPrice - quote.preSettlementPrice;
        double changePercent = quote.preSettlementPrice > 0 ?
            (change / quote.preSettlementPrice * 100) : 0;

        d->changeLabel->setText(formatChange(change, quote.preSettlementPrice));
        d->changePercentLabel->setText(QString("%1%").arg(changePercent, 0, 'f', 2));

        QColor color = change > 0 ? d->upColor : (change < 0 ? d->downColor : d->flatColor);
        d->changeLabel->setStyleSheet(QString("color: %1;").arg(color.name()));
        d->changePercentLabel->setStyleSheet(QString("color: %1;").arg(color.name()));
    }

    // 更新买一
    if (d->bidPriceLabel) {
        d->bidPriceLabel->setText(formatPrice(quote.bidPrice1));
        d->bidPriceLabel->setStyleSheet(QString("color: %1;").arg(d->downColor.name()));
    }
    if (d->bidVolumeLabel) {
        d->bidVolumeLabel->setText(formatVolume(quote.bidVolume1));
    }

    // 更新卖一
    if (d->askPriceLabel) {
        d->askPriceLabel->setText(formatPrice(quote.askPrice1));
        d->askPriceLabel->setStyleSheet(QString("color: %1;").arg(d->upColor.name()));
    }
    if (d->askVolumeLabel) {
        d->askVolumeLabel->setText(formatVolume(quote.askVolume1));
    }

    // 更新统计信息
    if (d->openLabel) d->openLabel->setText(formatPrice(quote.openPrice));
    if (d->highLabel) d->highLabel->setText(formatPrice(quote.highestPrice));
    if (d->lowLabel) d->lowLabel->setText(formatPrice(quote.lowestPrice));
    if (d->volumeLabel) d->volumeLabel->setText(formatVolume(quote.volume));
    if (d->openInterestLabel) d->openInterestLabel->setText(formatVolume(quote.openInterest));
}

void MarketDepthWidget::clear()
{
    if (d->priceLabel) d->priceLabel->setText("--");
    if (d->changeLabel) d->changeLabel->setText("--");
    if (d->changePercentLabel) d->changePercentLabel->setText("--");
    if (d->bidPriceLabel) d->bidPriceLabel->setText("--");
    if (d->bidVolumeLabel) d->bidVolumeLabel->setText("--");
    if (d->askPriceLabel) d->askPriceLabel->setText("--");
    if (d->askVolumeLabel) d->askVolumeLabel->setText("--");
}

QString MarketDepthWidget::instrumentId() const
{
    return d->instrumentId;
}

// ========== 事件处理 ==========

void MarketDepthWidget::mousePressEvent(QMouseEvent *event)
{
    QWidget::mousePressEvent(event);

    // 检查点击的是哪个价格标签
    QLabel* label = qobject_cast<QLabel*>(childAt(event->pos()));
    if (!label) return;

    // 检查是否是买盘价格
    if (label == d->bidPriceLabel) {
        bool ok = false;
        double price = label->text().toDouble(&ok);
        if (ok && price > 0) {
            emit buyClicked(price);
        }
        return;
    }
    
    if (label == d->askPriceLabel) {
        bool ok = false;
        double price = label->text().toDouble(&ok);
        if (ok && price > 0) {
            emit sellClicked(price);
        }
        return;
    }
}

// ========== 私有方法 ==========

void MarketDepthWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // 头部
    mainLayout->addLayout(createHeaderLayout());

    // 分隔线
    QFrame* line1 = new QFrame(this);
    line1->setFrameShape(QFrame::HLine);
    line1->setStyleSheet("background-color: #374151;");
    mainLayout->addWidget(line1);

    // 价格面板
    mainLayout->addLayout(createPriceLayout());

    // 分隔线
    QFrame* line2 = new QFrame(this);
    line2->setFrameShape(QFrame::HLine);
    line2->setStyleSheet("background-color: #374151;");
    mainLayout->addWidget(line2);

    // 买卖盘口
    QHBoxLayout* depthLayout = new QHBoxLayout();
    
    // 买盘
    QVBoxLayout* bidLayout = new QVBoxLayout();
    QLabel* bidTitle = new QLabel(QStringLiteral("买一"), this);
    bidTitle->setStyleSheet("color: #10B981; font-weight: bold;");
    bidLayout->addWidget(bidTitle);
    d->bidPriceLabel = new QLabel("--", this);
    d->bidPriceLabel->setStyleSheet("color: #10B981; font-size: 16px;");
    bidLayout->addWidget(d->bidPriceLabel);
    d->bidVolumeLabel = new QLabel("--", this);
    d->bidVolumeLabel->setStyleSheet("color: #9CA3AF;");
    bidLayout->addWidget(d->bidVolumeLabel);
    depthLayout->addLayout(bidLayout);
    
    depthLayout->addStretch();
    
    // 卖盘
    QVBoxLayout* askLayout = new QVBoxLayout();
    QLabel* askTitle = new QLabel(QStringLiteral("卖一"), this);
    askTitle->setStyleSheet("color: #EF4444; font-weight: bold;");
    askLayout->addWidget(askTitle);
    d->askPriceLabel = new QLabel("--", this);
    d->askPriceLabel->setStyleSheet("color: #EF4444; font-size: 16px;");
    askLayout->addWidget(d->askPriceLabel);
    d->askVolumeLabel = new QLabel("--", this);
    d->askVolumeLabel->setStyleSheet("color: #9CA3AF;");
    askLayout->addWidget(d->askVolumeLabel);
    depthLayout->addLayout(askLayout);
    
    mainLayout->addLayout(depthLayout);

    // 分隔线
    QFrame* line3 = new QFrame(this);
    line3->setFrameShape(QFrame::HLine);
    line3->setStyleSheet("background-color: #374151;");
    mainLayout->addWidget(line3);

    // 统计面板
    mainLayout->addLayout(createStatisticsLayout());

    mainLayout->addStretch();
    
    // 设置整体样式
    setStyleSheet(R"(
        MarketDepthWidget {
            background-color: #1F2937;
        }
    )");
}

QLayout* MarketDepthWidget::createHeaderLayout()
{
    QHBoxLayout* layout = new QHBoxLayout();

    d->instrumentLabel = new QLabel(this);
    d->instrumentLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #F3F4F6;");
    layout->addWidget(d->instrumentLabel);

    layout->addStretch();

    return layout;
}

QLayout* MarketDepthWidget::createPriceLayout()
{
    QVBoxLayout* layout = new QVBoxLayout();

    // 最新价行
    QHBoxLayout* priceRow = new QHBoxLayout();
    d->priceLabel = new QLabel("--", this);
    d->priceLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #F3F4F6;");
    priceRow->addWidget(d->priceLabel);
    priceRow->addStretch();
    layout->addLayout(priceRow);

    // 涨跌行
    QHBoxLayout* changeRow = new QHBoxLayout();
    QLabel* changeTitle = new QLabel(QStringLiteral("涨跌"), this);
    changeTitle->setStyleSheet("color: #9CA3AF;");
    changeRow->addWidget(changeTitle);
    d->changeLabel = new QLabel("--", this);
    changeRow->addWidget(d->changeLabel);
    d->changePercentLabel = new QLabel("--", this);
    changeRow->addWidget(d->changePercentLabel);
    changeRow->addStretch();
    layout->addLayout(changeRow);

    return layout;
}

QLayout* MarketDepthWidget::createStatisticsLayout()
{
    QGridLayout* layout = new QGridLayout();
    layout->setSpacing(8);

    // 第一行
    QLabel* openTitle = new QLabel(QStringLiteral("开盘"), this);
    openTitle->setStyleSheet("color: #9CA3AF;");
    layout->addWidget(openTitle, 0, 0);
    d->openLabel = new QLabel("--", this);
    d->openLabel->setStyleSheet("color: #F3F4F6;");
    layout->addWidget(d->openLabel, 0, 1);

    QLabel* highTitle = new QLabel(QStringLiteral("最高"), this);
    highTitle->setStyleSheet("color: #9CA3AF;");
    layout->addWidget(highTitle, 0, 2);
    d->highLabel = new QLabel("--", this);
    d->highLabel->setStyleSheet("color: #EF4444;");
    layout->addWidget(d->highLabel, 0, 3);

    // 第二行
    QLabel* lowTitle = new QLabel(QStringLiteral("最低"), this);
    lowTitle->setStyleSheet("color: #9CA3AF;");
    layout->addWidget(lowTitle, 1, 0);
    d->lowLabel = new QLabel("--", this);
    d->lowLabel->setStyleSheet("color: #10B981;");
    layout->addWidget(d->lowLabel, 1, 1);

    QLabel* volTitle = new QLabel(QStringLiteral("成交量"), this);
    volTitle->setStyleSheet("color: #9CA3AF;");
    layout->addWidget(volTitle, 1, 2);
    d->volumeLabel = new QLabel("--", this);
    d->volumeLabel->setStyleSheet("color: #F3F4F6;");
    layout->addWidget(d->volumeLabel, 1, 3);

    // 第三行
    QLabel* oiTitle = new QLabel(QStringLiteral("持仓量"), this);
    oiTitle->setStyleSheet("color: #9CA3AF;");
    layout->addWidget(oiTitle, 2, 0);
    d->openInterestLabel = new QLabel("--", this);
    d->openInterestLabel->setStyleSheet("color: #F3F4F6;");
    layout->addWidget(d->openInterestLabel, 2, 1);

    return layout;
}

void MarketDepthWidget::updatePriceColor(QLabel* label, double change)
{
    QColor color = change > 0 ? d->upColor : (change < 0 ? d->downColor : d->flatColor);
    label->setStyleSheet(QString("color: %1;").arg(color.name()));
}

QString MarketDepthWidget::formatPrice(double price, int precision)
{
    if (price <= 0) return "--";
    return QString::number(price, 'f', precision);
}

QString MarketDepthWidget::formatVolume(qint64 volume)
{
    if (volume <= 0) return "--";

    if (volume >= 100000000) {
        return QString("%1亿").arg(volume / 100000000.0, 0, 'f', 2);
    } else if (volume >= 10000) {
        return QString("%1万").arg(volume / 10000.0, 0, 'f', 2);
    }
    return QString::number(volume);
}

QString MarketDepthWidget::formatChange(double change, double base)
{
    Q_UNUSED(base);
    if (qAbs(change) < 0.0001) return "0.00";
    QString sign = change > 0 ? "+" : "";
    return sign + QString::number(change, 'f', 2);
}
