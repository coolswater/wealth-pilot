/**
 * @file FuturesKLinePage.h
 * @brief Futures K-Line Page - High-performance K-line chart and technical analysis
 */

#ifndef FUTURES_KLINE_PAGE_H
#define FUTURES_KLINE_PAGE_H

#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QTableWidget>
#include <QSplitter>

#include "core/base/BasePage.h"
#include "plugins/ICTPPlugin.h"
#include "ui/components/KLineChart.h"

// Forward declarations
class TechnicalIndicatorPanel;
class TradingPanel;
class RealtimeQuoteWidget;

/**
 * @brief K-Line period enumeration
 */
enum class KLinePeriod {
    Minute1,
    Minute5,
    Minute15,
    Minute30,
    Hour1,
    Hour4,
    Day1,
    Week1,
    Month1
};

/**
 * @brief Futures K-Line Page
 */
class FuturesKLinePage : public BasePage
{
    Q_OBJECT

public:
    explicit FuturesKLinePage(QWidget *parent = nullptr);
    ~FuturesKLinePage() override;

    QString pageId() const override { return "FuturesKLine"; }
    void initializePage() override;
    void refresh();

    void setInstrument(const QString& instrumentId, const QString& instrumentName = QString());
    QString instrument() const;
    
    void setPeriod(KLinePeriod period);
    void setIndicatorEnabled(const QString& indicator, bool enabled);

    void onPageActivated(const QVariantMap& params) override;

signals:
    void tradeRequested(const QString& instrumentId, 
                       const QString& direction,
                       double price, 
                       int volume);
    void pageTitleChanged(const QString& title);

protected:
    void setupUI();
    void setupConnections();

private slots:
    void onMarketDataUpdated(const MarketData& data);
    void onKLineDataReceived(const QVector<KLineData>& data);
    void onPeriodChanged(int period);
    void onIndicatorToggled(const QString& indicator, bool enabled);

private:
    void loadKLineData();
    void calculateIndicators();
    void updateQuoteDisplay(const MarketData& quote);

    struct Impl;
    std::unique_ptr<Impl> d;
};

/**
 * @brief Technical Indicator Panel
 */
class TechnicalIndicatorPanel : public QWidget
{
    Q_OBJECT

public:
    explicit TechnicalIndicatorPanel(QWidget *parent = nullptr);
    ~TechnicalIndicatorPanel();
    
    void setIndicatorData(const QString& name, const QMap<QString, double>& data);
    void clearData();

signals:
    void indicatorToggled(const QString& indicator, bool enabled);

private:
    void setupUI();
    
    struct Impl;
    std::unique_ptr<Impl> d;
};

/**
 * @brief Trading Panel
 */
class TradingPanel : public QWidget
{
    Q_OBJECT

public:
    explicit TradingPanel(QWidget *parent = nullptr);
    ~TradingPanel();
    
    void setInstrument(const QString& instrumentId);
    void setPrice(double price);
    void setAvailable(double available);

signals:
    void buyClicked();
    void sellClicked();

private:
    void setupUI();
    
    struct Impl;
    std::unique_ptr<Impl> d;
};

/**
 * @brief Realtime Quote Widget
 */
class RealtimeQuoteWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RealtimeQuoteWidget(QWidget *parent = nullptr);
    ~RealtimeQuoteWidget();
    
    void updateQuote(const MarketData& quote);
    void setInstrument(const QString& instrumentId);

private:
    void setupUI();
    
    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // FUTURES_KLINE_PAGE_H
