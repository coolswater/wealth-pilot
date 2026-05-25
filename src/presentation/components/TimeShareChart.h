/**
 * @file TimeShareChart.h
 * @brief 分时图组件 - 股票/期货日内分时走势图
 *
 * @details 实现功能：
 * - 分时价格曲线绘制
 * - 均线绘制
 * - 成交量柱状图
 * - 价格区域填充
 * - 十字光标
 * - 昨收基准线
 * - 涨跌区域着色
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef TIMESHARECHART_H
#define TIMESHARECHART_H

#include <QWidget>
#include <QVector>
#include <QDateTime>
#include <QColor>
#include <memory>

// 使用现有的 TimeShareData 定义
#include "data/market/StockDataSource.h"

/**
 * @brief 分时图样式配置
 */
struct TimeShareStyle
{
    // 价格线
    int priceLineWidth = 2;
    QColor priceLineColor = QColor("#3B82F6"); ///< 价格线颜色（主色）

    // 均线
    int avgLineWidth = 1;
    QColor avgLineColor = QColor("#F59E0B"); ///< 均线颜色（金色）

    // 昨收基准线
    int baselineWidth = 1;
    QColor baselineColor = QColor("#6B7280"); ///< 昨收线颜色（灰色）
    Qt::PenStyle baselineStyle = Qt::DashLine; ///< 虚线样式

    // 涨跌区域填充
    QColor upFillColor = QColor(239, 68, 68, 30); ///< 上涨区域（红色半透明）
    QColor downFillColor = QColor(16, 185, 129, 30); ///< 下跌区域（绿色半透明）

    // 成交量
    int volumeBarWidth = 4;
    QColor volumeUpColor = QColor("#EF4444"); ///< 成交量上涨色
    QColor volumeDownColor = QColor("#10B981"); ///< 成交量下跌色

    // 十字光标
    int crosshairWidth = 1;
    QColor crosshairColor = QColor("#3B82F6"); ///< 十字光标颜色
    QColor crosshairBgColor = QColor(26, 35, 50, 200); ///< 信息框背景

    // 网格
    bool showGrid = true;
    QColor gridColor = QColor(45, 55, 72, 100); ///< 网格颜色

    // 字体
    int labelFontSize = 11;
    int dataFontSize = 12;
};

/**
 * @brief 分时图组件
 */
class TimeShareChart : public QWidget
{
    Q_OBJECT

public:
    explicit TimeShareChart(QWidget* parent = nullptr);
    ~TimeShareChart() override;

    // 设置数据
    void setData(const QVector<TimeShareData>& data);
    void setYesterdayClose(double price); ///< 设置昨收价
    void setInstrumentInfo(const QString& symbol, const QString& name);

    // 兼容旧 API
    void setData(const QVector<QPair<QDateTime, double>>& prices,
                 const QVector<qint64>& volumes,
                 double basePrice = 0.0)
    {
        QVector<TimeShareData> data;
        for (int i = 0; i < prices.size() && i < volumes.size(); ++i)
        {
            TimeShareData d;
            d.time = prices[i].first;
            d.price = prices[i].second;
            d.volume = volumes[i];
            data.append(d);
        }
        setData(data);
        if (basePrice > 0)
        {
            setYesterdayClose(basePrice);
        }
    }

    void clearData() { clear(); }

    // 获取数据
    QVector<TimeShareData> data() const;
    double yesterdayClose() const;

    // 兼容旧 API
    QVector<QPair<QDateTime, double>> prices() const;
    QVector<qint64> volumes() const;
    double basePrice() const;

    // 样式配置
    void setStyle(const TimeShareStyle& style);
    TimeShareStyle style() const;

    // 清除数据
    void clear();

    // 刷新显示
    void refresh();

    signals :
    /**
     * @brief 鼠标移动信号
     * @param data 当前数据点
     * @param index 数据索引
     */

    void mouseMoved(const TimeShareData& data, int index);

    /**
     * @brief 鼠标离开信号
     */
    void mouseLeft();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    // PIMPL 实现
    struct Impl;
    std::unique_ptr<Impl> d;

    // 绘制方法
    void drawBackground(QPainter* painter);
    void drawGrid(QPainter* painter);
    void drawBaseline(QPainter* painter);
    void drawPriceArea(QPainter* painter);
    void drawPriceLine(QPainter* painter);
    void drawAvgLine(QPainter* painter);
    void drawVolume(QPainter* painter);
    void drawCrosshair(QPainter* painter);
    void drawLabels(QPainter* painter);
    void drawInfoBox(QPainter* painter);

    // 计算方法
    void calculateLayout();
    void calculatePriceRange();
    void calculateVolumeRange();
    int timeToX(qint64 timestamp) const;
    int timeToX(const QDateTime& time) const;
    int priceToY(double price) const;
    double yToPrice(int y) const;
    qint64 xToTime(int x) const;
    int findDataIndex(int x) const;
};

#endif // TIMESHARECHART_H
