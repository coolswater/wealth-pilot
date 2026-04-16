#ifndef NEWSPAGE_H
#define NEWSPAGE_H

#include <memory>

#include <core/base/BasePage.h>

class NewsPage : public BasePage
{
    Q_OBJECT
public:
    explicit NewsPage(QWidget *parent = nullptr);
    ~NewsPage();

    QString pageId() const override;
    void initializePage() override;

private:
    void setupUI();
    struct Impl;
    std::unique_ptr<Impl> d;
};
#endif // NEWSPAGE_H

