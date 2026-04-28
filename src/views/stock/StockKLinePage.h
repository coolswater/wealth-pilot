/**
 * @file StockKLinePage.h
 * @brief 股票K线图页面 - 专业级股票行情分析
 *
 * @details 功能：
 * - K线图展示（日K、周K、月K、分钟K）
 * - 技术指标叠加（MA、MACD、KDJ、BOLL、RSI）
 * - 成交量分析
 * - 资金流向
 * - 十字光标信息
 * - 复权处理
 * - 多股票对比
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef STOCKKLINEPAGE_H
#define STOCKKLINEPAGE_H

#include "core/base/BasePage.h"
#include <QWidget>
#include <QVector>
#include <QDateTime>
#include <memory>

// 前向声明
class QComboBox;
class QPushButton;
class QLabel;
class QTabWidget;
class QSplitter;
class QTableWidget;

/**
 * @brief K线周期枚举
 */
enum class StockKLinePeriod {
    Min1 = 0,       ///< 1分钟
    Min5,           ///< 5分钟
    Min15,          ///< 15分钟
    Min30,          ///< 30分钟
    Min60,          ///< 60分钟
    Day,            ///< 日K
    Week,           ///< 周K
    Month           ///< 月K
};

/**
 * @brief 技术指标类型
 */
enum class TechnicalIndicator {
    None = 0,       ///< 无
    MA,             ///< 移动平均线
    EMA,            ///< 指数移动平均
    MACD,           ///< MACD
    KDJ,            ///< KDJ
    BOLL,           ///< 布林带
    RSI,            ///< 相对强弱指数
    VOL,            ///< 成交量
    VMA             ///< 均量线
};

/**
 * @brief 股票K线图页面
 * @details 专业级股票行情分析页面，支持多周期、多指标
 */
class StockKLinePage : public BasePage
{
    Q_OBJECT

public:
    explicit StockKLinePage(QWidget* parent = nullptr);
    ~StockKLinePage() override;

    // ========== BasePage 接口实现 ==========

    QString pageId() const override { return "stock-kline"; }
    QString pageName() const override { return QStringLiteral("股票K线"); }
    void initializePage() override {}
    void onPageActivated(const QVariantMap& params = {}) override;
    void onPageDeactivated() override;

    // ========== 数据接口 ==========

    /**
     * @brief 设置股票代码
     * @param stockCode 股票代码（如 "000001"）
     * @param exchange 交易所代码（如 "SZ"）
     */
    void setStock(const QString& stockCode, const QString& exchange = "SZ");

    /**
     * @brief 获取当前股票代码
     */
    QString stockCode() const;

    /**
     * @brief 设置K线周期
     * @param period K线周期
     */
    void setPeriod(StockKLinePeriod period);

    /**
     * @brief 设置复权方式
     * @param adjust 复权方式（0-不复权，1-前复权，2-后复权）
     */
    void setAdjustType(int adjust);

    /**
     * @brief 添加技术指标
     * @param indicator 指标类型
     * @param params 指标参数
     */
    void addIndicator(TechnicalIndicator indicator, const QVector<int>& params = {});

    /**
     * @brief 移除技术指标
     * @param indicator 指标类型
     */
    void removeIndicator(TechnicalIndicator indicator);

    /**
     * @brief 清空所有技术指标
     */
    void clearIndicators();

signals:
    /**
     * @brief 股票切换信号
     */
    void stockChanged(const QString& stockCode);

    /**
     * @brief 周期切换信号
     */
    void periodChanged(StockKLinePeriod period);

    /**
     * @brief 十字光标移动信号
     */
    void crosshairMoved(const QDateTime& time, double price, double volume);

private slots:
    /**
     * @brief 周期切换
     */
    void onPeriodChanged(int index);

    /**
     * @brief 复权方式切换
     */
    void onAdjustChanged(int index);

    /**
     * @brief 指标切换
     */
    void onIndicatorChanged(int index);

    /**
     * @brief 刷新数据
     */
    void onRefresh();

    /**
     * @brief 搜索股票
     */
    void onSearchStock(const QString& keyword);

    /**
     * @brief 处理K线数据更新
     */
    void onKLineDataUpdated();

