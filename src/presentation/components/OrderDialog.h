#ifndef ORDERDIALOG_H
#define ORDERDIALOG_H

#include <QDialog>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QMap>
#include <QString>
#include "core/trading/TradingTypes.h"
#include "infrastructure/config/Tokens.h"

// Use types from TradingTypes.h
// OrderType, PositionDirection, OpenCloseFlag are already defined there

/**
 * @brief OrderDialog - Trading order dialog
 * 
 * Provides a comprehensive interface for placing trading orders including:
 * - Market/Limit/Stop/StopLimit orders
 * - Open/Close position selection
 * - Quantity and price input
 * - Stop loss and take profit settings
 * - Risk validation display
 */
class OrderDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OrderDialog(QWidget *parent = nullptr);
    ~OrderDialog();

    // Set contract information
    void setContract(const QString &instrumentId, 
                     const QString &instrumentName,
                     double lastPrice,
                     double tickSize,
                     int volumeMultiple,
                     double marginRatio);
    
    // Set account information
    void setAccount(double available, 
                    double margin,
                    double frozenMargin);
    
    // Set current position
    void setCurrentPosition(int longPos, int shortPos);
    
    // Get order parameters
    struct OrderParams {
        QString instrumentId;
        OrderType orderType;
        PositionDirection direction;
        OpenCloseFlag openClose;
        int quantity;
        double price;
        double stopPrice;       // For stop orders
        double takeProfitPrice;
        double stopLossPrice;
        bool enableTakeProfit;
        bool enableStopLoss;
    };
    
    OrderParams getOrderParams() const;
    
    // Validate order
    bool validateOrder(QString &errorMsg);

signals:
    void orderSubmitted(const OrderParams &params);
    void orderCancelled();

private slots:
    void onOrderTypeChanged(int index);
    void onDirectionChanged(int index);
    void onOpenCloseChanged(int index);
    void onQuantityChanged(int value);
    void onPriceChanged(double value);
    void onCalculateClicked();
    void onSubmitClicked();
    void onCancelClicked();
    void onResetClicked();

private:
    void initUI();
    void initConnections();
    void updateStyles();
    void updateCalculations();
    void updateRiskDisplay();
    void updatePositionInfo();
    void clearInputs();

    // UI Components - Header
    QLabel *m_instrumentLabel;
    QLabel *m_priceLabel;
    QLabel *m_changeLabel;
    
    // UI Components - Order Type
    QComboBox *m_orderTypeCombo;
    QComboBox *m_directionCombo;
    QComboBox *m_openCloseCombo;
    
    // UI Components - Price & Quantity
    QDoubleSpinBox *m_priceSpinBox;
    QDoubleSpinBox *m_stopPriceSpinBox;
    QSpinBox *m_quantitySpinBox;
    
    // UI Components - Calculations
    QLabel *m_marginLabel;
    QLabel *m_commissionLabel;
    QLabel *m_totalLabel;
    
    // UI Components - Stop Loss & Take Profit
    QCheckBox *m_enableTakeProfitCheck;
    QDoubleSpinBox *m_takeProfitSpinBox;
    QCheckBox *m_enableStopLossCheck;
    QDoubleSpinBox *m_stopLossSpinBox;
    
    // UI Components - Position Info
    QLabel *m_longPosLabel;
    QLabel *m_shortPosLabel;
    QLabel *m_availableLabel;
    
    // UI Components - Risk Display
    QLabel *m_riskRatioLabel;
    QLabel *m_riskAmountLabel;
    QLabel *m_profitRatioLabel;
    QLabel *m_profitAmountLabel;
    
    // UI Components - Buttons
    QPushButton *m_calculateBtn;
    QPushButton *m_submitBtn;
    QPushButton *m_cancelBtn;
    QPushButton *m_resetBtn;
    
    // Contract data
    QString m_instrumentId;
    QString m_instrumentName;
    double m_lastPrice;
    double m_tickSize;
    int m_volumeMultiple;
    double m_marginRatio;
    
    // Account data
    double m_available;
    double m_margin;
    double m_frozenMargin;
    
    // Position data
    int m_longPosition;
    int m_shortPosition;
    
    // Colors - using design tokens from Tokens.h
    // These map to the standard WealthPilot color palette:
    // - COLOR_UP: Danger (#f85149) - 涨/红色 (China: red for up)
    // - COLOR_DOWN: Success (#3fb950) - 跌/绿色 (China: green for down)
    // - COLOR_BG: BgBase (#0d1117) - 主背景
    // - COLOR_CARD: BgCard (#161b22) - 卡片背景
    // - COLOR_TEXT: TextPrimary (#e6edf3) - 主文字
    // - COLOR_TEXT_SECONDARY: TextSecondary (#8b949e) - 次要文字
    const QString COLOR_UP = Tokens::Colors::Danger;
    const QString COLOR_DOWN = Tokens::Colors::Success;
    const QString COLOR_BG = Tokens::Colors::BgBase;
    const QString COLOR_CARD = Tokens::Colors::BgCard;
    const QString COLOR_TEXT = Tokens::Colors::TextPrimary;
    const QString COLOR_TEXT_SECONDARY = Tokens::Colors::TextSecondary;
};

#endif // ORDERDIALOG_H
