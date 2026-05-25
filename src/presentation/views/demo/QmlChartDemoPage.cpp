/**
 * @file QmlChartDemoPage.cpp
 * @brief QML 图表演示页面实现
 */

#include "QmlChartDemoPage.h"
#include "presentation/components/QmlKLineWidget.h"
#include "shared/types/MarketTypes.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QRandomGenerator>
#include <QDebug>

QmlChartDemoPage::QmlChartDemoPage(QWidget* parent)
    : BasePage(parent)
{
    setupUI();
    setupConnections();
}

void QmlChartDemoPage::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // 工具栏
    auto* toolbar = new QWidget(this);
    auto* toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);

    // 图表类型选择
    auto* typeLabel = new QLabel(QStringLiteral("图表类型:"), this);
    m_chartTypeCombo = new QComboBox(this);
    m_chartTypeCombo->addItem(QStringLiteral("K线图"), static_cast<int>(QmlKLineWidget::ChartType::KLine));
    m_chartTypeCombo->addItem(QStringLiteral("分时图"), static_cast<int>(QmlKLineWidget::ChartType::TimeShare));

    // 按钮
    m_loadDataBtn = new QPushButton(QStringLiteral("加载演示数据"), this);
    m_realtimeBtn = new QPushButton(QStringLiteral("开始实时更新"), this);
    m_realtimeBtn->setCheckable(true);

    // 状态标签
    m_statusLabel = new QLabel(QStringLiteral("就绪"), this);
    m_statusLabel->setStyleSheet("color: #9CA3AF;");

    toolbarLayout->addWidget(typeLabel);
    toolbarLayout->addWidget(m_chartTypeCombo);
    toolbarLayout->addSpacing(20);
    toolbarLayout->addWidget(m_loadDataBtn);
    toolbarLayout->addWidget(m_realtimeBtn);
    toolbarLayout->addStretch();
    toolbarLayout->addWidget(m_statusLabel);

    // 图表区域
    m_chartWidget = new QmlKLineWidget(this);
    m_chartWidget->setMinimumSize(800, 500);

    mainLayout->addWidget(toolbar);
    mainLayout->addWidget(m_chartWidget, 1);

    // 应用样式
    toolbar->setStyleSheet(R"(
        QWidget {
            background: #111827;
        }
        QLabel {
            color: #E6EDF3;
            font-size: 13px;
        }
        QComboBox {
            background: #1A2332;
            border: 1px solid #2D3748;
            border-radius: 4px;
            padding: 5px 10px;
            color: #E6EDF3;
            min-width: 100px;
        }
        QComboBox::drop-down {
            border: none;
        }
        QComboBox::down-arrow {
            image: none;
            border-left: 5px solid transparent;
            border-right: 5px solid transparent;
            border-top: 5px solid #9CA3AF;
            margin-right: 5px;
        }
        QPushButton {
            background: #3B82F6;
            border: none;
            border-radius: 4px;
            padding: 6px 16px;
            color: white;
            font-size: 13px;
        }
        QPushButton:hover {
            background: #2563EB;
        }
        QPushButton:pressed {
            background: #1D4ED8;
        }
        QPushButton:checked {
            background: #EF4444;
        }
    )");
}

void QmlChartDemoPage::setupConnections()
{
    connect(m_chartTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &QmlChartDemoPage::onChartTypeChanged);
    connect(m_loadDataBtn, &QPushButton::clicked,
            this, &QmlChartDemoPage::onLoadDemoData);
    connect(m_realtimeBtn, &QPushButton::toggled,
            this, &QmlChartDemoPage::onGenerateRealtimeData);

    connect(m_chartWidget, &QmlKLineWidget::chartLoaded, this, [this]() {
        m_statusLabel->setText(QStringLiteral("图表加载完成"));
    });
}

