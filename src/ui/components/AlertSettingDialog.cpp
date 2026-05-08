/**
 * @file AlertSettingDialog.cpp
 * @brief 预警设置对话框实现
 */

#include "AlertSettingDialog.h"
#include "core/config/Tokens.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>

using namespace Tokens;

AlertSettingDialog::AlertSettingDialog(QWidget* parent)
    : QDialog(parent)
{
    setupUI();
    loadAlertConditions();
}

AlertSettingDialog::~AlertSettingDialog()
{
}

void AlertSettingDialog::setSymbol(const QString& symbol)
{
    m_currentSymbol = symbol;
    m_symbolEdit->setText(symbol);
    loadAlertConditions();
}

void AlertSettingDialog::onAddAlert()
{
    QString symbol = m_symbolEdit->text().trimmed();
    if (symbol.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"),
            QStringLiteral("请输入股票代码"));
        return;
    }

    AlertCondition condition;
    condition.symbol = symbol;

    // 设置预警类型
    switch (m_typeCombo->currentIndex()) {
    case 0: condition.type = SmartAlertType::PriceBreakUp;
        break;
    case 1: condition.type = SmartAlertType::PriceBreakDown;
        break;
    case 2: condition.type = SmartAlertType::MaGoldenCross;
        break;
    case 3: condition.type = SmartAlertType::MaDeathCross;
        break;
    case 4: condition.type = SmartAlertType::VolumeSpike;
        break;
    case 5: condition.type = SmartAlertType::RsiOverbought;
        break;
    case 6: condition.type = SmartAlertType::RsiOversold;
        break;
    }

    condition.threshold = m_thresholdSpin->value();
    condition.enabled = true;

    // 设置推送方式
    PushMethods methods;
    if (m_desktopCheck->isChecked()) {
        methods |= PushMethod::Desktop;
    }
    if (m_emailCheck->isChecked()) {
        methods |= PushMethod::Email;
    }
    if (m_webhookCheck->isChecked()) {
        methods |= PushMethod::Webhook;
    }
    condition.pushMethods = methods;

    // 添加到预警系统
    QString conditionId = SmartAlertSystem::instance()->addAlertCondition(condition);

    m_conditions.append(condition);
    updateAlertTable();

    QMessageBox::information(this, QStringLiteral("成功"),
        QStringLiteral("预警条件已添加"));
}

void AlertSettingDialog::onRemoveAlert()
{
    int row = m_alertTable->currentRow();
    if (row < 0 || row >= m_conditions.size()) {
        QMessageBox::warning(this, QStringLiteral("提示"),
            QStringLiteral("请选择要删除的预警"));
        return;
    }

    AlertCondition condition = m_conditions[row];
    SmartAlertSystem::instance()->removeAlertCondition(condition.id);

    m_conditions.removeAt(row);
    updateAlertTable();
}

void AlertSettingDialog::onSaveClicked()
{
    // 保存Webhook配置
    if (m_webhookCheck->isChecked() && !m_webhookUrlEdit->text().isEmpty()) {
        WebhookConfig webhook;
        webhook.name = QStringLiteral("default");
        webhook.url = m_webhookUrlEdit->text();
        webhook.method = QStringLiteral("POST");
        webhook.contentType = QStringLiteral("application/json");
        SmartAlertSystem::instance()->setWebhookConfig(QStringLiteral("default"), webhook);
    }

    // 保存邮件配置
    if (m_emailCheck->isChecked() && !m_emailEdit->text().isEmpty()) {
        // 保存邮件配置到SmartAlertSystem
        QString email = m_emailEdit->text().trimmed();
        // 注意：实际应用中应该从专门的配置界面获取SMTP服务器等信息
        // 这里简化处理，使用默认配置
        SmartAlertSystem::instance()->setEmailConfig(QString(), 0, QString(), QString());
    }

    accept();
}

void AlertSettingDialog::onTestWebhook()
{
    QString url = m_webhookUrlEdit->text().trimmed();
    if (url.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"),
            QStringLiteral("请输入Webhook URL"));
        return;
    }

    // 创建测试预警
    AlertTrigger testTrigger;
    testTrigger.symbol = m_currentSymbol.isEmpty() ? QStringLiteral("sh600000") : m_currentSymbol;
    testTrigger.type = SmartAlertType::PriceBreakUp;
    testTrigger.triggerValue = 10.50;
    testTrigger.threshold = 10.00;
    testTrigger.triggerTime = QDateTime::currentDateTime();
    testTrigger.message = QStringLiteral("【测试】预警系统测试消息");

    // 临时设置Webhook并发送
    WebhookConfig webhook;
    webhook.name = QStringLiteral("test");
    webhook.url = url;
    SmartAlertSystem::instance()->setWebhookConfig(QStringLiteral("test"), webhook);

    QMessageBox::information(this, QStringLiteral("测试"),
        QStringLiteral("Webhook测试消息已发送，请检查接收端"));
}

