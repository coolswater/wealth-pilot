/**
 * @file MarketDepthWidget.cpp
 * @brief 盘口信息组件实现 - 使用属性选择器替代硬编码样式
 */

#include "MarketDepthWidget.h"
#include "core/config/Tokens.h"
#include "ui/ThemeManager.h"
#include "ui/components/StyleHelper.h"
#include "ui/utils/PageStyleHelper.h"
#include "plugins/ICTPPlugin.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QFrame>
#include <QMouseEvent>

using namespace Tokens;

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
};

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

    // 更新最新价 - 使用属性选择器
    if (d->priceLabel) {
        d->priceLabel->setText(formatPrice(quote.lastPrice));
        QString status = quote.lastPrice >= quote.preSettlementPrice ? "up" : "down";
        d->priceLabel->setProperty("status", status);
        d->priceLabel->style()->unpolish(d->priceLabel);
        d->priceLabel->style()->polish(d->priceLabel);
    }

    // 更新涨跌
    if (d->changeLabel && d->changePercentLabel) {
        double change = quote.lastPrice - quote.preSettlementPrice;
        double changePercent = quote.preSettlementPrice > 0 ?
            (change / quote.preSettlementPrice * 100) : 0;

        d->changeLabel->setText(formatChange(change, quote.preSettlementPrice));
        d->changePercentLabel->setText(QString("%1%").arg(changePercent, 0, 'f', 2));

        QString status = change > 0 ? "up" : (change < 0 ? "down" : "flat");
        d->changeLabel->setProperty("status", status);
        d->changePercentLabel->setProperty("status", status);
        d->changeLabel->style()->unpolish(d->changeLabel);
        d->changeLabel->style()->polish(d->changeLabel);
        d->changePercentLabel->style()->unpolish(d->changePercentLabel);
        d->changePercentLabel->style()->polish(d->changePercentLabel);
    }

    // 更新买一 - 使用属性选择器
    if (d->bidPriceLabel) {
        d->bidPriceLabel->setText(formatPrice(quote.bidPrice1));
        d->bidPriceLabel->setProperty("status", "down");
        d->bidPriceLabel->style()->unpolish(d->bidPriceLabel);
        d->bidPriceLabel->style()->polish(d->bidPriceLabel);
    }
    if (d->bidVolumeLabel) {
        d->bidVolumeLabel->setText(formatVolume(quote.bidVolume1));
    }

    // 更新卖一 - 使用属性选择器
    if (d->askPriceLabel) {
        d->askPriceLabel->setText(formatPrice(quote.askPrice1));
        d->askPriceLabel->setProperty("status", "up");
        d->askPriceLabel->style()->unpolish(d->askPriceLabel);
        d->askPriceLabel->style()->polish(d->askPriceLabel);
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
    mainLayout->setContentsMargins(Spacing::MD, Spacing::MD, Spacing::MD, Spacing::MD);
    mainLayout->setSpacing(Spacing::SM);

    // 头部
    mainLayout->addLayout(createHeaderLayout());

    // 分隔线 - 使用属性选择器
    QFrame* line1 = new QFrame(this);
    line1->setFrameShape(QFrame::HLine);
    line1->setProperty("lineType", "horizontal");
    mainLayout->addWidget(line1);

    // 价格面板
    mainLayout->addLayout(createPriceLayout());

    // 分隔线
    QFrame* line2 = new QFrame(this);
    line2->setFrameShape(QFrame::HLine);
    line2->setProperty("lineType", "horizontal");
    mainLayout->addWidget(line2);

    // 买卖盘口
    mainLayout->addLayout(createDepthLayout());

    // 分隔线
    QFrame* line3 = new QFrame(this);
    line3->setFrameShape(QFrame::HLine);
    line3->setProperty("lineType", "horizontal");
    mainLayout->addWidget(line3);

    // 统计面板
    mainLayout->addLayout(createStatisticsLayout());

    mainLayout->addStretch();
}

QLayout* MarketDepthWidget::createHeaderLayout()
{
    QHBoxLayout* layout = new QHBoxLayout();

    d->instrumentLabel = new QLabel(this);
    d->instrumentLabel->setObjectName("instrumentLabel");
    layout->addWidget(d->instrumentLabel);

    layout->addStretch();

    return layout;
}

