#include "FuturesQuoteDelegate.h"
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QModelIndex>
#include "FuturesQuoteItem.h"
#include "core/config/Tokens.h"

FuturesQuoteDelegate::FuturesQuoteDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

void FuturesQuoteDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const
{
    // 保存画家状�?
    painter->save();

    // 获取数据
    QVariant data = index.data(Qt::UserRole);
    if (data.canConvert<FuturesQuoteItem>()) {
        FuturesQuoteItem item = data.value<FuturesQuoteItem>();

        // 设置字体
        QFont font = option.font;
        font.setPointSize(10);
        font.setBold(true);  // 行情数字加粗
        painter->setFont(font);

        // 根据列决定颜�?
        QColor textColor;
        QString text = index.data(Qt::DisplayRole).toString();

        // 红涨绿跌配色方案（中国期货市场惯例）
        switch (index.column()) {
        case 3:  // 最新价
        case 4:  // 涨跌
        case 5: // 涨幅%
        case 7: // 买价
        case 8: // 卖价
        case 14: // 开盘价
        case 15: // 最高价
        case 16: // 最低价
        case 18: // 速涨
            if (item.change > 0) {
                textColor = QColor(255, 50, 50);  // 鲜红�?- 上涨
            } else if (item.change < 0) {
                textColor = QColor(50, 205, 50);  // 鲜绿�?- 下跌
            } else {
                textColor = QColor(200, 200, 200);  // 灰白�?- 平盘
            }
            break;
        case 24: // 资金流向
            if (item.capitalFlow > 0) {
                textColor = QColor(255, 50, 50);  // 红色 - 流入
            } else if (item.capitalFlow < 0) {
                textColor = QColor(50, 205, 50);  // 绿色 - 流出
            } else {
                textColor = QColor(200, 200, 200);
            }
            break;
        default:
            textColor = QColor(200, 200, 200);  // 默认灰白�?
        }

        // 绘制背景
        if (option.state & QStyle::State_Selected) {
            painter->fillRect(option.rect, QColor(40, 60, 100));  // 深蓝色选中背景
        } else {
            // 斑马纹背�?
            if (index.row() % 2 == 0)
                painter->fillRect(option.rect, QColor(36, 41, 55));
            else
                painter->fillRect(option.rect, QColor(26, 31, 46));
        }

        // 绘制文字
        painter->setPen(textColor);
        QRect textRect = option.rect.adjusted(5, 0, -5, 0);

        // 根据对齐方式绘制
        Qt::Alignment align = index.data(Qt::TextAlignmentRole).value<Qt::Alignment>();
        painter->drawText(textRect, align | Qt::AlignVCenter, text);

        // 绘制网格�?
        painter->setPen(QColor(50, 50, 50));
        painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight());

    } else {
        // 默认绘制
        QStyledItemDelegate::paint(painter, option, index);
    }

    painter->restore();
}
