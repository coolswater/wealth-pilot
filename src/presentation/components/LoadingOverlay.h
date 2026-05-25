/**
 * @file LoadingOverlay.h
 * @brief 加载遮罩组件 - 显示加载状态和进度
 *
 * @details 功能：
 * - 半透明遮罩层
 * - 加载动画（旋转圆圈）
 * - 加载文本提示
 * - 进度条支持
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef LOADINGOVERLAY_H
#define LOADINGOVERLAY_H

#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QTimer>
#include <QPropertyAnimation>
#include <memory>

/**
 * @brief 加载遮罩组件
 */
class LoadingOverlay : public QWidget
{
    Q_OBJECT

public:
    explicit LoadingOverlay(QWidget* parent = nullptr);
    ~LoadingOverlay() override;

    /**
     * @brief 设置加载文本
     */
    void setText(const QString& text);

    /**
     * @brief 设置进度（0-100）
     */
    void setProgress(int value);

    /**
     * @brief 显示加载状态
     */
    void showLoading();

    /**
     * @brief 隐藏加载状态
     */
    void hideLoading();

    /**
     * @brief 是否正在加载
     */
    bool isLoading() const;

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void setupUI();
    void startAnimation();
    void stopAnimation();

    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // LOADINGOVERLAY_H
