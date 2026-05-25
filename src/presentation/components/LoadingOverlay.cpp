/**
 * @file LoadingOverlay.cpp
 * @brief 加载遮罩组件实现
 */

#include "LoadingOverlay.h"
#include "infrastructure/config/Tokens.h"
#include <QPainter>
#include <QPainterPath>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsDropShadowEffect>

struct LoadingOverlay::Impl
{
    QWidget* container = nullptr;
    QLabel* spinnerLabel = nullptr;
    QLabel* textLabel = nullptr;
    QProgressBar* progressBar = nullptr;
    QTimer* animationTimer = nullptr;
    int rotationAngle = 0;
    bool isLoading = false;
};

LoadingOverlay::LoadingOverlay(QWidget* parent)
    : QWidget(parent)
      , d(std::make_unique<Impl>())
{
    setupUI();
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    setAttribute(Qt::WA_TranslucentBackground, true);
    hide();
}

LoadingOverlay::~LoadingOverlay()
{
    stopAnimation();
}

void LoadingOverlay::setupUI()
{
    // 主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setAlignment(Qt::AlignCenter);

    // 容器
    d->container = new QWidget(this);
    d->container->setObjectName("loadingContainer");
    d->container->setFixedSize(200, 120);
    d->container->setStyleSheet(R"(
        #loadingContainer {
            background-color: rgba(22, 27, 34, 0.95);
            border-radius: 12px;
            border: 1px solid #30363d;
        }
    )");

    QVBoxLayout* containerLayout = new QVBoxLayout(d->container);
    containerLayout->setContentsMargins(20, 16, 20, 16);
    containerLayout->setSpacing(12);
    containerLayout->setAlignment(Qt::AlignCenter);

    // 旋转图标
    d->spinnerLabel = new QLabel(d->container);
    d->spinnerLabel->setFixedSize(32, 32);
    d->spinnerLabel->setText("⏳");
    d->spinnerLabel->setAlignment(Qt::AlignCenter);
    d->spinnerLabel->setStyleSheet("font-size: 28px; background: transparent;");
    containerLayout->addWidget(d->spinnerLabel, 0, Qt::AlignCenter);

    // 文本标签
    d->textLabel = new QLabel(QStringLiteral("加载中..."), d->container);
    d->textLabel->setAlignment(Qt::AlignCenter);
    d->textLabel->setStyleSheet(R"(
        color: #e6edf3;
        font-size: 14px;
        background: transparent;
    )");
    containerLayout->addWidget(d->textLabel);

    // 进度条
    d->progressBar = new QProgressBar(d->container);
    d->progressBar->setRange(0, 100);
    d->progressBar->setValue(0);
    d->progressBar->setTextVisible(false);
    d->progressBar->setFixedHeight(4);
    d->progressBar->setStyleSheet(R"(
        QProgressBar {
            background-color: #30363d;
            border: none;
            border-radius: 2px;
        }
        QProgressBar::chunk {
            background-color: #58a6ff;
            border-radius: 2px;
        }
    )");
    d->progressBar->hide();
    containerLayout->addWidget(d->progressBar);

    mainLayout->addWidget(d->container);

    // 动画定时器
    d->animationTimer = new QTimer(this);
    connect(d->animationTimer, &QTimer::timeout, this, [this]()
    {
        d->rotationAngle = (d->rotationAngle + 15) % 360;
        update();
    });
}

void LoadingOverlay::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 半透明背景
    painter.fillRect(rect(), QColor(0, 0, 0, 150));

    // 绘制旋转圆圈
    if (d->isLoading)
    {
        QPainterPath path;
        int centerX = d->spinnerLabel->geometry().center().x();
        int centerY = d->spinnerLabel->geometry().center().y();
        int radius = 14;

        painter.setPen(QPen(QColor(Tokens::Colors::Primary), 3, Qt::SolidLine, Qt::RoundCap));
        painter.translate(centerX, centerY);
        painter.rotate(d->rotationAngle);

        // 绘制弧线
        QRectF arcRect(-radius, -radius, radius * 2, radius * 2);
        painter.drawArc(arcRect, 0, 270 * 16);

        painter.resetTransform();
    }
}

void LoadingOverlay::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    // 确保遮罩覆盖整个父窗口
    if (parentWidget())
    {
        setGeometry(parentWidget()->rect());
    }
}

void LoadingOverlay::setText(const QString& text)
{
    if (d->textLabel)
    {
        d->textLabel->setText(text);
    }
}

void LoadingOverlay::setProgress(int value)
{
    if (d->progressBar)
    {
        d->progressBar->setValue(qBound(0, value, 100));
        d->progressBar->setVisible(value > 0);
    }
}

void LoadingOverlay::showLoading()
{
    d->isLoading = true;
    startAnimation();
    show();
    raise();
}

void LoadingOverlay::hideLoading()
{
    d->isLoading = false;
    stopAnimation();
    hide();
}

bool LoadingOverlay::isLoading() const
{
    return d->isLoading;
}

void LoadingOverlay::startAnimation()
{
    if (d->animationTimer && !d->animationTimer->isActive())
    {
        d->animationTimer->start(50);
    }
}

void LoadingOverlay::stopAnimation()
{
    if (d->animationTimer && d->animationTimer->isActive())
    {
        d->animationTimer->stop();
    }
}
