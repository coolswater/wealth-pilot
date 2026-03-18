/**
 * @file AnimationManager.h
 * @brief Qt动画管理器头文件 - 高可用高性能版本 (修复版)
 *
 * @version 1.0.0
 * @author 系统架构组
 */

#ifndef ANIMATIONMANAGER_H
#define ANIMATIONMANAGER_H

#include <QObject>
#include <QWidget>
#include <QPropertyAnimation>
#include <QByteArray>
#include <QEasingCurve>
#include <QPointer>
#include <memory>

// 前置声明
class QGraphicsOpacityEffect;
class QGraphicsDropShadowEffect;
class QPushButton;
class QLabel;
class QProgressBar;

/**
 * @class AnimationManager
 * @brief 动画管理器类 - 线程安全的高性能动画管理
 *
 * @warning 所有方法必须在 Qt 主线程（GUI 线程）中调用
 */
class AnimationManager : public QObject
{
    Q_OBJECT

private:
    explicit AnimationManager(QObject *parent = nullptr);
    ~AnimationManager() override;

public:
    // 禁止拷贝和赋值
    AnimationManager(const AnimationManager&) = delete;
    AnimationManager& operator=(const AnimationManager&) = delete;

    /**
     * @brief 获取单例实例（线程安全）
     * @return AnimationManager* 单例实例指针，始终非空
     */
    static AnimationManager* instance();

    /**
     * @brief 初始化动画管理器
     * @details 预创建对象池，建议在 main 函数中调用
     */
    void initialize();

    // ==================== 动画创建与基础管理 ====================

    /**
     * @brief 创建或从对象池获取属性动画对象
     * @param target 动画目标对象
     * @param property 要动画化的属性名称
     * @return QPropertyAnimation* 动画对象指针，失败返回 nullptr
     * @note 返回的动画对象**禁止**设置 DeleteWhenStopped，由对象池管理生命周期
     */
    QPropertyAnimation* createAnimation(QObject* target, const QByteArray& property);

    /**
     * @brief 停止指定对象的所有动画
     * @param target 目标对象
     */
    void stopAnimation(QObject* target);

    /**
     * @brief 停止指定对象指定属性的动画
     * @param target 目标对象
     * @param property 属性名称
     */
    void stopAnimation(QObject* target, const QByteArray& property);

    /**
     * @brief 停止所有动画
     */
    void stopAllAnimations();

    /**
     * @brief 设置全局动画启用状态
     * @param enabled 是否启用动画
     */
    void setAnimationsEnabled(bool enabled);

    /**
     * @brief 获取全局动画启用状态
     * @return bool 动画是否启用
     */
    bool animationsEnabled() const;

    // ==================== 基础动画效果 ====================

    /**
     * @brief 淡入动画
     * @param widget 要淡入显示的控件
     * @param duration 动画时长（毫秒），0表示使用默认时长(200ms)
     * @return QPropertyAnimation* 动画对象指针
     */
    QPropertyAnimation* fadeIn(QWidget* widget, int duration = 0);

    /**
     * @brief 淡出动画
     * @param widget 要淡出隐藏的控件
     * @param duration 动画时长（毫秒），0表示使用默认时长(200ms)
     * @return QPropertyAnimation* 动画对象指针
     */
    QPropertyAnimation* fadeOut(QWidget* widget, int duration = 0);

    /**
     * @brief 滑入动画
     * @param widget 要滑入显示的控件
     * @param direction 滑动方向
     * @param duration 动画时长（毫秒），0表示使用默认时长(250ms)
     * @return QPropertyAnimation* 动画对象指针
     */
    QPropertyAnimation* slideIn(QWidget* widget, Qt::Edge direction, int duration = 0);

    /**
     * @brief 滑出动画
     * @param widget 要滑出隐藏的控件
     * @param direction 滑动方向
     * @param duration 动画时长（毫秒），0表示使用默认时长(250ms)
     * @return QPropertyAnimation* 动画对象指针
     */
    QPropertyAnimation* slideOut(QWidget* widget, Qt::Edge direction, int duration = 0);

