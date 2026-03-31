#include "FuturesMockDataGenerator.h"

FuturesMockDataGenerator::FuturesMockDataGenerator()
{
    // 初始化合约列表（模仿图片中的沪铜、沪铝系列）
    m_contractNames = {
        "沪铜加权", "沪铜主连", "沪铜2404", "沪铜2405", "沪铜2406",
        "沪铜2407", "沪铜2408", "沪铜2409", "沪铜2410", "沪铜2411",
        "沪铝加权", "沪铝主连", "沪铝2404", "沪铝2405", "沪铝2406",
        "沪铝2407", "沪铝2408", "沪铝2409", "沪铝2410",
        "沪金加权", "沪金主连", "沪金2406", "沪金2408", "沪金2410", "沪金2412"
    };

    m_contractCodes = {
        "CUW", "CUM", "cu2404", "cu2405", "cu2406",
        "cu2407", "cu2408", "cu2409", "cu2410", "cu2411",
        "ALW", "ALM", "al2404", "al2405", "al2406",
        "al2407", "al2408", "al2409", "al2410",
        "AUW", "AUM", "au2406", "au2408", "au2410", "au2412"
    };
}

double FuturesMockDataGenerator::randomPrice(double base, double range)
{
    double offset = (QRandomGenerator::global()->generateDouble() - 0.5) * 2 * range;
    return base + offset;
}

int FuturesMockDataGenerator::randomInt(int min, int max)
{
    return QRandomGenerator::global()->bounded(min, max + 1);
}

QVector<FuturesQuoteItem> FuturesMockDataGenerator::generateInitialData()
{
    QVector<FuturesQuoteItem> data;

    for (int i = 0; i < m_contractNames.size(); ++i) {
        FuturesQuoteItem item;
        item.serialNo = i + 1;
        item.contractName = m_contractNames[i];
        item.contractCode = m_contractCodes[i];

        // 基础价格设置（铜价约70000，铝价约19000，金价约550）
        double basePrice = 70000;
        if (m_contractNames[i].contains("沪铝")) basePrice = 19000;
        if (m_contractNames[i].contains("沪金")) basePrice = 550;
        if (m_contractNames[i].contains("加权") || m_contractNames[i].contains("主连")) {
            basePrice *= 1.02;  // 加权/主连略高
        }

        // 生成昨日数据
        item.preSettlement = basePrice + randomPrice(0, 500);
        item.preClose = item.preSettlement + randomPrice(0, 100);

        // 生成今日数据（围绕昨结波动）
        item.openPrice = item.preSettlement + randomPrice(0, 200);
        item.latestPrice = item.openPrice + randomPrice(0, 300);
        item.highPrice = std::max(item.openPrice, item.latestPrice) + randomInt(50, 200);
        item.lowPrice = std::min(item.openPrice, item.latestPrice) - randomInt(50, 200);
        item.settlement = item.latestPrice;  // 简化为最新价

        // 计算涨跌
        item.change = item.latestPrice - item.preSettlement;
        item.changePercent = (item.change / item.preSettlement) * 100;

        // 盘口数据
        item.bidPrice = item.latestPrice - randomInt(0, 10);
        item.askPrice = item.latestPrice + randomInt(0, 10);
        item.bidVolume = randomInt(1, 100);
        item.askVolume = randomInt(1, 100);

        // 成交量持仓
        item.volume = randomInt(10000, 500000);
        item.currentHand = randomInt(1, 50);
        item.openInterest = randomInt(100000, 1000000);
        item.oiChange = randomInt(-5000, 5000);

        // 资金数据
        item.capital = randomInt(10, 200);  // 亿
        item.capitalFlow = randomPrice(0, 5);  // 正负5亿

        // 其他指标
        item.speedChange = randomPrice(0, 0.5);
        item.currentChange = item.change;
        item.currentOiChange = item.oiChange;
        item.trendDegree = randomPrice(0, 1);
        item.speculationDegree = randomPrice(0, 1);

        data.append(item);
    }

    return data;
}

QVector<FuturesQuoteItem> FuturesMockDataGenerator::generateTickUpdates(const QVector<FuturesQuoteItem> &currentData)
{
    QVector<FuturesQuoteItem> updates;
    int updateCount = QRandomGenerator::global()->bounded(1, 6);  // 每次更新1-5个合约

    for (int i = 0; i < updateCount; ++i) {
        int idx = QRandomGenerator::global()->bounded(currentData.size());
        FuturesQuoteItem item = currentData[idx];  // 复制

        // 模拟价格跳动（跳1-5个点）
        double tickSize = item.contractName.contains("沪金") ? 0.02 : 10;
        int direction = QRandomGenerator::global()->bounded(3) - 1;  // -1, 0, 1
        double priceChange = direction * tickSize * randomInt(1, 5);

        item.latestPrice += priceChange;
        item.change = item.latestPrice - item.preSettlement;
        item.changePercent = (item.change / item.preSettlement) * 100;

        // 更新盘口
        item.bidPrice = item.latestPrice - tickSize * randomInt(1, 3);
        item.askPrice = item.latestPrice + tickSize * randomInt(1, 3);
        item.bidVolume = randomInt(1, 100);
        item.askVolume = randomInt(1, 100);
        item.currentHand = randomInt(1, 20);
        item.volume += item.currentHand;

        // 资金流向微变
        item.capitalFlow += randomPrice(-0.1, 0.1);

        updates.append(item);
    }

    return updates;
}
