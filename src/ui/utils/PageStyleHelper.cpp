/**
 * @file PageStyleHelper.cpp
 * @brief 页面样式辅助实现
 */

#include "PageStyleHelper.h"

using namespace Tokens;

QString PageStyleHelper::pageStyle()
{
    return QString(
        "QWidget {"
        "  background-color: %1;"
        "  color: %2;"
        "}"
    )
    .arg(Colors::BgBase)
    .arg(Colors::TextPrimary);
}

QString PageStyleHelper::pageTitleStyle()
{
    return QString(
        "QLabel {"
        "  color: %1;"
        "  font-size: %2px;"
        "  font-weight: %3;"
        "}"
    )
    .arg(Colors::TextPrimary)
    .arg(Font::Size::H2)
    .arg(Font::Weight::Bold);
}

QString PageStyleHelper::pageSubtitleStyle()
{
    return QString(
        "QLabel {"
        "  color: %1;"
        "  font-size: %2px;"
        "}"
    )
    .arg(Colors::TextSecondary)
    .arg(Font::Size::Body);
}

QString PageStyleHelper::cardContainerStyle()
{
    return QString(
        "QWidget {"
        "  background-color: %1;"
        "  border: 1px solid %2;"
        "  border-radius: %3px;"
        "  padding: %4px;"
        "}"
    )
    .arg(Colors::BgElevated)
    .arg(Colors::Border)
    .arg(Radius::LG)
    .arg(Spacing::MD);
}

QString PageStyleHelper::cardTitleStyle()
{
    return QString(
        "QLabel {"
        "  color: %1;"
        "  font-size: %2px;"
        "  font-weight: %3;"
        "  margin-bottom: %4px;"
        "}"
    )
    .arg(Colors::TextPrimary)
    .arg(Font::Size::H3)
    .arg(Font::Weight::SemiBold)
    .arg(Spacing::SM);
}

QString PageStyleHelper::cardContentStyle()
{
    return QString(
        "QWidget {"
        "  background-color: transparent;"
        "  color: %1;"
        "  font-size: %2px;"
        "}"
    )
    .arg(Colors::TextPrimary)
    .arg(Font::Size::Body);
}

QString PageStyleHelper::dataLabelStyle()
{
    return QString(
        "QLabel {"
        "  color: %1;"
        "  font-size: %2px;"
        "}"
    )
    .arg(Colors::TextSecondary)
    .arg(Font::Size::Small);
}

QString PageStyleHelper::dataValueLargeStyle()
{
    return QString(
        "QLabel {"
        "  color: %1;"
        "  font-size: %2px;"
        "  font-weight: %3;"
        "}"
    )
    .arg(Colors::TextPrimary)
    .arg(Font::Size::DataXLarge)
    .arg(Font::Weight::Bold);
}

QString PageStyleHelper::dataValueStyle()
{
    return QString(
        "QLabel {"
        "  color: %1;"
        "  font-size: %2px;"
        "  font-weight: %3;"
        "}"
    )
    .arg(Colors::TextPrimary)
    .arg(Font::Size::Data)
    .arg(Font::Weight::Medium);
}

QString PageStyleHelper::dataChangeStyle(double change, bool useChineseStyle)
{
    QString color;
    if (useChineseStyle) {
        color = (change > 0.0001) ? Colors::Danger :
                (change < -0.0001) ? Colors::Success :
                Colors::TextSecondary;
    } else {
        color = (change > 0.0001) ? Colors::Success :
                (change < -0.0001) ? Colors::Danger :
                Colors::TextSecondary;
    }

    return QString(
        "QLabel {"
        "  color: %1;"
        "  font-size: %2px;"
        "  font-weight: %3;"
        "}"
    )
    .arg(color)
    .arg(Font::Size::Body)
    .arg(Font::Weight::Medium);
}

