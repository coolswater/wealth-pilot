/**
 * @file AnimationManager.cpp
 * @brief Qt动画管理器实现 - 高可用高性能版本 (修复版)
 *
 * @details
 * 1. 移除所有 DeleteWhenStopped，避免与对象池冲突
 * 2. 使用 QPointer 包装所有 QObject 指针，防止野指针
 * 3. 修复 QGraphicsEffect 生命周期：效果不归控件所有，避免双重释放
 * 4. 添加强制线程检查，非主线程调用直接返回并警告
 * 5. 优化 lambda 捕获，确保对象存在时才访问
 *
 * @version 1.0.0
 */

#include "AnimationManager.h"

#include <QGraphicsOpacityEffect>
#include <QGraphicsDropShadowEffect>
#include <QParallelAnimationGroup>
#include <QSequentialAnimationGroup>
#include <QTimer>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QThread>
#include <QVariantAnimation>
#include <QMetaObject>
#include <QPointer>
#include <QCoreApplication>
#include <QDebug>

// 静态单例实例指针初始化
AnimationManager* AnimationManager::s_instance = nullptr;

/**
 * @struct AnimationManager::Impl
 * @brief PIMPL模式实现结构体
 */
struct AnimationManager::Impl {
    // ========== 对象池 ==========
    QMap<QByteArray, QList<QPropertyAnimation*>> animationPool;
    QList<QGraphicsOpacityEffect*> opacityEffectPool;
    QList<QGraphicsDropShadowEffect*> shadowEffectPool;

    // ========== 活跃对象跟踪 ==========
    // 使用 QPointer 自动置空，防止悬空指针
    QHash<QString, QPointer<QPropertyAnimation>> activeAnimations;
    QMap<QWidget*, QPointer<QGraphicsEffect>> widgetEffects;
    QMap<QPointer<QPushButton>, QString> originalButtonStyles;
    QMap<QPointer<QWidget>, QRect> originalGeometryCache;

    // ========== 配置参数 ==========
    bool animationsEnabled = true;
    int maxAnimationPoolSize = 20;
    int maxEffectPoolSize = 10;
    QEasingCurve defaultEasing = QEasingCurve::OutCubic;

    int defaultFadeDuration = 200;
    int defaultSlideDuration = 250;
    int defaultScaleDuration = 300;
    int defaultButtonAnimDuration = 150;
    int defaultCardPressDuration = 100;

    int defaultShadowBlurRadius = 20;
    QColor defaultShadowColor = QColor(0, 0, 0, 80);
    QPointF defaultShadowOffsetHover = QPointF(0, 4);

    QString versionString = "3.2.0-fix";

    // ========== 内部方法 ==========

    /**
     * @brief 检查动画是否活跃
     * @param animationKey 动画键字符串
     * @return bool 是否有活跃动画
     */
    bool hasActiveAnimation(const QString& animationKey) {
        auto it = activeAnimations.find(animationKey);
        if (it != activeAnimations.end()) {
            QPropertyAnimation* anim = it.value();
            // 使用 QPointer 检查有效性，并检查状态
            return anim && anim->state() != QAbstractAnimation::Stopped;
        }
        return false;
    }

    /**
     * @brief 获取活跃动画
     */
    QPropertyAnimation* getActiveAnimation(const QString& animationKey) {
        QPointer<QPropertyAnimation> ptr = activeAnimations.value(animationKey);
        return ptr; // QPointer 自动转换为原始指针，如果已销毁则为 nullptr
    }

    /**
     * @brief 注册活跃动画
     */
    void registerAnimation(const QString& animationKey, QPropertyAnimation* animation) {
        if (animation) {
            activeAnimations[animationKey] = animation;
        }
    }

    /**
     * @brief 注销活跃动画
     */
    void unregisterAnimation(const QString& animationKey) {
        activeAnimations.remove(animationKey);
    }

    /**
     * @brief 从对象池获取透明度效果
     * @note 效果对象不归控件所有，由 AnimationManager 管理
     */
    QGraphicsOpacityEffect* takeOpacityEffectFromPool() {
        while (!opacityEffectPool.isEmpty()) {
            QGraphicsOpacityEffect* effect = opacityEffectPool.takeFirst();
            if (effect) {
                effect->setOpacity(1.0);
                // 确保效果没有父对象，避免被意外销毁
                if (effect->parent()) {
                    effect->setParent(nullptr);
                }
                return effect;
            }
        }
        return nullptr;
    }

    /**
     * @brief 从对象池获取阴影效果
     */
    QGraphicsDropShadowEffect* takeShadowEffectFromPool() {
        while (!shadowEffectPool.isEmpty()) {
            QGraphicsDropShadowEffect* effect = shadowEffectPool.takeFirst();
            if (effect) {
                effect->setBlurRadius(0);
                effect->setOffset(0, 0);
                if (effect->parent()) {
                    effect->setParent(nullptr);
                }
                return effect;
            }
        }
        return nullptr;
    }

    /**
     * @brief 将透明度效果归还到对象池
     */
    void returnOpacityEffectToPool(QGraphicsOpacityEffect* effect) {
        if (!effect) return;

        // 断开所有信号槽连接，防止残留连接导致崩溃
        effect->disconnect();

        // 【关键修复】确保效果被禁用，防止残留绘制状态
        effect->setEnabled(false);

        // 重置为默认状态
        effect->setOpacity(1.0);
        effect->setOpacityMask(QBrush());

        // 从原父对象分离
        if (effect->parent()) {
            effect->setParent(nullptr);
        }

        if (opacityEffectPool.size() < maxEffectPoolSize) {
            opacityEffectPool.append(effect);
        } else {
            delete effect;
        }
    }

