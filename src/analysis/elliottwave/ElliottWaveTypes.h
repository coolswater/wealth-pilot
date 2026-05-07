/**
 * @file ElliottWaveTypes.h
 * @brief 波浪理论类型定义
 *
 * @details 定义波浪理论的核心数据结构：
 * - 波浪级别
 * - 波浪类型（推动浪/调整浪）
 * - 波浪结构
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef ELLIOTT_WAVE_TYPES_H
#define ELLIOTT_WAVE_TYPES_H

#include <QString>
#include <QDateTime>
#include <QVector>
#include <QMap>

namespace WealthPilot {
namespace ElliottWave {

/**
 * @brief 波浪级别（从大到小）
 */
enum class WaveDegree {
    GrandSuperCycle = 0,    ///< 超级大循环
    SuperCycle,             ///< 超级循环
    Cycle,                  ///< 循环
    Primary,                ///< 主级
    Intermediate,           ///< 中级
    Minor,                  ///< 小级
    Minute,                 ///< 细级
    Minuette,               ///< 微级
    Subminuette             ///< 亚微级
};

/**
 * @brief 波浪类型
 */
enum class WaveType {
    Impulse,    ///< 推动浪 (1, 3, 5, A, C)
    Corrective  ///< 调整浪 (2, 4, B)
};

/**
 * @brief 波浪方向
 */
enum class WaveDirection {
    Up,     ///< 上升浪
    Down    ///< 下降浪
};

/**
 * @brief 波浪模式
 */
enum class WavePattern {
    Impulse5,           ///< 5浪推动 (1-2-3-4-5)
    ZigZag,             ///< 锯齿形调整 (A-B-C, 5-3-5)
    Flat,               ///< 平台形调整 (A-B-C, 3-3-5)
    Triangle,           ///< 三角形调整 (A-B-C-D-E)
    Diagonal,           ///< 楔形 (引导/终结楔形)
    Complex,            ///< 复杂调整
    Unknown             ///< 未知
};

/**
 * @brief 波浪编号
 */
enum class WaveNumber {
    Wave1 = 1,
    Wave2 = 2,
    Wave3 = 3,
    Wave4 = 4,
    Wave5 = 5,
    WaveA = 10,
    WaveB = 11,
    WaveC = 12,
    WaveD = 13,
    WaveE = 14,
    Unknown = 99
};

/**
 * @brief 单个波浪结构
 */
struct Wave {
    int id = 0;                         ///< 波浪ID
    WaveNumber number = WaveNumber::Unknown; ///< 波浪编号
    WaveType type = WaveType::Impulse;  ///< 波浪类型
    WaveDirection direction = WaveDirection::Up; ///< 波浪方向
    WaveDegree degree = WaveDegree::Minor; ///< 波浪级别
    WavePattern pattern = WavePattern::Unknown; ///< 波浪模式

    QDateTime startTime;                ///< 起始时间
    QDateTime endTime;                  ///< 结束时间
    double startPrice = 0.0;            ///< 起始价格
    double endPrice = 0.0;              ///< 结束价格
    double highPrice = 0.0;             ///< 最高价
    double lowPrice = 0.0;              ///< 最低价

    double amplitude = 0.0;             ///< 振幅
    double retracement = 0.0;           ///< 回撤比例
    int barCount = 0;                   ///< K线数量

    QVector<int> subWaveIds;            ///< 子波浪ID列表
    int parentWaveId = -1;              ///< 父波浪ID

    double confidence = 0.0;            ///< 置信度
    QString note;                       ///< 备注

    /**
     * @brief 获取波浪编号字符串
     */
    QString numberString() const {
        switch (number) {
            case WaveNumber::Wave1: return "1";
            case WaveNumber::Wave2: return "2";
            case WaveNumber::Wave3: return "3";
            case WaveNumber::Wave4: return "4";
            case WaveNumber::Wave5: return "5";
            case WaveNumber::WaveA: return "A";
            case WaveNumber::WaveB: return "B";
            case WaveNumber::WaveC: return "C";
            case WaveNumber::WaveD: return "D";
            case WaveNumber::WaveE: return "E";
            default: return "?";
        }
    }

    /**
     * @brief 获取波浪标签（带级别）
     */
    QString label() const {
        return numberString(); // 可以根据需要添加级别标识
    }

    /**
     * @brief 判断是否为推动浪
     */
    bool isImpulse() const { return type == WaveType::Impulse; }

    /**
     * @brief 判断是否为调整浪
     */
    bool isCorrective() const { return type == WaveType::Corrective; }

