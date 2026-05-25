/**
 * @file DelegateHelper.h
 * @brief 委托辅助工具
 *
 * @details 提供便捷的委托应用函数
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef DELEGATEHELPER_H
#define DELEGATEHELPER_H

#include "ColorDelegates.h"
#include <QTableView>
#include <QTableWidget>

namespace WealthPilot {

/**
 * @brief 委托应用器
 *
 * 简化在表格中应用颜色委托
 */
class DelegateHelper {
public:
    /**
     * @brief 为股票行情表格应用颜色委托
     * @param tableView 表格视图
     * @param priceCol 价格列索引
     * @param changeCol 涨跌额列索引
     * @param changePercentCol 涨跌幅列索引
     */
    static void applyStockQuoteDelegates(QTableView* tableView,
                                         int priceCol,
                                         int changeCol,
                                         int changePercentCol) {
        if (!tableView) return;

        auto* changeDelegate = new ChangeColorDelegate(tableView);
        auto* priceDelegate = new PriceColorDelegate(tableView);

        if (priceCol >= 0) {
            tableView->setItemDelegateForColumn(priceCol, priceDelegate);
        }
        if (changeCol >= 0) {
            tableView->setItemDelegateForColumn(changeCol, changeDelegate);
        }
        if (changePercentCol >= 0) {
            tableView->setItemDelegateForColumn(changePercentCol, changeDelegate);
        }
    }

    /**
     * @brief 为资金流向表格应用颜色委托
     * @param tableView 表格视图
     * @param columns 需要应用委托的列索引列表
     */
    static void applyMoneyFlowDelegates(QTableView* tableView, const QVector<int>& columns) {
        if (!tableView) return;

        auto* delegate = new MoneyFlowDelegate(tableView);
        for (int col : columns) {
            if (col >= 0) {
                tableView->setItemDelegateForColumn(col, delegate);
            }
        }
    }

    /**
     * @brief 为持仓表格应用颜色委托
     * @param tableView 表格视图
     * @param profitCol 盈亏列索引
     * @param profitPercentCol 盈亏比例列索引
     */
    static void applyPositionDelegates(QTableView* tableView,
                                       int profitCol,
                                       int profitPercentCol) {
        if (!tableView) return;

        auto* profitDelegate = new ProfitLossDelegate(tableView);
        auto* positionDelegate = new PositionProfitDelegate(tableView);

        if (profitCol >= 0) {
            tableView->setItemDelegateForColumn(profitCol, profitDelegate);
        }
        if (profitPercentCol >= 0) {
            tableView->setItemDelegateForColumn(profitPercentCol, positionDelegate);
        }
    }

    /**
     * @brief 为 QTableWidget 应用颜色委托
     */
    static void applyChangeDelegate(QTableWidget* tableWidget, int column) {
        if (!tableWidget || column < 0) return;
        tableWidget->setItemDelegateForColumn(column, new ChangeColorDelegate(tableWidget));
    }

    /**
     * @brief 为 QTableWidget 应用价格颜色委托
     */
    static void applyPriceDelegate(QTableWidget* tableWidget, int column) {
        if (!tableWidget || column < 0) return;
        tableWidget->setItemDelegateForColumn(column, new PriceColorDelegate(tableWidget));
    }

    /**
     * @brief 为 QTableWidget 应用盈亏颜色委托
     */
    static void applyProfitDelegate(QTableWidget* tableWidget, int column) {
        if (!tableWidget || column < 0) return;
        tableWidget->setItemDelegateForColumn(column, new ProfitLossDelegate(tableWidget));
    }
};

} // namespace WealthPilot

#endif // DELEGATEHELPER_H
