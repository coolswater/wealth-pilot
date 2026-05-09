/**
 * @file MarketDepthWidget.h
 * @brief 盘口信息组件 - 实时行情深度显示
 *
 * @details 功能：
 * - 合约基本信息（代码、名称）
 * - 最新价、涨跌幅
 * - 买卖五档盘口
 * - 成交量、持仓量统计
 * - 涨停/跌停价格
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#ifndef MARKETDEPTHWIDGET_H
#define MARKETDEPTHWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QGridLayout>
#include <memory>
#include "core/types/MarketTypes.h"  // 使用统一的类型定义

// 前向声明
struct MarketData;

/**
 * @brief 盘口信息组件
 *
 * @details 显示期货合约的实时行情深度信息，包括：
 * - 合约代码和名称
 * - 最新价、涨跌额、涨跌幅
 * - 买卖五档价格和数量
 * - 开高低收、成交量、持仓量
 * - 涨停价、跌停价
 *
 * @example
 * @code
 * MarketDepthWidget* depth = new MarketDepthWidget(this);
 * depth->setInstrument("IF2501", "沪深300指数期货");
 * depth->updateQuote(marketData);
 * @endcode
 */
class MarketDepthWidget : public QWidget
{
    Q_OBJECT

public:
    // ========== 构造与析构 ==========

    /**
     * @brief 构造函数
     * @param parent 父控件
     */
    explicit MarketDepthWidget(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~MarketDepthWidget() override;

    // ========== 公共接口 ==========

    /**
     * @brief 设置合约信息
     * @param instrumentId 合约代码
     * @param instrumentName 合约名称
     */
    void setInstrument(const QString& instrumentId, const QString& instrumentName);

    /**
     * @brief 更新行情数据
     * @param quote 行情数据
     */
    void updateQuote(const MarketData& quote);

    /**
     * @brief 清空显示
     */
    void clear();

    /**
     * @brief 获取当前合约代码
     * @return 合约代码
     */
    QString instrumentId() const;

signals:
    /**
     * @brief 买入信号（点击买价）
     * @param price 买入价格
     */
    void buyClicked(double price);

    /**
     * @brief 卖出信号（点击卖价）
     * @param price 卖出价格
     */
    void sellClicked(double price);

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    // ========== 私有方法 ==========

    void setupUI();
    void setupHeader();
    void setupPricePanel();
    void setupDepthPanel();
    void setupStatisticsPanel();
    void updatePriceColor(QLabel* label, double change);
    QString formatPrice(double price, int precision = 2);
    QString formatVolume(qint64 volume);
    QString formatChange(double change, double base);
    
    // 布局创建方法
    QLayout* createHeaderLayout();
    QLayout* createPriceLayout();
    QLayout* createDepthLayout();
    QLayout* createStatisticsLayout();

    // ========== PIMPL ==========

    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // MARKETDEPTHWIDGET_H
