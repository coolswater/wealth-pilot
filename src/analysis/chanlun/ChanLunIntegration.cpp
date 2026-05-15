/**
 * @file ChanLunIntegration.cpp
 * @brief 缠论指标集成实现
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "ChanLunIntegration.h"
#include "ui/components/KLineChart.h"
#include "utils/Logger.h"
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

    LOG_DEBUG(QString("ChanLunIntegration: Starting analysis for %1 klines").arg(m_klines.size()));

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

    LOG_DEBUG(QString("ChanLunIntegration: Analysis completed: pens=%1 segments=%2 pivots=%3 signals=%4")
             .arg(m_result.pens.size())
             .arg(m_result.segments.size())
             .arg(m_result.pivots.size())
             .arg(m_result.tradeSignals.size()));
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
    
    // 实现图表叠加层绘制
    // 使用 KLineChart 的 addIndicator 方法添加缠论指标
    
    // 1. 绘制笔标记
    if (!m_result.pens.isEmpty()) {
        QVector<double> penMarks;
        penMarks.resize(m_result.klines.size());
        
        for (const auto& pen : m_result.pens) {
            if (pen.startKLineIndex >= 0 && pen.startKLineIndex < penMarks.size()) {
                penMarks[pen.startKLineIndex] = pen.isUp() ? 1.0 : -1.0;
            }
            if (pen.endKLineIndex >= 0 && pen.endKLineIndex < penMarks.size()) {
                penMarks[pen.endKLineIndex] = pen.isUp() ? -1.0 : 1.0;
            }
        }
        
        m_chart->addIndicator(QStringLiteral("ChanLun_Pens"), penMarks, QColor(255, 165, 0));
    }
    
    // 2. 绘制买卖点标记
    if (!m_result.tradeSignals.isEmpty()) {
        QVector<double> buyPoints, sellPoints;
        buyPoints.resize(m_result.klines.size());
        sellPoints.resize(m_result.klines.size());
        
        for (const auto& signal : m_result.tradeSignals) {
            if (signal.index >= 0 && signal.index < m_result.klines.size()) {
                if (signal.isBuy()) {
                    buyPoints[signal.index] = m_result.klines[signal.index].low;
                } else {
                    sellPoints[signal.index] = m_result.klines[signal.index].high;
                }
            }
        }
        
        m_chart->addIndicator(QStringLiteral("ChanLun_Buy"), buyPoints, QColor(0, 255, 0));
        m_chart->addIndicator(QStringLiteral("ChanLun_Sell"), sellPoints, QColor(255, 0, 0));
    }
    
    // 3. 绘制中枢区间（使用布林带方式显示）
    if (!m_result.pivots.isEmpty()) {
        QVector<double> upperLine, lowerLine, middleLine;
        upperLine.resize(m_result.klines.size());
        lowerLine.resize(m_result.klines.size());
        middleLine.resize(m_result.klines.size());
        
        for (const auto& pivot : m_result.pivots) {
            double midPrice = pivot.middle();
            for (int i = pivot.startKLineIndex; i <= pivot.endKLineIndex && i < m_result.klines.size(); ++i) {
                upperLine[i] = pivot.zd;
                lowerLine[i] = pivot.zg;
                middleLine[i] = midPrice;
            }
        }
        
        m_chart->addIndicator(QStringLiteral("ChanLun_PivotUpper"), upperLine, QColor(100, 100, 255, 150));
        m_chart->addIndicator(QStringLiteral("ChanLun_PivotLower"), lowerLine, QColor(100, 100, 255, 150));
        m_chart->addIndicator(QStringLiteral("ChanLun_PivotMid"), middleLine, QColor(150, 150, 255, 100));
    }
    
    LOG_INFO(QString("ChanLun overlay updated: %1 pens, %2 signals, %3 pivots")
        .arg(m_result.pens.size())
        .arg(m_result.tradeSignals.size())
        .arg(m_result.pivots.size()));
}

} // namespace ChanLun
} // namespace WealthPilot
