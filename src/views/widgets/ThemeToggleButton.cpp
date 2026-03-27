#include "ThemeToggleButton.h"
#include <QPainterPath>
#include <QtMath>

// SVG图标数据（内嵌资源，避免外部文件依赖）
// 月亮图标（Dark）- 现在放在第一位
static const char *SVG_DARK = R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M21 12.79A9 9 0 1 1 11.21 3 7 7 0 0 0 21 12.79z"></path></svg>)";

// 太阳图标（Light）- 现在放在第二位
static const char *SVG_LIGHT = R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="5"></circle><line x1="12" y1="1" x2="12" y2="3"></line><line x1="12" y1="21" x2="12" y2="23"></line><line x1="4.22" y1="4.22" x2="5.64" y2="5.64"></line><line x1="18.36" y1="18.36" x2="19.78" y2="19.78"></line><line x1="1" y1="12" x2="3" y2="12"></line><line x1="21" y1="12" x2="23" y2="12"></line><line x1="4.22" y1="19.78" x2="5.64" y2="18.36"></line><line x1="18.36" y1="5.64" x2="19.78" y2="4.22"></line></svg>)";

// 眼睛/护眼图标（EyeCare）- 保持第三位
static const char *SVG_EYECARE = R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"></path><circle cx="12" cy="12" r="3"></circle></svg>)";

ThemeToggleButton::ThemeToggleButton(QWidget *parent)
    : QWidget(parent)
{
    initialize();
    loadIcons();
    updateColors();

    // 连接ThemeManager信号，实现外部主题变更同步
    connect(ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &ThemeToggleButton::onThemeChanged);
}

ThemeToggleButton::~ThemeToggleButton()
{
    // 清理SVG渲染器
    delete m_iconLight;
    delete m_iconDark;
    delete m_iconEyeCare;
}

void ThemeToggleButton::initialize()
{
    // 设置固定大小（可自定义）
    setFixedSize(m_buttonSize);
    setCursor(Qt::PointingHandCursor);

    // 初始化位置动画（使用弹簧效果，更自然）
    m_positionAnimation = new QPropertyAnimation(this, "indicatorPosition", this);
    m_positionAnimation->setDuration(300);  // 300ms，平衡流畅度和响应速度
    m_positionAnimation->setEasingCurve(QEasingCurve::OutBack);  // 回弹效果

    // 初始化颜色动画
    m_colorAnimation = new QPropertyAnimation(this, "backgroundColor", this);
    m_colorAnimation->setDuration(250);
    m_colorAnimation->setEasingCurve(QEasingCurve::InOutQuad);

    // 设置初始位置（根据当前主题）
    m_indicatorPosition = positionFromTheme(ThemeManager::instance()->currentTheme());
}

void ThemeToggleButton::loadIcons()
{
    // 从字节数组加载SVG（避免文件IO，提升性能）
    m_iconDark = new QSvgRenderer(QByteArray(SVG_DARK), this);
    m_iconLight = new QSvgRenderer(QByteArray(SVG_LIGHT), this);
    m_iconEyeCare = new QSvgRenderer(QByteArray(SVG_EYECARE), this);

    // 确保SVG正确加载
    Q_ASSERT(m_iconDark->isValid());
    Q_ASSERT(m_iconLight->isValid());
    Q_ASSERT(m_iconEyeCare->isValid());
}

void ThemeToggleButton::updateColors()
{
    auto *tm = ThemeManager::instance();

    // 获取主题颜色（使用color()接口）
    m_currentBgColor = tm->color("border");  // 使用边框色作为按钮背景
    m_indicatorColor = tm->color("card");    // 使用卡片色作为指示器
    m_iconColorNormal = tm->color("neutral");  // 中性色用于未选中图标
    m_iconColorActive = tm->color("foreground"); // 前景色用于选中图标

    // 如果获取失败，使用默认安全色
    if (!m_currentBgColor.isValid()) m_currentBgColor = QColor(0xE5E7EB);  // #E5E7EB
    if (!m_indicatorColor.isValid()) m_indicatorColor = QColor(0xFFFFFF);  // #FFFFFF
    if (!m_iconColorNormal.isValid()) m_iconColorNormal = QColor(0x9CA3AF); // #9CA3AF
    if (!m_iconColorActive.isValid()) m_iconColorActive = QColor(0x1F2937); // #1F2937

    update();  // 触发重绘
}

void ThemeToggleButton::setIconSize(int size)
{
    m_iconSize = size;
    m_pathCacheValid = false;  // 路径缓存失效
    update();
}

void ThemeToggleButton::setButtonSize(const QSize &size)
{
    m_buttonSize = size;
    setFixedSize(size);
    m_pathCacheValid = false;
    update();
}

