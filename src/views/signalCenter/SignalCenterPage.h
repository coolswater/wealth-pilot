/**
 * @file SignalCenterPage.h
 * @brief 信号中心页面 - 参考资讯页面样式优化
 */

#ifndef SIGNALCENTERPAGE_H
#define SIGNALCENTERPAGE_H

#include <memory>
#include <ui/components/BasePage.h>

// 前向声明
class SignalCardWidget;

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

private:
    void setupUI();
    void setupCategoryBar();
    void setupScrollArea();
    void loadDemoData();
    void updateCards(const QString& filter = QString());

    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace WealthPilot

#endif // SIGNALCENTERPAGE_H
