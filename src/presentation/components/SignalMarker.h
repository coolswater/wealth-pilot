/**
 * @file SignalMarker.h
 * @brief 信号标记组件 - 在K线图上显示交易信号
 *
 * @details 功能：
 * - 在K线图上绘制买卖信号标记
 * - 支持多种信号类型（波浪、缠论、道氏、量价）
 * - 支持信号强度可视化
 * - 支持悬停提示
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef SIGNAL_MARKER_H
#define SIGNAL_MARKER_H

#include <QWidget>
#include <QVector>
#include <QMap>
#include <QPainter>
#include <QToolTip>
#include "domain/analysis/legacy/AnalysisTypes.h"

namespace WealthPilot {
namespace UI {

/**
 * @brief 单个信号标记数据
 */
struct SignalMarkerData {
    int barIndex = -1;              // K线索引
    double price = 0.0;             // 价格位置
    Analysis::SignalDirection direction; // 信号方向
    Analysis::SignalStrength strength;   // 信号强度
    Analysis::TheoryType theory;    // 理论来源
    QString description;            // 描述
    QDateTime time;                 // 时间

    // 绘制位置（由组件计算）
    int x = 0;
    int y = 0;
};

/**
 * @brief 信号标记样式
 */
struct SignalMarkerStyle {
    // 买入信号颜色
    QColor buyColor = QColor("#3fb950");
    QColor buyStrongColor = QColor("#3fb950");

    // 卖出信号颜色
    QColor sellColor = QColor("#f85149");
    QColor sellStrongColor = QColor("#f85149");

    // 中性信号颜色
    QColor neutralColor = QColor("#6e7681");

    // 标记大小
    int markerSize = 12;
    int markerSpacing = 3;

    // 是否显示标签
    bool showLabels = true;

    // 字体
    QFont labelFont;
};

/**
 * @brief 信号标记组件
 *
 * @details 作为K线图的叠加层，显示交易信号标记
 */
class SignalMarker : public QWidget
{
    Q_OBJECT

public:
    explicit SignalMarker(QWidget* parent = nullptr);
    ~SignalMarker() override;

    // ========== 信号管理 ==========

    /**
     * @brief 设置信号列表
     */
    void setSignals(const QVector<Analysis::UnifiedSignal>& signalList);

    /**
     * @brief 添加信号
     */
    void addSignal(const Analysis::UnifiedSignal& signal);

    /**
     * @brief 设置综合信号
     */
    void setCompositeSignal(const Analysis::CompositeSignal& signal);

    /**
     * @brief 清空信号
     */
    void clearSignals();

    /**
     * @brief 获取信号列表
     */
    QVector<SignalMarkerData> markers() const;

    // ========== 样式设置 ==========

    /**
     * @brief 设置样式
     */
    void setStyle(const SignalMarkerStyle& style);

    /**
     * @brief 获取样式
     */
    SignalMarkerStyle style() const;

    // ========== 坐标映射 ==========

    /**
     * @brief 设置坐标映射函数
     * @param timeToX 时间转X坐标
     * @param priceToY 价格转Y坐标
     */
    void setCoordinateMapping(
        std::function<int(int)> timeToX,
        std::function<int(double)> priceToY
    );

    /**
     * @brief 更新标记位置
     */
    void updatePositions();

    // ========== 显示控制 ==========

    /**
     * @brief 设置可见范围
     */
    void setVisibleRange(int startIndex, int count);

    /**
     * @brief 设置是否显示
     */
    void setVisible(bool visible);

    /**
     * @brief 设置显示的理论类型（空=显示全部）
     */
    void setFilterTheory(Analysis::TheoryType theory);

signals:
    /**
     * @brief 信号被点击
     */
    void signalClicked(const SignalMarkerData& marker);

    /**
     * @brief 信号被悬停
     */
    void signalHovered(const SignalMarkerData& marker);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    // ========== 绘制方法 ==========

    /**
     * @brief 绘制单个信号标记
     */
    void drawMarker(QPainter& painter, const SignalMarkerData& marker);

    /**
     * @brief 绘制买入信号
     */
    void drawBuySignal(QPainter& painter, const SignalMarkerData& marker);

    /**
     * @brief 绘制卖出信号
     */
    void drawSellSignal(QPainter& painter, const SignalMarkerData& marker);

    /**
     * @brief 绘制中性信号
     */
    void drawNeutralSignal(QPainter& painter, const SignalMarkerData& marker);

    /**
     * @brief 获取信号颜色
     */
    QColor getMarkerColor(const SignalMarkerData& marker) const;

    /**
     * @brief 检测鼠标悬停
     */
    SignalMarkerData* getMarkerAt(const QPoint& pos);

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace UI
} // namespace WealthPilot

#endif // SIGNAL_MARKER_H
