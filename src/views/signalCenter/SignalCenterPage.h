/**
 * @file SignalCenterPage.h
 * @brief 信号中心页面 - 使用 DataHub 数据中心
 *
 * @details 功能：
 * - 我的订阅、推荐信号、排行榜、最新上线四个分类
 * - 列表排序功能（胜率、订阅数、收益率）
 * - 订阅/取消订阅功能
 *
 * DataHub 集成：
 * - 通过 DataHub 订阅信号数据
 * - 自动生命周期管理
 * - 实时信号更新
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#ifndef SIGNALCENTERPAGE_H
#define SIGNALCENTERPAGE_H

#include <memory>
#include <ui/components/DataHubPageBase.h>
#include <QVector>

/**
 * @brief 信号卡片数据结构
 */
struct SignalCardData {
    QString id;             ///< 信号ID
    QString name;           ///< 信号名称
    double returnRate = 0.0; ///< 收益率
    int winRate = 0;        ///< 胜率
    int followers = 0;      ///< 订阅数
    double price = 0.0;     ///< 价格
    QString strategy;       ///< 策略类型
    QString description;    ///< 描述
    bool subscribed = false; ///< 是否已订阅
};

namespace WealthPilot {

/**
 * @brief 信号中心页面
 *
 * @details 继承 DataHubPageBase，自动管理数据订阅：
 * - 订阅信号列表（signal:list）
 * - 订阅我的订阅（signal:subscribed）
 * - 页面销毁时自动取消订阅
 */
class SignalCenterPage : public DataHubPageBase
{
    Q_OBJECT

public:
    explicit SignalCenterPage(QWidget *parent = nullptr);
    ~SignalCenterPage() override;

    // ========== 页面信息 ==========

    QString pageId() const override;
    QString pageName() const override { return QStringLiteral("信号中心"); }

    /**
     * @brief 初始化页面
     *
     * @details 初始化流程：
     * 1. 设置 UI 组件
     * 2. 订阅 DataHub 信号数据
     * 3. 加载初始数据
     */
    void initializePage() override;

private slots:
    // ========== UI 交互槽函数 ==========

    void onCategoryClicked(const QString& category);
    void onCardClicked();
    void onSubscribeClicked(const SignalCardData& data);
    void onUnsubscribeClicked(const SignalCardData& data);
    void onSortChanged(const QString& sortKey, bool ascending);

private:
    // ========== UI 初始化 ==========

    void setupUI();
    void setupCategoryBar();
    void setupToolBar();
    void setupScrollArea();
    void setupConnections();

    // ========== DataHub 数据订阅 ==========

    /**
     * @brief 设置 DataHub 数据订阅
     *
     * @details 订阅流程：
     * 1. 使用模式订阅 signal:*
     * 2. 回调函数中更新卡片列表
     */
    void setupDataHubSubscriptions();

    // ========== 数据加载 ==========

    void loadDemoData();
    void updateCards();

    QVector<SignalCardData> getFilteredSignals();
    void sortSignals(QVector<SignalCardData>& signalList);

    // ========== 私有实现类（PIMPL） ==========
    struct Impl;
    std::unique_ptr<Impl> d;

    // ========== DataHub 相关 ==========

    /**
     * @brief 已订阅的信号分类
     */
    QStringList m_subscribedCategories;
};

} // namespace WealthPilot

#endif // SIGNALCENTERPAGE_H