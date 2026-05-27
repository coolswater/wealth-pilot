/**
 * @file main.cpp
 * @brief 应用程序入口
 *
 * @details WealthPilot智能投资管理软件主入口
 * 负责：
 * - 初始化Qt应用框架
 * - 配置高DPI支持
 * - 加载字体和主题
 * - 初始化核心服务
 * - 启动应用主循环
 *
 * @note 程序启动流程
 * 1. 设置Qt应用属性（高DPI、事件压缩）
 * 2. 创建Qt应用实例
 * 3. 初始化配置管理器
 * 4. 初始化日志系统
 * 5. 加载自定义字体
 * 6. 初始化主题管理器
 * 7. 初始化网络管理器
 * 8. 初始化数据服务
 * 9. 初始化AI服务
 * 10. 创建并显示主窗口
 * 11. 进入主事件循环
 *
 * @author WealthPilot Team
 * @version 1.0.0
 * @date 2026
 */
#include "presentation/views/mainWindow/MainWindow.h"
#include "shared/utils/Logger.h"
#include "infrastructure/database/DatabaseManager.h"
#include "src/presentation/viewmodels/ViewModelRegistration.h"
#include "core/services/lifecycle/ServiceLifecycle.h"
#include "core/services/alert/AlertNotificationService.h"
#include <QApplication>
#include <QFontDatabase>
#include <QLocale>
#include <QTranslator>
#include <presentation/styles/ThemeManager.h>
#include <QQmlEngine>
#include <QtCharts/QChart>

#include "app/FeatureIntegration.h"

// 导入 Qt Charts QML 插件
#ifdef QT_CHARTS_LIB
#include <QtCharts/QtCharts>
#endif

/**
 * @brief 清理所有服务 - 使用 ServiceLifecycle 优雅关闭
 */
void cleanupServices()
{
    LOG_INFO("========================================");
    LOG_INFO("Cleaning up services...");

    // 使用 ServiceLifecycle 关闭所有服务（按依赖逆序）
    WealthPilot::ServiceLifecycle::instance()->shutdownAll();

    LOG_INFO("All services cleaned up");
    LOG_INFO("========================================");
}

/**
 * @brief 应用程序入口函数
 * @param argc 命令行参数数量
 * @param argv 命令行参数数组
 * @return int 程序退出码：0=成功，非0=错误
 */
int main(int argc, char* argv[])
{
    // 设置应用程序属性：压缩高频事件（如鼠标移动），提高性能
    QApplication::setAttribute(Qt::AA_CompressHighFrequencyEvents);

    // ========== 创建Qt应用实例 ==========
    const QApplication app(argc, argv);

    // ========== 设置应用元信息 ==========
    QApplication::setApplicationName("WealthPilot-财富领航AI助手");
    QApplication::setOrganizationName("Hexd");
    QApplication::setApplicationVersion("1.0.0");

    // 设置应用ICon
    QApplication::setWindowIcon(QIcon(":/images/app_icon.png"));

    // ========== 初始化日志系统 ==========
    // 日志保存到 D:\C++\wealth-pilot\logs 目录
    QString logPath = "D:/C++/wealth-pilot/logs/wealthpilot.log";
    Logger::instance()->init(logPath);
    LOG_INFO("========================================");
    LOG_INFO("WealthPilot-Wealth Navigator AI Assistant");
    LOG_INFO(QString("Version: %1").arg(QApplication::applicationVersion()));
    LOG_INFO(QString("Log file: %1").arg(logPath));
    LOG_INFO("========================================");

    // ========== 加载自定义字体 ==========
    const int fontRegular = QFontDatabase::addApplicationFont(":/fonts/Roboto-Regular.ttf");
    const int fontMedium = QFontDatabase::addApplicationFont(":/fonts/Roboto-Medium.ttf");
    const int fontBold = QFontDatabase::addApplicationFont(":/fonts/Roboto-Bold.ttf");

    if (fontRegular < 0 || fontMedium < 0 || fontBold < 0)
    {
        LOG_WARNING("Failed to load some custom fonts, using system defaults");
    }

    // 设置应用默认字体 - Segoe UI 为主，Roboto 为备用
    QFont defaultFont("Segoe UI", 10);
    defaultFont.setStyleStrategy(QFont::PreferAntialias); // 优先使用抗锯齿
    // 设置备用字体族
    QStringList fallbackFonts;
    fallbackFonts << "Roboto" << "Microsoft YaHei UI";
    defaultFont.setFamilies(fallbackFonts);
    QApplication::setFont(defaultFont);

    // ========== 初始化主题管理器 ==========
        ThemeManager::instance()->initialize();

        // ========== 注册服务到 ServiceLifecycle ==========
        auto* lifecycle = WealthPilot::ServiceLifecycle::instance();
    
        // 注册数据库服务（优先级最高，最先启动）
        lifecycle->registerService({
            QStringLiteral("DatabaseManager"),
            1,  // 最高优先级
            {},  // 无依赖
            []() -> bool {
                return DatabaseManager::instance()->initialize();
            },
            []() {
                DatabaseManager::instance()->shutdown();
            }
        });
    
        // 注册智能预警服务（依赖数据库）
        lifecycle->registerService({
            QStringLiteral("AlertNotificationService"),
            10,
            {QStringLiteral("DatabaseManager")},
            []() -> bool {
                // 预警服务初始化（配置从数据库加载）
                return true;
            },
            []() {
                // 预警服务清理
            }
        });
    
        // 注册主题服务
        lifecycle->registerService({
            QStringLiteral("ThemeManager"),
            5,
            {},
            []() -> bool {
                ThemeManager::instance()->initialize();
                return true;
            },
            []() {
                ThemeManager::instance()->clearCache();
            }
        });
    
        // 初始化所有服务
        if (!lifecycle->initializeAll()) {
            LOG_ERROR("Failed to initialize some services");
            // 继续运行，但记录错误
        }

    // ========== 国际化 ==========
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

    // ========== 创建主窗口 ==========
    MainWindow w;

    // 初始化所有功能
    WealthPilot::FeatureIntegration::instance()->initialize(&w);

    // 确保退出时清理服务
    QObject::connect(&app, &QApplication::aboutToQuit, []()
    {
        cleanupServices();
    });

    LOG_INFO("Showing MainWindow...");
    w.show();

    LOG_INFO("Application started successfully");

    return QApplication::exec();
}
