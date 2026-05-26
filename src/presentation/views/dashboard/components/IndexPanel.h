/**
 * @file IndexPanel.h
 * @brief 指数面板组件 - 显示大盘指数
 *
 * @details 显示上证、深证、创业板等主要指数
 * 独立组件，可复用
 */

#ifndef INDEXPANEL_H
#define INDEXPANEL_H

#include <QFrame>
#include <QLabel>
#include <QVector>
#include <QString>

// 前向声明
class QHBoxLayout;

namespace WealthPilot {

/**
 * @brief 指数数据结构
 */
struct IndexInfo {
    QString code;               ///< 指数代码
    QString name;               ///< 指数名称
    double current = 0.0;       ///< 当前点位
    double change = 0.0;        ///< 涨跌点数
    double changePercent = 0.0; ///< 涨跌幅百分比
    double volume = 0.0;        ///< 成交量
    double amount = 0.0;        ///< 成交额
};

/**
 * @brief 指数面板组件
 *
 * 显示多个指数卡片，支持主题切换
 */
class IndexPanel : public QFrame {
    Q_OBJECT

public:
    explicit IndexPanel(QWidget* parent = nullptr);
    ~IndexPanel() override = default;

    /**
     * @brief 设置指数数据
     * @param data 指数数据列表
     */
    void setData(const QVector<IndexInfo>& data);

    /**
     * @brief 更新单个指数
     * @param code 指数代码
     * @param data 新数据
     */
    void updateIndex(const QString& code, const IndexInfo& data);

    /**
     * @brief 清空数据
     */
    void clear();

signals:
    /**
     * @brief 指数点击信号
     */
    void indexClicked(const QString& code);

protected:
    void setupUI();

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace WealthPilot

#endif // INDEXPANEL_H