QString PageStyleHelper::tableStyle()
{
    return QString(
        "QTableWidget {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: 1px solid %3;"
        "  border-radius: %4px;"
        "  gridline-color: %5;"
        "  font-size: %6px;"
        "  selection-background-color: %7;"
        "}"
        "QTableWidget::item {"
        "  padding: %8px;"
        "  border: none;"
        "}"
        "QTableWidget::item:selected {"
        "  background-color: %7;"
        "  color: %9;"
        "}"
        "QHeaderView::section {"
        "  background-color: %10;"
        "  color: %11;"
        "  border: none;"
        "  border-bottom: 1px solid %12;"
        "  padding: %13px;"
        "  font-weight: %14;"
        "  font-size: %15px;"
        "}"
        "QTableWidget QScrollBar:vertical {"
        "  background-color: %1;"
        "  width: 10px;"
        "}"
        "QTableWidget QScrollBar::handle:vertical {"
        "  background-color: %12;"
        "  border-radius: 5px;"
        "}"
    )
    .arg(Colors::BgSurface)
    .arg(Colors::TextPrimary)
    .arg(Colors::Border)
    .arg(Radius::MD)
    .arg(Colors::BorderLight)
    .arg(Font::Size::Body)
    .arg(Colors::Primary)
    .arg(Spacing::XS)
    .arg(Colors::TextPrimary)
    .arg(Colors::BgElevated)
    .arg(Colors::TextPrimary)
    .arg(Colors::Border)
    .arg(Spacing::SM)
    .arg(Font::Weight::SemiBold)
    .arg(Font::Size::Small);
}

QString PageStyleHelper::tableHeaderStyle()
{
    return QString(
        "QLabel {"
        "  color: %1;"
        "  font-size: %2px;"
        "  font-weight: %3;"
        "  padding: %4px;"
        "}"
    )
    .arg(Colors::TextSecondary)
    .arg(Font::Size::Small)
    .arg(Font::Weight::Medium)
    .arg(Spacing::XS);
}

QString PageStyleHelper::primaryButtonStyle()
{
    return QString(
        "QPushButton {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: none;"
        "  border-radius: %3px;"
        "  padding: %4px %5px;"
        "  font-size: %6px;"
        "  font-weight: %7;"
        "}"
        "QPushButton:hover {"
        "  background-color: %8;"
        "}"
        "QPushButton:pressed {"
        "  background-color: %9;"
        "}"
        "QPushButton:disabled {"
        "  background-color: %10;"
        "  color: %11;"
        "}"
    )
    .arg(Colors::Primary)
    .arg(Colors::TextPrimary)
    .arg(Radius::MD)
    .arg(Spacing::SM)
    .arg(Spacing::MD)
    .arg(Font::Size::Body)
    .arg(Font::Weight::Medium)
    .arg(Colors::PrimaryHover)
    .arg(Colors::PrimaryDark)
    .arg(Colors::BgHover)
    .arg(Colors::TextDisabled);
}

QString PageStyleHelper::secondaryButtonStyle()
{
    return QString(
        "QPushButton {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: 1px solid %3;"
        "  border-radius: %4px;"
        "  padding: %5px %6px;"
        "  font-size: %7px;"
        "}"
        "QPushButton:hover {"
        "  background-color: %8;"
        "}"
        "QPushButton:pressed {"
        "  background-color: %9;"
        "}"
    )
    .arg(Colors::BgElevated)
    .arg(Colors::TextPrimary)
    .arg(Colors::Border)
    .arg(Radius::MD)
    .arg(Spacing::SM)
    .arg(Spacing::MD)
    .arg(Font::Size::Body)
    .arg(Colors::BgHover)
    .arg(Colors::BgActive);
}

QString PageStyleHelper::dangerButtonStyle()
{
    return QString(
        "QPushButton {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: none;"
        "  border-radius: %3px;"
        "  padding: %4px %5px;"
        "  font-size: %6px;"
        "}"
        "QPushButton:hover {"
        "  background-color: %7;"
        "}"
    )
    .arg(Colors::Danger)
    .arg(Colors::TextPrimary)
    .arg(Radius::MD)
    .arg(Spacing::SM)
    .arg(Spacing::MD)
    .arg(Font::Size::Body)
    .arg(Colors::DangerLight);
}

