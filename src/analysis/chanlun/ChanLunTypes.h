/**
 * @file ChanLunTypes.h
 * @brief 缠论基础类型定义
 *
 * @details 定义缠论分析所需的基础数据结构：
 * - K线数据（包含处理后）
 * - 分型（顶分型、底分型）
 * - 笔
 * - 线段
 * - 中枢
 * - 买卖点
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef CHANLUN_TYPES_H
#define CHANLUN_TYPES_H

#include <QVector>
#include <QDateTime>
#include <QPointF>
#include <QRectF>
#include <QString>

namespace WealthPilot {
namespace ChanLun {

// ============================================================================
// 基础K线数据
// ============================================================================

/**
 * @brief 原始K线数据
 */
struct RawKLine {
    QDateTime time;         ///< 时间
    double open = 0.0;      ///< 开盘价
    double high = 0.0;      ///< 最高价
    double low = 0.0;       ///< 最低价
    double close = 0.0;     ///< 收盘价
    double volume = 0.0;    ///< 成交量
    
    /**
     * @brief 判断是否有效
     */
    bool isValid() const {
        return time.isValid() && high >= low && high >= open && high >= close 
               && low <= open && low <= close;
    }
};

/**
 * @brief 标准K线（包含处理后）
 */
struct StandardKLine {
    int originalIndex = -1;     ///< 原始K线索引
    QDateTime time;             ///< 时间
    double high = 0.0;          ///< 处理后最高价
    double low = 0.0;           ///< 处理后最低价
    double open = 0.0;          ///< 开盘价（原始）
    double close = 0.0;         ///< 收盘价（原始）
    double volume = 0.0;        ///< 成交量
    int direction = 0;          ///< 方向：1=向上处理，-1=向下处理，0=无包含
    
    /**
     * @brief 获取K线中点
     */
    double middle() const { return (high + low) / 2.0; }
    
    /**
     * @brief 获取K线实体高度
     */
    double height() const { return high - low; }
};

// ============================================================================
// 分型定义
// ============================================================================

/**
 * @brief 分型类型
 */
enum class FractalType {
    None = 0,       ///< 无分型
    Top = 1,        ///< 顶分型
    Bottom = -1     ///< 底分型
};

/**
 * @brief 分型结构
 */
struct Fractal {
    int index = -1;             ///< 标准K线索引
    FractalType type = FractalType::None;   ///< 分型类型
    double value = 0.0;         ///< 分型值（顶的高点或底的低点）
    QDateTime time;             ///< 时间
    
    /**
     * @brief 是否为顶分型
     */
    bool isTop() const { return type == FractalType::Top; }
    
    /**
     * @brief 是否为底分型
     */
    bool isBottom() const { return type == FractalType::Bottom; }
    
    /**
     * @brief 是否有效
     */
    bool isValid() const { return index >= 0 && type != FractalType::None; }
};

// ============================================================================
// 笔定义
// ============================================================================

/**
 * @brief 笔方向
 */
enum class PenDirection {
    Up = 1,         ///< 向上笔
    Down = -1       ///< 向下笔
};

/**
 * @brief 笔结构
 */
struct Pen {
    int startIndex = -1;        ///< 起始分型索引
    int endIndex = -1;          ///< 结束分型索引
    int startKLineIndex = -1;   ///< 起始K线索引
    int endKLineIndex = -1;     ///< 结束K线索引
    PenDirection direction = PenDirection::Up;  ///< 方向
    double startValue = 0.0;    ///< 起始值
    double endValue = 0.0;      ///< 结束值
    QDateTime startTime;        ///< 起始时间
    QDateTime endTime;          ///< 结束时间
    
    /**
     * @brief 是否为向上笔
     */
    bool isUp() const { return direction == PenDirection::Up; }
    
    /**
     * @brief 是否为向下笔
     */
    bool isDown() const { return direction == PenDirection::Down; }
    
    /**
     * @brief 笔的长度
     */
    double length() const { return qAbs(endValue - startValue); }
    
    /**
     * @brief 是否有效
     */
    bool isValid() const { return startIndex >= 0 && endIndex >= 0; }
};

// ============================================================================
// 线段定义
// ============================================================================

/**
 * @brief 线段方向
 */
enum class SegmentDirection {
    Up = 1,         ///< 向上线段
    Down = -1       ///< 向下线段
};

/**
 * @brief 线段结构
 */
struct Segment {
    int startIndex = -1;        ///< 起始笔索引
    int endIndex = -1;          ///< 结束笔索引
    int startKLineIndex = -1;   ///< 起始K线索引
    int endKLineIndex = -1;     ///< 结束K线索引
    SegmentDirection direction = SegmentDirection::Up;  ///< 方向
    double startValue = 0.0;    ///< 起始值
    double endValue = 0.0;      ///< 结束值
    QDateTime startTime;        ///< 起始时间
    QDateTime endTime;          ///< 结束时间
    bool isConfirmed = false;   ///< 是否确认（被破坏）
    
    /**
     * @brief 线段长度
     */
    double length() const { return qAbs(endValue - startValue); }
    
    /**
     * @brief 是否有效
     */
    bool isValid() const { return startIndex >= 0 && endIndex >= 0; }
};

// ============================================================================
// 中枢定义
// ============================================================================

