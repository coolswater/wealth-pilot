/**
 * @file QmlChartDemoPage.h
 * @brief QML 图表演示页面 - 展示混合架构能力
 *
 * @details 功能：
 * - 展示 QML K线图
 * - 展示 QML 分时图
 * - 数据切换演示
 * - 性能对比
 */

#ifndef QMLCHARTDEMOPAGE_H
#define QMLCHARTDEMOPAGE_H

#include "presentation/components/BasePage.h"
#include <QWidget>

class QmlKLineWidget;
class QComboBox;
class QPushButton;
class QLabel;

class QmlChartDemoPage : public WealthPilot::BasePage
{
    Q_OBJECT

public:
    explicit QmlChartDemoPage(QWidget* parent = nullptr);
    ~QmlChartDemoPage() override = default;

    QString pageId() const override { return "qml-chart-demo"; }
    QString pageName() const override { return QStringLiteral("QML图表演示"); }

private slots:
    void onChartTypeChanged(int index);
    void onLoadDemoData();
    void onGenerateRealtimeData();

private:
    void setupUI();
    void setupConnections();
    void loadDemoKLineData();
    void loadDemoTimeShareData();
    void generateRandomKLine(int count = 200);

    QmlKLineWidget* m_chartWidget = nullptr;
    QComboBox* m_chartTypeCombo = nullptr;
    QPushButton* m_loadDataBtn = nullptr;
    QPushButton* m_realtimeBtn = nullptr;
    QLabel* m_statusLabel = nullptr;

    bool m_realtimeEnabled = false;
};

#endif // QMLCHARTDEMOPAGE_H
