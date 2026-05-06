/**
 * @file StockKLinePage.h
 * @brief 股票K线图页面
 */

#ifndef STOCKKLINEPAGE_H
#define STOCKKLINEPAGE_H

#include "ui/components/BasePage.h"
#include "ui/components/KLineChart.h"
#include <QWidget>
#include <QVector>
#include <QDateTime>
#include <memory>

class QComboBox;
class QPushButton;
class QLabel;
class QLineEdit;
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
 * @brief 图表类型
 */
enum class ChartType {
    KLine,      // K线图
    TimeShare   // 分时图
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

    void setStock(const QString& stockCode, const QString& stockName = QString());
    QString stockCode() const { return m_stockCode; }
    QString stockName() const { return m_stockName; }
    
    void setPeriod(StockKLinePeriod period);
    void setChartType(ChartType type);

signals:
    void stockChanged(const QString& stockCode);
    void periodChanged(int period);

private slots:
    void onChartTypeChanged(int index);
    void onPeriodChanged(int index);
    void onMainIndicatorChanged(int index);
    void onSubIndicatorChanged(int index);
    void onRefresh();
    void onCrosshairMoved(const QDateTime& time, double price);
    void onKLineInfoChanged(const KLineData& kline, int index);

private:
    void setupUI();
    void setupConnections();
    void loadKLineData();
    void loadTimeShareData();
    void generateDemoKLineData();
    void generateDemoTimeShareData();
    void updateInfoLabel(const KLineData& kline);

    QString m_stockCode;
    QString m_stockName;
    StockKLinePeriod m_period = StockKLinePeriod::Day;
    ChartType m_chartType = ChartType::KLine;
    
    // UI组件
    QLabel* m_stockNameLabel = nullptr;
    QTabWidget* m_chartTypeTab = nullptr;
    QComboBox* m_periodCombo = nullptr;
    QComboBox* m_mainIndicatorCombo = nullptr;
    QComboBox* m_subIndicatorCombo = nullptr;
    QPushButton* m_refreshBtn = nullptr;
    QLabel* m_infoLabel = nullptr;
    
    // 图表组件
    KLineChart* m_klineChart = nullptr;
    QWidget* m_timeShareWidget = nullptr;
    
    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // STOCKKLINEPAGE_H
