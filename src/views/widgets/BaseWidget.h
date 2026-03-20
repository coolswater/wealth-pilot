#ifndef BASEWIDGET_H
#define BASEWIDGET_H

#include <QWidget>
#include <QPropertyAnimation>
#include <memory>

/**
 * @file BaseWidget.h
 * @brief 基础控件类 - 所有自定义控件的基类
 *
 * @details 本类提供：
 * - 统一的动画属性支持（缩放、旋转、透明度）
 * - 常用动画效果的封装（淡入淡出、滑动、缩放）
 * - 脉冲和抖动等特效动画
 * - 悬停事件的统一处理
 *
 * @note 所有动画通过AnimationManager统一管理，确保性能
 * @note 使用PIMPL模式隐藏实现细节
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */
class BaseWidget : public QWidget
{
    Q_OBJECT
    // 声明动画属性，支持QPropertyAnimation
    Q_PROPERTY(qreal scale READ scale WRITE setScale NOTIFY scaleChanged)
    Q_PROPERTY(qreal rotation READ rotation WRITE setRotation NOTIFY rotationChanged)
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity NOTIFY opacityChanged)

public:
    /**
     * @brief 构造函数
     * @param parent 父控件
     */
    explicit BaseWidget(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~BaseWidget();

    // ========== 动画属性 ==========

    /**
     * @brief 获取缩放值
     * @return qreal 当前缩放值（1.0为原始大小）
     */
    qreal scale() const;

    /**
     * @brief 设置缩放值
     * @param scale 缩放值（0.5=缩小一半，2.0=放大两倍）
     *
     * @note 会触发重绘，建议在动画中使用
     */
    void setScale(qreal scale);

    /**
     * @brief 获取旋转角度
     * @return qreal 当前旋转角度（度）
     */
    qreal rotation() const;

    /**
     * @brief 设置旋转角度
     * @param rotation 旋转角度（度，正值为顺时针）
     */
    void setRotation(qreal rotation);

    /**
     * @brief 获取透明度
     * @return qreal 当前透明度（0.0=完全透明，1.0=完全不透明）
     */
    qreal opacity() const;

    /**
     * @brief 设置透明度
     * @param opacity 透明度值
     *
     * @details 自动创建QGraphicsOpacityEffect
     */
    void setOpacity(qreal opacity);

    // ========== 基础动画 ==========

    /**
     * @brief 淡入动画
     * @param duration 动画持续时间（毫秒），默认300ms
     */
    void fadeIn(int duration = 300);

    /**
     * @brief 淡出动画
     * @param duration 动画持续时间（毫秒），默认300ms
     */
    void fadeOut(int duration = 300);

    /**
     * @brief 滑入动画
     * @param direction 滑入方向
     * @param duration 动画持续时间（毫秒），默认300ms
     */
    void slideIn(Qt::Edge direction, int duration = 300);

    /**
     * @brief 滑出动画
     * @param direction 滑出方向
     * @param duration 动画持续时间（毫秒），默认300ms
     */
    void slideOut(Qt::Edge direction, int duration = 300);

    /**
     * @brief 缩放进入动画
     * @param duration 动画持续时间（毫秒），默认300ms
     */
    void scaleIn(int duration = 300);

    /**
     * @brief 缩放退出动画
     * @param duration 动画持续时间（毫秒），默认300ms
     */
    void scaleOut(int duration = 300);

    // ========== 特效动画 ==========

    /**
     * @brief 脉冲动画
     * @param duration 单次脉冲周期（毫秒），默认1000ms
     *
     * @details 产生呼吸效果，无限循环直到调用stopPulse()
     */
    void pulse(int duration = 1000);

    /**
     * @brief 停止脉冲动画
     */
    void stopPulse();

    /**
     * @brief 抖动动画（用于错误提示）
     * @param duration 动画持续时间（毫秒），默认500ms
     *
     * @details 水平方向快速抖动，吸引用户注意
     */
    void shake(int duration = 500);

signals:
    /**
     * @brief 缩放值变化信号
     * @param scale 新的缩放值
     */
    void scaleChanged(qreal scale);

    /**
     * @brief 旋转角度变化信号
     * @param rotation 新的旋转角度
     */
    void rotationChanged(qreal rotation);

    /**
     * @brief 透明度变化信号
     * @param opacity 新的透明度
     */
    void opacityChanged(qreal opacity);

protected:
    /**
     * @brief 绘制事件
     * @param event 绘制事件对象
     *
     * @details 应用缩放和旋转变换后调用父类绘制
     */
    void paintEvent(QPaintEvent *event) override;

    /**
     * @brief 鼠标进入事件
     * @param event 进入事件对象
     */
    void enterEvent(QEnterEvent *event) override;

    /**
     * @brief 鼠标离开事件
     * @param event 离开事件对象
     */
    void leaveEvent(QEvent *event) override;

    /**
     * @brief 悬停进入处理（子类可重写）
     *
     * @details 默认空实现，子类可添加自定义悬停效果
     */
    virtual void onHoverEnter();

    /**
     * @brief 悬停离开处理（子类可重写）
     */
    virtual void onHoverLeave();

private:
    /**
     * @brief PIMPL实现结构体
     */
    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // BASEWIDGET_H
