/**
 * @file BacktestChartWidget.h
 * @brief 回测结果可视化图表组件
 *
 * @details 显示：
 * - 资金曲线（权益变化）
 * - 收益分布
 * - 交易标记点
 * - 回撤曲线
 */

#ifndef BACKTESTCHARTWIDGET_H
#define BACKTESTCHARTWIDGET_H

#include <QWidget>
#include <QVector>
#include <QDateTime>
#include <QPainter>
#include <QMap>
#include <memory>

namespace WealthPilot {

/**
 * @brief 回测数据点
 */
struct BacktestDataPoint {
    QDateTime date;             ///< 日期
    double equity = 0.0;        ///< 权益
    double cash = 0.0;          ///< 现金
    double positionValue = 0.0; ///< 持仓市值
    double returnRate = 0.0;    ///< 收益率
    double drawdown = 0.0;      ///< 回撤
};

/**
 * @brief 交易标记
 */
struct TradeMarker {
    QDateTime date;             ///< 日期
    QString symbol;             ///< 标的代码
    QString action;             ///< 动作（buy/sell）
    double price;               ///< 价格
    int quantity;               ///< 数量
    double profit;              ///< 盈亏
    bool isWin = false;         ///< 是否盈利
};

/**
 * @brief 回测可视化图表
 */
class BacktestChartWidget : public QWidget {
    Q_OBJECT

public:
    explicit BacktestChartWidget(QWidget* parent = nullptr);
    ~BacktestChartWidget() override;  // 在 cpp 中定义

    /**
     * @brief 设置回测数据
     */
    void setData(const QVector<BacktestDataPoint>& equityCurve,
                 const QVector<TradeMarker>& trades);

    /**
     * @brief 清空数据
     */
    void clearData();

    /**
     * @brief 设置显示范围
     */
    void setDisplayRange(const QDateTime& start, const QDateTime& end);

    /**
     * @brief 显示/隐藏交易标记
     */
    void setShowTrades(bool show);

    /**
     * @brief 显示/隐藏回撤曲线
     */
    void setShowDrawdown(bool show);

    /**
     * @brief 获取当前权益
     */
    double currentEquity() const;

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    void drawEquityCurve(QPainter& painter);
    void drawDrawdownCurve(QPainter& painter);
    void drawTradeMarkers(QPainter& painter);
    void drawGrid(QPainter& painter);
    void drawAxis(QPainter& painter);
    void drawTooltip(QPainter& painter);

    void calculateRange();
    int dateToX(const QDateTime& date) const;
    double equityToY(double equity) const;
    QDateTime xToDate(int x) const;

    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace WealthPilot

#endif // BACKTESTCHARTWIDGET_H