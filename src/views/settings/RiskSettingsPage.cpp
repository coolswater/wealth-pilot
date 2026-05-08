/**
 * @file RiskSettingsPage.cpp
 * @brief 风控设置页面实现 - 风控参数配置
 *
 * @details 功能：
 * - 止损止盈设置
 * - 仓位控制
 * - 交易限制
 * - 预警配置
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "RiskSettingsPage.h"
#include "ui/components/StyleHelper.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QSettings>
#include <QMessageBox>
#include "../../utils/Logger.h"

RiskSettingsPage::RiskSettingsPage(QWidget *parent)
    : BasePage(parent)
{
}

RiskSettingsPage::~RiskSettingsPage()
{
}

void RiskSettingsPage::initializePage()
{
    initUI();
    initConnections();
    loadSettings();
}

void RiskSettingsPage::onPageActivated(const QVariantMap &params)
{
    Q_UNUSED(params);
    loadSettings();
}

void RiskSettingsPage::initUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(24, 16, 24, 16);
    
    // Header
    QHBoxLayout *headerLayout = new QHBoxLayout();
    QLabel *titleLabel = new QLabel("风控设置", this);
    StyleHelper::setTitleLabel(titleLabel);
    
    m_saveBtn = new QPushButton("保存", this);
    m_resetBtn = new QPushButton("重置", this);
    StyleHelper::setPrimaryButton(m_saveBtn);
    StyleHelper::setSecondaryButton(m_resetBtn);
    
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(m_resetBtn);
    headerLayout->addWidget(m_saveBtn);
    mainLayout->addLayout(headerLayout);
    
    // Position Risk Group
    QGroupBox *positionGroup = new QGroupBox("持仓风险控制", this);
    QFormLayout *positionLayout = new QFormLayout(positionGroup);
    positionLayout->setSpacing(12);
    
    m_maxPositionValueSpin = new QDoubleSpinBox(this);
    m_maxPositionValueSpin->setRange(0, 100000000);
    m_maxPositionValueSpin->setSuffix(" 元");
    m_maxPositionValueSpin->setDecimals(0);
    positionLayout->addRow("最大持仓金额:", m_maxPositionValueSpin);
    
    m_maxPositionCountSpin = new QSpinBox(this);
    m_maxPositionCountSpin->setRange(0, 100);
    m_maxPositionCountSpin->setSuffix(" 手");
    positionLayout->addRow("最大持仓数量:", m_maxPositionCountSpin);
    
    mainLayout->addWidget(positionGroup);
    
    // Loss Risk Group
    QGroupBox *lossGroup = new QGroupBox("亏损风险控制", this);
    QFormLayout *lossLayout = new QFormLayout(lossGroup);
    lossLayout->setSpacing(12);
    
    m_maxDailyLossSpin = new QDoubleSpinBox(this);
    m_maxDailyLossSpin->setRange(0, 1000000);
    m_maxDailyLossSpin->setSuffix(" 元");
    m_maxDailyLossSpin->setDecimals(0);
    lossLayout->addRow("日最大亏损:", m_maxDailyLossSpin);
    
    m_maxSingleLossSpin = new QDoubleSpinBox(this);
    m_maxSingleLossSpin->setRange(0, 100000);
    m_maxSingleLossSpin->setSuffix(" 元");
    m_maxSingleLossSpin->setDecimals(0);
    lossLayout->addRow("单笔最大亏损:", m_maxSingleLossSpin);
    
    m_maxDrawdownSpin = new QDoubleSpinBox(this);
    m_maxDrawdownSpin->setRange(0, 100);
    m_maxDrawdownSpin->setSuffix(" %");
    m_maxDrawdownSpin->setDecimals(1);
    lossLayout->addRow("最大回撤:", m_maxDrawdownSpin);
    
    mainLayout->addWidget(lossGroup);
    
    // Leverage Risk Group
    QGroupBox *leverageGroup = new QGroupBox("杠杆风险控制", this);
    QFormLayout *leverageLayout = new QFormLayout(leverageGroup);
    leverageLayout->setSpacing(12);
    
    m_maxLeverageSpin = new QDoubleSpinBox(this);
    m_maxLeverageSpin->setRange(1, 100);
    m_maxLeverageSpin->setSuffix(" 倍");
    m_maxLeverageSpin->setDecimals(1);
    leverageLayout->addRow("最大杠杆:", m_maxLeverageSpin);
    
    m_maxMarginRatioSpin = new QDoubleSpinBox(this);
    m_maxMarginRatioSpin->setRange(0, 100);
    m_maxMarginRatioSpin->setSuffix(" %");
    m_maxMarginRatioSpin->setDecimals(1);
    leverageLayout->addRow("最大保证金比例:", m_maxMarginRatioSpin);
    
    mainLayout->addWidget(leverageGroup);
    
    // Trading Restriction Group
    QGroupBox *restrictGroup = new QGroupBox("交易限制", this);
    QFormLayout *restrictLayout = new QFormLayout(restrictGroup);
    restrictLayout->setSpacing(12);
    
    m_enableNightTradeCheck = new QCheckBox("允许夜盘交易", this);
    restrictLayout->addRow(m_enableNightTradeCheck);
    
    m_enableReverseTradeCheck = new QCheckBox("允许反向交易", this);
    restrictLayout->addRow(m_enableReverseTradeCheck);
    
    mainLayout->addWidget(restrictGroup);
    
    // Info
    QLabel *infoLabel = new QLabel("提示: 风控规则将在下单时自动检查，违规订单将被拒绝。", this);
    StyleHelper::setLabelText(infoLabel);
    mainLayout->addWidget(infoLabel);
    
    mainLayout->addStretch();
}

void RiskSettingsPage::initConnections()
{
    connect(m_saveBtn, &QPushButton::clicked, this, &RiskSettingsPage::onSaveClicked);
    connect(m_resetBtn, &QPushButton::clicked, this, &RiskSettingsPage::onResetClicked);
}

void RiskSettingsPage::loadSettings()
{
    QSettings settings("WealthPilot", "RiskSettings");
    
    if (m_maxPositionValueSpin) m_maxPositionValueSpin->setValue(settings.value("maxPositionValue", 1000000).toDouble());
    if (m_maxPositionCountSpin) m_maxPositionCountSpin->setValue(settings.value("maxPositionCount", 10).toInt());
    if (m_maxDailyLossSpin) m_maxDailyLossSpin->setValue(settings.value("maxDailyLoss", 50000).toDouble());
    if (m_maxSingleLossSpin) m_maxSingleLossSpin->setValue(settings.value("maxSingleLoss", 5000).toDouble());
    if (m_maxLeverageSpin) m_maxLeverageSpin->setValue(settings.value("maxLeverage", 10).toDouble());
    if (m_maxMarginRatioSpin) m_maxMarginRatioSpin->setValue(settings.value("maxMarginRatio", 80).toDouble());
    if (m_maxDrawdownSpin) m_maxDrawdownSpin->setValue(settings.value("maxDrawdown", 20).toDouble());
    if (m_enableNightTradeCheck) m_enableNightTradeCheck->setChecked(settings.value("enableNightTrade", true).toBool());
    if (m_enableReverseTradeCheck) m_enableReverseTradeCheck->setChecked(settings.value("enableReverseTrade", false).toBool());
}

void RiskSettingsPage::saveSettings()
{
    QSettings settings("WealthPilot", "RiskSettings");
    settings.setValue("maxPositionValue", m_maxPositionValueSpin->value());
    settings.setValue("maxPositionCount", m_maxPositionCountSpin->value());
    settings.setValue("maxDailyLoss", m_maxDailyLossSpin->value());
    settings.setValue("maxSingleLoss", m_maxSingleLossSpin->value());
    settings.setValue("maxLeverage", m_maxLeverageSpin->value());
    settings.setValue("maxMarginRatio", m_maxMarginRatioSpin->value());
    settings.setValue("maxDrawdown", m_maxDrawdownSpin->value());
    settings.setValue("enableNightTrade", m_enableNightTradeCheck->isChecked());
    settings.setValue("enableReverseTrade", m_enableReverseTradeCheck->isChecked());
    settings.sync();
}

void RiskSettingsPage::setRiskRules(const QVector<RiskRule> &rules) { m_rules = rules; }
QVector<RiskSettingsPage::RiskRule> RiskSettingsPage::getRiskRules() const { return m_rules; }

void RiskSettingsPage::onSaveClicked()
{
    saveSettings();
    LOG_INFO("Risk settings saved");
    QMessageBox::information(this, "保存成功", "风控设置已保存。");
}

void RiskSettingsPage::onResetClicked()
{
    m_maxPositionValueSpin->setValue(1000000);
    m_maxPositionCountSpin->setValue(10);
    m_maxDailyLossSpin->setValue(50000);
    m_maxSingleLossSpin->setValue(5000);
    m_maxLeverageSpin->setValue(10);
    m_maxMarginRatioSpin->setValue(80);
    m_maxDrawdownSpin->setValue(20);
    m_enableNightTradeCheck->setChecked(true);
    m_enableReverseTradeCheck->setChecked(false);
    LOG_INFO("Risk settings reset to defaults");
}

void RiskSettingsPage::onRuleChanged() { }
