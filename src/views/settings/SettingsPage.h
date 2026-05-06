/**
 * @file SettingsPage.h
 * @brief 设置页面
 */

#ifndef SETTINGSPAGE_H
#define SETTINGSPAGE_H

#include <memory>

#include <ui/components/BasePage.h>

class QComboBox;
class QSlider;
class QLineEdit;

/**
 * @brief 设置页面
 */
class SettingsPage : public WealthPilot::BasePage
{
    Q_OBJECT

public:
    explicit SettingsPage(QWidget *parent = nullptr);
    ~SettingsPage();

    QString pageId() const override;
    void initializePage() override;


private slots:
    void onThemeChanged(int index);
    void onFontSizeChanged(int value);
    void onNotificationChanged();
    void onClearCacheClicked();
    void onExportDataClicked();
    void onAIConfigChanged();

private:
    void setupUI();
    void createAppearanceSection();
    void createNotificationSection();
    void createSecuritySection();
    void createAISection();
    void createAboutSection();
    void loadSettings();
    void saveSettings();

    struct Impl;
    std::unique_ptr<Impl> d;
};



 // SETTINGSPAGE_H

#endif
