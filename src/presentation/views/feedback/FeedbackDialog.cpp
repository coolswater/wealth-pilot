/**
 * @file FeedbackDialog.cpp
 * @brief 用户反馈对话框实现
 */

#include "FeedbackDialog.h"
#include "core/feedback/FeedbackSystem.h"
#include "shared/utils/Logger.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QApplication>
#include <QSysInfo>

namespace WealthPilot {

FeedbackDialog::FeedbackDialog(QWidget* parent)
    : QDialog(parent)
{
    setupUI();
}

void FeedbackDialog::setupUI()
{
    setWindowTitle(tr("提交反馈"));
    setMinimumWidth(400);

    auto* mainLayout = new QVBoxLayout(this);

    // 表单布局
    auto* formLayout = new QFormLayout();

    // 标题
    m_titleEdit = new QLineEdit(this);
    m_titleEdit->setPlaceholderText(tr("请输入反馈标题"));
    formLayout->addRow(tr("标题:"), m_titleEdit);

    // 类型
    m_typeCombo = new QComboBox(this);
    m_typeCombo->addItem(tr("Bug 报告"), static_cast<int>(FeedbackCategoryType::BugReport));
    m_typeCombo->addItem(tr("功能请求"), static_cast<int>(FeedbackCategoryType::FeatureRequest));
    m_typeCombo->addItem(tr("改进建议"), static_cast<int>(FeedbackCategoryType::Improvement));
    m_typeCombo->addItem(tr("问题咨询"), static_cast<int>(FeedbackCategoryType::Question));
    m_typeCombo->addItem(tr("其他"), static_cast<int>(FeedbackCategoryType::Other));
    formLayout->addRow(tr("类型:"), m_typeCombo);

    // 优先级
    m_priorityCombo = new QComboBox(this);
    m_priorityCombo->addItem(tr("低"), static_cast<int>(FeedbackPriority::Low));
    m_priorityCombo->addItem(tr("中"), static_cast<int>(FeedbackPriority::Medium));
    m_priorityCombo->addItem(tr("高"), static_cast<int>(FeedbackPriority::High));
    m_priorityCombo->addItem(tr("紧急"), static_cast<int>(FeedbackPriority::Critical));
    formLayout->addRow(tr("优先级:"), m_priorityCombo);

    // 邮箱
    m_emailEdit = new QLineEdit(this);
    m_emailEdit->setPlaceholderText(tr("可选，用于接收回复通知"));
    formLayout->addRow(tr("邮箱:"), m_emailEdit);

    mainLayout->addLayout(formLayout);

    // 内容
    auto* contentLabel = new QLabel(tr("详细描述:"), this);
    mainLayout->addWidget(contentLabel);

    m_contentEdit = new QTextEdit(this);
    m_contentEdit->setPlaceholderText(tr("请详细描述您的问题或建议..."));
    m_contentEdit->setMinimumHeight(150);
    mainLayout->addWidget(m_contentEdit);

    // 按钮
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_cancelBtn = new QPushButton(tr("取消"), this);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(m_cancelBtn);

    m_submitBtn = new QPushButton(tr("提交"), this);
    m_submitBtn->setDefault(true);
    connect(m_submitBtn, &QPushButton::clicked, this, &FeedbackDialog::onSubmit);
    buttonLayout->addWidget(m_submitBtn);

    mainLayout->addLayout(buttonLayout);
}

void FeedbackDialog::onSubmit()
{
    QString title = m_titleEdit->text().trimmed();
    QString content = m_contentEdit->toPlainText().trimmed();

    if (title.isEmpty()) {
        m_titleEdit->setFocus();
        return;
    }

    if (content.isEmpty()) {
        m_contentEdit->setFocus();
        return;
    }

    // 创建反馈信息
    FeedbackInfo feedback;
    feedback.title = title;
    feedback.content = content;
    feedback.type = static_cast<FeedbackCategoryType>(m_typeCombo->currentData().toInt());
    feedback.priority = static_cast<FeedbackPriority>(m_priorityCombo->currentData().toInt());
    feedback.userEmail = m_emailEdit->text().trimmed();
    feedback.userVersion = QApplication::applicationVersion();
    feedback.userPlatform = QSysInfo::prettyProductName();

    // 提交反馈
    QString feedbackId = FeedbackSystem::instance().submitFeedback(feedback);

    if (!feedbackId.isEmpty()) {
        LOG_INFO(QString("Feedback submitted successfully: %1").arg(feedbackId));
        accept();
    } else {
        LOG_ERROR("Failed to submit feedback");
    }
}

QString FeedbackDialog::getTitle() const
{
    return m_titleEdit->text().trimmed();
}

QString FeedbackDialog::getContent() const
{
    return m_contentEdit->toPlainText().trimmed();
}

int FeedbackDialog::getFeedbackType() const
{
    return m_typeCombo->currentData().toInt();
}

int FeedbackDialog::getPriority() const
{
    return m_priorityCombo->currentData().toInt();
}

QString FeedbackDialog::getUserEmail() const
{
    return m_emailEdit->text().trimmed();
}

void FeedbackDialog::onCancel()
{
    reject();
}

} // namespace WealthPilot