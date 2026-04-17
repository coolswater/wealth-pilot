/**
 * @file MarketDepthWidget.cpp
 * @brief 盘口信息组件实现
 */

#include "MarketDepthWidget.h"
#include "plugins/ICTPPlugin.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QFrame>
#include <QMouseEvent>
#include <QApplication>

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

    // 买盘标签（五档）
    QLabel* bidPriceLabels[5] = {};
    QLabel* bidVolumeLabels[5] = {};

    // 卖盘标签（五档）
    QLabel* askPriceLabels[5] = {};
    QLabel* askVolumeLabels[5] = {};

    // 统计标签
    QLabel* openLabel = nullptr;
    QLabel* highLabel = nullptr;
    QLabel* lowLabel = nullptr;
    QLabel* volumeLabel = nullptr;
    QLabel* turnoverLabel = nullptr;
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

    // 更新买盘
    for (int i = 0; i < 5; ++i) {
        if (d->bidPriceLabels[i]) {
            double price = 0;
            int volume = 0;
            switch (i) {
                case 0: price = quote.bidPrice1; volume = quote.bidVolume1; break;
                case 1: price = quote.bidPrice2; volume = quote.bidVolume2; break;
                case 2: price = quote.bidPrice3; volume = quote.bidVolume3; break;
                case 3: price = quote.bidPrice4; volume = quote.bidVolume4; break;
                case 4: price = quote.bidPrice5; volume = quote.bidVolume5; break;
            }
            d->bidPriceLabels[i]->setText(formatPrice(price));
            d->bidVolumeLabels[i]->setText(formatVolume(volume));
        }
    }

    // 更新卖盘
    for (int i = 0; i < 5; ++i) {
        if (d->askPriceLabels[i]) {
            double price = 0;
            int volume = 0;
            switch (i) {
                case 0: price = quote.askPrice1; volume = quote.askVolume1; break;
                case 1: price = quote.askPrice2; volume = quote.askVolume2; break;
                case 2: price = quote.askPrice3; volume = quote.askVolume3; break;
                case 3: price = quote.askPrice4; volume = quote.askVolume4; break;
                case 4: price = quote.askPrice5; volume = quote.askVolume5; break;
            }
            d->askPriceLabels[i]->setText(formatPrice(price));
            d->askVolumeLabels[i]->setText(formatVolume(volume));
        }
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

    for (int i = 0; i < 5; ++i) {
        if (d->bidPriceLabels[i]) d->bidPriceLabels[i]->setText("--");
        if (d->bidVolumeLabels[i]) d->bidVolumeLabels[i]->setText("--");
        if (d->askPriceLabels[i]) d->askPriceLabels[i]->setText("--");
        if (d->askVolumeLabels[i]) d->askVolumeLabels[i]->setText("--");
    }
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
    for (int i = 0; i < 5; ++i) {
        if (label == d->bidPriceLabels[i]) {
            bool ok = false;
            double price = label->text().toDouble(&ok);
            if (ok && price > 0) {
                emit buyClicked(price);
            }
            return;
        }
        if (label == d->askPriceLabels[i]) {
            bool ok = false;
            double price = label->text().toDouble(&ok);
            if (ok && price > 0) {
                emit sellClicked(price);
            }
            return;
        }
    }
}

// ========== 私有方法 ==========

void MarketDepthWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // 头部
    setupHeader();
    mainLayout->addLayout(createHeaderLayout());

    // 分隔线
    QFrame* line1 = new QFrame(this);
    line1->setFrameShape(QFrame::HLine);
    line1->setStyleSheet("background-color: #374151;");
    mainLayout->addWidget(line1);

    // 价格面板
    setupPricePanel();
    mainLayout->addLayout(createPriceLayout());

    // 分隔线
    QFrame* line2 = new QFrame(this);
    line2->setFrameShape(QFrame::HLine);
    line2->setStyleSheet("background-color: #374151;");
    mainLayout->addWidget(line2);

    // 统计面板
    setupStatisticsPanel();
    mainLayout->addLayout(createStatisticsLayout());

    mainLayout->addStretch();
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

