#ifndef FUTURESQUOTEITEM_H
#define FUTURESQUOTEITEM_H

#include <QString>
#include <QMetaType>

/**
 * @brief 期货行情数据结构
 * 对应图片中的26个字段
 */
struct FuturesQuoteItem {
    int     serialNo;           // 序号
    QString contractName;       // 合约名称（如"沪铜加权"）
    QString contractCode;       // 合约代码（如"cu2504"）
    double  lastPrice;          // 最新价
    int     currentHand;        // 现手
    double  bidPrice;           // 买价
    double  askPrice;           // 卖价
    int     bidVolume;          // 买量
    int     askVolume;          // 卖量
    qint64  volume;             // 成交量
    double  change;             // 涨跌（最新-昨结）
    double  changePercent;      // 涨幅%
    qint64  openInterest;       // 持仓量
    int     oiChange;           // 日增仓
    double  openPrice;          // 开盘
    double  highPrice;          // 最高
    double  lowPrice;           // 最低
    double  settlement;         // 结算
    double  speedChange;        // 速涨
    double  currentChange;      // 现涨
    int     currentOiChange;    // 现增仓
    QString dynamic;            // 动态（标记等）
    double  preSettlementPrice; // 昨结算
    double  preClose;           // 昨收
    double  capital;            // 沉淀资金
    double  capitalFlow;        // 资金流向（亿）
    double  trendDegree;        // 趋势度
    double  speculationDegree;  // 投机度
    bool isMainContract = false;    // 是否主力合约
    int activityStatus = 0;          // 活跃度状态（供代理模型过滤）

    // 便捷构造函数
    FuturesQuoteItem() = default;

    // 获取涨跌颜色：红涨绿跌（A股传统，期货同理）
    [[nodiscard]] Qt::GlobalColor changeColor() const {
        if (change > 0) return Qt::red;
        if (change < 0) return Qt::green;
        return Qt::white;
    }
};

Q_DECLARE_METATYPE(FuturesQuoteItem);

#endif // FUTURESQUOTEITEM_H
