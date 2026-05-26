/**
 * @file AnimationTypes.h
 * @brief 动画类型定义和基础结构
 *
 * @details 定义动画相关的类型：
 * - 动画类型枚举
 * - 动画配置结构
 * - 动画方向定义
 * - 缓动函数封装
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef ANIMATIONTYPES_H
#define ANIMATIONTYPES_H

#include <QEasingCurve>
#include <QColor>
#include <QPoint>
#include <QSize>

namespace WealthPilot {

/**
 * @brief 动画类型
 */
enum class AnimationType {
    None,           ///< 无动画
    Fade,           ///< 淡入淡出
    Slide,          ///< 滑动
    Scale,          ///< 缩放
    Bounce,         ///< 弹跳
    Rotate,         ///< 旋转
    Expand,         ///< 展开/折叠
    Custom          ///< 自定义
};

/**
 * @brief 动画方向
 */
enum class AnimationDirection {
    Left,
    Right,
    Top,
    Bottom,
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight
};

/**
 * @brief 动画配置
 */
struct AnimationConfig {
    int duration = 250;                     ///< 持续时间（毫秒）
    int delay = 0;                          ///< 延迟（毫秒）
    QEasingCurve::Type easing = QEasingCurve::OutCubic;  ///< 缓动曲线
    bool autoReverse = false;               ///< 是否自动反向
    int loopCount = 1;                      ///< 循环次数
    
    // 淡入淡出参数
    qreal startOpacity = 0.0;
    qreal endOpacity = 1.0;
    
    // 滑动参数
    AnimationDirection direction = AnimationDirection::Left;
    QPoint startOffset;
    QPoint endOffset;
    
    // 缩放参数
    qreal startScale = 0.0;
    qreal endScale = 1.0;
    
    // 旋转参数
    qreal startAngle = 0.0;
    qreal endAngle = 360.0;
    
    /**
     * @brief 创建默认淡入配置
     */
    static AnimationConfig fadeIn(int duration = 200) {
        AnimationConfig config;
        config.duration = duration;
        config.startOpacity = 0.0;
        config.endOpacity = 1.0;
        return config;
    }
    
    /**
     * @brief 创建默认淡出配置
     */
    static AnimationConfig fadeOut(int duration = 200) {
        AnimationConfig config;
        config.duration = duration;
        config.startOpacity = 1.0;
        config.endOpacity = 0.0;
        return config;
    }
    
    /**
     * @brief 创建默认滑入配置
     */
    static AnimationConfig slideIn(AnimationDirection dir, int duration = 250) {
        AnimationConfig config;
        config.duration = duration;
        config.direction = dir;
        config.startOpacity = 0.0;
        config.endOpacity = 1.0;
        return config;
    }
    
    /**
     * @brief 创建默认缩放配置
     */
    static AnimationConfig scaleIn(int duration = 300) {
        AnimationConfig config;
        config.duration = duration;
        config.startScale = 0.0;
        config.endScale = 1.0;
        config.easing = QEasingCurve::OutBack;
        return config;
    }
};

/**
 * @brief 动画状态
 */
enum class AnimationState {
    Idle,           ///< 空闲
    Running,        ///< 运行中
    Paused,         ///< 已暂停
    Finished        ///< 已完成
};

/**
 * @brief 动画回调
 */
struct AnimationCallbacks {
    std::function<void()> onStart;          ///< 开始回调
    std::function<void()> onFinish;         ///< 完成回调
    std::function<void(qreal)> onUpdate;    ///< 更新回调（进度 0-1）
};

} // namespace WealthPilot

#endif // ANIMATIONTYPES_H
