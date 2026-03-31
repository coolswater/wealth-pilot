#include "FuturesQuoteDelegate.h"
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QModelIndex>
#include "FuturesQuoteItem.h"
#include "core/Tokens.h"

FuturesQuoteDelegate::FuturesQuoteDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

void FuturesQuoteDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const
{
    // 保存画家状态
    painter->save();

    // 获取数据
    QVariant data = index.data(Qt::UserRole);
    if (data.canConvert<FuturesQuoteItem>()) {
        FuturesQuoteItem item = data.value<FuturesQuoteItem>();

        // 设置字体
        QFont font = option.font;
        font.setPointSize(10);  // 行情字体通常较小
        painter->setFont(font);

        // 根据列决定颜色
        QColor textColor = Qt::white;
        QString text = index.data(Qt::DisplayRole).toString();

        switch (index.column()) {
        case 2:  // 最新
        case 9:  // 涨跌
        case 10: // 涨幅%
        case 17: // 速涨
        case 18: // 现涨
            textColor = item.changeColor();
            break;
        case 24: // 资金流向
            textColor = item.capitalFlow > 0 ? Qt::red : (item.capitalFlow < 0 ? Qt::green : Qt::white);
            break;
        default:
            textColor = Tokens::Colors::TextSecondary;
        }

        // 绘制背景（选中状态）
        if (option.state & QStyle::State_Selected) {
            painter->fillRect(option.rect, QColor(40, 60, 100));  // 深蓝色选中背景
        } else {
            // 斑马纹背景
            if (index.row() % 2 == 0)
                painter->fillRect(option.rect, QColor(36, 41, 55));  // 深灰
            else
                painter->fillRect(option.rect, QColor(26, 31, 46));  // 更深灰
        }

        // 绘制文字
        painter->setPen(textColor);
        QRect textRect = option.rect.adjusted(5, 0, -5, 0);  // 内边距

        // 根据对齐方式绘制
        Qt::Alignment align = index.data(Qt::TextAlignmentRole).value<Qt::Alignment>();
        painter->drawText(textRect, align | Qt::AlignVCenter, text);

        // 绘制网格线（可选）
        painter->setPen(QColor(50, 50, 50));
        painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight());

    } else {
        // 默认绘制
        QStyledItemDelegate::paint(painter, option, index);
    }

    painter->restore();
}