    /**
     * @brief 将阴影效果归还到对象池
     */
    void returnShadowEffectToPool(QGraphicsDropShadowEffect* effect) {
        if (!effect) return;

        effect->disconnect();

        // 【关键修复】确保效果被禁用
        effect->setEnabled(false);

        // 重置为默认状态
        effect->setBlurRadius(0);
        effect->setOffset(0, 0);
        effect->setColor(QColor(0, 0, 0, 80)); // 使用默认阴影颜色

        if (effect->parent()) {
            effect->setParent(nullptr);
        }

        if (shadowEffectPool.size() < maxEffectPoolSize) {
            shadowEffectPool.append(effect);
        } else {
            delete effect;
        }
    }

    /**
     * @brief 缓存控件原始尺寸
     */
    void cacheOriginalGeometry(QWidget* widget) {
        if (widget && !originalGeometryCache.contains(widget)) {
            originalGeometryCache[widget] = widget->geometry();
        }
    }

    /**
     * @brief 获取缓存的控件原始尺寸
     */
    QRect getCachedOriginalGeometry(QWidget* widget) {
        return originalGeometryCache.value(widget, QRect());
    }

    /**
     * @brief 清除控件尺寸缓存
     */
    void clearGeometryCache(QWidget* widget) {
        originalGeometryCache.remove(widget);
    }

    /**
     * @brief 清理已销毁对象的缓存条目
     */
    void cleanupExpiredEntries() {
        // 清理按钮样式缓存中已销毁的按钮
        auto it = originalButtonStyles.begin();
        while (it != originalButtonStyles.end()) {
            if (!it.key()) {
                it = originalButtonStyles.erase(it);
            } else {
                ++it;
            }
        }

        // 清理几何缓存中已销毁的控件
        auto geoIt = originalGeometryCache.begin();
        while (geoIt != originalGeometryCache.end()) {
            if (!geoIt.key()) {
                geoIt = originalGeometryCache.erase(geoIt);
            } else {
                ++geoIt;
            }
        }
    }
};

/**
 * @brief 生成动画键（字符串格式）
 */
QString AnimationManager::makeAnimationKey(QObject* target, const QByteArray& property) {
    if (!target || property.isEmpty()) return QString();
    // 使用 quintptr 确保 64 位安全
    return QString("%1_%2").arg(reinterpret_cast<quintptr>(target)).arg(QString(property));
}

/**
 * @brief 构造函数
 */
AnimationManager::AnimationManager(QObject *parent)
    : QObject(parent)
    , d(std::make_unique<Impl>())
{
    s_instance = this;
}

/**
 * @brief 析构函数
 */
AnimationManager::~AnimationManager()
{
    qDebug() << "AnimationManager cleaning up...";

    // 停止所有活跃动画
    stopAllAnimations();

    // 清理动画对象池
    for (auto& list : d->animationPool) {
        for (QPropertyAnimation* anim : list) {
            if (anim) {
                anim->disconnect();
                delete anim;
            }
        }
    }
    d->animationPool.clear();

    // 清理效果对象池
    for (QGraphicsOpacityEffect* effect : std::as_const(d->opacityEffectPool)) {
        if (effect) delete effect;
    }
    d->opacityEffectPool.clear();

    for (QGraphicsDropShadowEffect* effect : std::as_const(d->shadowEffectPool)) {
        if (effect) delete effect;
    }
    d->shadowEffectPool.clear();

    s_instance = nullptr;
    qDebug() << "AnimationManager destroyed";
}

/**
 * @brief 获取动画管理器单例实例
 */
AnimationManager* AnimationManager::instance()
{
    if (!s_instance) {
        // 确保在主线程创建
        if (QThread::currentThread() != QCoreApplication::instance()->thread()) {
            qWarning() << "AnimationManager::instance() called from non-main thread!";
            // 通过元对象系统排队到主线程创建，但此时返回 nullptr，调用方需要处理
            QMetaObject::invokeMethod(QCoreApplication::instance(), []() {
                AnimationManager::instance();
            }, Qt::QueuedConnection);
            return nullptr;
        }
        s_instance = new AnimationManager();
    }
    return s_instance;
}

QString AnimationManager::version() const
{
    return d->versionString;
}

bool AnimationManager::isMainThread()
{
    return QThread::currentThread() == QCoreApplication::instance()->thread();
}

/**
 * @brief 初始化动画管理器
 */
void AnimationManager::initialize()
{
    // 强制主线程执行
    if (!isMainThread()) {
        QMetaObject::invokeMethod(this, &AnimationManager::initialize, Qt::QueuedConnection);
        return;
    }

    qDebug() << "Initializing AnimationManager object pools...";

    // 预创建常用属性的动画对象
    const QByteArray commonProps[] = {"opacity", "pos", "geometry", "blurRadius", "offset", "value"};
    for (const QByteArray& prop : commonProps) {
        QList<QPropertyAnimation*>& pool = d->animationPool[prop];
        for (int i = pool.size(); i < 5; ++i) {
            QPropertyAnimation* anim = new QPropertyAnimation(this); // 父对象为 Manager，确保不泄露
            anim->setPropertyName(prop);
            pool.append(anim);
        }
    }

    // 预创建效果对象
    for (int i = d->opacityEffectPool.size(); i < 3; ++i) {
        d->opacityEffectPool.append(new QGraphicsOpacityEffect(this));
    }
    for (int i = d->shadowEffectPool.size(); i < 3; ++i) {
        QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect(this);
        effect->setBlurRadius(0);
        effect->setColor(d->defaultShadowColor);
        effect->setOffset(0, 0);
        d->shadowEffectPool.append(effect);
    }

    qDebug() << "AnimationManager initialized with pools ready";
}

