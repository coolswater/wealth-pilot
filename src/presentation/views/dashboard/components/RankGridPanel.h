/**
 * @file RankGridPanel.h
 * @brief 排行榜六宫格面板组件
 *
 * @details 显示沪A/深A涨跌榜、5分钟涨跌榜、板块热力图
 */

#ifndef RANKGRIDPANEL_H
#define RANKGRIDPANEL_H

#include <QFrame>
#include <QTableView>
#include <QTabWidget>
#include <QVector>
#include <QString>
#include <memory>

// 前向声明
class QGridLayout;

namespace WealthPilot {

/**
 * @brief 股票排行数据
 */
struct StockRankInfo {
    QString code;
    QString name;
    double price = 0.0;
    double change = 0.0;
    double changePercent = 0.0;
    double volume = 0.0;
    double amount = 0.0;
    int rank = 0;
};

/**
 * @brief 板块数据
 */
struct SectorInfo {
    QString code;
    QString name;
    double changePercent = 0.0;
    int upCount = 0;
    int downCount = 0;
    double amount = 0.0;
};

/**
 * @brief 排行榜六宫格面板
 */
class RankGridPanel : public QFrame {
    Q_OBJECT

public:
    explicit RankGridPanel(QWidget* parent = nullptr);
    ~RankGridPanel() override = default;

    /**
     * @brief 设置沪A涨幅榜数据
     */
    void setShGainData(const QVector<StockRankInfo>& data);

    /**
     * @brief 设置深A涨幅榜数据
     */
    void setSzGainData(const QVector<StockRankInfo>& data);

    /**
     * @brief 设置沪5分钟涨幅数据
     */
    void setSh5MinData(const QVector<StockRankInfo>& data);

    /**
     * @brief 设置深5分钟涨幅数据
     */
    void setSz5MinData(const QVector<StockRankInfo>& data);

    /**
     * @brief 设置板块数据
     */
    void setSectorData(const QVector<SectorInfo>& data);

    /**
     * @brief 清空所有数据
     */
    void clear();

signals:
    void stockDoubleClicked(const QString& code, const QString& name);
    void sectorDoubleClicked(const QString& sectorName);

private:
    void setupUI();
    QFrame* createRankCard(const QString& title, QTableView*& table, QAbstractItemModel*& model);

    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace WealthPilot

#endif // RANKGRIDPANEL_H