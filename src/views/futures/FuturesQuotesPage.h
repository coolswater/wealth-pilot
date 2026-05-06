/**
 * @file FuturesQuotesPage.h
 * @brief 期货行情页面
 * @details 显示期货实时行情列表，支持CTP实时数据、主力合约识别、活跃度筛选
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef FUTURESQUOTESPAGE_H
#define FUTURESQUOTESPAGE_H

#include <QTableView>
#include <QSortFilterProxyModel>
#include <QTimer>
#include <memory>
#include <atomic>
#include <ui/components/BasePage.h>

// 前向声明
QT_BEGIN_NAMESPACE
class QLabel;
class QLineEdit;
class QComboBox;
class QPushButton;
QT_END_NAMESPACE

namespace CTP {
class CTPService;
struct MarketData;
}

class FuturesQuoteModel;
class FuturesQuoteItem;

namespace WealthPilot {

/**
 * @brief 期货行情页面
 * @details 提供期货实时行情展示，支持CTP数据源、主力合约识别、活跃度筛选等功能
 */
class FuturesQuotesPage : public BasePage
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口
     */
    explicit FuturesQuotesPage(QWidget* parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~FuturesQuotesPage() override;

    /**
     * @brief 获取页面ID
     */
    QString pageId() const override;
    
    /**
     * @brief 初始化页面
     */
    void initializePage() override;
    
    /**
     * @brief 页面激活时调用
     */
    void onPageActivated(const QVariantMap& params = {}) override;
    
    /**
     * @brief 页面停用时调用
     */
    void onPageDeactivated() override;

signals:
    /**
     * @brief 导航到K线页面信号
     * @param instrumentId 合约ID
     * @param params 参数
     */
    void navigateToKLinePage(const QString& instrumentId, const QVariantMap& params);

private slots:
    /**
     * @brief 行点击槽函数
     */
    void onRowClicked(const QModelIndex& index) const;
    
    /**
     * @brief 行双击槽函数
     */
    void onRowDoubleClicked(const QModelIndex& index);
    
    /**
     * @brief 批量行情数据接收槽函数
     */
    void onCtpBatchMarketData(const QList<CTP::MarketData>& dataList);
    
    /**
     * @brief 单条行情数据接收槽函数
     */
    void onCtpSingleMarketData(const CTP::MarketData& data);
    
    /**
     * @brief 合约查询完成槽函数
     */
    void onInstrumentQueryFinished(int totalCount);
    
    /**
     * @brief 更新主力合约
     */
    void updateMainContracts();
    
    /**
     * @brief 订阅合约
     */
    void onSubscribeContract();

private:
    /**
     * @brief 初始化UI
     */
    void setupUI();
    
    /**
     * @brief 初始化信号连接
     */
    void setupConnections();
    
    /**
     * @brief 初始化CTP连接
     */
    void setupCtpConnections();
    
    /**
     * @brief 初始化示例数据
     */
    void initializeSampleData();
    
    /**
     * @brief 刷新待更新数据
     */
    void flushPendingUpdates() const;
    
    /**
     * @brief 更新连接状态
     */
    void updateConnectionStatus(const QString& text, const QString& color) const;
    
    /**
     * @brief 合约查询回调
     */
    void onInstrumentQueried(const QString& instrumentId, const QString& exchangeId,
                             const QString& instrumentName, double priceTick, int volumeMultiple);
    
    /**
     * @brief 设置筛选模式
     */
    void setFilterMode(int mode) const;
    
    /**
     * @brief 订阅合约列表
     */
    void subscribeContracts(const QList<QString>& contracts) const;
    
    /**
     * @brief 解析合约代码
     */
    static std::tuple<QString, QDate, bool> parseContractCode(const QString& contractId);
    
    /**
     * @brief 识别主力合约
     */
    void identifyMainContracts() const;
    
    /**
     * @brief 按优先级订阅合约
     */
    void subscribeContractsByPriority();
    
    /**
     * @brief 批量订阅合约
     */
    void subscribeContractsInBatches(const QList<QString>& contracts) const;
    
    /**
     * @brief 更新合约活跃度
     */
    bool updateContractActivity(const QString& contractId, const CTP::MarketData& data, qint64 currentTime);
    
    /**
     * @brief 更新主力合约
     */
    void updateMainContractByVolume(const QString& contractId, int volume) const;
    
    /**
     * @brief 判断是否应该显示合约
     */
    bool shouldDisplayContract(const QString& contractId, const CTP::MarketData& data) const;

    /**
     * @brief 私有实现类
     */
    class Impl;
    std::unique_ptr<Impl> d;
};

} // namespace WealthPilot

#endif // FUTURESQUOTESPAGE_H
