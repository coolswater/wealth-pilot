/**
 * @file CTPBrokerDialog.h
 * @brief CTP服务商配置对话框
 *
 * @details 功能：
 * - 显示所有可用服务商
 * - 添加/编辑/删除服务商
 * - 设置用户凭证
 * - 切换当前服务商
 * - 测试连接
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#ifndef CTPBROKERDIALOG_H
#define CTPBROKERDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QTableWidget>
#include <QGroupBox>
#include <memory>
#include "../../core/CTPConfigManager.h"

namespace CTP {

/**
 * @brief CTP服务商配置对话框
 */
class CTPBrokerDialog : public QDialog {
    Q_OBJECT

public:
    explicit CTPBrokerDialog(QWidget* parent = nullptr);
    ~CTPBrokerDialog() override;

    /**
     * @brief 获取选中的服务商ID
     */
    QString selectedBrokerId() const;

signals:
    /**
     * @brief 服务商配置已更改
     */
    void brokerConfigChanged(const QString& brokerId);

    /**
     * @brief 请求切换服务商
     */
    void switchBrokerRequested(const QString& brokerId);

private slots:
    void onBrokerSelected(int index);
    void onAddBroker();
    void onEditBroker();
    void onDeleteBroker();
    void onTestConnection();
    void onSaveCredentials();
    void onSwitchBroker();
    void onResetToDefaults();

private:
    void setupUI();
    void loadBrokers();
    void updateBrokerDetails(const CTPBrokerConfig& config);
    void clearBrokerDetails();
    void saveCurrentBroker();

    struct Impl;
    std::unique_ptr<Impl> d;
};

/**
 * @brief 添加/编辑服务商对话框
 */
class CTPBrokerEditDialog : public QDialog {
    Q_OBJECT

public:
    explicit CTPBrokerEditDialog(QWidget* parent = nullptr);
    explicit CTPBrokerEditDialog(const CTPBrokerConfig& config, QWidget* parent = nullptr);
    ~CTPBrokerEditDialog() override;

    /**
     * @brief 获取配置
     */
    CTPBrokerConfig getBrokerConfig() const;

private slots:
    void onAddMarketFront();
    void onRemoveMarketFront();
    void onAddTradingFront();
    void onRemoveTradingFront();

private:
    void setupUI();
    void loadConfig(const CTPBrokerConfig& config);

    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace CTP

#endif // CTPBROKERDIALOG_H
