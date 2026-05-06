#ifndef SIGNALCENTERPAGE_H
#define SIGNALCENTERPAGE_H

#include <memory>

#include <ui/components/BasePage.h>

#include <ui/components//CardWidget.h>

namespace WealthPilot {
class SignalCenterPage : public BasePage
{
    Q_OBJECT
public:
    explicit SignalCenterPage(QWidget *parent = nullptr);
    ~SignalCenterPage();

    QString pageId() const override;
    void initializePage() override;

protected:
    void resizeEvent(QResizeEvent *event) override;
private:
    void setupUI();
    struct Impl;
    std::unique_ptr<Impl> d;
    CardWidget *createCard(const QString &name, double returnRate, int winRate, int followers);
    void updateGridLayout();
};


 // SIGNALCENTERPAGE_H

} // namespace WealthPilot

#endif