/**
 * @brief 获取或创建透明度效果对象
 * @warning 返回的效果对象**不归 widget 所有**，禁止手动 delete
 */
QGraphicsOpacityEffect* AnimationManager::getOrCreateOpacityEffect(QWidget* widget)
{
    if (!widget) return nullptr;

    // 检查是否已有透明度效果
    QGraphicsEffect* existing = widget->graphicsEffect();
    if (existing) {
        // 如果已经是透明度效果，直接返回
        if (QGraphicsOpacityEffect* opacityEffect = qobject_cast<QGraphicsOpacityEffect*>(existing)) {
            return opacityEffect;
        }
        // 【关键修复】如果有其他效果，先禁用再移除，避免绘制冲突
        existing->setEnabled(false);
        widget->setGraphicsEffect(nullptr);
    }

    // 从对象池获取或创建新效果
    QGraphicsOpacityEffect* effect = d->takeOpacityEffectFromPool();
    if (!effect) {
        effect = new QGraphicsOpacityEffect();
        // 不归 widget 所有，而是作为 AnimationManager 的子对象或独立对象
        effect->setParent(this);
    }

    effect->setOpacity(1.0);
    widget->setGraphicsEffect(effect);

    // 记录映射关系
    d->widgetEffects[widget] = effect;

    return effect;
}

/**
 * @brief 获取或创建阴影效果对象
 */
QGraphicsDropShadowEffect* AnimationManager::getOrCreateShadowEffect(QWidget* widget)
{
    if (!widget) return nullptr;

    QGraphicsEffect* existing = widget->graphicsEffect();
    if (existing) {
        if (QGraphicsDropShadowEffect* shadowEffect = qobject_cast<QGraphicsDropShadowEffect*>(existing)) {
            return shadowEffect;
        }
        // 【关键修复】先禁用再移除
        existing->setEnabled(false);
        widget->setGraphicsEffect(nullptr);
    }

    QGraphicsDropShadowEffect* effect = d->takeShadowEffectFromPool();
    if (!effect) {
        effect = new QGraphicsDropShadowEffect();
        effect->setColor(d->defaultShadowColor);
        effect->setParent(this); // 归 Manager 所有
    }

    effect->setBlurRadius(0);
    effect->setOffset(0, 0);
    widget->setGraphicsEffect(effect);

    d->widgetEffects[widget] = effect;

    return effect;
}

/**
 * @brief 安全移除控件的所有动画效果
 */
void AnimationManager::removeEffects(QPointer<QWidget> widget, int delayMs)
{
    // 使用 QPointer 确保 widget 未被销毁
    if (!widget) return;

    auto doRemove = [this, widget]() {
        if (!widget) return;

        QGraphicsEffect* effect = widget->graphicsEffect();
        if (!effect) return;

        // 【关键修复】先禁用效果，确保 Qt 绘制引擎停止对此 effect 的调用
        effect->setEnabled(false);

        // 检查是否有活跃动画在该效果上
        QString opacityKey = makeAnimationKey(effect, "opacity");
        QString blurKey = makeAnimationKey(effect, "blurRadius");
        QString offsetKey = makeAnimationKey(effect, "offset");

        if (d->hasActiveAnimation(opacityKey) ||
            d->hasActiveAnimation(blurKey) ||
            d->hasActiveAnimation(offsetKey)) {
            // 有活跃动画，延迟后重试
            QTimer::singleShot(50, this, [this, widget]() {
                removeEffects(widget, 0);
            });
            return;
        }

        // 停止效果上的所有动画
        stopAnimation(effect);

        // 从控件上移除效果
        widget->setGraphicsEffect(nullptr);

        // 归还到对象池
        if (QGraphicsOpacityEffect* opacityEffect = qobject_cast<QGraphicsOpacityEffect*>(effect)) {
            d->returnOpacityEffectToPool(opacityEffect);
        } else if (QGraphicsDropShadowEffect* shadowEffect = qobject_cast<QGraphicsDropShadowEffect*>(effect)) {
            d->returnShadowEffectToPool(shadowEffect);
        } else {
            delete effect;
        }

        d->widgetEffects.remove(widget);
    };

    if (delayMs > 0) {
        QTimer::singleShot(delayMs, this, doRemove);
    } else {
        doRemove();
    }
}

/**
 * @brief 创建或从对象池获取属性动画对象
 * @note 返回的动画对象**禁止**设置 DeleteWhenStopped，由对象池管理
 */
