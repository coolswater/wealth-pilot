/**
 * @file FeedbackDialog.h
 * @brief 用户反馈对话框
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef FEEDBACKDIALOG_H
#define FEEDBACKDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QButtonGroup>
#include <QRadioButton>

namespace WealthPilot {

/**
 * @brief 用户反馈对话框
 */
class FeedbackDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FeedbackDialog(QWidget* parent = nullptr);
    ~FeedbackDialog() = default;

    /**
     * @brief 获取反馈标题
     */
    QString getTitle() const;

    /**
     * @brief 获取反馈内容
     */
    QString getContent() const;

    /**
     * @brief 获取反馈类型
     */
    int getFeedbackType() const;

    /**
     * @brief 获取优先级
     */
    int getPriority() const;

    /**
     * @brief 获取用户邮箱
     */
    QString getUserEmail() const;

private slots:
    void onSubmit();
    void onCancel();

private:
    void setupUI();
    void setupConnections();

    QLineEdit* m_titleEdit = nullptr;
    QTextEdit* m_contentEdit = nullptr;
    QComboBox* m_typeCombo = nullptr;
    QComboBox* m_priorityCombo = nullptr;
    QLineEdit* m_emailEdit = nullptr;
    QPushButton* m_submitBtn = nullptr;
    QPushButton* m_cancelBtn = nullptr;
};

} // namespace WealthPilot

#endif // FEEDBACKDIALOG_H