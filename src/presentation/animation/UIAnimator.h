/**
 * @file UIAnimator.h
 * @brief UI 交互动画器
 *
 * @details 处理 UI 控件的交互动画：
 * - 按钮 hover/press 效果
 * - 卡片 hover/press 效果
 * - 数字滚动动画
 * - 进度条动画
 * - 列表项动画
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef UIANIMATOR_H
#define UIANIMATOR_H

#include "AnimationTypes.h"
#include <QObject>
#include <QPointer>
#include <memory>

class QWidget;
class QPushButton;
class QLabel;
class QProgressBar;

namespace WealthPilot {

/**
 * @brief UI 交互动画器
 */
class UIAnimator : public QObject {
    Q_OBJECT

public:
    explicit UIAnimator(QObject* parent = nullptr);
    ~UIAnimator() override;

    // ========== 按钮动画 ==========
    
    /**
     * @brief 按钮悬停进入效果
     */
    void buttonHoverEnter(QPushButton* button);
    
    /**
     * @brief 按钮悬停离开效果
     */
    void buttonHoverLeave(QPushButton* button);
    
    /**
     * @brief 按钮按下效果
     */
    void buttonPress(QPushButton* button);
    
    /**
     * @brief 按钮释放效果
     */
    void buttonRelease(QPushButton* button);
    
    // ========== 卡片动画 ==========
    
    /**
     * @brief 卡片悬停进入效果
     */
    void cardHoverEnter(QWidget* card);
    
    /**
     * @brief 卡片悬停离开效果
     */
    void cardHoverLeave(QWidget* card);
    
    /**
     * @brief 卡片按下效果
     */
    void cardPress(QWidget* card);
    
    /**
     * @brief 卡片释放效果
     */
    void cardRelease(QWidget* card);
    
    // ========== 数值动画 ==========
    
    /**
     * @brief 数字滚动动画（整数）
     */
    void animateNumber(QLabel* label, 
                        int startValue, int endValue, 
                        int duration = 500);
    
    /**
     * @brief 数字滚动动画（浮点数）
     */
    void animateNumber(QLabel* label,
                        double startValue, double endValue,
                        int duration = 500);
    
    /**
     * @brief 进度条动画
     */
    void animateProgress(QProgressBar* progressBar,
                          int startValue, int endValue,
                          int duration = 500);
    
    // ========== 列表动画 ==========
    
    /**
     * @brief 列表项插入动画
     */
    void listItemInsert(QWidget* item, int index);
    
    /**
     * @brief 列表项移除动画
     */
    void listItemRemove(QWidget* item);
    
    /**
     * @brief 列表项重排序动画
     */
    void listItemReorder(QWidget* item, int newIndex);
    
    // ========== 控制方法 ==========
    
    /**
     * @brief 停止指定控件的动画
     */
    void stop(QWidget* widget);
    
    /**
     * @brief 停止所有动画
     */
    void stopAll();

signals:
    void animationFinished(QWidget* widget);

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace WealthPilot

#endif // UIANIMATOR_H