QPropertyAnimation* AnimationManager::createAnimation(QObject* target, const QByteArray& property)
{
    if (!target || !d->animationsEnabled) {
        return nullptr;
    }

    // 强制主线程检查
    if (!isMainThread()) {
        qWarning() << "AnimationManager::createAnimation must be called from main thread!";
        return nullptr;
    }

    QString animationKey = makeAnimationKey(target, property);
    if (animationKey.isEmpty()) {
        return nullptr;
    }

    // 检查并停止同一属性的现有动画
    QPropertyAnimation* existingAnim = d->getActiveAnimation(animationKey);
    if (existingAnim && existingAnim->state() != QAbstractAnimation::Stopped) {
        existingAnim->stop();
        // stop 会触发 finished 信号，进而触发回收，但这里我们立即复用
    }

    // 从对象池获取动画对象
    QList<QPropertyAnimation*>& pool = d->animationPool[property];
    QPropertyAnimation* animation = nullptr;

    if (!pool.isEmpty()) {
        animation = pool.takeFirst();
        animation->setTargetObject(target);
        animation->setPropertyName(property);
        // 清除之前的连接
        animation->disconnect();
    } else {
        animation = new QPropertyAnimation(target, property, this);
    }

    // 注册到活跃动画映射表（使用 QPointer）
    d->registerAnimation(animationKey, animation);

    // 连接动画完成信号 - 使用 QPointer 包装 animation 防止 lambda 中访问已销毁对象
    QPointer<QPropertyAnimation> animPtr(animation);
    connect(animation, &QPropertyAnimation::finished, this, [this, animPtr, target, property]() {
        if (!animPtr) return; // 安全检查

        QString animKey = makeAnimationKey(target, property);
        if (!animKey.isEmpty()) {
            d->unregisterAnimation(animKey);
        }
        // 延迟回收
        delayedReturnAnimation(animPtr);
    });

    return animation;
}

/**
 * @brief 延迟回收动画对象
 */
void AnimationManager::delayedReturnAnimation(QPointer<QPropertyAnimation> animation)
{
    if (!animation) return;

    // 延迟一帧后回收，确保所有信号处理完成
    QTimer::singleShot(0, this, [this, animation]() {
        if (animation) {
            returnAnimation(animation);
        }
    });
}

/**
 * @brief 将动画对象归还到对象池
 */
void AnimationManager::returnAnimation(QPropertyAnimation* animation)
{
    if (!animation) return;

    // 强制主线程
    if (!isMainThread()) {
        QMetaObject::invokeMethod(this, [this, animation]() {
            returnAnimation(animation);
        }, Qt::QueuedConnection);
        return;
    }

    // 确保动画已停止
    if (animation->state() != QAbstractAnimation::Stopped) {
        animation->stop();
    }

    // 断开所有信号连接
    animation->disconnect();

    // 解除目标对象引用
    animation->setTargetObject(nullptr);

    QByteArray property = animation->propertyName();
    QList<QPropertyAnimation*>& pool = d->animationPool[property];

    if (pool.size() < d->maxAnimationPoolSize) {
        pool.append(animation);
    } else {
        delete animation;
    }
}

/**
 * @brief 执行淡入动画
 */
QPropertyAnimation* AnimationManager::fadeIn(QWidget* widget, int duration)
{
    if (!widget || !d->animationsEnabled) {
        if (widget) widget->setVisible(true);
        return nullptr;
    }

    // 停止该控件现有的透明度动画
    QGraphicsEffect* existingEffect = widget->graphicsEffect();
    if (existingEffect) {
        stopAnimation(existingEffect, "opacity");
    }

    QGraphicsOpacityEffect* effect = getOrCreateOpacityEffect(widget);
    if (!effect) {
        widget->setVisible(true);
        return nullptr;
    }

    // 设置起始透明度
    effect->setOpacity(0.0);
    widget->setVisible(true);

    // 创建透明度属性动画
    QPropertyAnimation* animation = createAnimation(effect, "opacity");
    if (!animation) {
        effect->setOpacity(1.0);
        return nullptr;
    }

    animation->setStartValue(0.0);
    animation->setEndValue(1.0);
    animation->setDuration(duration > 0 ? duration : d->defaultFadeDuration);
    animation->setEasingCurve(d->defaultEasing);

    // 重要：不使用 DeleteWhenStopped，由对象池管理生命周期
    animation->start();

    return animation;
}

/**
 * @brief 执行淡出动画
 */
QPropertyAnimation* AnimationManager::fadeOut(QWidget* widget, int duration)
{
    if (!widget || !d->animationsEnabled) {
        if (widget) widget->setVisible(false);
        return nullptr;
    }

    // 停止该控件现有的透明度动画
    QGraphicsEffect* existingEffect = widget->graphicsEffect();
    if (existingEffect) {
        stopAnimation(existingEffect, "opacity");
    }

    QGraphicsOpacityEffect* effect = getOrCreateOpacityEffect(widget);
    if (!effect) {
        widget->setVisible(false);
        return nullptr;
    }

    effect->setOpacity(1.0);

    QPropertyAnimation* animation = createAnimation(effect, "opacity");
    if (!animation) {
        widget->setVisible(false);
        return nullptr;
    }

    animation->setStartValue(1.0);
    animation->setEndValue(0.0);
    animation->setDuration(duration > 0 ? duration : d->defaultFadeDuration);
    animation->setEasingCurve(d->defaultEasing);

    // 使用 QPointer 捕获 widget 和 effect，防止对象已销毁
    QPointer<QWidget> widgetPtr(widget);
    QPointer<QGraphicsOpacityEffect> effectPtr(effect);

    connect(animation, &QPropertyAnimation::finished, this, [this, widgetPtr, effectPtr]() {
        // 【关键修复】先禁用效果，防止在隐藏过程中绘制
        if (effectPtr) {
            effectPtr->setEnabled(false);
        }

        if (widgetPtr) {
            // 使用 update() 确保任何待处理的绘制事件先完成
            widgetPtr->update();

            // 使用 QMetaObject::invokeMethod 延迟到事件循环下一轮执行移除和隐藏
            QMetaObject::invokeMethod(this, [this, widgetPtr]() {
                if (widgetPtr) {
                    removeEffects(widgetPtr, 0);
                    widgetPtr->setVisible(false);
                }
            }, Qt::QueuedConnection);
        }
    });

    animation->start();
    return animation;
}

