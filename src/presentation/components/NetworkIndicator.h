#ifndef NETWORKINDICATOR_H
#define NETWORKINDICATOR_H

#include <QWidget>
#include <QPropertyAnimation>
#include <QTimer>
#include <atomic>

/**
 * @brief 极简网络信号指示器
 * @details 无边框、无留白、高性能、超紧凑设计
 */
class NetworkIndicator : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal signalLevel READ signalLevel WRITE setSignalLevel NOTIFY signalLevelChanged)

public:
    enum SignalStrength {
        NoSignal = 0,
        WeakSignal = 1,
        FairSignal = 2,
        GoodSignal = 3,
        ExcellentSignal = 4
    };
    Q_ENUM(SignalStrength)

    explicit NetworkIndicator(QWidget *parent = nullptr);
    ~NetworkIndicator();

    void setSignalLevel(qreal level);
    qreal signalLevel() const { return m_currentLevel.load(std::memory_order_relaxed); }

    void setColorScheme(SignalStrength strength, const QColor &color);
    void setAnimationDuration(int ms) { m_animationDuration = ms; }
    void setCheckInterval(int ms);

    // 设置指示器尺寸（默认24x16超小）
    void setIndicatorSize(int width, int height);

public slots:
    void startMonitoring();
    void stopMonitoring();
    void forceRefresh();

signals:
    void signalLevelChanged(qreal level);
    void latencyChanged(int ms);
    void connectionLost();
    void connectionRestored();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void drawSignalBars(QPainter &painter);
    QColor calculateColor(qreal level) const;
    void updateBarRects();
    void updateNetworkStatus();

    std::atomic<qreal> m_currentLevel{0.0};
    std::atomic<int> m_currentLatency{0};
    std::atomic<bool> m_isConnected{false};

    QPropertyAnimation *m_levelAnimation = nullptr;
    int m_animationDuration = 200;
    QTimer *m_networkTimer = nullptr;
    int m_checkInterval = 2000;

    QColor m_colors[5];
    QRectF m_barRects[4];

    // 紧凑尺寸配置
    static constexpr int BAR_COUNT = 4;
    int m_barWidth = 3;        // 默认3像素宽
    int m_barSpacing = 1;      // 默认1像素间距
};

#endif // NETWORKINDICATOR_H