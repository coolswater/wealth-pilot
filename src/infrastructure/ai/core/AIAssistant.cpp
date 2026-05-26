/**
 * @file AIAssistant.cpp
 * @brief AI 智能助手实现
 */

#include "AIAssistant.h"
#include "core/services/cache/CacheManager.h"
#include "data/DataStorageService.h"
#include "shared/utils/Logger.h"
#include <QRegularExpression>
#include <QUuid>
#include <QDateTime>
#include <QtConcurrent>

AIAssistant* AIAssistant::instance()
{
    static AIAssistant* inst = new AIAssistant();
    return inst;
}

AIAssistant::AIAssistant(QObject* parent)
    : QObject(parent)
{
    initializeIntentKeywords();
    LOG_INFO("AIAssistant initialized");
}

void AIAssistant::initializeIntentKeywords()
{
    // 查询股票关键词
    m_intentKeywords[UserIntent::QueryStock] = {
        "查询", "查一下", "看看", "股票", "行情", "价格", "股价", "现价"
    };

    // 查询行情关键词
    m_intentKeywords[UserIntent::QueryMarket] = {
        "大盘", "指数", "市场", "涨跌", "行情", "走势"
    };

    // 查询账户关键词
    m_intentKeywords[UserIntent::QueryAccount] = {
        "账户", "持仓", "资金", "余额", "盈亏", "收益"
    };

    // 下单关键词
    m_intentKeywords[UserIntent::PlaceOrder] = {
        "买入", "卖出", "下单", "委托", "购买", "清仓"
    };

    // 分析股票关键词
    m_intentKeywords[UserIntent::AnalyzeStock] = {
        "分析", "诊断", "评估", "技术分析", "基本面"
    };

    // 获取推荐关键词
    m_intentKeywords[UserIntent::GetRecommendation] = {
        "推荐", "建议", "选股", "买什么", "看好"
    };

    // 设置预警关键词
    m_intentKeywords[UserIntent::SetAlert] = {
        "预警", "提醒", "通知", "报警", "监控"
    };
}

AIResponse AIAssistant::processInput(const QString& input)
{
    LOG_DEBUG(QString("Processing input: %1").arg(input));

    // 识别意图
    IntentResult intent = recognizeIntent(input);

    // 根据意图处理
    AIResponse response;

    switch (intent.intent) {
    case UserIntent::QueryStock:
        response = handleQueryStock(intent);
        break;
    case UserIntent::QueryMarket:
        response = handleQueryMarket(intent);
        break;
    case UserIntent::QueryAccount:
        response = handleQueryAccount(intent);
        break;
    case UserIntent::PlaceOrder:
        response = handlePlaceOrder(intent);
        break;
    case UserIntent::AnalyzeStock:
        response = handleAnalyzeStock(intent);
        break;
    case UserIntent::GetRecommendation:
        response = handleGetRecommendation(intent);
        break;
    case UserIntent::SetAlert:
        response = handleSetAlert(intent);
        break;
    case UserIntent::GeneralQuestion:
        response = handleGeneralQuestion(intent);
        break;
    default:
        response.text = "抱歉，我没有理解您的问题。您可以尝试问我关于股票查询、行情分析、或者投资建议等问题。";
        response.suggestions = {"查询股票行情", "分析某只股票", "获取投资建议"};
        break;
    }

    // 更新对话历史
    if (m_conversations.contains(m_currentSession)) {
        m_conversations[m_currentSession].history.append(input);
        m_conversations[m_currentSession].lastIntent = intent.intent;
        m_conversations[m_currentSession].lastTime = QDateTime::currentDateTime();
    }

    return response;
}

void AIAssistant::processInputAsync(const QString& input)
{
    QThreadPool::globalInstance()->start([this, input]() {
        AIResponse response = processInput(input);
        emit responseReady(response);
    });
}

IntentResult AIAssistant::recognizeIntent(const QString& input)
{
    IntentResult result;
    result.originalQuery = input;

    // 计算各意图的匹配分数
    QHash<UserIntent, int> scores;

    for (auto it = m_intentKeywords.begin(); it != m_intentKeywords.end(); ++it) {
        int score = 0;
        for (const QString& keyword : it.value()) {
            if (input.contains(keyword)) {
                score++;
            }
        }
        scores[it.key()] = score;
    }

    // 找出最高分的意图
    int maxScore = 0;
    for (auto it = scores.begin(); it != scores.end(); ++it) {
        if (it.value() > maxScore) {
            maxScore = it.value();
            result.intent = it.key();
        }
    }

    // 提取实体
    result.entities = extractEntities(input);

    // 计算置信度
    result.confidence = maxScore > 0 ? static_cast<double>(maxScore) / 3.0 : 0.0;
    result.confidence = qMin(result.confidence, 1.0);

    LOG_DEBUG(QString("Intent recognized: %1, confidence: %2")
        .arg(static_cast<int>(result.intent)).arg(result.confidence));

    return result;
}

