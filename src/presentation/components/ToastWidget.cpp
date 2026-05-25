/**
 * @file ToastWidget.cpp
 * @brief Toast 提示控件实现
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "ToastWidget.h"
#include <QApplication>
#include <QScreen>
#include <QPainter>
#include <QFont>
#include <QDebug>

namespace WealthPilot {
namespace UI {

// ========== ToastItem 实现 ==========

ToastItem::ToastItem(const QString& title, const QString& message,
                     FeedbackType type, int duration,
                     QWidget* parent)
    : QWidget(parent)
    , m_title(title)
    , m_message(message)
    , m_type(type)
    , m_duration(duration) {
    setupUI();
    applyStyle();
}

ToastItem::~ToastItem() {
    if (m_fadeInAnimation) {
        m_fadeInAnimation->stop();
        delete m_fadeInAnimation;
    }
    if (m_fadeOutAnimation) {
        m_fadeOutAnimation->stop();
        delete m_fadeOutAnimation;
    }
    if (m_autoHideTimer) {
        m_autoHideTimer->stop();
        delete m_autoHideTimer;
    }
}

void ToastItem::setupUI() {
    // 设置窗口属性
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);

    // 创建容器
    m_container = new QWidget(this);
    m_container->setObjectName("toastContainer");

    // 创建布局
    auto* layout = new QVBoxLayout(m_container);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(8);

    // 创建标题行
    auto* titleLayout = new QHBoxLayout();
    titleLayout->setSpacing(8);

    // 图标
    m_iconLabel = new QLabel();
    m_iconLabel->setFixedSize(24, 24);
    m_iconLabel->setAlignment(Qt::AlignCenter);
    titleLayout->addWidget(m_iconLabel);

    // 标题
    m_titleLabel = new QLabel(m_title);
    m_titleLabel->setObjectName("toastTitle");
    QFont titleFont = m_titleLabel->font();
    titleFont.setBold(true);
    titleFont.setPointSize(10);
    m_titleLabel->setFont(titleFont);
    titleLayout->addWidget(m_titleLabel, 1);

    layout->addLayout(titleLayout);

    // 消息内容
    if (!m_message.isEmpty()) {
        m_messageLabel = new QLabel(m_message);
        m_messageLabel->setObjectName("toastMessage");
        m_messageLabel->setWordWrap(true);
        QFont messageFont = m_messageLabel->font();
        messageFont.setPointSize(9);
        m_messageLabel->setFont(messageFont);
        layout->addWidget(m_messageLabel);
    }

    // 主布局
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(m_container);

    // 设置固定宽度
    setFixedWidth(350);
    setMinimumHeight(60);

    // 设置透明度效果
    m_opacityEffect = new QGraphicsOpacityEffect(this);
    m_opacityEffect->setOpacity(0.0);
    setGraphicsEffect(m_opacityEffect);
}

void ToastItem::applyStyle() {
    // 根据类型设置样式和图标
    QString iconChar;
    QString bgColor;
    QString borderColor;
    QString textColor;

    switch (m_type) {
        case FeedbackType::Info:
            iconChar = "ℹ";
            bgColor = "#E3F2FD";
            borderColor = "#2196F3";
            textColor = "#1976D2";
            break;
        case FeedbackType::Warning:
            iconChar = "⚠";
            bgColor = "#FFF3E0";
            borderColor = "#FF9800";
            textColor = "#F57C00";
            break;
        case FeedbackType::Error:
            iconChar = "✕";
            bgColor = "#FFEBEE";
            borderColor = "#F44336";
            textColor = "#D32F2F";
            break;
        case FeedbackType::Success:
            iconChar = "✓";
            bgColor = "#E8F5E9";
            borderColor = "#4CAF50";
            textColor = "#388E3C";
            break;
    }

    // 设置图标
    m_iconLabel->setText(iconChar);
    m_iconLabel->setStyleSheet(QString(
        "QLabel {"
        "  color: %1;"
        "  font-size: 18px;"
        "  font-weight: bold;"
        "}"
    ).arg(textColor));

    // 设置标题颜色
    m_titleLabel->setStyleSheet(QString(
        "QLabel {"
        "  color: %1;"
        "}"
    ).arg(textColor));

    // 设置消息颜色
    if (m_messageLabel) {
        m_messageLabel->setStyleSheet(QString(
            "QLabel {"
            "  color: #424242;"
            "}"
        ));
    }

    // 设置容器样式
    m_container->setStyleSheet(QString(
        "QWidget#toastContainer {"
        "  background-color: %1;"
        "  border: 2px solid %2;"
        "  border-radius: 8px;"
        "}"
    ).arg(bgColor, borderColor));
}

void ToastItem::show() {
    QWidget::show();

    // 启动淡入动画
    m_fadeInAnimation = new QPropertyAnimation(m_opacityEffect, "opacity", this);
    m_fadeInAnimation->setDuration(200);
    m_fadeInAnimation->setStartValue(0.0);
    m_fadeInAnimation->setEndValue(1.0);
    m_fadeInAnimation->start();

    // 设置自动隐藏定时器
    m_autoHideTimer = new QTimer(this);
    m_autoHideTimer->setSingleShot(true);
    connect(m_autoHideTimer, &QTimer::timeout, this, &ToastItem::hide);
    m_autoHideTimer->start(m_duration);
}

void ToastItem::hide() {
    // 启动淡出动画
    m_fadeOutAnimation = new QPropertyAnimation(m_opacityEffect, "opacity", this);
    m_fadeOutAnimation->setDuration(200);
    m_fadeOutAnimation->setStartValue(1.0);
    m_fadeOutAnimation->setEndValue(0.0);

    connect(m_fadeOutAnimation, &QPropertyAnimation::finished, [this]() {
        QWidget::hide();
        emit finished();
    });

    m_fadeOutAnimation->start();
}

// ========== ToastManager 实现 ==========

ToastManager* ToastManager::instance() {
    static ToastManager instance;
    return &instance;
}

ToastManager::ToastManager(QWidget* parent)
    : QWidget(parent) {
    // 设置窗口属性
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents);

    // 获取屏幕大小
    QScreen* screen = QApplication::primaryScreen();
    if (screen) {
        setGeometry(screen->availableGeometry());
    }
}

ToastManager::~ToastManager() {
    clearAll();
}

void ToastManager::showToast(const QString& title, const QString& message,
                              FeedbackType type, int duration) {
    // 添加到队列
    m_queue.enqueue(std::make_tuple(title, message, type, duration));

    // 处理队列
    processQueue();
}

void ToastManager::setPosition(ToastPosition position) {
    m_position = position;
    updateLayout();
}

void ToastManager::clearAll() {
    // 清除所有活跃的 Toast
    for (auto* toast : m_activeToasts) {
        if (toast) {
            toast->close();
            toast->deleteLater();
        }
    }
    m_activeToasts.clear();

    // 清空队列
    m_queue.clear();
}

void ToastManager::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    updateLayout();
}

void ToastManager::updateLayout() {
    if (m_activeToasts.isEmpty()) {
        return;
    }

    // 计算起始位置
    int x = 0;
    int y = 0;
    const int margin = 20;

    QRect screenRect = geometry();
    int totalHeight = 0;

    // 计算总高度
    for (const auto* toast : m_activeToasts) {
        totalHeight += toast->height() + m_spacing;
    }

    // 根据位置计算坐标
    switch (m_position) {
        case ToastPosition::TopLeft:
            x = margin;
            y = margin;
            break;
        case ToastPosition::TopCenter:
            x = (screenRect.width() - m_activeToasts.first()->width()) / 2;
            y = margin;
            break;
        case ToastPosition::TopRight:
            x = screenRect.width() - m_activeToasts.first()->width() - margin;
            y = margin;
            break;
        case ToastPosition::BottomLeft:
            x = margin;
            y = screenRect.height() - totalHeight - margin;
            break;
        case ToastPosition::BottomCenter:
            x = (screenRect.width() - m_activeToasts.first()->width()) / 2;
            y = screenRect.height() - totalHeight - margin;
            break;
        case ToastPosition::BottomRight:
            x = screenRect.width() - m_activeToasts.first()->width() - margin;
            y = screenRect.height() - totalHeight - margin;
            break;
        case ToastPosition::Center:
            x = (screenRect.width() - m_activeToasts.first()->width()) / 2;
            y = (screenRect.height() - totalHeight) / 2;
            break;
    }

    // 更新每个 Toast 的位置
    for (auto* toast : m_activeToasts) {
        toast->move(x, y);
        y += toast->height() + m_spacing;
    }
}

void ToastManager::processQueue() {
    // 如果达到最大显示数量，则等待
    if (m_activeToasts.size() >= m_maxVisible) {
        return;
    }

    // 如果队列为空，则返回
    if (m_queue.isEmpty()) {
        return;
    }

    // 从队列中取出
    auto [title, message, type, duration] = m_queue.dequeue();

    // 创建 Toast
    auto* toast = new ToastItem(title, message, type, duration, this);
    connect(toast, &ToastItem::finished, this, [this, toast]() {
        removeToast(toast);
    });

    // 添加到活跃列表
    m_activeToasts.append(toast);

    // 显示
    toast->show();

    // 更新布局
    updateLayout();
}

void ToastManager::removeToast(ToastItem* toast) {
    m_activeToasts.removeOne(toast);

    if (toast) {
        toast->close();
        toast->deleteLater();
    }

    // 更新布局
    updateLayout();

    // 处理队列中的下一个
    processQueue();
}

} // namespace UI
} // namespace WealthPilot