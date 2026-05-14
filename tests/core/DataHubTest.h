/**
 * @file DataHubTest.h
 * @brief DataHub 单元测试
 *
 * @details 测试内容：
 * - 订阅/发布功能
 * - 生命周期管理
 * - 模式匹配
 * - 数据缓存
 * - 性能测试
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef DATAHUBTEST_H
#define DATAHUBTEST_H

#include <QObject>
#include <QtTest/QtTest>
#include "core/datahub/DataHub.h"
#include "core/datahub/MarketDataProducer.h"

/**
 * @brief DataHub 单元测试类
 */
class DataHubTest : public QObject
{
    Q_OBJECT

private slots:
    /**
     * @brief 初始化测试
     */
    void initTestCase();

    /**
     * @brief 清理测试
     */
    void cleanupTestCase();

    // ========== 基础功能测试 ==========

    /**
     * @brief 测试单例模式
     */
    void testSingleton();

    /**
     * @brief 测试订阅/发布
     */
    void testSubscribePublish();

    /**
     * @brief 测试取消订阅
     */
    void testUnsubscribe();

    /**
     * @brief 测试模式匹配
     */
    void testPatternMatching();

    /**
     * @brief 测试数据缓存
     */
    void testDataCache();

    /**
     * @brief 测试生命周期管理
     */
    void testLifecycle();

    // ========== 性能测试 ==========

    /**
     * @brief 测试大量订阅性能
     */
    void testPerformanceManySubscriptions();

    /**
     * @brief 测试高频发布性能
     */
    void testPerformanceHighFrequency();

    /**
     * @brief 测试内存使用
     */
    void testMemoryUsage();

private:
    DataHub* m_hub = nullptr;
    int m_callbackCount = 0;
};

#endif // DATAHUBTEST_H