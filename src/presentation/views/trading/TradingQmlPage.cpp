/**
 * @file TradingQmlPage.cpp
 * @brief 交易 QML 页面实现 - MVVM 架构示例
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "TradingQmlPage.h"
#include "presentation/viewmodels/ViewModelRegistration.h"
#include "shared/utils/Logger.h"

#include <QVBoxLayout>
#include <QQuickWidget>
#include <QQmlContext>
#include <QUrl>

namespace WealthPilot
{
    TradingQmlPage::TradingQmlPage(QWidget* parent)
        : BasePage(parent)
          , m_qmlWidget(nullptr)
          , m_qmlEngine(nullptr)
          , m_initialized(false)
    {
        setupUI();
        LOG_DEBUG("TradingQmlPage created (MVVM)");
    }

    TradingQmlPage::~TradingQmlPage()
    {
        LOG_DEBUG("TradingQmlPage destroyed");
    }

    void TradingQmlPage::initializePage()
    {
        if (m_initialized)
        {
            return;
        }

        initializeQml();
        m_initialized = true;

        LOG_INFO("TradingQmlPage initialized");
    }

    void TradingQmlPage::setupUI()
    {
        auto* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);

        // 创建 QML 视图控件
        m_qmlWidget = new QQuickWidget(this);
        m_qmlWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);

        mainLayout->addWidget(m_qmlWidget);
    }

    void TradingQmlPage::initializeQml()
    {
        // 获取或创建 QML 引擎
        m_qmlEngine = m_qmlWidget->engine();

        // 注册 ViewModel 到 QML
        registerViewModels(*m_qmlEngine);
        registerStyleConstants(*m_qmlEngine);

        // 加载 QML 文件
        QUrl qmlUrl = QUrl("qrc:/qml/TradingPanel.qml");
        m_qmlWidget->setSource(qmlUrl);

        // 检查加载状态
        if (m_qmlWidget->status() == QQuickWidget::Error)
        {
            LOG_ERROR("Failed to load QML: " + qmlUrl.toString());
            for (const auto& error : m_qmlWidget->errors())
            {
                LOG_ERROR(error.toString());
            }
        }
        else
        {
            LOG_INFO("QML loaded successfully: " + qmlUrl.toString());
        }
    }

    void TradingQmlPage::setupConnections()
    {
        // 可以在这里添加 QML 与 C++ 的交互
    }
} // namespace WealthPilot