/**
 * @file CTPBrokerDialog.cpp
 * @brief CTP服务商配置对话框实现
 */

#include "CTPBrokerDialog.h"
#include "../../core/CTPConfigManager.h"
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
    // 服务商列表
    QComboBox* brokerCombo = nullptr;
    QPushButton* addButton = nullptr;
    QPushButton* editButton = nullptr;
    QPushButton* deleteButton = nullptr;
    QPushButton* resetButton = nullptr;
    
    // 详情显示
    QLabel* nameLabel = nullptr;
    QLabel* brokerIdLabel = nullptr;
    QLabel* descriptionLabel = nullptr;
    QLabel* websiteLabel = nullptr;
    QLabel* envLabel = nullptr;
    QTableWidget* marketFrontTable = nullptr;
    QTableWidget* tradingFrontTable = nullptr;
    
    // 用户凭证
    QLineEdit* userIdEdit = nullptr;
    QLineEdit* passwordEdit = nullptr;
    QCheckBox* rememberCheck = nullptr;
    QPushButton* saveCredentialsButton = nullptr;
    
    // 操作按钮
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
    setWindowTitle("CTP 服务商配置");
    setMinimumSize(700, 600);
    
    auto* mainLayout = new QVBoxLayout(this);
    
    // 服务商选择区域
    auto* brokerGroup = new QGroupBox("服务商选择", this);
    auto* brokerLayout = new QHBoxLayout(brokerGroup);
    
    d->brokerCombo = new QComboBox(this);
    d->brokerCombo->setMinimumWidth(300);
    connect(d->brokerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CTPBrokerDialog::onBrokerSelected);
    
    d->addButton = new QPushButton("添加", this);
    d->editButton = new QPushButton("编辑", this);
    d->deleteButton = new QPushButton("删除", this);
    d->resetButton = new QPushButton("重置默认", this);
    
    connect(d->addButton, &QPushButton::clicked, this, &CTPBrokerDialog::onAddBroker);
    connect(d->editButton, &QPushButton::clicked, this, &CTPBrokerDialog::onEditBroker);
    connect(d->deleteButton, &QPushButton::clicked, this, &CTPBrokerDialog::onDeleteBroker);
    connect(d->resetButton, &QPushButton::clicked, this, &CTPBrokerDialog::onResetToDefaults);
    
    brokerLayout->addWidget(new QLabel("服务商:", this));
    brokerLayout->addWidget(d->brokerCombo);
    brokerLayout->addWidget(d->addButton);
    brokerLayout->addWidget(d->editButton);
    brokerLayout->addWidget(d->deleteButton);
    brokerLayout->addWidget(d->resetButton);
    brokerLayout->addStretch();
    
    mainLayout->addWidget(brokerGroup);
    
    // 服务商详情区域
    auto* detailGroup = new QGroupBox("服务商详情", this);
    auto* detailLayout = new QGridLayout(detailGroup);
    
    int row = 0;
    detailLayout->addWidget(new QLabel("名称:", this), row, 0);
    d->nameLabel = new QLabel(this);
    detailLayout->addWidget(d->nameLabel, row, 1);
    
    row++;
    detailLayout->addWidget(new QLabel("经纪商代码:", this), row, 0);
    d->brokerIdLabel = new QLabel(this);
    detailLayout->addWidget(d->brokerIdLabel, row, 1);
    
    row++;
    detailLayout->addWidget(new QLabel("描述:", this), row, 0);
    d->descriptionLabel = new QLabel(this);
    d->descriptionLabel->setWordWrap(true);
    detailLayout->addWidget(d->descriptionLabel, row, 1);
    
    row++;
    detailLayout->addWidget(new QLabel("官网:", this), row, 0);
    d->websiteLabel = new QLabel(this);
    d->websiteLabel->setOpenExternalLinks(true);
    detailLayout->addWidget(d->websiteLabel, row, 1);
    
    row++;
    detailLayout->addWidget(new QLabel("环境:", this), row, 0);
    d->envLabel = new QLabel(this);
    detailLayout->addWidget(d->envLabel, row, 1);
    
    mainLayout->addWidget(detailGroup);
    
    // 前置地址区域
    auto* frontGroup = new QGroupBox("前置地址", this);
    auto* frontLayout = new QHBoxLayout(frontGroup);
    
    // 行情前置
    auto* marketLayout = new QVBoxLayout();
    marketLayout->addWidget(new QLabel("行情前置:", this));
    d->marketFrontTable = new QTableWidget(this);
    d->marketFrontTable->setColumnCount(1);
    d->marketFrontTable->setHorizontalHeaderLabels({"地址"});
    d->marketFrontTable->horizontalHeader()->setStretchLastSection(true);
    d->marketFrontTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    d->marketFrontTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    marketLayout->addWidget(d->marketFrontTable);
    
    // 交易前置
    auto* tradingLayout = new QVBoxLayout();
    tradingLayout->addWidget(new QLabel("交易前置:", this));
    d->tradingFrontTable = new QTableWidget(this);
    d->tradingFrontTable->setColumnCount(1);
    d->tradingFrontTable->setHorizontalHeaderLabels({"地址"});
    d->tradingFrontTable->horizontalHeader()->setStretchLastSection(true);
    d->tradingFrontTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    d->tradingFrontTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tradingLayout->addWidget(d->tradingFrontTable);
    
    frontLayout->addLayout(marketLayout);
    frontLayout->addLayout(tradingLayout);
    
    mainLayout->addWidget(frontGroup);
    
    // 用户凭证区域
    auto* credGroup = new QGroupBox("用户凭证", this);
    auto* credLayout = new QGridLayout(credGroup);
    
    row = 0;
    credLayout->addWidget(new QLabel("用户ID:", this), row, 0);
    d->userIdEdit = new QLineEdit(this);
    d->userIdEdit->setPlaceholderText("请输入用户ID");
    credLayout->addWidget(d->userIdEdit, row, 1);
    
    row++;
    credLayout->addWidget(new QLabel("密码:", this), row, 0);
    d->passwordEdit = new QLineEdit(this);
    d->passwordEdit->setEchoMode(QLineEdit::Password);
    d->passwordEdit->setPlaceholderText("请输入密码");
    credLayout->addWidget(d->passwordEdit, row, 1);
    
    row++;
    d->rememberCheck = new QCheckBox("记住密码（加密存储）", this);
    d->rememberCheck->setChecked(true);
    credLayout->addWidget(d->rememberCheck, row, 0, 1, 2);
    
    row++;
    d->saveCredentialsButton = new QPushButton("保存凭证", this);
    connect(d->saveCredentialsButton, &QPushButton::clicked, this, &CTPBrokerDialog::onSaveCredentials);
    credLayout->addWidget(d->saveCredentialsButton, row, 0, 1, 2);
    
    mainLayout->addWidget(credGroup);
    
    // 操作按钮区域
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    d->testButton = new QPushButton("测试连接", this);
    d->switchButton = new QPushButton("切换到此服务商", this);
    d->closeButton = new QPushButton("关闭", this);
    
    connect(d->testButton, &QPushButton::clicked, this, &CTPBrokerDialog::onTestConnection);
    connect(d->switchButton, &QPushButton::clicked, this, &CTPBrokerDialog::onSwitchBroker);
    connect(d->closeButton, &QPushButton::clicked, this, &QDialog::accept);
    
    buttonLayout->addWidget(d->testButton);
    buttonLayout->addWidget(d->switchButton);
    buttonLayout->addWidget(d->closeButton);
    
    mainLayout->addLayout(buttonLayout);
}

