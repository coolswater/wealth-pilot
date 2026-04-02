/**
 * @file FuturesQuotesPage.h
 * @brief 期货行情页面 - CTP实时行情
 *
 * 功能：
 * - 显示期货合约行情列表
 * - 连接Simnow行情服务器
 * - 自动查询并订阅所有合约
 * - 显示实时价格变动
 */
#pragma once

#include <QTableView>
#include <memory>
#include <core/BasePage.h>
#include <services/CTPService.h>
#include <QTimer>

QT_BEGIN_NAMESPACE
class QTableView;
class QLabel;
class QLineEdit;
class QSortFilterProxyModel;
QT_END_NAMESPACE

class FuturesQuotesPage : public BasePage
{
    Q_OBJECT
public:
    explicit FuturesQuotesPage(QWidget* parent = nullptr);
    ~FuturesQuotesPage();

    QString pageId() const override;
    void initializePage() override;
    void onPageActivated(const QVariantMap &params = {}) override;
    void onPageDeactivated() override;

    void flushPendingUpdates();
    void setupCtpConnections();
    void updateConnectionStatus(const QString& text, const QString& color);

private slots:
    void onRowClicked(const QModelIndex &index);
    void onCtpBatchMarketData(const QList<CTP::MarketData>& dataList);
    void onCtpSingleMarketData(const CTP::MarketData& data);
    void onSubscribeContract();
    void subscribeContracts(const QList<QString>& contracts);
    void onInstrumentQueried(const QString& instrumentId, const QString& exchangeId,
                             const QString& instrumentName, double priceTick, int volumeMultiple);
    void onInstrumentQueryFinished(int totalCount);

private:
    void setupUI();
    void setupConnections();

    class Impl;
    std::unique_ptr<Impl> d;
};
