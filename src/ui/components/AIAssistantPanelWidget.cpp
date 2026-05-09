#include "AIAssistantPanelWidget.h"
/**
 * @file AIAssistantPanel.cpp
 * @brief AI 助理面板实现 - 使用属性选择器替代硬编码样式
 */
#include "../../ui/animation/AnimationManager.h"
#include "../../ui/ThemeManager.h"
#include "../../core/config/Tokens.h"
#include "../../core/config/ConfigManager.h"
#include "../../utils/Logger.h"
#include "../../core/navigation/PageNavigator.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QScrollArea>
#include <QFrame>
#include <QTimer>
#include <QScrollBar>
#include <QStyle>

#include "ai/service/AIService.h"

using namespace Tokens;

struct AIAssistantPanelWidget::Impl {
    QWidget* messagesContainer = nullptr;
    QVBoxLayout* messagesLayout = nullptr;
    QLineEdit* inputField = nullptr;
    QScrollArea* scrollArea = nullptr;
    QPushButton* collapseBtn = nullptr;
    QPushButton* sendBtn = nullptr;

    // 状态
    bool isCollapsed = false;
    bool isTyping = false;
    QWidget* typingIndicator = nullptr;

    // 缓存
    QList<AIMessage> conversationHistory;
};

AIAssistantPanelWidget::AIAssistantPanelWidget(QWidget *parent)
    : BaseWidget(parent)
    , d(std::make_unique<Impl>())
{
    setFixedWidth(Size::AIPanelWidth);
    // 设置对象名以便 QSS 选择器使用
    setObjectName("AIAssistantPanelWidget");
    setupUI();

    // 连接 AI 服务
    connectToAIService();
}

AIAssistantPanelWidget::~AIAssistantPanelWidget() = default;

void AIAssistantPanelWidget::connectToAIService() const
{
    connect(AIService::instance(), &AIService::responseComplete,
            this, &AIAssistantPanelWidget::onAIResponseReceived);
    connect(AIService::instance(), &AIService::errorOccurred,
            this, &AIAssistantPanelWidget::onAIError);
}

void AIAssistantPanelWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(Spacing::MD, Spacing::MD, Spacing::MD, Spacing::MD);
    mainLayout->setSpacing(Spacing::MD);

    setupHeader();
    setupQuickActions();
    setupMessagesArea();
    setupInputArea();

    // 注册主题监听器
    ThemeManager::instance()->registerThemeChangeListener(this, [this]() {
        updateTheme();
    });
}

void AIAssistantPanelWidget::setupHeader()
{
    auto* mainLayout = qobject_cast<QVBoxLayout*>(layout());

    auto* headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(Spacing::SM);

    // AI 头像 - 使用属性选择器
    auto* avatarLabel = new QLabel(this);
    avatarLabel->setFixedSize(Size::AvatarLG, Size::AvatarLG);
    avatarLabel->setObjectName("aiAvatar");
    headerLayout->addWidget(avatarLabel);

    // 名称和状态
    auto* infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(2);

    auto* nameLabel = new QLabel("WealthPilot AI", this);
    nameLabel->setObjectName("aiName");
    infoLayout->addWidget(nameLabel);

    auto* statusLabel = new QLabel("在线", this);
    statusLabel->setObjectName("aiStatus");
    statusLabel->setProperty("status", "connected");
    infoLayout->addWidget(statusLabel);

    headerLayout->addLayout(infoLayout);
    headerLayout->addStretch();

    // 折叠按钮 - 使用属性选择器
    d->collapseBtn = new QPushButton("◀", this);
    d->collapseBtn->setFixedSize(32, 32);
    d->collapseBtn->setCursor(Qt::PointingHandCursor);
    d->collapseBtn->setObjectName("collapseBtn");
    connect(d->collapseBtn, &QPushButton::clicked, this, [this]() {
        setCollapsed(!d->isCollapsed);
    });
    headerLayout->addWidget(d->collapseBtn);

    mainLayout->addLayout(headerLayout);
}

