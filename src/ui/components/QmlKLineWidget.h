/**
 * @file QmlKLineWidget.h
 * @brief QML K线图容器组件 - 在 Widgets 中嵌入 QML 图表
 *
 * @details 功能：
 * - 封装 QQuickWidget 用于嵌入 QML K线图
 * - 提供 C++ 数据接口
 * - 支持主题切换
 * - 性能优化
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef QMLKLINEWIDGET_H
#define QMLKLINEWIDGET_H

#include <QWidget>
#include <QQuickWidget>
#include <QVector>
#include "core/types/MarketTypes.h"  // 包含完整类型定义

class KLineQmlModel;
class TimeShareQmlModel;
class QQmlContext;

/**
 * @brief QML K线图容器组件
 *
 * 用于在 Widgets 界面中嵌入 QML 实现的高性能 K线图
 */
class QmlKLineWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 图表类型
     */
    enum class ChartType {
        KLine,      ///< K线图
        TimeShare   ///< 分时图
    };

    explicit QmlKLineWidget(QWidget* parent = nullptr);
    ~QmlKLineWidget() override;

    // ========== 数据接口 ==========

    /**
     * @brief 设置K线数据
     * @param data K线数据列表
     */
    void setKLineData(const QVector<KLineData>& data);

    /**
     * @brief 更新最后一条K线（实时更新）
     * @param data 新的K线数据
     */
    void updateLastKLine(const KLineData& data);

    /**
     * @brief 设置分时图数据
     * @param data 分时数据列表
     * @param basePrice 昨收价（基准价）
     */
    void setTimeShareData(const QVector<TimeShareData>& data, double basePrice = 0.0);

    /**
     * @brief 追加分时数据
     * @param data 分时数据
     */
    void appendTimeShareData(const TimeShareData& data);

    /**
     * @brief 清除所有数据
     */
    void clearData();

    // ========== 图表控制 ==========

    /**
     * @brief 设置图表类型
     * @param type 图表类型
     */
    void setChartType(ChartType type);

    /**
     * @brief 设置可见K线数量
     * @param count 可见数量
     */
    void setVisibleCount(int count);

    /**
     * @brief 跳转到指定位置
     * @param index 起始索引
     */
    void scrollTo(int index);

    /**
     * @brief 缩放到全部显示
     */
    void zoomToFit();

    // ========== 主题 ==========

    /**
     * @brief 应用主题
     * @param isDark 是否深色主题
     */
    void applyTheme(bool isDark);

signals:
    /**
     * @brief K线点击信号
     * @param index K线索引
     */
    void candleClicked(int index);

    /**
     * @brief 十字光标移动信号
     * @param index K线索引
     * @param price 价格
     */
    void crosshairMoved(int index, double price);

    /**
     * @brief 图表加载完成信号
     */
    void chartLoaded();

protected:
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    void setupUI();
    void setupConnections();
    void loadQmlSource();
    void updateQmlProperty(const char* name, const QVariant& value);

    QQuickWidget* m_quickWidget = nullptr;
    KLineQmlModel* m_klineModel = nullptr;
    TimeShareQmlModel* m_timeShareModel = nullptr;

    ChartType m_chartType = ChartType::KLine;
    bool m_isDarkTheme = true;
    bool m_qmlLoaded = false;
};

#endif // QMLKLINEWIDGET_H
