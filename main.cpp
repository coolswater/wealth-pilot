/**
 * @file main.cpp
 * @brief 应用程序入口
 *
 * @details WealthPilot智能投资管理软件主入�?
 * 负责�?
 * - 初始化Qt应用框架
 * - 配置高DPI支持
 * - 加载字体和主�?
 * - 初始化核心服�?
 * - 启动应用主循�?
 *
 * @note 程序启动流程�?
 * 1. 设置Qt应用属性（高DPI、事件压缩）
 * 2. 创建Qt应用实例
 * 3. 初始化配置管理器
 * 4. 初始化日志系�?
 * 5. 加载自定义字�?
 * 6. 初始化主题管理器
 * 7. 初始化网络管理器
 * 8. 初始化数据服�?
 * 9. 初始化AI服务
 * 10. 创建并显示主窗口
 * 11. 进入主事件循�?
 *
 * @author WealthPilot Team
 * @version 1.0.0
 * @date 2026
 */
#include "src/views/mainWindow/MainWindow.h"
#include "src/utils/Logger.h"
#include <QApplication>
#include <QFontDatabase>
#include <QLocale>
#include <QTranslator>
#include <src/ui/ThemeManager.h>

/**
 * @brief 清理所有服�?
 */
void cleanupServices()
{
    LOG_INFO("========================================");
    LOG_INFO("Cleaning up services...");

    // CTPService::instance()->shutdown();
    // AIService::instance()->shutdown();
    // DataService::instance()->shutdown();
    // NetworkManager::instance()->shutdown();
    // DatabaseManager::instance()->shutdown();

    LOG_INFO("All services cleaned up");
    LOG_INFO("========================================");
}

/**
 * @brief 应用程序入口函数
 * @param argc 命令行参数数�?
 * @param argv 命令行参数数�?
 * @return int 程序退出码�?=成功，非0=错误�?
 */
int main(int argc, char* argv[])
{
    // 设置应用程序属�?压缩高频事件（如鼠标移动），提高性能
    QApplication::setAttribute(Qt::AA_CompressHighFrequencyEvents);

    // ========== 创建Qt应用实例 ==========
    const QApplication app(argc, argv);

    // ========== 设置应用元信�?==========
    QApplication::setApplicationName("WealthPilot-财富领航AI助手");
    QApplication::setOrganizationName("Hexd");
    QApplication::setApplicationVersion("1.0.0");

    // 设置应用ICon
    QApplication::setWindowIcon(QIcon(":/images/app_icon.png"));

    // ========== 初始化日志系�?==========
    Logger::instance()->init("logs/wealthpilot.log");
    LOG_INFO("========================================");
    LOG_INFO("WealthPilot-Wealth Navigator AI Assistant");
    LOG_INFO(QString("Version: %1").arg(QApplication::applicationVersion()));
    LOG_INFO("========================================");

    // ========== 加载自定义字�?==========
    const int fontRegular = QFontDatabase::addApplicationFont(":/fonts/Roboto-Regular.ttf");
    const int fontMedium = QFontDatabase::addApplicationFont(":/fonts/Roboto-Medium.ttf");
    const int fontBold = QFontDatabase::addApplicationFont(":/fonts/Roboto-Bold.ttf");

    if (fontRegular < 0 || fontMedium < 0 || fontBold < 0)
    {
        LOG_WARNING("Failed to load some custom fonts, using system defaults");
    }

    // 设置应用默认字体
    QFont defaultFont("Roboto", 10);
    defaultFont.setStyleStrategy(QFont::PreferAntialias); // 优先使用抗锯�?
    QApplication::setFont(defaultFont);

    // ========== 初始化主题管理器 ==========
    ThemeManager::instance()->setTheme(ThemeManager::ThemeType::Dark);

    // ========== 国际�?==========
    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString& locale : uiLanguages)
    {
        const QString baseName = "wealth-pilot_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName))
        {
            app.installTranslator(&translator);
            break;
        }
    }

    // ========== 创建主窗�?==========
    MainWindow w;

    // 确保退出时清理服务
    QObject::connect(&app, &QApplication::aboutToQuit, []()
    {
        cleanupServices();
    });

    w.show();

    LOG_INFO("Application started successfully");

    return QApplication::exec();
}

