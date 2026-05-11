/**
 * @file FeatureIntegration.h
 * @brief 新功能集成入口
 *
 * @details 提供所有新开发功能的统一入口：
 * - 短期规划功能
 * - 中期规划功能
 * - 长期规划功能
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef FEATUREINTEGRATION_H
#define FEATUREINTEGRATION_H

#include <QObject>

// 前向声明
class QMainWindow;

namespace WealthPilot {

class FeatureIntegration : public QObject {
    Q_OBJECT

public:
    static FeatureIntegration* instance();

    void initialize(QMainWindow* mainWindow);
    void initializeShortTermFeatures();
    void initializeMidTermFeatures();
    void initializeLongTermFeatures();
    void registerDefaultShortcuts();
    void startPerformanceMonitoring();
    QString getStatusReport() const;

private:
    explicit FeatureIntegration(QObject* parent = nullptr);
    ~FeatureIntegration() override = default;

    QMainWindow* m_mainWindow = nullptr;
    bool m_initialized = false;
};

} // namespace WealthPilot

#endif // FEATUREINTEGRATION_H
