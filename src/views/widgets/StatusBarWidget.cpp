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
