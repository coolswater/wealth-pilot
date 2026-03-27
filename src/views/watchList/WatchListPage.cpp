#include "WatchListPage.h"


struct WatchListPage::Impl {
};

WatchListPage::WatchListPage(QWidget *parent)
    : BasePage(parent)
    , d(std::make_unique<Impl>())
{
    // 构造函数保持轻量，所有UI延迟到initializePage构建
    setObjectName("WatchListPagePage");
}

WatchListPage::~WatchListPage() = default;

QString WatchListPage::pageId() const
{
    return QStringLiteral("WatchListPage");
}

void WatchListPage::initializePage()
{

}

void WatchListPage::setupUI()
{

}

void WatchListPage::setupAnimations()
{

}

void WatchListPage::connectSignals()
{

}
