/**
 * @file ThemeToggleButton.cpp
 * @brief 动态主题切换按钮实现
 */

#include "ThemeToggleButton.h"
#include "core/config/Tokens.h"
#include <QPainterPath>
#include <QtMath>

// SVG图标数据（内嵌资源）
static const char* SVG_SUN =
    R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="5"></circle><line x1="12" y1="1" x2="12" y2="3"></line><line x1="12" y1="21" x2="12" y2="23"></line><line x1="4.22" y1="4.22" x2="5.64" y2="5.64"></line><line x1="18.36" y1="18.36" x2="19.78" y2="19.78"></line><line x1="1" y1="12" x2="3" y2="12"></line><line x1="21" y1="12" x2="23" y2="12"></line><line x1="4.22" y1="19.78" x2="5.64" y2="18.36"></line><line x1="18.36" y1="5.64" x2="19.78" y2="4.22"></line></svg>)";

static const char* SVG_MOON =
    R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M21 12.79A9 9 0 1 1 11.21 3 7 7 0 0 0 21 12.79z"></path></svg>)";

static const char* SVG_EYE =
    R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"></path><circle cx="12" cy="12" r="3"></circle></svg>)";

// 静态成员初始化
QSvgRenderer* ThemeToggleButton::s_sunIcon = nullptr;
QSvgRenderer* ThemeToggleButton::s_moonIcon = nullptr;
QSvgRenderer* ThemeToggleButton::s_eyeIcon = nullptr;

ThemeToggleButton::ThemeToggleButton(QWidget* parent)
    : QWidget(parent)
      , m_positionAnimation(new QPropertyAnimation(this, "indicatorPosition", this))
      , m_colorAnimation(new QPropertyAnimation(this, "backgroundColor", this))
{
    initialize();
}

ThemeToggleButton::~ThemeToggleButton()
{
}

void ThemeToggleButton::initialize()
{
    setFixedSize(m_buttonSize);
    setCursor(Qt::PointingHandCursor);

    loadIcons();
    updateColors();

    // 设置初始位置
    m_indicatorPosition = positionFromTheme(ThemeType::Dark);

    // 动画配置
    m_positionAnimation->setDuration(300);
    m_positionAnimation->setEasingCurve(QEasingCurve::OutCubic);

    m_colorAnimation->setDuration(300);
    m_colorAnimation->setEasingCurve(QEasingCurve::OutCubic);
}

void ThemeToggleButton::loadIcons()
{
    if (!s_sunIcon)
    {
        s_sunIcon = new QSvgRenderer(QByteArray(SVG_SUN));
    }
    if (!s_moonIcon)
    {
        s_moonIcon = new QSvgRenderer(QByteArray(SVG_MOON));
    }
    if (!s_eyeIcon)
    {
        s_eyeIcon = new QSvgRenderer(QByteArray(SVG_EYE));
    }
}

void ThemeToggleButton::updateColors()
{
    m_currentBgColor = QColor(Tokens::Colors::BgElevated);
    m_indicatorColor = QColor(Tokens::Colors::Primary);
    m_iconColorNormal = QColor(Tokens::Colors::TextSecondary);
    m_iconColorActive = QColor(Tokens::Colors::TextPrimary);
    update();
}

void ThemeToggleButton::setIconSize(int size)
{
    m_iconSize = size;
    update();
}

void ThemeToggleButton::setButtonSize(const QSize& size)
{
    m_buttonSize = size;
    setFixedSize(size);
    update();
}

void ThemeToggleButton::setIndicatorPosition(qreal position)
{
    if (qFuzzyCompare(m_indicatorPosition, position))
        return;

    m_indicatorPosition = position;
    emit indicatorPositionChanged(position);
    update();
}

void ThemeToggleButton::setBackgroundColor(const QColor& color)
{
    if (m_currentBgColor == color)
        return;

    m_currentBgColor = color;
    emit backgroundColorChanged(color);
    update();
}

void ThemeToggleButton::onThemeChanged(ThemeType newTheme)
{
    m_currentTheme = newTheme;
    animateTo(newTheme);
}

ThemeType ThemeToggleButton::themeFromPosition(qreal position) const
{
    if (position < 0.5) return ThemeType::Light;
    if (position < 1.5) return ThemeType::Dark;
    return ThemeType::HighContrast;
}

