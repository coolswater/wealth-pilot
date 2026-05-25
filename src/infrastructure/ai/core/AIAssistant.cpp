/**
 * @file AIAssistant.cpp
 * @brief AI 智能助手实现
 */

#include "AIAssistant.h"
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

    // TODO: 基于用户偏好和历史生成推荐
    // 这里返回示例数据
    Recommendation r1;
    r1.symbol = "600519";
    r1.name = "贵州茅台";
    r1.score = 0.95;
    r1.reason = "基本面优秀，估值合理，适合长期持有";
    r1.category = "价值投资";
    r1.targetPrice = 1800.0;
    recommendations.append(r1);

    Recommendation r2;
    r2.symbol = "000858";
    r2.name = "五粮液";
    r2.score = 0.88;
    r2.reason = "白酒龙头，业绩稳定增长";
    r2.category = "价值投资";
    r2.targetPrice = 180.0;
    recommendations.append(r2);

    emit recommendationsUpdated(recommendations);
    return recommendations;
}

QVector<Recommendation> AIAssistant::getMarketHotspots()
{
    QVector<Recommendation> hotspots;

    // TODO: 从数据源获取市场热点
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

    // TODO: 基于行业、概念等找相似股票
    Q_UNUSED(symbol)
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
    // TODO: 从数据源获取真实数据
    return QString("股票 %1 的行情信息：\n"
                   "• 现价：100.00元\n"
                   "• 涨跌幅：+2.5%\n"
                   "• 成交量：1,234,567手\n"
                   "• 市值：1,234亿元").arg(symbol);
}

QString AIAssistant::formatMarketInfo()
{
    // TODO: 从数据源获取真实数据
    return "今日市场概况：\n"
           "• 上证指数：3,123.45 (+0.85%)\n"
           "• 深证成指：10,234.56 (+1.12%)\n"
           "• 创业板指：2,045.67 (+1.56%)\n"
           "• 两市成交：8,567亿元";
}

QString AIAssistant::formatAccountInfo()
{
    // TODO: 从数据源获取真实数据
    return "您的账户概况：\n"
           "• 总资产：1,234,567.89元\n"
           "• 可用资金：234,567.89元\n"
           "• 持仓市值：1,000,000.00元\n"
           "• 今日盈亏：+12,345.67元 (+1.23%)";
}

QString AIAssistant::formatStockAnalysis(const QString& symbol)
{
    // TODO: 从分析模块获取真实分析
    return QString("%1 技术分析：\n"
                   "• 均线：多头排列，趋势向上\n"
                   "• MACD：金叉，看涨信号\n"
                   "• RSI：65，偏强\n"
                   "• 成交量：放量上涨\n"
                   "• 支撑位：95.00元\n"
                   "• 压力位：105.00元\n\n"
                   "综合评价：建议逢低关注").arg(symbol);
}