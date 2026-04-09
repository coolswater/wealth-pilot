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
    ~FuturesQuotesPage() override;

    QString pageId() const override;
    void initializePage() override;
    void onPageActivated(const QVariantMap &params = {}) override;
    void onPageDeactivated() override;

    void flushPendingUpdates() const;
    void setupCtpConnections();
    void onInstrumentQueried(const QString& instrumentId, const QString& exchangeId, const QString& instrumentName,
                             double priceTick, int volumeMultiple);
    void updateConnectionStatus(const QString& text, const QString& color) const;

private slots:
    void onRowClicked(const QModelIndex &index) const;
    void onCtpBatchMarketData(const QList<CTP::MarketData>& dataList);
    bool updateContractActivity(const QString& contractId, const CTP::MarketData& data, qint64 currentTime);
    void updateMainContractByVolume(const QString& contractId, int volume) const;
    bool shouldDisplayContract(const QString& contractId, const CTP::MarketData& data) const;
    void onCtpSingleMarketData(const CTP::MarketData& data);
    void updateMainContracts();
    void onSubscribeContract();
    void subscribeContracts(const QList<QString>& contracts) const;
    void onInstrumentQueryFinished(int totalCount);
    static std::tuple<QString, QDate, bool> parseContractCode(const QString& contractId);
    void identifyMainContracts() const;
    void subscribeContractsByPriority();
    void subscribeContractsInBatches(const QList<QString>& contracts) const;
    void setActivityFilterMode(int mode) const;

private:
    void setupUI();
    void setupConnections();
    void initializeSampleData() const;  // 【新增】初始化示例数据

    class Impl;
    std::unique_ptr<Impl> d;
};
