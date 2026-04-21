// AssetPieChart.h
#ifndef ASSETPIECHART_H
#define ASSETPIECHART_H

#include <QWidget>
#include <QDateTime>  // 添加：解决不完整类型
#include <QVector>    // 添加
#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QLineSeries>
#include <QtCharts/QAreaSeries>
#include <utility>

struct TimeValuePoint {
    QDateTime time;
    double value{};

    // 显式定义拷贝构造（因为 QDateTime 删除了拷贝赋值）
    TimeValuePoint() = default;
    TimeValuePoint(QDateTime  t, double v) : time(std::move(t)), value(v) {}
    TimeValuePoint(const TimeValuePoint&) = default;
    TimeValuePoint& operator=(const TimeValuePoint&) = default;
    TimeValuePoint(TimeValuePoint&&) = default;
    TimeValuePoint& operator=(TimeValuePoint&&) = default;
};

/**
 * @brief 资产配置饼图组件
 */
class AssetPieChart : public QWidget
{
    Q_OBJECT

public:
    explicit AssetPieChart(QWidget* parent = nullptr);

    // 设置数据 - 使用C++17结构化绑定友好格式
    void setData(const QVector<std::pair<QString, double>>& assets) const;

    // 动画显示
    void animate();

private:
    QChartView* m_chartView = nullptr;
    QPieSeries* m_series = nullptr;
};

/**
 * @brief 净值走势线图组件
 */
class NetValueChart : public QWidget
{
    Q_OBJECT

public:
    explicit NetValueChart(QWidget* parent = nullptr);

    // 设置数据
    void setData(const QVector<TimeValuePoint>& values);
    void setBenchmarkData(const QVector<TimeValuePoint>& values);

    // 添加实时数据点（高性能更新）
    void appendValue(const QDateTime& time, double value);

    // 设置时间范围
    static void setTimeRange(int days);

private:
    void setupChart();

    QChartView* m_chartView = nullptr;
    QLineSeries* m_series = nullptr;
    QLineSeries* m_benchmarkSeries = nullptr;
    QAreaSeries* m_areaSeries = nullptr;

    // 数据缓存，用于重绘优化
    QVector<TimeValuePoint> m_dataCache;
    std::atomic<bool> m_updating{false};
    QVector<QDateTime> m_timeLabels; // 单独存储时间用于显示
};

#endif // ASSETPIECHART_H