    /**
     * @brief 处理十字光标移动
     */
    void onCrosshairMoved(const QDateTime& time, double price);

    /**
     * @brief 定时刷新（实时行情）
     */
    void onTimerRefresh();

protected:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    // UI 初始化
    void initUI();
    void initTopBar();
    void initToolBar();
    void initMainArea();
    void initConnections();

    // 数据加载
    void loadKLineData();
    void loadStockInfo();
    void calculateIndicators();
    void generateMockData();  ///< 生成模拟数据

    // 技术指标计算
    QVector<double> calculateMA(const QVector<double>& prices, int period);
    QVector<double> calculateEMA(const QVector<double>& prices, int period);
    void calculateMACD(const QVector<double>& prices, int fast, int slow, int signal,
                       QVector<double>& dif, QVector<double>& dea, QVector<double>& macd);
    void calculateKDJ(const QVector<double>& highs, const QVector<double>& lows,
                      const QVector<double>& closes, int n, int m1, int m2,
                      QVector<double>& k, QVector<double>& d, QVector<double>& j);
    void calculateBOLL(const QVector<double>& prices, int n, double k,
                       QVector<double>& mid, QVector<double>& upper, QVector<double>& lower);
    QVector<double> calculateRSI(const QVector<double>& prices, int period);

    // UI 更新
    void updateStockInfo();
    void updateIndicatorPanel();
    void updateInfoPanel(const QDateTime& time, double price, double volume);
    
    // 辅助函数
    static QString formatVolume(qint64 volume);
    static QString formatMoney(double value);

    // PIMPL 实现
    struct Impl;
    std::unique_ptr<Impl> d;
    
    // 信息标签（用于上方信息栏）
    QLabel* m_openLabel = nullptr;
    QLabel* m_highLabel = nullptr;
    QLabel* m_lowLabel = nullptr;
    QLabel* m_closeLabel = nullptr;
    QLabel* m_preCloseLabel = nullptr;
    QLabel* m_volumeLabel = nullptr;
    QLabel* m_turnoverLabel = nullptr;
    QLabel* m_turnoverRateLabel = nullptr;
    QLabel* m_amplitudeLabel = nullptr;
    QLabel* m_peLabel = nullptr;
    QLabel* m_pbLabel = nullptr;
    QLabel* m_totalValueLabel = nullptr;
    QLabel* m_circulationValueLabel = nullptr;
    QLabel* m_codeLabel = nullptr;
    
    // 新增信息标签
    QLabel* m_limitUpLabel = nullptr;           // 涨停
    QLabel* m_limitDownLabel = nullptr;         // 跌停
    QLabel* m_afterHoursVolumeLabel = nullptr;  // 盘后量
    QLabel* m_volumeRatioLabel = nullptr;       // 量比
    QLabel* m_afterHoursAmountLabel = nullptr;  // 盘后额
    QLabel* m_orderRatioLabel = nullptr;        // 委比
    QLabel* m_peTTMLabel = nullptr;             // 市盈率(TTM)
    QLabel* m_peDynamicLabel = nullptr;         // 市盈率(动)
    QLabel* m_peStaticLabel = nullptr;          // 市盈率(静)
    QLabel* m_epsLabel = nullptr;               // 每股收益
    QLabel* m_dividendLabel = nullptr;          // 股息(TTM)
    QLabel* m_bpsLabel = nullptr;               // 每股净资产
    QLabel* m_dividendYieldLabel = nullptr;     // 股息率(TTM)
    QLabel* m_week52HighLabel = nullptr;        // 52周最高
    QLabel* m_week52LowLabel = nullptr;         // 52周最低
    QLabel* m_totalSharesLabel = nullptr;       // 总股本
    QLabel* m_floatSharesLabel = nullptr;       // 流通股
    QLabel* m_pledgeRatioLabel = nullptr;       // 质押率
    QLabel* m_registrationLabel = nullptr;      // 注册制
    QLabel* m_goodwillRatioLabel = nullptr;     // 商誉/净资产
    QLabel* m_currencyLabel = nullptr;          // 货币单位
    
    // 副图指标面板
    QWidget* m_indicatorPanel = nullptr;
    
    // 成交明细表格
    QTableWidget* m_tradeDetailTable = nullptr;
};

#endif // STOCKKLINEPAGE_H
