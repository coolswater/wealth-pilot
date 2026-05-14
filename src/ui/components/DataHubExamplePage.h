/**
 * @file DataHubExamplePage.h
 * @brief DataHub 示例页面 - 展示如何使用 DataHub 订阅数据
 *
 * @details 这是一个示例页面，展示：
 * - 如何继承 DataHubPageBase
 * - 如何订阅股票行情
 * - 如何订阅行情快照
 * - 如何请求刷新数据
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef DATAHUBEXAMPLEPAGE_H
#define DATAHUBEXAMPLEPAGE_H

#include "ui/components/DataHubPageBase.h"
#include <QLabel>
#include <QTableWidget>
#include <QPushButton>

namespace WealthPilot {
namespace UI {

/**
 * @brief DataHub 示例页面
 * 
 * 这个页面展示了如何使用 DataHub 进行数据订阅
 * 可以作为其他页面迁移的参考模板
 */
class DataHubExamplePage : public DataHubPageBase
{
    Q_OBJECT

public:
    explicit DataHubExamplePage(QWidget* parent = nullptr);
    ~DataHubExamplePage() override;

    void initializePage() override;

private slots:
    void onRefreshClicked();
    void onSubscribeNewSymbol();

private:
    void setupUI();
    void setupDataHubSubscriptions();
    void updateQuoteDisplay(const QString& symbol, const StockQuote& quote);
    void updateSnapshotDisplay(const QString& symbol, const MarketSnapshot& snapshot);

    // UI 组件
    QLabel* m_titleLabel;
    QTableWidget* m_quoteTable;
    QPushButton* m_refreshBtn;
    QPushButton* m_subscribeBtn;

    // 订阅的股票列表
    QStringList m_subscribedSymbols;
};

} // namespace UI
} // namespace WealthPilot

#endif // DATAHUBEXAMPLEPAGE_H