void MarketDepthWidget::setupHeader()
{
    d->instrumentLabel = new QLabel(this);
    d->instrumentLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #F3F4F6;");
}

void MarketDepthWidget::setupPricePanel()
{
    // 最新价
    d->priceLabel = new QLabel("--", this);
    d->priceLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #F3F4F6;");

    // 涨跌
    d->changeLabel = new QLabel("--", this);
    d->changePercentLabel = new QLabel("--", this);
}

QLayout* MarketDepthWidget::createPriceLayout()
{
    QVBoxLayout* layout = new QVBoxLayout();

    // 最新价行
    QHBoxLayout* priceRow = new QHBoxLayout();
    priceRow->addWidget(d->priceLabel);
    priceRow->addStretch();
    layout->addLayout(priceRow);

    // 涨跌行
    QHBoxLayout* changeRow = new QHBoxLayout();
    QLabel* changeTitle = new QLabel(QStringLiteral("涨跌"), this);
    changeTitle->setStyleSheet("color: #9CA3AF;");
    changeRow->addWidget(changeTitle);
    changeRow->addWidget(d->changeLabel);
    changeRow->addWidget(d->changePercentLabel);
    changeRow->addStretch();
    layout->addLayout(changeRow);

    return layout;
}

void MarketDepthWidget::setupDepthPanel()
{
    // 五档盘口
    for (int i = 0; i < 5; ++i) {
        d->bidPriceLabels[i] = new QLabel("--", this);
        d->bidVolumeLabels[i] = new QLabel("--", this);
        d->askPriceLabels[i] = new QLabel("--", this);
        d->askVolumeLabels[i] = new QLabel("--", this);

        d->bidPriceLabels[i]->setStyleSheet("color: #10B981;");  // 买盘绿色
        d->askPriceLabels[i]->setStyleSheet("color: #EF4444;");  // 卖盘红色
    }
}

void MarketDepthWidget::setupStatisticsPanel()
{
    d->openLabel = new QLabel("--", this);
    d->highLabel = new QLabel("--", this);
    d->lowLabel = new QLabel("--", this);
    d->volumeLabel = new QLabel("--", this);
    d->openInterestLabel = new QLabel("--", this);
}

QLayout* MarketDepthWidget::createStatisticsLayout()
{
    QGridLayout* layout = new QGridLayout();
    layout->setSpacing(8);

    // 第一行
    QLabel* openTitle = new QLabel(QStringLiteral("开盘"), this);
    openTitle->setStyleSheet("color: #9CA3AF;");
    layout->addWidget(openTitle, 0, 0);
    layout->addWidget(d->openLabel, 0, 1);

    QLabel* highTitle = new QLabel(QStringLiteral("最高"), this);
    highTitle->setStyleSheet("color: #9CA3AF;");
    layout->addWidget(highTitle, 0, 2);
    layout->addWidget(d->highLabel, 0, 3);

    // 第二行
    QLabel* lowTitle = new QLabel(QStringLiteral("最低"), this);
    lowTitle->setStyleSheet("color: #9CA3AF;");
    layout->addWidget(lowTitle, 1, 0);
    layout->addWidget(d->lowLabel, 1, 1);

    QLabel* volTitle = new QLabel(QStringLiteral("成交量"), this);
    volTitle->setStyleSheet("color: #9CA3AF;");
    layout->addWidget(volTitle, 1, 2);
    layout->addWidget(d->volumeLabel, 1, 3);

    // 第三行
    QLabel* oiTitle = new QLabel(QStringLiteral("持仓量"), this);
    oiTitle->setStyleSheet("color: #9CA3AF;");
    layout->addWidget(oiTitle, 2, 0);
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
