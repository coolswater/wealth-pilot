/**
 * @file CTPBrokerDialog.cpp
 * @brief CTP Broker Configuration Dialog Implementation
 */

#include "CTPBrokerDialog.h"
#include "../../ctp/config/CTPConfigManager.h"
#include "../../utils/Logger.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QTableWidget>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QDialogButtonBox>

namespace CTP {

// ========== CTPBrokerDialog::Impl ==========

struct CTPBrokerDialog::Impl {
    QComboBox* brokerCombo = nullptr;
    QPushButton* addButton = nullptr;
    QPushButton* editButton = nullptr;
    QPushButton* deleteButton = nullptr;
    QPushButton* resetButton = nullptr;
    
    QLabel* nameLabel = nullptr;
    QLabel* brokerIdLabel = nullptr;
    QLabel* descriptionLabel = nullptr;
    QLabel* websiteLabel = nullptr;
    QLabel* envLabel = nullptr;
    QTableWidget* marketFrontTable = nullptr;
    QTableWidget* tradingFrontTable = nullptr;
    
    QLineEdit* userIdEdit = nullptr;
    QLineEdit* passwordEdit = nullptr;
    QCheckBox* rememberCheck = nullptr;
    QPushButton* saveCredentialsButton = nullptr;
    
    QPushButton* testButton = nullptr;
    QPushButton* switchButton = nullptr;
    QPushButton* closeButton = nullptr;
    
