/**
 * @file AkShareDataSource.h
 * @brief AkShare 数据源实现
 *
 * @details 使用 Python AkShare 库获取中国A股市场数据
 */

#ifndef WEALTHPILOT_AKSHARE_DATA_SOURCE_H
#define WEALTHPILOT_AKSHARE_DATA_SOURCE_H

#include "DataSourceInterface.h"
#include "infrastructure/python/PythonRunner.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace WealthPilot {

/**
 * @brief AkShare 数据源 - 中国A股数据
 */
class AkShareDataSource : public DataSourceInterface {
public:
    AkShareDataSource() = default;
    ~AkShareDataSource() override = default;

    // ========== 基本信息 ==========
    
    QString name() const override { return QStringLiteral("AkShare"); }
    QString id() const override { return QStringLiteral("akshare"); }
    QString description() const override { 
        return QStringLiteral("中国A股市场数据源，支持股票、基金、期货等"); 
    }
    
    DataSourceCapabilities capabilities() const override {
        DataSourceCapabilities caps;
        caps.supportsStocks = true;
        caps.supportsFutures = true;
        caps.supportsFund = true;
        caps.supportsHistory = true;
        caps.supportsRealtime = false;  // AkShare 不支持实时推送
        caps.supportsNews = true;
        caps.supportsSectors = true;
        caps.supportsFinancials = true;
        return caps;
    }

    // ========== 连接管理 ==========
    
    Result<void> initialize() override {
        // 检查 Python 是否可用
        if (!PythonRunner::instance().isPythonAvailable()) {
            return Result<void>::error("PYTHON_NOT_AVAILABLE", 
                "Python is not available");
        }
        
        // 检查 akshare 是否已安装
        if (!PythonRunner::instance().isPackageInstalled("akshare")) {
            // 尝试安装
            auto result = PythonRunner::instance().installPackage("akshare");
            if (result.isError()) {
                return Result<void>::error("PACKAGE_INSTALL_FAILED",
                    "Failed to install akshare: " + result.errorMessage());
            }
        }
        
        m_initialized = true;
        return Result<void>::ok();
    }
    
    void shutdown() override {
        m_initialized = false;
    }
    
    bool isInitialized() const override { return m_initialized; }
    bool isConnected() const override { return m_initialized; }

    // ========== 行情数据 ==========
    
    Result<Quote> getQuote(const QString& symbol) override {
        // 使用 Python 脚本获取行情
        QString code = QString(R"(
import akshare as ak
import json

symbol = "%1"
try:
    df = ak.stock_zh_a_spot_em()
    row = df[df['代码'] == symbol.lstrip('sh').lstrip('sz')]
    if len(row) > 0:
        data = row.iloc[0].to_dict()
        result = {
            "symbol": symbol,
            "name": data.get("名称", ""),
            "lastPrice": float(data.get("最新价", 0)),
            "openPrice": float(data.get("今开", 0)),
            "highPrice": float(data.get("最高", 0)),
            "lowPrice": float(data.get("最低", 0)),
            "preClose": float(data.get("昨收", 0)),
            "change": float(data.get("涨跌额", 0)),
            "changePercent": float(data.get("涨跌幅", 0)),
            "volume": float(data.get("成交量", 0)),
            "amount": float(data.get("成交额", 0)),
        }
        print(json.dumps(result))
    else:
        print(json.dumps({"error": "Symbol not found"}))
except Exception as e:
    print(json.dumps({"error": str(e)}))
)").arg(symbol);
        
        auto result = PythonRunner::instance().executeCode(code);
        if (result.isError()) {
            return Result<Quote>::error(result.error());
        }
        
        // 解析结果
        auto& pyResult = result.value();
        if (pyResult.data.isValid()) {
            auto map = pyResult.data.toMap();
            if (map.contains("error")) {
                return Result<Quote>::error("QUOTE_ERROR", map["error"].toString());
            }
            
            Quote quote;
            quote.symbol = map["symbol"].toString();
            quote.name = map["name"].toString();
            quote.lastPrice = map["lastPrice"].toDouble();
            quote.openPrice = map["openPrice"].toDouble();
            quote.highPrice = map["highPrice"].toDouble();
            quote.lowPrice = map["lowPrice"].toDouble();
            quote.preClose = map["preClose"].toDouble();
            quote.change = map["change"].toDouble();
            quote.changePercent = map["changePercent"].toDouble();
            quote.volume = map["volume"].toDouble();
            quote.amount = map["amount"].toDouble();
            quote.time = QDateTime::currentDateTime();
            
            return Result<Quote>::ok(quote);
        }
        
        return Result<Quote>::error("PARSE_ERROR", "Failed to parse quote data");
    }
    
    Result<QVector<Quote>> getQuotes(const QStringList& symbols) override {
        QVector<Quote> quotes;
        for (const auto& symbol : symbols) {
            auto result = getQuote(symbol);
            if (result.isOk()) {
                quotes.append(result.value());
            }
        }
        return Result<QVector<Quote>>::ok(quotes);
    }
    
    void subscribeQuotes(const QStringList& symbols) override {
        Q_UNUSED(symbols)
        // AkShare 不支持实时订阅
    }
    
    void unsubscribeQuotes(const QStringList& symbols) override {
        Q_UNUSED(symbols)
    }

    // ========== K线数据 ==========
    
