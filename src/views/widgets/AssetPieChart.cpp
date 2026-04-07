// AssetPieChart.cpp
#include "AssetPieChart.h"
#include <QtCharts/QValueAxis>
#include <QtCharts/QCategoryAxis>
#include <QVBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>

// 资产配置饼图
AssetPieChart::AssetPieChart(QWidget* parent) : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_series = new QPieSeries(this);
    m_series->setHoleSize(0.65);
    m_series->setPieSize(0.8);

    auto* chart = new QChart();
    chart->addSeries(m_series);
    chart->setBackgroundVisible(false);
    chart->setPlotAreaBackgroundVisible(false);
    chart->legend()->setVisible(false);

    // 修复：在 QChart 上设置动画，而不是 QPieSeries
    chart->setAnimationOptions(QChart::SeriesAnimations);

    m_chartView = new QChartView(chart, this);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setBackgroundBrush(Qt::transparent);

    layout->addWidget(m_chartView);
}

void AssetPieChart::setData(const QVector<std::pair<QString, double>>& assets) const
{
    m_series->clear();

    for (const auto& [name, value] : assets) {
        QPieSlice* slice = m_series->append(name, value);
        slice->setLabelVisible(false);
    }

    static const QColor colors[] = {
        QColor("#3B82F6"), QColor("#10B981"), QColor("#F97316"),
        QColor("#8B5CF6"), QColor("#EC4899")
    };

    int i = 0;
    for (auto* slice : m_series->slices()) {
        slice->setColor(colors[i % 5]);
        slice->setBorderColor(QColor("#1A1F2E"));
        slice->setBorderWidth(2);
        ++i;
    }
}

void AssetPieChart::animate()
{
    // 修复：移除不存在的 API 调用
    // QPieSeries 的动画由 QChart::SeriesAnimations 控制
    // 如果需要特殊效果，使用 QPropertyAnimation 单独控制
    if (m_series) {
        m_series->setPieSize(0.0); // 从0开始
        // 使用属性动画实现展开效果
        QPropertyAnimation* anim = new QPropertyAnimation(m_series, "pieSize");
        anim->setDuration(1000);
        anim->setStartValue(0.0);
        anim->setEndValue(0.8);
        anim->setEasingCurve(QEasingCurve::OutBack);
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    }
}

// 净值走势图
NetValueChart::NetValueChart(QWidget* parent) : QWidget(parent)
{
    setupChart();
}

void NetValueChart::setupChart()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_series = new QLineSeries(this);
    m_series->setName("我的收益");

    QPen pen(QColor("#3B82F6"));
    pen.setWidth(3);
    m_series->setPen(pen);

    m_benchmarkSeries = new QLineSeries(this);
    m_benchmarkSeries->setName("沪深300");
    QPen benchPen(QColor("#9CA3AF"));
    benchPen.setWidth(2);
    benchPen.setStyle(Qt::DashLine);
    m_benchmarkSeries->setPen(benchPen);

    QLinearGradient gradient(0, 0, 0, 400);
    gradient.setColorAt(0.0, QColor("#3B82F6").lighter(120));
    gradient.setColorAt(1.0, Qt::transparent);

    m_areaSeries = new QAreaSeries(m_series);
    m_areaSeries->setBrush(gradient);
    m_areaSeries->setPen(Qt::NoPen);

    auto* chart = new QChart();
    chart->addSeries(m_areaSeries);
    chart->addSeries(m_series);
    chart->addSeries(m_benchmarkSeries);
    chart->setBackgroundVisible(false);
    chart->setPlotAreaBackgroundVisible(false);
    chart->legend()->setAlignment(Qt::AlignTop);
    chart->legend()->setLabelColor(Qt::white);
    chart->legend()->setMarkerShape(QLegend::MarkerShapeRectangle);

    auto* axisX = new QValueAxis();
    axisX->setLabelsColor(QColor("#9CA3AF"));
    axisX->setGridLineColor(QColor("#2A3142"));
    axisX->setLinePenColor(QColor("#2A3142"));
    axisX->setLabelFormat("%d日");

    auto* axisY = new QValueAxis();
    axisY->setLabelsColor(QColor("#9CA3AF"));
    axisY->setGridLineColor(QColor("#2A3142"));
    axisY->setLinePenColor(QColor("#2A3142"));
    axisY->setLabelFormat("%+.1f%%");

    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);

    m_series->attachAxis(axisX);
    m_series->attachAxis(axisY);
    m_benchmarkSeries->attachAxis(axisX);
    m_benchmarkSeries->attachAxis(axisY);

    m_chartView = new QChartView(chart, this);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setBackgroundBrush(Qt::transparent);

    layout->addWidget(m_chartView);
}

void NetValueChart::setData(const QVector<TimeValuePoint>& values)
{
    m_series->clear();
    m_dataCache = values;

    QList<QPointF> points;
    points.reserve(values.size());

    int i = 0;
    for (const auto& point : values) {
        points.append(QPointF(i++, point.value));
        m_timeLabels.append(point.time);
    }

    m_series->replace(points);
}

void NetValueChart::setBenchmarkData(const QVector<TimeValuePoint>& values)
{
    m_benchmarkSeries->clear();

    QList<QPointF> points;
    points.reserve(values.size());

    int i = 0;
    for (const auto& point : values) {
        points.append(QPointF(i++, point.value));
    }

    m_benchmarkSeries->replace(points);
}

void NetValueChart::appendValue(const QDateTime& time, double value)
{
    if (m_updating.exchange(true)) return;

    QMetaObject::invokeMethod(this, [this, time, value]() {
        int count = m_series->count();
        m_series->append(count, value);
        m_timeLabels.append(time);

        // 限制数据点数量
        if (count > 300) {
            m_series->remove(0);
            m_timeLabels.removeFirst();

            // 修复：使用 points() 代替 pointsVector()
            QList<QPointF> points = m_series->points();
            for (int i = 0; i < points.size(); ++i) {
                points[i].setX(i);
            }
            m_series->replace(points);
        }

        m_updating = false;
    }, Qt::QueuedConnection);
}

void NetValueChart::setTimeRange(int days)
{
    // 实现时间范围筛选逻辑
    Q_UNUSED(days)
}