    /**
     * @brief 缩放进入动画
     * @param widget 要缩放显示的控件
     * @param duration 动画时长（毫秒），0表示使用默认时长(300ms)
     * @return QPropertyAnimation* 动画对象指针
     * @warning 不适用于受布局管理的控件
     */
    QPropertyAnimation* scaleIn(QWidget* widget, int duration = 0);

    /**
     * @brief 缩放退出动画
     * @param widget 要缩放隐藏的控件
     * @param duration 动画时长（毫秒），0表示使用默认时长(300ms)
     * @return QPropertyAnimation* 动画对象指针
     */
    QPropertyAnimation* scaleOut(QWidget* widget, int duration = 0);

    // ==================== 控件交互动画 ====================

    void cardHoverEnter(QWidget* card);
    void cardHoverLeave(QWidget* card);
    void cardPress(QWidget* card);
    void cardRelease(QWidget* card);

    void buttonHoverEnter(QPushButton* button);
    void buttonHoverLeave(QPushButton* button);
    void buttonPress(QPushButton* button);
    void buttonRelease(QPushButton* button);

    // ==================== 数值动画 ====================

    void animateNumber(QLabel* label, int startValue, int endValue, int duration = 500);
    void animateNumber(QLabel* label, double startValue, double endValue, int duration = 500);
    void animateProgress(QProgressBar* progressBar, int startValue, int endValue, int duration = 300);

    // ==================== 布局与列表动画 ====================

    void pageTransition(QWidget* fromPage, QWidget* toPage, Qt::Edge direction);
    void listItemInsert(QWidget* item, int index);
    void listItemRemove(QWidget* item);
    void listItemReorder(QWidget* item, int newIndex);

    /**
     * @brief 获取当前活跃动画数量
     * @return int 活跃动画数量
     */
    int activeAnimationCount() const;

    /**
     * @brief 获取动画管理器版本
     * @return QString 版本字符串
     */
    QString version() const;

private slots:
    /**
     * @brief 延迟回收动画对象
     * @param animation 要回收的动画对象（使用 QPointer 包装确保安全）
     */
    void delayedReturnAnimation(QPointer<QPropertyAnimation> animation);

private:
    // PIMPL实现结构体
    struct Impl;
    std::unique_ptr<Impl> d;

    // 静态单例实例指针
    static AnimationManager* s_instance;

    /**
     * @brief 将动画对象归还到对象池
     * @param animation 要归还的动画对象
     */
    void returnAnimation(QPropertyAnimation* animation);

    /**
     * @brief 获取或创建透明度效果对象
     * @param widget 目标控件
     * @return QGraphicsOpacityEffect* 效果对象指针
     * @note 效果对象不归控件所有，由对象池管理
     */
    QGraphicsOpacityEffect* getOrCreateOpacityEffect(QWidget* widget);

    /**
     * @brief 获取或创建阴影效果对象
     * @param widget 目标控件
     * @return QGraphicsDropShadowEffect* 效果对象指针
     */
    QGraphicsDropShadowEffect* getOrCreateShadowEffect(QWidget* widget);

    /**
     * @brief 安全移除控件的所有动画效果
     * @param widget 目标控件（使用 QPointer 包装）
     * @param delayMs 延迟移除时间（毫秒）
     */
    void removeEffects(QPointer<QWidget> widget, int delayMs = 0);

    /**
     * @brief 检查是否在主线程运行
     * @return bool 是否在主线程
     */
    static bool isMainThread();

    /**
     * @brief 生成动画键（字符串格式）
     * @param target 目标对象
     * @param property 属性名
     * @return QString 动画键字符串
     */
    static QString makeAnimationKey(QObject* target, const QByteArray& property);
};

#endif // ANIMATIONMANAGER_H
