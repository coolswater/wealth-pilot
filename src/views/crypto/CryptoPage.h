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

#include "core/base/BasePage.h"
#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <memory>

class KLineChart;

/**
 * @brief 加密货币行情数据
 */
struct CryptoQuote {
    QString symbol;             ///< 代币符号（BTC）
    QString name;               ///< 名称（Bitcoin）
    double price = 0.0;         ///< 当前价格（USD）
    double priceCny = 0.0;      ///< 当前价格（CNY）
    double change24h = 0.0;     ///< 24小时涨跌幅
    double volume24h = 0.0;     ///< 24小时成交量
    double marketCap = 0.0;     ///< 市值
    double high24h = 0.0;       ///< 24小时最高
    double low24h = 0.0;        ///< 24小时最低
    int rank = 0;               ///< 排名
    QDateTime updateTime;       ///< 更新时间
    
    bool isValid() const { return !symbol.isEmpty() && price > 0; }
};

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
    void updateCryptoDetail(const CryptoQuote& quote);
    void loadCryptoKLine(const QString& symbol);

    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // CRYPTOPAGE_H