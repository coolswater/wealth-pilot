/**
 * @file EquityCurveWidget.h
 * @brief 资金曲线组件 - 账户资金变化可视化
 *
 * @details 实现功能：
 * - 资金曲线绘制
 * - 收益率曲线
 * - 最大回撤标注
 * - 基准对比
 * - 时间范围选择
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef EQUITYCURVEWIDGET_H
#define EQUITYCURVEWIDGET_H

#include <QWidget>
#include <QVector>
#include <QDateTime>
#include <QColor>
#include <memory>

/**
 * @brief 资金曲线数据点
 */
struct EquityPoint
{
    QDateTime date; ///< 日期
    double equity; ///< 总资产
    double cash; ///< 现金
    double marketValue; ///< 持仓市值
    double returnRate; ///< 收益率
    double benchmarkReturn; ///< 基准收益率
};

/**
 * @brief 资金曲线样式
 */
struct EquityCurveStyle
{
    QColor equityColor = QColor("#3B82F6"); ///< 资产线颜色
    QColor returnColor = QColor("#10B981"); ///< 收益率线颜色
    QColor benchmarkColor = QColor("#F59E0B"); ///< 基准线颜色
    QColor drawdownColor = QColor("#EF4444"); ///< 回撤区域颜色

    int lineWidth = 2;
    bool showGrid = true;
    bool showDrawdown = true;
    bool showBenchmark = true;
};

/**
 * @brief 资金曲线组件
 */
class EquityCurveWidget : public QWidget
{
    Q_OBJECT

public:
    explicit EquityCurveWidget(QWidget* parent = nullptr);
    ~EquityCurveWidget() override;

    // 设置数据
    void setData(const QVector<EquityPoint>& data);
    void setInitialEquity(double equity);

    // 获取数据
    QVector<EquityPoint> data() const;

    // 样式
    void setStyle(const EquityCurveStyle& style);

    // 清除
    void clear();

    // 刷新
    void refresh();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    // 绘制方法
    void drawBackground(QPainter* painter);
    void drawGrid(QPainter* painter);
    void drawEquityCurve(QPainter* painter);
    void drawReturnCurve(QPainter* painter);
    void drawBenchmarkCurve(QPainter* painter);
    void drawDrawdownArea(QPainter* painter);
    void drawLabels(QPainter* painter);
    void drawCrosshair(QPainter* painter);
    void drawInfoBox(QPainter* painter);

    // 计算方法
    void calculateLayout();
    void calculateRange();
    int dateToX(const QDateTime& date) const;
    int equityToY(double equity) const;
    int returnToY(double returnRate) const;
    double yToEquity(int y) const;
    double yToReturn(int y) const;
    int findDataIndex(int x) const;

    // 统计计算
    double calculateMaxDrawdown() const;
    double calculateTotalReturn() const;
    double calculateAnnualizedReturn() const;
    double calculateSharpeRatio() const;

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // EQUITYCURVEWIDGET_H