void AIAssistantPanelWidget::setupQuickActions()
{
    auto* mainLayout = qobject_cast<QVBoxLayout*>(layout());

    auto* quickActionsLayout = new QHBoxLayout();
    quickActionsLayout->setSpacing(Spacing::XS);

    QStringList actions = {"分析持仓", "市场总结", "设置提醒"};
    for (const QString& action : actions) {
        auto* btn = new QPushButton(action, this);
        btn->setFixedHeight(Size::ButtonHeightSM);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setProperty("quickAction", true);
        connect(btn, &QPushButton::clicked, this, &AIAssistantPanelWidget::onQuickActionClicked);
        quickActionsLayout->addWidget(btn);
    }

    mainLayout->addLayout(quickActionsLayout);
}

void AIAssistantPanelWidget::setupMessagesArea()
{
    auto* mainLayout = qobject_cast<QVBoxLayout*>(layout());

    d->scrollArea = new QScrollArea(this);
    d->scrollArea->setWidgetResizable(true);
    d->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    d->scrollArea->setObjectName("messagesScrollArea");

    d->messagesContainer = new QWidget();
    d->messagesContainer->setObjectName("messagesContainer");
    d->messagesLayout = new QVBoxLayout(d->messagesContainer);
    d->messagesLayout->setContentsMargins(0, 0, 0, 0);
    d->messagesLayout->setSpacing(Spacing::SM);
    d->messagesLayout->addStretch();

    d->scrollArea->setWidget(d->messagesContainer);
    mainLayout->addWidget(d->scrollArea, 1);

    // 检查 AI 配置
    checkAIConfiguration();
}

void AIAssistantPanelWidget::checkAIConfiguration()
{
    QString apiKey = ConfigManager::instance()->getSecure("secure/ai_api_key");
    bool aiEnabled = ConfigManager::instance()->getBool("ai/enabled", false);

    if (apiKey.isEmpty() || !aiEnabled) {
        showConfigurationGuide();
    } else {
        addMessage(QStringLiteral(
                       "您好！我是 WealthPilot AI，您的投资助手。\n\n我可以帮您：\n- 分析投资组合和投资决策\n- 提供市场洞察和趋势\n- 提供投资建议和风险提示\n- 价格预警和预测\n\n请问有什么可以帮您的？"),
                   false);
    }
}

void AIAssistantPanelWidget::showConfigurationGuide()
{
    // 创建配置引导消息 - 使用属性选择器
    auto* guideWidget = new QFrame(d->messagesContainer);
    guideWidget->setObjectName("guideWidget");
    guideWidget->setProperty("cardType", "elevated");

    auto* guideLayout = new QVBoxLayout(guideWidget);
    guideLayout->setContentsMargins(Spacing::MD, Spacing::MD, Spacing::MD, Spacing::MD);
    guideLayout->setSpacing(Spacing::SM);

    // 标题 - 使用属性选择器
    QLabel* titleLabel = new QLabel(QStringLiteral("AI 配置未完成"), guideWidget);
    titleLabel->setProperty("dataType", "title");
    guideLayout->addWidget(titleLabel);

    // 说明文字 - 使用属性选择器
    QLabel* descLabel = new QLabel(
        QStringLiteral("要使用 AI 助手，请先完成以下配置：\n\n"
        "1. 进入设置页面\n"
        "2. 找到 'AI 配置' 部分\n"
        "3. 输入您的 API 密钥和其他设置\n\n"
        "您的 API 密钥将使用 Windows DPAPI 加密安全存储。"),
        guideWidget);
    descLabel->setWordWrap(true);
    descLabel->setProperty("dataType", "label");
    guideLayout->addWidget(descLabel);

    // 跳转按钮 - 使用属性选择器
    QPushButton* gotoSettingsBtn = new QPushButton(QStringLiteral("前往设置"), guideWidget);
    gotoSettingsBtn->setObjectName("gotoSettingsBtn");
    gotoSettingsBtn->setProperty("primary", true);

    connect(gotoSettingsBtn, &QPushButton::clicked, this, [this]() {
        LOG_INFO("Navigating to settings page from AI panel");
        PageNavigator::instance().navigateTo(QStringLiteral("settings"));
    });

    guideLayout->addWidget(gotoSettingsBtn);

    // 添加到消息区域
    d->messagesLayout->insertWidget(d->messagesLayout->count() - 1, guideWidget);

    // 强制刷新样式
    guideWidget->style()->unpolish(guideWidget);
    guideWidget->style()->polish(guideWidget);
}

