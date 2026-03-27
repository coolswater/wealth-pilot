#ifndef SIGNALCENTERPAGE_H
#define SIGNALCENTERPAGE_H

#include <memory>

#include <core/BasePage.h>

class SignalCenterPage : public BasePage
{
    Q_OBJECT
public:
    explicit SignalCenterPage(QWidget *parent = nullptr);
    ~SignalCenterPage();

    QString pageId() const override;
    void initializePage() override;

private:
    void setupUI();
    struct Impl;
    std::unique_ptr<Impl> d;
};
#endif // SIGNALCENTERPAGE_H
