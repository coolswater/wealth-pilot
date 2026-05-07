/**
 * @file AnalysisTest.cpp
 * @brief 技术分析系统单元测试
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include <QtTest/QtTest>
#include "analysis/AnalysisManager.h"
#include "analysis/elliottwave/ElliottWaveAnalyzer.h"
#include "analysis/dowtheory/DowTheoryAnalyzer.h"
#include "analysis/volumepattern/VolumePatternAnalyzer.h"
#include "analysis/signal/SignalFilter.h"

using namespace WealthPilot::Analysis;

/**
 * @brief 波浪理论分析器测试
 */
class ElliottWaveAnalyzerTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        analyzer = new ElliottWave::ElliottWaveAnalyzer();
    }

    void cleanupTestCase()
    {
        delete analyzer;
    }

    void testBasicAnalysis()
    {
        // 生成测试数据
        QVector<KLine> klines = generateTestKlines(100);

        // 执行分析
        auto result = analyzer->analyze(klines);

        // 验证结果
        QVERIFY(result.isValid);
        QVERIFY(!result.signals.isEmpty());
    }

    void testWaveIdentification()
    {
        QVector<KLine> klines = generateTrendingKlines(200, true);

        auto result = analyzer->analyze(klines);

        QVERIFY(result.isValid);

        // 检查是否识别出波浪
        auto* waveCount = analyzer->currentWaveCount();
        if (waveCount) {
            QVERIFY(!waveCount->waves.isEmpty());
        }
    }

    void testEmptyData()
    {
        QVector<KLine> klines;
        auto result = analyzer->analyze(klines);

        QVERIFY(!result.isValid);
    }

    void testInsufficientData()
    {
        QVector<KLine> klines = generateTestKlines(20);
        auto result = analyzer->analyze(klines);

        QVERIFY(!result.isValid);
    }

private:
    ElliottWave::ElliottWaveAnalyzer* analyzer;

    QVector<KLine> generateTestKlines(int count)
    {
        QVector<KLine> klines;
        double price = 100.0;

        for (int i = 0; i < count; ++i) {
            KLine kline;
            kline.time = QDateTime::currentDateTime().addSecs(i * 60);
            kline.open = price;
            kline.high = price + 1;
            kline.low = price - 1;
            kline.close = price + (qrand() % 100 - 50) / 100.0;
            kline.volume = 10000 + qrand() % 50000;

            klines.append(kline);
            price = kline.close;
        }

        return klines;
    }

    QVector<KLine> generateTrendingKlines(int count, bool upward)
    {
        QVector<KLine> klines;
        double price = 100.0;
        double trend = upward ? 0.5 : -0.5;

        for (int i = 0; i < count; ++i) {
            KLine kline;
            kline.time = QDateTime::currentDateTime().addSecs(i * 60);
            kline.open = price;
            kline.high = price + 2;
            kline.low = price - 2;
            kline.close = price + trend + (qrand() % 100 - 50) / 100.0;
            kline.volume = 10000 + qrand() % 50000;

            klines.append(kline);
            price = kline.close;
        }

        return klines;
    }
};

/**
 * @brief 道氏理论分析器测试
 */
class DowTheoryAnalyzerTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        analyzer = new DowTheory::DowTheoryAnalyzer();
    }

    void cleanupTestCase()
    {
        delete analyzer;
    }

    void testTrendIdentification()
    {
        QVector<KLine> klines = generateTrendingKlines(100, true);

        auto result = analyzer->analyze(klines);

        QVERIFY(result.isValid);
        QVERIFY(!result.signals.isEmpty());
    }

    void testUpwardTrend()
    {
        QVector<KLine> klines = generateTrendingKlines(100, true);

        analyzer->analyze(klines);

        auto trend = analyzer->currentTrend();
        QCOMPARE(trend, DowTheory::TrendDirection::Upward);
    }

    void testDownwardTrend()
    {
        QVector<KLine> klines = generateTrendingKlines(100, false);

        analyzer->analyze(klines);

        auto trend = analyzer->currentTrend();
        QCOMPARE(trend, DowTheory::TrendDirection::Downward);
    }

private:
    DowTheory::DowTheoryAnalyzer* analyzer;

    QVector<KLine> generateTrendingKlines(int count, bool upward)
    {
        QVector<KLine> klines;
        double price = 100.0;
        double trend = upward ? 0.5 : -0.5;

        for (int i = 0; i < count; ++i) {
            KLine kline;
            kline.time = QDateTime::currentDateTime().addSecs(i * 60);
            kline.open = price;
            kline.high = price + 2;
            kline.low = price - 2;
            kline.close = price + trend + (qrand() % 100 - 50) / 100.0;
            kline.volume = 10000 + qrand() % 50000;

            klines.append(kline);
            price = kline.close;
        }

        return klines;
    }
};

