/**
 * @file DataSourceTest.cpp
 * @brief 数据源测试示例
 *
 * 演示如何使用StockDataSource获取真实行情数据
 */

#include "market/StockDataSource.h"
#include "utils/Logger.h"
#include <QCoreApplication>
#include <QTimer>

using namespace WealthPilot;

/**
 * @brief 数据源测试类
 */
class DataSourceTest : public QObject
{
    Q_OBJECT

public:
    DataSourceTest() : m_dataSource(nullptr) {}

public slots:
    void run()
    {
        LOG_INFO("=== 数据源测试开始 ===");

        // 创建数据源（使用新浪财经）
        m_dataSource = new StockDataSource(StockDataSource::Source::Sina, this);

        // 连接信号
        connect(m_dataSource, &StockDataSource::quotesReceived,
                this, &DataSourceTest::onQuotesReceived);
        connect(m_dataSource, &StockDataSource::kLineReceived,
                this, &DataSourceTest::onKLineReceived);
        connect(m_dataSource, &StockDataSource::errorOccurred,
                this, &DataSourceTest::onError);

        // 测试1: 请求实时行情
        LOG_INFO("测试1: 请求实时行情");
        QStringList symbols = {"sh600000", "sh600519", "sz000001"};
        m_dataSource->requestQuotes(symbols);

        // 测试2: 请求K线数据（延迟2秒执行）
        QTimer::singleShot(2000, this, [this]() {
            LOG_INFO("测试2: 请求K线数据");
            m_dataSource->requestKLine("sh600000", KLinePeriod::Day1, 100);
        });

        // 测试3: 启动自动刷新（延迟4秒执行）
        QTimer::singleShot(4000, this, [this]() {
            LOG_INFO("测试3: 启动自动刷新（5秒间隔）");
            m_dataSource->startAutoRefresh(5000);
        });

        // 10秒后退出
        QTimer::singleShot(10000, qApp, &QCoreApplication::quit);
    }

private slots:
    void onQuotesReceived(const QVector<StockQuote>& quotes)
    {
        LOG_INFO(QString("收到 %1 条行情数据").arg(quotes.size()));

        for (const auto& quote : quotes) {
            LOG_INFO(QString("股票: %1 (%2)")
                .arg(quote.name, quote.symbol));
            LOG_INFO(QString("  最新价: %1, 涨跌幅: %2%%")
                .arg(quote.lastPrice).arg(quote.changePercent));
            LOG_INFO(QString("  开盘: %1, 最高: %2, 最低: %3, 昨收: %4")
                .arg(quote.openPrice).arg(quote.highPrice)
                .arg(quote.lowPrice).arg(quote.preClose));
            LOG_INFO(QString("  成交量: %1, 成交额: %2")
                .arg(quote.volume).arg(quote.turnover));
        }
    }

    void onKLineReceived(const QString& symbol, const QVector<KLineData>& data)
    {
        LOG_INFO(QString("收到 %1 条K线数据，股票: %2").arg(data.size()).arg(symbol));

        if (!data.isEmpty()) {
            // 显示最新的5条K线
            int start = qMax(0, data.size() - 5);
            for (int i = start; i < data.size(); ++i) {
                const auto& kline = data[i];
                LOG_INFO(QString("K线[%1]: 时间=%2, 开=%3, 高=%4, 低=%5, 收=%6, 量=%7")
                    .arg(i)
                    .arg(kline.time.toString("yyyy-MM-dd"))
                    .arg(kline.open).arg(kline.high)
                    .arg(kline.low).arg(kline.close)
                    .arg(kline.volume));
            }
        }
    }

    void onError(const QString& error)
    {
        LOG_ERROR(QString("数据源错误: %1").arg(error));
    }

private:
    StockDataSource* m_dataSource;
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    DataSourceTest test;
    QTimer::singleShot(0, &test, &DataSourceTest::run);

    return app.exec();
}

#include "DataSourceTest.moc"
