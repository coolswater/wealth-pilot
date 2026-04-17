/**
 * @file ChartStatusBar.h
 * @brief 图表状态栏 - 显示账户、连接、坐标信息
 *
 * @details 功能：
 * - 账户信息（账户ID、可用资金、保证金）
 * - 连接状态（CTP连接状态）
 * - 坐标信息（十字光标位置）
 * - 时间显示
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#ifndef CHARTSTATUSBAR_H
#define CHARTSTATUSBAR_H

#include <QWidget>
#include <QLabel>
#include <QDateTime>
#include <memory>

/**
 * @brief 图表状态栏组件
 *
 * @details 显示K线图表的状态信息，包括：
 * - 账户资金信息
 * - CTP连接状态
 * - 十字光标坐标
 *
 * @example
 * @code
 * ChartStatusBar* statusBar = new ChartStatusBar(this);
 * statusBar->setConnectionStatus("CTP 已连接", QColor("#10B981"));
 * statusBar->setCrosshairInfo(QDateTime::currentDateTime(), 3850.0, 100);
 * @endcode
 */
class ChartStatusBar : public QWidget
{
    Q_OBJECT

public:
    // ========== 构造与析构 ==========

    /**
     * @brief 构造函数
     * @param parent 父控件
     */
    explicit ChartStatusBar(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~ChartStatusBar() override;

    // ========== 公共接口 ==========

    /**
     * @brief 设置账户信息
     * @param account 账户ID
     * @param available 可用资金
     * @param margin 占用保证金
     */
    void setAccountInfo(const QString& account, double available, double margin);

    /**
     * @brief 设置连接状态
     * @param status 状态文本
     * @param color 状态颜色
     */
    void setConnectionStatus(const QString& status, const QColor& color);

    /**
     * @brief 设置坐标信息
     * @param info 坐标信息文本
     */
    void setCoordinateInfo(const QString& info);

    /**
     * @brief 设置十字光标信息
     * @param time 时间
     * @param price 价格
     * @param volume 成交量
     */
    void setCrosshairInfo(const QDateTime& time, double price, qint64 volume);

    /**
     * @brief 清空显示
     */
    void clear();

private:
    // ========== 私有方法 ==========

    void setupUI();
    QString formatMoney(double value);

    // ========== PIMPL ==========

    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // CHARTSTATUSBAR_H
