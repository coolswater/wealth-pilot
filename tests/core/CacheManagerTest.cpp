/**
 * @file CacheManagerTest.cpp
 * @brief CacheManager 单元测试
 */

#include <QtTest/QtTest>
#include "core/services/cache/CacheManager.h"

using namespace WealthPilot;

class CacheManagerTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // 基础功能测试
    void testSetAndGet();
    void testContains();
    void testRemove();
    void testClear();
    
    // TTL 测试
    void testTTL();
    void testExpired();
    
    // 容量测试
    void testMaxSize();
    void testEviction();
    
    // 并发测试
    void testConcurrentAccess();
    
    // 性能测试
    void testPerformance();

private:
    CacheManager* m_cache = nullptr;
};

void CacheManagerTest::initTestCase()
{
    m_cache = CacheManager::instance();
    QVERIFY(m_cache != nullptr);
    QVERIFY(m_cache->initialize());
}

void CacheManagerTest::cleanupTestCase()
{
    if (m_cache) {
        m_cache->clear();
    }
}

void CacheManagerTest::testSetAndGet()
{
    // 基础读写
    m_cache->set("key1", QVariant("value1"));
    QVERIFY(m_cache->contains("key1"));
    QCOMPARE(m_cache->get("key1").toString(), QString("value1"));
    
    // 整数
    m_cache->set("int_key", QVariant(42));
    QCOMPARE(m_cache->get("int_key").toInt(), 42);
    
    // 复杂对象
    QVariantMap map;
    map["name"] = "test";
    map["value"] = 100;
    m_cache->set("map_key", map);
    QCOMPARE(m_cache->get("map_key").toMap()["name"].toString(), QString("test"));
}

void CacheManagerTest::testContains()
{
    m_cache->set("exists", QVariant(true));
    QVERIFY(m_cache->contains("exists"));
    QVERIFY(!m_cache->contains("not_exists"));
}

void CacheManagerTest::testRemove()
{
    m_cache->set("to_remove", QVariant(1));
    QVERIFY(m_cache->contains("to_remove"));
    
    m_cache->remove("to_remove");
    QVERIFY(!m_cache->contains("to_remove"));
}

void CacheManagerTest::testClear()
{
    m_cache->set("key1", QVariant(1));
    m_cache->set("key2", QVariant(2));
    m_cache->set("key3", QVariant(3));
    
    m_cache->clear();
    
    QVERIFY(!m_cache->contains("key1"));
    QVERIFY(!m_cache->contains("key2"));
    QVERIFY(!m_cache->contains("key3"));
}

void CacheManagerTest::testTTL()
{
    // 设置 1 秒 TTL
    m_cache->set("ttl_key", QVariant("ttl_value"), 1);
    QVERIFY(m_cache->contains("ttl_key"));
    
    // 等待过期
    QTest::qWait(1100);
    
    QVERIFY(!m_cache->contains("ttl_key"));
}

void CacheManagerTest::testExpired()
{
    m_cache->set("expire_key", QVariant(1), 1);
    QTest::qWait(1100);
    
    // 获取过期数据应返回无效 QVariant
    QVariant value = m_cache->get("expire_key");
    QVERIFY(!value.isValid());
}

void CacheManagerTest::testMaxSize()
{
    // 设置最大容量
    m_cache->setMaxSize(3);
    
    m_cache->set("a", QVariant(1));
    m_cache->set("b", QVariant(2));
    m_cache->set("c", QVariant(3));
    m_cache->set("d", QVariant(4)); // 应该触发淘汰
    
    // 最早插入的应该被淘汰
    QVERIFY(!m_cache->contains("a"));
    QVERIFY(m_cache->contains("d"));
    
    // 恢复默认容量
    m_cache->setMaxSize(1000);
}

void CacheManagerTest::testEviction()
{
    m_cache->clear();
    m_cache->setMaxSize(2);
    
    m_cache->set("x", QVariant(1));
    m_cache->set("y", QVariant(2));
    
    // 访问 x，使其成为最近使用
    m_cache->get("x");
    
    // 插入新数据，应淘汰 y（最久未使用）
    m_cache->set("z", QVariant(3));
    
    QVERIFY(m_cache->contains("x"));
    QVERIFY(!m_cache->contains("y"));
    QVERIFY(m_cache->contains("z"));
    
    m_cache->setMaxSize(1000);
}

void CacheManagerTest::testConcurrentAccess()
{
    // 并发读写测试
    QMutex mutex;
    QList<QFuture<void>> futures;
    
    auto writer = [this, &mutex]() {
        QMutexLocker locker(&mutex);
        for (int i = 0; i < 100; ++i) {
            m_cache->set(QString("concurrent_%1").arg(i), QVariant(i));
        }
    };
    
    auto reader = [this, &mutex]() {
        QMutexLocker locker(&mutex);
        for (int i = 0; i < 100; ++i) {
            m_cache->get(QString("concurrent_%1").arg(i));
        }
    };
    
    futures.append(QtConcurrent::run(writer));
    futures.append(QtConcurrent::run(reader));
    
    for (auto& f : futures) {
        f.waitForFinished();
    }
}

void CacheManagerTest::testPerformance()
{
    QBENCHMARK {
        for (int i = 0; i < 1000; ++i) {
            m_cache->set(QString("perf_%1").arg(i), QVariant(i));
        }
        for (int i = 0; i < 1000; ++i) {
            m_cache->get(QString("perf_%1").arg(i));
        }
    }
}

QTEST_MAIN(CacheManagerTest)
#include "CacheManagerTest.moc"
