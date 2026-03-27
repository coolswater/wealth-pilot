#ifndef PORTFOLIOPAGE_H
#define PORTFOLIOPAGE_H

#include <core/BasePage.h>



class PortfolioPage : public BasePage
{
    Q_OBJECT
public:
    explicit PortfolioPage(QWidget *parent = nullptr);
    ~PortfolioPage();

    QString pageId() const override;
    void initializePage() override;

private:
    void setupUI();
    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // PORTFOLIOPAGE_H