/**
 * @brief 执行滑入动画
 */
QPropertyAnimation* AnimationManager::slideIn(QWidget* widget, Qt::Edge direction, int duration)
{
    if (!widget || !d->animationsEnabled) {
        if (widget) widget->setVisible(true);
        return nullptr;
    }

    stopAnimation(widget, "pos");

    QPoint startPos = widget->pos();
    QPoint endPos = startPos;

    // 根据滑动方向计算起始位置
    switch (direction) {
    case Qt::LeftEdge:
        startPos.setX(startPos.x() - widget->width());
        break;
    case Qt::RightEdge:
        startPos.setX(startPos.x() + widget->width());
        break;
    case Qt::TopEdge:
        startPos.setY(startPos.y() - widget->height());
        break;
    case Qt::BottomEdge:
        startPos.setY(startPos.y() + widget->height());
        break;
    }

    widget->move(startPos);
    widget->setVisible(true);

    QPropertyAnimation* animation = createAnimation(widget, "pos");
    if (!animation) {
        widget->move(endPos);
        return nullptr;
    }

    animation->setStartValue(startPos);
    animation->setEndValue(endPos);
    animation->setDuration(duration > 0 ? duration : d->defaultSlideDuration);
    animation->setEasingCurve(QEasingCurve::OutBack);

    animation->start();
    return animation;
}

/**
 * @brief 执行滑出动画
 */
QPropertyAnimation* AnimationManager::slideOut(QWidget* widget, Qt::Edge direction, int duration)
{
    if (!widget || !d->animationsEnabled) {
        if (widget) widget->setVisible(false);
        return nullptr;
    }

    stopAnimation(widget, "pos");

    QPoint startPos = widget->pos();
    QPoint endPos = startPos;

    switch (direction) {
    case Qt::LeftEdge:
        endPos.setX(startPos.x() - widget->width());
        break;
    case Qt::RightEdge:
        endPos.setX(startPos.x() + widget->width());
        break;
    case Qt::TopEdge:
        endPos.setY(startPos.y() - widget->height());
        break;
    case Qt::BottomEdge:
        endPos.setY(startPos.y() + widget->height());
        break;
    }

    QPropertyAnimation* animation = createAnimation(widget, "pos");
    if (!animation) {
        widget->setVisible(false);
        return nullptr;
    }

    animation->setStartValue(startPos);
    animation->setEndValue(endPos);
    animation->setDuration(duration > 0 ? duration : d->defaultSlideDuration);
    animation->setEasingCurve(d->defaultEasing);

    QPointer<QWidget> widgetPtr(widget);
    connect(animation, &QPropertyAnimation::finished, this, [widgetPtr]() {
        if (widgetPtr) widgetPtr->setVisible(false);
    });

    animation->start();
    return animation;
}

/**
 * @brief 执行缩放进入动画
 */
QPropertyAnimation* AnimationManager::scaleIn(QWidget* widget, int duration)
{
    if (!widget || !d->animationsEnabled) {
        if (widget) widget->setVisible(true);
        return nullptr;
    }

    stopAnimation(widget, "geometry");

    QRect startGeometry = widget->geometry();
    QRect endGeometry = startGeometry;

    // 缓存原始尺寸
    d->cacheOriginalGeometry(widget);

    QPoint center = startGeometry.center();
    startGeometry.setSize(QSize(0, 0));
    startGeometry.moveCenter(center);

    widget->setGeometry(startGeometry);
    widget->setVisible(true);

    QPropertyAnimation* animation = createAnimation(widget, "geometry");
    if (!animation) {
        widget->setGeometry(endGeometry);
        return nullptr;
    }

    animation->setStartValue(startGeometry);
    animation->setEndValue(endGeometry);
    animation->setDuration(duration > 0 ? duration : d->defaultScaleDuration);
    animation->setEasingCurve(QEasingCurve::OutBack);

    animation->start();
    return animation;
}

/**
 * @brief 执行缩放退出动画
 */
QPropertyAnimation* AnimationManager::scaleOut(QWidget* widget, int duration)
{
    if (!widget || !d->animationsEnabled) {
        if (widget) widget->setVisible(false);
        return nullptr;
    }

    stopAnimation(widget, "geometry");

    QRect startGeometry = widget->geometry();
    QPoint center = startGeometry.center();
    QRect endGeometry = startGeometry;
    endGeometry.setSize(QSize(0, 0));
    endGeometry.moveCenter(center);

    QPropertyAnimation* animation = createAnimation(widget, "geometry");
    if (!animation) {
        widget->setVisible(false);
        return nullptr;
    }

    animation->setStartValue(startGeometry);
    animation->setEndValue(endGeometry);
    animation->setDuration(duration > 0 ? duration : d->defaultScaleDuration);
    animation->setEasingCurve(d->defaultEasing);

    QPointer<QWidget> widgetPtr(widget);
    connect(animation, &QPropertyAnimation::finished, this, [this, widgetPtr]() {
        if (widgetPtr) {
            widgetPtr->setVisible(false);
            d->clearGeometryCache(widgetPtr);
        }
    });

    animation->start();
    return animation;
}

