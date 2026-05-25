#include "StockQuoteItem.h"
#include <QtMath>

/**
 * @brief 默认构造函数
 * 初始化所有数值成员为0，避免未定义行为
 */
StockQuoteItem::StockQuoteItem()
    : code("")
    , name("")
    , industry("")
    , price(0.0)
    , preClose(0.0)
    , change(0.0)
    , changePercent(0.0)
    , marketCap(0.0)
    , volume(0.0)
    , turnover(0.0)
    , openInterest(0.0)
    , settlement(0.0)
    , isFutures(false)
    , weight(0.0)
{
    // rect 自动初始化为空 QRectF
}

/**
 * @brief 带参数构造函数
 */
StockQuoteItem::StockQuoteItem(const QString &code, const QString &name,
                               double price, double preClose, double marketCap)
    : code(code)
    , name(name)
    , industry("")
    , price(price)
    , preClose(preClose)
    , change(price - preClose)
    , changePercent(preClose > 0 ? ((price - preClose) / preClose * 100.0) : 0.0)
    , marketCap(marketCap)
    , volume(0.0)
    , turnover(0.0)
    , openInterest(0.0)
    , settlement(0.0)
    , isFutures(false)
    , weight(0.0)
{
}

/**
 * @brief 拷贝构造函数
 * Qt容器（QVector, QMap）操作必需
 */
StockQuoteItem::StockQuoteItem(const StockQuoteItem &other)
    : code(other.code)
    , name(other.name)
    , industry(other.industry)
    , price(other.price)
    , preClose(other.preClose)
    , change(other.change)
    , changePercent(other.changePercent)
    , marketCap(other.marketCap)
    , volume(other.volume)
    , turnover(other.turnover)
    , openInterest(other.openInterest)
    , settlement(other.settlement)
    , isFutures(other.isFutures)
    , rect(other.rect)
    , weight(other.weight)
{
}

/**
 * @brief 赋值操作符
 */
StockQuoteItem& StockQuoteItem::operator=(const StockQuoteItem &other)
{
    if (this != &other) {
        code = other.code;
        name = other.name;
        industry = other.industry;
        price = other.price;
        preClose = other.preClose;
        change = other.change;
        changePercent = other.changePercent;
        marketCap = other.marketCap;
        volume = other.volume;
        turnover = other.turnover;
        openInterest = other.openInterest;
        settlement = other.settlement;
        isFutures = other.isFutures;
        rect = other.rect;
        weight = other.weight;
    }
    return *this;
}

/**
 * @brief 析构函数
 */
StockQuoteItem::~StockQuoteItem()
{
    // 无动态分配资源，默认即可
}

/**
 * @brief 获取涨跌幅（实时计算）
 */
double StockQuoteItem::getChangePercent() const
{
    if (preClose > 0) {
        return ((price - preClose) / preClose) * 100.0;
    }
    return changePercent;  // 如果preClose无效，使用缓存值
}

/**
 * @brief 根据涨跌获取显示颜色
 * A股风格：红涨绿跌
 */
QColor StockQuoteItem::getColor() const
{
    if (changePercent > 0) {
        // 上涨：红色系，涨幅越大越亮
        int intensity = qMin(255, static_cast<int>(55 + changePercent * 20));
        return QColor(intensity, 0, 0);
    } else if (changePercent < 0) {
        // 下跌：绿色系，跌幅越大越亮
        int intensity = qMin(255, static_cast<int>(55 - changePercent * 20));
        return QColor(0, intensity, 0);
    } else {
        // 平盘：灰色
        return QColor(60, 60, 60);
    }
}

/**
 * @brief 判断是否涨停
 * A股主板10%，创业板/科创板20%，ST股5%
 * 期货根据品种不同（6%-20%不等），这里简化处理
 */
bool StockQuoteItem::isLimitUp() const
{
    if (isFutures) {
        // 期货通常涨跌幅较大，这里假设±10%为涨停
        return changePercent >= 9.9;
    } else {
        // A股判断
        if (code.startsWith("68") || code.startsWith("3")) {
            // 科创板/创业板 20%
            return changePercent >= 19.9;
        } else if (code.startsWith("ST") || name.startsWith("ST")) {
            // ST股 5%
            return changePercent >= 4.9;
        } else {
            // 主板/中小板 10%
            return changePercent >= 9.9;
        }
    }
}

/**
 * @brief 判断是否跌停
 */
bool StockQuoteItem::isLimitDown() const
{
    if (isFutures) {
        return changePercent <= -9.9;
    } else {
        if (code.startsWith("68") || code.startsWith("3")) {
            return changePercent <= -19.9;
        } else if (code.startsWith("ST") || name.startsWith("ST")) {
            return changePercent <= -4.9;
        } else {
            return changePercent <= -9.9;
        }
    }
}