#include "NetworkIndicator.h"
#include <QPainter>
#include <QtMath>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QElapsedTimer>
#include <QUrl>

NetworkIndicator::NetworkIndicator(QWidget *parent)
    : QWidget(parent)
{
    // 透明背景，无系统边框
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_OpaquePaintEvent);

    // 默认超小尺寸 24x16（可调整）
    setFixedSize(24, 16);

    // 初始化颜色
    m_colors[NoSignal] = QColor(255, 59, 48);
    m_colors[WeakSignal] = QColor(255, 149, 0);
    m_colors[FairSignal] = QColor(255, 204, 0);
    m_colors[GoodSignal] = QColor(52, 199, 89);
    m_colors[ExcellentSignal] = QColor(0, 255, 136);

    m_levelAnimation = new QPropertyAnimation(this, "signalLevel", this);
    m_levelAnimation->setDuration(m_animationDuration);
    m_levelAnimation->setEasingCurve(QEasingCurve::OutCubic);

    m_networkTimer = new QTimer(this);
    m_networkTimer->setTimerType(Qt::PreciseTimer);
    connect(m_networkTimer, &QTimer::timeout, this, &NetworkIndicator::updateNetworkStatus);

    updateBarRects();
}

NetworkIndicator::~NetworkIndicator() = default;

void NetworkIndicator::setIndicatorSize(int width, int height)
{
    setFixedSize(width, height);
    updateBarRects();
}

void NetworkIndicator::setSignalLevel(qreal level)
{
    level = qBound(0.0, level, 4.0);
    m_currentLevel.store(level, std::memory_order_relaxed);
    update();
}

void NetworkIndicator::setColorScheme(SignalStrength strength, const QColor &color)
{
    if (strength >= NoSignal && strength <= ExcellentSignal) {
        m_colors[strength] = color;
        update();
    }
}

void NetworkIndicator::setCheckInterval(int ms)
{
    m_checkInterval = ms;
    if (m_networkTimer->isActive()) {
        m_networkTimer->setInterval(ms);
    }
}

void NetworkIndicator::startMonitoring()
{
    if (!m_networkTimer->isActive()) {
        updateNetworkStatus();
        m_networkTimer->start(m_checkInterval);
    }
}

void NetworkIndicator::stopMonitoring()
{
    m_networkTimer->stop();
}

void NetworkIndicator::forceRefresh()
{
    updateNetworkStatus();
}

void NetworkIndicator::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    // 抗锯齿开启，使细条更平滑
    painter.setRenderHint(QPainter::Antialiasing);

    // 只绘制信号条，无背景、无边框、无留白
    drawSignalBars(painter);
}

void NetworkIndicator::drawSignalBars(QPainter &painter)
{
    const qreal level = m_currentLevel.load(std::memory_order_relaxed);
    const QColor activeColor = calculateColor(level);

    for (int i = 0; i < BAR_COUNT; ++i) {
        qreal barFill = qBound(0.0, level - i, 1.0);

        if (barFill > 0.01) {
            // 实心填充，根据填充程度调整透明度
            QColor c = activeColor;
            c.setAlphaF(0.3 + 0.7 * barFill);
            painter.fillRect(m_barRects[i], c);
        }
        // 未激活时不绘制（完全透明，无背景条）
    }
}

QColor NetworkIndicator::calculateColor(qreal level) const
{
    if (level <= 0.5) return m_colors[NoSignal];
    if (level >= 3.5) return m_colors[ExcellentSignal];

    int idx = static_cast<int>(level);
    return m_colors[qBound(0, idx, 4)];
}

void NetworkIndicator::updateBarRects()
{
    int w = width();
    int h = height();

    // 无留白，撑满整个控件
    int totalBarWidth = BAR_COUNT * m_barWidth + (BAR_COUNT - 1) * m_barSpacing;
    int startX = (w - totalBarWidth) / 2;

    // 底部对齐，高度递增（4个条高度比例 1:2:3:4）
    int heights[4] = {
        h / 4,
        h * 2 / 4,
        h * 3 / 4,
        h
    };

    for (int i = 0; i < BAR_COUNT; ++i) {
        int x = startX + i * (m_barWidth + m_barSpacing);
        int barH = heights[i];
        int y = h - barH;
        m_barRects[i] = QRectF(x, y, m_barWidth, barH);
    }
}

void NetworkIndicator::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateBarRects();
}

void NetworkIndicator::updateNetworkStatus()
{
    static QNetworkAccessManager *manager = nullptr;
    if (!manager) {
        manager = new QNetworkAccessManager(this);
    }

    QElapsedTimer timer;
    timer.start();

    QNetworkRequest request(QUrl(QStringLiteral("http://www.baidu.com")));
    request.setTransferTimeout(5000);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply *reply = manager->head(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, timer]() {
        reply->deleteLater();

        int latency = static_cast<int>(timer.elapsed());
        bool success = (reply->error() == QNetworkReply::NoError);

        m_currentLatency.store(latency);

        if (success) {
            if (!m_isConnected.exchange(true)) {
                emit connectionRestored();
            }

            qreal targetLevel = 0.0;
            if (latency < 50) targetLevel = 4.0;
            else if (latency < 150) targetLevel = 3.0;
            else if (latency < 300) targetLevel = 2.0;
            else targetLevel = 1.0;

            m_levelAnimation->stop();
            m_levelAnimation->setStartValue(m_currentLevel.load());
            m_levelAnimation->setEndValue(targetLevel);
            m_levelAnimation->start();

            emit latencyChanged(latency);
        } else {
            if (m_isConnected.exchange(false)) {
                emit connectionLost();
            }
            m_levelAnimation->stop();
            m_levelAnimation->setStartValue(m_currentLevel.load());
            m_levelAnimation->setEndValue(0.0);
            m_levelAnimation->start();
        }
    });
}