/**
 * @file StockScreener.cpp
 * @brief 股票筛选器实现
 */

#include "StockScreener.h"
#include "../utils/Logger.h"
#include <QSettings>
#include <QtConcurrent>
#include <algorithm>

StockScreener* StockScreener::instance()
{
    static StockScreener* inst = new StockScreener();
    return inst;
}

StockScreener::StockScreener(QObject* parent)
    : QObject(parent)
{
    LOG_INFO("StockScreener initialized");
}

void StockScreener::addCondition(const ScreenerCondition& condition)
{
    m_conditions.append(condition);
    LOG_DEBUG(QString("Condition added: %1 %2 %3")
        .arg(condition.field).arg(condition.op).arg(condition.value.toString()));
}

void StockScreener::addCondition(const QString& field, const QString& op,
                                 const QVariant& value, double weight)
{
    ScreenerCondition condition;
    condition.field = field;
    condition.op = op;
    condition.value = value;
    condition.weight = weight;
    addCondition(condition);
}

void StockScreener::clearConditions()
{
    m_conditions.clear();
    m_results.clear();
    LOG_DEBUG("All conditions cleared");
}

void StockScreener::setScope(const QStringList& symbols)
{
    m_scope = symbols;
    LOG_DEBUG(QString("Scope set: %1 symbols").arg(symbols.size()));
}

void StockScreener::execute()
{
    m_results.clear();

    int total = m_scope.size();
    int current = 0;

    for (const QString& symbol : m_scope) {
        current++;
        emit progressChanged(current, total);

        QVector<ScreenerCondition> matchedConditions;

        // 评估每个条件
        for (const ScreenerCondition& condition : m_conditions) {
            if (!condition.enabled) continue;

            if (evaluateCondition(symbol, condition)) {
                matchedConditions.append(condition);
            }
        }

        // 至少匹配一个条件
        if (!matchedConditions.isEmpty()) {
            ScreenerResult result;
            result.symbol = symbol;
            result.matchCount = matchedConditions.size();
            result.score = calculateScore(symbol, matchedConditions);
            result.details = getStockData(symbol);

            // 获取股票名称
            if (result.details.contains("name")) {
                result.name = result.details["name"].toString();
            }

            m_results.append(result);
        }
    }

    // 按得分排序
    std::sort(m_results.begin(), m_results.end(),
              [](const ScreenerResult& a, const ScreenerResult& b) {
                  return a.score > b.score;
              });

    emit screeningCompleted(m_results);
    LOG_INFO(QString("Screening completed: %1/%2 stocks matched")
        .arg(m_results.size()).arg(total));
}

void StockScreener::executeAsync()
{
    QtConcurrent::run([this]() {
        execute();
    });
}

void StockScreener::saveTemplate(const QString& name)
{
    QSettings settings("WealthPilot", "ScreenerTemplates");
    settings.beginGroup(name);

    settings.beginWriteArray("conditions", m_conditions.size());
    for (int i = 0; i < m_conditions.size(); ++i) {
        const ScreenerCondition& c = m_conditions[i];
        settings.setArrayIndex(i);
        settings.setValue("field", c.field);
        settings.setValue("op", c.op);
        settings.setValue("value", c.value);
        settings.setValue("weight", c.weight);
        settings.setValue("enabled", c.enabled);
    }
    settings.endArray();

    settings.endGroup();
    settings.sync();

    LOG_INFO(QString("Template saved: %1").arg(name));
}

bool StockScreener::loadTemplate(const QString& name)
{
    QSettings settings("WealthPilot", "ScreenerTemplates");
    if (!settings.childGroups().contains(name)) {
        LOG_WARNING(QString("Template not found: %1").arg(name));
        return false;
    }

    clearConditions();

    settings.beginGroup(name);
    int count = settings.beginReadArray("conditions");

    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        ScreenerCondition c;
        c.field = settings.value("field").toString();
        c.op = settings.value("op").toString();
        c.value = settings.value("value");
        c.weight = settings.value("weight", 1.0).toDouble();
        c.enabled = settings.value("enabled", true).toBool();
        m_conditions.append(c);
    }

    settings.endArray();
    settings.endGroup();

    LOG_INFO(QString("Template loaded: %1 (%2 conditions)").arg(name).arg(count));
    return true;
}

