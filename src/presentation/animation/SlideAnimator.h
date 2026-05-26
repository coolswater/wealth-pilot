/**
 * @file SlideAnimator.h
 * @brief 滑动动画器
 *
 * @details 专门处理滑动动画：
 * - slideIn: 从边缘滑入
 * - slideOut: 滑出到边缘
 * - slide: 水平/垂直滑动
 * - pageTransition: 页面切换滑动效果
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef SLIDEANIMATOR_H
#define SLIDEANIMATOR_H

#include "AnimationTypes.h"
#include <QObject>
#include <QPointer>
#include <QEasingCurve>
#include <memory>

class QWidget;
class QPropertyAnimation;

namespace WealthPilot {

/**
 * @brief 滑动动画器
 */
class SlideAnimator : public QObject {
    Q_OBJECT

public:
    explicit SlideAnimator(QObject* parent = nullptr);
    ~SlideAnimator() override;

    /**
     * @brief 滑入动画
     * @param widget 目标控件
     * @param direction 滑动方向
     * @param config 动画配置
     */
    QPropertyAnimation* slideIn(QWidget* widget,
                                 AnimationDirection direction = AnimationDirection::Left,
                                 const AnimationConfig& config = AnimationConfig::slideIn(AnimationDirection::Left));
    
    /**
     * @brief 滑出动画
     */
    QPropertyAnimation* slideOut(QWidget* widget,
                                  AnimationDirection direction = AnimationDirection::Left,
                                  int duration = 250);
    
    /**
     * @brief 页面切换动画
     * @param fromPage 当前页面
     * @param toPage 目标页面
     * @param direction 切换方向
     */
    void pageTransition(QWidget* fromPage, QWidget* toPage,
                         AnimationDirection direction = AnimationDirection::Left,
                         int duration = 300);
    
    /**
     * @brief 停止动画
     */
    void stop(QWidget* widget);
    
    /**
     * @brief 停止所有动画
     */
    void stopAll();

signals:
    void animationStarted(QWidget* widget);
    void animationFinished(QWidget* widget);
    void pageTransitionFinished();

private:
    QPoint calculateStartPosition(QWidget* widget, AnimationDirection direction) const;
    QPoint calculateEndPosition(QWidget* widget, AnimationDirection direction) const;
    
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace WealthPilot

#endif // SLIDEANIMATOR_H