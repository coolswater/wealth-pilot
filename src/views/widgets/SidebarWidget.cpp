#include "SidebarWidget.h"
#include "utils/Logger.h"

#include <QPushButton>
#include <QVBoxLayout>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QLabel>
#include <QIcon>

struct SidebarWidget::Impl {
    QVBoxLayout* layout = nullptr;
    QMap<QString, QPushButton*> items;
    QString currentId;
    QPushButton* toggleBtn = nullptr;
    QIcon collapseLeftIcon;
    QIcon collapseRightIcon;

    bool collapsed = false;
    int expandedWidth = 100;
    int collapsedWidth = 20;

    QWidget* logoWidget = nullptr;
};

SidebarWidget::SidebarWidget(QWidget *parent)
    : BaseWidget(parent)
    , d(std::make_unique<Impl>())
{
    setFixedWidth(d->expandedWidth);
    setupUI();
}

SidebarWidget::~SidebarWidget() = default;

void SidebarWidget::addItem(const QString& id, const QString& text, const QIcon& icon)
{
    auto* btn = new QPushButton(this);
    btn->setObjectName(id);
    btn->setCheckable(true);
    btn->setCursor(Qt::PointingHandCursor);

    // 直接使用传入的 QIcon（自动包含 SvgColorIcon 的主题感知能力）
    btn->setIcon(icon);
    btn->setIconSize(QSize(16, 16));

    btn->setText(text);
    btn->setFixedHeight(48);

    connect(btn, &QPushButton::clicked, this, &SidebarWidget::onItemClicked);

    d->items[id] = btn;
    d->layout->insertWidget(d->layout->count(), btn);
}

void SidebarWidget::addItem(const QString& id, const QString& text)
{
    auto* btn = new QPushButton(this);
    btn->setObjectName(id);
    btn->setCheckable(true);
    btn->setCursor(Qt::PointingHandCursor);

    btn->setText(text);
    btn->setFixedHeight(48);

    connect(btn, &QPushButton::clicked, this, &SidebarWidget::onItemClicked);

    d->items[id] = btn;
    d->layout->insertWidget(d->layout->count(), btn);
}

void SidebarWidget::setCollapseIcons(const QIcon& left, const QIcon& right) const
{
    d->collapseLeftIcon = left;
    d->collapseRightIcon = right;
    updateToggleButtonIcon();
}

void SidebarWidget::updateToggleButtonIcon() const
{
    const bool isCollapsed = d->collapsed;
    d->toggleBtn->setIcon(isCollapsed ? d->collapseRightIcon : d->collapseLeftIcon);
}

void SidebarWidget::setCurrentItem(const QString& id) const
{
    // 修复：即使当前已是选中状态，也要确保按钮保持 checked
    if (d->currentId == id) {
        if (!id.isEmpty() && d->items.contains(id)) {
            d->items[id]->setChecked(true);  // 强制保持选中状态
        }
        return;
    }

    // 取消之前的选中
    if (!d->currentId.isEmpty() && d->items.contains(d->currentId)) {
        d->items[d->currentId]->setChecked(false);
    }

    // 设置新的选中
    d->currentId = id;
    if (d->items.contains(id)) {
        d->items[id]->setChecked(true);
    }
}

QString SidebarWidget::currentItem() const
{
    return d->currentId;
}

void SidebarWidget::setCollapsed(bool collapsed)
{
    if (d->collapsed == collapsed) return;

    d->collapsed = collapsed;
    animateCollapse(collapsed);
    emit collapsedChanged(collapsed);
}

bool SidebarWidget::isCollapsed() const
{
    return d->collapsed;
}

void SidebarWidget::toggle()
{
    setCollapsed(!d->collapsed);
}

void SidebarWidget::setExpandedWidth(int width)
{
    d->expandedWidth = width;
    if (!d->collapsed) {
        setFixedWidth(width);
    }
}

void SidebarWidget::setCollapsedWidth(int width)
{
    d->collapsedWidth = width;
    if (d->collapsed) {
        setFixedWidth(width);
    }
}

void SidebarWidget::onItemClicked()
{
    auto* btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    QString id = btn->objectName();
    LOG_DEBUG("当前id：" + d->currentId + "点击id：" + id);
    setCurrentItem(id);
    emit itemClicked(id);
}

void SidebarWidget::setupUI()
{
    d->layout = new QVBoxLayout(this);
    d->layout->setContentsMargins(0, 0, 0, 0);
    d->layout->setSpacing(4);
    d->layout->setAlignment(Qt::AlignTop);

    // 折叠按钮
    // d->toggleBtn = new QPushButton(this);
    // d->toggleBtn->setIcon(QIcon(":/icons/chevron_left.svg"));
    // d->toggleBtn->setIconSize(QSize(20, 20));
    // d->toggleBtn->setFixedSize(40, 40);
    // d->toggleBtn->setCursor(Qt::PointingHandCursor);

    // connect(d->toggleBtn, &QPushButton::clicked, this, &SidebarWidget::toggle);

    // QHBoxLayout* toggleLayout = new QHBoxLayout();
    // toggleLayout->addStretch();
    // toggleLayout->addWidget(d->toggleBtn);
    // toggleLayout->addStretch();

    // d->layout->addLayout(toggleLayout);
}

void SidebarWidget::animateCollapse(bool collapse)
{
    const int startWidth = collapse ? d->expandedWidth : d->collapsedWidth;
    const int endWidth = collapse ? d->collapsedWidth : d->expandedWidth;

    auto* widthAnim = new QPropertyAnimation(this, "minimumWidth");
    widthAnim->setDuration(250);
    widthAnim->setStartValue(startWidth);
    widthAnim->setEndValue(endWidth);
    widthAnim->setEasingCurve(QEasingCurve::InOutCubic);

    auto* maxWidthAnim = new QPropertyAnimation(this, "maximumWidth");
    maxWidthAnim->setDuration(250);
    maxWidthAnim->setStartValue(startWidth);
    maxWidthAnim->setEndValue(endWidth);
    maxWidthAnim->setEasingCurve(QEasingCurve::InOutCubic);

    auto* group = new QParallelAnimationGroup();
    group->addAnimation(widthAnim);
    group->addAnimation(maxWidthAnim);

    // 更新图标
    connect(group, &QParallelAnimationGroup::finished, [this, collapse]() {
        d->toggleBtn->setIcon(QIcon(collapse ? ":/icons/chevron_right.svg" : ":/icons/chevron_left.svg"));

        // 显示/隐藏文字
        auto it = d->items.begin();
        while (it != d->items.end()) {
            auto btn = it.value(); // 对于QMap，迭代器的value()方法获取值（你的按钮指针）
            btn->setText(collapse ? "" : btn->objectName());
            ++it;
        }
        d->logoWidget->setVisible(!collapse);
    });

    group->start(QAbstractAnimation::DeleteWhenStopped);
}
