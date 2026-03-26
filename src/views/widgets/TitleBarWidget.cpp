// TitleBarWidget.cpp
#include "SvgIcon.h"
#include "SvgIconEngine.h"
#include "TitleBarWidget.h"

#include <QLabel>
#include <QPushButton>
#include <QMouseEvent>
#include <QHBoxLayout>
#include <QWindowStateChangeEvent>
#include <QGraphicsDropShadowEffect>
#include <QFile>

#include <core/ThemeManager.h>

struct TitleBarWidget::Impl {
    // UI组件
    QLabel *m_iconLabel = nullptr;           // 窗口图标
    QLabel *m_titleLabel = nullptr;          // 窗口标题
    QPushButton *m_minimizeBtn = nullptr;    // 最小化按钮
    QPushButton *m_maximizeBtn = nullptr;    // 最大化/还原按钮
    QPushButton *m_closeBtn = nullptr;         // 关闭按钮

    // 拖动相关变量
    QPoint m_dragStartPos;           // 拖动起始鼠标位置（全局坐标）
    QPoint m_dragStartWindowPos;     // 拖动起始窗口位置
    bool m_isDragging = false;       // 是否正在拖动
    bool m_isMaximized = false;      // 当前是否最大化/全屏

    QWidget *m_mainWindow = nullptr; // 父窗口指针（主窗口）
};

TitleBarWidget::TitleBarWidget(QWidget *parent)
    : QWidget(parent)
    , d(std::make_unique<Impl>())
{
    d->m_mainWindow = parent;

    // 设置固定高度（标准标题栏高度36-40像素）
    setFixedHeight(40);

    // 重要：启用鼠标跟踪，否则mouseMoveEvent在按住按钮时才触发
    setMouseTracking(true);

    // 初始化UI和连接
    setupUI();
    initConnections();
    applyThemeStyle();  // 应用初始主题样式

    // 监听父窗口状态变化
    if (d->m_mainWindow) {
        d->m_mainWindow->installEventFilter(this);
    }
}

TitleBarWidget::~TitleBarWidget() = default;

void TitleBarWidget::setupUI()
{
    // 主布局 - 水平排列
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 0, 12, 0);  // 左右边距12，上下0
    layout->setSpacing(5);
    layout->setAlignment(Qt::AlignVCenter);

    // 左侧：Logo/图标 + 标题
    d->m_iconLabel = new QLabel(this);
    d->m_iconLabel->setFixedSize(24, 24);
    d->m_iconLabel->setScaledContents(true);
    layout->addWidget(d->m_iconLabel);

    // 应用图标（默认应用图标）
    d->m_iconLabel->setPixmap(QPixmap(":/images/app_icon.png"));

    d->m_titleLabel = new QLabel(tr("领航财富 - 您的AI助理"), this);
    d->m_titleLabel->setObjectName("titleLabel");  // 用于QSS样式
    layout->addWidget(d->m_titleLabel);

    // 中间：弹性空间（将右侧按钮推到右边）
    layout->addStretch(1);

    // 右侧：主题切换 + 窗口控制按钮
    // d->m_themeToggle = new ThemeToggleButton(this);
    // layout->addWidget(d->m_themeToggle);

    // 添加小间距分隔
    layout->addSpacing(12);

    QFile file(":/icons/minus.svg");
    if (!file.exists()) {
        qCritical() << "SVG resource not found!";
    }

    // 最小化按钮SvgColorIcon
    QIcon minIcon = SvgIcon::themedIcon(":/icons/minus.svg", "foreground");
    d->m_minimizeBtn = new QPushButton(this);
    d->m_minimizeBtn->setProperty("icon", "true");
    d->m_minimizeBtn->setIcon(minIcon);
    d->m_minimizeBtn->setIconSize(QSize(16, 16));
    d->m_minimizeBtn->setFixedSize(20, 20);
    layout->addWidget(d->m_minimizeBtn);

    // 最大化/还原按钮
    QIcon maxIcon = SvgIcon::themedIcon(":/icons/maximize.svg", "foreground");
    d->m_maximizeBtn = new QPushButton(this);
    d->m_maximizeBtn->setProperty("icon", "true");
    d->m_maximizeBtn->setIcon(maxIcon);
    d->m_maximizeBtn->setIconSize(QSize(16, 16));
    d->m_maximizeBtn->setFixedSize(20, 20);
    layout->addWidget(d->m_maximizeBtn);

    // 关闭按钮
    QIcon closeIcon = SvgIcon::themedIcon(":/icons/close.svg", "foreground");
    d->m_closeBtn = new QPushButton(this);
    d->m_closeBtn->setProperty("icon", "true");
    d->m_closeBtn->setIcon(closeIcon);
    d->m_closeBtn->setIconSize(QSize(16, 16));
    d->m_closeBtn->setFixedSize(20, 20);
    layout->addWidget(d->m_closeBtn);

    // 设置布局
    setLayout(layout);

}

