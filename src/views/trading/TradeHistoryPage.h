/**
 * @file TradeHistoryPage.h
 * @brief 交易历史页面 - 使用 DataHub 数据中心
 *
 * @details 功能：
 * - 交易记录查询
 * - 按日期、合约筛选
 * - 导出交易记录
 *
 * DataHub 集成：
 * - 通过 DataHub 订阅交易数据
 * - 自动生命周期管理
 * - 实时交易更新
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#ifndef TRADEHISTORYPAGE_H
#define TRADEHISTORYPAGE_H

#include "ui/components/DataHubPageBase.h"
#include <QTableWidget>
#include <QPushButton>
#include <QComboBox>
#include <QDateEdit>
#include <QLabel>
#include <QVector>
#include <QDateTime>

namespace WealthPilot {

/**
 * @brief 交易历史页面
 *
 * @details 继承 DataHubPageBase，自动管理数据订阅：
 * - 订阅交易记录（trade:history）
 * - 页面销毁时自动取消订阅
 */
class TradeHistoryPage : public DataHubPageBase
{
    Q_OBJECT

public:
    /**
     * @brief 交易记录结构
     */
    struct TradeRecord {
        QString tradeId;            ///< 成交ID
        QString orderId;            ///< 订单ID
        QString instrumentId;       ///< 合约代码
        QString instrumentName;     ///< 合约名称
        QDateTime tradeTime;       ///< 成交时间
        double price = 0.0;         ///< 成交价格
        int quantity = 0;           ///< 成交数量
    };

    explicit TradeHistoryPage(QWidget* parent = nullptr);
    ~TradeHistoryPage() override;

    // ========== 页面信息 ==========

    QString pageId() const override { return "tradeHistory"; }
    QString pageName() const override { return QStringLiteral("交易历史"); }

    /**
     * @brief 初始化页面
     *
     * @details 初始化流程：
     * 1. 设置 UI 组件
     * 2. 订阅 DataHub 交易数据
     * 3. 加载初始数据
     */
    void initializePage() override;

public slots:
    // ========== UI 交互槽函数 ==========

    void onRefreshClicked();
    void onFilterChanged();
    void onExportClicked();

private:
    // ========== UI 初始化 ==========

    void setupUI();
    void setupConnections();

    // ========== DataHub 数据订阅 ==========

    /**
     * @brief 设置 DataHub 数据订阅
     *
     * @details 订阅流程：
     * 1. 订阅交易记录（trade:history）
     * 2. 回调函数中更新表格
     */
    void setupDataHubSubscriptions();

    // ========== 数据更新 ==========

    void updateTable();

    // ========== UI 组件 ==========
    QTableWidget* m_table = nullptr;
    QPushButton* m_refreshBtn = nullptr;
    QPushButton* m_exportBtn = nullptr;
    QComboBox* m_filterCombo = nullptr;
    QDateEdit* m_startDate = nullptr;
    QDateEdit* m_endDate = nullptr;

    // ========== 数据存储 ==========
    QVector<TradeRecord> m_records;
};

} // namespace WealthPilot

#endif // TRADEHISTORYPAGE_H