/**
 * @brief 卡片悬停进入效果
 */
void AnimationManager::cardHoverEnter(QWidget* card)
{
    if (!card || !d->animationsEnabled) return;

    QGraphicsDropShadowEffect* effect = getOrCreateShadowEffect(card);
    if (!effect) return;

    // 停止效果上可能正在运行的动画
    stopAnimation(effect, "blurRadius");
    stopAnimation(effect, "offset");

    // 模糊半径动画
    QPropertyAnimation* blurAnim = createAnimation(effect, "blurRadius");
    if (blurAnim) {
        blurAnim->setStartValue(effect->blurRadius());
        blurAnim->setEndValue(d->defaultShadowBlurRadius);
        blurAnim->setDuration(d->defaultButtonAnimDuration);
        blurAnim->setEasingCurve(d->defaultEasing);
        blurAnim->start();
    }

    // 偏移动画
    QPropertyAnimation* offsetAnim = createAnimation(effect, "offset");
    if (offsetAnim) {
        offsetAnim->setStartValue(effect->offset());
        offsetAnim->setEndValue(d->defaultShadowOffsetHover);
        offsetAnim->setDuration(d->defaultButtonAnimDuration);
        offsetAnim->setEasingCurve(d->defaultEasing);
        offsetAnim->start();
    }
}

/**
 * @brief 卡片悬停离开效果
 */
void AnimationManager::cardHoverLeave(QWidget* card)
{
    if (!card || !d->animationsEnabled) return;

    QGraphicsEffect* existingEffect = card->graphicsEffect();
    if (!existingEffect) return;

    QGraphicsDropShadowEffect* effect = qobject_cast<QGraphicsDropShadowEffect*>(existingEffect);
    if (!effect) return;

    // 停止现有动画
    stopAnimation(effect, "blurRadius");
    stopAnimation(effect, "offset");

    QPointer<QWidget> cardPtr(card);
    QPointer<QGraphicsDropShadowEffect> effectPtr(effect);

    // 模糊半径动画
    QPropertyAnimation* blurAnim = createAnimation(effect, "blurRadius");
    if (blurAnim) {
        blurAnim->setStartValue(effect->blurRadius());
        blurAnim->setEndValue(0);
        blurAnim->setDuration(d->defaultButtonAnimDuration);
        blurAnim->setEasingCurve(d->defaultEasing);
        blurAnim->start();
    }

    // 偏移动画
    QPropertyAnimation* offsetAnim = createAnimation(effect, "offset");
    if (offsetAnim) {
        offsetAnim->setStartValue(effect->offset());
        offsetAnim->setEndValue(QPointF(0, 0));
        offsetAnim->setDuration(d->defaultButtonAnimDuration);
        offsetAnim->setEasingCurve(d->defaultEasing);

        // 动画完成后移除效果
        connect(offsetAnim, &QPropertyAnimation::finished, this, [this, cardPtr, effectPtr]() {
            if (effectPtr) {
                effectPtr->setEnabled(false); // 【关键修复】先禁用
            }
            if (cardPtr) {
                // 使用 update() 确保任何待处理的绘制事件先完成
                cardPtr->update();

                // 使用 QMetaObject::invokeMethod 延迟到事件循环下一轮执行移除和隐藏
                QMetaObject::invokeMethod(this, [this, cardPtr]() {
                    if (cardPtr) {
                        removeEffects(cardPtr, 0);
                        cardPtr->setVisible(false);
                    }
                }, Qt::QueuedConnection);
            }
        });

        offsetAnim->start();
    }
}

/**
 * @brief 卡片按下效果
 */
void AnimationManager::cardPress(QWidget* card)
{
    if (!card || !d->animationsEnabled) return;

    d->cacheOriginalGeometry(card);
    stopAnimation(card, "geometry");

    QPropertyAnimation* animation = createAnimation(card, "geometry");
    if (!animation) return;

    QRect geo = card->geometry();
    animation->setStartValue(geo);
    geo.adjust(2, 2, -2, -2);
    animation->setEndValue(geo);
    animation->setDuration(d->defaultCardPressDuration);
    animation->start();
}

/**
 * @brief 卡片释放效果
 */
void AnimationManager::cardRelease(QWidget* card)
{
    if (!card || !d->animationsEnabled) return;

    stopAnimation(card, "geometry");

    QPropertyAnimation* animation = createAnimation(card, "geometry");
    if (!animation) return;

    QRect geo = card->geometry();
    QRect originalGeo = d->getCachedOriginalGeometry(card);

    animation->setStartValue(geo);
    if (originalGeo.isValid()) {
        animation->setEndValue(originalGeo);
    } else {
        geo.adjust(-2, -2, 2, 2);
        animation->setEndValue(geo);
    }

    animation->setDuration(d->defaultCardPressDuration);
    animation->start();

    d->clearGeometryCache(card);
}

/**
 * @brief 按钮悬停进入效果
 */
void AnimationManager::buttonHoverEnter(QPushButton* button)
{
    if (!button || !d->animationsEnabled) return;

    // 清理已销毁的条目
    d->cleanupExpiredEntries();

    // 缓存原始样式表
    if (!d->originalButtonStyles.contains(button)) {
        d->originalButtonStyles[button] = button->styleSheet();
    }

    // 使用属性选择器标记悬停状态
    button->setProperty("hover", true);
    button->style()->unpolish(button);
    button->style()->polish(button);
}

