#include "PortfolioPage.h"


struct PortfolioPage::Impl {
};

PortfolioPage::PortfolioPage(QWidget *parent)
    : BasePage(parent)
    , d(std::make_unique<Impl>())
{
    // 构造函数保持轻量，所有UI延迟到initializePage构建
    setObjectName("PortfolioPagePage");
}

PortfolioPage::~PortfolioPage() = default;

QString PortfolioPage::pageId() const
{
    return QStringLiteral("PortfolioPage");
}

void PortfolioPage::initializePage()
{

}

void PortfolioPage::setupUI()
{

}
