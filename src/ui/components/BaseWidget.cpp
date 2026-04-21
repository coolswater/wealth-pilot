#include "BaseWidget.h"
#include "../../ui/animation/AnimationManager.h"
#include "../../utils/Logger.h"

#include <QPainter>
#include <QGraphicsOpacityEffect>
#include <QSequentialAnimationGroup>

/**
 * @file BaseWidget.cpp
 * @brief 基础控件实现
 *
 * @details 提供动画属性的实现和常用动画效�?
 * 所有变换通过QPainter在paintEvent中应�?
 */

/**
 * @struct BaseWidget::Impl
 * @brief PIMPL实现结构�?
 */
struct BaseWidget::Impl {
    qreal scale = 1.0;           ///< 当前缩放�?
    qreal rotation = 0.0;        ///< 当前旋转角度
    qreal opacity = 1.0;         ///< 当前透明�?
    QPropertyAnimation* pulseAnimation = nullptr;  ///< 脉冲动画对象
    QGraphicsOpacityEffect* opacityEffect = nullptr;  ///< 透明度效果对�?
};

/**
 * @brief 构造函�?
 *
 * @details 初始化PIMPL对象，启用鼠标跟�?
 */
BaseWidget::BaseWidget(QWidget *parent)
    : QWidget(parent)
    , d(std::make_unique<Impl>())
{
    // 启用鼠标跟踪，确保即使不按下鼠标也能接收移动事件
    setMouseTracking(true);
}

/**
 * @brief 析构函数
 *
 * @details 确保脉冲动画停止，避免悬空指�?
 */
BaseWidget::~BaseWidget() = default;

/**
 * @brief 获取缩放�?
 */
qreal BaseWidget::scale() const
{
    return d->scale;
}

/**
 * @brief 设置缩放�?
 * @param scale 新的缩放�?
 *
 * @details 更新缩放值并触发重绘
 */
void BaseWidget::setScale(qreal scale)
{
    if (qFuzzyCompare(d->scale, scale)) {
        return;  // 避免不必要的重绘
    }

    d->scale = scale;
    emit scaleChanged(scale);
    update();  // 触发重绘以应用新缩放
}

/**
 * @brief 获取旋转角度
 */
qreal BaseWidget::rotation() const
{
    return d->rotation;
}

/**
 * @brief 设置旋转角度
 * @param rotation 新的旋转角度
 */
void BaseWidget::setRotation(qreal rotation)
{
    if (qFuzzyCompare(d->rotation, rotation)) {
        return;
    }

    d->rotation = rotation;
    emit rotationChanged(rotation);
    update();
}

/**
 * @brief 获取透明�?
 */
qreal BaseWidget::opacity() const
{
    return d->opacity;
}

/**
 * @brief 设置透明�?
 * @param opacity 新的透明�?
 *
 * @details 延迟创建QGraphicsOpacityEffect，避免不必要的开销
 */
void BaseWidget::setOpacity(qreal opacity)
{
    if (qFuzzyCompare(d->opacity, opacity)) {
        return;
    }

    d->opacity = opacity;
    emit opacityChanged(opacity);

    // 延迟创建透明度效果
    if (!d->opacityEffect) {
        d->opacityEffect = new QGraphicsOpacityEffect(this);
        setGraphicsEffect(d->opacityEffect);
    }
    d->opacityEffect->setOpacity(opacity);
}

/**
 * @brief 淡入动画
 */
void BaseWidget::fadeIn(int duration)
{
    AnimationManager::instance()->fadeIn(this, duration);
}

/**
 * @brief 淡出动画
 */
void BaseWidget::fadeOut(int duration)
{
    AnimationManager::instance()->fadeOut(this, duration);
}

/**
 * @brief 滑入动画
 */
void BaseWidget::slideIn(Qt::Edge direction, int duration)
{
    AnimationManager::instance()->slideIn(this, direction, duration);
}

/**
 * @brief 滑出动画
 */
void BaseWidget::slideOut(Qt::Edge direction, int duration)
{
    AnimationManager::instance()->slideOut(this, direction, duration);
}

/**
 * @brief 缩放进入动画
 */
