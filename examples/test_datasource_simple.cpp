/**
 * @file test_datasource_simple.cpp
 * @brief 简单的数据源测试 - 直接测试网络请求
 */

#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QDebug>
#include <QTimer>
#include <QUrl>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    qDebug() << "=== 测试新浪财经API ===";

    QNetworkAccessManager manager;

    // 测试1: 获取实时行情
    qDebug() << "\n测试1: 获取浦发银行(sh600000)实时行情";
    QUrl quoteUrl("http://hq.sinajs.cn/list=sh600000");
    QNetworkRequest quoteRequest(quoteUrl);
    quoteRequest.setRawHeader("User-Agent", "Mozilla/5.0");
    quoteRequest.setRawHeader("Referer", "http://finance.sina.com.cn");

    QNetworkReply *quoteReply = manager.get(quoteRequest);
    QObject::connect(quoteReply, &QNetworkReply::finished, [&]() {
        if (quoteReply->error() == QNetworkReply::NoError) {
            QString data = QString::fromLocal8Bit(quoteReply->readAll());
            qDebug() << "✅ 成功获取行情数据:";
            qDebug() << data.left(200) << "...";
        } else {
            qDebug() << "❌ 获取行情失败:" << quoteReply->errorString();
        }
        quoteReply->deleteLater();
    });

    // 测试2: 获取K线数据
    QTimer::singleShot(2000, [&]() {
        qDebug() << "\n测试2: 获取浦发银行(sh600000)日K线数据";
        QUrl klineUrl("http://money.finance.sina.com.cn/quotes_service/api/json_v2.php/CN_MarketData.getKLineData?symbol=sh600000&scale=daily&datalen=10");
        QNetworkRequest klineRequest(klineUrl);

        QNetworkReply *klineReply = manager.get(klineRequest);
        QObject::connect(klineReply, &QNetworkReply::finished, [&]() {
            if (klineReply->error() == QNetworkReply::NoError) {
                QString data = QString::fromLocal8Bit(klineReply->readAll());
                qDebug() << "✅ 成功获取K线数据:";
                qDebug() << data.left(300) << "...";

                // 简单解析
                QStringList lines = data.split('\n', Qt::SkipEmptyParts);
                qDebug() << "\n解析结果: 共" << lines.size() << "条K线";
                if (lines.size() > 0) {
                    qDebug() << "第一条K线:" << lines[0];
                    if (lines.size() > 1) {
                        qDebug() << "第二条K线:" << lines[1];
                    }
                }
            } else {
                qDebug() << "❌ 获取K线失败:" << klineReply->errorString();
            }
            klineReply->deleteLater();

            // 退出程序
            QTimer::singleShot(1000, &app, &QCoreApplication::quit);
        });
    });

    return app.exec();
}