/**
 * @brief 按钮悬停离开效果
 */
void AnimationManager::buttonHoverLeave(QPushButton* button)
{
    if (!button || !d->animationsEnabled) return;

    // 移除悬停属性
    button->setProperty("hover", false);
    button->style()->unpolish(button);
    button->style()->polish(button);

    auto it = d->originalButtonStyles.find(button);
    if (it != d->originalButtonStyles.end()) {
        d->originalButtonStyles.erase(it);
    }
}

void AnimationManager::buttonPress(QPushButton* button)
{
    if (!button || !d->animationsEnabled) return;

    // 使用属性选择器标记按下状态
    button->setProperty("pressed", true);
    button->style()->unpolish(button);
    button->style()->polish(button);
}

void AnimationManager::buttonRelease(QPushButton* button)
{
    if (!button || !d->animationsEnabled) return;

    // 移除按下属性
    button->setProperty("pressed", false);
    button->style()->unpolish(button);
    button->style()->polish(button);

    buttonHoverLeave(button);
}

/**
 * @brief 整数数字滚动动画
 */
void AnimationManager::animateNumber(QLabel* label, int startValue, int endValue, int duration)
{
    if (!label || !d->animationsEnabled) {
        if (label) label->setText(QString::number(endValue));
        return;
    }

    // 停止该标签上可能存在的数值动画
    // stopAnimation(label, "text"); // 如果有的话，但通常是 QVariantAnimation

    QPointer<QLabel> labelPtr(label);
    QVariantAnimation* animation = new QVariantAnimation(this);
    animation->setStartValue(startValue);
    animation->setEndValue(endValue);
    animation->setDuration(duration);
    animation->setEasingCurve(d->defaultEasing);

    connect(animation, &QVariantAnimation::valueChanged, this, [labelPtr](const QVariant& value) {
        if (labelPtr) labelPtr->setText(QString::number(value.toInt()));
    });

    connect(animation, &QVariantAnimation::finished, animation, &QObject::deleteLater);
    animation->start();
}

/**
 * @brief 浮点数数字滚动动画
 */
void AnimationManager::animateNumber(QLabel* label, double startValue, double endValue, int duration)
{
    if (!label || !d->animationsEnabled) {
        if (label) label->setText(QString::number(endValue, 'f', 2));
        return;
    }

    QPointer<QLabel> labelPtr(label);
    QVariantAnimation* animation = new QVariantAnimation(this);
    animation->setStartValue(startValue);
    animation->setEndValue(endValue);
    animation->setDuration(duration);
    animation->setEasingCurve(d->defaultEasing);

    connect(animation, &QVariantAnimation::valueChanged, this, [labelPtr](const QVariant& value) {
        if (labelPtr) labelPtr->setText(QString::number(value.toDouble(), 'f', 2));
    });

    connect(animation, &QVariantAnimation::finished, animation, &QObject::deleteLater);
    animation->start();
}

/**
 * @brief 进度条数值动画
 */
void AnimationManager::animateProgress(QProgressBar* progressBar, int startValue, int endValue, int duration)
{
    if (!progressBar || !d->animationsEnabled) {
        if (progressBar) progressBar->setValue(endValue);
        return;
    }

    QPropertyAnimation* animation = createAnimation(progressBar, "value");
    if (!animation) {
        progressBar->setValue(endValue);
        return;
    }

    animation->setStartValue(startValue);
    animation->setEndValue(endValue);
    animation->setDuration(duration);
    animation->setEasingCurve(d->defaultEasing);
    animation->start();
}

/**
 * @brief 页面切换过渡动画
 */
void AnimationManager::pageTransition(QWidget* fromPage, QWidget* toPage, Qt::Edge direction)
{
    if (!fromPage || !toPage) return;

    if (!d->animationsEnabled) {
        fromPage->setVisible(false);
        toPage->setVisible(true);
        return;
    }

    QParallelAnimationGroup* group = new QParallelAnimationGroup(this);
    QPointer<QWidget> fromPtr(fromPage);
    QPointer<QWidget> toPtr(toPage);

    QPoint fromStart = fromPage->pos();
    QPoint fromEnd = fromStart;
    QPoint toEnd = toPage->pos();
    QPoint toStart = toEnd;

    switch (direction) {
    case Qt::LeftEdge:
        fromEnd.setX(fromStart.x() + fromPage->width());
        toStart.setX(toEnd.x() - toPage->width());
        break;
    case Qt::RightEdge:
        fromEnd.setX(fromStart.x() - fromPage->width());
        toStart.setX(toEnd.x() + toPage->width());
        break;
    case Qt::TopEdge:
        fromEnd.setY(fromStart.y() + fromPage->height());
        toStart.setY(toEnd.y() - toPage->height());
        break;
    case Qt::BottomEdge:
        fromEnd.setY(fromStart.y() - fromPage->height());
        toStart.setY(toEnd.y() + toPage->height());
        break;
    }

    toPage->move(toStart);
    toPage->setVisible(true);
    toPage->raise();

    // 当前页面滑出动画
    QPropertyAnimation* fromAnim = createAnimation(fromPage, "pos");
    if (fromAnim) {
        fromAnim->setStartValue(fromStart);
        fromAnim->setEndValue(fromEnd);
        fromAnim->setDuration(300);
        fromAnim->setEasingCurve(d->defaultEasing);
        group->addAnimation(fromAnim);
    }

    // 目标页面滑入动画
    QPropertyAnimation* toAnim = createAnimation(toPage, "pos");
    if (toAnim) {
        toAnim->setStartValue(toStart);
        toAnim->setEndValue(toEnd);
        toAnim->setDuration(300);
        toAnim->setEasingCurve(d->defaultEasing);
        group->addAnimation(toAnim);
    }

    connect(group, &QParallelAnimationGroup::finished, this, [fromPtr, group]() {
        if (fromPtr) fromPtr->setVisible(false);
        group->deleteLater();
    });

    group->start();
}