void BaseWidget::scaleIn(int duration)
{
    AnimationManager::instance()->scaleIn(this, duration);
}

/**
 * @brief 缩放退出动�?
 */
void BaseWidget::scaleOut(int duration)
{
    AnimationManager::instance()->scaleOut(this, duration);
}

/**
 * @brief 脉冲动画
 * @param duration 单次脉冲周期
 *
 * @details 创建呼吸效果动画，从1.0�?.05再回�?.0
 */
void BaseWidget::pulse(int duration)
{
    // 停止现有脉冲动画
    stopPulse();

    d->pulseAnimation = new QPropertyAnimation(this, "scale", this);
    d->pulseAnimation->setDuration(duration);
    d->pulseAnimation->setStartValue(1.0);
    d->pulseAnimation->setKeyValueAt(0.5, 1.05);  // 中点最�?
    d->pulseAnimation->setEndValue(1.0);
    d->pulseAnimation->setLoopCount(-1);  // 无限循环
    d->pulseAnimation->setEasingCurve(QEasingCurve::InOutSine);

    d->pulseAnimation->start();

    LOG_DEBUG("Pulse animation started");
}

/**
 * @brief 停止脉冲动画
 */
void BaseWidget::stopPulse()
{
    if (d->pulseAnimation) {
        d->pulseAnimation->stop();
        delete d->pulseAnimation;
        d->pulseAnimation = nullptr;
        setScale(1.0);  // 恢复原始大小
        LOG_DEBUG("Pulse animation stopped");
    }
}

/**
 * @brief 抖动动画（错误提示）
 * @param duration 动画持续时间
 *
 * @details 水平方向快速抖动，幅度逐渐减小
 */
void BaseWidget::shake(int duration)
{
    QSequentialAnimationGroup* group = new QSequentialAnimationGroup(this);

    // 计算每步持续时间
    int stepDuration = duration / 10;

    // Implementation
    int offsets[] = {-10, 10, -10, 10, -5, 5, -5, 5, 0};

    QPoint basePos = pos();

    for (int offset : offsets) {
        QPropertyAnimation* anim = new QPropertyAnimation(this, "pos", group);
        anim->setDuration(stepDuration);
        anim->setStartValue(pos());
        anim->setEndValue(basePos + QPoint(offset, 0));
        anim->setEasingCurve(QEasingCurve::InOutQuad);
        group->addAnimation(anim);
    }

    // 动画完成后确保回到原
    connect(group, &QSequentialAnimationGroup::finished, this, [this, basePos]() {
        move(basePos);
    });

    group->start(QAbstractAnimation::DeleteWhenStopped);

    LOG_DEBUG("Shake animation started");
}

/**
 * @brief 绘制事件
 * @param event 绘制事件对象
 *
 * @details 应用缩放和旋转变换后调用父类绘制
 * 使用QPainter的save/restore确保变换不影响其他绘�?
 */
void BaseWidget::paintEvent(QPaintEvent *event)
{
    // 如果有变换，应用变换矩阵
    if (d->scale != 1.0 || d->rotation != 0.0) {
        QPainter painter(this);
        painter.save();

        // 以控件中心为变换原点
        QPoint center = rect().center();
        painter.translate(center);
        painter.scale(d->scale, d->scale);
        painter.rotate(d->rotation);
        painter.translate(-center);

        // 调用父类绘制
        QWidget::paintEvent(event);

        painter.restore();
    } else {
        // 无变换，直接调用父类
        QWidget::paintEvent(event);
    }
}

/**
 * @brief 鼠标进入事件
 */
void BaseWidget::enterEvent(QEnterEvent *event)
{
    QWidget::enterEvent(event);
    onHoverEnter();
}

/**
 * @brief 鼠标离开事件
 */
void BaseWidget::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
    onHoverLeave();
}

/**
 * @brief 悬停进入处理
 *
 * @details 默认空实现，子类可重写添加自定义效果
 */
void BaseWidget::onHoverEnter()
{
    // 子类重写
}

/**
 * @brief 悬停离开处理
 */
void BaseWidget::onHoverLeave()
{
    // 子类重写
}
