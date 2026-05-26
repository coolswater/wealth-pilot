/**
 * @file FadeAnimator.h
 * @brief 淡入淡出动画器
 *
 * @details 专门处理透明度动画：
 * - fadeIn: 从透明到不透明
 * - fadeOut: 从不透明到透明
 * - crossFade: 交叉淡入淡出
 * - fadeInOut: 淡入后淡出
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef FADEANIMATOR_H
#define FADEANIMATOR_H

#include "AnimationTypes.h"
#include <QObject>
#include <QPointer>
#include <memory>

class QWidget;
class QGraphicsOpacityEffect;
class QPropertyAnimation;

namespace WealthPilot {

/**
 * @brief 淡入淡出动画器
 */
class FadeAnimator : public QObject {
    Q_OBJECT

public:
    explicit FadeAnimator(QObject* parent = nullptr);
    ~FadeAnimator() override;

    /**
     * @brief 淡入动画
     * @param widget 目标控件
     * @param config 动画配置
     * @return 动画对象
     */
    QPropertyAnimation* fadeIn(QWidget* widget, 
                                const AnimationConfig& config = AnimationConfig::fadeIn());
    
    /**
     * @brief 淡出动画
     */
    QPropertyAnimation* fadeOut(QWidget* widget,
                                 const AnimationConfig& config = AnimationConfig::fadeOut());
    
    /**
     * @brief 交叉淡入淡出（两个控件）
     */
    void crossFade(QWidget* fromWidget, QWidget* toWidget, 
                   int duration = 300);
    
    /**
     * @brief 淡入后淡出
     */
    QPropertyAnimation* fadeInOut(QWidget* widget,
                                   int fadeInDuration = 200,
                                   int fadeOutDuration = 200,
                                   int stayDuration = 1000);
    
    /**
     * @brief 停止动画
     */
    void stop(QWidget* widget);
    
    /**
     * @brief 停止所有动画
     */
    void stopAll();

signals:
    /**
     * @brief 动画开始信号
     */
    void animationStarted(QWidget* widget);
    
    /**
     * @brief 动画完成信号
     */
    void animationFinished(QWidget* widget);

private:
    void setupEffect(QWidget* widget);
    void cleanupEffect(QWidget* widget);
    
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace WealthPilot

#endif // FADEANIMATOR_H
