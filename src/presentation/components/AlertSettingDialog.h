/**
 * @file AlertSettingDialog.h
 * @brief 预警设置对话框 - 配置预警条件和推送方式
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef ALERTSETTINGDIALOG_H
#define ALERTSETTINGDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QTableWidget>
#include <QLineEdit>
#include "services/alert/SmartAlertSystem.h"

class AlertSettingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AlertSettingDialog(QWidget* parent = nullptr);
    ~AlertSettingDialog() override;

    void setSymbol(const QString& symbol);

private slots:
    void onAddAlert();
    void onRemoveAlert();
    void onSaveClicked();
    void onTestWebhook();

private:
    void setupUI();
    void loadAlertConditions();
    void updateAlertTable();
    QString alertTypeToString(SmartAlertType type) const;

    // 预警设置
    QComboBox* m_typeCombo = nullptr;
    QDoubleSpinBox* m_thresholdSpin = nullptr;
    QLineEdit* m_symbolEdit = nullptr;
    QPushButton* m_addBtn = nullptr;
    QPushButton* m_removeBtn = nullptr;
    QTableWidget* m_alertTable = nullptr;

    // 推送设置
    QCheckBox* m_desktopCheck = nullptr;
    QCheckBox* m_emailCheck = nullptr;
    QCheckBox* m_webhookCheck = nullptr;
    QLineEdit* m_webhookUrlEdit = nullptr;
    QLineEdit* m_emailEdit = nullptr;

    QPushButton* m_saveBtn = nullptr;
    QPushButton* m_testWebhookBtn = nullptr;

    QString m_currentSymbol;
    QVector<AlertCondition> m_conditions;
};

#endif // ALERTSETTINGDIALOG_H