/**
 * @brief 中枢结构
 */
struct Pivot {
    int startIndex = -1;        ///< 起始线段索引
    int endIndex = -1;          ///< 结束线段索引
    int startKLineIndex = -1;   ///< 起始K线索引
    int endKLineIndex = -1;     ///< 结束K线索引
    double zg = 0.0;            ///< 中枢区间下沿（ZG）
    double zd = 0.0;            ///< 中枢区间上沿（ZD）
    double gg = 0.0;            ///< 中枢波动下沿（GG）
    double dd = 0.0;            ///< 中枢波动上沿（DD）
    int level = 0;              ///< 中枢级别
    QDateTime startTime;        ///< 起始时间
    QDateTime endTime;          ///< 结束时间
    bool isExtended = false;    ///< 是否延伸
    
    /**
     * @brief 中枢中点
     */
    double middle() const { return (zg + zd) / 2.0; }
    
    /**
     * @brief 中枢高度
     */
    double height() const { return zd - zg; }
    
    /**
     * @brief 是否有效
     */
    bool isValid() const { return startIndex >= 0 && endIndex >= 0 && zd > zg; }
    
    /**
     * @brief 判断价格是否在中枢内
     */
    bool contains(double price) const {
        return price >= zg && price <= zd;
    }
};

// ============================================================================
// 买卖点定义
// ============================================================================

/**
 * @brief 买卖点类型
 */
enum class SignalType {
    None = 0,           ///< 无信号
    Buy1 = 1,           ///< 第一类买点
    Sell1 = -1,         ///< 第一类卖点
    Buy2 = 2,           ///< 第二类买点
    Sell2 = -2,         ///< 第二类卖点
    Buy3 = 3,           ///< 第三类买点
    Sell3 = -3          ///< 第三类卖点
};

/**
 * @brief 买卖点结构
 */
struct TradeSignal {
    int index = -1;             ///< K线索引
    SignalType type = SignalType::None;     ///< 信号类型
    double price = 0.0;         ///< 信号价格
    QDateTime time;             ///< 信号时间
    int pivotIndex = -1;        ///< 关联的中枢索引
    double strength = 0.0;      ///< 信号强度（0-1）
    QString description;        ///< 信号描述
    
    /**
     * @brief 是否为买点
     */
    bool isBuy() const {
        return type == SignalType::Buy1 || type == SignalType::Buy2 || type == SignalType::Buy3;
    }
    
    /**
     * @brief 是否为卖点
     */
    bool isSell() const {
        return type == SignalType::Sell1 || type == SignalType::Sell2 || type == SignalType::Sell3;
    }
    
    /**
     * @brief 获取信号名称
     */
    QString name() const {
        switch (type) {
            case SignalType::Buy1: return QStringLiteral("一买");
            case SignalType::Sell1: return QStringLiteral("一卖");
            case SignalType::Buy2: return QStringLiteral("二买");
            case SignalType::Sell2: return QStringLiteral("二卖");
            case SignalType::Buy3: return QStringLiteral("三买");
            case SignalType::Sell3: return QStringLiteral("三卖");
            default: return QString();
        }
    }
    
    /**
     * @brief 是否有效
     */
    bool isValid() const { return index >= 0 && type != SignalType::None; }
};

// ============================================================================
// 背驰定义
// ============================================================================

/**
 * @brief 背驰类型
 */
enum class DivergenceType {
    None = 0,           ///< 无背驰
    TopDivergence = 1,   ///< 顶背驰
    BottomDivergence = -1 ///< 底背驰
};

/**
 * @brief 背驰结构
 */
struct Divergence {
    int index1 = -1;            ///< 第一个比较点索引
    int index2 = -1;            ///< 第二个比较点索引
    DivergenceType type = DivergenceType::None;  ///< 背驰类型
    double macd1 = 0.0;         ///< 第一个点的MACD值
    double macd2 = 0.0;         ///< 第二个点的MACD值
    double price1 = 0.0;        ///< 第一个点的价格
    double price2 = 0.0;        ///< 第二个点的价格
    double strength = 0.0;      ///< 背驰强度
    
    /**
     * @brief 是否有效
     */
    bool isValid() const { return index1 >= 0 && index2 >= 0 && type != DivergenceType::None; }
};

// ============================================================================
// 缠论分析结果
// ============================================================================

/**
 * @brief 缠论完整分析结果
 */
struct ChanLunResult {
    QVector<StandardKLine> klines;          ///< 标准K线
    QVector<Fractal> fractals;              ///< 分型
    QVector<Pen> pens;                      ///< 笔
    QVector<Segment> segments;              ///< 线段
    QVector<Pivot> pivots;                  ///< 中枢
    QVector<TradeSignal> tradeSignals;      ///< 买卖点信号
    QVector<Divergence> divergences;        ///< 背驰
    
    /**
     * @brief 清空所有数据
     */
    void clear() {
        klines.clear();
        fractals.clear();
        pens.clear();
        segments.clear();
        pivots.clear();
        tradeSignals.clear();
        divergences.clear();
    }
    
    /**
     * @brief 是否有效
     */
    bool isValid() const { return !klines.isEmpty(); }
};

} // namespace ChanLun
} // namespace WealthPilot

#endif // CHANLUN_TYPES_H
