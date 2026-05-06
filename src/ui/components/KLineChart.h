/**
 * @file KLineChart.h
 * @brief K线图组件 - 高性能K线图表
 *
 * @details 功能：
 * - K线绘制（开高低收）
 * - 成交量柱状图
 * - 技术指标叠加
 * - 十字光标
 * - 缩放和平移
 * - 性能优化：双缓冲绘制、数据压缩
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */
#ifndef KLINECHART_H
#define KLINECHART_H

#include <QWidget>
#include <QVector>
#include <QDateTime>
#include <QPainter>
#include <QTimer>
#include <memory>
#include "core/types/MarketTypes.h"  // 使用统一的类型定义

/**
 * @brief 主图指标类型
 */
enum class MainIndicator {
    None,       // 无指标
    MA,         // 移动平均线组合（MA5, MA10, MA20）
    EMA,        // 指数移动平均组合（EMA12, EMA26）
    BOLL,       // 布林带
    DMI,        // DMI指标（趋向指标）
    ENE,        // ENE轨道线
    SAR,        // 抛物线转向
    TD,         // 神奇九转
    CHANLUN     // 缠论指标（笔、线段、中枢、买卖点）
};

/**
 * @brief 副图指标类型
 */
enum class SubIndicator {
    None,       // 无指标
    MACD,       // MACD指标
    KDJ,        // KDJ指标
    RSI,        // RSI指标
    EXPMA,      // EXPMA指标（指数平均数）
    VOLUME,     // 成交量（默认显示）
    OBV,        // OBV指标
    CCI,        // CCI指标
    ATR,        // ATR指标
    WR,         // WR指标
    PSY,        // PSY指标
    KD,         // KD指标
    DMA         // DMA指标
};

/**
 * @brief K线样式配置
 */
struct KLineStyle {
    QColor upColor;             // 上涨颜色
    QColor downColor;           // 下跌颜色
    QColor flatColor;           // 平盘颜色
    int candleWidth;           // 蜡烛宽度
    int candleSpacing;         // 蜡烛间距
    bool showVolume;           // 是否显示成交量
    double volumeHeightRatio;  // 成交量高度比例
};

/**
 * @brief K线图组件
 */
class KLineChart : public QWidget
{
    Q_OBJECT

public:
    explicit KLineChart(QWidget *parent = nullptr);
    ~KLineChart();

    // ========== 数据管理 ==========

    /**
     * @brief 设置K线数据
     */
    void setData(const QVector<KLineData>& data);

    /**
     * @brief 添加K线数据
     */
    void addData(const KLineData& data);

    /**
     * @brief 更新最后一条K线
     */
    void updateLastData(const KLineData& data);

    /**
     * @brief 清空数据
     */
    void clearData();

    /**
     * @brief 获取数据
     */
    QVector<KLineData> data() const;

    // ========== 样式设置 ==========

    /**
     * @brief 设置样式
     */
    void setStyle(const KLineStyle& style);

    /**
     * @brief 获取样式
     */
    KLineStyle style() const;

    // ========== 视图控制 ==========

    /**
     * @brief 缩放
     */
    void zoom(double factor);

    /**
     * @brief 平移
     */
    void pan(int dx);

    /**
     * @brief 重置视图
     */
    void resetView();

    /**
     * @brief 显示指定范围
     */
    void showRange(int startIndex, int count);

    /**
     * @brief 显示最新数据
     */
    void showLatest(int count = 100);
    
    /**
     * @brief 获取可见起始索引
     */
    int visibleStartIndex() const;
    
    /**
     * @brief 获取可见数量
     */
    int visibleCount() const;

    // ========== 技术指标 ==========

    /**
     * @brief 设置主图指标（仅显示一个）
     * @param indicator 主图指标类型
     */
    void setMainIndicator(MainIndicator indicator);

    /**
     * @brief 获取当前主图指标
     */
    MainIndicator mainIndicator() const;

    /**
     * @brief 设置副图指标
     * @param indicator 副图指标类型
     */
    void setSubIndicator(SubIndicator indicator);

    /**
     * @brief 获取当前副图指标
     */
    SubIndicator subIndicator() const;

    /**
     * @brief 添加技术指标
     */
    void addIndicator(const QString& name, const QVector<double>& values, const QColor& color);

    /**
     * @brief 移除技术指标
     */
    void removeIndicator(const QString& name);

    /**
     * @brief 清空所有指标
     */
    void clearIndicators();

signals:
    /**
     * @brief 十字光标移动信号（带完整K线信息）
     */
    void crosshairMoved(const QDateTime& time, double price);

    /**
     * @brief K线信息信号
     */
    void klineInfoChanged(const KLineData& kline, int index);

    /**
     * @brief 点击信号
     */
    void clicked(const QDateTime& time, double price);
    
    /**
     * @brief 可见范围变化信号（用于主图副图联动）
     */
    void visibleRangeChanged(int startIndex, int count);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    // 绘制方法
    void drawBackground(QPainter& painter);
    void drawGrid(QPainter& painter);
    void drawCandles(QPainter& painter);
    void drawVolume(QPainter& painter);
    void drawMainIndicators(QPainter& painter);      // 绘制主图指标
    void drawSubIndicators(QPainter& painter);       // 绘制副图指标
    void drawCrosshair(QPainter& painter);
    void drawAxis(QPainter& painter);
    void drawKLineInfo(QPainter& painter);  // 绘制K线信息
    
    // 指标计算
    void calculateMA(int period = 5);
    void calculateEMA(int period = 12);
    void calculateBOLL(int period = 20);
    void calculateDMI(int period = 14);       // DMI指标
    void calculateENE(int period = 10);      // ENE轨道线
    void calculateMACD();
    void calculateKDJ();
    void calculateRSI(int period = 14);
    void calculateEXPMA(int period = 12);    // EXPMA指标
    
    // 坐标转换
    int timeToX(int index) const;
    int priceToY(double price) const;
    double xToTime(int x) const;
    double yToPrice(int y) const;
    
    // 数据压缩（性能优化）
    void compressData();
    
    // 计算可见范围
    void calculateVisibleRange();

    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // KLINECHART_H
