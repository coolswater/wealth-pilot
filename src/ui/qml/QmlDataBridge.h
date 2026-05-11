/**
 * @file QmlDataBridge.h
 * @brief QML 数据桥接类 - 连接 C++ 数据模型和 QML 视图
 *
 * @details 实现功能：
 * - K线数据模型暴露
 * - 分时数据模型暴露
 * - 实时行情数据暴露
 * - 主题配置暴露
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef QMLDATABRIDGE_H
#define QMLDATABRIDGE_H

#include <QObject>
#include <QAbstractListModel>
#include <QDateTime>
#include <QColor>
#include "core/types/MarketTypes.h"  // 包含完整类型定义

// 前向声明不再需要，因为已包含 MarketTypes.h

/**
 * @brief K线数据模型（QML）
 */
class KLineQmlModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles {
        TimestampRole = Qt::UserRole + 1,
        OpenRole,
        HighRole,
        LowRole,
        CloseRole,
        VolumeRole,
        Ma5Role,
        Ma10Role,
        Ma20Role
    };

    explicit KLineQmlModel(QObject* parent = nullptr);
    ~KLineQmlModel() override = default;

    // QAbstractListModel 接口
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // 数据管理
    Q_INVOKABLE void setData(const QVector<KLineData>& data);
    Q_INVOKABLE void appendData(const KLineData& data);
    Q_INVOKABLE void updateLastData(const KLineData& data);
    Q_INVOKABLE void clear();
    
    // QML 便捷方法 - 返回单条数据的 QVariantMap
    Q_INVOKABLE QVariantMap get(int index) const;

signals:
    void countChanged();

private:
    QVector<KLineData> m_data;
};

/**
 * @brief 分时数据模型（QML）
 */
class TimeShareQmlModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(double basePrice READ basePrice WRITE setBasePrice NOTIFY basePriceChanged)

public:
    enum Roles {
        TimeRole = Qt::UserRole + 1,
        PriceRole,
        AvgPriceRole,
        VolumeRole
    };

    explicit TimeShareQmlModel(QObject* parent = nullptr);
    ~TimeShareQmlModel() override = default;

    // QAbstractListModel 接口
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // 属性
    double basePrice() const { return m_basePrice; }
    void setBasePrice(double price);

    // 数据管理
    Q_INVOKABLE void setData(const QVector<TimeShareData>& data);
    Q_INVOKABLE void appendData(const TimeShareData& data);
    Q_INVOKABLE void clear();
    
    // QML 便捷方法
    Q_INVOKABLE QVariantMap get(int index) const;

signals:
    void countChanged();
    void basePriceChanged();

private:
    QVector<TimeShareData> m_data;
    double m_basePrice = 0.0;
};

/**
 * @brief 实时行情数据（QML）
 */
class RealtimeQuoteQml : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString symbol READ symbol NOTIFY symbolChanged)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(double price READ price NOTIFY priceChanged)
    Q_PROPERTY(double change READ change NOTIFY changeChanged)
    Q_PROPERTY(double changePercent READ changePercent NOTIFY changePercentChanged)
    Q_PROPERTY(qint64 volume READ volume NOTIFY volumeChanged)
    Q_PROPERTY(double turnover READ turnover NOTIFY turnoverChanged)
    Q_PROPERTY(double open READ open NOTIFY openChanged)
    Q_PROPERTY(double high READ high NOTIFY highChanged)
    Q_PROPERTY(double low READ low NOTIFY lowChanged)
    Q_PROPERTY(double preClose READ preClose NOTIFY preCloseChanged)

public:
    explicit RealtimeQuoteQml(QObject* parent = nullptr);

    // 属性访问器
    QString symbol() const { return m_symbol; }
    QString name() const { return m_name; }
    double price() const { return m_price; }
    double change() const { return m_change; }
    double changePercent() const { return m_changePercent; }
    qint64 volume() const { return m_volume; }
    double turnover() const { return m_turnover; }
    double open() const { return m_open; }
    double high() const { return m_high; }
    double low() const { return m_low; }
    double preClose() const { return m_preClose; }

    // 数据更新
    void updateData(const QString& symbol, const QString& name,
                   double price, double change, double changePercent,
                   qint64 volume, double turnover,
                   double open, double high, double low, double preClose);

signals:
    void symbolChanged();
    void nameChanged();
    void priceChanged();
    void changeChanged();
    void changePercentChanged();
    void volumeChanged();
    void turnoverChanged();
    void openChanged();
    void highChanged();
    void lowChanged();
    void preCloseChanged();

private:
    QString m_symbol;
    QString m_name;
    double m_price = 0.0;
    double m_change = 0.0;
    double m_changePercent = 0.0;
    qint64 m_volume = 0;
    double m_turnover = 0.0;
    double m_open = 0.0;
    double m_high = 0.0;
    double m_low = 0.0;
    double m_preClose = 0.0;
};

/**
 * @brief QML 数据桥接管理器
 */
class QmlDataBridge : public QObject {
    Q_OBJECT

public:
    static QmlDataBridge* instance();

    // 注册到 QML
    static void registerQmlTypes();

    // 获取模型
    Q_INVOKABLE KLineQmlModel* klineModel() { return m_klineModel; }
    Q_INVOKABLE TimeShareQmlModel* timeShareModel() { return m_timeShareModel; }
    Q_INVOKABLE RealtimeQuoteQml* realtimeQuote() { return m_realtimeQuote; }

private:
    explicit QmlDataBridge(QObject* parent = nullptr);
    ~QmlDataBridge() override = default;

    KLineQmlModel* m_klineModel;
    TimeShareQmlModel* m_timeShareModel;
    RealtimeQuoteQml* m_realtimeQuote;
};

#endif // QMLDATABRIDGE_H