void CTPBrokerDialog::loadBrokers()
{
    d->brokerCombo->clear();
    
    auto brokers = CTPConfigManager::instance()->getEnabledBrokers();
    QString currentId = CTPConfigManager::instance()->currentBrokerId();
    
    for (const auto& broker : brokers) {
        QString displayText = QString("%1 (%2)").arg(broker.name, broker.isSimulation ? "模拟" : "实盘");
        d->brokerCombo->addItem(displayText, broker.id);
        
        if (broker.id == currentId) {
            d->brokerCombo->setCurrentIndex(d->brokerCombo->count() - 1);
        }
    }
}

void CTPBrokerDialog::onBrokerSelected(int index)
{
    if (index < 0) {
        clearBrokerDetails();
        return;
    }
    
    QString brokerId = d->brokerCombo->itemData(index).toString();
    d->currentBrokerId = brokerId;
    
    auto config = CTPConfigManager::instance()->getBroker(brokerId);
    if (config) {
        updateBrokerDetails(*config);
        
        // 加载用户凭证
        QString userId = CTPConfigManager::instance()->getUserId(brokerId);
        QString password = CTPConfigManager::instance()->getPassword(brokerId);
        
        d->userIdEdit->setText(userId);
        d->passwordEdit->setText(password);
    }
}

