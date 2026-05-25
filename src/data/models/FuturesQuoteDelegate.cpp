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
    // ä¿å­ç»å®¶ç¶æ?
    painter->save();

    // è·åæ°æ®
    QVariant data = index.data(Qt::UserRole);
    if (data.canConvert<FuturesQuoteItem>()) {
        FuturesQuoteItem item = data.value<FuturesQuoteItem>();

        // è®¾ç½®å­ä½
        QFont font = option.font;
        font.setPointSize(10);
        font.setBold(true);  // è¡ææ ° å­ å ç²
        painter->setFont(font);

        // æ ¹æ®åå³å®é¢è?
        QColor textColor;
        QString text = index.data(Qt::DisplayRole).toString();

        // çº¢æ¶¨ç»¿è·é è ²æ ¹æ¡  ï¼ ä¸­å ½æ  è´§ å¸ å ºæ ¯ä¾ ï¼
        switch (index.column()) {
        case 3:  // ææ°ä»·
        case 4:  // æ¶¨è·
        case 5: // æ¶¨å¹%
        case 7: // ä¹°ä»·
        case 8: // åä»·
        case 14: // å¼çä»·
        case 15: // æé«ä»·
        case 16: // æä½ä»·
        case 18: // éæ¶¨
            if (item.change > 0) {
                textColor = QColor(255, 50, 50);  // é²çº¢è?- ä¸æ¶¨
            } else if (item.change < 0) {
                textColor = QColor(50, 205, 50);  // é²ç»¿è?- ä¸è·
            } else {
                textColor = QColor(200, 200, 200);  // ç°ç½è?- å¹³ç
            }
            break;
        case 24: // èµéæµå
            if (item.capitalFlow > 0) {
                textColor = QColor(255, 50, 50);  // çº¢è² - æµå¥
            } else if (item.capitalFlow < 0) {
                textColor = QColor(50, 205, 50);  // ç»¿è² - æµåº
            } else {
                textColor = QColor(200, 200, 200);
            }
            break;
        default:
            textColor = QColor(200, 200, 200);  // é»è®¤ç°ç½è?
        }

        // ç»å¶èæ¯
        if (option.state & QStyle::State_Selected) {
            painter->fillRect(option.rect, QColor(40, 60, 100));  // æ·±èè²éä¸­èæ¯
        } else {
            // æé©¬çº¹èæ?
            if (index.row() % 2 == 0)
                painter->fillRect(option.rect, QColor(36, 41, 55));
            else
                painter->fillRect(option.rect, QColor(26, 31, 46));
        }

        // ç»å¶æå­
        painter->setPen(textColor);
        QRect textRect = option.rect.adjusted(5, 0, -5, 0);

        // æ ¹æ®å¯¹é½æ¹å¼ç»å¶
        Qt::Alignment align = index.data(Qt::TextAlignmentRole).value<Qt::Alignment>();
        painter->drawText(textRect, align | Qt::AlignVCenter, text);

        // ç»å¶ç½æ ¼çº?
        painter->setPen(QColor(50, 50, 50));
        painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight());

    } else {
        // é»è®¤ç»å¶
        QStyledItemDelegate::paint(painter, option, index);
    }

    painter->restore();
}
