/**
 * @file ChartConfig.cpp
 * @brief 图表配置管理实现
 */

#include "ChartConfig.h"
#include "core/config/Tokens.h"
#include <QStandardPaths>
#include <QDir>

// ============================================================================
// 构造函数
// ============================================================================

ChartConfig::ChartConfig()
    : QObject(nullptr)
{
    // 创建配置文件路径
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configPath);
    QString configFile = configPath + "/chart_config.ini";

    m_settings = std::make_unique<QSettings>(configFile, QSettings::IniFormat);

    // 初始化默认指标配置
    initDefaultIndicators();

    // 加载配置
    load();
}

// ============================================================================
// K线样式配置
// ============================================================================

void ChartConfig::setUpColor(const QColor& color)
{
    if (m_upColor != color) {
        m_upColor = color;
        emit colorConfigChanged();
    }
}

void ChartConfig::setDownColor(const QColor& color)
{
    if (m_downColor != color) {
        m_downColor = color;
        emit colorConfigChanged();
    }
}

void ChartConfig::setFlatColor(const QColor& color)
{
    if (m_flatColor != color) {
        m_flatColor = color;
        emit colorConfigChanged();
    }
}

void ChartConfig::setCandleWidth(int width)
{
    if (m_candleWidth != width) {
        m_candleWidth = qBound(2, width, 20);
        emit configChanged();
    }
}

void ChartConfig::setCandleSpacing(int spacing)
{
    if (m_candleSpacing != spacing) {
        m_candleSpacing = qBound(0, spacing, 10);
        emit configChanged();
    }
}

// ============================================================================
// 显示选项配置
// ============================================================================

void ChartConfig::setShowGrid(bool show)
{
    if (m_showGrid != show) {
        m_showGrid = show;
        emit configChanged();
    }
}

void ChartConfig::setShowCrosshair(bool show)
{
    if (m_showCrosshair != show) {
        m_showCrosshair = show;
        emit configChanged();
    }
}

void ChartConfig::setShowVolume(bool show)
{
    if (m_showVolume != show) {
        m_showVolume = show;
        emit configChanged();
    }
}

void ChartConfig::setVolumeHeightRatio(double ratio)
{
    ratio = qBound(0.1, ratio, 0.5);
    if (!qFuzzyCompare(m_volumeHeightRatio, ratio)) {
        m_volumeHeightRatio = ratio;
        emit configChanged();
    }
}

// ============================================================================
// 指标配置
// ============================================================================

int ChartConfig::indicatorPeriod(const QString& name) const
{
    return m_indicatorPeriods.value(name, 0);
}

void ChartConfig::setIndicatorPeriod(const QString& name, int period)
{
    if (m_indicatorPeriods.value(name) != period) {
        m_indicatorPeriods[name] = period;
        emit indicatorConfigChanged();
    }
}

QColor ChartConfig::indicatorColor(const QString& name) const
{
    return m_indicatorColors.value(name, QColor("#FFFFFF"));
}

void ChartConfig::setIndicatorColor(const QString& name, const QColor& color)
{
    if (m_indicatorColors.value(name) != color) {
        m_indicatorColors[name] = color;
        emit indicatorConfigChanged();
    }
}

bool ChartConfig::indicatorEnabled(const QString& name) const
{
    return m_indicatorEnabled.value(name, false);
}

void ChartConfig::setIndicatorEnabled(const QString& name, bool enabled)
{
    if (m_indicatorEnabled.value(name) != enabled) {
        m_indicatorEnabled[name] = enabled;
        emit indicatorConfigChanged();
    }
}

// ============================================================================
// 配置持久化
// ============================================================================

void ChartConfig::load()
{
    if (!m_settings) return;

    m_settings->beginGroup(QStringLiteral("KLine"));

    // 颜色 - 使用设计令牌
    m_upColor = QColor(m_settings->value(QStringLiteral("upColor"), Tokens::Colors::Danger).toString());
    m_downColor = QColor(m_settings->value(QStringLiteral("downColor"), Tokens::Colors::Success).toString());
    m_flatColor = QColor(m_settings->value(QStringLiteral("flatColor"), Tokens::Colors::TextSecondary).toString());

    // 样式
    m_candleWidth = m_settings->value(QStringLiteral("candleWidth"), 8).toInt();
    m_candleSpacing = m_settings->value(QStringLiteral("candleSpacing"), 2).toInt();

    // 显示选项
    m_showGrid = m_settings->value(QStringLiteral("showGrid"), true).toBool();
    m_showCrosshair = m_settings->value(QStringLiteral("showCrosshair"), true).toBool();
    m_showVolume = m_settings->value(QStringLiteral("showVolume"), true).toBool();
    m_volumeHeightRatio = m_settings->value(QStringLiteral("volumeHeightRatio"), 0.1).toDouble();

    m_settings->endGroup();

    // 指标配置
    m_settings->beginGroup(QStringLiteral("Indicators"));
    for (const QString& name : m_indicatorPeriods.keys()) {
        m_indicatorPeriods[name] = m_settings->value(name + QStringLiteral("_period"), m_indicatorPeriods[name]).toInt();
        m_indicatorColors[name] = QColor(m_settings->value(name + QStringLiteral("_color"), m_indicatorColors[name].name()).toString());
        m_indicatorEnabled[name] = m_settings->value(name + QStringLiteral("_enabled"), m_indicatorEnabled[name]).toBool();
    }
    m_settings->endGroup();
}

