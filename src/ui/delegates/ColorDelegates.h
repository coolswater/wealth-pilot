/**
 * @file ColorDelegates.h
 * @brief 通用颜色委托类集合
 *
 * @details 提供可复用的颜色委托：
 * - ChangeColorDelegate: 涨跌颜色委托（红涨绿跌）
 * - MoneyFlowDelegate: 资金流向颜色委托
 * - PriceColorDelegate: 价格颜色委托
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef COLORDELEGATES_H
#define COLORDELEGATES_H

#include <QStyledItemDelegate>
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QModelIndex>

#include "core/config/Tokens.h"

namespace WealthPilot {

/**
 * @brief 涨跌颜色委托（红涨绿跌）
 *
 * 根据数值正负显示不同颜色：
 * - 正数（上涨）：红色
 * - 负数（下跌）：绿色
 * - 零（平盘）：灰色
 */
class ChangeColorDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override {
        // 绘制背景（留出网格线空间）
        painter->save();
        QRect bgRect = option.rect.adjusted(0, 0, -1, -1);
        if (option.state & QStyle::State_Selected) {
            painter->fillRect(bgRect, Tokens::Color::primary());
        } else if (option.state & QStyle::State_MouseOver) {
            painter->fillRect(bgRect, QColor(255, 255, 255, 13));
        }
        painter->restore();

        // 获取数值和文本
        double value = index.data(Qt::UserRole).toDouble();
        QString text = index.data(Qt::DisplayRole).toString();

        // 确定颜色（红涨绿跌）
        QColor textColor;
        if (value > 0.0) {
            textColor = Tokens::Color::danger();   // 红色 - 上涨
        } else if (value < 0.0) {
            textColor = Tokens::Color::success();  // 绿色 - 下跌
        } else {
            textColor = Tokens::Color::textSecondary(); // 灰色 - 平盘
        }

        // 绘制文字
        painter->save();
        painter->setPen(textColor);
        painter->setFont(option.font);

        QRect textRect = option.rect.adjusted(4, 0, -4, 0);
        int flags = Qt::AlignRight | Qt::AlignVCenter;
        painter->drawText(textRect, flags, text);
        painter->restore();
    }
};

/**
 * @brief 资金流向颜色委托
 *
 * 根据资金流向显示颜色：
 * - 流入（正数）：红色
 * - 流出（负数）：绿色
 */
class MoneyFlowDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override {
        // 绘制背景
        painter->save();
        QRect bgRect = option.rect.adjusted(0, 0, -1, -1);
        if (option.state & QStyle::State_Selected) {
            painter->fillRect(bgRect, Tokens::Color::primary());
        } else if (option.state & QStyle::State_MouseOver) {
            painter->fillRect(bgRect, QColor(255, 255, 255, 13));
        }
        painter->restore();

        // 获取数值和文本
        double value = index.data(Qt::UserRole).toDouble();
        QString text = index.data(Qt::DisplayRole).toString();

        // 确定颜色
        QColor textColor = value >= 0
            ? Tokens::Color::danger()   // 红色 - 流入
            : Tokens::Color::success();  // 绿色 - 流出

        // 绘制文字
        painter->save();
        painter->setPen(textColor);
        painter->setFont(option.font);

        QRect textRect = option.rect.adjusted(4, 0, -4, 0);
        int flags = Qt::AlignRight | Qt::AlignVCenter;
        painter->drawText(textRect, flags, text);
        painter->restore();
    }
};

/**
 * @brief 价格颜色委托
 *
 * 根据涨跌幅显示价格颜色
 */
class PriceColorDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override {
        // 绘制背景
        painter->save();
        QRect bgRect = option.rect.adjusted(0, 0, -1, -1);
        if (option.state & QStyle::State_Selected) {
            painter->fillRect(bgRect, Tokens::Color::primary());
        } else if (option.state & QStyle::State_MouseOver) {
            painter->fillRect(bgRect, QColor(255, 255, 255, 13));
        }
        painter->restore();

        // 获取涨跌幅数据
        double changePercent = index.data(Qt::UserRole).toDouble();
        QString text = index.data(Qt::DisplayRole).toString();

        // 确定颜色
        QColor textColor;
        if (changePercent > 0.0) {
            textColor = Tokens::Color::danger();    // 红色 - 上涨
        } else if (changePercent < 0.0) {
            textColor = Tokens::Color::success();   // 绿色 - 下跌
        } else {
            textColor = Tokens::Color::textSecondary(); // 灰色 - 平盘
        }

        // 绘制文字
        painter->save();
        painter->setPen(textColor);
        painter->setFont(option.font);

        QRect textRect = option.rect.adjusted(4, 0, -4, 0);
        int flags = Qt::AlignRight | Qt::AlignVCenter;
        painter->drawText(textRect, flags, text);
        painter->restore();
    }
};

/**
 * @brief 涨跌额颜色委托
 */
class ChangeAmountDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override {
        painter->save();
        QRect bgRect = option.rect.adjusted(0, 0, -1, -1);
        if (option.state & QStyle::State_Selected) {
            painter->fillRect(bgRect, Tokens::Color::primary());
        } else if (option.state & QStyle::State_MouseOver) {
            painter->fillRect(bgRect, QColor(255, 255, 255, 13));
        }
        painter->restore();

        double value = index.data(Qt::UserRole).toDouble();
        QString text = index.data(Qt::DisplayRole).toString();

        QColor textColor;
        if (value > 0.0) {
            textColor = Tokens::Color::danger();
        } else if (value < 0.0) {
            textColor = Tokens::Color::success();
        } else {
            textColor = Tokens::Color::textSecondary();
        }

        painter->save();
        painter->setPen(textColor);
        painter->setFont(option.font);
        QRect textRect = option.rect.adjusted(4, 0, -4, 0);
        painter->drawText(textRect, Qt::AlignRight | Qt::AlignVCenter, text);
        painter->restore();
    }
};

/**
 * @brief 成交量颜色委托
 *
 * 根据成交量变化显示颜色
 */
class VolumeChangeDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override {
        painter->save();
        QRect bgRect = option.rect.adjusted(0, 0, -1, -1);
        if (option.state & QStyle::State_Selected) {
            painter->fillRect(bgRect, Tokens::Color::primary());
        } else if (option.state & QStyle::State_MouseOver) {
            painter->fillRect(bgRect, QColor(255, 255, 255, 13));
        }
        painter->restore();

        // 成交量变化率
        double volumeChange = index.data(Qt::UserRole).toDouble();
        QString text = index.data(Qt::DisplayRole).toString();

        // 放量显示红色，缩量显示绿色
        QColor textColor;
        if (volumeChange > 1.5) {
            textColor = Tokens::Color::danger();    // 明显放量 - 红色
        } else if (volumeChange < 0.7) {
            textColor = Tokens::Color::success();   // 明显缩量 - 绿色
        } else {
            textColor = Tokens::Color::primary();      // 正常 - 默认颜色
        }

        painter->save();
        painter->setPen(textColor);
        painter->setFont(option.font);
        QRect textRect = option.rect.adjusted(4, 0, -4, 0);
        painter->drawText(textRect, Qt::AlignRight | Qt::AlignVCenter, text);
        painter->restore();
    }
};

/**
 * @brief 盈亏颜色委托
 *
 * 用于账户盈亏、持仓盈亏等
 */
class ProfitLossDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override {
        painter->save();
        QRect bgRect = option.rect.adjusted(0, 0, -1, -1);
        if (option.state & QStyle::State_Selected) {
            painter->fillRect(bgRect, Tokens::Color::primary());
        } else if (option.state & QStyle::State_MouseOver) {
            painter->fillRect(bgRect, QColor(255, 255, 255, 13));
        }
        painter->restore();

        double profit = index.data(Qt::UserRole).toDouble();
        QString text = index.data(Qt::DisplayRole).toString();

        QColor textColor;
        if (profit > 0.0) {
            textColor = Tokens::Color::danger();    // 盈利 - 红色
        } else if (profit < 0.0) {
            textColor = Tokens::Color::success();   // 亏损 - 绿色
        } else {
            textColor = Tokens::Color::textSecondary();
        }

        painter->save();
        painter->setPen(textColor);
        painter->setFont(option.font);
        QRect textRect = option.rect.adjusted(4, 0, -4, 0);
        painter->drawText(textRect, Qt::AlignRight | Qt::AlignVCenter, text);
        painter->restore();
    }
};

/**
 * @brief 持仓收益颜色委托
 */
class PositionProfitDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override {
        painter->save();
        QRect bgRect = option.rect.adjusted(0, 0, -1, -1);
        if (option.state & QStyle::State_Selected) {
            painter->fillRect(bgRect, Tokens::Color::primary());
        } else if (option.state & QStyle::State_MouseOver) {
            painter->fillRect(bgRect, QColor(255, 255, 255, 13));
        }
        painter->restore();

        double profitPercent = index.data(Qt::UserRole).toDouble();
        QString text = index.data(Qt::DisplayRole).toString();

        QColor textColor;
        if (profitPercent > 5.0) {
            textColor = Tokens::Color::danger();     // 大幅盈利 - 深红
        } else if (profitPercent > 0.0) {
            textColor = QColor(255, 100, 100);       // 小幅盈利 - 浅红
        } else if (profitPercent < -5.0) {
            textColor = Tokens::Color::success();    // 大幅亏损 - 深绿
        } else if (profitPercent < 0.0) {
            textColor = QColor(100, 200, 100);       // 小幅亏损 - 浅绿
        } else {
            textColor = Tokens::Color::textSecondary();
        }

        painter->save();
        painter->setPen(textColor);
        painter->setFont(option.font);
        QRect textRect = option.rect.adjusted(4, 0, -4, 0);
        painter->drawText(textRect, Qt::AlignRight | Qt::AlignVCenter, text);
        painter->restore();
    }
};

} // namespace WealthPilot

#endif // COLORDELEGATES_H
