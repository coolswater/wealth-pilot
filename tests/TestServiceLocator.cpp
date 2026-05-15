/**
 * @file TestServiceLocator.cpp
 * @brief ServiceLocator单元测试
 */

#include <QtTest/QtTest>
#include "../src/core/di/ServiceLocator.h"

class TestService : public QObject
{
    Q_OBJECT
public:
    explicit TestService(QObject* parent = nullptr) : QObject(parent) {}
    QString name() const { return "TestService"; }
};

class TestServiceLocator : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        qDebug() << "ServiceLocator Test Suite Started";
    }

    void cleanupTestCase()
    {
        qDebug() << "ServiceLocator Test Suite Completed";
    }

    void testRegisterSingleton()
    {
        ServiceLocator::instance().registerSingleton<TestService, TestService>();
        
        QVERIFY(ServiceLocator::instance().isRegistered<TestService>());
        
        auto service1 = ServiceLocator::instance().resolve<TestService>();
        auto service2 = ServiceLocator::instance().resolve<TestService>();
        
        QVERIFY(service1 != nullptr);
        QVERIFY(service2 != nullptr);
        QCOMPARE(service1, service2); // 单例应该返回相同实例
        
        ServiceLocator::instance().unregister<TestService>();
    }

    void testRegisterTransient()
    {
        ServiceLocator::instance().registerTransient<TestService, TestService>();
        
        QVERIFY(ServiceLocator::instance().isRegistered<TestService>());
        
        auto service1 = ServiceLocator::instance().resolve<TestService>();
        auto service2 = ServiceLocator::instance().resolve<TestService>();
        
        QVERIFY(service1 != nullptr);
        QVERIFY(service2 != nullptr);
        QVERIFY(service1 != service2); // 瞬态应该返回不同实例
        
        ServiceLocator::instance().unregister<TestService>();
    }

    void testRegisterInstance()
    {
        TestService* instance = new TestService();
        ServiceLocator::instance().registerInstance<TestService>(instance);
        
        QVERIFY(ServiceLocator::instance().isRegistered<TestService>());
        
        auto service = ServiceLocator::instance().resolve<TestService>();
        QCOMPARE(service, instance);
        
        ServiceLocator::instance().unregister<TestService>();
    }

    void testTryResolve()
    {
        // 未注册的服务应该返回nullptr
        auto service = ServiceLocator::instance().tryResolve<TestService>();
        QVERIFY(service == nullptr);
    }

    void testIsRegistered()
    {
        QVERIFY(!ServiceLocator::instance().isRegistered<TestService>());
        
        ServiceLocator::instance().registerSingleton<TestService, TestService>();
        QVERIFY(ServiceLocator::instance().isRegistered<TestService>());
        
        ServiceLocator::instance().unregister<TestService>();
        QVERIFY(!ServiceLocator::instance().isRegistered<TestService>());
    }

    void testClear()
    {
        ServiceLocator::instance().registerSingleton<TestService, TestService>();
        QVERIFY(ServiceLocator::instance().isRegistered<TestService>());
        
        ServiceLocator::instance().clear();
        QVERIFY(!ServiceLocator::instance().isRegistered<TestService>());
    }
};

QTEST_MAIN(TestServiceLocator)
#include "TestServiceLocator.moc"
