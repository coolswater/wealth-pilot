/**
 * @file PerformanceTest.cpp
 * @brief 性能测试套件（简化版）
 */

#include <QtTest/QtTest>
#include <QElapsedTimer>
#include <QRandomGenerator>

class PerformanceTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        qDebug() << "Performance Test Suite Started";
    }

    void cleanupTestCase()
    {
        qDebug() << "Performance Test Suite Completed";
    }

    void testLoopPerformance()
    {
        QElapsedTimer timer;
        timer.start();

        int sum = 0;
        for (int i = 0; i < 1000000; ++i) {
            sum += i;
        }

        qint64 elapsed = timer.elapsed();
        qDebug() << QString("Loop 1M iterations: %1ms").arg(elapsed);

        // 应该在 100ms 内完成
        QVERIFY(elapsed < 100);
    }

    void testDataGenerationPerformance()
    {
        QElapsedTimer timer;
        timer.start();

        QVector<double> data;
        data.reserve(10000);
        auto rng = QRandomGenerator::global();

        for (int i = 0; i < 10000; ++i) {
            data.append(100.0 + rng->bounded(100) / 100.0);
        }

        qint64 elapsed = timer.elapsed();
        qDebug() << QString("Generate 10K items: %1ms").arg(elapsed);

        QCOMPARE(data.size(), 10000);
        QVERIFY(elapsed < 50);
    }

    void testStringPerformance()
    {
        QElapsedTimer timer;
        timer.start();

        QString result;
        for (int i = 0; i < 10000; ++i) {
            result = QString("Item %1: %2").arg(i).arg(i * 2);
        }

        qint64 elapsed = timer.elapsed();
        qDebug() << QString("String format 10K: %1ms").arg(elapsed);

        QVERIFY(elapsed < 100);
    }
};

QTEST_APPLESS_MAIN(PerformanceTest)
#include "PerformanceTest.moc"