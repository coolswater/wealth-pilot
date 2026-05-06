/**
 * @file CryptoPage.h
 * @brief 数字货币页面 - 加密货币行情展示与分析
 *
 * @details 功能：
 * - 主流加密货币行情（BTC、ETH、BNB等）
 * - 实时价格更新
 * - K线图表
 * - 市值排名
 * - 涨跌幅排行
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef CRYPTOPAGE_H
#define CRYPTOPAGE_H

#include "ui/components/BasePage.h"
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
class CryptoPage : public BasePage
{
    Q_OBJECT

public:
    explicit CryptoPage(QWidget *parent = nullptr);
    ~CryptoPage() override;

    QString pageId() const override { return QStringLiteral("Crypto"); }
    QString pageName() const override { return QStringLiteral("数字货币"); }

    void initializePage() override;
    void refresh();

signals:
    void cryptoSelected(const QString& symbol, double price);

private slots:
    void onCryptoListClicked(int row, int column);
    void onRefreshData();

private:
    void setupUI();
    void initCryptoList();
    void initDetailPanel();
    void initConnections();
    void loadCryptoList();
    void updateCryptoTable();
    void updateCryptoDetail(const CryptoQuote& quote);
    void loadCryptoKLine(const QString& symbol);

    struct Impl;
    std::unique_ptr<Impl> d;
};



 // CRYPTOPAGE_H

} // namespace WealthPilot

#endif