void TitleBarWidget::initConnections()
{
    // 连接按钮信号
    connect(d->m_minimizeBtn, &QPushButton::clicked,
            this, &TitleBarWidget::onMinimizeClicked);
    connect(d->m_maximizeBtn, &QPushButton::clicked,
            this, &TitleBarWidget::onMaximizeClicked);
    connect(d->m_closeBtn, &QPushButton::clicked,
            this, &TitleBarWidget::onCloseClicked);

    // 连接主题管理器的信号到主窗口槽
    // connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
    //         this, &TitleBarWidget::onThemeChanged);
}

void TitleBarWidget::applyThemeStyle()
{
    auto tm = ThemeManager::instance();
    if (!tm) return;

    // 动态构建样式表，使用ThemeManager当前颜色

     // Tokens tokens = tm->tokens();
    // 强制刷新
    update();
}

void TitleBarWidget::setTitle(const QString& title)
{
    if (d->m_titleLabel) {
        d->m_titleLabel->setText(title);
    }
}

void TitleBarWidget::setWindowIcon(const QPixmap& icon)
{
    if (d->m_iconLabel) {
        d->m_iconLabel->setPixmap(icon.scaled(24, 24, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

void TitleBarWidget::updateMaximizeButton(bool isMaximized)
{
    d->m_isMaximized = isMaximized;
    if (isMaximized) {
        QIcon restoreIcon = SvgIcon::themedIcon(":/icons/restore.svg", "foreground");
        d->m_maximizeBtn->setIcon(restoreIcon);
    } else {
        QIcon maxIcon = SvgIcon::themedIcon(":/icons/maximize.svg", "foreground");
        d->m_maximizeBtn->setIcon(maxIcon);  // 最大化图标
    }
}

// 鼠标事件实现 - 窗口拖动
void TitleBarWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && d->m_mainWindow) {
        d->m_isDragging = true;
        d->m_dragStartPos = event->globalPosition().toPoint();
        d->m_dragStartWindowPos = d->m_mainWindow->frameGeometry().topLeft();
    }
    QWidget::mousePressEvent(event);
}

void TitleBarWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (d->m_isDragging && (event->buttons() & Qt::LeftButton) && d->m_mainWindow) {
        QPoint delta = event->globalPosition().toPoint() - d->m_dragStartPos;

        // 如果窗口最大化，先恢复普通状态再拖动
        if (d->m_mainWindow->isMaximized() || d->m_mainWindow->isFullScreen()) {
            d->m_mainWindow->showNormal();
            d->m_isMaximized = false;
            updateMaximizeButton(false);

            // 重新计算位置，使鼠标位于标题栏中心附近
            int newX = event->globalPosition().toPoint().x() - d->m_mainWindow->width() / 2;
            int newY = event->globalPosition().toPoint().y() - 10;
            d->m_dragStartWindowPos = QPoint(newX, newY);
            d->m_dragStartPos = event->globalPosition().toPoint();
        } else {
            d->m_mainWindow->move(d->m_dragStartWindowPos + delta);
        }
    }
    QWidget::mouseMoveEvent(event);
}

void TitleBarWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        d->m_isDragging = false;
    }
    QWidget::mouseReleaseEvent(event);
}

void TitleBarWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // 检查点击位置是否有子控件，没有才触发最大化
        QWidget* child = childAt(event->position().toPoint());
        if (!child) {
            onMaximizeClicked();
            return;
        }
    }
    QWidget::mouseDoubleClickEvent(event);
}

bool TitleBarWidget::eventFilter(QObject *watched, QEvent *event)
{
    // 监听父窗口状态变化，同步最大化按钮图标
    if (watched == d->m_mainWindow && event->type() == QEvent::WindowStateChange) {
        bool isMax = d->m_mainWindow->isMaximized() || d->m_mainWindow->isFullScreen();
        if (isMax != d->m_isMaximized) {
            updateMaximizeButton(isMax);
        }
    }
    return QWidget::eventFilter(watched, event);
}

// 槽函数实现
void TitleBarWidget::onThemeChanged()
{
    // 重新应用样式以匹配新主题
    applyThemeStyle();

    // 如果ThemeToggleButton存在，它也会自动响应，但我们需要确保一致性
    // 实际切换逻辑在ThemeToggleButton中处理，这里只更新标题栏样式
}

void TitleBarWidget::onMinimizeClicked()
{
    if (d->m_mainWindow) {
        d->m_mainWindow->showMinimized();
    }
}

void TitleBarWidget::onMaximizeClicked()
{
    if (!d->m_mainWindow) return;

    if (d->m_mainWindow->isFullScreen()) {
        d->m_mainWindow->showNormal();
        d->m_isMaximized = false;
    } else if (d->m_mainWindow->isMaximized()) {
        d->m_mainWindow->showNormal();
        d->m_isMaximized = false;
    } else {
        d->m_mainWindow->showMaximized();
        d->m_isMaximized = true;
    }
    updateMaximizeButton(d->m_isMaximized);
}

void TitleBarWidget::onCloseClicked()
{
    if (d->m_mainWindow) {
        d->m_mainWindow->close();
    }
}
