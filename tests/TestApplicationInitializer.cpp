/**
 * @file TestApplicationInitializer.cpp
 * @brief ApplicationInitializer集成测试
 */

#include <QtTest/QtTest>
#include "../src/app/ApplicationInitializer.h"
#include "../src/core/cache/CacheManager.h"
#include "../src/core/di/ServiceLocator.h"

class TestApplicationInitializer : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        qDebug() << "ApplicationInitializer Test Suite Started";
    }

    void cleanupTestCase()
    {
        ApplicationInitializer::instance().shutdown();
        qDebug() << "ApplicationInitializer Test Suite Completed";
    }

    void testInitialize()
    {
        // 初始化应用
        bool result = ApplicationInitializer::instance().initialize();
        QVERIFY(result);
        
        // 检查当前阶段
        QCOMPARE(ApplicationInitializer::instance().currentPhase(), InitPhase::Complete);
    }

    void testResults()
    {
        // 获取初始化结果
        auto results = ApplicationInitializer::instance().results();
        QVERIFY(!results.isEmpty());
        
        // 检查所有模块是否成功初始化
        for (auto it = results.begin(); it != results.end(); ++it) {
            qDebug() << it.key() << ":" << it.value().success << it.value().duration << "ms";
            QVERIFY(it.value().success);
        }
    }

    void testModuleInitialization()
    {
        // 重新初始化
        ApplicationInitializer::instance().shutdown();
        
        bool result = ApplicationInitializer::instance().initialize();
        QVERIFY(result);
        
        // 验证核心模块已初始化
        QVERIFY(CacheManager::instance()->statistics().itemCount >= 0);
    }

    void testPerformance()
    {
        // 性能测试：初始化时间
        ApplicationInitializer::instance().shutdown();
        
        QElapsedTimer timer;
        timer.start();
        
        bool result = ApplicationInitializer::instance().initialize();
        qint64 initTime = timer.elapsed();
        
        QVERIFY(result);
        qDebug() << "Application initialized in" << initTime << "ms";
        
        // 初始化时间应该小于2000ms
        QVERIFY(initTime < 2000);
    }

    void testShutdown()
    {
        // 初始化
        ApplicationInitializer::instance().initialize();
        
        // 关闭
        ApplicationInitializer::instance().shutdown();
        
        // 验证资源已清理
        // 注意：ServiceLocator应该被清空
        QVERIFY(ServiceLocator::instance().count() == 0);
    }
};

QTEST_MAIN(TestApplicationInitializer)
#include "TestApplicationInitializer.moc"
