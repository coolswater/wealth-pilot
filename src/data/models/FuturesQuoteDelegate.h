#ifndef FUTURESQUOTEDELEGATE_H
#define FUTURESQUOTEDELEGATE_H

#include <QStyledItemDelegate>

/**
 * @brief 行情颜色委托
 * 负责绘制红涨绿跌的颜色效果，支持Qt6
 */
class FuturesQuoteDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit FuturesQuoteDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

    // 可在此实现自定义编辑器，如快速下单按钮等
};

#endif // FUTURESQUOTEDELEGATE_H
