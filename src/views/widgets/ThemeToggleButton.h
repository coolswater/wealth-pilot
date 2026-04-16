#ifndef THEMETOGGLEBUTTON_H
#define THEMETOGGLEBUTTON_H

#include <QWidget>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QSvgRenderer>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPainterPath>  // 添加：修复不完整类型错误
#include "src/ui/ThemeManager.h"

/**
 * @brief 三态主题切换按钮控�?
 *
 * 支持亮色(Light)、暗�?Dark)、护�?EyeCare)三种主题切换
 * 特点�?
 * 1. 流畅的滑动指示器动画（使用QPropertyAnimation�?
 * 2. SVG矢量图标，自动跟随主题变�?
 * 3. 智能缓存绘制，优化性能
 * 4. 与ThemeManager深度集成，状态同�?
 */
class ThemeToggleButton : public QWidget
{
    Q_OBJECT
    // 声明动画属性：指示器位�?(0.0 ~ 2.0 对应三个状�?
    Q_PROPERTY(qreal indicatorPosition READ indicatorPosition WRITE setIndicatorPosition NOTIFY indicatorPositionChanged)
    // 声明动画属性：背景色过�?
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY backgroundColorChanged)

public:
    explicit ThemeToggleButton(QWidget *parent = nullptr);
    ~ThemeToggleButton();

    // 设置图标大小（默�?0px�?
    void setIconSize(int size);
    int iconSize() const { return m_iconSize; }

    // 设置按钮尺寸（默�?20x40�?
    void setButtonSize(const QSize &size);
    QSize buttonSize() const { return m_buttonSize; }

    // 获取当前指示器位置（用于动画�?
    qreal indicatorPosition() const { return m_indicatorPosition; }

    // 获取当前背景色（用于动画过渡�?
    QColor backgroundColor() const { return m_currentBgColor; }

public slots:
    // 设置指示器位置（动画驱动�?
    void setIndicatorPosition(qreal position);
    // 设置背景色（动画驱动�?
    void setBackgroundColor(const QColor &color);
    // 外部主题变更响应
    void onThemeChanged(ThemeManager::ThemeType newTheme);

signals:
    void indicatorPositionChanged(qreal position);
    void backgroundColorChanged(const QColor &color);
    // 用户点击切换主题信号
    void themeSwitchRequested(ThemeManager::ThemeType targetTheme);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void enterEvent(QEnterEvent *event) override;  // Qt6 使用 QEnterEvent
    void leaveEvent(QEvent *event) override;

private:
    // 初始化控�?
    void initialize();
    // 加载SVG图标（只加载一次，缓存�?
    void loadIcons();
    // 根据位置计算目标主题
    ThemeManager::ThemeType themeFromPosition(qreal position) const;
    // 根据主题计算目标位置
    qreal positionFromTheme(ThemeManager::ThemeType theme) const;
    // 更新颜色配置（从ThemeManager获取�?
    void updateColors();
    // 启动切换动画
    void animateTo(ThemeManager::ThemeType targetTheme);

    // 绘制辅助函数（优化：分层绘制，减少重复计算）
    void drawBackground(QPainter *painter);
    void drawIndicator(QPainter *painter);
    void drawIcons(QPainter *painter);

    // 获取指定位置的图标颜色（根据指示器位置计算渐变）
    QColor calculateIconColor(int index) const;

private:
    // 配置参数
    int m_iconSize = 20;
    QSize m_buttonSize = QSize(120, 40);
    int m_indicatorMargin = 4;
    int m_cornerRadius = 20;

    // 状�?
    ThemeManager::ThemeType m_currentTheme = ThemeManager::ThemeType::Dark;
    qreal m_indicatorPosition = 0.0;  // 0=Light, 1=Dark, 2=EyeCare
    bool m_isHovered = false;

    // 颜色缓存（避免每次都从ThemeManager查询�?
    QColor m_currentBgColor;
    QColor m_indicatorColor;
    QColor m_iconColorNormal;
    QColor m_iconColorActive;

    // SVG渲染器（缓存，避免重复解析）
    QSvgRenderer *m_iconLight = nullptr;    // 太阳/亮色图标
    QSvgRenderer *m_iconDark = nullptr;     // 月亮/暗色图标
    QSvgRenderer *m_iconEyeCare = nullptr;  // 护眼/夜间图标

    // 动画
    QPropertyAnimation *m_positionAnimation = nullptr;
    QPropertyAnimation *m_colorAnimation = nullptr;

    // 性能优化：缓存绘制路�?
    QPainterPath m_backgroundPath;    // 现在编译器能看到完整定义
    QPainterPath m_indicatorPath;
    bool m_pathCacheValid = false;
};

#endif // THEMETOGGLEBUTTON_H

