/**
 * @file ThemeManagerTest.cpp
 * @brief ThemeManager 单元测试
 */

#include <QtTest/QtTest>
#include "presentation/styles/ThemeManager.h"

using namespace WealthPilot;

class ThemeManagerTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // 主题切换测试
    void testSetTheme();
    void testThemeChanged();
    
    // 配色测试
    void testGetThemeColors();
    void testThemeColorsValidity();
    
    // 监听器测试
    void testRegisterListener();
    void testListenerCalled();
    
    // 性能测试
    void testThemeSwitchPerformance();

private:
    ThemeManager* m_themeManager = nullptr;
    int m_listenerCallCount = 0;
};

void ThemeManagerTest::initTestCase()
{
    m_themeManager = ThemeManager::instance();
    QVERIFY(m_themeManager != nullptr);
    QVERIFY(m_themeManager->initialize());
}

void ThemeManagerTest::cleanupTestCase()
{
    // 恢复默认主题
    m_themeManager->setTheme(ThemeType::Dark);
}

void ThemeManagerTest::testSetTheme()
{
    // 切换到浅色主题
    m_themeManager->setTheme(ThemeType::Light);
    QCOMPARE(m_themeManager->currentThemeType(), ThemeType::Light);
    
    // 切换到深色主题
    m_themeManager->setTheme(ThemeType::Dark);
    QCOMPARE(m_themeManager->currentThemeType(), ThemeType::Dark);
    
    // 切换到护眼主题
    m_themeManager->setTheme(ThemeType::EyeCare);
    QCOMPARE(m_themeManager->currentThemeType(), ThemeType::EyeCare);
}

void ThemeManagerTest::testThemeChanged()
{
    QSignalSpy spy(m_themeManager, &ThemeManager::themeChanged);
    
    m_themeManager->setTheme(ThemeType::Light);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).value<ThemeType>(), ThemeType::Light);
    
    // 切换到相同主题不应发射信号
    m_themeManager->setTheme(ThemeType::Light);
    QCOMPARE(spy.count(), 1);
}

void ThemeManagerTest::testGetThemeColors()
{
    ThemeColors darkTheme = m_themeManager->getTheme(ThemeType::Dark);
    QVERIFY(!darkTheme.bgPrimary.isEmpty());
    QVERIFY(!darkTheme.textPrimary.isEmpty());
    QVERIFY(!darkTheme.primary.isEmpty());
    
    ThemeColors lightTheme = m_themeManager->getTheme(ThemeType::Light);
    QVERIFY(lightTheme.bgPrimary != darkTheme.bgPrimary);
}

void ThemeManagerTest::testThemeColorsValidity()
{
    // 验证所有主题配色有效
    QList<ThemeType> types = {
        ThemeType::Dark, ThemeType::Light, 
        ThemeType::HighContrast, ThemeType::EyeCare
    };
    
    for (ThemeType type : types) {
        ThemeColors colors = m_themeManager->getTheme(type);
        QVERIFY2(!colors.bgPrimary.isEmpty(), 
                 qPrintable(QString("bgPrimary empty for theme %1").arg(static_cast<int>(type))));
        QVERIFY2(!colors.textPrimary.isEmpty(),
                 qPrintable(QString("textPrimary empty for theme %1").arg(static_cast<int>(type))));
        QVERIFY2(!colors.danger.isEmpty(),
                 qPrintable(QString("danger empty for theme %1").arg(static_cast<int>(type))));
        QVERIFY2(!colors.success.isEmpty(),
                 qPrintable(QString("success empty for theme %1").arg(static_cast<int>(type))));
    }
}

void ThemeManagerTest::testRegisterListener()
{
    QWidget testWidget;
    m_themeManager->registerThemeChangeListener(&testWidget, [this]() {
        m_listenerCallCount++;
    });
    
    // 注册后立即不会触发
    QCOMPARE(m_listenerCallCount, 0);
}

void ThemeManagerTest::testListenerCalled()
{
    m_listenerCallCount = 0;
    
    QWidget testWidget;
    m_themeManager->registerThemeChangeListener(&testWidget, [this]() {
        m_listenerCallCount++;
    });
    
    // 切换主题应触发监听器
    m_themeManager->setTheme(ThemeType::Light);
    
    // 注意：监听器是异步调用的
    QTest::qWait(100);
    
    QCOMPARE(m_listenerCallCount, 1);
}

void ThemeManagerTest::testThemeSwitchPerformance()
{
    // 测量主题切换性能（应在 100ms 以内）
    QBENCHMARK {
        m_themeManager->setTheme(ThemeType::Light);
        m_themeManager->setTheme(ThemeType::Dark);
    }
}

QTEST_MAIN(ThemeManagerTest)
#include "ThemeManagerTest.moc"
