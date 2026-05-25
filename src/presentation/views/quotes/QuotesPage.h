/**
 * @file QuotesPage.h
 * @brief 行情页面 - 使用 DataHub 数据中心
 *
 * @details 使用 Tab 切换不同市场行情：
 * - 股票行情
 * - 期货行情
 * - 基金行情
 * - 外汇行情
 * - 数字货币行情
 *
 * DataHub 集成：
 * - 通过 DataHub 订阅各市场行情数据
 * - 自动生命周期管理
 * - Tab 切换时自动订阅/取消订阅
 *
 * @author WealthPilot Team
 * @version 3.0.0
 */

#ifndef QUOTESPAGE_H
#define QUOTESPAGE_H

#include "presentation/components/DataHubPageBase.h"
#include <QTabWidget>
#include <QMap>

class QTabWidget;

namespace WealthPilot
{
    // 前向声明
    class StockQuotesPage;
    class FuturesQuotesPage;
    class CryptoPage;
} // namespace WealthPilot

// 前向声明（不在 WealthPilot 命名空间中）
class FundPage;
class ForexPage;

namespace WealthPilot
{
    /**
     * @brief 行情页面 - 整合多市场行情
     *
     * @details 继承 DataHubPageBase，自动管理数据订阅：
     * - Tab 切换时订阅对应市场数据
     * - 页面销毁时自动取消所有订阅
     */
    class QuotesPage : public DataHubPageBase
    {
        Q_OBJECT

    public:
        explicit QuotesPage(QWidget* parent = nullptr);
        ~QuotesPage() override;

        // ========== 页面信息 ==========

        QString pageId() const override { return QStringLiteral("quotes"); }
        QString pageName() const override { return QStringLiteral("行情"); }

        /**
         * @brief 初始化页面
         *
         * @details 初始化流程：
         * 1. 设置 UI 组件（Tab 控件）
         * 2. 创建各市场行情子页面
         * 3. 订阅当前 Tab 的数据
         */
        void initializePage() override;

        /**
         * @brief 切换到指定市场
         * @param market 市场名称 (stock/futures/fund/forex/crypto)
         */
        void switchToMarket(const QString& market);

    signals:
        void navigateToKLinePage(const QString& symbol, const QString& name);

    private slots:
        /**
         * @brief Tab 切换槽函数
         * @param index Tab 索引
         */
        void onTabChanged(int index);

    private:
        // ========== UI 初始化 ==========

        void setupUI();
        QWidget* createStockPage();
        QWidget* createFuturesPage();
        QWidget* createFundPage();
        QWidget* createForexPage();
        QWidget* createCryptoPage();
        void setupConnections();
        
        /**
         * @brief 获取当前选中的市场名称
         * @return 市场名称 (stock/futures/fund/forex/crypto)
         */
        QString getCurrentMarket() const;

        // ========== DataHub 数据订阅 ==========

        /**
         * @brief 设置 DataHub 数据订阅
         *
         * @details 订阅流程：
         * 1. 根据当前 Tab 订阅对应市场数据
         * 2. Tab 切换时更新订阅
         */
        void setupDataHubSubscriptions();

        // ========== 成员变量 ==========

        QTabWidget* m_tabWidget = nullptr;
        StockQuotesPage* m_stockPage = nullptr;
        FuturesQuotesPage* m_futuresPage = nullptr;
        FundPage* m_fundPage = nullptr;
        ForexPage* m_forexPage = nullptr;
        CryptoPage* m_cryptoPage = nullptr;
        QMap<QString, int> m_marketIndexMap;

        /**
         * @brief 当前激活的市场
         */
        QString m_currentMarket;
    };
} // namespace WealthPilot

#endif // QUOTESPAGE_H