    /**
     * @brief 判断是否为上升浪
     */
    bool isUp() const { return direction == WaveDirection::Up; }

    /**
     * @brief 计算价格变化
     */
    double priceChange() const {
        return endPrice - startPrice;
    }

    /**
     * @brief 计算价格变化百分比
     */
    double priceChangePercent() const {
        if (startPrice <= 0) return 0;
        return (endPrice - startPrice) / startPrice * 100;
    }
};

/**
 * @brief 波浪计数方案
 */
struct WaveCount {
    QString id;                         ///< 方案ID
    QString symbol;                     ///< 标的代码
    QDateTime analysisTime;             ///< 分析时间
    WaveDegree degree = WaveDegree::Minor; ///< 主级别

    QVector<Wave> waves;                ///< 波浪列表
    QMap<int, int> waveIndexMap;        ///< ID到索引的映射

    int currentWaveIndex = -1;          ///< 当前波浪索引
    WavePattern currentPattern = WavePattern::Unknown; ///< 当前模式
    WaveNumber nextExpectedWave = WaveNumber::Unknown; ///< 预期下一波浪

    double confidence = 0.0;            ///< 整体置信度
    QString scenario;                   ///< 场景描述
    QString alternativeScenario;        ///< 备选场景

    /**
     * @brief 获取当前波浪
     */
    const Wave* currentWave() const {
        if (currentWaveIndex >= 0 && currentWaveIndex < waves.size()) {
            return &waves[currentWaveIndex];
        }
        return nullptr;
    }

    /**
     * @brief 获取最后一个完成的波浪
     */
    const Wave* lastCompletedWave() const {
        for (int i = waves.size() - 1; i >= 0; --i) {
            if (waves[i].endTime.isValid()) {
                return &waves[i];
            }
        }
        return nullptr;
    }

    /**
     * @brief 根据ID获取波浪
     */
    const Wave* getWaveById(int id) const {
        auto it = waveIndexMap.find(id);
        if (it != waveIndexMap.end() && it.value() >= 0 && it.value() < waves.size()) {
            return &waves[it.value()];
        }
        return nullptr;
    }
};

/**
 * @brief 波浪分析结果
 */
struct ElliottWaveResult {
    QString symbol;                     ///< 标的代码
    QDateTime analysisTime;             ///< 分析时间
    bool isValid = false;               ///< 是否有效
    QString errorMessage;               ///< 错误信息

    QVector<WaveCount> waveCounts;      ///< 波浪计数方案（可能有多个备选）
    int preferredCountIndex = 0;        ///< 首选方案索引

    /**
     * @brief 获取首选方案
     */
    const WaveCount* preferredCount() const {
        if (preferredCountIndex >= 0 && preferredCountIndex < waveCounts.size()) {
            return &waveCounts[preferredCountIndex];
        }
        return nullptr;
    }
};

/**
 * @brief 斐波那契比例
 */
namespace Fibonacci {
    // 关键斐波那契比例
    constexpr double RETRACEMENT_236 = 0.236;
    constexpr double RETRACEMENT_382 = 0.382;
    constexpr double RETRACEMENT_500 = 0.500;
    constexpr double RETRACEMENT_618 = 0.618;
    constexpr double RETRACEMENT_786 = 0.786;

    // 扩展比例
    constexpr double EXTENSION_618 = 0.618;
    constexpr double EXTENSION_100 = 1.000;
    constexpr double EXTENSION_127 = 1.272;
    constexpr double EXTENSION_1618 = 1.618;
    constexpr double EXTENSION_2618 = 2.618;

    /**
     * @brief 计算回撤位
     */
    inline double calculateRetracement(double start, double end, double ratio) {
        return end - (end - start) * ratio;
    }

    /**
     * @brief 计算扩展位
     */
    inline double calculateExtension(double start, double end, double ratio) {
        return end + (end - start) * ratio;
    }

    /**
     * @brief 判断价格是否接近斐波那契位
     */
    inline bool isNearFibLevel(double price, double target, double tolerance = 0.02) {
        if (target <= 0) return false;
        double diff = qAbs(price - target) / target;
        return diff <= tolerance;
    }
}

} // namespace ElliottWave
} // namespace WealthPilot

Q_DECLARE_METATYPE(WealthPilot::ElliottWave::Wave)
Q_DECLARE_METATYPE(WealthPilot::ElliottWave::WaveCount)
Q_DECLARE_METATYPE(WealthPilot::ElliottWave::WaveNumber)
Q_DECLARE_METATYPE(WealthPilot::ElliottWave::WaveDirection)

#endif // ELLIOTT_WAVE_TYPES_H
