#ifndef THEMETOGGLEBUTTON_H
#define THEMETOGGLEBUTTON_H

#include <QWidget>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QSvgRenderer>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPainterPath>
#include "src/ui/ThemeManager.h"

/**
 * @brief 动态主题切换按钮控件
 *
 * 支持亮色(Light)、暗色(Dark)、护眼(EyeCare)三种主题切换
 * 特点：
 * 1. 平滑滑动指示器动画（使用QPropertyAnimation）
 * 2. SVG矢量图标，自动缩放不失真
 * 3. 高性能绘制（优化绘图）
 * 4. 与ThemeManager深度集成，状态同步
 */
class ThemeToggleButton : public QWidget
{
    Q_OBJECT
    // 指示器位置属性（用于动画）
    Q_PROPERTY(qreal indicatorPosition READ indicatorPosition WRITE setIndicatorPosition NOTIFY indicatorPositionChanged)
    // 背景颜色属性（用于动画）
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY backgroundColorChanged)

public:
    explicit ThemeToggleButton(QWidget *parent = nullptr);
    ~ThemeToggleButton();

    // 设置图标大小（默认20px）
    void setIconSize(int size);
    int iconSize() const { return m_iconSize; }

    // 设置按钮尺寸（默认120x40）
    void setButtonSize(const QSize &size);
    QSize buttonSize() const { return m_buttonSize; }

    // 获取当前指示器位置（用于动画）
    qreal indicatorPosition() const { return m_indicatorPosition; }

    // 获取当前背景颜色（用于动画和样式）
    QColor backgroundColor() const { return m_currentBgColor; }

public slots:
    // 设置指示器位置（用于动画）
    void setIndicatorPosition(qreal position);
    // 设置背景颜色（用于动画）
    void setBackgroundColor(const QColor &color);
    // 外部主题变化响应
    void onThemeChanged(ThemeManager::ThemeType newTheme);

signals:
    void indicatorPositionChanged(qreal position);
    void backgroundColorChanged(const QColor &color);
    // 用户主动切换主题的信号
    void themeSwitchRequested(ThemeManager::ThemeType targetTheme);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void enterEvent(QEnterEvent *event) override;  // Qt6 使用 QEnterEvent
    void leaveEvent(QEvent *event) override;

private:
    // 初始化
    void initialize();
    // 加载SVG图标（只加载一次，缓存）
    void loadIcons();
    // 根据位置计算目标主题
    ThemeManager::ThemeType themeFromPosition(qreal position) const;
    // 根据主题计算目标位置
    qreal positionFromTheme(ThemeManager::ThemeType theme) const;
    // 更新颜色（从ThemeManager获取）
    void updateColors();
    // 执行切换动画
    void animateTo(ThemeManager::ThemeType targetTheme);

    // 绘制各部分（优化：分层绘制，避免重复计算）
    void drawBackground(QPainter *painter);
    void drawIndicator(QPainter *painter);
    void drawIcons(QPainter *painter);

    // 获取指定位置的图标颜色（根据指示器位置计算渐变）
    QColor calculateIconColor(int index) const;

private:
    // 尺寸参数
    int m_iconSize = 20;                    ///< 图标大小
    QSize m_buttonSize = QSize(120, 40);    ///< 按钮尺寸
    int m_indicatorMargin = 4;              ///< 指示器边距
    int m_cornerRadius = 20;                ///< 圆角半径

    // 状态
    ThemeManager::ThemeType m_currentTheme = ThemeManager::ThemeType::Dark;  ///< 当前主题
    qreal m_indicatorPosition = 0.0;        ///< 指示器位置（0=Light, 1=Dark, 2=EyeCare）
    bool m_isHovered = false;               ///< 是否悬停

    // 颜色缓存（避免每次都从ThemeManager查询）
    QColor m_currentBgColor;                ///< 当前背景色
    QColor m_indicatorColor;                ///< 指示器颜色
    QColor m_iconColorNormal;               ///< 图标正常颜色
    QColor m_iconColorActive;               ///< 图标激活颜色

    // SVG渲染器（缓存，避免重复加载文件）
    QSvgRenderer *m_iconLight = nullptr;    ///< 太阳/亮色图标
    QSvgRenderer *m_iconDark = nullptr;     ///< 月亮/暗色图标
    QSvgRenderer *m_iconEyeCare = nullptr;  ///< 眼睛/护眼图标

    // 动画
    QPropertyAnimation *m_positionAnimation = nullptr;  ///< 位置动画
    QPropertyAnimation *m_colorAnimation = nullptr;     ///< 颜色动画

    // 绘图优化（缓存路径）
    QPainterPath m_backgroundPath;          ///< 背景路径（用于避免重复计算）
    QPainterPath m_indicatorPath;           ///< 指示器路径
    bool m_pathCacheValid = false;          ///< 路径缓存是否有效
};

#endif // THEMETOGGLEBUTTON_H