/**
 * @brief 列表项插入动画
 */
void AnimationManager::listItemInsert(QWidget* item, int index)
{
    Q_UNUSED(index)
    if (!item || !d->animationsEnabled) {
        if (item) item->setVisible(true);
        return;
    }

    item->setVisible(false);

    if (item->size().isEmpty()) {
        item->resize(item->sizeHint().isEmpty() ? QSize(100, 30) : item->sizeHint());
    }

    QPointer<QWidget> itemPtr(item);

    // 使用单次定时器延迟启动动画
    QTimer::singleShot(0, this, [this, itemPtr]() {
        if (!itemPtr) return;

        if (itemPtr->isVisible()) {
            itemPtr->setVisible(false);
        }

        QTimer::singleShot(10, this, [this, itemPtr]() {
            if (itemPtr) {
                fadeIn(itemPtr, 250);
                slideIn(itemPtr, Qt::TopEdge, 250);
            }
        });
    });
}

/**
 * @brief 列表项删除动画
 */
void AnimationManager::listItemRemove(QWidget* item)
{
    if (!item || !d->animationsEnabled) {
        if (item) item->deleteLater();
        return;
    }

    QPointer<QWidget> itemPtr(item);
    fadeOut(item, 200);

    QTimer::singleShot(250, this, [itemPtr]() {
        if (itemPtr) itemPtr->deleteLater();
    });
}

/**
 * @brief 列表项重排序动画
 */
void AnimationManager::listItemReorder(QWidget* item, int newIndex)
{
    Q_UNUSED(newIndex)

    if (!item || !d->animationsEnabled) return;

    QGraphicsOpacityEffect* effect = getOrCreateOpacityEffect(item);
    if (!effect) return;

    stopAnimation(effect, "opacity");

    QSequentialAnimationGroup* group = new QSequentialAnimationGroup(this);

    QPropertyAnimation* fadeOut = createAnimation(effect, "opacity");
    if (fadeOut) {
        fadeOut->setStartValue(1.0);
        fadeOut->setEndValue(0.5);
        fadeOut->setDuration(150);
        group->addAnimation(fadeOut);
    }

    QPropertyAnimation* fadeIn = createAnimation(effect, "opacity");
    if (fadeIn) {
        fadeIn->setStartValue(0.5);
        fadeIn->setEndValue(1.0);
        fadeIn->setDuration(150);
        group->addAnimation(fadeIn);
    }

    connect(group, &QSequentialAnimationGroup::finished, group, &QObject::deleteLater);
    group->start();
}

/**
 * @brief 停止指定对象的所有动画
 */
void AnimationManager::stopAnimation(QObject* target)
{
    if (!target) return;

    // 收集需要停止的动画
    QList<QPropertyAnimation*> animsToStop;
    for (auto it = d->activeAnimations.begin(); it != d->activeAnimations.end(); ++it) {
        QPropertyAnimation* anim = it.value();
        if (anim && anim->targetObject() == target && anim->state() != QAbstractAnimation::Stopped) {
            animsToStop.append(anim);
        }
    }

    // 停止动画
    for (QPropertyAnimation* anim : animsToStop) {
        anim->stop();
    }
}

/**
 * @brief 停止指定对象指定属性的动画
 */
void AnimationManager::stopAnimation(QObject* target, const QByteArray& property)
{
    if (!target) return;

    QString animationKey = makeAnimationKey(target, property);
    if (animationKey.isEmpty()) return;

    QPropertyAnimation* anim = d->getActiveAnimation(animationKey);
    if (anim && anim->state() != QAbstractAnimation::Stopped) {
        anim->stop();
    }
}

/**
 * @brief 停止所有动画
 */
void AnimationManager::stopAllAnimations()
{
    // 收集所有活跃动画
    QList<QPropertyAnimation*> allAnims;
    for (auto it = d->activeAnimations.begin(); it != d->activeAnimations.end(); ++it) {
        QPropertyAnimation* anim = it.value();
        if (anim && anim->state() != QAbstractAnimation::Stopped) {
            allAnims.append(anim);
        }
    }

    // 停止所有动画
    for (QPropertyAnimation* anim : allAnims) {
        anim->stop();
    }

    d->activeAnimations.clear();
}

void AnimationManager::setAnimationsEnabled(bool enabled)
{
    d->animationsEnabled = enabled;

    if (!enabled) {
        stopAllAnimations();
    }
}

bool AnimationManager::animationsEnabled() const
{
    return d->animationsEnabled;
}

int AnimationManager::activeAnimationCount() const
{
    int count = 0;
    for (auto it = d->activeAnimations.begin(); it != d->activeAnimations.end(); ++it) {
        QPropertyAnimation* anim = it.value();
        if (anim && anim->state() != QAbstractAnimation::Stopped) {
            ++count;
        }
    }
    return count;
}
