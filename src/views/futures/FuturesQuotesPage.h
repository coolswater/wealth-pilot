/**
 * @file FuturesQuotesPage.h
 * @brief 期货行情页面
 *
 * 功能：
 * - 显示期货合约行情列表
 * - 连接Simnow行情服务器
 * - 订阅/取消订阅合约
 * - 显示实时价格变动
 */
#pragma once

#include <QTableWidget>
#include <memory>
#include <core/BasePage.h>
#include <services/CTPService.h>

class FuturesQuotesPage : public BasePage
{
    Q_OBJECT
public:
    explicit FuturesQuotesPage(QWidget* parent = nullptr);
    ~FuturesQuotesPage();

    QString pageId() const override;
    void initializePage() override;

private slots:
    void onConnectClicked();
    void onSubscribeClicked();
    void onMarketDataReceived(const FuturesQuote& quote);
    void onConnectionStateChanged();
    void onQuoteItemClicked(int row, int column);

private:
    void setupUI();
    void createToolbar();
    void createQuoteTable();
    void updateQuoteDisplay(const FuturesQuote& quote);
    void addQuoteRow(const FuturesQuote& quote);
    void setConnectionStatus(bool connected);

    class Impl;
    std::unique_ptr<Impl> d;
};
