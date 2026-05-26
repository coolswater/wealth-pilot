/**
 * @file InfoPanel.h
 * @brief 底部信息面板组件
 *
 * @details 包含自选股、新闻、资金流向三个区域
 */

#ifndef INFOPANEL_H
#define INFOPANEL_H

#include <QFrame>
#include <QTableView>
#include <QListWidget>
#include <QVector>
#include <QString>
#include <memory>

namespace WealthPilot {

/**
 * @brief 新闻数据
 */
struct NewsInfo {
    QString id;
    QString title;
    QString category;
    QString source;
    QString publishTime;
    int importance = 0;
};

/**
 * @brief 资金流向数据
 */
struct MoneyFlowInfo {
    QString code;
    QString name;
    double netInflow = 0.0;
    double netInflowPercent = 0.0;
    double day3Inflow = 0.0;
    double day5Inflow = 0.0;
    int rank = 0;
};

/**
 * @brief 底部信息面板
 */
class InfoPanel : public QFrame {
    Q_OBJECT

public:
    explicit InfoPanel(QWidget* parent = nullptr);
    ~InfoPanel() override = default;

    /**
     * @brief 设置自选股数据
     */
    void setWatchlistData(const QVector<StockRankInfo>& data);

    /**
     * @brief 设置新闻数据
     */
    void setNewsData(const QVector<NewsInfo>& data);

    /**
     * @brief 设置资金流向数据
     */
    void setMoneyFlowData(const QVector<MoneyFlowInfo>& data);

    /**
     * @brief 清空所有数据
     */
    void clear();

signals:
    void stockDoubleClicked(const QString& code, const QString& name);
    void newsClicked(const QString& newsId, const QString& title);

private:
    void setupUI();
    void setupWatchlistSection();
    void setupNewsSection();
    void setupMoneyFlowSection();

    struct Impl;
    std::unique_ptr<Impl> d;
};

// 引用 StockRankInfo 定义
using StockRankInfo = StockRankInfo;

} // namespace WealthPilot

#endif // INFOPANEL_H