/**
 * @file AboutUSPage.h
 * @brief 关于页面
 */

#ifndef ABOUTUSPAGE_H
#define ABOUTUSPAGE_H

#include <memory>
#include <ui/components/BasePage.h>

/**
 * @brief 关于页面
 */
class AboutUSPage : public WealthPilot::BasePage
{
    Q_OBJECT

public:
    explicit AboutUSPage(QWidget *parent = nullptr);
    ~AboutUSPage();

    QString pageId() const override;
    void initializePage() override;

private slots:
    void onCheckUpdateClicked();
    void onVisitWebsiteClicked();
    void onViewLicenseClicked();

private:
    void setupUI();
    
    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // ABOUTUSPAGE_H
