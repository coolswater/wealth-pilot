/**
 * @file ChartConfig.h
 * @brief 图表配置管理 - 统一管理图表相关配置
 *
 * @details 功能：
 * - K线样式配置
 * - 指标参数配置
 * - 显示选项配置
 * - 配置持久化
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#ifndef CHARTCONFIG_H
#define CHARTCONFIG_H

#include <QObject>
#include <QColor>
#include <QSettings>
#include "core/config/Tokens.h"
#include <QMap>
#include <QString>
#include <memory>

#include "core/base/Singleton.h"

/**
 * @brief 图表配置管理类
 *
 * @details 提供统一的图表配置管理：
 * - K线样式（颜色、宽度）
 * - 指标参数（周期、颜色）
 * - 显示选项（网格、十字光标）
 * - 配置保存与加载
 *
 * @example
 * @code
 * // 获取配置
 * QColor upColor = ChartConfig::instance()->upColor();
 * int ma5Period = ChartConfig::instance()->indicatorPeriod("MA5");
 *
 * // 设置配置
 * ChartConfig::instance()->setUpColor(QColor("#EF4444"));
 * ChartConfig::instance()->save();
 * @endcode
 */
class ChartConfig : public QObject, public Singleton<ChartConfig>
{
    Q_OBJECT
    friend class Singleton<ChartConfig>;

public:
    // ========== K线样式配置 ==========

    /**
     * @brief 获取上涨颜色
     */
    QColor upColor() const { return m_upColor; }

    /**
     * @brief 设置上涨颜色
     */
    void setUpColor(const QColor& color);

    /**
     * @brief 获取下跌颜色
     */
    QColor downColor() const { return m_downColor; }

    /**
     * @brief 设置下跌颜色
     */
    void setDownColor(const QColor& color);

    /**
     * @brief 获取平盘颜色
     */
    QColor flatColor() const { return m_flatColor; }

    /**
     * @brief 设置平盘颜色
     */
    void setFlatColor(const QColor& color);

    /**
     * @brief 获取蜡烛宽度
     */
    int candleWidth() const { return m_candleWidth; }

    /**
     * @brief 设置蜡烛宽度
     */
    void setCandleWidth(int width);

    /**
     * @brief 获取蜡烛间距
     */
    int candleSpacing() const { return m_candleSpacing; }

    /**
     * @brief 设置蜡烛间距
     */
    void setCandleSpacing(int spacing);

    // ========== 显示选项配置 ==========

    /**
     * @brief 是否显示网格
     */
    bool showGrid() const { return m_showGrid; }

    /**
     * @brief 设置是否显示网格
     */
    void setShowGrid(bool show);

    /**
     * @brief 是否显示十字光标
     */
    bool showCrosshair() const { return m_showCrosshair; }

    /**
     * @brief 设置是否显示十字光标
     */
    void setShowCrosshair(bool show);

    /**
     * @brief 是否显示成交量
     */
    bool showVolume() const { return m_showVolume; }

    /**
     * @brief 设置是否显示成交量
     */
    void setShowVolume(bool show);

    /**
     * @brief 成交量高度比例
     */
    double volumeHeightRatio() const { return m_volumeHeightRatio; }

    /**
     * @brief 设置成交量高度比例
     */
    void setVolumeHeightRatio(double ratio);

    // ========== 指标配置 ==========

    /**
     * @brief 获取指标周期
     * @param name 指标名称
     * @return 周期值
     */
    int indicatorPeriod(const QString& name) const;

    /**
     * @brief 设置指标周期
     * @param name 指标名称
     * @param period 周期值
     */
    void setIndicatorPeriod(const QString& name, int period);

    /**
     * @brief 获取指标颜色
     * @param name 指标名称
     * @return 颜色
     */
    QColor indicatorColor(const QString& name) const;

    /**
     * @brief 设置指标颜色
     * @param name 指标名称
     * @param color 颜色
     */
    void setIndicatorColor(const QString& name, const QColor& color);

    /**
     * @brief 获取指标启用状态
     * @param name 指标名称
     * @return 是否启用
     */
    bool indicatorEnabled(const QString& name) const;

    /**
     * @brief 设置指标启用状态
     * @param name 指标名称
     * @param enabled 是否启用
     */
    void setIndicatorEnabled(const QString& name, bool enabled);

    // ========== 配置持久化 ==========

    /**
     * @brief 加载配置
     */
    void load();

    /**
     * @brief 保存配置
     */
    void save();

    /**
     * @brief 重置为默认值
     */
    void reset();

signals:
    /**
     * @brief 配置改变信号
     */
    void configChanged();

    /**
     * @brief 颜色配置改变信号
     */
    void colorConfigChanged();

    /**
     * @brief 指标配置改变信号
     */
    void indicatorConfigChanged();

private:
    ChartConfig();
    ~ChartConfig() = default;

    // K线颜色 - 使用 Tokens 中的颜色
    QColor m_upColor{Tokens::Colors::Danger};     // 红色=涨
    QColor m_downColor{Tokens::Colors::Success};  // 绿色=跌
    QColor m_flatColor{Tokens::Colors::TextSecondary};

    // K线样式
    int m_candleWidth = 8;
    int m_candleSpacing = 2;

    // 显示选项
    bool m_showGrid = true;
    bool m_showCrosshair = true;
    bool m_showVolume = true;
    double m_volumeHeightRatio = 0.1;

    // 指标配置
    QMap<QString, int> m_indicatorPeriods;
    QMap<QString, QColor> m_indicatorColors;
    QMap<QString, bool> m_indicatorEnabled;

    // 配置文件
    std::unique_ptr<QSettings> m_settings;

    /**
     * @brief 初始化默认指标配置
     */
    void initDefaultIndicators();
};

#endif // CHARTCONFIG_H