void AIAssistantPanelWidget::setupInputArea()
{
    auto* mainLayout = qobject_cast<QVBoxLayout*>(layout());

    auto* inputLayout = new QHBoxLayout();
    inputLayout->setSpacing(Spacing::SM);

    // 输入框 - 使用对象名选择器
    d->inputField = new QLineEdit(this);
    d->inputField->setPlaceholderText("输入问题或说 'Hey Pilot'...");
    d->inputField->setFixedHeight(Size::InputHeightLG);
    d->inputField->setObjectName("aiInputField");
    connect(d->inputField, &QLineEdit::returnPressed, this, &AIAssistantPanelWidget::onSendClicked);
    inputLayout->addWidget(d->inputField);

    // 发送按钮 - 使用对象名选择器
    d->sendBtn = new QPushButton("Send", this);
    d->sendBtn->setFixedSize(Size::InputHeightLG, Size::InputHeightLG);
    d->sendBtn->setCursor(Qt::PointingHandCursor);
    d->sendBtn->setObjectName("sendBtn");
    connect(d->sendBtn, &QPushButton::clicked, this, &AIAssistantPanelWidget::onSendClicked);
    inputLayout->addWidget(d->sendBtn);

    mainLayout->addLayout(inputLayout);
}

void AIAssistantPanelWidget::sendMessage(const QString& message)
{
    if (message.trimmed().isEmpty()) return;

    addMessage(message, true);
    emit messageSent(message);

    addTypingIndicator();

    AIService::instance()->chat(message, [this](Result<QString> result) {
        removeTypingIndicator();

        if (result.isOk()) {
            addAIResponse(result.unwrap());
        } else {
            showSystemMessage(QString("AI 响应失败: %1").arg(result.errorMessage()), "error");
        }
    });
}

void AIAssistantPanelWidget::showSystemMessage(const QString& message, const QString& type) const
{
    auto* frame = new QFrame(d->messagesContainer);
    frame->setProperty("messageType", type);

    auto* layout = new QHBoxLayout(frame);
    layout->setContentsMargins(Spacing::MD, Spacing::SM, Spacing::MD, Spacing::SM);

    auto* label = new QLabel(message, frame);
    label->setWordWrap(true);
    label->setProperty("dataType", "value");
    layout->addWidget(label);

    d->messagesLayout->insertWidget(d->messagesLayout->count() - 1, frame);

    // 强制刷新样式
    frame->style()->unpolish(frame);
    frame->style()->polish(frame);
    label->style()->unpolish(label);
    label->style()->polish(label);

    AnimationManager::instance()->fadeIn(frame, Animation::DurationFast);
}

void AIAssistantPanelWidget::addAIResponse(const QString& response) const
{
    addMessage(response, false);
}

