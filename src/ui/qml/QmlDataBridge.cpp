/**
 * @file QmlDataBridge.cpp
 * @brief QML 数据桥接实现
 */

#include "QmlDataBridge.h"
#include "../../market/StockDataSource.h"
#include <QQmlEngine>
#include <QJSEngine>

// ========== KLineQmlModel ==========

KLineQmlModel::KLineQmlModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int KLineQmlModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return m_data.size();
}

QVariant KLineQmlModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_data.size())
        return QVariant();

    const auto& kline = m_data[index.row()];

    switch (role) {
        case TimestampRole:
            return kline.time.toMSecsSinceEpoch();
        case OpenRole:
            return kline.open;
        case HighRole:
            return kline.high;
        case LowRole:
            return kline.low;
        case CloseRole:
            return kline.close;
        case VolumeRole:
            return kline.volume;
        case Ma5Role:
            return QVariant(); // TODO: 计算MA5
        case Ma10Role:
            return QVariant(); // TODO: 计算MA10
        case Ma20Role:
            return QVariant(); // TODO: 计算MA20
        default:
            return QVariant();
    }
}

QHash<int, QByteArray> KLineQmlModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[TimestampRole] = "timestamp";
    roles[OpenRole] = "open";
    roles[HighRole] = "high";
    roles[LowRole] = "low";
    roles[CloseRole] = "close";
    roles[VolumeRole] = "volume";
    roles[Ma5Role] = "ma5";
    roles[Ma10Role] = "ma10";
    roles[Ma20Role] = "ma20";
    return roles;
}

void KLineQmlModel::setData(const QVector<KLineData>& data)
{
    beginResetModel();
    m_data = data;
    endResetModel();
    emit countChanged();
}

void KLineQmlModel::appendData(const KLineData& data)
{
    beginInsertRows(QModelIndex(), m_data.size(), m_data.size());
    m_data.append(data);
    endInsertRows();
    emit countChanged();
}

void KLineQmlModel::updateLastData(const KLineData& data)
{
    if (m_data.isEmpty()) return;
    
    m_data.last() = data;
    QModelIndex idx = index(m_data.size() - 1);
    emit dataChanged(idx, idx);
}

void KLineQmlModel::clear()
{
    beginResetModel();
    m_data.clear();
    endResetModel();
    emit countChanged();
}

// ========== TimeShareQmlModel ==========

TimeShareQmlModel::TimeShareQmlModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int TimeShareQmlModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return m_data.size();
}

QVariant TimeShareQmlModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_data.size())
        return QVariant();

    const auto& point = m_data[index.row()];

    switch (role) {
        case TimeRole:
            return point.time;
        case PriceRole:
            return point.price;
        case AvgPriceRole:
            return point.avgPrice;
        case VolumeRole:
            return point.volume;
        default:
            return QVariant();
    }
}

QHash<int, QByteArray> TimeShareQmlModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[TimeRole] = "time";
    roles[PriceRole] = "price";
    roles[AvgPriceRole] = "avgPrice";
    roles[VolumeRole] = "volume";
    return roles;
}

void TimeShareQmlModel::setBasePrice(double price)
{
    if (m_basePrice != price) {
        m_basePrice = price;
        emit basePriceChanged();
    }
}

void TimeShareQmlModel::setData(const QVector<TimeShareData>& data)
{
    beginResetModel();
    m_data = data;
    endResetModel();
    emit countChanged();
}

void TimeShareQmlModel::appendData(const TimeShareData& data)
{
    beginInsertRows(QModelIndex(), m_data.size(), m_data.size());
    m_data.append(data);
    endInsertRows();
    emit countChanged();
}

void TimeShareQmlModel::clear()
{
    beginResetModel();
    m_data.clear();
    endResetModel();
    emit countChanged();
}

// ========== RealtimeQuoteQml ==========

RealtimeQuoteQml::RealtimeQuoteQml(QObject* parent)
    : QObject(parent)
{
}

void RealtimeQuoteQml::updateData(const QString& symbol, const QString& name,
                                  double price, double change, double changePercent,
                                  qint64 volume, double turnover,
                                  double open, double high, double low, double preClose)
{
    if (m_symbol != symbol) {
        m_symbol = symbol;
        emit symbolChanged();
    }
    if (m_name != name) {
        m_name = name;
        emit nameChanged();
    }
    if (m_price != price) {
        m_price = price;
        emit priceChanged();
    }
    if (m_change != change) {
        m_change = change;
        emit changeChanged();
    }
    if (m_changePercent != changePercent) {
        m_changePercent = changePercent;
        emit changePercentChanged();
    }
    if (m_volume != volume) {
        m_volume = volume;
        emit volumeChanged();
    }
    if (m_turnover != turnover) {
        m_turnover = turnover;
        emit turnoverChanged();
    }
    if (m_open != open) {
        m_open = open;
        emit openChanged();
    }
    if (m_high != high) {
        m_high = high;
        emit highChanged();
    }
    if (m_low != low) {
        m_low = low;
        emit lowChanged();
    }
    if (m_preClose != preClose) {
        m_preClose = preClose;
        emit preCloseChanged();
    }
}

// ========== QmlDataBridge ==========

QmlDataBridge* QmlDataBridge::instance()
{
    static QmlDataBridge* inst = new QmlDataBridge();
    return inst;
}

QmlDataBridge::QmlDataBridge(QObject* parent)
    : QObject(parent)
    , m_klineModel(new KLineQmlModel(this))
    , m_timeShareModel(new TimeShareQmlModel(this))
    , m_realtimeQuote(new RealtimeQuoteQml(this))
{
}

void QmlDataBridge::registerQmlTypes()
{
    // 注册数据模型
    qmlRegisterSingletonType<QmlDataBridge>(
        "WealthPilot.Core", 1, 0, "DataBridge",
        [](QQmlEngine* engine, QJSEngine*) -> QObject* {
            return QmlDataBridge::instance();
        }
    );
    
    qmlRegisterType<KLineQmlModel>("WealthPilot.Models", 1, 0, "KLineModel");
    qmlRegisterType<TimeShareQmlModel>("WealthPilot.Models", 1, 0, "TimeShareModel");
    qmlRegisterType<RealtimeQuoteQml>("WealthPilot.Models", 1, 0, "RealtimeQuote");
}