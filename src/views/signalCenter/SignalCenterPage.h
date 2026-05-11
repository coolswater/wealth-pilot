/**
 * @file SignalCenterPage.h
 * @brief 信号中心页面
 *
 * @details 功能：
 * - 我的订阅、推荐信号、排行榜、最新上线四个分类
 * - 列表排序功能（胜率、订阅数、收益率）
 * - 订阅/取消订阅功能
 */

#ifndef SIGNALCENTERPAGE_H
#define SIGNALCENTERPAGE_H

#include <memory>
#include <ui/components/BasePage.h>
#include <QVector>

// 信号卡片数据结构
struct SignalCardData {
    QString id;
    QString name;
    double returnRate = 0.0;
    int winRate = 0;
    int followers = 0;
    double price = 0.0;
    QString strategy;
    QString description;
    bool subscribed = false;  // 改名为 subscribed 避免与 public 冲突
};

namespace WealthPilot {

class SignalCenterPage : public BasePage
{
    Q_OBJECT

public:
    explicit SignalCenterPage(QWidget *parent = nullptr);
    ~SignalCenterPage();

    QString pageId() const override;
    void initializePage() override;

private slots:
    void onCategoryClicked(const QString& category);
    void onCardClicked();
    void onSubscribeClicked(const SignalCardData& data);
    void onUnsubscribeClicked(const SignalCardData& data);

private:
    void setupUI();
    void setupCategoryBar();
    void setupToolBar();
    void setupScrollArea();
    void loadDemoData();
    void updateCards();

    QVector<SignalCardData> getFilteredSignals();
    void sortSignals(QVector<SignalCardData>& signalList);

    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace WealthPilot

#endif // SIGNALCENTERPAGE_H