void CTPBrokerDialog::updateBrokerDetails(const CTPBrokerConfig& config)
{
    d->nameLabel->setText(config.name);
    d->brokerIdLabel->setText(config.brokerId);
    d->descriptionLabel->setText(config.description);
    d->websiteLabel->setText(QString("<a href=\"%1\">%1</a>").arg(config.website));
    d->envLabel->setText(config.isSimulation ? "模拟环境" : "生产环境");
    d->envLabel->setStyleSheet(config.isSimulation ? "color: green;" : "color: red;");
    
    // 更新行情前置表
    d->marketFrontTable->setRowCount(config.marketFronts.size());
    for (int i = 0; i < config.marketFronts.size(); ++i) {
        d->marketFrontTable->setItem(i, 0, new QTableWidgetItem(config.marketFronts[i]));
    }
    
    // 更新交易前置表
    d->tradingFrontTable->setRowCount(config.tradingFronts.size());
    for (int i = 0; i < config.tradingFronts.size(); ++i) {
        d->tradingFrontTable->setItem(i, 0, new QTableWidgetItem(config.tradingFronts[i]));
    }
}

void CTPBrokerDialog::clearBrokerDetails()
{
    d->nameLabel->clear();
    d->brokerIdLabel->clear();
    d->descriptionLabel->clear();
    d->websiteLabel->clear();
    d->envLabel->clear();
    d->marketFrontTable->setRowCount(0);
    d->tradingFrontTable->setRowCount(0);
    d->userIdEdit->clear();
    d->passwordEdit->clear();
}

void CTPBrokerDialog::onAddBroker()
{
    CTPBrokerEditDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        CTPBrokerConfig config = dialog.getBrokerConfig();
        CTPConfigManager::instance()->setBroker(config);
        loadBrokers();
        
        emit brokerConfigChanged(config.id);
    }
}

void CTPBrokerDialog::onEditBroker()
{
    if (d->currentBrokerId.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择一个服务商");
        return;
    }
    
    auto config = CTPConfigManager::instance()->getBroker(d->currentBrokerId);
    if (!config) return;
    
    CTPBrokerEditDialog dialog(*config, this);
    if (dialog.exec() == QDialog::Accepted) {
        CTPBrokerConfig newConfig = dialog.getBrokerConfig();
        CTPConfigManager::instance()->setBroker(newConfig);
        loadBrokers();
        
        emit brokerConfigChanged(newConfig.id);
    }
}

void CTPBrokerDialog::onDeleteBroker()
{
    if (d->currentBrokerId.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择一个服务商");
        return;
    }
    
    auto config = CTPConfigManager::instance()->getBroker(d->currentBrokerId);
    if (!config) return;
    
    // SimNow 预设不允许删除
    if (d->currentBrokerId.startsWith("simnow_")) {
        QMessageBox::warning(this, "提示", "SimNow 预设服务商不能删除，只能禁用");
        return;
    }
    
    auto reply = QMessageBox::question(this, "确认删除",
        QString("确定要删除服务商 \"%1\" 吗？").arg(config->name));
    
    if (reply == QMessageBox::Yes) {
        CTPConfigManager::instance()->removeBroker(d->currentBrokerId);
        loadBrokers();
    }
}

void CTPBrokerDialog::onTestConnection()
{
    if (d->currentBrokerId.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择一个服务商");
        return;
    }
    
    if (d->userIdEdit->text().isEmpty() || d->passwordEdit->text().isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入用户ID和密码");
        return;
    }
    
    // 保存凭证
    onSaveCredentials();
    
    // 发送切换请求（由外部处理实际连接）
    emit switchBrokerRequested(d->currentBrokerId);
    
    QMessageBox::information(this, "测试连接", 
        "正在尝试连接，请查看日志和状态栏了解连接结果。");
}

void CTPBrokerDialog::onSaveCredentials()
{
    if (d->currentBrokerId.isEmpty()) return;
    
    QString userId = d->userIdEdit->text();
    QString password = d->passwordEdit->text();
    
    if (d->rememberCheck->isChecked()) {
        CTPConfigManager::instance()->setUserCredentials(d->currentBrokerId, userId, password);
    } else {
        CTPConfigManager::instance()->clearUserCredentials(d->currentBrokerId);
    }
    
    QMessageBox::information(this, "保存成功", "用户凭证已保存");
}