void AlertSettingDialog::setupUI()
{
    setWindowTitle(QStringLiteral("预警设置"));
    setMinimumSize(700, 500);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // 预警条件设置组
    QGroupBox* conditionGroup = new QGroupBox(QStringLiteral("添加预警条件"));
    QVBoxLayout* conditionLayout = new QVBoxLayout(conditionGroup);

    // 第一行：股票代码和预警类型
    QHBoxLayout* row1 = new QHBoxLayout();
    QLabel* symbolLabel = new QLabel(QStringLiteral("股票代码:"));
    m_symbolEdit = new QLineEdit();
    m_symbolEdit->setPlaceholderText(QStringLiteral("如: sh600000"));
    m_symbolEdit->setMaximumWidth(120);
    row1->addWidget(symbolLabel);
    row1->addWidget(m_symbolEdit);

    QLabel* typeLabel = new QLabel(QStringLiteral("预警类型:"));
    m_typeCombo = new QComboBox();
    m_typeCombo->addItem(QStringLiteral("价格突破上限"));
    m_typeCombo->addItem(QStringLiteral("价格突破下限"));
    m_typeCombo->addItem(QStringLiteral("均线金叉"));
    m_typeCombo->addItem(QStringLiteral("均线死叉"));
    m_typeCombo->addItem(QStringLiteral("成交量异动"));
    m_typeCombo->addItem(QStringLiteral("RSI超买"));
    m_typeCombo->addItem(QStringLiteral("RSI超卖"));
    row1->addWidget(typeLabel);
    row1->addWidget(m_typeCombo);

    QLabel* thresholdLabel = new QLabel(QStringLiteral("阈值:"));
    m_thresholdSpin = new QDoubleSpinBox();
    m_thresholdSpin->setRange(-10000, 10000);
    m_thresholdSpin->setDecimals(2);
    m_thresholdSpin->setValue(10.0);
    m_thresholdSpin->setMaximumWidth(100);
    row1->addWidget(thresholdLabel);
    row1->addWidget(m_thresholdSpin);
    row1->addStretch();
    conditionLayout->addLayout(row1);

    // 添加/删除按钮
    QHBoxLayout* btnLayout = new QHBoxLayout();
    m_addBtn = new QPushButton(QStringLiteral("添加预警"));
    m_removeBtn = new QPushButton(QStringLiteral("删除选中"));
    btnLayout->addWidget(m_addBtn);
    btnLayout->addWidget(m_removeBtn);
    btnLayout->addStretch();
    conditionLayout->addLayout(btnLayout);

    connect(m_addBtn, &QPushButton::clicked, this, &AlertSettingDialog::onAddAlert);
    connect(m_removeBtn, &QPushButton::clicked, this, &AlertSettingDialog::onRemoveAlert);

    mainLayout->addWidget(conditionGroup);

    // 预警列表
    QGroupBox* listGroup = new QGroupBox(QStringLiteral("已设置的预警"));
    QVBoxLayout* listLayout = new QVBoxLayout(listGroup);

    m_alertTable = new QTableWidget();
    m_alertTable->setColumnCount(5);
    m_alertTable->setHorizontalHeaderLabels({
        QStringLiteral("股票代码"),
        QStringLiteral("预警类型"),
        QStringLiteral("阈值"),
        QStringLiteral("推送方式"),
        QStringLiteral("状态")
    });
    m_alertTable->horizontalHeader()->setStretchLastSection(true);
    m_alertTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_alertTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_alertTable->verticalHeader()->setVisible(false);
    listLayout->addWidget(m_alertTable);

    mainLayout->addWidget(listGroup);

    // 推送设置组
    QGroupBox* pushGroup = new QGroupBox(QStringLiteral("推送设置"));
    QVBoxLayout* pushLayout = new QVBoxLayout(pushGroup);

    m_desktopCheck = new QCheckBox(QStringLiteral("桌面弹窗"));
    m_desktopCheck->setChecked(true);
    pushLayout->addWidget(m_desktopCheck);

    QHBoxLayout* emailLayout = new QHBoxLayout();
    m_emailCheck = new QCheckBox(QStringLiteral("邮件通知"));
    m_emailEdit = new QLineEdit();
    m_emailEdit->setPlaceholderText(QStringLiteral("your@email.com"));
    m_emailEdit->setMaximumWidth(200);
    emailLayout->addWidget(m_emailCheck);
    emailLayout->addWidget(m_emailEdit);
    emailLayout->addStretch();
    pushLayout->addLayout(emailLayout);

    QHBoxLayout* webhookLayout = new QHBoxLayout();
    m_webhookCheck = new QCheckBox(QStringLiteral("Webhook"));
    m_webhookUrlEdit = new QLineEdit();
    m_webhookUrlEdit->setPlaceholderText(QStringLiteral("https://oapi.dingtalk.com/robot/send?access_token=xxx"));
    m_testWebhookBtn = new QPushButton(QStringLiteral("测试"));
    m_testWebhookBtn->setFixedWidth(60);
    webhookLayout->addWidget(m_webhookCheck);
    webhookLayout->addWidget(m_webhookUrlEdit);
    webhookLayout->addWidget(m_testWebhookBtn);
    pushLayout->addLayout(webhookLayout);

    connect(m_testWebhookBtn, &QPushButton::clicked, this, &AlertSettingDialog::onTestWebhook);

    mainLayout->addWidget(pushGroup);

    // 底部按钮
    QHBoxLayout* bottomLayout = new QHBoxLayout();
    bottomLayout->addStretch();

    m_saveBtn = new QPushButton(QStringLiteral("保存"));
    m_saveBtn->setFixedWidth(80);
    m_saveBtn->setStyleSheet(QString(
        "QPushButton { background-color: %1; color: white; border-radius: 4px; }"
        "QPushButton:hover { background-color: %2; }"
    ).arg(Colors::Primary, Colors::PrimaryHover));
    connect(m_saveBtn, &QPushButton::clicked, this, &AlertSettingDialog::onSaveClicked);

    QPushButton* cancelBtn = new QPushButton(QStringLiteral("取消"));
    cancelBtn->setFixedWidth(80);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    bottomLayout->addWidget(m_saveBtn);
    bottomLayout->addWidget(cancelBtn);
    mainLayout->addLayout(bottomLayout);

    setStyleSheet(QString("QDialog { background-color: %1; }")
        .arg(Colors::BgSurface));
}

