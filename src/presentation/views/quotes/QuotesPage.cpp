/**
 * @file QuotesPage.cpp
 * @brief 行情页面实现 - 整合股票/期货/基金/外汇/数字货币
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#include "QuotesPage.h"
#include "presentation/views/stock/StockQuotesPage.h"
#include "presentation/views/futures/FuturesQuotesPage.h"
#include "presentation/views/fund/FundPage.h"
#include "presentation/views/forex/ForexPage.h"
#include "presentation/views/crypto/CryptoPage.h"
#include "shared/utils/Logger.h"

// 使用 WealthPilot 命名空间中的类
using WealthPilot::StockQuotesPage;
using WealthPilot::FuturesQuotesPage;
using WealthPilot::CryptoPage;
// FundPage 和 ForexPage 不在 WealthPilot 命名空间中

#include <QVBoxLayout>
#include <QTabWidget>
#include <QTabBar>
#include <QLabel>

namespace WealthPilot
{
    QuotesPage::QuotesPage(QWidget* parent)
        : DataHubPageBase(parent)
          , m_tabWidget(nullptr)
          , m_stockPage(nullptr)
          , m_futuresPage(nullptr)
          , m_fundPage(nullptr)
          , m_forexPage(nullptr)
          , m_cryptoPage(nullptr)
    {
        setupUI();
        setupConnections();

        LOG_DEBUG("QuotesPage created");
    }

    QuotesPage::~QuotesPage()
    {
        LOG_DEBUG("QuotesPage destroyed");
    }

    void QuotesPage::initializePage()
    {
        if (isInitialized())
        {
            return;
        }

        // ============================================================
        // 1. 设置 DataHub 订阅
        // ============================================================
        setupDataHubSubscriptions();

        // ============================================================
        // 2. 初始化各子页面
        // ============================================================
        if (m_stockPage)
        {
            m_stockPage->initializePage();
        }
        if (m_futuresPage)
        {
            m_futuresPage->initializePage();
        }
        if (m_fundPage)
        {
            m_fundPage->initializePage();
        }
        if (m_forexPage)
        {
            m_forexPage->initializePage();
        }
        if (m_cryptoPage)
        {
            m_cryptoPage->initializePage();
        }

        setInitialized(true);
        LOG_INFO("QuotesPage initialized with DataHub");
    }

    void QuotesPage::setupDataHubSubscriptions()
    {
        // 订阅所有市场行情
        dataHub().subscribePattern(this, "market:*",
            [this](const QString& topic, const QVariant& value) {
                // 根据当前 Tab 更新对应市场
                Q_UNUSED(topic)
                Q_UNUSED(value)
            });
        
        // Tab 切换时订阅对应市场
        connect(m_tabWidget, &QTabWidget::currentChanged, this, [this](int index) {
            Q_UNUSED(index)
            // 更新当前市场
            m_currentMarket = getCurrentMarket();
        });
        
        LOG_INFO("[QuotesPage] DataHub subscriptions setup complete");
    }
    
    QString QuotesPage::getCurrentMarket() const
    {
        if (!m_tabWidget) return QString();
        
        int index = m_tabWidget->currentIndex();
        return m_marketIndexMap.key(index, QString());
    }

    void QuotesPage::switchToMarket(const QString& market)
    {
        if (m_marketIndexMap.contains(market))
        {
            m_tabWidget->setCurrentIndex(m_marketIndexMap[market]);
        }
    }

    void QuotesPage::setupUI()
    {
        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);

        // 创建 Tab 控件
        m_tabWidget = new QTabWidget(this);
        m_tabWidget->setObjectName("quotesTabWidget");
        m_tabWidget->setTabPosition(QTabWidget::North);
        m_tabWidget->setDocumentMode(true);
        m_tabWidget->setMovable(false);

        // 添加各市场行情页
        int index = 0;

        // 股票行情
        QWidget* stockPage = createStockPage();
        m_tabWidget->addTab(stockPage, QStringLiteral("股票"));
        m_marketIndexMap["stock"] = index++;

        // 期货行情
        QWidget* futuresPage = createFuturesPage();
        m_tabWidget->addTab(futuresPage, QStringLiteral("期货"));
        m_marketIndexMap["futures"] = index++;

        // 基金行情
        QWidget* fundPage = createFundPage();
        m_tabWidget->addTab(fundPage, QStringLiteral("基金"));
        m_marketIndexMap["fund"] = index++;

        // 外汇行情
        QWidget* forexPage = createForexPage();
        m_tabWidget->addTab(forexPage, QStringLiteral("外汇"));
        m_marketIndexMap["forex"] = index++;

        // 数字货币行情
        QWidget* cryptoPage = createCryptoPage();
        m_tabWidget->addTab(cryptoPage, QStringLiteral("数字货币"));
        m_marketIndexMap["crypto"] = index++;

        mainLayout->addWidget(m_tabWidget);
    }

    QWidget* QuotesPage::createStockPage()
    {
        m_stockPage = new StockQuotesPage(this);
        return m_stockPage;
    }

    QWidget* QuotesPage::createFuturesPage()
    {
        m_futuresPage = new FuturesQuotesPage(this);
        return m_futuresPage;
    }

    QWidget* QuotesPage::createFundPage()
    {
        m_fundPage = new FundPage(this);
        return m_fundPage;
    }

    QWidget* QuotesPage::createForexPage()
    {
        m_forexPage = new ForexPage(this);
        return m_forexPage;
    }

    QWidget* QuotesPage::createCryptoPage()
    {
        m_cryptoPage = new CryptoPage(this);
        return m_cryptoPage;
    }

    void QuotesPage::setupConnections()
    {
        // 连接股票页面的导航信号
        if (m_stockPage)
        {
            connect(m_stockPage, &StockQuotesPage::navigateToKLinePage,
                    this, &QuotesPage::navigateToKLinePage);
        }

        // 连接期货页面的导航信号
        if (m_futuresPage)
        {
            connect(m_futuresPage, &FuturesQuotesPage::navigateToKLinePage,
                    this, [this](const QString& symbol, const QVariantMap& params)
                    {
                        Q_UNUSED(params);
                        emit navigateToKLinePage(symbol, symbol);
                    });
        }
        
        // 连接 Tab 切换信号
        if (m_tabWidget)
        {
            connect(m_tabWidget, &QTabWidget::currentChanged, this, &QuotesPage::onTabChanged);
        }
    }
    
    void QuotesPage::onTabChanged(int index)
    {
        // 更新当前市场
        m_currentMarket = getCurrentMarket();
        LOG_DEBUG(QString("Tab changed to index %1, market: %2").arg(index).arg(m_currentMarket));
    }
} // namespace WealthPilot