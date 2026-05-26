/**
 * @file DashboardTypes.h
 * @brief Dashboard 页面统一类型定义
 *
 * @details 将原 DashboardPage 中的类型定义抽取到独立文件
 * 便于各组件共享
 */

#ifndef DASHBOARDTYPES_H
#define DASHBOARDTYPES_H

#include <QString>
#include <QVector>

namespace WealthPilot {

// ============================================================================
// 指数相关
// ============================================================================

/**
 * @brief 指数数据结构
 */
struct IndexData {
    QString code;               ///< 指数代码
    QString name;               ///< 指数名称
    double current = 0.0;       ///< 当前点位
    double change = 0.0;        ///< 涨跌点数
    double changePercent = 0.0; ///< 涨跌幅百分比
    double volume = 0.0;        ///< 成交量
    double amount = 0.0;        ///< 成交额
    QVector<double> prices;     ///< 分时价格序列
    QVector<double> volumes;    ///< 分时成交量序列
};

// ============================================================================
// 排行榜相关
// ============================================================================

/**
 * @brief 股票排行数据结构
 */
struct StockRankData {
    QString code;               ///< 股票代码
    QString name;               ///< 股票名称
    double price = 0.0;         ///< 现价
    double change = 0.0;        ///< 涨跌额
    double changePercent = 0.0; ///< 涨跌幅
    double volume = 0.0;        ///< 成交量
    double amount = 0.0;        ///< 成交额
    double turnover = 0.0;      ///< 换手率
    double pe = 0.0;            ///< 市盈率
    int rank = 0;               ///< 排名
};

/**
 * @brief 板块数据结构
 */
struct SectorData {
    QString code;               ///< 板块代码
    QString name;               ///< 板块名称
    double changePercent = 0.0; ///< 涨跌幅
    int upCount = 0;            ///< 上涨家数
    int downCount = 0;          ///< 下跌家数
    double amount = 0.0;        ///< 成交额
    double marketCap = 0.0;     ///< 总市值
    int rank = 0;               ///< 排名
};

// ============================================================================
// 新闻相关
// ============================================================================

/**
 * @brief 新闻数据结构
 */
struct NewsData {
    QString id;                 ///< 新闻ID
    QString title;              ///< 新闻标题
    QString category;           ///< 分类（要闻/研报/公告）
    QString source;             ///< 来源
    QString publishTime;        ///< 发布时间
    QString content;            ///< 内容
    int importance = 0;         ///< 重要程度（1-3）
};

// ============================================================================
// 资金流向相关
// ============================================================================

/**
 * @brief 资金流向数据结构
 */
struct MoneyFlowData {
    QString code;               ///< 股票代码
    QString name;               ///< 股票名称
    double netInflow = 0.0;     ///< 净流入金额
    double netInflowPercent = 0.0; ///< 净流入占比
    double day3Inflow = 0.0;    ///< 3日净流入
    double day5Inflow = 0.0;    ///< 5日净流入
    int rank = 0;               ///< 排名
};

} // namespace WealthPilot

#endif // DASHBOARDTYPES_H