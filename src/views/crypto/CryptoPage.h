/**
 * @file CryptoPage.h
 * @brief 数字货币页面 - 使用 DataHub 数据中心
 *
 * @details 功能：
 * - 主流加密货币行情（BTC、ETH、BNB等）
 * - 实时价格更新
 * - K线图表
 * - 市值排名
 * - 涨跌幅排行
 *
 * DataHub 集成：
 * - 通过 DataHub 订阅加密货币行情数据
 * - 自动生命周期管理
 * - 实时价格更新
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#ifndef CRYPTOPAGE_H
#define CRYPTOPAGE_H

#include "ui/components/DataHubPageBase.h"
#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QDateTime>
#include <memory>

class KLineChart;

/**
 * @brief 加密货币行情数据
 */
#include "market/CryptoDataSource.h"

namespace WealthPilot {

/**
 * @brief 数字货币页面
 *
 * @details 继承 DataHubPageBase，自动管理数据订阅：
 * - 订阅加密货币行情数据（market:crypto:*）
 * - 实时价格更新
 * - 页面销毁时自动取消订阅
 */
class CryptoPage : public DataHubPageBase
{
    Q_OBJECT

public:
    explicit CryptoPage(QWidget *parent = nullptr);
    ~CryptoPage() override;

    // ========== 页面信息 ==========

    QString pageId() const override { return QStringLiteral("Crypto"); }
    QString pageName() const override { return QStringLiteral("数字货币"); }

    /**
     * @brief 初始化页面
     *
     * @details 初始化流程：
     * 1. 设置 UI 组件
     * 2. 订阅 DataHub 加密货币数据
     * 3. 加载初始数据
     */
    void initializePage() override;

    /**
     * @brief 刷新数据
     */
    void refresh();

signals:
    /**
     * @brief 加密货币选中信号
     */
    void cryptoSelected(const QString& symbol, double price);

private slots:
    // ========== UI 交互槽函数 ==========

    void onCryptoListClicked(int row, int column);
    void onRefreshData();

private:
    // ========== UI 初始化 ==========

    void setupUI();
    void initCryptoList();
    void initDetailPanel();
    void initConnections();

    // ========== DataHub 数据订阅 ==========

    /**
     * @brief 设置 DataHub 数据订阅
     *
     * @details 订阅流程：
     * 1. 使用 subscribeQuote() 订阅主流加密货币
     * 2. 使用模式订阅 market:crypto:*
     * 3. 回调函数中更新表格显示
     */
    void setupDataHubSubscriptions();

    // ========== 数据加载 ==========

    void loadCryptoList();
    void updateCryptoTable();
    void updateCryptoDetail(const CryptoQuote& quote);
    void loadCryptoKLine(const QString& symbol);

    // ========== 私有实现类（PIMPL） ==========
    struct Impl;
    std::unique_ptr<Impl> d;

    // ========== DataHub 相关 ==========

    /**
     * @brief 已订阅的加密货币列表
     */
    QStringList m_subscribedSymbols;
};

} // namespace WealthPilot

#endif // CRYPTOPAGE_H