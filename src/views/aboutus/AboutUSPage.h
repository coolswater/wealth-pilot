/**
 * @file SettingsPage.h
 * @brief 设置页面
 */

#ifndef ABOUTUSPAGE_H
#define ABOUTUSPAGE_H

#include <memory>

#include <core/BasePage.h>

class QComboBox;
class QSlider;

/**
 * @brief 设置页面
 */
class AboutUSPage : public BasePage
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

#endif // ABOUTUSPAGE_H