void AlertSettingDialog::loadAlertConditions()
{
    m_conditions = SmartAlertSystem::instance()->getAlertConditions(m_currentSymbol);
    updateAlertTable();
}

void AlertSettingDialog::updateAlertTable()
{
    m_alertTable->setRowCount(0);

    for (const AlertCondition& condition : m_conditions) {
        int row = m_alertTable->rowCount();
        m_alertTable->insertRow(row);

        m_alertTable->setItem(row, 0, new QTableWidgetItem(condition.symbol));
        m_alertTable->setItem(row, 1, new QTableWidgetItem(alertTypeToString(condition.type)));
        m_alertTable->setItem(row, 2, new QTableWidgetItem(
            QString::number(condition.threshold, 'f', 2)));

        // 推送方式
        QStringList methods;
        if (condition.pushMethods & PushMethod::Desktop) {
            methods << QStringLiteral("桌面");
        }
        if (condition.pushMethods & PushMethod::Email) {
            methods << QStringLiteral("邮件");
        }
        if (condition.pushMethods & PushMethod::Webhook) {
            methods << QStringLiteral("Webhook");
        }
        m_alertTable->setItem(row, 3, new QTableWidgetItem(methods.join(QStringLiteral(", "))));

        // 状态
        QString status = condition.enabled ? QStringLiteral("启用") : QStringLiteral("禁用");
        QTableWidgetItem* statusItem = new QTableWidgetItem(status);
        statusItem->setForeground(QColor(condition.enabled ? Colors::Success : Colors::TextSecondary));
        m_alertTable->setItem(row, 4, statusItem);
    }
}

QString AlertSettingDialog::alertTypeToString(SmartAlertType type) const
{
    switch (type) {
    case SmartAlertType::PriceBreakUp: return QStringLiteral("价格突破上限");
    case SmartAlertType::PriceBreakDown: return QStringLiteral("价格突破下限");
    case SmartAlertType::MaGoldenCross: return QStringLiteral("均线金叉");
    case SmartAlertType::MaDeathCross: return QStringLiteral("均线死叉");
    case SmartAlertType::VolumeSpike: return QStringLiteral("成交量异动");
    case SmartAlertType::RsiOverbought: return QStringLiteral("RSI超买");
    case SmartAlertType::RsiOversold: return QStringLiteral("RSI超卖");
    case SmartAlertType::Custom: return QStringLiteral("自定义");
    default: return QStringLiteral("未知");
    }
}
