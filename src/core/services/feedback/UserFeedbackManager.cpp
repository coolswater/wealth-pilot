/**
 * @file UserFeedbackManager.cpp
 * @brief 用户反馈管理器实现
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "UserFeedbackManager.h"
#include "../../ui/components/ToastWidget.h"
#include <QApplication>
#include <QInputDialog>
#include <QUuid>
#include <QDebug>

namespace WealthPilot {

// ========== 单例实现 ==========

UserFeedbackManager* UserFeedbackManager::instance() {
    static UserFeedbackManager instance;
    return &instance;
}

UserFeedbackManager::UserFeedbackManager(QObject* parent)
    : QObject(parent) {
    // 初始化
}

UserFeedbackManager::~UserFeedbackManager() {
    // 清理所有进度对话框
    QMutexLocker locker(&m_progressMutex);
    for (auto* dialog : m_progressDialogs) {
        if (dialog) {
            dialog->close();
            dialog->deleteLater();
        }
    }
    m_progressDialogs.clear();
}

// ========== 基础反馈 ==========

void UserFeedbackManager::showInfo(const QString& title, const QString& message,
                                    FeedbackLevel level, int duration) {
    auto record = createRecord(title, message, FeedbackType::Info, level);
    record.duration = duration;

    switch (level) {
        case FeedbackLevel::Toast:
            showToast(record, duration);
            break;
        case FeedbackLevel::Dialog:
            showDialog(record);
            break;
        case FeedbackLevel::Notification:
            showNotification(record);
            break;
    }

    recordFeedback(record);
}

void UserFeedbackManager::showWarning(const QString& title, const QString& message,
                                       FeedbackLevel level) {
    auto record = createRecord(title, message, FeedbackType::Warning, level);

    switch (level) {
        case FeedbackLevel::Toast:
            showToast(record, 5000); // 警告显示更长时间
            break;
        case FeedbackLevel::Dialog:
            showDialog(record);
            break;
        case FeedbackLevel::Notification:
            showNotification(record);
            break;
    }

    recordFeedback(record);
}

void UserFeedbackManager::showError(const QString& title, const QString& message,
                                     FeedbackLevel level) {
    auto record = createRecord(title, message, FeedbackType::Error, level);

    switch (level) {
        case FeedbackLevel::Toast:
            showToast(record, 8000); // 错误显示更长时间
            break;
        case FeedbackLevel::Dialog:
            showDialog(record);
            break;
        case FeedbackLevel::Notification:
            showNotification(record);
            break;
    }

    recordFeedback(record);
}

void UserFeedbackManager::showSuccess(const QString& title, const QString& message,
                                       FeedbackLevel level, int duration) {
    auto record = createRecord(title, message, FeedbackType::Success, level);
    record.duration = duration;

    switch (level) {
        case FeedbackLevel::Toast:
            showToast(record, duration);
            break;
        case FeedbackLevel::Dialog:
            showDialog(record);
            break;
        case FeedbackLevel::Notification:
            showNotification(record);
            break;
    }

    recordFeedback(record);
}

// ========== 对话框 ==========

bool UserFeedbackManager::showConfirm(const QString& title, const QString& message,
                                       const QString& confirmText, const QString& cancelText) {
    QMessageBox box(QMessageBox::Question, title, message,
                    QMessageBox::NoButton, qApp->activeWindow());
    box.addButton(confirmText, QMessageBox::AcceptRole);
    box.addButton(cancelText, QMessageBox::RejectRole);

    int result = box.exec();
    bool confirmed = (result == 0); // AcceptRole

    // 记录
    auto record = createRecord(title, message, FeedbackType::Info, FeedbackLevel::Dialog);
    record.context = confirmed ? "用户确认" : "用户取消";
    record.acknowledged = true;
    recordFeedback(record);

    return confirmed;
}

QString UserFeedbackManager::showInput(const QString& title, const QString& label,
                                        const QString& defaultValue) {
    bool ok = false;
    QString text = QInputDialog::getText(qApp->activeWindow(), title, label,
                                          QLineEdit::Normal, defaultValue, &ok);

    // 记录
    auto record = createRecord(title, label, FeedbackType::Info, FeedbackLevel::Dialog);
    record.context = ok ? QString("用户输入: %1").arg(text) : "用户取消输入";
    record.acknowledged = true;
    recordFeedback(record);

    return ok ? text : QString();
}

// ========== 进度反馈 ==========

void UserFeedbackManager::beginProgress(const QString& operationId, const ProgressConfig& config) {
    QMutexLocker locker(&m_progressMutex);

    // 如果已存在，先关闭
    if (m_progressDialogs.contains(operationId)) {
        auto* oldDialog = m_progressDialogs[operationId];
        if (oldDialog) {
            oldDialog->close();
            oldDialog->deleteLater();
        }
    }

    // 创建新进度对话框
    auto* dialog = new QProgressDialog(config.title, config.cancelButtonText,
                                        config.minimum, config.maximum,
                                        qApp->activeWindow());
    dialog->setWindowModality(Qt::WindowModal);
    dialog->setAutoClose(false);
    dialog->setAutoReset(false);
    dialog->setMinimumDuration(0); // 立即显示
    dialog->setValue(config.minimum);

    if (!config.cancellable) {
        dialog->setCancelButton(nullptr);
    }

    // 连接取消信号
    connect(dialog, &QProgressDialog::canceled, this, [this, operationId]() {
        emit progressCancelled(operationId);
    });

    m_progressDialogs[operationId] = dialog;
}

void UserFeedbackManager::updateProgress(const QString& operationId, int value, const QString& message) {
    QMutexLocker locker(&m_progressMutex);

    if (m_progressDialogs.contains(operationId)) {
        auto* dialog = m_progressDialogs[operationId];
        if (dialog) {
            dialog->setValue(value);
            if (!message.isEmpty()) {
                dialog->setLabelText(message);
            }
        }
    }

    emit progressUpdated(operationId, value, message);
}

void UserFeedbackManager::endProgress(const QString& operationId, bool success, const QString& message) {
    QMutexLocker locker(&m_progressMutex);

    if (m_progressDialogs.contains(operationId)) {
        auto* dialog = m_progressDialogs[operationId];
        if (dialog) {
            if (success) {
                dialog->setValue(dialog->maximum());
                if (!message.isEmpty()) {
                    showSuccess(tr("操作完成"), message);
                }
            } else {
                showError(tr("操作失败"), message);
            }

            // 延迟关闭
            QTimer::singleShot(1000, dialog, &QProgressDialog::close);
            dialog->deleteLater();
        }
        m_progressDialogs.remove(operationId);
    }
}

bool UserFeedbackManager::isProgressCancelled(const QString& operationId) const {
    QMutexLocker locker(&m_progressMutex);

    if (m_progressDialogs.contains(operationId)) {
        auto* dialog = m_progressDialogs[operationId];
        return dialog && dialog->wasCanceled();
    }
    return false;
}

// ========== 反馈历史 ==========

QList<FeedbackRecord> UserFeedbackManager::getHistory(int limit) const {
    QMutexLocker locker(&m_historyMutex);

    if (limit <= 0 || limit >= m_history.size()) {
        return m_history;
    }

    return m_history.mid(0, limit);
}

void UserFeedbackManager::clearHistory() {
    QMutexLocker locker(&m_historyMutex);
    m_history.clear();
}

QString UserFeedbackManager::exportHistory(const QString& format) const {
    QMutexLocker locker(&m_historyMutex);

    if (format == "text") {
        QString output;
        output += "=== 用户反馈历史 ===\n\n";

        for (const auto& record : m_history) {
            QString typeStr;
            switch (record.type) {
                case FeedbackType::Info: typeStr = "信息"; break;
                case FeedbackType::Warning: typeStr = "警告"; break;
                case FeedbackType::Error: typeStr = "错误"; break;
                case FeedbackType::Success: typeStr = "成功"; break;
            }

            output += QString("[%1] %2 - %3\n")
                .arg(record.timestamp.toString("yyyy-MM-dd hh:mm:ss"))
                .arg(typeStr)
                .arg(record.title);
            output += QString("  消息: %1\n").arg(record.message);
            if (!record.context.isEmpty()) {
                output += QString("  上下文: %1\n").arg(record.context);
            }
            output += "\n";
        }

        return output;
    }

    // TODO: 支持其他格式（JSON、CSV等）
    return QString();
}

UserFeedbackManager::FeedbackStats UserFeedbackManager::getStats() const {
    QMutexLocker locker(&m_historyMutex);

    FeedbackStats stats;
    stats.totalCount = m_history.size();

    for (const auto& record : m_history) {
        switch (record.type) {
            case FeedbackType::Info: stats.infoCount++; break;
            case FeedbackType::Warning: stats.warningCount++; break;
            case FeedbackType::Error: stats.errorCount++; break;
            case FeedbackType::Success: stats.successCount++; break;
        }
        if (record.acknowledged) {
            stats.acknowledgedCount++;
        }
    }

    return stats;
}

// ========== 私有方法 ==========

FeedbackRecord UserFeedbackManager::createRecord(const QString& title, const QString& message,
                                                   FeedbackType type, FeedbackLevel level) {
    FeedbackRecord record;
    record.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    record.title = title;
    record.message = message;
    record.type = type;
    record.level = level;
    record.timestamp = QDateTime::currentDateTime();
    return record;
}

void UserFeedbackManager::recordFeedback(const FeedbackRecord& record) {
    QMutexLocker locker(&m_historyMutex);

    m_history.prepend(record);

    // 清理过期历史
    cleanupHistory();

    emit feedbackShown(record);
}

void UserFeedbackManager::showToast(const FeedbackRecord& record, int duration) {
    // 使用自定义 Toast 控件
    UI::ToastManager::instance()->showToast(
        record.title, record.message, record.type, duration
    );
}

void UserFeedbackManager::showDialog(const FeedbackRecord& record) {
    QMessageBox::Icon icon;
    switch (record.type) {
        case FeedbackType::Info:
            icon = QMessageBox::Information;
            break;
        case FeedbackType::Warning:
            icon = QMessageBox::Warning;
            break;
        case FeedbackType::Error:
            icon = QMessageBox::Critical;
            break;
        case FeedbackType::Success:
            icon = QMessageBox::Information;
            break;
    }

    QMessageBox box(icon, record.title, record.message, QMessageBox::Ok, qApp->activeWindow());
    box.exec();

    // 标记为已确认
    QMutexLocker locker(&m_historyMutex);
    if (!m_history.isEmpty() && m_history.first().id == record.id) {
        m_history.first().acknowledged = true;
    }

    emit feedbackAcknowledged(record.id);
}

void UserFeedbackManager::showNotification(const FeedbackRecord& record) {
    // TODO: 实现系统通知
    // 使用 QSystemTrayIcon 或第三方库
    qDebug() << "[Notification]" << record.title << ":" << record.message;
}

void UserFeedbackManager::cleanupHistory() {
    // 注意：调用此方法时必须已持有 m_historyMutex

    if (m_history.size() > m_maxHistorySize) {
        m_history = m_history.mid(0, m_maxHistorySize);
    }
}

} // namespace WealthPilot