qreal ThemeToggleButton::positionFromTheme(ThemeType theme) const
{
    switch (theme)
    {
    case ThemeType::Light: return 0.0;
    case ThemeType::Dark: return 1.0;
    case ThemeType::HighContrast: return 2.0;
    default: return 1.0;
    }
}

void ThemeToggleButton::animateTo(ThemeType targetTheme)
{
    qreal targetPos = positionFromTheme(targetTheme);

    m_positionAnimation->stop();
    m_positionAnimation->setStartValue(m_indicatorPosition);
    m_positionAnimation->setEndValue(targetPos);
    m_positionAnimation->start();
}

void ThemeToggleButton::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    drawBackground(&painter);
    drawIndicator(&painter);
    drawIcons(&painter);
}

void ThemeToggleButton::drawBackground(QPainter* painter)
{
    QPainterPath path;
    path.addRoundedRect(rect(), m_cornerRadius, m_cornerRadius);

    painter->fillPath(path, m_currentBgColor);

    if (m_isHovered) {
        painter->fillPath(path, QColor(255, 255, 255, 20));
    }
}

void ThemeToggleButton::drawIndicator(QPainter* painter)
{
    qreal indicatorWidth = (width() - 2 * m_indicatorMargin) / 3.0;
    qreal x = m_indicatorMargin + m_indicatorPosition * indicatorWidth;

    QRectF indicatorRect(x, m_indicatorMargin,
                         indicatorWidth - 2 * m_indicatorMargin,
                         height() - 2 * m_indicatorMargin);

    QPainterPath path;
    path.addRoundedRect(indicatorRect, m_cornerRadius - 4, m_cornerRadius - 4);

    painter->fillPath(path, m_indicatorColor);
}

void ThemeToggleButton::drawIcons(QPainter* painter)
{
    qreal iconWidth = width() / 3.0;

    QSvgRenderer* icons[] = {s_sunIcon, s_moonIcon, s_eyeIcon};

    for (int i = 0; i < 3; ++i) {
        if (!icons[i] || !icons[i]->isValid())
            continue;

        qreal x = i * iconWidth + (iconWidth - m_iconSize) / 2.0;
        qreal y = (height() - m_iconSize) / 2.0;

        QRectF iconRect(x, y, m_iconSize, m_iconSize);

        QColor color = calculateIconColor(i);
        painter->setOpacity(color.alphaF());

        icons[i]->render(painter, iconRect);
    }

    painter->setOpacity(1.0);
}

QColor ThemeToggleButton::calculateIconColor(int index) const
{
    qreal distance = qAbs(m_indicatorPosition - static_cast<qreal>(index));

    if (distance < 0.5)
    {
        return m_iconColorActive;
    } else {
        return m_iconColorNormal;
    }
}

void ThemeToggleButton::mousePressEvent(QMouseEvent* event)
{
    Q_UNUSED(event);

    // 根据点击位置确定目标主题
    // 计算每个图标区域的中心位置
    qreal iconWidth = width() / 3.0;
    qreal clickX = event->position().x();

    ThemeType targetTheme;
    if (clickX < iconWidth)
    {
        targetTheme = ThemeType::Light;
    }
    else if (clickX < iconWidth * 2)
    {
        targetTheme = ThemeType::Dark;
    }
    else
    {
        targetTheme = ThemeType::HighContrast;
    }

    // 如果点击的是当前主题，则切换到下一个主题
    if (targetTheme == m_currentTheme)
    {
        switch (m_currentTheme)
        {
        case ThemeType::Light:
            targetTheme = ThemeType::Dark;
            break;
        case ThemeType::Dark:
            targetTheme = ThemeType::HighContrast;
            break;
        case ThemeType::HighContrast:
        default:
            targetTheme = ThemeType::Light;
            break;
        }
    }

    m_currentTheme = targetTheme;
    animateTo(targetTheme);
    emit themeSwitchRequested(targetTheme);
}

void ThemeToggleButton::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    update();
}

void ThemeToggleButton::enterEvent(QEnterEvent* event)
{
    Q_UNUSED(event);
    m_isHovered = true;
    update();
}

void ThemeToggleButton::leaveEvent(QEvent* event)
{
    Q_UNUSED(event);
    m_isHovered = false;
    update();
}