    QString currentBrokerId;
};

// ========== CTPBrokerDialog ==========

CTPBrokerDialog::CTPBrokerDialog(QWidget* parent)
    : QDialog(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
    loadBrokers();
}

CTPBrokerDialog::~CTPBrokerDialog() = default;

void CTPBrokerDialog::setupUI()
{
    setWindowTitle("CTP Broker Configuration");
    setMinimumSize(700, 600);
    
    auto* mainLayout = new QVBoxLayout(this);
    
    // Broker selection
    auto* brokerGroup = new QGroupBox("Broker Selection", this);
    auto* brokerLayout = new QHBoxLayout(brokerGroup);
    
    d->brokerCombo = new QComboBox(this);
    d->brokerCombo->setMinimumWidth(300);
    QObject::connect(d->brokerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CTPBrokerDialog::onBrokerChanged);
    
    d->addButton = new QPushButton("Add", this);
    d->editButton = new QPushButton("Edit", this);
    d->deleteButton = new QPushButton("Delete", this);
    d->resetButton = new QPushButton("Reset Defaults", this);
    
    QObject::connect(d->addButton, &QPushButton::clicked, this, &CTPBrokerDialog::onAddBroker);
    QObject::connect(d->editButton, &QPushButton::clicked, this, &CTPBrokerDialog::onEditBroker);
    QObject::connect(d->deleteButton, &QPushButton::clicked, this, &CTPBrokerDialog::onDeleteBroker);
    QObject::connect(d->resetButton, &QPushButton::clicked, this, &CTPBrokerDialog::onTestConnection);
    
    brokerLayout->addWidget(new QLabel("Broker:", this));
    brokerLayout->addWidget(d->brokerCombo);
    brokerLayout->addWidget(d->addButton);
    brokerLayout->addWidget(d->editButton);
    brokerLayout->addWidget(d->deleteButton);
    brokerLayout->addWidget(d->resetButton);
    
    mainLayout->addWidget(brokerGroup);
    
    // Details
    auto* detailGroup = new QGroupBox("Broker Details", this);
    auto* detailLayout = new QGridLayout(detailGroup);
    
    d->nameLabel = new QLabel(this);
    d->brokerIdLabel = new QLabel(this);
    d->descriptionLabel = new QLabel(this);
    d->websiteLabel = new QLabel(this);
    d->envLabel = new QLabel(this);
    
    detailLayout->addWidget(new QLabel("Name:", this), 0, 0);
    detailLayout->addWidget(d->nameLabel, 0, 1);
    detailLayout->addWidget(new QLabel("Broker ID:", this), 1, 0);
    detailLayout->addWidget(d->brokerIdLabel, 1, 1);
    detailLayout->addWidget(new QLabel("Description:", this), 2, 0);
    detailLayout->addWidget(d->descriptionLabel, 2, 1);
    detailLayout->addWidget(new QLabel("Website:", this), 3, 0);
    detailLayout->addWidget(d->websiteLabel, 3, 1);
    detailLayout->addWidget(new QLabel("Environment:", this), 4, 0);
    detailLayout->addWidget(d->envLabel, 4, 1);
    
    mainLayout->addWidget(detailGroup);
    
    // Front addresses
    auto* frontGroup = new QGroupBox("Front Addresses", this);
    auto* frontLayout = new QVBoxLayout(frontGroup);
    
    d->marketFrontTable = new QTableWidget(this);
    d->marketFrontTable->setColumnCount(1);
    d->marketFrontTable->setHorizontalHeaderLabels(QStringList() << "Market Front");
    d->marketFrontTable->horizontalHeader()->setStretchLastSection(true);
    d->marketFrontTable->setMaximumHeight(100);
    
    d->tradingFrontTable = new QTableWidget(this);
    d->tradingFrontTable->setColumnCount(1);
    d->tradingFrontTable->setHorizontalHeaderLabels(QStringList() << "Trading Front");
    d->tradingFrontTable->horizontalHeader()->setStretchLastSection(true);
    d->tradingFrontTable->setMaximumHeight(100);
    
    frontLayout->addWidget(new QLabel("Market Fronts:", this));
    frontLayout->addWidget(d->marketFrontTable);
    frontLayout->addWidget(new QLabel("Trading Fronts:", this));
    frontLayout->addWidget(d->tradingFrontTable);
    
    mainLayout->addWidget(frontGroup);
    
    // User credentials
    auto* credGroup = new QGroupBox("User Credentials", this);
    auto* credLayout = new QGridLayout(credGroup);
    
    d->userIdEdit = new QLineEdit(this);
    d->userIdEdit->setPlaceholderText("User ID");
    d->passwordEdit = new QLineEdit(this);
    d->passwordEdit->setEchoMode(QLineEdit::Password);
    d->passwordEdit->setPlaceholderText("Password");
    d->rememberCheck = new QCheckBox("Remember Password", this);
    d->saveCredentialsButton = new QPushButton("Save Credentials", this);
    
    QObject::connect(d->saveCredentialsButton, &QPushButton::clicked, this, &CTPBrokerDialog::onSaveCredentials);
    
    credLayout->addWidget(new QLabel("User ID:", this), 0, 0);
    credLayout->addWidget(d->userIdEdit, 0, 1);
    credLayout->addWidget(new QLabel("Password:", this), 1, 0);
    credLayout->addWidget(d->passwordEdit, 1, 1);
    credLayout->addWidget(d->rememberCheck, 2, 0, 1, 2);
    credLayout->addWidget(d->saveCredentialsButton, 3, 0, 1, 2);
    
    mainLayout->addWidget(credGroup);
    
    // Buttons
    auto* buttonLayout = new QHBoxLayout();
    d->testButton = new QPushButton("Test Connection", this);
    d->switchButton = new QPushButton("Switch Broker", this);
    d->closeButton = new QPushButton("Close", this);
    
    QObject::connect(d->testButton, &QPushButton::clicked, this, &CTPBrokerDialog::onTestConnection);
    QObject::connect(d->switchButton, &QPushButton::clicked, this, &CTPBrokerDialog::onAccept);
    QObject::connect(d->closeButton, &QPushButton::clicked, this, &QDialog::reject);
    
    buttonLayout->addWidget(d->testButton);
    buttonLayout->addWidget(d->switchButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(d->closeButton);
    
    mainLayout->addLayout(buttonLayout);
}

void CTPBrokerDialog::loadBrokers()
{
    auto* configManager = CTP::CTPConfigManager::instance();
    auto brokers = configManager->getAllBrokers();
    
    d->brokerCombo->clear();
    for (const auto& broker : brokers) {
        QString displayText = QString("%1 (%2)").arg(broker.name, broker.brokerId);
        d->brokerCombo->addItem(displayText, broker.id);
    }
    
    if (!brokers.isEmpty()) {
        onBrokerChanged(0);
    }
}

void CTPBrokerDialog::onBrokerChanged(int index)
{
    if (index < 0) return;
    
    auto* configManager = CTP::CTPConfigManager::instance();
    QString brokerId = d->brokerCombo->itemData(index).toString();
    auto brokerOpt = configManager->getBroker(brokerId);
    
    if (!brokerOpt.has_value()) return;
    
    auto broker = brokerOpt.value();
    d->currentBrokerId = brokerId;
    d->nameLabel->setText(broker.name);
    d->brokerIdLabel->setText(broker.brokerId);
    d->descriptionLabel->setText(broker.description);
    d->websiteLabel->setText(broker.website);
    d->envLabel->setText(broker.isSimulation ? "Simulation" : "Production");
    
    // Market fronts
    d->marketFrontTable->setRowCount(broker.marketFronts.size());
    for (int i = 0; i < broker.marketFronts.size(); i++) {
        d->marketFrontTable->setItem(i, 0, new QTableWidgetItem(broker.marketFronts[i]));
    }
    
    // Trading fronts
    d->tradingFrontTable->setRowCount(broker.tradingFronts.size());
    for (int i = 0; i < broker.tradingFronts.size(); i++) {
        d->tradingFrontTable->setItem(i, 0, new QTableWidgetItem(broker.tradingFronts[i]));
    }
    
    // Load credentials
    d->userIdEdit->setText(configManager->getUserId(brokerId));
    d->passwordEdit->setText(configManager->getPassword(brokerId));
}

void CTPBrokerDialog::onAddBroker()
{
    CTPBrokerEditDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        auto config = dialog.getBrokerConfig();
        CTP::CTPConfigManager::instance()->setBroker(config);
        loadBrokers();
    }
}

void CTPBrokerDialog::onEditBroker()
{
    if (d->currentBrokerId.isEmpty()) return;
    
    auto* configManager = CTP::CTPConfigManager::instance();
    auto brokerOpt = configManager->getBroker(d->currentBrokerId);
    
    if (!brokerOpt.has_value()) return;
    
    CTPBrokerEditDialog dialog(brokerOpt.value(), this);
    if (dialog.exec() == QDialog::Accepted) {
        auto config = dialog.getBrokerConfig();
        configManager->setBroker(config);
        loadBrokers();
    }
}

void CTPBrokerDialog::onDeleteBroker()
{
    if (d->currentBrokerId.isEmpty()) return;
    
    auto reply = QMessageBox::question(this, "Confirm Delete",
            "Are you sure you want to delete this broker?",
            QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        CTP::CTPConfigManager::instance()->removeBroker(d->currentBrokerId);
        loadBrokers();
    }
}

void CTPBrokerDialog::onTestConnection()
{
    QMessageBox::information(this, "Test Connection", "Connection test not implemented yet.");
}

void CTPBrokerDialog::onSaveCredentials()
{
    if (d->currentBrokerId.isEmpty()) return;
    
    QString userId = d->userIdEdit->text();
    QString password = d->passwordEdit->text();
    
    CTP::CTPConfigManager::instance()->setUserCredentials(d->currentBrokerId, userId, password);
    QMessageBox::information(this, "Success", "Credentials saved successfully.");
}

void CTPBrokerDialog::onAccept()
{
    onSaveCredentials();
    accept();
}

// ========== CTPBrokerEditDialog::Impl ==========

struct CTPBrokerEditDialog::Impl {
    QLineEdit* idEdit = nullptr;
    QLineEdit* nameEdit = nullptr;
    QLineEdit* brokerIdEdit = nullptr;
    QLineEdit* descriptionEdit = nullptr;
    QLineEdit* websiteEdit = nullptr;
    QCheckBox* simulationCheck = nullptr;
    QTableWidget* marketFrontTable = nullptr;
    QTableWidget* tradingFrontTable = nullptr;
    QPushButton* addMarketButton = nullptr;
    QPushButton* removeMarketButton = nullptr;
    QPushButton* addTradingButton = nullptr;
    QPushButton* removeTradingButton = nullptr;
    bool isEditMode = false;
};

CTPBrokerEditDialog::CTPBrokerEditDialog(QWidget* parent)
    : QDialog(parent)
    , d(std::make_unique<Impl>())
{
    d->isEditMode = false;
    setupUI();
}

CTPBrokerEditDialog::CTPBrokerEditDialog(const CTPBrokerConfig& config, QWidget* parent)
    : QDialog(parent)
    , d(std::make_unique<Impl>())
{
    d->isEditMode = true;
    setupUI();
    loadConfig(config);
}

CTPBrokerEditDialog::~CTPBrokerEditDialog() = default;

void CTPBrokerEditDialog::setupUI()
{
    setWindowTitle(d->isEditMode ? "Edit Broker" : "Add Broker");
    setMinimumSize(500, 400);
    
    auto* mainLayout = new QVBoxLayout(this);
    
    // Basic info
    auto* infoGroup = new QGroupBox("Basic Info", this);
    auto* infoLayout = new QGridLayout(infoGroup);
    
    d->idEdit = new QLineEdit(this);
    d->nameEdit = new QLineEdit(this);
    d->brokerIdEdit = new QLineEdit(this);
    d->descriptionEdit = new QLineEdit(this);
    d->websiteEdit = new QLineEdit(this);
    d->simulationCheck = new QCheckBox("Simulation", this);
    
    infoLayout->addWidget(new QLabel("ID:", this), 0, 0);
    infoLayout->addWidget(d->idEdit, 0, 1);
    infoLayout->addWidget(new QLabel("Name:", this), 1, 0);
    infoLayout->addWidget(d->nameEdit, 1, 1);
    infoLayout->addWidget(new QLabel("Broker ID:", this), 2, 0);
    infoLayout->addWidget(d->brokerIdEdit, 2, 1);
    infoLayout->addWidget(new QLabel("Description:", this), 3, 0);
    infoLayout->addWidget(d->descriptionEdit, 3, 1);
    infoLayout->addWidget(new QLabel("Website:", this), 4, 0);
    infoLayout->addWidget(d->websiteEdit, 4, 1);
    infoLayout->addWidget(d->simulationCheck, 5, 0, 1, 2);
    
    mainLayout->addWidget(infoGroup);
    
    // Front addresses
    auto* frontGroup = new QGroupBox("Front Addresses", this);
    auto* frontLayout = new QVBoxLayout(frontGroup);
    
    // Market fronts
    auto* marketLayout = new QHBoxLayout();
    d->marketFrontTable = new QTableWidget(this);
    d->marketFrontTable->setColumnCount(1);
    d->marketFrontTable->setHorizontalHeaderLabels(QStringList() << "Market Front");
    d->marketFrontTable->horizontalHeader()->setStretchLastSection(true);
    d->addMarketButton = new QPushButton("Add", this);
    d->removeMarketButton = new QPushButton("Remove", this);
    
    QObject::connect(d->addMarketButton, &QPushButton::clicked, this, &CTPBrokerEditDialog::onAddMarketFront);
    QObject::connect(d->removeMarketButton, &QPushButton::clicked, this, &CTPBrokerEditDialog::onRemoveMarketFront);
    
    marketLayout->addWidget(d->marketFrontTable);
    marketLayout->addWidget(d->addMarketButton);
    marketLayout->addWidget(d->removeMarketButton);
    
    // Trading fronts
    auto* tradingLayout = new QHBoxLayout();
    d->tradingFrontTable = new QTableWidget(this);
    d->tradingFrontTable->setColumnCount(1);
    d->tradingFrontTable->setHorizontalHeaderLabels(QStringList() << "Trading Front");
    d->tradingFrontTable->horizontalHeader()->setStretchLastSection(true);
    d->addTradingButton = new QPushButton("Add", this);
    d->removeTradingButton = new QPushButton("Remove", this);
    
    QObject::connect(d->addTradingButton, &QPushButton::clicked, this, &CTPBrokerEditDialog::onAddTradingFront);
    QObject::connect(d->removeTradingButton, &QPushButton::clicked, this, &CTPBrokerEditDialog::onRemoveTradingFront);
    
    tradingLayout->addWidget(d->tradingFrontTable);
    tradingLayout->addWidget(d->addTradingButton);
    tradingLayout->addWidget(d->removeTradingButton);
    
    frontLayout->addLayout(marketLayout);
    frontLayout->addLayout(tradingLayout);
    
    mainLayout->addWidget(frontGroup);
    
    // Buttons
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    mainLayout->addWidget(buttonBox);
}

void CTPBrokerEditDialog::loadConfig(const CTPBrokerConfig& config)
{
    d->idEdit->setText(config.id);
    d->nameEdit->setText(config.name);
    d->brokerIdEdit->setText(config.brokerId);
    d->descriptionEdit->setText(config.description);
    d->websiteEdit->setText(config.website);
    d->simulationCheck->setChecked(config.isSimulation);
    
    d->marketFrontTable->setRowCount(config.marketFronts.size());
    for (int i = 0; i < config.marketFronts.size(); i++) {
        d->marketFrontTable->setItem(i, 0, new QTableWidgetItem(config.marketFronts[i]));
    }
    
    d->tradingFrontTable->setRowCount(config.tradingFronts.size());
    for (int i = 0; i < config.tradingFronts.size(); i++) {
        d->tradingFrontTable->setItem(i, 0, new QTableWidgetItem(config.tradingFronts[i]));
    }
}

CTPBrokerConfig CTPBrokerEditDialog::getBrokerConfig() const
{
    CTPBrokerConfig config;
    config.id = d->idEdit->text();
    config.name = d->nameEdit->text();
    config.brokerId = d->brokerIdEdit->text();
    config.description = d->descriptionEdit->text();
    config.website = d->websiteEdit->text();
    config.isSimulation = d->simulationCheck->isChecked();
    
    for (int i = 0; i < d->marketFrontTable->rowCount(); i++) {
        auto* item = d->marketFrontTable->item(i, 0);
        if (item) {
            config.marketFronts.append(item->text());
        }
    }
    
    for (int i = 0; i < d->tradingFrontTable->rowCount(); i++) {
        auto* item = d->tradingFrontTable->item(i, 0);
        if (item) {
            config.tradingFronts.append(item->text());
        }
    }
    
    return config;
}

void CTPBrokerEditDialog::onAddMarketFront()
{
    int row = d->marketFrontTable->rowCount();
    d->marketFrontTable->insertRow(row);
    d->marketFrontTable->setItem(row, 0, new QTableWidgetItem("tcp://"));
    d->marketFrontTable->editItem(d->marketFrontTable->item(row, 0));
}

void CTPBrokerEditDialog::onRemoveMarketFront()
{
    int row = d->marketFrontTable->currentRow();
    if (row >= 0) {
        d->marketFrontTable->removeRow(row);
    }
}

void CTPBrokerEditDialog::onAddTradingFront()
{
    int row = d->tradingFrontTable->rowCount();
    d->tradingFrontTable->insertRow(row);
    d->tradingFrontTable->setItem(row, 0, new QTableWidgetItem("tcp://"));
    d->tradingFrontTable->editItem(d->tradingFrontTable->item(row, 0));
}

void CTPBrokerEditDialog::onRemoveTradingFront()
{
    int row = d->tradingFrontTable->currentRow();
    if (row >= 0) {
        d->tradingFrontTable->removeRow(row);
    }
}

} // namespace CTP