void QmlChartDemoPage::onChartTypeChanged(int index)
{
    auto type = static_cast<QmlKLineWidget::ChartType>(m_chartTypeCombo->itemData(index).toInt());
    m_chartWidget->setChartType(type);
    m_statusLabel->setText(QStringLiteral("切换到: ") + m_chartTypeCombo->currentText());
}

void QmlChartDemoPage::onLoadDemoData()
{
    int chartType = m_chartTypeCombo->currentData().toInt();

    if (chartType == static_cast<int>(QmlKLineWidget::ChartType::KLine)) {
        loadDemoKLineData();
    } else {
        loadDemoTimeShareData();
    }
}

void QmlChartDemoPage::onGenerateRealtimeData()
{
    m_realtimeEnabled = m_realtimeBtn->isChecked();

    if (m_realtimeEnabled) {
        m_realtimeBtn->setText(QStringLiteral("停止实时更新"));
        m_statusLabel->setText(QStringLiteral("实时更新中..."));
        // TODO: 启动定时器更新数据
    } else {
        m_realtimeBtn->setText(QStringLiteral("开始实时更新"));
        m_statusLabel->setText(QStringLiteral("已停止实时更新"));
    }
}

void QmlChartDemoPage::loadDemoKLineData()
{
    m_statusLabel->setText(QStringLiteral("加载K线数据..."));

    // 生成演示数据
    QVector<KLineData> data;
    data.reserve(200);

    QDateTime time = QDateTime::currentDateTime().addDays(-200);
    double basePrice = 100.0;
    double price = basePrice;

    QRandomGenerator* rng = QRandomGenerator::global();

    for (int i = 0; i < 200; ++i) {
        KLineData kline;
        kline.time = time;

        // 随机波动
        double change = (rng->bounded(100) - 50) / 100.0 * 3.0;  // -1.5% ~ +1.5%
        double open = price;
        double close = price * (1 + change / 100.0);
        double high = qMax(open, close) * (1 + rng->bounded(100) / 1000.0);  // +0~0.1%
        double low = qMin(open, close) * (1 - rng->bounded(100) / 1000.0);  // -0~0.1%
        qint64 volume = 100000 + rng->bounded(900000);

        kline.open = open;
        kline.high = high;
        kline.low = low;
        kline.close = close;
        kline.volume = volume;

        data.append(kline);

        price = close;
        time = time.addDays(1);
    }

    m_chartWidget->setKLineData(data);
    m_statusLabel->setText(QStringLiteral("已加载 %1 条K线数据").arg(data.size()));
}

void QmlChartDemoPage::loadDemoTimeShareData()
{
    m_statusLabel->setText(QStringLiteral("加载分时数据..."));

    QVector<TimeShareData> data;
    data.reserve(240);

    QDateTime time = QDateTime::currentDateTime();
    time.setTime(QTime(9, 30, 0));

    double basePrice = 100.0;
    double price = basePrice;
    double totalVolume = 0;
    double totalAmount = 0;

    QRandomGenerator* rng = QRandomGenerator::global();

    // 生成一天的分时数据（每分钟一条，共240条）
    for (int i = 0; i < 240; ++i) {
        TimeShareData point;
        point.time = time;

        // 随机波动
        double change = (rng->bounded(100) - 50) / 1000.0;  // -0.05 ~ +0.05
        price = basePrice * (1 + change);

        qint64 volume = 10000 + rng->bounded(50000);
        totalVolume += volume;
        totalAmount += price * volume;

        point.price = price;
        point.avgPrice = totalAmount / totalVolume;
        point.volume = volume;

        data.append(point);

        time = time.addSecs(60);

        // 跳过中午休市
        if (time.time().hour() == 11 && time.time().minute() == 30) {
            time = time.addSecs(90 * 60);  // 跳到13:00
        }
    }

    m_chartWidget->setTimeShareData(data, basePrice);
    m_statusLabel->setText(QStringLiteral("已加载 %1 条分时数据").arg(data.size()));
}

void QmlChartDemoPage::generateRandomKLine(int count)
{
    Q_UNUSED(count);
    // TODO: 实现随机K线生成
}
