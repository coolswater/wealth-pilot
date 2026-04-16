#ifndef STOCKQUOTES_H
#define STOCKQUOTES_H

#include <memory>

#include <core/base/BasePage.h>

class StockQuotesPage: public BasePage
{
    Q_OBJECT

public:
    explicit StockQuotesPage(QWidget *parent = nullptr);
    ~StockQuotesPage();

    QString pageId() const override;
    void initializePage() override;

private:
    void setupUI();

    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // STOCKQUOTES_H

