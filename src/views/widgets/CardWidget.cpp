#include "CardWidget.h"

#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QPainter>

struct CardWidget::Impl {
    QString title;
    QLabel* titleLabel = nullptr;
    QLabel* iconLabel = nullptr;
    QWidget* contentWidget = nullptr;
    QVBoxLayout* mainLayout = nullptr;
    QVBoxLayout* contentLayout = nullptr;
    QColor customBorderColor;
    QColor customBgColor;
    bool hasCustomBorder = false;
    bool hasCustomBg = false;
};

CardWidget::CardWidget(const QString& title, QWidget *parent)
    : BaseWidget(parent)
    , d(std::make_unique<Impl>())
{
    d->title = title;
    setupUI();
    applyStyle();

    // 设置阴影效果
    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setColor(QColor(0, 0, 0, 60));
    shadow->setOffset(0, 4);
    shadow->setBlurRadius(20);
    setGraphicsEffect(shadow);
}

CardWidget::~CardWidget() = default;

void CardWidget::setTitle(const QString& title)
{
    d->title = title;
    if (d->titleLabel) {
        d->titleLabel->setText(title);
    }
}

QString CardWidget::title() const
{
    return d->title;
}

void CardWidget::setContent(QWidget* content)
{
    if (d->contentWidget) {
        d->contentLayout->removeWidget(d->contentWidget);
        d->contentWidget->deleteLater();
    }

    d->contentWidget = content;
    if (content) {
        d->contentLayout->addWidget(content);
    }
}

QWidget* CardWidget::content() const
{
    return d->contentWidget;
}

void CardWidget::setIcon(const QString& iconPath)
{
    if (d->iconLabel) {
        d->iconLabel->setPixmap(QPixmap(iconPath).scaled(24, 24, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

void CardWidget::setBorderColor(const QColor& color)
{
    d->customBorderColor = color;
    d->hasCustomBorder = true;
    applyStyle();
}

void CardWidget::resetBorderColor()
{
    d->hasCustomBorder = false;
    applyStyle();
}

void CardWidget::setBackgroundColor(const QColor& color)
{
    d->customBgColor = color;
    d->hasCustomBg = true;
    applyStyle();
}

void CardWidget::resetBackgroundColor()
{
    d->hasCustomBg = false;
    applyStyle();
}

void CardWidget::onHoverEnter()
{
    // 阴影增强动画
    QGraphicsDropShadowEffect* shadow = qobject_cast<QGraphicsDropShadowEffect*>(graphicsEffect());
    if (shadow) {
        QPropertyAnimation* anim = new QPropertyAnimation(shadow, "blurRadius");
        anim->setDuration(200);
        anim->setStartValue(20);
        anim->setEndValue(40);
        anim->setEasingCurve(QEasingCurve::OutCubic);
        anim->start(QAbstractAnimation::DeleteWhenStopped);

        // 偏移量变化
        QPropertyAnimation* offsetAnim = new QPropertyAnimation(shadow, "yOffset");
        offsetAnim->setDuration(200);
        offsetAnim->setStartValue(4);
        offsetAnim->setEndValue(8);
        offsetAnim->setEasingCurve(QEasingCurve::OutCubic);
        offsetAnim->start(QAbstractAnimation::DeleteWhenStopped);
    }

    // 边框高亮
    if (!d->hasCustomBorder) {
        setStyleSheet(styleSheet() + QString("\n            CardWidget {\n                border-color: rgba(59, 130, 246, 0.4);\n            }\n        "));
    }

    // 轻微上移
    QPropertyAnimation* moveAnim = new QPropertyAnimation(this, "pos");
    moveAnim->setDuration(200);
    moveAnim->setStartValue(pos());
    moveAnim->setEndValue(pos() + QPoint(0, -4));
    moveAnim->setEasingCurve(QEasingCurve::OutCubic);
    moveAnim->start(QAbstractAnimation::DeleteWhenStopped);
}

void CardWidget::onHoverLeave()
{
    // 恢复阴影
    QGraphicsDropShadowEffect* shadow = qobject_cast<QGraphicsDropShadowEffect*>(graphicsEffect());
    if (shadow) {
        QPropertyAnimation* anim = new QPropertyAnimation(shadow, "blurRadius");
        anim->setDuration(200);
        anim->setStartValue(40);
        anim->setEndValue(20);
        anim->setEasingCurve(QEasingCurve::OutCubic);
        anim->start(QAbstractAnimation::DeleteWhenStopped);

        QPropertyAnimation* offsetAnim = new QPropertyAnimation(shadow, "yOffset");
        offsetAnim->setDuration(200);
        offsetAnim->setStartValue(8);
        offsetAnim->setEndValue(4);
        offsetAnim->setEasingCurve(QEasingCurve::OutCubic);
        offsetAnim->start(QAbstractAnimation::DeleteWhenStopped);
    }

    // 恢复边框
    applyStyle();

    // 恢复位置
    QPropertyAnimation* moveAnim = new QPropertyAnimation(this, "pos");
    moveAnim->setDuration(200);
    moveAnim->setStartValue(pos());
    moveAnim->setEndValue(pos() + QPoint(0, 4));
    moveAnim->setEasingCurve(QEasingCurve::OutCubic);
    moveAnim->start(QAbstractAnimation::DeleteWhenStopped);
}

void CardWidget::mousePressEvent(QMouseEvent *event)
{
    BaseWidget::mousePressEvent(event);

    // 按下效果
    QPropertyAnimation* anim = new QPropertyAnimation(this, "scale");
    anim->setDuration(100);
    anim->setStartValue(1.0);
    anim->setEndValue(0.98);
    anim->setEasingCurve(QEasingCurve::InOutQuad);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void CardWidget::mouseReleaseEvent(QMouseEvent *event)
{
    BaseWidget::mouseReleaseEvent(event);

    // 释放效果
    QPropertyAnimation* anim = new QPropertyAnimation(this, "scale");
    anim->setDuration(100);
    anim->setStartValue(0.98);
    anim->setEndValue(1.0);
    anim->setEasingCurve(QEasingCurve::InOutQuad);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void CardWidget::setupUI()
{
    setMinimumSize(200, 120);

    d->mainLayout = new QVBoxLayout(this);
    d->mainLayout->setContentsMargins(20, 16, 20, 16);
    d->mainLayout->setSpacing(12);

    // 标题栏
    if (!d->title.isEmpty()) {
        QHBoxLayout* titleLayout = new QHBoxLayout();
        titleLayout->setSpacing(8);

        d->iconLabel = new QLabel(this);
        d->iconLabel->setFixedSize(24, 24);
        titleLayout->addWidget(d->iconLabel);

        d->titleLabel = new QLabel(d->title, this);
        d->titleLabel->setStyleSheet("font-size: 16px; font-weight: 600; color: white;");
        titleLayout->addWidget(d->titleLabel);

        titleLayout->addStretch();

        d->mainLayout->addLayout(titleLayout);
    }

    // 内容区域
    d->contentLayout = new QVBoxLayout();
    d->contentLayout->setSpacing(0);
    d->mainLayout->addLayout(d->contentLayout);

    d->mainLayout->addStretch();
}

void CardWidget::applyStyle()
{
    // ThemeManager* theme = ThemeManager::instance();

    QString bgColor = d->hasCustomBg ? d->customBgColor.name() : "rgba(255, 255, 255, 0.03)";
    QString borderColor = d->hasCustomBorder ? d->customBorderColor.name() : "rgba(255, 255, 255, 0.08)";

    setStyleSheet(QString(R"(
        CardWidget {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 12px;
        }
    )").arg(bgColor, borderColor));
}