/**
 * @brief 量价形态分析器测试
 */
class VolumePatternAnalyzerTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        analyzer = new VolumePattern::VolumePatternAnalyzer();
    }

    void cleanupTestCase()
    {
        delete analyzer;
    }

    void testBasicAnalysis()
    {
        QVector<KLine> klines = generateTestKlines(100);

        auto result = analyzer->analyze(klines);

        QVERIFY(result.isValid);
    }

    void testVolumeBreakout()
    {
        QVector<KLine> klines = generateTestKlines(100);

        // 添加放量突破
        KLine breakout;
        breakout.time = QDateTime::currentDateTime();
        breakout.open = 105;
        breakout.high = 110;
        breakout.low = 104;
        breakout.close = 109;
        breakout.volume = 500000; // 大成交量

        klines.append(breakout);

        auto result = analyzer->analyze(klines);

        QVERIFY(result.isValid);
        QVERIFY(result.hasBreakout);
    }

    void testOBVCalculation()
    {
        QVector<KLine> klines = generateTestKlines(50);

        auto result = analyzer->analyze(klines);

        QVERIFY(result.isValid);
    }

private:
    VolumePattern::VolumePatternAnalyzer* analyzer;

    QVector<KLine> generateTestKlines(int count)
    {
        QVector<KLine> klines;
        double price = 100.0;

        for (int i = 0; i < count; ++i) {
            KLine kline;
            kline.time = QDateTime::currentDateTime().addSecs(i * 60);
            kline.open = price;
            kline.high = price + 1;
            kline.low = price - 1;
            kline.close = price + (qrand() % 100 - 50) / 100.0;
            kline.volume = 10000 + qrand() % 50000;

            klines.append(kline);
            price = kline.close;
        }

        return klines;
    }
};

/**
 * @brief 信号过滤器测试
 */
class SignalFilterTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        filter = new SignalFilter();
    }

    void cleanupTestCase()
    {
        delete filter;
    }

    void testBasicFiltering()
    {
        QVector<UnifiedSignal> signals = generateTestSignals();

        auto result = filter->filter(signals);

        QVERIFY(result.confidence >= 0);
    }

    void testConsistencyCheck()
    {
        QVector<UnifiedSignal> signals;

        // 添加一致的信号
        UnifiedSignal s1;
        s1.direction = SignalDirection::Bullish;
        s1.strength = SignalStrength::Strong;
        s1.confidence = 75;
        s1.source = TheoryType::ElliottWave;
        signals.append(s1);

        UnifiedSignal s2;
        s2.direction = SignalDirection::Bullish;
        s2.strength = SignalStrength::Strong;
        s2.confidence = 80;
        s2.source = TheoryType::ChanLun;
        signals.append(s2);

        QVERIFY(filter->checkConsistency(signals));
    }

    void testInconsistentSignals()
    {
        QVector<UnifiedSignal> signals;

        // 添加不一致的信号
        UnifiedSignal s1;
        s1.direction = SignalDirection::Bullish;
        s1.strength = SignalStrength::Strong;
        s1.confidence = 75;
        s1.source = TheoryType::ElliottWave;
        signals.append(s1);

        UnifiedSignal s2;
        s2.direction = SignalDirection::Bearish;
        s2.strength = SignalStrength::Strong;
        s2.confidence = 80;
        s2.source = TheoryType::ChanLun;
        signals.append(s2);

        QVERIFY(!filter->checkConsistency(signals));
    }

    void testScoreCalculation()
    {
        UnifiedSignal signal;
        signal.direction = SignalDirection::Bullish;
        signal.strength = SignalStrength::Strong;
        signal.confidence = 80;
        signal.source = TheoryType::ElliottWave;

        double score = filter->calculateScore(signal);

        QVERIFY(score > 0);
    }

private:
    SignalFilter* filter;

    QVector<UnifiedSignal> generateTestSignals()
    {
        QVector<UnifiedSignal> signals;

        UnifiedSignal s1;
        s1.direction = SignalDirection::Bullish;
        s1.strength = SignalStrength::Strong;
        s1.confidence = 75;
        s1.source = TheoryType::ElliottWave;
        signals.append(s1);

        UnifiedSignal s2;
        s2.direction = SignalDirection::Bullish;
        s2.strength = SignalStrength::Moderate;
        s2.confidence = 65;
        s2.source = TheoryType::ChanLun;
        signals.append(s2);

        return signals;
    }
};

// 运行测试
QTEST_APPLESS_MAIN(ElliottWaveAnalyzerTest)
QTEST_APPLESS_MAIN(DowTheoryAnalyzerTest)
QTEST_APPLESS_MAIN(VolumePatternAnalyzerTest)
QTEST_APPLESS_MAIN(SignalFilterTest)

#include "AnalysisTest.moc"
