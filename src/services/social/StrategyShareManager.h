/**
 * @file StrategyShareManager.h
 * @brief 策略分享管理器 - 社交交易核心
 *
 * @details 功能：
 * - 发布交易策略
 * - 策略订阅
 * - 策略评分
 * - 跟单交易
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef STRATEGYSHAREMANAGER_H
#define STRATEGYSHAREMANAGER_H

#include <QObject>
#include <QVector>
#include <QString>
#include <QDateTime>
#include <QHash>

/**
 * @brief 分享策略信息
 */
struct SharedStrategy {
    QString id;                 ///< 策略ID
    QString name;               ///< 策略名称
    QString description;        ///< 描述
    QString authorId;           ///< 作者ID
    QString authorName;         ///< 作者名称
    QDateTime publishTime;      ///< 发布时间
    int subscribers = 0;        ///< 订阅数
    double rating = 0.0;        ///< 评分
    int ratingCount = 0;        ///< 评分人数
    double totalReturn = 0.0;   ///< 总收益率
    double maxDrawdown = 0.0;   ///< 最大回撤
    double sharpeRatio = 0.0;   ///< 夏普比率
    int tradeCount = 0;         ///< 交易次数
    double winRate = 0.0;       ///< 胜率
    QString strategyData;       ///< 策略数据（JSON）
    QStringList tags;           ///< 标签
    bool isPublic = true;       ///< 是否公开
};

/**
 * @brief 订阅记录
 */
struct Subscription {
    QString id;                 ///< 订阅ID
    QString strategyId;         ///< 策略ID
    QString userId;             ///< 用户ID
    QDateTime subscribeTime;    ///< 订阅时间
    bool autoFollow = false;    ///< 自动跟单
    double followRatio = 1.0;   ///< 跟单比例
};

/**
 * @brief 策略信号
 */
struct StrategyTradeSignal {
    QString strategyId;         ///< 策略ID
    QString symbol;             ///< 股票代码
    QString action;             ///< 动作
    double price;               ///< 价格
    int quantity;               ///< 数量
    QDateTime time;             ///< 时间
    QString reason;             ///< 原因
};

/**
 * @brief 策略评分
 */
struct StrategyRating {
    QString strategyId;         ///< 策略ID
    QString userId;             ///< 用户ID
    double rating = 0.0;        ///< 评分 (1-5)
    QString comment;            ///< 评论
    QDateTime time;             ///< 时间
};

/**
 * @brief 跟单配置
 */
struct FollowConfig {
    bool enabled = false;       ///< 是否启用跟单
    double ratio = 1.0;         ///< 跟单比例
    double maxAmount = 0.0;     ///< 最大跟投金额
};

/**
 * @brief 策略分享管理器
 *
 * 提供社交交易功能：
 * - 策略发布和订阅
 * - 策略评分和评论
 * - 跟单交易
 */
class StrategyShareManager : public QObject {
    Q_OBJECT

public:
    static StrategyShareManager* instance();

    // ========== 策略发布 ==========

    /**
     * @brief 发布策略
     */
    bool publishStrategy(const SharedStrategy& strategy);

    /**
     * @brief 更新策略
     */
    bool updateStrategy(const SharedStrategy& strategy);

    /**
     * @brief 删除策略
     */
    bool deleteStrategy(const QString& strategyId);

    /**
     * @brief 获取策略详情
     */
    SharedStrategy getStrategy(const QString& strategyId) const;

    /**
     * @brief 获取热门策略
     */
    QVector<SharedStrategy> getHotStrategies(int limit = 20) const;

    /**
     * @brief 获取最新策略
     */
    QVector<SharedStrategy> getLatestStrategies(int limit = 20) const;

    /**
     * @brief 搜索策略
     */
    QVector<SharedStrategy> searchStrategies(const QString& keyword,
                                             const QStringList& tags = {}) const;

    // ========== 订阅管理 ==========

    /**
     * @brief 订阅策略
     */
    bool subscribeStrategy(const QString& strategyId,
                          bool autoFollow = false,
                          double followRatio = 1.0);

    /**
     * @brief 取消订阅
     */
    bool unsubscribeStrategy(const QString& strategyId);

    /**
     * @brief 获取订阅列表
     */
    QVector<Subscription> getSubscriptions() const;

    /**
     * @brief 获取策略订阅者
     */
    QVector<Subscription> getSubscribers(const QString& strategyId) const;

    // ========== 评分系统 ==========

    /**
     * @brief 评分策略
     */
    bool rateStrategy(const QString& strategyId, double rating, const QString& comment = QString());

    /**
     * @brief 获取策略评分
     */
    QVector<StrategyRating> getRatings(const QString& strategyId) const;

    /**
     * @brief 获取用户评分
     */
    StrategyRating getUserRating(const QString& strategyId, const QString& userId) const;

    // ========== 跟单交易 ==========

    /**
     * @brief 设置跟单参数
     */
    void setFollowConfig(const QString& strategyId,
                        bool enabled,
                        double ratio,
                        double maxAmount = 0);

    /**
     * @brief 处理策略信号
     */
    void processSignal(const StrategyTradeSignal& signal);

    /**
     * @brief 获取跟单记录
     */
    QVector<StrategyTradeSignal> getFollowRecords(const QString& strategyId = QString()) const;

signals:
    /**
     * @brief 策略发布信号
     */
    void strategyPublished(const SharedStrategy& strategy);

    /**
     * @brief 订阅变化信号
     */
    void subscriptionChanged(const QString& strategyId, bool subscribed);

    /**
     * @brief 收到交易信号
     */
    void signalReceived(const StrategyTradeSignal& signal);

    /**
     * @brief 评分更新信号
     */
    void ratingUpdated(const QString& strategyId, double newRating);

private:
    explicit StrategyShareManager(QObject* parent = nullptr);
    ~StrategyShareManager() override = default;

    void updateStrategyRating(const QString& strategyId);
    void executeFollowTrade(const StrategyTradeSignal& signal, const Subscription& sub);

    QHash<QString, SharedStrategy> m_strategies;
    QHash<QString, QVector<Subscription>> m_subscribers;
    QHash<QString, QVector<StrategyRating>> m_ratings;
    QVector<Subscription> m_mySubscriptions;
    QVector<StrategyTradeSignal> m_followRecords;
    QHash<QString, FollowConfig> m_followConfigs;  ///< 跟单配置

    QString m_currentUserId;
};

#endif // STRATEGYSHAREMANAGER_H