QHash<QString, QString> AIAssistant::extractEntities(const QString& input)
{
    QHash<QString, QString> entities;

    // 提取股票代码（6位数字）
    QRegularExpression stockCodeRegex("\\b(\\d{6})\\b");
    QRegularExpressionMatch match = stockCodeRegex.match(input);
    if (match.hasMatch()) {
        entities["symbol"] = match.captured(1);
    }

    // 提取价格
    QRegularExpression priceRegex("(\\d+\\.?\\d*)\\s*[元块]");
    match = priceRegex.match(input);
    if (match.hasMatch()) {
        entities["price"] = match.captured(1);
    }

    // 提取数量
    QRegularExpression quantityRegex("(\\d+)\\s*[股手]");
    match = quantityRegex.match(input);
    if (match.hasMatch()) {
        entities["quantity"] = match.captured(1);
    }

    // 提取方向
    if (input.contains("买入") || input.contains("购买")) {
        entities["direction"] = "buy";
    } else if (input.contains("卖出") || input.contains("清仓")) {
        entities["direction"] = "sell";
    }

    return entities;
}

QVector<Recommendation> AIAssistant::getPersonalizedRecommendations()
{
    QVector<Recommendation> recommendations;

    // 基于用户偏好和历史生成推荐
    auto* cache = CacheManager::instance();
    auto* storage = DataStorageService::instance();
    
    // 获取用户自选股列表
    QStringList watchlist = storage->getWatchlistSymbols();
    
    // 获取用户持仓
    QStringList holdings = storage->getHoldingSymbols();
    
    // 分析用户偏好行业
    QMap<QString, int> industryCount;
    for (const QString& symbol : watchlist + holdings) {
        QString industry = storage->getStockIndustry(symbol);
        if (!industry.isEmpty()) {
            industryCount[industry]++;
        }
    }
    
    // 根据偏好行业推荐相似股票
    QString topIndustry;
    int maxCount = 0;
    for (auto it = industryCount.begin(); it != industryCount.end(); ++it) {
        if (it.value() > maxCount) {
            maxCount = it.value();
            topIndustry = it.key();
        }
    }
    
    // 从热门股票中筛选推荐
    if (!topIndustry.isEmpty()) {
        QVector<Quote> topGainers = storage->getTopGainers(20);
        for (const Quote& quote : topGainers) {
            QString industry = storage->getStockIndustry(quote.symbol);
            if (industry == topIndustry && !watchlist.contains(quote.symbol)) {
                Recommendation r;
                r.symbol = quote.symbol;
                r.name = quote.name;
                r.score = 0.8 + (quote.changePercent / 100.0) * 0.2;  // 根据涨幅调整分数
                r.reason = QString("属于您关注的%1行业，近期表现强势").arg(topIndustry);
                r.category = "智能推荐";
                r.targetPrice = quote.price * 1.1;
                recommendations.append(r);
                
                if (recommendations.size() >= 5) break;
            }
        }
    }
    
    // 如果没有找到，返回默认推荐
    if (recommendations.isEmpty()) {
        Recommendation r1;
        r1.symbol = "600519";
        r1.name = "贵州茅台";
        r1.score = 0.95;
        r1.reason = "基本面优秀，估值合理，适合长期持有";
        r1.category = "价值投资";
        r1.targetPrice = 1800.0;
        recommendations.append(r1);
    }

    emit recommendationsUpdated(recommendations);
    return recommendations;
}

