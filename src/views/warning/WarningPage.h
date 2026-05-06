/**
 * @file WarningPage.h
 * @brief 预警页面
 */

#ifndef WARNINGPAGE_H
#define WARNINGPAGE_H

#include <memory>
#include "ui/components/BasePage.h"

namespace WealthPilot {

class WarningPage : public BasePage
{
    Q_OBJECT
public:
    explicit WarningPage(QWidget *parent = nullptr);
    ~WarningPage();

    QString pageId() const override { return "warning"; }
    QString pageName() const override { return QStringLiteral("预警"); }
    void initializePage() override;
private:
    void setupUI();
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace WealthPilot

#endif // WARNINGPAGE_H
