#ifndef FUTURESMOCKDATAGENERATOR_H
#define FUTURESMOCKDATAGENERATOR_H

#include <QRandomGenerator>
#include <QDateTime>

#include <data/models/FuturesQuoteitem.h>

/**
 * @brief 模拟行情数据生成器
 * 生成初始数据和模拟实时推送
 */
class FuturesMockDataGenerator
{
public:
    FuturesMockDataGenerator();

    // 生成初始完整数据（25个合约，模仿图片）
    QVector<FuturesQuoteItem> generateInitialData();

    // 模拟行情推送（随机更新几个合约的价格）
    QVector<FuturesQuoteItem> generateTickUpdates(const QVector<FuturesQuoteItem> &currentData);

private:
    QStringList m_contractNames;
    QStringList m_contractCodes;

    double randomPrice(double base, double range);
    int randomInt(int min, int max);
};

#endif // FUTURESMOCKDATAGENERATOR_H
