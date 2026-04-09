#ifndef WEALTHPILOT_AIASSISTANTPANELWIDGET_H
#define WEALTHPILOT_AIASSISTANTPANELWIDGET_H

/**
 * @file AIAssistantPanel.h
 * @brief AI 助理面板 - 右侧可折叠面板
 *
 * @details 功能：
 * - AI 对话交互
 * - 快捷操作
 * - 智能分析
 */

#include <memory>
#include <QList>

#include "BaseWidget.h"

class QTextEdit;
class QLineEdit;
class QScrollArea;
class QVBoxLayout;

/**
 * @brief AI 助理面板
 */
class AIAssistantPanelWidget : public BaseWidget
{
    Q_OBJECT

public:
    explicit AIAssistantPanelWidget(QWidget* parent = nullptr);
    ~AIAssistantPanelWidget() override;

    /**
     * @brief 发送消息
     */
    void sendMessage(const QString& message);

    /**
     * @brief 显示系统消息
     */
    void showSystemMessage(const QString& message, const QString& type = "info") const;

    /**
     * @brief 添加 AI 回复
     */
    void addAIResponse(const QString& response) const;

    /**
     * @brief 清除对话历史
     */
    void clearHistory() const;

    /**
     * @brief 设置折叠状态
     */
    void setCollapsed(bool collapsed);

    /**
     * @brief 是否折叠
     */
    [[nodiscard]] bool isCollapsed() const;

signals:
    /**
     * @brief 消息发送信号
     */
    void messageSent(const QString& message);

    /**
     * @brief 折叠状态改变信号
     */
    void collapsedChanged(bool collapsed);

private slots:
    void onSendClicked();
    void onQuickActionClicked();
    void onAIResponseReceived(const QString& response) const;
    void onAIError(const QString& error) const;

private:
    void setupUI();
    void setupHeader();
    void setupQuickActions();
    void setupMessagesArea();
    void setupInputArea();
    void addMessage(const QString& text, bool isUser) const;
    void addTypingIndicator() const;
    void removeTypingIndicator() const;
    void connectToAIService() const;

    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif //WEALTHPILOT_AIASSISTANTPANELWIDGET_H