void CTPBrokerDialog::onSwitchBroker()
{
    if (d->currentBrokerId.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择一个服务商");
        return;
    }
    
    if (d->userIdEdit->text().isEmpty() || d->passwordEdit->text().isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入用户ID和密码");
        return;
    }
    
    // 保存凭证
    onSaveCredentials();
    
    // 设置为当前服务商
    CTPConfigManager::instance()->setCurrentBroker(d->currentBrokerId);
    
    // 发送切换请求
    emit switchBrokerRequested(d->currentBrokerId);
    
    accept();
}

void CTPBrokerDialog::onResetToDefaults()
{
    auto reply = QMessageBox::question(this, "确认重置",
        "确定要重置为默认配置吗？这将清除所有自定义服务商。");
    
    if (reply == QMessageBox::Yes) {
        CTPConfigManager::instance()->resetToDefaults();
        loadBrokers();
    }
}

QString CTPBrokerDialog::selectedBrokerId() const
{
    return d->currentBrokerId;
}

// ========== CTPBrokerEditDialog::Impl ==========

struct CTPBrokerEditDialog::Impl {
    QLineEdit* idEdit = nullptr;
    QLineEdit* nameEdit = nullptr;
    QLineEdit* brokerIdEdit = nullptr;
    QLineEdit* descriptionEdit = nullptr;
    QLineEdit* websiteEdit = nullptr;
    QCheckBox* simulationCheck = nullptr;
    QCheckBox* enabledCheck = nullptr;
    QCheckBox* requireAuthCheck = nullptr;
    QLineEdit* appIdEdit = nullptr;
    QLineEdit* authCodeEdit = nullptr;
    QTableWidget* marketFrontTable = nullptr;
    QTableWidget* tradingFrontTable = nullptr;
    QPushButton* addMarketButton = nullptr;
    QPushButton* removeMarketButton = nullptr;
    QPushButton* addTradingButton = nullptr;
    QPushButton* removeTradingButton = nullptr;
    
    bool isEditMode = false;
};