void AIAssistantPanelWidget::clearHistory() const
{
    // 清除消息
    while (d->messagesLayout->count() > 1) {
        QLayoutItem* item = d->messagesLayout->takeAt(0);
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    // 清除 AI 服务历史
    AIService::instance()->clearHistory();
    d->conversationHistory.clear();

    // Re-display welcome message
    addMessage("Conversation cleared. What can I help you with?", false);
}

void AIAssistantPanelWidget::setCollapsed(bool collapsed)
{
    if (d->isCollapsed == collapsed) return;

    d->isCollapsed = collapsed;

    if (collapsed) {
        setFixedWidth(Size::AIPanelCollapsed);
        d->collapseBtn->setText(">");
    } else {
        setFixedWidth(Size::AIPanelWidth);
        d->collapseBtn->setText("<");
    }

    emit collapsedChanged(collapsed);
}

bool AIAssistantPanelWidget::isCollapsed() const
{
    return d->isCollapsed;
}

void AIAssistantPanelWidget::onSendClicked()
{
    QString text = d->inputField->text().trimmed();
    if (text.isEmpty()) return;

    sendMessage(text);
    d->inputField->clear();
}

void AIAssistantPanelWidget::onQuickActionClicked()
{
    auto* btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    QString action = btn->text();
    sendMessage(action);
}

void AIAssistantPanelWidget::onAIResponseReceived(const QString& response) const
{
    removeTypingIndicator();
    addAIResponse(response);
}

void AIAssistantPanelWidget::onAIError(const QString& error) const
{
    removeTypingIndicator();
    showSystemMessage(QString("错误: %1").arg(error), "error");
}

void AIAssistantPanelWidget::addMessage(const QString& text, bool isUser) const
{
    auto* bubble = new QFrame(d->messagesContainer);
    bubble->setProperty("messageType", isUser ? "user" : "assistant");

    auto* bubbleLayout = new QHBoxLayout(bubble);
    bubbleLayout->setContentsMargins(0, 0, 0, 0);
    bubbleLayout->setSpacing(0);

    auto* label = new QLabel(text, bubble);
    label->setWordWrap(true);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    label->setMaximumWidth(280);
    label->setProperty("dataType", "value");

    if (isUser) {
        bubbleLayout->addStretch();
        bubbleLayout->addWidget(label);
    } else {
        bubbleLayout->addWidget(label);
        bubbleLayout->addStretch();
    }

    d->messagesLayout->insertWidget(d->messagesLayout->count() - 1, bubble);

    // 强制刷新样式
    bubble->style()->unpolish(bubble);
    bubble->style()->polish(bubble);
    label->style()->unpolish(label);
    label->style()->polish(label);

    // 滚动到底
    QTimer::singleShot(100, this, [this]() {
        d->scrollArea->verticalScrollBar()->setValue(
            d->scrollArea->verticalScrollBar()->maximum()
        );
    });

    AnimationManager::instance()->fadeIn(bubble, Animation::DurationFast);
}

void AIAssistantPanelWidget::addTypingIndicator() const
{
    if (d->typingIndicator) return;

    d->typingIndicator = new QWidget(d->messagesContainer);
    d->typingIndicator->setProperty("messageType", "typing");

    auto* layout = new QHBoxLayout(d->typingIndicator);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* label = new QLabel("AI 正在思考...", d->typingIndicator);
    label->setProperty("dataType", "label");
    layout->addWidget(label);
    layout->addStretch();

    d->messagesLayout->insertWidget(d->messagesLayout->count() - 1, d->typingIndicator);

    // 强制刷新样式
    d->typingIndicator->style()->unpolish(d->typingIndicator);
    d->typingIndicator->style()->polish(d->typingIndicator);

    QTimer::singleShot(100, this, [this]() {
        d->scrollArea->verticalScrollBar()->setValue(
            d->scrollArea->verticalScrollBar()->maximum()
        );
    });
}

void AIAssistantPanelWidget::removeTypingIndicator() const
{
    if (d->typingIndicator) {
        d->typingIndicator->deleteLater();
        d->typingIndicator = nullptr;
    }
}

void AIAssistantPanelWidget::updateTheme()
{
    // 由于使用属性选择器，大部分样式由 QSS 处理
    // 只需要强制刷新即可
    update();
    repaint();

    // 刷新所有子组件
    for (auto* child : findChildren<QWidget*>())
    {
        child->style()->unpolish(child);
        child->style()->polish(child);
    }
}