void ThemeToggleButton::setIndicatorPosition(qreal position)
{
    if (!qFuzzyCompare(m_indicatorPosition, position)) {
        m_indicatorPosition = position;
        emit indicatorPositionChanged(position);
        update();  // 位置变化触发重绘
    }
}

void ThemeToggleButton::setBackgroundColor(const QColor &color)
{
    if (m_currentBgColor != color) {
        m_currentBgColor = color;
        emit backgroundColorChanged(color);
        update();
    }
}

void ThemeToggleButton::onThemeChanged(ThemeManager::ThemeType newTheme)
{
    if (m_currentTheme != newTheme) {
        m_currentTheme = newTheme;

        // 同步更新颜色配置
        updateColors();

        // 启动位置动画（平滑滑动到新位置）
        animateTo(newTheme);
    }
}

void ThemeToggleButton::animateTo(ThemeManager::ThemeType targetTheme)
{
    qreal targetPos = positionFromTheme(targetTheme);

    // 停止当前动画，避免冲突
    m_positionAnimation->stop();

    // 设置新的动画起止值
    m_positionAnimation->setStartValue(m_indicatorPosition);
    m_positionAnimation->setEndValue(targetPos);
    m_positionAnimation->start();
}

qreal ThemeToggleButton::positionFromTheme(ThemeManager::ThemeType theme) const
{
    // 新顺序：Dark(0) -> Light(1) -> EyeCare(2)
    switch (theme) {
    case ThemeManager::ThemeType::Dark:     return 0.0;  // 第一个位置
    case ThemeManager::ThemeType::Light:    return 1.0;  // 第二个位置
    case ThemeManager::ThemeType::EyeCare:  return 2.0;  // 第三个位置
    default: return 0.0;
    }
}

ThemeManager::ThemeType ThemeToggleButton::themeFromPosition(qreal position) const
{
    // 四舍五入到最近的整数位置
    int rounded = qRound(position);
    switch (rounded) {
    case 0: return ThemeManager::ThemeType::Dark;     // 第一个位置是Dark
    case 1: return ThemeManager::ThemeType::Light;    // 第二个位置是Light
    case 2: return ThemeManager::ThemeType::EyeCare;  // 第三个位置是EyeCare
    default: return ThemeManager::ThemeType::Dark;
    }
}

void ThemeToggleButton::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // 计算点击位置，决定切换到哪个主题
        qreal clickX = event->pos().x();
        qreal width = this->width();

        // 将宽度分为三等分
        int section = static_cast<int>((clickX / width) * 3);
        section = qBound(0, section, 2);  // 限制在0-2范围

        // 新映射：0=Dark, 1=Light, 2=EyeCare
        ThemeManager::ThemeType targetTheme;
        switch (section) {
        case 0: targetTheme = ThemeManager::ThemeType::Dark; break;
        case 1: targetTheme = ThemeManager::ThemeType::Light; break;
        case 2: targetTheme = ThemeManager::ThemeType::EyeCare; break;
        default: targetTheme = ThemeManager::ThemeType::Dark;
        }

        // 如果点击当前激活区域，则循环到下一个主题（增强交互）
        if (targetTheme == m_currentTheme) {
            // 循环顺序：Dark -> Light -> EyeCare -> Dark
            int next;
            switch (m_currentTheme) {
            case ThemeManager::ThemeType::Dark:    next = 1; break; // 到Light
            case ThemeManager::ThemeType::Light:   next = 2; break; // 到EyeCare
            case ThemeManager::ThemeType::EyeCare: next = 0; break; // 到Dark
            default: next = 0;
            }
            targetTheme = static_cast<ThemeManager::ThemeType>(next);
        }

        // 触发主题切换
        emit themeSwitchRequested(targetTheme);

        // 通过ThemeManager实际切换（实现单向数据流）
        ThemeManager::instance()->setTheme(targetTheme);
    }
    QWidget::mousePressEvent(event);
}

void ThemeToggleButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);      // 抗锯齿，确保圆角平滑
    painter.setRenderHint(QPainter::SmoothPixmapTransform);  // 平滑变换，SVG缩放更清晰

    // 分层绘制（性能优化：使用缓存路径）
    drawBackground(&painter);
    drawIndicator(&painter);
    drawIcons(&painter);
}

void ThemeToggleButton::drawBackground(QPainter *painter)
{
    // 缓存背景路径（避免每次重新计算）
    if (!m_pathCacheValid) {
        m_backgroundPath = QPainterPath();
        m_backgroundPath.addRoundedRect(rect(), m_cornerRadius, m_cornerRadius);
    }

    // 绘制背景（使用当前动画颜色）
    painter->fillPath(m_backgroundPath, m_currentBgColor);

    // 绘制边框（可选，增强视觉层次）
    QPen pen(m_currentBgColor.darker(110));
    pen.setWidth(1);
    painter->setPen(pen);
    painter->drawPath(m_backgroundPath);
}

