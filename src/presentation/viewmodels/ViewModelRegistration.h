/**
 * @file ViewModelRegistration.h
 * @brief ViewModel QML 类型注册
 * 
 * @details 注册所有 ViewModel 到 QML 引擎，使其可在 QML 中使用
 * 
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef VIEWMODELREGISTRATION_H
#define VIEWMODELREGISTRATION_H

#include <QQmlEngine>
#include <QtQml>

#include "ViewModelBase.h"
#include "TradingViewModel.h"
#include "OrderViewModel.h"
#include "BacktestViewModel.h"

namespace WealthPilot
{
    /**
 * @brief 注册所有 ViewModel 类型到 QML
 *
 * @details 在 main.cpp 中调用此函数：
 * @code
 * #include "presentation/viewmodels/ViewModelRegistration.h"
 *
 * int main(int argc, char *argv[]) {
 *     QGuiApplication app(argc, argv);
 *     QQmlApplicationEngine engine;
 *
 *     WealthPilot::registerViewModels(engine);
 *
 *     engine.load(QUrl("qrc:/main.qml"));
 *     return app.exec();
 * }
 * @endcode
 */
    inline void registerViewModels(QQmlEngine& engine)
    {
        // 注册 Command 类型
        qmlRegisterUncreatableType<Command>(
            "WealthPilot.ViewModels", 1, 0, "Command",
            "Command cannot be created in QML"
        );

        // 注册 CommandState 枚举
        qmlRegisterUncreatableType<ViewModelBase>(
            "WealthPilot.ViewModels", 1, 0, "ViewModelBase",
            "ViewModelBase cannot be created in QML"
        );

        // 注册 TradingViewModel
        qmlRegisterType<TradingViewModel>(
            "WealthPilot.ViewModels", 1, 0, "TradingViewModel"
        );

        // 注册 OrderViewModel
        qmlRegisterType<OrderViewModel>(
            "WealthPilot.ViewModels", 1, 0, "OrderViewModel"
        );

        // 注册 BacktestViewModel
        qmlRegisterType<BacktestViewModel>(
            "WealthPilot.ViewModels", 1, 0, "BacktestViewModel"
        );

        // 设置单例 ViewModel（如果需要）
        // engine.setObjectOwnership(viewModel, QQmlEngine::CppOwnership);

        qDebug() << "ViewModels registered to QML";
    }

    /**
 * @brief 注册样式常量到 QML
 */
    inline void registerStyleConstants(QQmlEngine& engine)
    {
        // 注册颜色常量
        QObject* styleColors = new QObject(&engine);

        styleColors->setProperty("bgPrimary", "#0A0E17");
        styleColors->setProperty("bgSecondary", "#111827");
        styleColors->setProperty("bgElevated", "#1A2332");
        styleColors->setProperty("bgSurface", "#1A2332");
        styleColors->setProperty("bgHover", "rgba(255, 255, 255, 0.05)");

        styleColors->setProperty("textPrimary", "#F3F4F6");
        styleColors->setProperty("textSecondary", "#9CA3AF");
        styleColors->setProperty("textTertiary", "#6B7280");
        styleColors->setProperty("textDisabled", "#4B5563");

        styleColors->setProperty("primary", "#58A6FF");
        styleColors->setProperty("primaryHover", "#79B8FF");
        styleColors->setProperty("primaryDark", "#0366D6");

        styleColors->setProperty("success", "#3FB950");
        styleColors->setProperty("danger", "#F85149");
        styleColors->setProperty("warning", "#F0883E");
        styleColors->setProperty("info", "#58A6FF");

        styleColors->setProperty("border", "#30363D");
        styleColors->setProperty("borderLight", "#484F58");

        // 注册为单例
        qmlRegisterSingletonInstance<QObject>(
            "WealthPilot.Styles", 1, 0, "StyleColors", styleColors
        );

        qDebug() << "Style constants registered to QML";
    }

    /**
 * @brief 完整初始化 QML 环境
 */
    inline void initializeQmlEnvironment(QQmlEngine& engine)
    {
        registerViewModels(engine);
        registerStyleConstants(engine);

        qDebug() << "QML environment initialized";
    }
} // namespace WealthPilot

#endif // VIEWMODELREGISTRATION_H
