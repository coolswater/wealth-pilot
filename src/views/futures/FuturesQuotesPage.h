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
#include <QTimer>

QT_BEGIN_NAMESPACE
class QTableView;
class QLabel;
QT_END_NAMESPACE

class FuturesQuotesPage : public BasePage
{
    Q_OBJECT
public:
    explicit FuturesQuotesPage(QWidget* parent = nullptr);
    ~FuturesQuotesPage();

    QString pageId() const override;
    void initializePage() override;

    void flushPendingUpdates();
private slots:
    void onRowClicked(const QModelIndex &index);
    void onSimulateTick();

private:
    void setupUI();
    void setupConnections();
    void initData();

    class Impl;
    std::unique_ptr<Impl> d;
};
