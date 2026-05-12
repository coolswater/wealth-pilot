/**
 * @file TradingQmlPage.h
 * @brief 交易 QML 页面 - MVVM 架构示例
 *
 * @details 展示如何在 Widget 应用中嵌入 QML 视图，
 *          使用 ViewModel 进行数据绑定
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef TRADINGQMLPAGE_H
#define TRADINGQMLPAGE_H

#include "ui/components/BasePage.h"
#include <QQuickWidget>
#include <QQmlEngine>

namespace WealthPilot
{
    /**
 * @brief 交易 QML 页面
 *
 * @details 使用 QML + ViewModel 的 MVVM 架构示例：
 * - QML 视图负责 UI 展示
 * - ViewModel 负责业务逻辑和数据管理
 * - 通过数据绑定实现解耦
 */
    class TradingQmlPage : public BasePage
    {
        Q_OBJECT

    public:
        /**
     * @brief 构造函数
     * @param parent 父窗口
     */
        explicit TradingQmlPage(QWidget* parent = nullptr);

        /**
     * @brief 析构函数
     */
        ~TradingQmlPage() override;

        /**
     * @brief 获取页面ID
     */
        QString pageId() const override { return QStringLiteral("trading-qml"); }

        /**
     * @brief 获取页面名称
     */
        QString pageName() const override { return QStringLiteral("交易面板 (MVVM)"); }

        /**
     * @brief 初始化页面
     */
        void initializePage() override;

    private:
        /**
     * @brief 设置 UI
     */
        void setupUI();

        /**
     * @brief 初始化 QML 环境
     */
        void initializeQml();

        /**
     * @brief 设置信号连接
     */
        void setupConnections();

    private:
        QQuickWidget* m_qmlWidget; ///< QML 视图控件
        QQmlEngine* m_qmlEngine; ///< QML 引擎
        bool m_initialized; ///< 是否已初始化
    };
} // namespace WealthPilot

#endif // TRADINGQMLPAGE_H