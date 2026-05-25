/**
 * @file CTPBrokerDialog.h
 * @brief CTP Broker Configuration Dialog
 *
 * @details Features:
 * - Display all available brokers
 * - Add/Edit/Delete brokers
 * - Set user credentials
 * - Switch current broker
 * - Test connection
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
#include "infrastructure/ctp/config/CTPConfigManager.h"

namespace CTP {

/**
 * @brief CTP Broker Configuration Dialog
 */
class CTPBrokerDialog : public QDialog {
    Q_OBJECT

public:
    explicit CTPBrokerDialog(QWidget* parent = nullptr);
    ~CTPBrokerDialog() override;

private slots:
    void onBrokerChanged(int index);
    void onAddBroker();
    void onEditBroker();
    void onDeleteBroker();
    void onTestConnection();
    void onSaveCredentials();
    void onAccept();

private:
    void setupUI();
    void loadBrokers();
    void updateBrokerList();
    void saveCurrentBroker();

    struct Impl;
    std::unique_ptr<Impl> d;
};

/**
 * @brief Add/Edit Broker Dialog
 */
class CTPBrokerEditDialog : public QDialog {
    Q_OBJECT

public:
    explicit CTPBrokerEditDialog(QWidget* parent = nullptr);
    explicit CTPBrokerEditDialog(const CTPBrokerConfig& config, QWidget* parent = nullptr);
    ~CTPBrokerEditDialog() override;

    /**
     * @brief Get configuration
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
