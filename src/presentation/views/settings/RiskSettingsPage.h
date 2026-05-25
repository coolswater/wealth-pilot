#ifndef RISKSETTINGSPAGE_H
#define RISKSETTINGSPAGE_H

#include "presentation/components/BasePage.h"
#include <QLabel>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QVector>

/**
 * @brief RiskSettingsPage - Risk control settings page
 */
class RiskSettingsPage : public WealthPilot::BasePage
{
    Q_OBJECT

public:
    // Risk rule structure
    struct RiskRule {
        QString id;
        QString name;
        bool enabled;
        double value;
        QString unit;       // "%", "�?, "�?
        QString description;
    };

    explicit RiskSettingsPage(QWidget *parent = nullptr);
    ~RiskSettingsPage();
    
    // BasePage interface
    QString pageId() const override { return "riskSettings"; }
    QString pageName() const override { return QStringLiteral("风控设置"); }
    void initializePage() override;
    void onPageActivated(const QVariantMap &params) override;

    // Set risk rules
    void setRiskRules(const QVector<RiskRule> &rules);
    QVector<RiskRule> getRiskRules() const;

public slots:
    void onSaveClicked();
    void onResetClicked();
    void onRuleChanged();

signals:
    void riskRulesChanged(const QVector<RiskRule> &rules);

private:
    void initUI();
    void initConnections();
    void updateStyles();
    void loadSettings();
    void saveSettings();
    
    QGroupBox* createRuleGroup(const QString &title, const QString &ruleId,
                                double defaultValue, double minValue, double maxValue,
                                const QString &suffix, const QString &desc);

    // Risk rule controls
    QDoubleSpinBox *m_maxPositionValueSpin;
    QSpinBox *m_maxPositionCountSpin;
    QDoubleSpinBox *m_maxDailyLossSpin;
    QDoubleSpinBox *m_maxSingleLossSpin;
    QDoubleSpinBox *m_maxLeverageSpin;
    QDoubleSpinBox *m_maxMarginRatioSpin;
    QDoubleSpinBox *m_maxDrawdownSpin;
    QCheckBox *m_enableNightTradeCheck;
    QCheckBox *m_enableReverseTradeCheck;
    
    // Buttons
    QPushButton *m_saveBtn;
    QPushButton *m_resetBtn;
    
    // Data
    QVector<RiskRule> m_rules;
};



 // RISKSETTINGSPAGE_H

#endif
