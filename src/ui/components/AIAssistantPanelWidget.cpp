#include "AIAssistantPanelWidget.h"
/**
 * @file AIAssistantPanel.cpp
 * @brief AI 助理面板实现
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

#include "ai/service/AIService.h"

using namespace Tokens;

struct AIAssistantPanelWidget::Impl {
    QWidget* messagesContainer = nullptr;
    QVBoxLayout* messagesLayout = nullptr;
    QLineEdit* inputField = nullptr;
    QScrollArea* scrollArea = nullptr;
    QPushButton* collapseBtn = nullptr;

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
    setupUI();

    // 初始化AI服务连接
    connectToAIService();
}

AIAssistantPanelWidget::~AIAssistantPanelWidget() = default;
void AIAssistantPanelWidget::connectToAIService() const
{
    // 连接 AI 服务信号
    connect(AIService::instance(), &AIService::responseComplete,
            this, &AIAssistantPanelWidget::onAIResponseReceived);
    connect(AIService::instance(), &AIService::errorOccurred,
            this, &AIAssistantPanelWidget::onAIError);
}

void AIAssistantPanelWidget::setupUI()
{
    setStyleSheet(QString(
        "background-color: %1; border-left: 1px solid %2;")
        .arg(Colors::BgSurface, Colors::BorderLight));

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(Spacing::MD, Spacing::MD, Spacing::MD, Spacing::MD);
    mainLayout->setSpacing(Spacing::MD);

    setupHeader();
    setupQuickActions();
    setupMessagesArea();
    setupInputArea();
}

void AIAssistantPanelWidget::setupHeader()
{
    auto* mainLayout = qobject_cast<QVBoxLayout*>(layout());

    auto* headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(Spacing::SM);

    // AI 头像
    auto* avatarLabel = new QLabel(this);
    avatarLabel->setFixedSize(Size::AvatarLG, Size::AvatarLG);
    avatarLabel->setStyleSheet(QString(R"(
        background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
            stop:0 %1, stop:1 %2);
        border-radius: %3px;
    )").arg(Colors::Primary, Colors::Secondary).arg(Size::AvatarLG / 2));
    headerLayout->addWidget(avatarLabel);

    // 名称和状态
    auto* infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(2);

    auto* nameLabel = new QLabel("WealthPilot AI", this);
    nameLabel->setStyleSheet(QString(
        "font-size: %1px; font-weight: 600; color: %2;")
        .arg(Font::Size::H3).arg(Colors::TextPrimary));
    infoLayout->addWidget(nameLabel);

    auto* statusLabel = new QLabel("�?在线", this);
    statusLabel->setStyleSheet(QString(
        "font-size: %1px; color: %2;")
        .arg(Font::Size::Small).arg(Colors::Success));
    infoLayout->addWidget(statusLabel);

    headerLayout->addLayout(infoLayout);
    headerLayout->addStretch();

    // 折叠按钮
    d->collapseBtn = new QPushButton("◀", this);
    d->collapseBtn->setFixedSize(32, 32);
    d->collapseBtn->setCursor(Qt::PointingHandCursor);
    d->collapseBtn->setStyleSheet(QString(R"(
        QPushButton {
            background-color: transparent;
            color: %1;
            border: none;
            border-radius: 6px;
        }
        QPushButton:hover {
            background-color: %2;
        }
    )").arg(Colors::TextSecondary, Colors::BgHover));
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
        btn->setStyleSheet(QString(R"(
            QPushButton {
                background-color: rgba(59, 130, 246, 0.15);
                color: %1;
                border: none;
                border-radius: %2px;
                padding: 0 %3px;
                font-size: %4px;
            }
            QPushButton:hover {
                background-color: rgba(59, 130, 246, 0.25);
            }
        )").arg(Colors::Primary).arg(Radius::Full).arg(Spacing::MD).arg(Font::Size::Small));
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
    d->scrollArea->setStyleSheet(QString(
        "background: transparent; border: none;"));

    d->messagesContainer = new QWidget();
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
    // 检查是否已配置 API Key
    QString apiKey = ConfigManager::instance()->getSecure("secure/ai_api_key");
    bool aiEnabled = ConfigManager::instance()->getBool("ai/enabled", false);
    
    if (apiKey.isEmpty() || !aiEnabled) {
        // 显示配置引导
        showConfigurationGuide();
    } else {
        // 显示欢迎消息
        addMessage(QStringLiteral("您好！我是 WealthPilot AI，您的投资助手。\\n\\n我可以帮您：\\n- 分析投资组合和投资决策\\n- 提供市场洞察和趋势\\n- 提供投资建议和风险提示\\n- 价格预警和预测\\n\\n请问有什么可以帮您的？"), false);
    }
}

void AIAssistantPanelWidget::showConfigurationGuide()
{
    // 创建配置引导消息
    QWidget* guideWidget = new QWidget(d->messagesContainer);
    QVBoxLayout* guideLayout = new QVBoxLayout(guideWidget);
    guideLayout->setContentsMargins(Spacing::MD, Spacing::MD, Spacing::MD, Spacing::MD);
    guideLayout->setSpacing(Spacing::SM);
    
    // 背景
    guideWidget->setStyleSheet(QString(
        "background-color: %1; border-radius: %2px;")
        .arg(Colors::BgElevated)
        .arg(Radius::MD));
    
    // 标题
    QLabel* titleLabel = new QLabel(QStringLiteral("AI 配置未完成"), guideWidget);
    titleLabel->setStyleSheet(QString("color: %1; font-weight: bold; font-size: 14px;")
        .arg(Colors::TextPrimary));
    guideLayout->addWidget(titleLabel);
    
    // 说明文字
    QLabel* descLabel = new QLabel(
        QStringLiteral("要使用 AI 助手，请先完成以下配置：\n\n"
        "1. 进入设置页面\n"
        "2. 找到 'AI 配置' 部分\n"
        "3. 输入您的 API 密钥和其他设置\n\n"
        "您的 API 密钥将使用 Windows DPAPI 加密安全存储。"),
        guideWidget);
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet(QString("color: %1; font-size: 13px;")
        .arg(Colors::TextSecondary));
    guideLayout->addWidget(descLabel);
    
    // 跳转按钮
    QPushButton* gotoSettingsBtn = new QPushButton(QStringLiteral("前往设置"), guideWidget);
    gotoSettingsBtn->setStyleSheet(QString(
        "QPushButton {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: none;"
        "  border-radius: %3px;"
        "  padding: 8px 16px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: %4;"
        "}")
        .arg(Colors::Primary)
        .arg(Colors::TextPrimary)
        .arg(Radius::SM)
        .arg(Colors::PrimaryHover));
    
    connect(gotoSettingsBtn, &QPushButton::clicked, this, [this]() {
        // 跳转到设置页面（页面ID是 "settings"）
        LOG_INFO("Navigating to settings page from AI panel");
        PageNavigator::instance().navigateTo(QStringLiteral("settings"));
    });
    
    guideLayout->addWidget(gotoSettingsBtn);
    
    // 添加到消息区域
    d->messagesLayout->insertWidget(d->messagesLayout->count() - 1, guideWidget);
}

void AIAssistantPanelWidget::setupInputArea()
{
    auto* mainLayout = qobject_cast<QVBoxLayout*>(layout());

    auto* inputLayout = new QHBoxLayout();
    inputLayout->setSpacing(Spacing::SM);

    d->inputField = new QLineEdit(this);
    d->inputField->setPlaceholderText("输入问题或说 'Hey Pilot'...");
    d->inputField->setFixedHeight(Size::InputHeightLG);
    d->inputField->setStyleSheet(QString(R"(
        QLineEdit {
            background-color: %1;
            border: 1px solid %2;
            border-radius: %3px;
            padding: 0 %4px;
            color: %5;
            font-size: %6px;
        }
        QLineEdit:focus {
            border-color: %7;
        }
    )").arg(Colors::BgHover, Colors::Border)
       .arg(Radius::Full)
       .arg(Spacing::MD)
       .arg(Colors::TextPrimary)
       .arg(Font::Size::Body)
       .arg(Colors::Primary));

    connect(d->inputField, &QLineEdit::returnPressed, this, &AIAssistantPanelWidget::onSendClicked);
    inputLayout->addWidget(d->inputField);

    auto* sendBtn = new QPushButton("Send", this);
    sendBtn->setFixedSize(Size::InputHeightLG, Size::InputHeightLG);
    sendBtn->setCursor(Qt::PointingHandCursor);
    sendBtn->setStyleSheet(QString(R"(
        QPushButton {
            background-color: %1;
            color: white;
            border: none;
            border-radius: %2px;
            font-size: %3px;
        }
        QPushButton:hover {
            background-color: %4;
        }
    )").arg(Colors::Primary)
       .arg(Size::InputHeightLG / 2)
       .arg(Font::Size::Body)
       .arg(Colors::PrimaryHover));
    connect(sendBtn, &QPushButton::clicked, this, &AIAssistantPanelWidget::onSendClicked);
    inputLayout->addWidget(sendBtn);

    mainLayout->addLayout(inputLayout);
}

void AIAssistantPanelWidget::sendMessage(const QString& message)
{
    if (message.trimmed().isEmpty()) return;

    addMessage(message, true);
    emit messageSent(message);

    // 显示输入中指示器
    addTypingIndicator();

    // 发送到 AI 服务
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

    QString bgColor, borderColor;
    if (type == "warning") {
        bgColor = Colors::WarningBg;
        borderColor = "rgba(249, 115, 22, 0.3)";
    } else if (type == "error") {
        bgColor = Colors::DangerBg;
        borderColor = "rgba(239, 68, 68, 0.3)";
    } else if (type == "success") {
        bgColor = Colors::SuccessBg;
        borderColor = "rgba(16, 185, 129, 0.3)";
    } else {
        bgColor = Colors::InfoBg;
        borderColor = "rgba(59, 130, 246, 0.3)";
    }

    frame->setStyleSheet(QString(R"(
        QFrame {
            background-color: %1;
            border: 1px solid %2;
            border-radius: %3px;
            padding: %4px;
        }
    )").arg(bgColor, borderColor).arg(Radius::MD).arg(Spacing::SM));

    auto* layout = new QHBoxLayout(frame);
    layout->setContentsMargins(Spacing::MD, Spacing::SM, Spacing::MD, Spacing::SM);

    auto* label = new QLabel(message, frame);
    label->setWordWrap(true);
    label->setStyleSheet(QString(
        "color: %1; font-size: %2px;")
        .arg(Colors::TextPrimary).arg(Font::Size::Small));
    layout->addWidget(label);

    d->messagesLayout->insertWidget(d->messagesLayout->count() - 1, frame);

    // 动画
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
    auto* bubble = new QWidget(d->messagesContainer);
    auto* bubbleLayout = new QHBoxLayout(bubble);
    bubbleLayout->setContentsMargins(0, 0, 0, 0);
    bubbleLayout->setSpacing(0);

    auto* label = new QLabel(text, bubble);
    label->setWordWrap(true);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    label->setMaximumWidth(280);

    if (isUser) {
        bubbleLayout->addStretch();
        label->setStyleSheet(QString(R"(
            background-color: %1;
            color: white;
            border-radius: %2px;
            padding: %3px %4px;
            font-size: %5px;
        )").arg(Colors::Primary)
           .arg(Radius::LG)
           .arg(Spacing::SM)
           .arg(Spacing::MD)
           .arg(Font::Size::Body));
        bubbleLayout->addWidget(label);
    } else {
        label->setStyleSheet(QString(R"(
            background-color: %1;
            color: %2;
            border-radius: %3px;
            padding: %4px %5px;
            font-size: %6px;
            line-height: 1.5;
        )").arg(Colors::BgHover, Colors::TextPrimary)
           .arg(Radius::LG)
           .arg(Spacing::SM)
           .arg(Spacing::MD)
           .arg(Font::Size::Body));
        bubbleLayout->addWidget(label);
        bubbleLayout->addStretch();
    }

    d->messagesLayout->insertWidget(d->messagesLayout->count() - 1, bubble);

    // 滚动到底
    QTimer::singleShot(100, this, [this]() {
        d->scrollArea->verticalScrollBar()->setValue(
            d->scrollArea->verticalScrollBar()->maximum()
        );
    });

    // 入场动画
    AnimationManager::instance()->fadeIn(bubble, Animation::DurationFast);
}

void AIAssistantPanelWidget::addTypingIndicator() const
{
    if (d->typingIndicator) return;

    d->typingIndicator = new QWidget(d->messagesContainer);
    auto* layout = new QHBoxLayout(d->typingIndicator);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* label = new QLabel("AI 正在思�?..", d->typingIndicator);
    label->setStyleSheet(QString(R"(
        background-color: %1;
        color: %2;
        border-radius: %3px;
        padding: %4px %5px;
        font-size: %6px;
    )").arg(Colors::BgHover, Colors::TextSecondary)
       .arg(Radius::LG)
       .arg(Spacing::SM)
       .arg(Spacing::MD)
       .arg(Font::Size::Small));
    layout->addWidget(label);
    layout->addStretch();

    d->messagesLayout->insertWidget(d->messagesLayout->count() - 1, d->typingIndicator);

    // 滚动到底
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