// ========== CTPBrokerEditDialog ==========

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
    setWindowTitle(d->isEditMode ? "编辑服务商" : "添加服务商");
    setMinimumSize(600, 500);
    
    auto* mainLayout = new QVBoxLayout(this);
    
    // 基本信息
    auto* basicGroup = new QGroupBox("基本信息", this);
    auto* basicLayout = new QGridLayout(basicGroup);
    
    int row = 0;
    basicLayout->addWidget(new QLabel("ID:", this), row, 0);
    d->idEdit = new QLineEdit(this);
    d->idEdit->setPlaceholderText("唯一标识，如 my_broker");
    if (d->isEditMode) d->idEdit->setReadOnly(true);
    basicLayout->addWidget(d->idEdit, row, 1);
    
    row++;
    basicLayout->addWidget(new QLabel("名称:", this), row, 0);
    d->nameEdit = new QLineEdit(this);
    d->nameEdit->setPlaceholderText("显示名称，如 我的期货公司");
    basicLayout->addWidget(d->nameEdit, row, 1);
    
    row++;
    basicLayout->addWidget(new QLabel("经纪商代码:", this), row, 0);
    d->brokerIdEdit = new QLineEdit(this);
    d->brokerIdEdit->setPlaceholderText("如 9999");
    basicLayout->addWidget(d->brokerIdEdit, row, 1);
    
    row++;
    basicLayout->addWidget(new QLabel("描述:", this), row, 0);
    d->descriptionEdit = new QLineEdit(this);
    basicLayout->addWidget(d->descriptionEdit, row, 1);
    
    row++;
    basicLayout->addWidget(new QLabel("官网:", this), row, 0);
    d->websiteEdit = new QLineEdit(this);
    basicLayout->addWidget(d->websiteEdit, row, 1);
    
    row++;
    d->simulationCheck = new QCheckBox("模拟环境", this);
    d->simulationCheck->setChecked(true);
    basicLayout->addWidget(d->simulationCheck, row, 0, 1, 2);
    
    row++;
    d->enabledCheck = new QCheckBox("启用", this);
    d->enabledCheck->setChecked(true);
    basicLayout->addWidget(d->enabledCheck, row, 0, 1, 2);
    
    mainLayout->addWidget(basicGroup);
    
    // 认证信息
    auto* authGroup = new QGroupBox("认证信息（CTP 6.6.1+）", this);
    auto* authLayout = new QGridLayout(authGroup);
    
    row = 0;
    d->requireAuthCheck = new QCheckBox("需要认证", this);
    authLayout->addWidget(d->requireAuthCheck, row, 0, 1, 2);
    
    row++;
    authLayout->addWidget(new QLabel("AppID:", this), row, 0);
    d->appIdEdit = new QLineEdit(this);
    authLayout->addWidget(d->appIdEdit, row, 1);
    
    row++;
    authLayout->addWidget(new QLabel("AuthCode:", this), row, 0);
    d->authCodeEdit = new QLineEdit(this);
    authLayout->addWidget(d->authCodeEdit, row, 1);
    
    mainLayout->addWidget(authGroup);
    
    // 前置地址
    auto* frontGroup = new QGroupBox("前置地址", this);
    auto* frontLayout = new QHBoxLayout(frontGroup);
    
    // 行情前置
    auto* marketLayout = new QVBoxLayout();
    auto* marketBtnLayout = new QHBoxLayout();
    d->addMarketButton = new QPushButton("添加", this);
    d->removeMarketButton = new QPushButton("删除", this);
    connect(d->addMarketButton, &QPushButton::clicked, this, &CTPBrokerEditDialog::onAddMarketFront);
    connect(d->removeMarketButton, &QPushButton::clicked, this, &CTPBrokerEditDialog::onRemoveMarketFront);
    marketBtnLayout->addWidget(d->addMarketButton);
    marketBtnLayout->addWidget(d->removeMarketButton);
    marketLayout->addLayout(marketBtnLayout);
    
    d->marketFrontTable = new QTableWidget(this);
    d->marketFrontTable->setColumnCount(1);
    d->marketFrontTable->setHorizontalHeaderLabels({"行情前置地址"});
    d->marketFrontTable->horizontalHeader()->setStretchLastSection(true);
    marketLayout->addWidget(d->marketFrontTable);
    
    // 交易前置
    auto* tradingLayout = new QVBoxLayout();
    auto* tradingBtnLayout = new QHBoxLayout();
    d->addTradingButton = new QPushButton("添加", this);
    d->removeTradingButton = new QPushButton("删除", this);
    connect(d->addTradingButton, &QPushButton::clicked, this, &CTPBrokerEditDialog::onAddTradingFront);
    connect(d->removeTradingButton, &QPushButton::clicked, this, &CTPBrokerEditDialog::onRemoveTradingFront);
    tradingBtnLayout->addWidget(d->addTradingButton);
    tradingBtnLayout->addWidget(d->removeTradingButton);
    tradingLayout->addLayout(tradingBtnLayout);
    
    d->tradingFrontTable = new QTableWidget(this);
    d->tradingFrontTable->setColumnCount(1);
    d->tradingFrontTable->setHorizontalHeaderLabels({"交易前置地址"});
    d->tradingFrontTable->horizontalHeader()->setStretchLastSection(true);
    tradingLayout->addWidget(d->tradingFrontTable);
    
    frontLayout->addLayout(marketLayout);
    frontLayout->addLayout(tradingLayout);
    
    mainLayout->addWidget(frontGroup);
    
    // 按钮
    auto* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
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
    d->enabledCheck->setChecked(config.isEnabled);
    d->requireAuthCheck->setChecked(config.requireAuth);
    d->appIdEdit->setText(config.defaultAppId);
    d->authCodeEdit->setText(config.defaultAuthCode);
    
    // 加载前置地址
    d->marketFrontTable->setRowCount(config.marketFronts.size());
    for (int i = 0; i < config.marketFronts.size(); ++i) {
        d->marketFrontTable->setItem(i, 0, new QTableWidgetItem(config.marketFronts[i]));
    }
    
    d->tradingFrontTable->setRowCount(config.tradingFronts.size());
    for (int i = 0; i < config.tradingFronts.size(); ++i) {
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
    config.isEnabled = d->enabledCheck->isChecked();
    config.requireAuth = d->requireAuthCheck->isChecked();
    config.defaultAppId = d->appIdEdit->text();
    config.defaultAuthCode = d->authCodeEdit->text();
    
    // 收集前置地址
    for (int i = 0; i < d->marketFrontTable->rowCount(); ++i) {
        auto* item = d->marketFrontTable->item(i, 0);
        if (item && !item->text().isEmpty()) {
            config.marketFronts.append(item->text());
        }
    }
    
    for (int i = 0; i < d->tradingFrontTable->rowCount(); ++i) {
        auto* item = d->tradingFrontTable->item(i, 0);
        if (item && !item->text().isEmpty()) {
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
