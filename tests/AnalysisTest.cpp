/**
 * @file AnalysisTest.cpp
 * @brief 技术分析系统单元测试
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include <QtTest/QtTest>
#include <QRandomGenerator>
#include "../src/analysis/AnalysisManager.h"
#include "../src/analysis/AnalysisTypes.h"

using namespace WealthPilot::Analysis;

/**
 * @brief K线数据生成辅助函数
 */
QVector<KLine> generateTestKlines(int count)
{
    QVector<KLine> klines;
    double price = 100.0;
    auto rng = QRandomGenerator::global();

    for (int i = 0; i < count; ++i) {
        KLine kline;
        kline.time = QDateTime::currentDateTime().addSecs(i * 60);
        kline.open = price;
        kline.high = price + 1;
        kline.low = price - 1;
        kline.close = price + (rng->bounded(100) - 50) / 100.0;
        kline.volume = 10000 + rng->bounded(50000);

        klines.append(kline);
        price = kline.close;
    }

    return klines;
}

/**
 * @brief 技术分析管理器测试
 */
class AnalysisManagerTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        // AnalysisManager 是单例，使用 instance()
        QVERIFY(AnalysisManager::instance() != nullptr);
    }

    void cleanupTestCase()
    {
        // 单例不需要删除
    }

    void testBasicAnalysis()
    {
        QVector<KLine> klines = generateTestKlines(100);

        // 执行分析 - 需要传入 symbol
        auto result = AnalysisManager::instance()->analyze("TEST", klines);

        // 验证结果有效 - 检查 symbol 是否正确
        QCOMPARE(result.symbol, QString("TEST"));
    }

    void testEmptyData()
    {
        QVector<KLine> klines;
        auto result = AnalysisManager::instance()->analyze("TEST", klines);

        // 空数据应该返回有效的 CompositeSignal，但 theoryCount 为 0
        QCOMPARE(result.theoryCount, 0);
    }

    void testLargeDataset()
    {
        QVector<KLine> klines = generateTestKlines(1000);
        auto result = AnalysisManager::instance()->analyze("TEST", klines);

        // 验证 symbol 正确
        QCOMPARE(result.symbol, QString("TEST"));
    }

    void testSignalScore()
    {
        CompositeSignal signal;
        signal.theoryCount = 3;
        signal.confidence = 80.0;

        // 验证得分计算
        double score = signal.score();
        QVERIFY(score > 0);
        QVERIFY(signal.isStrongSignal());
    }
};

/**
 * @brief K线数据测试
 */
class KLineDataTest : public QObject
{
    Q_OBJECT

private slots:
    void testKLineCreation()
    {
        KLine kline;
        kline.time = QDateTime::currentDateTime();
        kline.open = 100.0;
        kline.high = 105.0;
        kline.low = 98.0;
        kline.close = 103.0;
        kline.volume = 1000000;

        QCOMPARE(kline.open, 100.0);
        QCOMPARE(kline.high, 105.0);
        QCOMPARE(kline.low, 98.0);
        QCOMPARE(kline.close, 103.0);
        QCOMPARE(kline.volume, 1000000);
    }

    void testKLineValidation()
    {
        KLine kline;
        kline.open = 100.0;
        kline.high = 105.0;
        kline.low = 98.0;
        kline.close = 103.0;

        // 高价应该 >= 低价
        QVERIFY(kline.high >= kline.low);
    }

    void testKLineArrayGeneration()
    {
        QVector<KLine> klines = generateTestKlines(100);
        QCOMPARE(klines.size(), 100);

        // 验证连续性
        for (int i = 1; i < klines.size(); ++i) {
            QVERIFY(klines[i].time > klines[i-1].time);
        }
    }
};

// 运行测试 - 只能有一个 main
QTEST_APPLESS_MAIN(AnalysisManagerTest)

#include "AnalysisTest.moc"