#include "StatusBarWidget.h"
#include "core/Tokens.h"

#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QStatusBar>

#include <services/CTPService.h>


struct StatusBarWidget::Impl {
    // UI组件
    QHBoxLayout * layout = nullptr;
    QLabel * aiStatusLabel = nullptr;
    QLabel * ctpStatusLabel = nullptr;
    QLabel * versionLabel = nullptr;
    QLabel * timeLabel = nullptr;

    QTimer *timer = nullptr;
};

StatusBarWidget::StatusBarWidget(QWidget *parent)
    : BaseWidget(parent)
    , d(std::make_unique<Impl>())
{
    setFixedHeight(30);
    setupUI();
    initConnections();
}

StatusBarWidget::~StatusBarWidget() = default;

void StatusBarWidget::setupUI()
{
    d->layout = new QHBoxLayout(this);
    d->layout->setContentsMargins(10, 0, 10, 0);
    d->layout->addSpacing(5);

    // 版本
    d->versionLabel = new QLabel("@版本: v1.0.0", this);
    d->versionLabel->setStyleSheet(QString("color: %1;").arg(Tokens::Colors::TextTertiary));
    d->layout->addWidget(d->versionLabel);

    d->layout->addStretch(1);

    // AI 状态
    d->aiStatusLabel = new QLabel("AI: 就绪", this);
    d->aiStatusLabel->setStyleSheet(QString("color: %1;").arg(Tokens::Colors::Success));
    d->layout->addWidget(d->aiStatusLabel);

    // CTP 状态
    d->ctpStatusLabel = new QLabel("CTP: 未连接", this);
    d->ctpStatusLabel->setStyleSheet(QString("color: %1;").arg(Tokens::Colors::Danger));
    d->layout->addWidget(d->ctpStatusLabel);



    // 时间显示
    d->timeLabel = new QLabel(this);
    d->timeLabel->setStyleSheet(QString("color: %1;").arg(Tokens::Colors::TextSecondary));
    d->layout->addWidget(d->timeLabel);

    d->timer = new QTimer(this);
    // 立即显示
    d->timeLabel->setText(QDateTime::currentDateTime().toString("yyyy/MM/dd HH:mm:ss"));
    // 启动定时器，每秒更新一次
    d->timer->start(1000);
}

void StatusBarWidget::initConnections()
{
    connect(d->timer, &QTimer::timeout, this, [this]() {
        QDateTime currentTime = QDateTime::currentDateTime();
        d->timeLabel->setText(currentTime.toString("yyyy/MM/dd HH:mm:ss"));
    });
    // CTP 状态
    connect(CTPService::instance(), &CTPService::marketConnected,
            this, &StatusBarWidget::onCTPStatusChanged);
    connect(CTPService::instance(), &CTPService::marketDisconnected,
            this, &StatusBarWidget::onCTPStatusChanged);

    // AI 面板信号
    // connect(d->aiPanel, &AIAssistantPanel::messageSent,
    //         this, [](const QString& message) {
    //             LOG_DEBUG(QString("AI message: %1").arg(message));
    //         });

}

void StatusBarWidget::onCTPStatusChanged()
{
    if (!d->ctpStatusLabel) return;

    bool marketConnected = CTPService::instance()->isMarketConnected();
    bool tradeConnected = CTPService::instance()->isTradeConnected();

    if (marketConnected && tradeConnected) {
        d->ctpStatusLabel->setText("CTP: 已连接");
        d->ctpStatusLabel->setStyleSheet(QString("color: %1;").arg(Tokens::Colors::Success));
    } else if (marketConnected) {
        d->ctpStatusLabel->setText("CTP: 行情已连接");
        d->ctpStatusLabel->setStyleSheet(QString("color: %1;").arg(Tokens::Colors::Warning));
    } else {
        d->ctpStatusLabel->setText("CTP: 未连接");
        d->ctpStatusLabel->setStyleSheet(QString("color: %1;").arg(Tokens::Colors::Danger));
    }
}
