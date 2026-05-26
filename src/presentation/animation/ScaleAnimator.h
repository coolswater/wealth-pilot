/**
 * @file ScaleAnimator.h
 * @brief 缩放动画器
 *
 * @details 专门处理缩放动画：
 * - scaleIn: 从小到大
 * - scaleOut: 从大到小
 * - bounce: 弹跳缩放效果
 * - pulse: 脉冲缩放效果
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef SCALEANIMATOR_H
#define SCALEANIMATOR_H

#include "AnimationTypes.h"
#include <QObject>
#include <QPointer>
#include <memory>

class QWidget;
class QGraphicsEffect;
class QPropertyAnimation;

namespace WealthPilot {

/**
 * @brief 缩放动画器
 */
class ScaleAnimator : public QObject {
    Q_OBJECT

public:
    explicit ScaleAnimator(QObject* parent = nullptr);
    ~ScaleAnimator() override;

    /**
     * @brief 缩放进入动画
     * @param widget 目标控件
     * @param startScale 起始缩放比例（默认 0.0）
     * @param endScale 结束缩放比例（默认 1.0）
     * @param duration 持续时间
     */
    QPropertyAnimation* scaleIn(QWidget* widget,
                                 qreal startScale = 0.0,
                                 qreal endScale = 1.0,
                                 int duration = 300);
    
    /**
     * @brief 缩放退出动画
     */
    QPropertyAnimation* scaleOut(QWidget* widget,
                                  qreal startScale = 1.0,
                                  qreal endScale = 0.0,
                                  int duration = 300);
    
    /**
     * @brief 弹跳效果
     */
    QPropertyAnimation* bounce(QWidget* widget,
                                qreal scale = 1.1,
                                int duration = 200);
    
    /**
     * @brief 脉冲效果（缩放后恢复）
     */
    QPropertyAnimation* pulse(QWidget* widget,
                               qreal maxScale = 1.05,
                               int duration = 150);
    
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

private:
    void setupScaleEffect(QWidget* widget);
    
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace WealthPilot

#endif // SCALEANIMATOR_H