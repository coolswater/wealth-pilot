/**
 * @file ForexPage.h
 * @brief 外汇页面 - 外汇行情展示与分析
 *
 * @details 功能：
 * - 主要货币对行情展示（USD/CNY, EUR/USD等）
 * - 汇率走势图
 * - 外汇新闻资讯
 * - 汇率换算工具
 * - 历史汇率查询
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef FOREXPAGE_H
#define FOREXPAGE_H

#include "core/base/BasePage.h"
#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QLabel>
#include <QComboBox>
#include <QDateTime>
#include <memory>

// 前向声明
class KLineChart;

/**
 * @brief 外汇行情数据结构
 */
struct ForexQuote {
    QString pair;               ///< 货币对（如 USD/CNY）
    QString baseCurrency;       ///< 基础货币
    QString quoteCurrency;      ///< 计价货币
    double rate = 0.0;          ///< 当前汇率
    double bid = 0.0;           ///< 买入价
    double ask = 0.0;           ///< 卖出价
    double change = 0.0;        ///< 涨跌额
    double changePercent = 0.0; ///< 涨跌幅
    double high24h = 0.0;       ///< 24小时最高
    double low24h = 0.0;        ///< 24小时最低
    QDateTime updateTime;       ///< 更新时间
    
    bool isValid() const { return !pair.isEmpty() && rate > 0; }
};

/**
 * @brief 外汇页面类
 */
class ForexPage : public BasePage
{
    Q_OBJECT

public:
    explicit ForexPage(QWidget *parent = nullptr);
    ~ForexPage() override;

    QString pageId() const override { return QStringLiteral("Forex"); }
    QString pageName() const override { return QStringLiteral("外汇"); }

    void initializePage() override;
    void refresh();

signals:
    void forexPairSelected(const QString& pair, double rate);

private slots:
    void onForexListClicked(int row, int column);
    void onCurrencyFromChanged(int index);
    void onCurrencyToChanged(int index);
    void onAmountChanged(double amount);
    void onRefreshData();

private:
    void setupUI();
    void initToolBar();
    void initForexList();
    void initRateChart();
    void initConverter();
    void initConnections();
    void loadForexList();
    void updateForexDetail(const ForexQuote& quote);
    void loadRateHistory(const QString& pair);
    void calculateConversion();

    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // FOREXPAGE_H