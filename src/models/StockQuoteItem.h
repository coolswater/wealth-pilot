#ifndef STOCKQUOTEITEM_H
#define STOCKQUOTEITEM_H

#include <QString>
#include <QColor>
#include <QRectF>
#include <QMetaType>

/**
 * @brief 股票/期货行情数据项
 * 包含基础行情信息和绘制区域
 */
class StockQuoteItem
{
public:
    // 默认构造函数（Qt元对象系统必需）
    StockQuoteItem();

    // 带参数的构造函数
    StockQuoteItem(const QString &code, const QString &name,
                   double price, double preClose, double marketCap);

    // 拷贝构造函数
    StockQuoteItem(const StockQuoteItem &other);

    // 赋值操作符
    StockQuoteItem& operator=(const StockQuoteItem &other);

    // 析构函数
    ~StockQuoteItem();

    // 基础信息
    QString code;           // 代码 (如: 000001.SZ)
    QString name;           // 名称 (如: 平安银行)
    QString industry;       // 所属行业/板块

    // 行情数据
    double price;           // 当前价格
    double preClose;        // 昨收价
    double change;          // 涨跌额
    double changePercent;   // 涨跌幅 (%)
    double marketCap;       // 流通市值 (亿元)
    double volume;          // 成交量 (万手)
    double turnover;        // 成交额 (万元)

    // 期货扩展数据
    double openInterest;    // 持仓量
    double settlement;      // 结算价
    bool isFutures;         // 是否为期货

    // 绘制区域（运行时计算）
    QRectF rect;            // 在热力图中的位置
    double weight;          // 权重(市值占比)

    // 计算方法
    double getChangePercent() const;
    QColor getColor() const;        // 根据涨跌获取颜色
    bool isLimitUp() const;         // 是否涨停（A股10%/20%，期货各异）
    bool isLimitDown() const;       // 是否跌停
};

// 注册到Qt元对象系统（信号槽传递必需）
Q_DECLARE_METATYPE(StockQuoteItem)

#endif // STOCKQUOTEITEM_H