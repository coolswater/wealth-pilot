/**
 * @file SettingsPage.h
 * @brief 设置页面
 */

#ifndef ABOUTUSPAGE_H
#define ABOUTUSPAGE_H

#include <memory>

#include <ui/components/BasePage.h>

class QComboBox;
class QSlider;

/**
 * @brief 设置页面
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

private:
    void setupUI();

    struct Impl;
    std::unique_ptr<Impl> d;
};



 // ABOUTUSPAGE_H

#endif
