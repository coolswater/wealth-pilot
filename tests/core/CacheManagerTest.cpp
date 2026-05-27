/**
 * @file CacheManagerTest.cpp
 * @brief CacheManager 单元测试
 */

#include <QtTest/QtTest>
#include <QtConcurrent>
#include "core/services/cache/CacheManager.h"

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
        m_cache->clearAll();
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
