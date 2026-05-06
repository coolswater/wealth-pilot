/**
 * @file BasePage.cpp
 * @brief 页面基类实现
 */

#include "BasePage.h"
#include <QShowEvent>
#include <QHideEvent>

namespace WealthPilot {

BasePage::BasePage(QWidget* parent)
    : QWidget(parent)
{
}

BasePage::~BasePage()
{
}

void BasePage::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    
    if (!m_initialized) {
        initializePage();
        m_initialized = true;
        emit initialized();
    }
    
    onPageActivated();
    emit pageActivated();
}

void BasePage::hideEvent(QHideEvent* event)
{
    QWidget::hideEvent(event);
    
    onPageDeactivated();
    emit pageDeactivated();
}

} // namespace WealthPilot
