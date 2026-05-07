/**
 * @file StyleHelper.cpp
 * @brief 样式辅助工具实现
 */

#include "StyleHelper.h"
#include <QWidget>

using namespace Tokens;

QString StyleHelper::buttonStyle(
    const QString& bgColor,
    const QString& hoverColor,
    const QString& textColor,
    int radius,
    int height)
{
    return QString(
        "QPushButton {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: none;"
        "  border-radius: %3px;"
        "  padding: 0 %4px;"
        "  min-height: %5px;"
        "  font-size: %6px;"
        "}"
        "QPushButton:hover {"
        "  background-color: %7;"
        "}"
        "QPushButton:pressed {"
        "  background-color: %8;"
        "}"
        "QPushButton:disabled {"
        "  background-color: %9;"
        "  color: %10;"
        "}"
    )
    .arg(bgColor)
    .arg(textColor)
    .arg(radius)
    .arg(Spacing::MD)
    .arg(height)
    .arg(Font::Size::Body)
    .arg(hoverColor)
    .arg(hoverColor)
    .arg(Colors::BgHover)
    .arg(Colors::TextDisabled);
}

QString StyleHelper::inputStyle(
    const QString& bgColor,
    const QString& textColor,
    const QString& borderColor,
    const QString& focusBorderColor,
    int radius,
    int height)
{
    return QString(
        "QLineEdit, QTextEdit, QPlainTextEdit, QComboBox {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: 1px solid %3;"
        "  border-radius: %4px;"
        "  padding: 0 %5px;"
        "  min-height: %6px;"
        "  font-size: %7px;"
        "}"
        "QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus, QComboBox:focus {"
        "  border-color: %8;"
        "}"
        "QLineEdit:disabled, QTextEdit:disabled, QPlainTextEdit:disabled, QComboBox:disabled {"
        "  background-color: %9;"
        "  color: %10;"
        "}"
    )
    .arg(bgColor)
    .arg(textColor)
    .arg(borderColor)
    .arg(radius)
    .arg(Spacing::SM)
    .arg(height)
    .arg(Font::Size::Body)
    .arg(focusBorderColor)
    .arg(Colors::BgHover)
    .arg(Colors::TextDisabled);
}

QString StyleHelper::cardStyle(
    const QString& bgColor,
    const QString& borderColor,
    int radius,
    int padding)
{
    return QString(
        "QWidget {"
        "  background-color: %1;"
        "  border: 1px solid %2;"
        "  border-radius: %3px;"
        "  padding: %4px;"
        "}"
    )
    .arg(bgColor)
    .arg(borderColor)
    .arg(radius)
    .arg(padding);
}

QString StyleHelper::tableStyle(
    const QString& bgColor,
    const QString& textColor,
    const QString& borderColor,
    const QString& selectionColor)
{
    return QString(
        "QTableWidget {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: 1px solid %3;"
        "  border-radius: %4px;"
        "  gridline-color: %5;"
        "  font-size: %6px;"
        "}"
        "QTableWidget::item {"
        "  padding: %7px;"
        "}"
        "QTableWidget::item:selected {"
        "  background-color: %8;"
        "  color: %9;"
        "}"
        "QHeaderView::section {"
        "  background-color: %10;"
        "  color: %11;"
        "  border: none;"
        "  border-bottom: 1px solid %12;"
        "  padding: %13px;"
        "  font-weight: bold;"
        "}"
    )
    .arg(bgColor)
    .arg(textColor)
    .arg(borderColor)
    .arg(Radius::MD)
    .arg(borderColor)
    .arg(Font::Size::Body)
    .arg(Spacing::XS)
    .arg(selectionColor)
    .arg(Colors::TextPrimary)
    .arg(Colors::BgElevated)
    .arg(textColor)
    .arg(borderColor)
    .arg(Spacing::SM);
}

QString StyleHelper::labelStyle(
    const QString& textColor,
    int fontSize)
{
    return QString(
        "QLabel {"
        "  color: %1;"
        "  font-size: %2px;"
        "}"
    )
    .arg(textColor)
    .arg(fontSize);
}

QString StyleHelper::dividerStyle(
    const QString& color,
    int thickness)
{
    return QString(
        "QWidget {"
        "  background-color: %1;"
        "  min-height: %2px;"
        "  max-height: %2px;"
        "}"
    )
    .arg(color)
    .arg(thickness);
}

QString StyleHelper::trendStyle(double change, bool useChineseStyle)
{
    QString color = useChineseStyle ? getTrendColor(change) : getTrendColorInternational(change);
    return QString("color: %1;").arg(color);
}

