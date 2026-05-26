/**
 * @file AIStockPicker.cpp
 * @brief AI 智能选股服务实现
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "AIStockPicker.h"
#include "AIService.h"
#include "core/services/cache/CacheManager.h"
#include "data/DataStorageService.h"
#include "shared/types/MarketTypes.h"
#include "shared/utils/Logger.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QUuid>
#include <QRandomGenerator>

namespace WealthPilot
{
    // ============================================================================
    // 数据结构实现
    // ============================================================================

    StockPickResult StockPickResult::fromJson(const QJsonObject& json)
    {
        StockPickResult result;
        result.stockCode = json["stockCode"].toString();
        result.stockName = json["stockName"].toString();
        result.score = json["score"].toDouble();
        result.rank = json["rank"].toInt();
        result.reason = json["reason"].toString();
        result.pickedAt = QDateTime::fromString(json["pickedAt"].toString(), Qt::ISODate);
        return result;
    }

    QJsonObject StockPickResult::toJson() const
    {
        QJsonObject json;
        json["stockCode"] = stockCode;
        json["stockName"] = stockName;
        json["score"] = score;
        json["rank"] = rank;
        json["reason"] = reason;
        json["pickedAt"] = pickedAt.toString(Qt::ISODate);
        return json;
    }

    StockPickingStrategy StockPickingStrategy::fromJson(const QJsonObject& json)
    {
        StockPickingStrategy strategy;
        strategy.id = json["id"].toString();
        strategy.name = json["name"].toString();
        strategy.description = json["description"].toString();
        strategy.maxResults = json["maxResults"].toInt(50);
        strategy.useAI = json["useAI"].toBool(true);
        strategy.createdAt = QDateTime::fromString(json["createdAt"].toString(), Qt::ISODate);
        strategy.updatedAt = QDateTime::fromString(json["updatedAt"].toString(), Qt::ISODate);

        QJsonArray conditionsArray = json["conditions"].toArray();
        for (const auto& c : conditionsArray)
        {
            QJsonObject condObj = c.toObject();
            ScreeningCondition cond;
            cond.factor = static_cast<StockFactor>(condObj["factor"].toInt());
            cond.minValue = condObj["minValue"].toDouble();
            cond.maxValue = condObj["maxValue"].toDouble();
            cond.weight = condObj["weight"].toDouble(1.0);
            cond.enabled = condObj["enabled"].toBool(true);
            strategy.conditions.append(cond);
        }

        return strategy;
    }

    QJsonObject StockPickingStrategy::toJson() const
    {
        QJsonObject json;
        json["id"] = id;
        json["name"] = name;
        json["description"] = description;
        json["maxResults"] = maxResults;
        json["useAI"] = useAI;
        json["createdAt"] = createdAt.toString(Qt::ISODate);
        json["updatedAt"] = updatedAt.toString(Qt::ISODate);

        QJsonArray conditionsArray;
        for (const auto& cond : conditions)
        {
            QJsonObject condObj;
            condObj["factor"] = static_cast<int>(cond.factor);
            condObj["minValue"] = cond.minValue;
            condObj["maxValue"] = cond.maxValue;
            condObj["weight"] = cond.weight;
            condObj["enabled"] = cond.enabled;
            conditionsArray.append(condObj);
        }
        json["conditions"] = conditionsArray;

        return json;
    }

    // ============================================================================
    // AIStockPicker 实现
    // ============================================================================

    AIStockPicker::AIStockPicker(QObject* parent)
        : QObject(parent)
    {
        initializePresetStrategies();

        QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        m_storagePath = appDataPath + "/stock_picker";
        QDir dir(m_storagePath);
        if (!dir.exists())
        {
            dir.mkpath(".");
        }

        LOG_DEBUG("AIStockPicker created");
    }

    AIStockPicker::~AIStockPicker()
    {
        LOG_DEBUG("AIStockPicker destroyed");
    }

    QList<StockPickResult> AIStockPicker::screenByFactors(
            const QList<ScreeningCondition>& conditions,
            int maxResults)
    {
        QList<StockPickResult> results;

        // 使用默认热门股票列表
        // 完整实现需要从 DataHub 获取完整股票池
        QStringList stockList = {
            "sh600000", "sh600036", "sh600519", "sh601318", "sh601398",
            "sz000001", "sz000002", "sz000333", "sz000651", "sz000858",
            "sz002594", "sz300750", "sh601012", "sh600900", "sz002415"
        };

        for (const auto& code : stockList)
        {
            double score = calculateFactorScore(code, conditions);
            if (score > 0)
            {
                StockPickResult result;
                result.stockCode = code;
                result.stockName = code;  // 简化：使用代码作为名称
                result.score = score;
                result.pickedAt = QDateTime::currentDateTime();
                result.reason = QStringLiteral("符合筛选条件");
                results.append(result);
            }
        }

        // 按得分排序
        std::sort(results.begin(), results.end(),
                  [](const StockPickResult& a, const StockPickResult& b)
                  {
                      return a.score > b.score;
                  });

        // 截取前 N 个
        if (results.size() > maxResults)
        {
            results = results.mid(0, maxResults);
        }

        // 设置排名
        for (int i = 0; i < results.size(); ++i)
        {
            results[i].rank = i + 1;
        }

        emit screeningCompleted(results);
        LOG_INFO("Factor screening completed, found " + QString::number(results.size()) + " stocks");

        return results;
    }

    QList<StockPickResult> AIStockPicker::quickScreen(
        const QString& strategyType,
        int maxResults)
    {
        QList<ScreeningCondition> conditions;

        // 预设策略
        if (strategyType == "low_valuation")
        {
            // 低估值策略
            conditions.append({StockFactor::PE, 0, 15, 2.0});
            conditions.append({StockFactor::PB, 0, 1.5, 1.5});
        }
        else if (strategyType == "high_growth")
        {
            // 高成长策略
            conditions.append({StockFactor::RevenueGrowth, 20, 100, 1.5});
            conditions.append({StockFactor::ProfitGrowth, 20, 100, 1.5});
        }
        else if (strategyType == "high_dividend")
        {
            // 高股息策略
            conditions.append({StockFactor::DividendYield, 4, 20, 2.0});
            conditions.append({StockFactor::PE, 0, 20, 1.0});
        }
        else if (strategyType == "quality")
        {
            // 质量策略
            conditions.append({StockFactor::ROE, 15, 50, 2.0});
            conditions.append({StockFactor::DebtRatio, 0, 60, 1.0});
        }
        else
        {
            // 默认均衡策略
            conditions.append({StockFactor::PE, 0, 30, 1.0});
            conditions.append({StockFactor::ROE, 10, 30, 1.0});
        }

        return screenByFactors(conditions, maxResults);
    }

    void AIStockPicker::screenByNaturalLanguage(
        const QString& description,
        std::function<void(const QList<StockPickResult> &)> callback)
    {
        // 构建提示词
        QString prompt = QString(QStringLiteral(
            "请根据以下描述推荐股票：\n\n"
            "%1\n\n"
            "请返回推荐的股票代码和推荐理由，格式如下：\n"
            "股票代码|股票名称|推荐理由\n"
            "每行一只股票，最多推荐10只。"
        )).arg(description);

        // 调用 AI 服务
        AIService::instance()->chat(prompt, [this, callback](Result<QString> result)
        {
            if (result.isError())
            {
                emit errorOccurred(result.errorMessage());
                callback(QList<StockPickResult>());
                return;
            }

            // 解析 AI 响应
            QList<StockPickResult> picks;
            QString response = result.value();
            QStringList lines = response.split('\n');

            int rank = 1;
            for (const auto& line : lines)
            {
                QStringList parts = line.split('|');
                if (parts.size() >= 2)
                {
                    StockPickResult pick;
                    pick.stockCode = parts[0].trimmed();
                    pick.stockName = parts.size() > 1 ? parts[1].trimmed() : pick.stockCode;
                    pick.reason = parts.size() > 2 ? parts[2].trimmed() : QStringLiteral("AI 推荐");
                    pick.score = 0.8;
                    pick.rank = rank++;
                    pick.pickedAt = QDateTime::currentDateTime();
                    picks.append(pick);
                }
            }

            emit screeningCompleted(picks);
            callback(picks);
        });
    }

    QList<StockPickResult> AIStockPicker::findSimilarStocks(
        const QString& stockCode,
        int maxResults)
    {
        QList<StockPickResult> results;

        // TODO: 实现相似度计算
        // 这里使用模拟数据
        Q_UNUSED(stockCode)

        QStringList similarStocks = {
            "sh600001", "sh600002", "sh600003", "sh600004", "sh600005"
        };

        int rank = 1;
        for (const auto& code : similarStocks)
        {
            if (results.size() >= maxResults) break;

            StockPickResult result;
            result.stockCode = code;
            result.stockName = code;
            result.score = 0.9 - rank * 0.1;
            result.rank = rank++;
            result.reason = QStringLiteral("与参考股票相似");
            result.pickedAt = QDateTime::currentDateTime();
            results.append(result);
        }

        return results;
    }

    QList<StockPickResult> AIStockPicker::screenByConcept(

        const QString& conceptName,

        int maxResults
    )
{
    QList<StockPickResult> results;

    // TODO: 从概念数据库获取成分股
    Q_UNUSED(conceptName)
    Q_UNUSED(maxResults)

    // 模拟数据
    QStringList conceptStocks = {
        "sh600000", "sh600036", "sh600519"
    };

    int rank = 1;
    for (const auto& code : conceptStocks) {
        StockPickResult result;
        result.stockCode = code;
        result.stockName = code;
        result.score = 0.85;
        result.rank = rank++;
        result.reason = QStringLiteral("属于该概念板块");
        result.pickedAt = QDateTime::currentDateTime();
        results.append(result);
    }

    return results;
}

    QStringList AIStockPicker::getHotConcepts() const
    {
        return {
            QStringLiteral("人工智能"),
            QStringLiteral("新能源"),
            QStringLiteral("半导体"),
            QStringLiteral("医药生物"),
            QStringLiteral("消费电子"),
            QStringLiteral("碳中和"),
            QStringLiteral("数字经济"),
            QStringLiteral("智能制造")
        };
    }

    QString AIStockPicker::createStrategy(
        const QString& name,
        const QList<ScreeningCondition>& conditions)
    {
        QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);

        StockPickingStrategy strategy;
        strategy.id = id;
        strategy.name = name;
        strategy.conditions = conditions;
        strategy.createdAt = QDateTime::currentDateTime();
        strategy.updatedAt = strategy.createdAt;

        m_strategies[id] = strategy;

        LOG_INFO("Created stock picking strategy: " + name);
        return id;
    }

    QList<StockPickingStrategy> AIStockPicker::listStrategies() const
    {
        return m_strategies.values();
    }

    QString AIStockPicker::getFactorName(StockFactor factor)
    {
        switch (factor)
        {
        case StockFactor::PE: return QStringLiteral("市盈率");
        case StockFactor::PB: return QStringLiteral("市净率");
        case StockFactor::ROE: return QStringLiteral("净资产收益率");
        case StockFactor::RevenueGrowth: return QStringLiteral("营收增长率");
        case StockFactor::ProfitGrowth: return QStringLiteral("利润增长率");
        case StockFactor::DebtRatio: return QStringLiteral("资产负债率");
        case StockFactor::TurnoverRate: return QStringLiteral("换手率");
        case StockFactor::MarketCap: return QStringLiteral("市值");
        case StockFactor::DividendYield: return QStringLiteral("股息率");
        case StockFactor::Momentum: return QStringLiteral("动量");
        case StockFactor::Volatility: return QStringLiteral("波动率");
        default: return QStringLiteral("未知");
        }
    }

    QStringList AIStockPicker::getPresetStrategies() const
    {
        return {
            QStringLiteral("low_valuation|低估值策略"),
            QStringLiteral("high_growth|高成长策略"),
            QStringLiteral("high_dividend|高股息策略"),
            QStringLiteral("quality|质量策略")
        };
    }

    void AIStockPicker::initializePresetStrategies()
    {
        // 低估值策略
        StockPickingStrategy lowValuation;
        lowValuation.id = "preset_low_valuation";
        lowValuation.name = QStringLiteral("低估值策略");
        lowValuation.description = QStringLiteral("筛选低市盈率、低市净率的股票");
        lowValuation.conditions = {
            {StockFactor::PE, 0, 15, 2.0},
            {StockFactor::PB, 0, 1.5, 1.5}
        };
        m_strategies[lowValuation.id] = lowValuation;

        // 高成长策略
        StockPickingStrategy highGrowth;
        highGrowth.id = "preset_high_growth";
        highGrowth.name = QStringLiteral("高成长策略");
        highGrowth.description = QStringLiteral("筛选高营收增长、高利润增长的股票");
        highGrowth.conditions = {
            {StockFactor::RevenueGrowth, 20, 100, 1.5},
            {StockFactor::ProfitGrowth, 20, 100, 1.5}
        };
        m_strategies[highGrowth.id] = highGrowth;

        // 高股息策略
        StockPickingStrategy highDividend;
        highDividend.id = "preset_high_dividend";
        highDividend.name = QStringLiteral("高股息策略");
        highDividend.description = QStringLiteral("筛选高股息率的股票");
        highDividend.conditions = {
            {StockFactor::DividendYield, 4, 20, 2.0},
            {StockFactor::PE, 0, 20, 1.0}
        };
        m_strategies[highDividend.id] = highDividend;

        // 质量策略
        StockPickingStrategy quality;
        quality.id = "preset_quality";
        quality.name = QStringLiteral("质量策略");
        quality.description = QStringLiteral("筛选高 ROE、低负债的优质股票");
        quality.conditions = {
            {StockFactor::ROE, 15, 50, 2.0},
            {StockFactor::DebtRatio, 0, 60, 1.0}
        };
        m_strategies[quality.id] = quality;
    }

    double AIStockPicker::calculateFactorScore(
        const QString& stockCode,
        const QList<ScreeningCondition>& conditions)
    {
        double totalScore = 0;
        double totalWeight = 0;

        for (const auto& cond : conditions)
        {
            if (!cond.enabled) continue;

            double value = getStockFactorValue(stockCode, cond.factor);

            // 检查是否在范围内
            if (value >= cond.minValue && value <= cond.maxValue)
            {
                // 计算得分（越接近中间值得分越高）
                double range = cond.maxValue - cond.minValue;
                double mid = (cond.minValue + cond.maxValue) / 2;
                double distance = qAbs(value - mid);
                double score = 1.0 - (distance / (range / 2));
                score = qMax(0.0, score);

                totalScore += score * cond.weight;
                totalWeight += cond.weight;
            }
            else
            {
                // 不在范围内，得分为 0
                return 0;
            }
        }

        return totalWeight > 0 ? totalScore / totalWeight : 0;
    }

    double AIStockPicker::getStockFactorValue(const QString& stockCode, StockFactor factor)
        {
            // 从缓存获取因子值
            auto* cache = CacheManager::instance();
        
            // 尝试从缓存获取基本面数据
            QString fundamentalKey = QString("fundamental:%1").arg(stockCode);
            if (cache->contains(fundamentalKey)) {
                QVariant fundamentalVariant = cache->get(fundamentalKey);
                if (fundamentalVariant.canConvert<QVariantMap>()) {
                    QVariantMap fundamental = fundamentalVariant.toMap();
                
                    switch (factor) {
                    case StockFactor::PE:
                        return fundamental["pe"].toDouble();
                    case StockFactor::PB:
                        return fundamental["pb"].toDouble();
                    case StockFactor::ROE:
                        return fundamental["roe"].toDouble();
                    case StockFactor::RevenueGrowth:
                        return fundamental["revenueGrowth"].toDouble();
                    case StockFactor::ProfitGrowth:
                        return fundamental["profitGrowth"].toDouble();
                    case StockFactor::DebtRatio:
                        return fundamental["debtRatio"].toDouble();
                    case StockFactor::DividendYield:
                        return fundamental["dividendYield"].toDouble();
                    default:
                        break;
                    }
                }
            }
        
            // 尝试从缓存获取行情数据
            QString quoteKey = QString("quote:%1").arg(stockCode);
            if (cache->contains(quoteKey)) {
                QVariant quoteVariant = cache->get(quoteKey);
                if (quoteVariant.canConvert<StockQuote>()) {
                    StockQuote quote = quoteVariant.value<StockQuote>();
                
                    switch (factor) {
                    case StockFactor::TurnoverRate:
                        return quote.turnover;  // 使用 turnover 替代 turnoverRate
                    case StockFactor::Momentum:
                        return quote.changePercent;
                    case StockFactor::MarketCap:
                        return quote.amount * 100; // 估算市值
                    default:
                        break;
                    }
                }
            }
        
            // 如果没有真实数据，返回默认值
            LOG_WARNING(QString("No factor data available for %1, factor: %2")
                .arg(stockCode).arg(getFactorName(factor)));
        
            // 返回合理的默认值（不是随机值，保证筛选稳定性）
            switch (factor) {
            case StockFactor::PE: return 15.0;
            case StockFactor::PB: return 2.0;
            case StockFactor::ROE: return 10.0;
            case StockFactor::RevenueGrowth: return 5.0;
            case StockFactor::ProfitGrowth: return 5.0;
            case StockFactor::DebtRatio: return 40.0;
            case StockFactor::TurnoverRate: return 3.0;
            case StockFactor::MarketCap: return 500.0;
            case StockFactor::DividendYield: return 2.0;
            case StockFactor::Momentum: return 0.0;
            case StockFactor::Volatility: return 20.0;
            default: return 0.0;
            }
        }
    } // namespace WealthPilot