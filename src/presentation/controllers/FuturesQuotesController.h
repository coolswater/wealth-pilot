/**
 * @file FuturesQuotesController.h
 * @brief 期货行情控制器 - MVP 架构
 * 
 * @details 提供期货行情业务逻辑：
 * - CTP 数据获取和管理
 * - 主力合约识别
 * - 活跃度筛选
 * - 合约订阅管理
 * 
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef FUTURESQUOTESCONTROLLER_H
#define FUTURESQUOTESCONTROLLER_H

#include "ControllerBase.h"
#include "infrastructure/ctp/service/CTPService.h"
#include <QSortFilterProxyModel>
#include <QTimer>
#include <QSet>

// 前向声明
class FuturesQuoteModel;

namespace WealthPilot
{
    /**
 * @brief 期货行情控制器
 *
 * @details 职责：
 * - 管理 CTP 数据连接
 * - 处理行情数据更新
 * - 识别主力合约
 * - 管理合约订阅
 * - 提供筛选和排序功能
 */
    class FuturesQuotesController : public ControllerBase
    {
        Q_OBJECT

    public:
        /**
     * @brief 筛选模式
     */
        enum class FilterMode
        {
            All, ///< 全部合约
            MainContracts, ///< 主力合约
            Active, ///< 活跃合约
            Subscribed ///< 已订阅
        };

        /**
     * @brief 排序字段
     */
        enum class SortField
        {
            Code, ///< 合约代码
            Price, ///< 最新价
            Change, ///< 涨跌幅
            Volume, ///< 成交量
            OpenInterest, ///< 持仓量
            Activity ///< 活跃度
        };

        explicit FuturesQuotesController(QObject* parent = nullptr);
        ~FuturesQuotesController() override;

        // ========== 初始化 ==========

        void initialize() override;
        void cleanup() override;

        // ========== 模型访问 ==========

        /**
     * @brief 获取代理模型
     */
        QSortFilterProxyModel* proxyModel() const { return m_proxyModel; }

        /**
     * @brief 获取源模型
     */
        FuturesQuoteModel* sourceModel() const { return m_model; }

        // ========== 数据操作 ==========

        /**
     * @brief 刷新数据
     */
    Q_INVOKABLE void refreshData();

        /**
     * @brief 订阅合约
     */
    Q_INVOKABLE void subscribeContract(const QString& instrumentId);

        /**
     * @brief 取消订阅
     */
    Q_INVOKABLE void unsubscribeContract(const QString& instrumentId);

        /**
     * @brief 批量订阅
     */
    Q_INVOKABLE void subscribeContracts(const QStringList& instrumentIds);

        /**
     * @brief 设置筛选模式
     */
    Q_INVOKABLE void setFilterMode(int mode);

        /**
     * @brief 设置交易所筛选
     */
    Q_INVOKABLE void setExchangeFilter(const QString& exchange);

        /**
     * @brief 设置品种筛选
     */
    Q_INVOKABLE void setProductFilter(const QString& product);

        /**
     * @brief 搜索合约
     */
    Q_INVOKABLE void searchContracts(const QString& keyword);

        /**
     * @brief 排序
     */
    Q_INVOKABLE void sortByField(int field, Qt::SortOrder order = Qt::DescendingOrder);

        // ========== 主力合约 ==========

        /**
     * @brief 更新主力合约
     */
    Q_INVOKABLE void updateMainContracts();

        /**
     * @brief 获取主力合约列表
     */
    Q_INVOKABLE QStringList getMainContracts() const;

        /**
     * @brief 判断是否主力合约
     */
    Q_INVOKABLE bool isMainContract(const QString& instrumentId) const;

        // ========== 连接状态 ==========

        /**
     * @brief 获取连接状态
     */
    Q_INVOKABLE QString connectionStatus() const { return m_connectionStatus; }

        /**
     * @brief 是否已连接
     */
    Q_INVOKABLE bool isConnected() const { return m_isConnected; }

        // ========== 统计信息 ==========

        /**
     * @brief 获取合约总数
     */
    Q_INVOKABLE int totalCount() const;

        /**
     * @brief 获取筛选后数量
     */
    Q_INVOKABLE int filteredCount() const;

        /**
     * @brief 获取已订阅数量
     */
    Q_INVOKABLE int subscribedCount() const;

        /**
     * @brief 获取市场统计
     */
    Q_INVOKABLE QVariantMap getMarketStatistics() const;

        signals :
        /**
     * @brief 数据刷新完成
     */

        void dataRefreshed(int count);

        /**
     * @brief 合约数据更新
     */
        void contractUpdated(const QString& instrumentId);

        /**
     * @brief 主力合约更新
     */
        void mainContractsUpdated(const QStringList& mainContracts);

        /**
     * @brief 连接状态变化
     */
        void connectionStatusChanged(const QString& status, bool connected);

        /**
     * @brief 订阅状态变化
     */
        void subscriptionChanged(const QString& instrumentId, bool subscribed);

        /**
     * @brief 筛选完成
     */
        void filterCompleted(int visibleCount, int totalCount);

    private
        slots :
        // CTP 回调

        void onCtpConnected();
        void onCtpDisconnected();
        void onCtpError(const QString& error);
        void onMarketDataReceived(const CTP::MarketData& data);
        void onBatchMarketDataReceived(const QList<CTP::MarketData>& dataList);
        void onInstrumentQueried(const QString& instrumentId, const QString& exchangeId,
                                 const QString& instrumentName, double priceTick, int volumeMultiple);

        // 定时更新
        void onUpdateTimer();

    private:
        void setupConnections();
        void applyFilter();
        void identifyMainContracts();
        void updateActivity(const QString& instrumentId, qint64 volume, qint64 turnover);

        // ========== 成员变量 ==========

        FuturesQuoteModel* m_model = nullptr;
        QSortFilterProxyModel* m_proxyModel = nullptr;
        CTP::CTPService* m_ctpService = nullptr;

        // 筛选状态
        FilterMode m_filterMode = FilterMode::All;
        QString m_exchangeFilter;
        QString m_productFilter;
        QString m_searchKeyword;

        // 订阅管理
        QSet<QString> m_subscribedContracts;
        QSet<QString> m_mainContracts;

        // 连接状态
        QString m_connectionStatus;
        bool m_isConnected = false;

        // 定时器
        QTimer* m_updateTimer = nullptr;

        // 缓存
        QHash<QString, qint64> m_lastVolumeCache;
        QHash<QString, int> m_activityScores;
    };
} // namespace WealthPilot

#endif // FUTURESQUOTESCONTROLLER_H