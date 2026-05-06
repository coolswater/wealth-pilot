/**
 * @file ChanLunIntegration.cpp
 * @brief 缠论指标集成实现
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "ChanLunIntegration.h"
#include "ui/components/KLineChart.h"
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QDebug>

namespace WealthPilot {
namespace ChanLun {

struct ChanLunIntegration::Impl {
    QVector<QGraphicsItem*> overlayItems;
};

ChanLunIntegration::ChanLunIntegration(KLineChart* chart, QObject* parent)
    : QObject(parent)
    , m_chart(chart)
    , d(std::make_unique<Impl>())
{
}

ChanLunIntegration::~ChanLunIntegration()
{
    // 清理图形项
    for (auto* item : d->overlayItems) {
        delete item;
    }
}

void ChanLunIntegration::setKLineData(const QVector<KLineData>& klines)
{
    m_klines = klines;
    
    if (m_enabled) {
        performAnalysis();
    }
}

void ChanLunIntegration::setEnabled(bool enabled)
{
    if (m_enabled == enabled) return;
    
    m_enabled = enabled;
    
    if (enabled && !m_klines.isEmpty()) {
        performAnalysis();
    } else {
        updateChartOverlay();
    }
}

void ChanLunIntegration::setShowPens(bool show)
{
    m_showPens = show;
    if (m_enabled) updateChartOverlay();
}

void ChanLunIntegration::setShowSegments(bool show)
{
    m_showSegments = show;
    if (m_enabled) updateChartOverlay();
}

void ChanLunIntegration::setShowPivots(bool show)
{
    m_showPivots = show;
    if (m_enabled) updateChartOverlay();
}

void ChanLunIntegration::setShowSignals(bool show)
{
    m_showSignals = show;
    if (m_enabled) updateChartOverlay();
}

void ChanLunIntegration::setShowFractals(bool show)
{
    m_showFractals = show;
    if (m_enabled) updateChartOverlay();
}

void ChanLunIntegration::performAnalysis()
{
    if (m_klines.isEmpty()) return;
    
    qDebug() << "ChanLunIntegration: Starting analysis for" << m_klines.size() << "klines";
    
    // 转换 KLineData 到 RawKLine
    QVector<RawKLine> rawKlines;
    rawKlines.reserve(m_klines.size());
    
    for (int i = 0; i < m_klines.size(); ++i) {
        const auto& k = m_klines[i];
        RawKLine raw;
        raw.time = k.time;
        raw.open = k.open;
        raw.high = k.high;
        raw.low = k.low;
        raw.close = k.close;
        raw.volume = static_cast<double>(k.volume);
        rawKlines.append(raw);
    }
    
    // 执行缠论分析
    m_result = m_analyzer.analyze(rawKlines);
    
    qDebug() << "ChanLunIntegration: Analysis completed:"
             << "pens=" << m_result.pens.size()
             << "segments=" << m_result.segments.size()
             << "pivots=" << m_result.pivots.size()
             << "signals=" << m_result.tradeSignals.size();
    
    // 更新图表叠加层
    updateChartOverlay();
    
    // 发送信号
    emit analysisCompleted(m_result);
    
    // 发送买卖点信号
    for (const auto& sig : m_result.tradeSignals) {
        emit signalDetected(sig);
    }
}

void ChanLunIntegration::updateChartOverlay()
{
    // 清理旧的图形项
    for (auto* item : d->overlayItems) {
        delete item;
    }
    d->overlayItems.clear();
    
    if (!m_enabled || !m_chart || m_result.klines.isEmpty()) {
        return;
    }
    
    // TODO: 实现图表叠加层绘制
    // 这需要 KLineChart 提供图形场景访问接口
    // 当前版本先提供数据接口，图形绘制由 KLineChart 内部实现
}

} // namespace ChanLun
} // namespace WealthPilot