void ChartConfig::save()
{
    if (!m_settings) return;

    m_settings->beginGroup("KLine");

    // 颜色
    m_settings->setValue("upColor", m_upColor.name());
    m_settings->setValue("downColor", m_downColor.name());
    m_settings->setValue("flatColor", m_flatColor.name());

    // 样式
    m_settings->setValue("candleWidth", m_candleWidth);
    m_settings->setValue("candleSpacing", m_candleSpacing);

    // 显示选项
    m_settings->setValue("showGrid", m_showGrid);
    m_settings->setValue("showCrosshair", m_showCrosshair);
    m_settings->setValue("showVolume", m_showVolume);
    m_settings->setValue("volumeHeightRatio", m_volumeHeightRatio);

    m_settings->endGroup();

    // 指标配置
    m_settings->beginGroup("Indicators");
    for (const QString& name : m_indicatorPeriods.keys()) {
        m_settings->setValue(name + "_period", m_indicatorPeriods[name]);
        m_settings->setValue(name + "_color", m_indicatorColors[name].name());
        m_settings->setValue(name + "_enabled", m_indicatorEnabled[name]);
    }
    m_settings->endGroup();

    m_settings->sync();
}

void ChartConfig::reset()
{
    // 重置为默认值 - 使用 Tokens 中的颜色
    m_upColor = QColor(Tokens::Colors::Danger);     // 红色=涨
    m_downColor = QColor(Tokens::Colors::Success);  // 绿色=跌
    m_flatColor = QColor(Tokens::Colors::TextSecondary);
    m_candleWidth = 8;
    m_candleSpacing = 2;
    m_showGrid = true;
    m_showCrosshair = true;
    m_showVolume = true;
    m_volumeHeightRatio = 0.1;

    initDefaultIndicators();

    emit configChanged();
}

// ============================================================================
// 私有方法
// ============================================================================

void ChartConfig::initDefaultIndicators()
{
    // 均线 - 使用 Tokens 中的颜色
    m_indicatorPeriods["MA5"] = 5;
    m_indicatorPeriods["MA10"] = 10;
    m_indicatorPeriods["MA20"] = 20;
    m_indicatorPeriods["MA30"] = 30;
    m_indicatorPeriods["MA60"] = 60;

    m_indicatorColors["MA5"] = QColor(Tokens::Colors::ChartYellow);
    m_indicatorColors["MA10"] = QColor(Tokens::Colors::ChartCyan);
    m_indicatorColors["MA20"] = QColor(Tokens::Colors::DangerLight);
    m_indicatorColors["MA30"] = QColor(Tokens::Colors::ChartPurple);
    m_indicatorColors["MA60"] = QColor(Tokens::Colors::ChartBlue);

    m_indicatorEnabled["MA5"] = true;
    m_indicatorEnabled["MA10"] = true;
    m_indicatorEnabled["MA20"] = true;
    m_indicatorEnabled["MA30"] = false;
    m_indicatorEnabled["MA60"] = false;

    // MACD
    m_indicatorPeriods["MACD"] = 12;
    m_indicatorColors["MACD"] = QColor(Tokens::Colors::ChartYellow);
    m_indicatorEnabled["MACD"] = false;

    // RSI
    m_indicatorPeriods["RSI"] = 14;
    m_indicatorColors["RSI"] = QColor(Tokens::Colors::ChartPurple);
    m_indicatorEnabled["RSI"] = false;

    // KDJ
    m_indicatorPeriods["KDJ"] = 9;
    m_indicatorColors["KDJ"] = QColor(Tokens::Colors::ChartYellow);
    m_indicatorEnabled["KDJ"] = false;

    // BOLL
    m_indicatorPeriods["BOLL"] = 20;
    m_indicatorColors["BOLL"] = QColor(Tokens::Colors::ChartYellow);
    m_indicatorEnabled["BOLL"] = false;

    // VOL
    m_indicatorEnabled["VOL"] = true;
}
