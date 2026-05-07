/**
 * @file ChartStatusBar.cpp
 * @brief 图表状态栏实现
 */

#include "ChartStatusBar.h"
#include "ui/utils/PageStyleHelper.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>

using namespace Tokens;

// ========== PIMPL 实现 ==========

struct ChartStatusBar::Impl {
    // 标签
    QLabel* accountLabel = nullptr;
    QLabel* availableLabel = nullptr;
    QLabel* marginLabel = nullptr;
    QLabel* connectionLabel = nullptr;
    QLabel* coordinateLabel = nullptr;
    QLabel* timeLabel = nullptr;

    // 分隔线
    QFrame* createSeparator()
    {
        QFrame* line = new QFrame();
        line->setFrameShape(QFrame::VLine);
        line->setStyleSheet(QString("background-color: %1;").arg(Colors::Border));
        line->setFixedWidth(1);
        return line;
    }
};

// ========== 构造与析构 ==========

ChartStatusBar::ChartStatusBar(QWidget *parent)
    : QWidget(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
    setObjectName("ChartStatusBar");
    setFixedHeight(28);
}

ChartStatusBar::~ChartStatusBar() = default;

// ========== 公共接口 ==========

void ChartStatusBar::setAccountInfo(const QString& account, double available, double margin)
{
    if (d->accountLabel) {
        d->accountLabel->setText(QString("账户: %1").arg(account));
    }
    if (d->availableLabel) {
        d->availableLabel->setText(QString("可用: %1").arg(formatMoney(available)));
    }
    if (d->marginLabel) {
        d->marginLabel->setText(QString("保证金: %1").arg(formatMoney(margin)));
    }
}

void ChartStatusBar::setConnectionStatus(const QString& status, const QColor& color)
{
    if (d->connectionLabel) {
        d->connectionLabel->setText(status);
        d->connectionLabel->setStyleSheet(QString("color: %1;").arg(color.name()));
    }
}

void ChartStatusBar::setCoordinateInfo(const QString& info)
{
    if (d->coordinateLabel) {
        d->coordinateLabel->setText(info);
    }
}

void ChartStatusBar::setCrosshairInfo(const QDateTime& time, double price, qint64 volume)
{
    if (d->coordinateLabel) {
        QString info = QString("%1 | 价格: %2 | 成交量: %3")
            .arg(time.toString("yyyy-MM-dd hh:mm"))
            .arg(price, 0, 'f', 2)
            .arg(volume);
        d->coordinateLabel->setText(info);
    }
}

void ChartStatusBar::clear()
{
    if (d->accountLabel) d->accountLabel->setText("账户: --");
    if (d->availableLabel) d->availableLabel->setText("可用: --");
    if (d->marginLabel) d->marginLabel->setText("保证金: --");
    if (d->connectionLabel) d->connectionLabel->setText("未连接");
    if (d->coordinateLabel) d->coordinateLabel->setText("--");
}

// ========== 私有方法 ==========

void ChartStatusBar::setupUI()
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(Spacing::SM, Spacing::XS, Spacing::SM, Spacing::XS);
    layout->setSpacing(Spacing::SM);

    // 账户信息
    d->accountLabel = new QLabel(QStringLiteral("账户: --"), this);
    d->accountLabel->setStyleSheet(PageStyleHelper::dataLabelStyle());
    layout->addWidget(d->accountLabel);

    layout->addWidget(d->createSeparator());

    d->availableLabel = new QLabel(QStringLiteral("可用: --"), this);
    d->availableLabel->setStyleSheet(QString("color: %1;").arg(Colors::Success));
    layout->addWidget(d->availableLabel);

    layout->addWidget(d->createSeparator());

    d->marginLabel = new QLabel(QStringLiteral("保证金: --"), this);
    d->marginLabel->setStyleSheet(QString("color: %1;").arg(Colors::Warning));
    layout->addWidget(d->marginLabel);

    layout->addWidget(d->createSeparator());

    // 连接状态
    d->connectionLabel = new QLabel(QStringLiteral("未连接"), this);
    d->connectionLabel->setStyleSheet(PageStyleHelper::errorStyle());
    layout->addWidget(d->connectionLabel);

    // 弹性空间
    layout->addStretch();

    // 坐标信息
    d->coordinateLabel = new QLabel(QStringLiteral("--"), this);
    d->coordinateLabel->setStyleSheet(PageStyleHelper::dataLabelStyle());
    layout->addWidget(d->coordinateLabel);

    // 设置整体样式
    setStyleSheet(QString(
        "ChartStatusBar {"
        "  background-color: %1;"
        "  border-top: 1px solid %2;"
        "}"
        "QLabel {"
        "  font-size: %3px;"
        "}"
    )
    .arg(Colors::BgSurface)
    .arg(Colors::Border)
    .arg(Font::Size::Small));
}

QString ChartStatusBar::formatMoney(double value)
{
    if (qAbs(value) >= 100000000.0) {
        return QString::number(value / 100000000.0, 'f', 2) + "亿";
    } else if (qAbs(value) >= 10000.0) {
        return QString::number(value / 10000.0, 'f', 2) + "万";
    }
    return QString::number(value, 'f', 2);
}