QVector<Recommendation> AIAssistant::getMarketHotspots()
{
    QVector<Recommendation> hotspots;

    // 从数据源获取市场热点
    auto* storage = DataStorageService::instance();
    auto* cache = CacheManager::instance();
    
    // 获取涨幅榜前10作为热点
    QVector<Quote> topGainers = storage->getTopGainers(10);
    
    for (const Quote& quote : topGainers) {
        Recommendation r;
        r.symbol = quote.symbol;
        r.name = quote.name;
        r.score = 0.9 + (quote.changePercent / 200.0);  // 涨幅越大分数越高
        r.reason = QString("涨幅%1%，成交额%2亿")
            .arg(quote.changePercent, 'f', 2)
            .arg(quote.amount / 100000000.0, 'f', 2);
        r.category = "市场热点";
        r.targetPrice = quote.price * 1.05;
        hotspots.append(r);
    }
    
    // 如果没有数据，使用默认热点
    if (hotspots.isEmpty()) {
        Recommendation r1;
        r1.symbol = "300750";
        r1.name = "宁德时代";
        r1.score = 0.92;
    r1.reason = "新能源龙头，市场关注度高";
    r1.category = "热点题材";
    hotspots.append(r1);

    return hotspots;
}

QVector<Recommendation> AIAssistant::getSimilarStocks(const QString& symbol)
{
    QVector<Recommendation> similar;

    // 基于行业、概念等找相似股票
    auto* storage = DataStorageService::instance();
    
    // 获取当前股票的行业
    QString industry = storage->getStockIndustry(symbol);
    QString concept = storage->getStockConcept(symbol);
    
    if (industry.isEmpty() && concept.isEmpty()) {
        return similar;
    }
    
    // 查找同行业股票
    QStringList sameIndustryStocks = storage->getStocksByIndustry(industry, 10);
    
    for (const QString& code : sameIndustryStocks) {
        if (code == symbol) continue;
        
        Quote quote = storage->getStockQuote(code);
        if (quote.isValid()) {
            Recommendation r;
            r.symbol = code;
            r.name = quote.name;
            r.score = 0.85;  // 同行业基准分
            r.reason = QString("与 %1 同属%2行业").arg(symbol).arg(industry);
            r.category = "相似股票";
            similar.append(r);
            
            if (similar.size() >= 5) break;
        }
    }
    
    return similar;
}

void AIAssistant::setUserPreferences(const QHash<QString, QVariant>& preferences)
{
    m_userPreferences = preferences;
    LOG_INFO("User preferences updated");
}

QString AIAssistant::startConversation()
{
    QString sessionId = QUuid::createUuid().toString(QUuid::WithoutBraces);

    ConversationContext context;
    context.sessionId = sessionId;
    m_conversations[sessionId] = context;
    m_currentSession = sessionId;

    LOG_DEBUG(QString("Conversation started: %1").arg(sessionId));
    return sessionId;
}

void AIAssistant::endConversation(const QString& sessionId)
{
    m_conversations.remove(sessionId);
    if (m_currentSession == sessionId) {
        m_currentSession.clear();
    }
    LOG_DEBUG(QString("Conversation ended: %1").arg(sessionId));
}

QVector<QString> AIAssistant::getConversationHistory(const QString& sessionId) const
{
    return m_conversations.value(sessionId).history;
}

void AIAssistant::clearConversationHistory(const QString& sessionId)
{
    if (m_conversations.contains(sessionId)) {
        m_conversations[sessionId].history.clear();
    }
}

void AIAssistant::enableVoiceInput(bool enabled)
{
    m_voiceInputEnabled = enabled;
    LOG_INFO(QString("Voice input %1").arg(enabled ? "enabled" : "disabled"));
}

void AIAssistant::speak(const QString& text)
{
    // TODO: 集成 TTS
    LOG_DEBUG(QString("Speaking: %1").arg(text));
}

AIResponse AIAssistant::handleQueryStock(const IntentResult& intent)
{
    AIResponse response;

    QString symbol = intent.entities.value("symbol");
    if (symbol.isEmpty()) {
        response.text = "请问您想查询哪只股票？请提供股票代码或名称。";
        response.suggestions = {"查询600519", "查询贵州茅台", "查询平安银行"};
        return response;
    }

    response.text = formatStockInfo(symbol);
    response.action = "show_stock";
    response.data["symbol"] = symbol;
    response.suggestions = {"分析" + symbol, "设置预警", "查看K线图"};

    return response;
}

AIResponse AIAssistant::handleQueryMarket(const IntentResult& intent)
{
    Q_UNUSED(intent)

    AIResponse response;
    response.text = formatMarketInfo();
    response.action = "show_market";
    response.suggestions = {"查看板块涨幅", "查看资金流向", "查看热门股票"};

    return response;
}

AIResponse AIAssistant::handleQueryAccount(const IntentResult& intent)
{
    Q_UNUSED(intent)

    AIResponse response;
    response.text = formatAccountInfo();
    response.action = "show_account";
    response.suggestions = {"查看持仓明细", "查看交易记录", "资金转账"};

    return response;
}

AIResponse AIAssistant::handlePlaceOrder(const IntentResult& intent)
{
    AIResponse response;

    QString symbol = intent.entities.value("symbol");
    QString direction = intent.entities.value("direction");
    QString quantity = intent.entities.value("quantity");
    QString price = intent.entities.value("price");

    if (symbol.isEmpty()) {
        response.text = "请提供股票代码。";
        response.success = false;
        return response;
    }

    if (direction.isEmpty()) {
        response.text = "请说明是买入还是卖出？";
        response.suggestions = {"买入" + symbol, "卖出" + symbol};
        response.success = false;
        return response;
    }

    response.text = QString("确认%1%2，数量%3股，价格%4元？")
        .arg(direction == "buy" ? "买入" : "卖出")
        .arg(symbol)
        .arg(quantity.isEmpty() ? "100" : quantity)
        .arg(price.isEmpty() ? "市价" : price);
    response.action = "confirm_order";
    response.data["symbol"] = symbol;
    response.data["direction"] = direction;
    response.data["quantity"] = quantity.isEmpty() ? 100 : quantity.toInt();
    response.data["price"] = price.isEmpty() ? 0 : price.toDouble();

    return response;
}

AIResponse AIAssistant::handleAnalyzeStock(const IntentResult& intent)
{
    AIResponse response;

    QString symbol = intent.entities.value("symbol");
    if (symbol.isEmpty()) {
        response.text = "请提供要分析的股票代码。";
        response.suggestions = {"分析600519", "分析贵州茅台"};
        return response;
    }

    response.text = formatStockAnalysis(symbol);
    response.action = "show_analysis";
    response.data["symbol"] = symbol;

    return response;
}

AIResponse AIAssistant::handleGetRecommendation(const IntentResult& intent)
{
    Q_UNUSED(intent)

    AIResponse response;
    auto recommendations = getPersonalizedRecommendations();

    QString text = "根据您的偏好，为您推荐以下股票：\n\n";
    for (int i = 0; i < recommendations.size(); ++i) {
        const auto& r = recommendations[i];
        text += QString("%1. %2 (%3)\n   推荐理由：%4\n   推荐分数：%5\n\n")
            .arg(i + 1).arg(r.name).arg(r.symbol).arg(r.reason).arg(r.score);
    }

    response.text = text;
    response.action = "show_recommendations";
    response.data["recommendations"] = QVariant::fromValue(recommendations);

    return response;
}

AIResponse AIAssistant::handleSetAlert(const IntentResult& intent)
{
    AIResponse response;

    QString symbol = intent.entities.value("symbol");
    QString price = intent.entities.value("price");

    if (symbol.isEmpty()) {
        response.text = "请提供要设置预警的股票代码。";
        return response;
    }

    response.text = QString("已为%1设置价格预警").arg(symbol);
    if (!price.isEmpty()) {
        response.text += QString("，目标价格：%1元").arg(price);
    }
    response.action = "set_alert";
    response.data["symbol"] = symbol;
    response.data["price"] = price;

    return response;
}

AIResponse AIAssistant::handleGeneralQuestion(const IntentResult& intent)
{
    Q_UNUSED(intent)

    AIResponse response;
    response.text = "我是您的投资助手，可以帮您：\n"
                   "• 查询股票行情和账户信息\n"
                   "• 分析股票走势和基本面\n"
                   "• 提供投资建议和选股推荐\n"
                   "• 设置价格预警和监控\n"
                   "• 执行买卖操作\n\n"
                   "请问有什么可以帮您的？";
    response.suggestions = {"查询股票", "获取推荐", "分析持仓"};

    return response;
}

QString AIAssistant::formatStockInfo(const QString& symbol)
{
    // 从数据源获取真实数据
    auto* storage = DataStorageService::instance();
    Quote quote = storage->getStockQuote(symbol);
    
    if (!quote.isValid()) {
        return QString("未找到股票 %1 的信息").arg(symbol);
    }
    
    return QString("股票 %1 (%2) 的行情信息：\n"
                   "• 现价：%3元\n"
                   "• 涨跌幅：%4%\n"
                   "• 成交量：%5手\n"
                   "• 成交额：%6亿\n"
                   "• 市值：约%7亿")
        .arg(quote.name)
        .arg(symbol)
        .arg(quote.price, 'f', 2)
        .arg(quote.changePercent, 'f', 2)
        .arg(quote.volume)
        .arg(quote.amount / 100000000.0, 'f', 2)
        .arg(quote.amount * 100 / 100000000.0, 'f', 0);
}

QString AIAssistant::formatMarketInfo()
{
    // 从数据源获取真实数据
    auto* storage = DataStorageService::instance();
    
    Quote shIndex = storage->getStockQuote("sh000001");
    Quote szIndex = storage->getStockQuote("sz399001");
    Quote cybIndex = storage->getStockQuote("sz399006");
    
    QString result = "今日市场概况：\n";
    
    if (shIndex.isValid()) {
        result += QString("• 上证指数：%1 (%2%)\n")
            .arg(shIndex.price, 'f', 2)
            .arg(shIndex.changePercent >= 0 ? "+" + QString::number(shIndex.changePercent, 'f', 2) 
                                             : QString::number(shIndex.changePercent, 'f', 2));
    }
    
    if (szIndex.isValid()) {
        result += QString("• 深证成指：%1 (%2%)\n")
            .arg(szIndex.price, 'f', 2)
            .arg(szIndex.changePercent >= 0 ? "+" + QString::number(szIndex.changePercent, 'f', 2)
                                             : QString::number(szIndex.changePercent, 'f', 2));
    }
    
    if (cybIndex.isValid()) {
        result += QString("• 创业板指：%1 (%2%)\n")
            .arg(cybIndex.price, 'f', 2)
            .arg(cybIndex.changePercent >= 0 ? "+" + QString::number(cybIndex.changePercent, 'f', 2)
                                             : QString::number(cybIndex.changePercent, 'f', 2));
    }
    
    return result;
}

QString AIAssistant::formatAccountInfo()
{
    // 从数据源获取真实数据
    auto* storage = DataStorageService::instance();
    
    AccountInfo account = storage->getAccountInfo();
    
    if (!account.isValid()) {
        return "暂无账户信息";
    }
    
    double todayProfit = account.totalAsset - account.yesterdayAsset;
    double todayProfitPercent = (todayProfit / account.yesterdayAsset) * 100;
    
    return QString("您的账户概况：\n"
                   "• 总资产：%1元\n"
                   "• 可用资金：%2元\n"
                   "• 持仓市值：%3元\n"
                   "• 今日盈亏：%4元 (%5%)")
        .arg(account.totalAsset, 'f', 2)
        .arg(account.availableCash, 'f', 2)
        .arg(account.positionValue, 'f', 2)
        .arg(todayProfit >= 0 ? "+" + QString::number(todayProfit, 'f', 2) 
                              : QString::number(todayProfit, 'f', 2))
        .arg(todayProfitPercent >= 0 ? "+" + QString::number(todayProfitPercent, 'f', 2)
                                      : QString::number(todayProfitPercent, 'f', 2));
}

QString AIAssistant::formatStockAnalysis(const QString& symbol)
{
    // 从分析模块获取真实分析
    auto* storage = DataStorageService::instance();
    auto* cache = CacheManager::instance();
    
    Quote quote = storage->getStockQuote(symbol);
    if (!quote.isValid()) {
        return QString("未找到股票 %1 的分析信息").arg(symbol);
    }
    
    // 尝试从缓存获取技术分析
    QString analysisKey = QString("analysis:%1").arg(symbol);
    QString analysis;
    
    if (cache->contains(analysisKey)) {
        analysis = cache->get(analysisKey).toString();
    } else {
        // 生成基础分析
        QString trend = quote.changePercent >= 0 ? "上涨" : "下跌";
        QString strength = quote.changePercent >= 3 ? "强势" : 
                          quote.changePercent >= 1 ? "偏强" : 
                          quote.changePercent >= -1 ? "平稳" : "偏弱";
        
        analysis = QString("%1 (%2) 技术分析：\n"
                          "• 今日表现：%3%4，%5\n"
                          "• 成交量：%6手\n"
                          "• 成交额：%7亿\n\n"
                          "综合评价：建议结合基本面综合判断")
            .arg(quote.name)
            .arg(symbol)
            .arg(quote.changePercent >= 0 ? "+" : "")
            .arg(quote.changePercent, 'f', 2)
            .arg(strength)
            .arg(quote.volume)
            .arg(quote.amount / 100000000.0, 'f', 2);
    }
    
    return analysis;
}