QString StyleHelper::badgeStyle(
    const QString& bgColor,
    const QString& textColor,
    int radius)
{
    return QString(
        "QLabel {"
        "  background-color: %1;"
        "  color: %2;"
        "  border-radius: %3px;"
        "  padding: %4px %5px;"
        "  font-size: %6px;"
        "}"
    )
    .arg(bgColor)
    .arg(textColor)
    .arg(radius)
    .arg(Spacing::XS)
    .arg(Spacing::SM)
    .arg(Font::Size::Small);
}

QString StyleHelper::progressBarStyle(
    const QString& bgColor,
    const QString& chunkColor,
    int radius)
{
    return QString(
        "QProgressBar {"
        "  background-color: %1;"
        "  border: none;"
        "  border-radius: %2px;"
        "  text-align: center;"
        "  color: %3;"
        "}"
        "QProgressBar::chunk {"
        "  background-color: %4;"
        "  border-radius: %2px;"
        "}"
    )
    .arg(bgColor)
    .arg(radius)
    .arg(Colors::TextPrimary)
    .arg(chunkColor);
}

QString StyleHelper::scrollBarStyle(
    const QString& bgColor,
    const QString& handleColor,
    int width)
{
    return QString(
        "QScrollBar:vertical {"
        "  background-color: %1;"
        "  width: %2px;"
        "  border-radius: %3px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background-color: %4;"
        "  border-radius: %3px;"
        "  min-height: 20px;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "  height: 0px;"
        "}"
        "QScrollBar:horizontal {"
        "  background-color: %1;"
        "  height: %2px;"
        "  border-radius: %3px;"
        "}"
        "QScrollBar::handle:horizontal {"
        "  background-color: %4;"
        "  border-radius: %3px;"
        "  min-width: 20px;"
        "}"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {"
        "  width: 0px;"
        "}"
    )
    .arg(bgColor)
    .arg(width)
    .arg(width / 2)
    .arg(handleColor);
}

QString StyleHelper::toolTipStyle(
    const QString& bgColor,
    const QString& textColor,
    const QString& borderColor,
    int radius)
{
    return QString(
        "QToolTip {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: 1px solid %3;"
        "  border-radius: %4px;"
        "  padding: %5px;"
        "  font-size: %6px;"
        "}"
    )
    .arg(bgColor)
    .arg(textColor)
    .arg(borderColor)
    .arg(radius)
    .arg(Spacing::XS)
    .arg(Font::Size::Small);
}

QString StyleHelper::getTrendColor(double change)
{
    if (change > 0.0001) return Colors::Danger;   // 红涨
    if (change < -0.0001) return Colors::Success; // 绿跌
    return Colors::TextSecondary;
}

QString StyleHelper::getTrendColorInternational(double change)
{
    if (change > 0.0001) return Colors::Success;  // 绿涨
    if (change < -0.0001) return Colors::Danger;  // 红跌
    return Colors::TextSecondary;
}

QString StyleHelper::getRiskLevelColor(int level)
{
    switch (level) {
    case 0: return Colors::Success;       // 低风险
    case 1: return Colors::Warning;       // 中风险
    case 2: return Colors::WarningLight;  // 高风险
    case 3: return Colors::Danger;        // 极高风险
    default: return Colors::TextSecondary;
    }
}

QString StyleHelper::getSentimentColor(int sentiment)
{
    switch (sentiment) {
    case 1: return Colors::Success;  // 正面
    case -1: return Colors::Danger;  // 负面
    case 0: return Colors::TextSecondary; // 中性
    default: return Colors::TextSecondary;
    }
}

void StyleHelper::applyStyle(QWidget* widget, const QString& style)
{
    if (widget) {
        widget->setStyleSheet(style);
    }
}

void StyleHelper::applyCardStyle(QWidget* widget)
{
    if (widget) {
        widget->setStyleSheet(cardStyle());
    }
}

void StyleHelper::applyInputStyle(QWidget* widget)
{
    if (widget) {
        widget->setStyleSheet(inputStyle());
    }
}

void StyleHelper::applyButtonStyle(QWidget* widget, bool isPrimary)
{
    if (widget) {
        if (isPrimary) {
            widget->setStyleSheet(buttonStyle());
        } else {
            widget->setStyleSheet(buttonStyle(
                Colors::BgElevated,
                Colors::BgHover,
                Colors::TextPrimary
            ));
        }
    }
}

QString StyleHelper::fontSize(int size)
{
    return QString("font-size: %1px;").arg(size);
}

QString StyleHelper::fontWeight(int weight)
{
    return QString("font-weight: %1;").arg(weight);
}

QString StyleHelper::fontColor(const QString& color)
{
    return QString("color: %1;").arg(color);
}

QString StyleHelper::padding(int value)
{
    return QString("padding: %1px;").arg(value);
}

QString StyleHelper::margin(int value)
{
    return QString("margin: %1px;").arg(value);
}

QString StyleHelper::borderRadius(int radius)
{
    return QString("border-radius: %1px;").arg(radius);
}
