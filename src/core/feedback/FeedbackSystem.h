/**
 * @file FeedbackSystem.h
 * @brief 用户反馈系统
 *
 * @details 功能：
 * - 收集用户反馈
 * - 反馈分类和优先级
 * - 反馈状态跟踪
 * - 反馈统计分析
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef FEEDBACKSYSTEM_H
#define FEEDBACKSYSTEM_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QVector>
#include <QJsonObject>
#include <QFile>

namespace WealthPilot {

/**
 * @brief 反馈类型
 */
enum class FeedbackType {
    BugReport,      ///< Bug 报告
    FeatureRequest, ///< 功能请求
    Improvement,    ///< 改进建议
    Question,       ///< 问题咨询
    Other           ///< 其他
};

/**
 * @brief 反馈优先级
 */
enum class FeedbackPriority {
    Low,        ///< 低
    Medium,     ///< 中
    High,       ///< 高
    Critical    ///< 紧急
};

/**
 * @brief 反馈状态
 */
enum class FeedbackStatus {
    New,        ///< 新建
    Reviewed,   ///< 已审核
    InProgress, ///< 处理中
    Resolved,   ///< 已解决
    Closed      ///< 已关闭
};

/**
 * @brief 反馈信息
 */
struct FeedbackInfo {
    QString id;                 ///< 反馈ID
    FeedbackType type;          ///< 反馈类型
    FeedbackPriority priority;  ///< 优先级
    FeedbackStatus status;      ///< 状态
    QString title;              ///< 标题
    QString content;            ///< 内容
    QString category;           ///< 分类
    QString userId;             ///< 用户ID
    QString userEmail;          ///< 用户邮箱
    QString userVersion;        ///< 用户版本
    QString userPlatform;       ///< 用户平台
    QStringList attachments;    ///< 附件列表
    QString assignee;           ///< 处理人
    QString resolution;         ///< 解决方案
    QDateTime createTime;       ///< 创建时间
    QDateTime updateTime;       ///< 更新时间
    QDateTime resolveTime;      ///< 解决时间
    int votes = 0;              ///< 投票数
    QStringList tags;           ///< 标签
};

/**
 * @brief 反馈统计
 */
struct FeedbackStats {
    int totalCount = 0;         ///< 总数
    int newCount = 0;           ///< 新建数
    int inProgressCount = 0;    ///< 处理中数
    int resolvedCount = 0;      ///< 已解决数
    int closedCount = 0;        ///< 已关闭数
    int bugCount = 0;           ///< Bug 数
    int featureCount = 0;       ///< 功能请求数
    int improvementCount = 0;   ///< 改进建议数
    int questionCount = 0;      ///< 问题数
    double avgResolutionTime = 0.0; ///< 平均解决时间 (小时)
};

/**
 * @brief 用户反馈系统
 */
class FeedbackSystem : public QObject
{
    Q_OBJECT

public:
    static FeedbackSystem& instance();

    /**
     * @brief 初始化系统
     * @param storagePath 存储路径
     */
    void initialize(const QString& storagePath);

    /**
     * @brief 提交反馈
     * @param feedback 反馈信息
     * @return 反馈ID
     */
    QString submitFeedback(const FeedbackInfo& feedback);

    /**
     * @brief 更新反馈
     * @param feedback 反馈信息
     */
    bool updateFeedback(const FeedbackInfo& feedback);

    /**
     * @brief 获取反馈
     * @param feedbackId 反馈ID
     */
    FeedbackInfo getFeedback(const QString& feedbackId) const;

    /**
     * @brief 获取所有反馈
     */
    QVector<FeedbackInfo> getAllFeedbacks() const;

    /**
     * @brief 按状态获取反馈
     */
    QVector<FeedbackInfo> getFeedbacksByStatus(FeedbackStatus status) const;

    /**
     * @brief 按类型获取反馈
     */
    QVector<FeedbackInfo> getFeedbacksByType(FeedbackType type) const;

    /**
     * @brief 搜索反馈
     * @param keyword 关键词
     */
    QVector<FeedbackInfo> searchFeedbacks(const QString& keyword) const;

    /**
     * @brief 删除反馈
     */
    bool deleteFeedback(const QString& feedbackId);

    /**
     * @brief 获取统计信息
     */
    FeedbackStats getStats() const;

    /**
     * @brief 导出反馈
     * @param format 导出格式 (json, csv)
     */
    bool exportFeedbacks(const QString& filePath, const QString& format = "json");

    /**
     * @brief 投票
     */
    bool voteFeedback(const QString& feedbackId);

signals:
    /**
     * @brief 反馈提交成功
     */
    void feedbackSubmitted(const QString& feedbackId);

    /**
     * @brief 反馈更新
     */
    void feedbackUpdated(const QString& feedbackId);

    /**
     * @brief 反馈状态变更
     */
    void feedbackStatusChanged(const QString& feedbackId, FeedbackStatus newStatus);

private:
    FeedbackSystem();
    ~FeedbackSystem();
    FeedbackSystem(const FeedbackSystem&) = delete;
    FeedbackSystem& operator=(const FeedbackSystem&) = delete;

    void saveFeedbacks();
    void loadFeedbacks();
    QString generateId();

    QString m_storagePath;
    QVector<FeedbackInfo> m_feedbacks;
    bool m_initialized = false;
};

} // namespace WealthPilot

#endif // FEEDBACKSYSTEM_H