QStringList StockScreener::getTemplates() const
{
    QSettings settings("WealthPilot", "ScreenerTemplates");
    return settings.childGroups();
}

void StockScreener::setupBreakoutStrategy()
{
    clearConditions();

    // 突破策略：价格突破均线
    addCondition("price", ">", "ma20", 1.0);
    addCondition("volume", ">", "avgVolume5", 1.5);
    addCondition("changePercent", ">", 2.0, 1.0);
    addCondition("changePercent", "<", 9.0, 0.5);  // 排除涨停

    LOG_INFO("Breakout strategy setup");
}

void StockScreener::setupOversoldStrategy()
{
    clearConditions();

    // 超跌策略：RSI 超卖
    addCondition("rsi14", "<", 30, 1.5);
    addCondition("changePercent", "<", -3.0, 1.0);
    addCondition("volume", ">", "avgVolume5", 0.8);

    LOG_INFO("Oversold strategy setup");
}

void StockScreener::setupMomentumStrategy()
{
    clearConditions();

    // 强势策略：动量强势
    addCondition("changePercent_5d", ">", 5.0, 1.0);
    addCondition("changePercent_10d", ">", 10.0, 1.0);
    addCondition("volume", ">", "avgVolume20", 1.2);

    LOG_INFO("Momentum strategy setup");
}

void StockScreener::setupValueStrategy()
{
    clearConditions();

    // 价值策略：低估值
    addCondition("pe", "<", 20, 1.0);
    addCondition("pb", "<", 2.0, 1.0);
    addCondition("roe", ">", 15.0, 1.5);

    LOG_INFO("Value strategy setup");
}

void StockScreener::setupGrowthStrategy()
{
    clearConditions();

    // 成长策略：高增长
    addCondition("revenueGrowth", ">", 20.0, 1.0);
    addCondition("profitGrowth", ">", 20.0, 1.5);
    addCondition("roe", ">", 12.0, 0.8);

    LOG_INFO("Growth strategy setup");
}

bool StockScreener::evaluateCondition(const QString& symbol, const ScreenerCondition& condition)
{
    // TODO: 从数据源获取实际数据
    QVariantMap data = getStockData(symbol);

    if (!data.contains(condition.field)) {
        return false;
    }

    QVariant fieldValue = data[condition.field];
    double fieldVal = fieldValue.toDouble();
    double condVal = condition.value.toDouble();

    if (condition.op == ">") {
        return fieldVal > condVal;
    } else if (condition.op == "<") {
        return fieldVal < condVal;
    } else if (condition.op == "=" || condition.op == "==") {
        return qAbs(fieldVal - condVal) < 0.0001;
    } else if (condition.op == ">=") {
        return fieldVal >= condVal;
    } else if (condition.op == "<=") {
        return fieldVal <= condVal;
    } else if (condition.op == "between") {
        double val2 = condition.value2.toDouble();
        return fieldVal >= condVal && fieldVal <= val2;
    } else if (condition.op == "contains") {
        return fieldValue.toString().contains(condition.value.toString());
    }

    return false;
}

double StockScreener::calculateScore(const QString& symbol,
                                     const QVector<ScreenerCondition>& matchedConditions)
{
    double totalScore = 0.0;
    double totalWeight = 0.0;

    for (const ScreenerCondition& condition : matchedConditions) {
        totalScore += condition.weight;
        totalWeight += condition.weight;
    }

    return totalWeight > 0 ? totalScore / totalWeight * matchedConditions.size() : 0.0;
}

QVariantMap StockScreener::getStockData(const QString& symbol)
{
    // TODO: 从数据源获取实际数据
    // 这里返回模拟数据
    QVariantMap data;
    data["symbol"] = symbol;
    data["name"] = symbol;
    data["price"] = 100.0;
    data["changePercent"] = 0.0;
    data["volume"] = 1000000;
    data["ma5"] = 99.0;
    data["ma10"] = 98.0;
    data["ma20"] = 97.0;
    data["rsi14"] = 50.0;
    data["pe"] = 15.0;
    data["pb"] = 1.5;
    data["roe"] = 18.0;

    return data;
}
