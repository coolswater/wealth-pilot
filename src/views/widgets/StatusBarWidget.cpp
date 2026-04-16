#include "StatusBarWidget.h"
#include "core/Tokens.h"

#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QStatusBar>
#include <QTimer>

#include <services/CTPService.h>

#include "AIAssistantPanelWidget.h"
#include "NetworkIndicator.h"
#include "utils/Logger.h"


struct StatusBarWidget::Impl {
    // UI组件
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
    setFixedHeight(30);
    setupUI();
    initConnections();
}

StatusBarWidget::~StatusBarWidget() = default;

void StatusBarWidget::setupUI()
{
    d->layout = new QHBoxLayout(this);
    d->layout->setContentsMargins(10, 0, 10, 0);
    d->layout->addSpacing(2);

    // 版本
    d->versionLabel = new QLabel("@copyright: v1.0.0", this);
    d->versionLabel->setObjectName("versionLabel");
    d->layout->addWidget(d->versionLabel);

    d->layout->addStretch(1);

    // AI 状态
    d->aiStatusLabel = new QLabel("AI: 就绪", this);
    d->aiStatusLabel->setObjectName("aiStatus");
    d->layout->addWidget(d->aiStatusLabel);

    // CTP 状态
    d->ctpStatusLabel = new QLabel("CTP: 未连接", this);
    d->ctpStatusLabel->setObjectName("ctpStatus");
    d->layout->addWidget(d->ctpStatusLabel);

    // 网络指示器
    d->networkIndicator = new NetworkIndicator(this
        );
    d->networkIndicator->setCheckInterval(3000);  // 3秒检测一次
    d->networkIndicator->startMonitoring();
    d->networkIndicator->setIndicatorSize(20, 13);  // 更小 20x14

    // 自定义颜色
    d->networkIndicator->setColorScheme(NetworkIndicator::ExcellentSignal, QColor(0, 255, 0));
    d->layout->addWidget(d->networkIndicator);

    // 添加网络延迟标签
    d->latencyLabel = new QLabel("检测中...");
    d->latencyLabel->setStyleSheet("QLabel { color: #888; font-size: 12px; }");
    d->layout->addWidget(d->latencyLabel);

    // 时间显示
    d->timeLabel = new QLabel(this);
    d->timeLabel->setObjectName("timeLabel");
    d->layout->addWidget(d->timeLabel);

    d->timer = new QTimer(this);
    // 立即显示
    d->timeLabel->setText(QDateTime::currentDateTime().toString("MM/dd HH:mm:ss"));
    // 启动定时器，每秒更新一次
    d->timer->start(1000);
}

void StatusBarWidget::initConnections()
{
    connect(d->timer, &QTimer::timeout, this, [this]() {
        QDateTime currentTime = QDateTime::currentDateTime();
        d->timeLabel->setText(currentTime.toString("MM/dd HH:mm:ss"));
    });
    // // CTP 状态
    // connect(CTPService::instance(), &CTPService::marketConnected,
    //         this, &StatusBarWidget::onCTPStatusChanged);
    // connect(CTPService::instance(), &CTPService::marketDisconnected,
    //         this, &StatusBarWidget::onCTPStatusChanged);

    // AI 面板信号
    connect(d->aiPanel, &AIAssistantPanelWidget::messageSent,
            this, [](const QString& message) {
                LOG_DEBUG(QString("AI message: %1").arg(message));
    });

    // 监听信号
    connect(d->networkIndicator, &NetworkIndicator::latencyChanged,
        [&](int ms) {
            d->latencyLabel->setText(QString("延迟: %1ms").arg(ms));
            d->latencyLabel->setStyleSheet(
                ms < 100 ? "QLabel { color: #00ff88; font-size: 12px; font-weight: bold; }" :
                ms < 300 ? "QLabel { color: #ffcc00; font-size: 12px; }" :
                          "QLabel { color: #ff3b30; font-size: 12px; font-weight: bold; }"
            );
        });

    connect(d->networkIndicator, &NetworkIndicator::connectionLost, this, [this]() {
        d->latencyLabel->setText("连接断开");
        d->latencyLabel->setStyleSheet("QLabel { color: #ff3b30; font-size: 12px; font-weight: bold; }");
    });
}

void StatusBarWidget::onCTPStatusChanged() const
{
    if (!d->ctpStatusLabel) return;

    // bool marketConnected = CTPService::instance()->isMarketConnected();
    // bool tradeConnected = CTPService::instance()->isTradeConnected();

    // if (marketConnected && tradeConnected) {
    //     d->ctpStatusLabel->setText("CTP: 已连接");
    //     d->ctpStatusLabel->setStyleSheet(QString("color: %1;").arg(Tokens::Colors::Success));
    // } else if (marketConnected) {
    //     d->ctpStatusLabel->setText("CTP: 行情已连接");
    //     d->ctpStatusLabel->setStyleSheet(QString("color: %1;").arg(Tokens::Colors::Warning));
    // } else {
    //     d->ctpStatusLabel->setText("CTP: 未连接");
    //     d->ctpStatusLabel->setStyleSheet(QString("color: %1;").arg(Tokens::Colors::Danger));
    // }
}
