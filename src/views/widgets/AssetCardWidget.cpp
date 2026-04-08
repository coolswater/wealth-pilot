// AssetCardWidget.cpp
#include "AssetCardWidget.h"
#include <QPainter>
#include <QGraphicsDropShadowEffect>

AssetCardWidget::AssetCardWidget(QWidget* parent)
    : QFrame(parent)
{
    setupUI();
    setProperty("class", "AssetCardWidget");
    setFrameShape(QFrame::NoFrame);

    // 启用鼠标跟踪以获得更好的交互体验
    setMouseTracking(true);
}

AssetCardWidget::~AssetCardWidget() = default;

void AssetCardWidget::setupUI()
{
    // 主布局
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(12);

    // 顶部区域：图标和趋势
    auto* topLayout = new QHBoxLayout(this);
    topLayout->setSpacing(10);

    m_iconLabel = new QLabel(this);
    m_iconLabel->setFixedSize(40, 40);
    m_iconLabel->setAlignment(Qt::AlignCenter);
    m_iconLabel->setObjectName("cardIcon");

    topLayout->addWidget(m_iconLabel);
    topLayout->addStretch();

    m_trendLabel = new QLabel(this);
    m_trendLabel->setObjectName("trendLabel");
    m_trendLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    topLayout->addWidget(m_trendLabel);

    mainLayout->addLayout(topLayout);

    // 标题
    m_titleLabel = new QLabel(this);
    m_titleLabel->setObjectName("cardTitle");
    m_titleLabel->setProperty("secondary", "true");
    mainLayout->addWidget(m_titleLabel);

    // 主数值
    m_valueLabel = new QLabel(this);
    m_valueLabel->setObjectName("cardValue");
    QFont valueFont = m_valueLabel->font();
    valueFont.setPointSize(24);
    valueFont.setBold(true);
    m_valueLabel->setFont(valueFont);
    mainLayout->addWidget(m_valueLabel);

    // 副数值
    m_subValueLabel = new QLabel(this);
    m_subValueLabel->setObjectName("cardSubValue");
    m_subValueLabel->setProperty("tertiary", "true");
    mainLayout->addWidget(m_subValueLabel);

    mainLayout->addStretch();

    // 设置固定高度以保持统一
    setMinimumHeight(160);
    setMaximumHeight(180);

    // 初始化动画
    m_valueAnimation = new QPropertyAnimation(this, "value", this);
    m_valueAnimation->setDuration(800);
    m_valueAnimation->setEasingCurve(QEasingCurve::OutCubic);
}

void AssetCardWidget::setTitle(const QString& title) const
{
    m_titleLabel->setText(title);
}

void AssetCardWidget::setValue(const QString& value, bool animated)
{
    if (!animated) {
        m_valueLabel->setText(value);
        return;
    }

    // 解析数值进行动画
    QString numStr = value;
    // 修复后：
    numStr.remove(QStringLiteral("¥"))
        .remove(QStringLiteral(","))
        .remove(QStringLiteral("%"))
        .remove(QStringLiteral("+"));
    bool ok;
    qreal targetVal = numStr.toDouble(&ok);

    if (ok) {
        m_targetValue = targetVal;
        m_valueAnimation->setStartValue(m_currentValue);
        m_valueAnimation->setEndValue(targetVal);
        m_valueAnimation->start();
    } else {
        m_valueLabel->setText(value);
    }
}

void AssetCardWidget::setSubValue(const QString& subValue) const
{
    m_subValueLabel->setText(subValue);
}

void AssetCardWidget::setTrend(double percent, const QString& label) const
{
    QString trendText;
    QString styleClass;

    if (percent > 0) {
        trendText = QString("↗ +%1%").arg(percent, 0, 'f', 2);
        styleClass = "up";
    } else if (percent < 0) {
        trendText = QString("↘ %1%").arg(percent, 0, 'f', 2);
        styleClass = "down";
    } else {
        trendText = "→ 0.00%";
        styleClass = "flat";
    }

    if (!label.isEmpty()) {
        trendText = label;
    }

    m_trendLabel->setText(trendText);
    m_trendLabel->setProperty("class", styleClass);
    // updateStyle();
}

void AssetCardWidget::setIcon(const QString& iconPath) const
{
    m_iconLabel->setPixmap(QPixmap(iconPath).scaled(24, 24, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void AssetCardWidget::setTheme(CardTheme theme)
{
    m_theme = theme;
    QString themeStr;
    switch (theme) {
    case Success: themeStr = "success"; break;
    case Danger: themeStr = "danger"; break;
    case Warning: themeStr = "warning"; break;
    case Primary: themeStr = "primary"; break;
    default: themeStr = ""; break;
    }
    setProperty("theme", themeStr);
    // updateStyle();
}

void AssetCardWidget::setHoverEffect(bool enabled)
{
    m_hoverEffect = enabled;
}

void AssetCardWidget::setValue(qreal value)
{
    m_currentValue = value;
    // 根据原始格式更新显示
    emit valueChanged(value);
}

void AssetCardWidget::enterEvent(QEnterEvent* event)
{
    QFrame::enterEvent(event);
    if (m_hoverEffect) {
        setCursor(Qt::PointingHandCursor);
        // 悬停放大效果
        if (!m_scaleAnimation) {
            m_scaleAnimation = new QPropertyAnimation(this, "geometry", this);
            m_scaleAnimation->setDuration(150);
        }
    }
}

void AssetCardWidget::leaveEvent(QEvent* event)
{
    QFrame::leaveEvent(event);
    unsetCursor();
}

void AssetCardWidget::mousePressEvent(QMouseEvent* event)
{
    QFrame::mousePressEvent(event);
    emit clicked();
}

void AssetCardWidget::paintEvent(QPaintEvent* event)
{
    QFrame::paintEvent(event);

    // 自定义绘制：添加顶部边框色条
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QColor topColor;
    switch (m_theme) {
    case Success: topColor = QColor(0x10B981); break;
    case Danger: topColor = QColor(0xEF4444); break;
    case Warning: topColor = QColor(0xF97316); break;
    case Primary: topColor = QColor(0x3B82F6); break;
    default: topColor = QColor(0x3B82F6); break;
    }

    // 绘制顶部3px色条
    painter.fillRect(0, 0, width(), 3, topColor);
}
