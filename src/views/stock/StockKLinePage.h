/**
 * @file StockKLinePage.h
 * @brief 股票K线图页面
 */

#ifndef STOCKKLINEPAGE_H
#define STOCKKLINEPAGE_H

#include "ui/components/BasePage.h"
#include <QWidget>
#include <QVector>
#include <QDateTime>
#include <memory>

class QComboBox;
class QPushButton;
class QLabel;
class QLineEdit;
class QTableWidget;
class QTabWidget;
class QSplitter;

/**
 * @brief K线周期枚举
 */
enum class StockKLinePeriod {
    Min1 = 0,
    Min5,
    Min15,
    Min30,
    Min60,
    Day,
    Week,
    Month
};

/**
 * @brief 技术指标类型
 */
enum class TechnicalIndicator {
    None = 0,
    MA,
    EMA,
    MACD,
    KDJ,
    BOLL,
    RSI,
    VOL,
    VMA
};

/**
 * @brief 股票K线图页面
 */
class StockKLinePage : public WealthPilot::BasePage
{
    Q_OBJECT

public:
    explicit StockKLinePage(QWidget* parent = nullptr);
    ~StockKLinePage() override;

    QString pageId() const override { return "stock-kline"; }
    QString pageName() const override { return QStringLiteral("股票K线"); }

    void setStock(const QString& stockCode, const QString& exchange = "SZ");
    QString stockCode() const;
    void setPeriod(StockKLinePeriod period);
    void setAdjustType(int adjust);

signals:
    void stockChanged(const QString& stockCode);
    void periodChanged(int period);
    void crosshairMoved(const QDateTime& time, double price, double volume);

private slots:
    void onPeriodChanged(int index);
    void onAdjustChanged(int index);
    void onRefresh();

private:
    void setupUI();
    void setupConnections();

    QString m_stockCode;
    StockKLinePeriod m_period = StockKLinePeriod::Day;
    
    QComboBox* m_periodCombo = nullptr;
    QComboBox* m_adjustCombo = nullptr;
    QPushButton* m_refreshBtn = nullptr;
    QLabel* m_stockNameLabel = nullptr;
    
    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // STOCKKLINEPAGE_H
