/**
 * @file TestPluginLoader.cpp
 * @brief PluginLoader集成测试
 */

#include <QtTest/QtTest>
#include "../src/plugins/PluginLoader.h"
#include "../src/plugins/IPlugin.h"

// 测试插件实现
class TestPlugin : public IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.wealthpilot.IPlugin" FILE "testplugin.json")
    Q_INTERFACES(IPlugin)

public:
    PluginMetaData metaData() const override
    {
        PluginMetaData meta;
        meta.name = "TestPlugin";
        meta.version = "1.0.0";
        meta.description = "Test Plugin";
        meta.priority = 100;
        return meta;
    }

    PluginState state() const override { return m_state; }
    bool load() override { m_state = PluginState::Loaded; return true; }
    bool initialize(const QJsonObject&) override { m_state = PluginState::Initialized; return true; }
    bool start() override { m_state = PluginState::Running; return true; }
    void stop() override { m_state = PluginState::Stopped; }
    void unload() override { m_state = PluginState::Unloaded; }
    QJsonObject configuration() const override { return QJsonObject(); }
    void setConfiguration(const QJsonObject&) override {}
    bool checkDependencies() const override { return true; }
    QStringList dependencies() const override { return QStringList(); }

private:
    PluginState m_state = PluginState::Unloaded;
};

class TestPluginLoader : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        qDebug() << "PluginLoader Test Suite Started";
    }

    void cleanupTestCase()
    {
        PluginLoader::instance().unloadAll();
        qDebug() << "PluginLoader Test Suite Completed";
    }

    void testRegisterBuiltInPlugin()
    {
        TestPlugin* plugin = new TestPlugin();
        
        QVERIFY(PluginLoader::instance().registerBuiltInPlugin(plugin));
        
        // 重复注册应该失败
        QVERIFY(!PluginLoader::instance().registerBuiltInPlugin(plugin));
    }

    void testLoadPlugin()
    {
        // 注册测试插件
        TestPlugin* plugin = new TestPlugin();
        PluginLoader::instance().registerBuiltInPlugin(plugin);
        
        // 加载插件
        QVERIFY(PluginLoader::instance().loadPlugin("TestPlugin"));
        
        // 检查状态
        QCOMPARE(PluginLoader::instance().pluginState("TestPlugin"), PluginState::Running);
        QVERIFY(PluginLoader::instance().isLoaded("TestPlugin"));
    }

    void testUnloadPlugin()
    {
        TestPlugin* plugin = new TestPlugin();
        PluginLoader::instance().registerBuiltInPlugin(plugin);
        PluginLoader::instance().loadPlugin("TestPlugin");
        
        // 卸载插件
        QVERIFY(PluginLoader::instance().unloadPlugin("TestPlugin"));
        
        // 检查状态
        QVERIFY(!PluginLoader::instance().isLoaded("TestPlugin"));
    }

    void testGetPlugin()
    {
        TestPlugin* plugin = new TestPlugin();
        PluginLoader::instance().registerBuiltInPlugin(plugin);
        PluginLoader::instance().loadPlugin("TestPlugin");
        
        // 获取插件
        IPlugin* loadedPlugin = PluginLoader::instance().getPlugin<IPlugin>("TestPlugin");
        QVERIFY(loadedPlugin != nullptr);
        QCOMPARE(loadedPlugin->metaData().name, QString("TestPlugin"));
    }

    void testLoadedPlugins()
    {
        PluginLoader::instance().unloadAll();
        
        TestPlugin* plugin1 = new TestPlugin();
        plugin1->metaData().name = "TestPlugin1";
        PluginLoader::instance().registerBuiltInPlugin(plugin1);
        
        TestPlugin* plugin2 = new TestPlugin();
        plugin2->metaData().name = "TestPlugin2";
        PluginLoader::instance().registerBuiltInPlugin(plugin2);
        
        PluginLoader::instance().loadPlugin("TestPlugin1");
        PluginLoader::instance().loadPlugin("TestPlugin2");
        
        QStringList loaded = PluginLoader::instance().loadedPlugins();
        QVERIFY(loaded.contains("TestPlugin1"));
        QVERIFY(loaded.contains("TestPlugin2"));
    }

    void testPluginMetaData()
    {
        TestPlugin* plugin = new TestPlugin();
        PluginLoader::instance().registerBuiltInPlugin(plugin);
        PluginLoader::instance().loadPlugin("TestPlugin");
        
        PluginMetaData meta = PluginLoader::instance().pluginMetaData("TestPlugin");
        QCOMPARE(meta.name, QString("TestPlugin"));
        QCOMPARE(meta.version, QString("1.0.0"));
    }

    void testPerformance()
    {
        // 性能测试：加载多个插件
        QElapsedTimer timer;
        timer.start();
        
        for (int i = 0; i < 10; ++i) {
            TestPlugin* plugin = new TestPlugin();
            plugin->metaData().name = QString("PerfTestPlugin%1").arg(i);
            PluginLoader::instance().registerBuiltInPlugin(plugin);
        }
        
        qint64 registerTime = timer.elapsed();
        qDebug() << "Register 10 plugins in" << registerTime << "ms";
        
        timer.restart();
        for (int i = 0; i < 10; ++i) {
            PluginLoader::instance().loadPlugin(QString("PerfTestPlugin%1").arg(i));
        }
        
        qint64 loadTime = timer.elapsed();
        qDebug() << "Load 10 plugins in" << loadTime << "ms";
        
        // 加载时间应该小于500ms
        QVERIFY(loadTime < 500);
    }
};

QTEST_MAIN(TestPluginLoader)
#include "TestPluginLoader.moc"
