#ifndef WARNINGPAGE_H
#define WARNINGPAGE_H

#include <memory>

#include <core/base/BasePage.h>

class WarningPage : public BasePage
{
    Q_OBJECT
public:
    explicit WarningPage(QWidget *parent = nullptr);
    ~WarningPage();

    QString pageId() const override;
    void initializePage() override;
private:
    void setupUI();
    struct Impl;
    std::unique_ptr<Impl> d;
};
#endif // ALERTSPAGE_H

