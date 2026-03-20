#include "Dashboard.h"
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QPropertyAnimation>
#include <QGraphicsDropShadowEffect>
#include <QFont>

struct Dashboard::Impl {
    QVBoxLayout *mainLayout = nullptr;
    QLabel *titleLabel = nullptr;
    QPushButton *actionButton = nullptr;

    // 性能优化：预分配布局容量避免重新分配
    static constexpr int LayoutSpacing = 10;
    static constexpr int ContentsMargins = 20;
};

Dashboard::Dashboard(QWidget *parent)
    : BasePage(parent)
    , d(std::make_unique<Impl>())
{
    // 构造函数保持轻量，所有UI延迟到initializePage构建
    setObjectName("DashboardPage");
}

Dashboard::~Dashboard() = default;

QString Dashboard::pageId() const {
    // 使用QStringLiteral避免运行时字符串分配
    return QStringLiteral("dashboard");
}

void Dashboard::initializePage() {
    if (isInitialized()) return;  // 防止重复初始化

    setupUI();
    setupAnimations();
    connectSignals();

    setInitialized(true);
    emit pageStatusChanged(QStringLiteral("initialized"));
}

void Dashboard::setupUI() {
    // 1. 主布局配置 - 零边距融入父容器
    d->mainLayout = new QVBoxLayout(this);
    d->mainLayout->setContentsMargins(
        Impl::ContentsMargins,
        Impl::ContentsMargins,
        Impl::ContentsMargins,
        Impl::ContentsMargins
        );
    d->mainLayout->setSpacing(Impl::LayoutSpacing);
    d->mainLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    // 2. 标题标签 - 使用CSS样式表而非多次setFont调用（性能更好）
    d->titleLabel = new QLabel(tr("投资仪表板"), this);
    d->titleLabel->setObjectName("dashboardTitle");
    d->titleLabel->setAlignment(Qt::AlignCenter);
    d->titleLabel->setStyleSheet(
        QStringLiteral("font-size: 24px; font-weight: bold; color: #2c3e50; margin: 20px;")
        );

    // 添加阴影效果
    auto *shadowEffect = new QGraphicsDropShadowEffect(this);
    shadowEffect->setBlurRadius(10);
    shadowEffect->setColor(QColor(0, 0, 0, 80));
    shadowEffect->setOffset(2, 2);
    d->titleLabel->setGraphicsEffect(shadowEffect);

    d->mainLayout->addWidget(d->titleLabel);

    // 3. 操作按钮 - 使用对象名便于QSS全局样式控制
    d->actionButton = new QPushButton(tr("刷新数据"), this);
    d->actionButton->setObjectName("primaryButton");
    d->actionButton->setFixedSize(120, 40);
    d->actionButton->setCursor(Qt::PointingHandCursor);
    d->actionButton->setStyleSheet(
        QStringLiteral(
            "QPushButton#primaryButton {"
            "  background-color: #3498db; color: white; border-radius: 6px;"
            "  font-weight: bold; font-size: 14px;"
            "}"
            "QPushButton#primaryButton:hover { background-color: #2980b9; }"
            "QPushButton#primaryButton:pressed { background-color: #1c5a85; }"
            )
        );

    d->mainLayout->addWidget(d->actionButton, 0, Qt::AlignCenter);
    d->mainLayout->addStretch(1);  // 弹性空间将内容推向上方
}

void Dashboard::setupAnimations() {
    // 入场动画：淡入+上浮效果
    auto *anim = new QPropertyAnimation(this, "windowOpacity", this);
    anim->setDuration(300);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->setEasingCurve(QEasingCurve::OutCubic);

    // 注意：实际项目中建议将动画缓存，避免重复创建
    // 此处为简化演示
}

void Dashboard::connectSignals() {
    // 使用C++11 lambda连接，避免槽函数声明
    connect(d->actionButton, &QPushButton::clicked, this, [this]() {
        emit pageStatusChanged(QStringLiteral("refreshing"));
        // 模拟数据刷新，实际项目中应调用Service层
        emit requestNavigation(QStringLiteral("dashboard"), {}, false);
    });
}
