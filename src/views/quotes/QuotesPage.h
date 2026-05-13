/**
 * @file QuotesPage.h
 * @brief 行情页面 - 整合股票/期货/基金/外汇/数字货币
 *
 * @details 使用 Tab 切换不同市场行情：
 * - 股票行情
 * - 期货行情
 * - 基金行情
 * - 外汇行情
 * - 数字货币行情
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#ifndef QUOTESPAGE_H
#define QUOTESPAGE_H

#include "ui/components/BasePage.h"
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
 */
    class QuotesPage : public BasePage
    {
        Q_OBJECT

    public:
        /**
     * @brief 构造函数
     * @param parent 父窗口
     */
        explicit QuotesPage(QWidget* parent = nullptr);

        /**
     * @brief 析构函数
     */
        ~QuotesPage() override;

        /**
     * @brief 获取页面ID
     */
        QString pageId() const override { return QStringLiteral("quotes"); }

        /**
     * @brief 获取页面名称
     */
        QString pageName() const override { return QStringLiteral("行情"); }

        /**
     * @brief 初始化页面
     */
        void initializePage() override;

        /**
     * @brief 切换到指定市场
     * @param market 市场名称 (stock/futures/fund/forex/crypto)
     */
        void switchToMarket(const QString& market);

        signals :
        /**
     * @brief 导航到K线页面信号
     */

        void navigateToKLinePage(const QString& symbol, const QString& name);

    private:
        /**
     * @brief 设置 UI
     */
        void setupUI();

        /**
     * @brief 创建股票行情页
     */
        QWidget* createStockPage();

        /**
     * @brief 创建期货行情页
     */
        QWidget* createFuturesPage();

        /**
     * @brief 创建基金行情页
     */
        QWidget* createFundPage();

        /**
     * @brief 创建外汇行情页
     */
        QWidget* createForexPage();

        /**
     * @brief 创建数字货币行情页
     */
        QWidget* createCryptoPage();

        /**
     * @brief 设置信号连接
     */
        void setupConnections();

    private:
        QTabWidget* m_tabWidget; ///< Tab 控件
        StockQuotesPage* m_stockPage; ///< 股票行情页
        FuturesQuotesPage* m_futuresPage; ///< 期货行情页
        FundPage* m_fundPage; ///< 基金行情页
        ForexPage* m_forexPage; ///< 外汇行情页
        CryptoPage* m_cryptoPage; ///< 数字货币行情页
        QMap<QString, int> m_marketIndexMap; ///< 市场名称到 Tab 索引的映射
    };
} // namespace WealthPilot

#endif // QUOTESPAGE_H