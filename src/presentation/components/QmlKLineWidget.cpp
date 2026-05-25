/**
 * @file QmlKLineWidget.cpp
 * @brief QML K线图容器组件实现
 */

#include "QmlKLineWidget.h"
#include "presentation/qml/QmlDataBridge.h"
#include "presentation/styles/ThemeManager.h"
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QVBoxLayout>
#include <QResizeEvent>
#include <QShowEvent>
#include <QDebug>

QmlKLineWidget::QmlKLineWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    setupConnections();
}

QmlKLineWidget::~QmlKLineWidget()
{
}

void QmlKLineWidget::setupUI()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // 创建 QQuickWidget
    m_quickWidget = new QQuickWidget(this);
    m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_quickWidget->setAttribute(Qt::WA_AlwaysStackOnTop);  // 确保QML在最上层
    m_quickWidget->setClearColor(Qt::transparent);         // 透明背景
    
    // 添加 Qt Charts QML 插件导入路径
    m_quickWidget->engine()->addImportPath("qrc:/qt-project.org/imports");
    m_quickWidget->engine()->addPluginPath("C:/Qt/6.10.2/mingw_64/qml");

    // 获取数据模型 - 添加空指针检查
    auto* bridge = QmlDataBridge::instance();
    if (bridge) {
        m_klineModel = bridge->klineModel();
        m_timeShareModel = bridge->timeShareModel();
    }

    // 设置 QML 上下文属性 - 仅在模型有效时设置
    QQmlContext* context = m_quickWidget->rootContext();
    if (context) {
        if (m_klineModel) {
            context->setContextProperty("klineModel", m_klineModel);
        }
        if (m_timeShareModel) {
            context->setContextProperty("timeShareModel", m_timeShareModel);
        }
    }

    layout->addWidget(m_quickWidget);

    // 延迟加载 QML（等组件显示后再加载）
    m_qmlLoaded = false;
}

void QmlKLineWidget::setupConnections()
{
    // 连接主题变化
    connect(ThemeManager::instance(), &ThemeManager::themeChanged,
            this, [this](ThemeType type) {
                applyTheme(type == ThemeType::Dark);
            });

    // QML 加载状态
    connect(m_quickWidget, &QQuickWidget::statusChanged,
            this, [this](QQuickWidget::Status status) {
                if (status == QQuickWidget::Ready) {
                    m_qmlLoaded = true;
                    emit chartLoaded();
                    applyTheme(m_isDarkTheme);
                } else if (status == QQuickWidget::Error) {
                    qWarning() << "QML load error:" << m_quickWidget->errors();
                }
            });
}

void QmlKLineWidget::loadQmlSource()
{
    QString qmlFile;
    if (m_chartType == ChartType::KLine) {
        qmlFile = "qrc:/qml/charts/KLineChart.qml";
    } else {
        qmlFile = "qrc:/qml/charts/TimeShareChart.qml";
    }
    m_quickWidget->setSource(QUrl(qmlFile));
    
    // 加载后绑定模型到 QML 的 model 属性
    if (m_chartType == ChartType::KLine && m_klineModel) {
        updateQmlProperty("model", QVariant::fromValue(m_klineModel));
    } else if (m_chartType == ChartType::TimeShare && m_timeShareModel) {
        updateQmlProperty("model", QVariant::fromValue(m_timeShareModel));
    }
}

void QmlKLineWidget::setKLineData(const QVector<KLineData>& data)
{
    // 添加空指针检查
    if (!m_klineModel) {
        qWarning() << "QmlKLineWidget::setKLineData: m_klineModel is null";
        return;
    }
    
    m_klineModel->setData(data);
    
    // 确保 QML 组件已加载并绑定模型
    if (m_qmlLoaded) {
        updateQmlProperty("model", QVariant::fromValue(m_klineModel));
        // 触发数据更新
        if (auto* root = m_quickWidget->rootObject()) {
            QMetaObject::invokeMethod(root, "updateData");
        }
    }
}

void QmlKLineWidget::updateLastKLine(const KLineData& data)
{
    if (m_klineModel) {
        m_klineModel->updateLastData(data);
    }
}

void QmlKLineWidget::setTimeShareData(const QVector<TimeShareData>& data, double basePrice)
{
    if (m_timeShareModel) {
        m_timeShareModel->setBasePrice(basePrice);
        m_timeShareModel->setData(data);
    }
}

void QmlKLineWidget::appendTimeShareData(const TimeShareData& data)
{
    if (m_timeShareModel) {
        m_timeShareModel->appendData(data);
    }
}

void QmlKLineWidget::clearData()
{
    if (m_klineModel) {
        m_klineModel->clear();
    }
    if (m_timeShareModel) {
        m_timeShareModel->clear();
    }
}

void QmlKLineWidget::setChartType(ChartType type)
{
    if (m_chartType != type) {
        m_chartType = type;
        m_qmlLoaded = false;
        loadQmlSource();
    }
}

void QmlKLineWidget::setVisibleCount(int count)
{
    updateQmlProperty("visibleCount", count);
}

void QmlKLineWidget::scrollTo(int index)
{
    updateQmlProperty("startIndex", index);
}

void QmlKLineWidget::zoomToFit()
{
    if (auto* root = m_quickWidget->rootObject()) {
        QMetaObject::invokeMethod(root, "updateData");
    }
}

void QmlKLineWidget::applyTheme(bool isDark)
{
    m_isDarkTheme = isDark;

    if (!m_qmlLoaded) return;

    if (isDark) {
        updateQmlProperty("upColor", "#EF4444");      // 红涨
        updateQmlProperty("downColor", "#10B981");    // 绿跌
        updateQmlProperty("gridColor", "#2D3748");
        updateQmlProperty("textColor", "#9CA3AF");
    } else {
        updateQmlProperty("upColor", "#E8463A");     // 红涨（浅色主题）
        updateQmlProperty("downColor", "#14B143");   // 绿跌（浅色主题）
        updateQmlProperty("gridColor", "#D0D7DE");
        updateQmlProperty("textColor", "#24292F");
    }
}

void QmlKLineWidget::updateQmlProperty(const char* name, const QVariant& value)
{
    if (auto* root = m_quickWidget->rootObject()) {
        root->setProperty(name, value);
    }
}

void QmlKLineWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    // QQuickWidget 会自动处理大小变化
}

void QmlKLineWidget::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);

    // 首次显示时加载 QML
    if (!m_qmlLoaded) {
        loadQmlSource();
    }
}
