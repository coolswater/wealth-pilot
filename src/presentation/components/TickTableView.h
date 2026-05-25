/**
 * @file TickTableView.h
 * @brief 分笔成交表格 - 实时显示逐笔成交记录
 *
 * @details 功能：
 * - 显示时间、价格、成交量、买卖方向
 * - 自动滚动到最新记录
 * - 限制最大行数，自动清理旧数据
 * - 颜色区分买入/卖出
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#ifndef TICKTABLEVIEW_H
#define TICKTABLEVIEW_H

#include <QTableWidget>
#include <QHeaderView>
#include <memory>

/**
 * @brief 分笔成交表格组件
 *
 * @details 实时显示逐笔成交记录，支持：
 * - 自动滚动到最新
 * - 限制最大行数
 * - 颜色区分买卖方向
 * - 高性能更新
 *
 * @example
 * @code
 * TickTableView* tickTable = new TickTableView(this);
 * tickTable->setMaxRows(500);  // 最多显示500条
 * tickTable->addTick("09:30:15", 3850.0, 10, "买");
 * @endcode
 */
class TickTableView : public QTableWidget
{
    Q_OBJECT

public:
    // ========== 构造与析构 ==========

    /**
     * @brief 构造函数
     * @param parent 父控件
     */
    explicit TickTableView(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~TickTableView() override;

    // ========== 公共接口 ==========

    /**
     * @brief 添加分笔成交记录
     * @param time 成交时间（格式：hh:mm:ss）
     * @param price 成交价格
     * @param volume 成交量
     * @param flag 买卖方向（"买" 或 "卖"）
     */
    void addTick(const QString& time, double price, int volume, const QString& flag);

    /**
     * @brief 清空所有记录
     */
    void clearTicks();

    /**
     * @brief 设置最大行数
     * @param max 最大行数（超过后自动删除旧记录）
     */
    void setMaxRows(int max);

    /**
     * @brief 获取最大行数
     * @return 最大行数
     */
    int maxRows() const;

    /**
     * @brief 获取当前行数
     * @return 当前行数
     */
    int currentRowCount() const;

private:
    // ========== 私有方法 ==========

    void setupUI();
    void setupHeader();
    void autoScrollToBottom();
    void trimExcessRows();
    void updateRowStyle(int row, const QString& flag);

    // ========== PIMPL ==========

    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // TICKTABLEVIEW_H