    Result<QVector<Bar>> getBars(
        const QString& symbol,
        const QString& period,
        const QDateTime& start,
        const QDateTime& end) override 
    {
        // 映射周期
        QString akPeriod = "daily";
        if (period == "1m") akPeriod = "1";
        else if (period == "5m") akPeriod = "5";
        else if (period == "15m") akPeriod = "15";
        else if (period == "30m") akPeriod = "30";
        else if (period == "60m") akPeriod = "60";
        else if (period == "D") akPeriod = "daily";
        else if (period == "W") akPeriod = "weekly";
        else if (period == "M") akPeriod = "monthly";
        
        QString code = QString(R"(
import akshare as ak
import json

symbol = "%1"
period = "%2"
start = "%3"
end = "%4"

try:
    df = ak.stock_zh_a_hist(symbol=symbol.lstrip('sh').lstrip('sz'), 
                            period=period, 
                            start_date=start.replace("-", ""),
                            end_date=end.replace("-", ""),
                            adjust="qfq")
    
    bars = []
    for _, row in df.iterrows():
        bar = {
            "time": str(row["日期"]),
            "open": float(row["开盘"]),
            "high": float(row["最高"]),
            "low": float(row["最低"]),
            "close": float(row["收盘"]),
            "volume": float(row["成交量"]),
            "amount": float(row["成交额"]),
        }
        bars.append(bar)
    
    print(json.dumps({"bars": bars}))
except Exception as e:
    print(json.dumps({"error": str(e)}))
)").arg(symbol, akPeriod, 
        start.toString("yyyy-MM-dd"), 
        end.toString("yyyy-MM-dd"));
        
        auto result = PythonRunner::instance().executeCode(code);
        if (result.isError()) {
            return Result<QVector<Bar>>::error(result.error());
        }
        
        // 解析结果
        auto& pyResult = result.value();
        if (pyResult.data.isValid()) {
            auto map = pyResult.data.toMap();
            if (map.contains("error")) {
                return Result<QVector<Bar>>::error("BAR_ERROR", map["error"].toString());
            }
            
            QVector<Bar> bars;
            auto barsList = map["bars"].toList();
            for (const auto& barData : barsList) {
                auto barMap = barData.toMap();
                Bar bar;
                bar.time = QDateTime::fromString(barMap["time"].toString(), "yyyy-MM-dd");
                bar.open = barMap["open"].toDouble();
                bar.high = barMap["high"].toDouble();
                bar.low = barMap["low"].toDouble();
                bar.close = barMap["close"].toDouble();
                bar.volume = barMap["volume"].toDouble();
                bar.amount = barMap["amount"].toDouble();
                bars.append(bar);
            }
            
            return Result<QVector<Bar>>::ok(bars);
        }
        
        return Result<QVector<Bar>>::error("PARSE_ERROR", "Failed to parse bar data");
    }
    
    Result<QVector<Bar>> getRecentBars(
        const QString& symbol,
        const QString& period,
        int count) override 
    {
        QDateTime end = QDateTime::currentDateTime();
        QDateTime start;
        
        // 根据周期计算开始时间
        if (period == "D") {
            start = end.addDays(-count * 2);  // 多取一些，排除非交易日
        } else if (period == "W") {
            start = end.addDays(-count * 7 * 2);
        } else if (period == "M") {
            start = end.addMonths(-count * 2);
        } else {
            start = end.addSecs(-count * 3600 * 24);  // 分钟线
        }
        
        return getBars(symbol, period, start, end);
    }

    // ========== 分时数据 ==========
    
    Result<QVector<Tick>> getTicks(
        const QString& symbol,
        const QDateTime& start,
        const QDateTime& end) override 
    {
        // TODO: 实现
        Q_UNUSED(symbol)
        Q_UNUSED(start)
        Q_UNUSED(end)
        return Result<QVector<Tick>>::error("NOT_IMPLEMENTED", "Ticks not implemented");
    }

    // ========== 板块数据 ==========
    
    Result<QVector<Sector>> getSectors() override {
        // TODO: 实现
        return Result<QVector<Sector>>::error("NOT_IMPLEMENTED", "Sectors not implemented");
    }
    
    Result<QStringList>> getSectorStocks(const QString& sectorCode) override {
        Q_UNUSED(sectorCode)
        return Result<QStringList>>::error("NOT_IMPLEMENTED", "Sector stocks not implemented");
    }

    // ========== 新闻数据 ==========
    
    Result<QVector<News>> getNews(int count, const QString& category) override {
        Q_UNUSED(count)
        Q_UNUSED(category)
        return Result<QVector<News>>::error("NOT_IMPLEMENTED", "News not implemented");
    }
    
    Result<QVector<News>> getStockNews(const QString& symbol, int count) override {
        Q_UNUSED(symbol)
        Q_UNUSED(count)
        return Result<QVector<News>>::error("NOT_IMPLEMENTED", "Stock news not implemented");
    }

    // ========== 排行数据 ==========
    
    Result<QVector<Quote>> getTopGainers(int count) override {
        Q_UNUSED(count)
        // TODO: 实现
        return Result<QVector<Quote>>::error("NOT_IMPLEMENTED", "Top gainers not implemented");
    }
    
    Result<QVector<Quote>> getTopLosers(int count) override {
        Q_UNUSED(count)
        return Result<QVector<Quote>>::error("NOT_IMPLEMENTED", "Top losers not implemented");
    }
    
    Result<QVector<Quote>> getTopVolume(int count) override {
        Q_UNUSED(count)
        return Result<QVector<Quote>>::error("NOT_IMPLEMENTED", "Top volume not implemented");
    }

private:
    bool m_initialized = false;
};

} // namespace WealthPilot

#endif // WEALTHPILOT_AKSHARE_DATA_SOURCE_H