void ThemeToggleButton::drawIndicator(QPainter *painter)
{
    // 计算指示器位置（基于动画位置值）
    qreal sectionWidth = static_cast<qreal>(width()) / 3.0;
    qreal indicatorX = m_indicatorPosition * sectionWidth + m_indicatorMargin;
    qreal indicatorWidth = sectionWidth - 2 * m_indicatorMargin;
    qreal indicatorHeight = height() - 2 * m_indicatorMargin;

    QRectF indicatorRect(indicatorX, m_indicatorMargin, indicatorWidth, indicatorHeight);

    // 绘制圆形指示器
    QPainterPath indicatorPath;
    indicatorPath.addRoundedRect(indicatorRect, indicatorHeight / 2, indicatorHeight / 2);

    // 绘制阴影
    painter->save();
    painter->setOpacity(0.15);
    painter->translate(0, 2);
    painter->fillPath(indicatorPath, Qt::black);
    painter->restore();

    // 绘制红色背景（固定红色，不跟随主题）
    painter->fillPath(indicatorPath, QColor(0xEF4444)); // Tailwind Red-500

    // Hover效果：轻微变亮
    if (m_isHovered) {
        painter->save();
        painter->setOpacity(0.15);
        painter->fillPath(indicatorPath, Qt::white);
        painter->restore();
    }
}

void ThemeToggleButton::drawIcons(QPainter *painter)
{
    qreal sectionWidth = static_cast<qreal>(width()) / 3.0;
    qreal centerY = height() / 2.0;
    qreal iconHalfSize = m_iconSize / 2.0;

    QPointF positions[3] = {
        QPointF(sectionWidth * 0.5, centerY),   // Dark
        QPointF(sectionWidth * 1.5, centerY),   // Light
        QPointF(sectionWidth * 2.5, centerY)    // EyeCare
    };

    QSvgRenderer *icons[3] = {m_iconDark, m_iconLight, m_iconEyeCare};

    for (int i = 0; i < 3; ++i) {
        QColor iconColor = calculateIconColor(i);

        QRectF targetRect(
            positions[i].x() - iconHalfSize,
            positions[i].y() - iconHalfSize,
            m_iconSize,
            m_iconSize
            );

        // 1. 渲染SVG到透明图像
        QImage iconImage(m_iconSize, m_iconSize, QImage::Format_ARGB32_Premultiplied);
        iconImage.fill(Qt::transparent);

        QPainter svgPainter(&iconImage);
        icons[i]->render(&svgPainter);
        svgPainter.end();

        // 2. 创建颜色层并填充目标颜色
        QImage coloredImage(m_iconSize, m_iconSize, QImage::Format_ARGB32_Premultiplied);
        coloredImage.fill(iconColor);  // 这里填充白色（如果是选中状态）

        // 3. 使用 SourceIn 混合：保留颜色，但只在SVG形状处显示
        QPainter blendPainter(&coloredImage);
        blendPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        blendPainter.drawImage(0, 0, iconImage);
        blendPainter.end();

        // 4. 绘制到主画布
        painter->drawImage(targetRect.topLeft(), coloredImage);
    }
}

QColor ThemeToggleButton::calculateIconColor(int index) const
{
    // 计算当前指示器位置与图标索引的距离
    qreal distance = qAbs(m_indicatorPosition - static_cast<qreal>(index));

    // 距离为0时完全激活，距离>=1时完全未激活
    qreal activation = 1.0 - qBound(0.0, distance, 1.0);

    // 选中状态（activation接近1）：强制纯白色，方便在红色背景上显示
    // 未选中状态（activation接近0）：使用主题中性色，半透明
    if (activation > 0.9) {
        // 选中状态：纯白色，完全不透明
        return QColor(255, 255, 255, 255);
    } else if (activation < 0.1) {
        // 完全未选中：中性色，半透明
        QColor c = m_iconColorNormal;
        c.setAlphaF(0.4);
        return c;
    } else {
        // 过渡状态：插值计算
        QColor result;
        result.setRedF(m_iconColorNormal.redF() * (1 - activation) + 1.0 * activation);
        result.setGreenF(m_iconColorNormal.greenF() * (1 - activation) + 1.0 * activation);
        result.setBlueF(m_iconColorNormal.blueF() * (1 - activation) + 1.0 * activation);
        result.setAlphaF(0.4 + 0.6 * activation);
        return result;
    }
}

void ThemeToggleButton::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    m_pathCacheValid = false;  // 尺寸变化，路径缓存失效
}

void ThemeToggleButton::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event)
    m_isHovered = true;
    update();
}

void ThemeToggleButton::leaveEvent(QEvent *event)
{
    Q_UNUSED(event)
    m_isHovered = false;
    update();
}
