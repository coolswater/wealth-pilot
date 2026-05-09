/**
 * @file ChartStatusBar.cpp
 * @brief 图表状态栏实现
 */

#include "ChartStatusBar.h"
#include "ui/components/StyleHelper.h"
#include "core/config/Tokens.h"
#include "ui/components/ChartStyles.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>

using namespace Tokens;

struct ChartStatusBar::Impl {
    QLabel* accountLabel = nullptr;
    QLabel* availableLabel = nullptr;
    QLabel* marginLabel = nullptr;
    QLabel* connectionLabel = nullptr;
    QLabel* coordinateLabel = nullptr;
    QLabel* timeLabel = nullptr;
};

ChartStatusBar::ChartStatusBar(QWidget *parent)
    : QWidget(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
    setObjectName("ChartStatusBar");
    setFixedHeight(32);
}

ChartStatusBar::~ChartStatusBar() = default;

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
        // 动态颜色 - 根据连接状态设置属性
        if (color.name() == Tokens::Colors::Success)
        {
            d->connectionLabel->setProperty("status", "connected");
        }
        else if (color.name() == Tokens::Colors::Danger)
        {
            d->connectionLabel->setProperty("status", "disconnected");
        }
        else
        {
            d->connectionLabel->setProperty("status", "connecting");
        }
        StyleHelper::refreshStyle(d->connectionLabel);
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
    if (d->connectionLabel)
    {
        d->connectionLabel->setText("未连接");
        d->connectionLabel->setProperty("status", "disconnected");
        StyleHelper::refreshStyle(d->connectionLabel);
    }
    if (d->coordinateLabel) d->coordinateLabel->setText("--");
}

void ChartStatusBar::setupUI()
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(Tokens::Spacing::SM, Tokens::Spacing::XS, Tokens::Spacing::SM, Tokens::Spacing::XS);
    layout->setSpacing(Tokens::Spacing::SM);

    // 账户信息
    d->accountLabel = new QLabel(QStringLiteral("账户: --"), this);
    d->accountLabel->setObjectName("accountLabel");
    layout->addWidget(d->accountLabel);

    layout->addWidget(createSeparator());

    d->availableLabel = new QLabel(QStringLiteral("可用: --"), this);
    d->availableLabel->setObjectName("availableLabel");
    layout->addWidget(d->availableLabel);

    layout->addWidget(createSeparator());

    d->marginLabel = new QLabel(QStringLiteral("保证金: --"), this);
    d->marginLabel->setObjectName("marginLabel");
    layout->addWidget(d->marginLabel);

    layout->addWidget(createSeparator());

    // 连接状态
    d->connectionLabel = new QLabel(QStringLiteral("未连接"), this);
    d->connectionLabel->setObjectName("connectionLabel");
    d->connectionLabel->setProperty("status", "disconnected");
    layout->addWidget(d->connectionLabel);

    // 弹性空间
    layout->addStretch();

    // 坐标信息
    d->coordinateLabel = new QLabel(QStringLiteral("--"), this);
    d->coordinateLabel->setObjectName("coordinateLabel");
    layout->addWidget(d->coordinateLabel);
}

QFrame* ChartStatusBar::createSeparator()
{
    QFrame* line = new QFrame();
    line->setFrameShape(QFrame::VLine);
    line->setProperty("lineType", "vertical");
    line->setFixedWidth(1);
    return line;
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