QLayout* MarketDepthWidget::createPriceLayout()
{
    QVBoxLayout* layout = new QVBoxLayout();

    // 最新价行
    QHBoxLayout* priceRow = new QHBoxLayout();
    d->priceLabel = new QLabel(QStringLiteral("--"), this);
    d->priceLabel->setObjectName("priceLabel");
    priceRow->addWidget(d->priceLabel);
    priceRow->addStretch();
    layout->addLayout(priceRow);

    // 涨跌行
    QHBoxLayout* changeRow = new QHBoxLayout();
    QLabel* changeTitle = new QLabel(QStringLiteral("涨跌"), this);
    changeTitle->setProperty("dataType", "label");
    changeRow->addWidget(changeTitle);

    d->changeLabel = new QLabel("--", this);
    d->changeLabel->setProperty("dataType", "change");
    changeRow->addWidget(d->changeLabel);

    d->changePercentLabel = new QLabel("--", this);
    d->changePercentLabel->setProperty("dataType", "change");
    changeRow->addWidget(d->changePercentLabel);

    changeRow->addStretch();
    layout->addLayout(changeRow);

    return layout;
}

QLayout* MarketDepthWidget::createDepthLayout()
{
    QHBoxLayout* depthLayout = new QHBoxLayout();

    // 买盘
    QVBoxLayout* bidLayout = new QVBoxLayout();
    QLabel* bidTitle = new QLabel(QStringLiteral("买一"), this);
    bidTitle->setProperty("bidTitle", true);
    bidLayout->addWidget(bidTitle);

    d->bidPriceLabel = new QLabel(QStringLiteral("--"), this);
    d->bidPriceLabel->setObjectName("bidPriceLabel");
    bidLayout->addWidget(d->bidPriceLabel);

    d->bidVolumeLabel = new QLabel(QStringLiteral("--"), this);
    d->bidVolumeLabel->setProperty("dataType", "value");
    bidLayout->addWidget(d->bidVolumeLabel);
    depthLayout->addLayout(bidLayout);

    depthLayout->addStretch();

    // 卖盘
    QVBoxLayout* askLayout = new QVBoxLayout();
    QLabel* askTitle = new QLabel(QStringLiteral("卖一"), this);
    askTitle->setProperty("askTitle", true);
    askLayout->addWidget(askTitle);

    d->askPriceLabel = new QLabel(QStringLiteral("--"), this);
    d->askPriceLabel->setObjectName("askPriceLabel");
    askLayout->addWidget(d->askPriceLabel);

    d->askVolumeLabel = new QLabel(QStringLiteral("--"), this);
    d->askVolumeLabel->setProperty("dataType", "value");
    askLayout->addWidget(d->askVolumeLabel);
    depthLayout->addLayout(askLayout);

    return depthLayout;
}

QLayout* MarketDepthWidget::createStatisticsLayout()
{
    QGridLayout* layout = new QGridLayout();
    layout->setSpacing(Spacing::SM);

    // 第一行
    QLabel* openTitle = new QLabel(QStringLiteral("开盘"), this);
    openTitle->setProperty("dataType", "label");
    layout->addWidget(openTitle, 0, 0);

    d->openLabel = new QLabel("--", this);
    d->openLabel->setProperty("dataType", "value");
    layout->addWidget(d->openLabel, 0, 1);

    QLabel* highTitle = new QLabel(QStringLiteral("最高"), this);
    highTitle->setProperty("dataType", "label");
    layout->addWidget(highTitle, 0, 2);

    d->highLabel = new QLabel("--", this);
    d->highLabel->setProperty("dataType", "high");
    layout->addWidget(d->highLabel, 0, 3);

    // 第二行
    QLabel* lowTitle = new QLabel(QStringLiteral("最低"), this);
    lowTitle->setProperty("dataType", "label");
    layout->addWidget(lowTitle, 1, 0);

    d->lowLabel = new QLabel("--", this);
    d->lowLabel->setProperty("dataType", "low");
    layout->addWidget(d->lowLabel, 1, 1);

    QLabel* volTitle = new QLabel(QStringLiteral("成交量"), this);
    volTitle->setProperty("dataType", "label");
    layout->addWidget(volTitle, 1, 2);

    d->volumeLabel = new QLabel("--", this);
    d->volumeLabel->setProperty("dataType", "value");
    layout->addWidget(d->volumeLabel, 1, 3);

    // 第三行
    QLabel* oiTitle = new QLabel(QStringLiteral("持仓量"), this);
    oiTitle->setProperty("dataType", "label");
    layout->addWidget(oiTitle, 2, 0);

    d->openInterestLabel = new QLabel("--", this);
    d->openInterestLabel->setProperty("dataType", "value");
    layout->addWidget(d->openInterestLabel, 2, 1);

    return layout;
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
