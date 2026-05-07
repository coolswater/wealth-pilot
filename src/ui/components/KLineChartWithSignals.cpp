/**
 * @file KLineChartWithSignals.cpp
 * @brief 带信号标记的K线图组件实现
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "KLineChartWithSignals.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QDebug>

namespace WealthPilot {
namespace UI {

struct KLineChartWithSignals::Impl {
    KLineChart* klineChart = nullptr;
    SignalMarker* signalMarker = nullptr;
    SignalDetailPanel* detailPanel = nullptr;

    QSplitter* splitter = nullptr;

    QVector<KLineData> klineData;
    Analysis::CompositeSignal currentSignal;

    bool showSignalMarkers = true;
    bool showDetailPanel = true;
};

KLineChartWithSignals::KLineChartWithSignals(QWidget* parent)
    : QWidget(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
    setupConnections();
}

KLineChartWithSignals::~KLineChartWithSignals() = default;

void KLineChartWithSignals::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 创建分割器
    d->splitter = new QSplitter(Qt::Vertical, this);

    // 创建K线图
    d->klineChart = new KLineChart(this);

    // 创建信号标记层（叠加在K线图上）
    d->signalMarker = new SignalMarker(d->klineChart);
    d->signalMarker->setAttribute(Qt::WA_TransparentForMouseEvents, false);

    // 创建信号详情面板
    d->detailPanel = new SignalDetailPanel(this);
    d->detailPanel->setMaximumHeight(300);

    // 添加到分割器
    d->splitter->addWidget(d->klineChart);
    d->splitter->addWidget(d->detailPanel);

    // 设置分割器比例
    d->splitter->setStretchFactor(0, 7);
    d->splitter->setStretchFactor(1, 3);

    mainLayout->addWidget(d->splitter);

    // 初始隐藏详情面板
    d->detailPanel->hide();
}

void KLineChartWithSignals::setupConnections()
{
    // K线图信号
    connect(d->klineChart, &KLineChart::crosshairMoved,
            this, &KLineChartWithSignals::onKLineCrosshairMoved);

    connect(d->klineChart, &KLineChart::visibleRangeChanged,
            this, &KLineChartWithSignals::onVisibleRangeChanged);

    connect(d->klineChart, &KLineChart::klineInfoChanged,
            this, &KLineChartWithSignals::klineInfoChanged);

    // 信号标记
    connect(d->signalMarker, &SignalMarker::signalClicked,
            this, &KLineChartWithSignals::onSignalMarkerClicked);

    // 详情面板
    connect(d->detailPanel, &SignalDetailPanel::subscribeRequested,
            this, &KLineChartWithSignals::subscribeRequested);
}

void KLineChartWithSignals::setKLineData(const QVector<KLineData>& data)
{
    d->klineData = data;
    d->klineChart->setData(data);

    // 更新信号标记的坐标映射
    updateSignalPositions();
}

void KLineChartWithSignals::addKLineData(const KLineData& data)
{
    d->klineData.append(data);
    d->klineChart->addData(data);
}

void KLineChartWithSignals::clearData()
{
    d->klineData.clear();
    d->klineChart->clearData();
    d->signalMarker->clearSignals();
}

void KLineChartWithSignals::setCompositeSignal(const Analysis::CompositeSignal& signal)
{
    d->currentSignal = signal;

    // 设置信号标记
    d->signalMarker->setCompositeSignal(signal);

    // 更新详情面板
    if (d->showDetailPanel) {
        d->detailPanel->setCompositeSignal(signal);
    }

    // 更新信号位置
    updateSignalPositions();
}

void KLineChartWithSignals::setSignals(const QVector<Analysis::UnifiedSignal>& signals)
{
    d->signalMarker->setSignals(signals);
    updateSignalPositions();
}

void KLineChartWithSignals::clearSignals()
{
    d->signalMarker->clearSignals();
    d->detailPanel->clear();
}

void KLineChartWithSignals::setShowSignalMarkers(bool show)
{
    d->showSignalMarkers = show;
    d->signalMarker->setVisible(show);
}

void KLineChartWithSignals::setShowDetailPanel(bool show)
{
    d->showDetailPanel = show;

    if (show) {
        d->detailPanel->show();
    } else {
        d->detailPanel->hide();
    }
}

void KLineChartWithSignals::setSignalFilterTheory(Analysis::TheoryType theory)
{
    d->signalMarker->setFilterTheory(theory);
}

void KLineChartWithSignals::setMainIndicator(MainIndicator indicator)
{
    d->klineChart->setMainIndicator(indicator);
}

void KLineChartWithSignals::setSubIndicator(SubIndicator indicator)
{
    d->klineChart->setSubIndicator(indicator);
}

void KLineChartWithSignals::addCustomIndicator(const QString& name, const QVector<double>& values, const QColor& color)
{
    d->klineChart->addIndicator(name, values, color);
}

void KLineChartWithSignals::zoom(double factor)
{
    d->klineChart->zoom(factor);
}

void KLineChartWithSignals::resetView()
{
    d->klineChart->resetView();
}

void KLineChartWithSignals::showLatest(int count)
{
    d->klineChart->showLatest(count);
}

void KLineChartWithSignals::updateSignalPositions()
{
    // 设置坐标映射函数
    d->signalMarker->setCoordinateMapping(
        [this](int index) -> int {
            // 将K线索引转换为X坐标
            // 这里需要根据K线图的实际绘制逻辑计算
            return d->klineChart->visibleStartIndex() + index * 10; // 简化计算
        },
        [this](double price) -> int {
            // 将价格转换为Y坐标
            // 这里需要根据K线图的价格范围计算
            return static_cast<int>(d->klineChart->height() / 2); // 简化计算
        }
    );

    d->signalMarker->updatePositions();
}

void KLineChartWithSignals::onSignalMarkerClicked(const SignalMarker& marker)
{
    // 转换为UnifiedSignal
    Analysis::UnifiedSignal signal;
    signal.source = marker.theory;
    signal.direction = marker.direction;
    signal.strength = marker.strength;
    signal.time = marker.time;
    signal.price = marker.price;
    signal.description = marker.description;

    emit signalClicked(signal);

    // 显示详情面板
    if (!d->showDetailPanel) {
        setShowDetailPanel(true);
    }

    d->detailPanel->setUnifiedSignal(signal);
}

void KLineChartWithSignals::onKLineCrosshairMoved(const QDateTime& time, double price)
{
    Q_UNUSED(time)
    Q_UNUSED(price)
    // 可以在这里更新信号标记的高亮状态
}

void KLineChartWithSignals::onVisibleRangeChanged(int startIndex, int count)
{
    // 更新信号标记的可见范围
    d->signalMarker->setVisibleRange(startIndex, count);
}

} // namespace UI
} // namespace WealthPilot
