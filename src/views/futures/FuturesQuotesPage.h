/**
 * @file FuturesQuotesPage.h
 * @brief 期货行情页面 - 使用 DataHub 数据中心
 *
 * @details 功能：
 * - 显示期货实时行情列表
 * - 支持 CTP 实时数据、主力合约识别、活跃度筛选
 * - 通过 DataHub 订阅行情数据（自动生命周期管理）
 * - 无独立 QTimer，由 DataHub 统一调度刷新
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#ifndef FUTURESQUOTESPAGE_H
#define FUTURESQUOTESPAGE_H

#include <QTableView>
#include <QSortFilterProxyModel>
#include <memory>
#include <atomic>
#include <ui/components/DataHubPageBase.h>

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
 *
 * @details 继承 DataHubPageBase，自动管理数据订阅生命周期：
 * - 页面初始化时订阅期货行情数据
 * - 页面销毁时自动取消订阅
 * - 通过 DataHub 接收 CTP 实时数据更新
 *
 * 支持的 Topic 模式：
 * - market:futures:{symbol} - 期货行情
 * - market:snapshot:{symbol} - 行情快照
 */
class FuturesQuotesPage : public DataHubPageBase
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
     * @note DataHub 自动取消订阅，无需手动清理
     */
    ~FuturesQuotesPage() override;

    // ========== 页面信息 ==========

    /**
     * @brief 获取页面ID
     */
    QString pageId() const override;

    /**
     * @brief 初始化页面
     *
     * @details 初始化流程：
     * 1. 设置 UI 组件
     * 2. 订阅 DataHub 期货行情数据
     * 3. 加载初始数据
     */
    void initializePage() override;

    /**
     * @brief 页面激活时调用
     * @param params 参数（可选）
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
    // ========== UI 交互槽函数 ==========

    /**
     * @brief 行点击
     */
    void onRowClicked(const QModelIndex& index) const;

    /**
     * @brief 行双击（导航到K线）
     */
    void onRowDoubleClicked(const QModelIndex& index);

    /**
     * @brief 刷新按钮点击
     */
    void onRefreshData();

    /**
     * @brief 订阅合约按钮点击
     */
    void onSubscribeContract();

    // ========== CTP 数据槽函数 ==========

    /**
     * @brief 批量行情数据接收
     * @param dataList 行情数据列表
     */
    void onCtpBatchMarketData(const QList<CTP::MarketData>& dataList);

    /**
     * @brief 单条行情数据接收
     * @param data 行情数据
     */
    void onCtpSingleMarketData(const CTP::MarketData& data);

    /**
     * @brief 合约查询完成
     * @param totalCount 合约总数
     */
    void onInstrumentQueryFinished(int totalCount);

    /**
     * @brief 更新主力合约
     */
    void updateMainContracts();

private:
    // ========== UI 初始化 ==========

    /**
     * @brief 初始化UI组件
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

    // ========== DataHub 数据订阅 ==========

    /**
     * @brief 设置 DataHub 数据订阅
     *
     * @details 订阅流程：
     * 1. 使用 subscribeSnapshot() 订阅期货快照
     * 2. 使用模式订阅监听所有 market:futures:* 更新
     * 3. 回调函数中更新模型数据
     * 4. 页面销毁时自动取消订阅
     */
    void setupDataHubSubscriptions();

    /**
     * @brief 初始化示例数据
     */
    void initializeSampleData();

    // ========== 数据更新 ==========

    /**
     * @brief 刷新待更新数据
     */
    void flushPendingUpdates() const;

    /**
     * @brief 更新连接状态显示
     * @param text 状态文本
     * @param color 颜色
     */
    void updateConnectionStatus(const QString& text, const QString& color) const;

    /**
     * @brief 合约查询回调
     */
    void onInstrumentQueried(const QString& instrumentId, const QString& exchangeId,
                             const QString& instrumentName, double priceTick, int volumeMultiple);

    // ========== 筛选和订阅 ==========

    /**
     * @brief 设置筛选模式
     * @param mode 筛选模式
     */
    void setFilterMode(int mode) const;

    /**
     * @brief 订阅合约列表
     * @param contracts 合约列表
     */
    void subscribeContracts(const QList<QString>& contracts) const;

    /**
     * @brief 解析合约代码
     * @return (品种代码, 交割日期, 是否有效)
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
     * @param contracts 合约列表
     */
    void subscribeContractsInBatches(const QList<QString>& contracts) const;

    // ========== 活跃度和主力合约 ==========

    /**
     * @brief 更新合约活跃度
     * @return 是否更新成功
     */
    bool updateContractActivity(const QString& contractId, const CTP::MarketData& data, qint64 currentTime);

    /**
     * @brief 更新主力合约（按成交量）
     */
    void updateMainContractByVolume(const QString& contractId, int volume) const;

    /**
     * @brief 判断是否应该显示合约
     */
    bool shouldDisplayContract(const QString& contractId, const CTP::MarketData& data) const;

    // ========== 私有实现类（PIMPL） ==========
    class Impl;
    std::unique_ptr<Impl> d;

    // ========== DataHub 相关 ==========

    /**
     * @brief 已订阅的合约列表
     */
    QStringList m_subscribedContracts;
};

} // namespace WealthPilot

#endif // FUTURESQUOTESPAGE_H