QString PageStyleHelper::inputStyle()
{
    return QString(
        "QLineEdit, QTextEdit, QPlainTextEdit {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: 1px solid %3;"
        "  border-radius: %4px;"
        "  padding: %5px %6px;"
        "  font-size: %7px;"
        "}"
        "QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus {"
        "  border-color: %8;"
        "}"
        "QLineEdit:disabled, QTextEdit:disabled, QPlainTextEdit:disabled {"
        "  background-color: %9;"
        "  color: %10;"
        "}"
    )
    .arg(Colors::BgElevated)
    .arg(Colors::TextPrimary)
    .arg(Colors::Border)
    .arg(Radius::MD)
    .arg(Spacing::XS)
    .arg(Spacing::SM)
    .arg(Font::Size::Body)
    .arg(Colors::BorderFocus)
    .arg(Colors::BgHover)
    .arg(Colors::TextDisabled);
}

QString PageStyleHelper::comboBoxStyle()
{
    return QString(
        "QComboBox {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: 1px solid %3;"
        "  border-radius: %4px;"
        "  padding: %5px %6px;"
        "  font-size: %7px;"
        "}"
        "QComboBox:hover {"
        "  border-color: %8;"
        "}"
        "QComboBox::drop-down {"
        "  border: none;"
        "  width: 20px;"
        "}"
        "QComboBox::down-arrow {"
        "  image: none;"
        "  border-left: 4px solid transparent;"
        "  border-right: 4px solid transparent;"
        "  border-top: 6px solid %9;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background-color: %10;"
        "  color: %11;"
        "  border: 1px solid %12;"
        "  selection-background-color: %13;"
        "}"
    )
    .arg(Colors::BgElevated)
    .arg(Colors::TextPrimary)
    .arg(Colors::Border)
    .arg(Radius::MD)
    .arg(Spacing::XS)
    .arg(Spacing::SM)
    .arg(Font::Size::Body)
    .arg(Colors::BorderHover)
    .arg(Colors::TextSecondary)
    .arg(Colors::BgElevated)
    .arg(Colors::TextPrimary)
    .arg(Colors::Border)
    .arg(Colors::Primary);
}

QString PageStyleHelper::groupBoxStyle()
{
    return QString(
        "QGroupBox {"
        "  background-color: %1;"
        "  border: 1px solid %2;"
        "  border-radius: %3px;"
        "  margin-top: %4px;"
        "  padding-top: %5px;"
        "  font-size: %6px;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  subcontrol-position: top left;"
        "  padding: 0 %7px;"
        "  color: %8;"
        "  font-weight: %9;"
        "}"
    )
    .arg(Colors::BgElevated)
    .arg(Colors::Border)
    .arg(Radius::LG)
    .arg(Spacing::LG)
    .arg(Spacing::MD)
    .arg(Font::Size::Body)
    .arg(Spacing::SM)
    .arg(Colors::TextPrimary)
    .arg(Font::Weight::SemiBold);
}

QString PageStyleHelper::dividerStyle()
{
    return QString(
        "QWidget {"
        "  background-color: %1;"
        "  min-height: 1px;"
        "  max-height: 1px;"
        "}"
    )
    .arg(Colors::Border);
}

QString PageStyleHelper::successStyle()
{
    return QString(
        "QWidget {"
        "  color: %1;"
        "}"
    )
    .arg(Colors::Success);
}

QString PageStyleHelper::warningStyle()
{
    return QString(
        "QWidget {"
        "  color: %1;"
        "}"
    )
    .arg(Colors::Warning);
}

QString PageStyleHelper::errorStyle()
{
    return QString(
        "QWidget {"
        "  color: %1;"
        "}"
    )
    .arg(Colors::Danger);
}

QString PageStyleHelper::infoStyle()
{
    return QString(
        "QWidget {"
        "  color: %1;"
        "}"
    )
    .arg(Colors::Info);
}
