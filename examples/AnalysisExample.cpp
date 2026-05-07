/**
 * @file AnalysisExample.cpp
 * @brief 技术分析系统使用示例
 *
 * @details 演示如何使用技术分析系统进行实时行情分析
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "analysis/AnalysisManager.h"
#include "analysis/AnalysisTypes.h"
#include <QCoreApplication>
#include <QDebug>
#include <QTimer>

using namespace WealthPilot::Analysis;

/**
 * @brief 生成模拟K线数据
 */
QVector<KLine> generateMockKlines(int count = 100)
{
    QVector<KLine> klines;
    double price = 3500.0; // 初始价格

    for (int i = 0; i < count; ++i) {
        KLine kline;
        kline.time = QDateTime::currentDateTime().addSecs(-count * 60 + i * 60);

        // 模拟价格波动
        double change = (qrand() % 100 - 50) / 100.0;
        price += change;

        kline.open = price;
        kline.high = price + qrand() % 10;
        kline.low = price - qrand() % 10;
        kline.close = price + (qrand() % 20 - 10);
        kline.volume = 10000 + qrand() % 50000;

        klines.append(kline);
    }

    return klines;
}

/**
 * @brief 打印信号详情
 */
void printSignal(const UnifiedSignal& signal)
{
    qDebug() << "=== 单理论信号 ===";
    qDebug() << "理论:" << signal.theoryName();
    qDebug() << "方向:" << signal.directionText();
    qDebug() << "强度:" << signal.strengthText();
    qDebug() << "置信度:" << QString::number(signal.confidence, 'f', 1) << "%";
    qDebug() << "描述:" << signal.description;
    qDebug() << "";
}

/**
 * @brief 打印综合信号详情
 */
void printCompositeSignal(const CompositeSignal& signal)
{
    qDebug() << "╔══════════════════════════════════════════════════╗";
    qDebug() << "║              综合交易信号分析结果                  ║";
    qDebug() << "╚══════════════════════════════════════════════════╝";
    qDebug() << "";
    qDebug() << "标的代码:" << signal.symbol;
    qDebug() << "信号时间:" << signal.time.toString("yyyy-MM-dd hh:mm:ss");
    qDebug() << "信号价格:" << QString::number(signal.price, 'f', 2);
    qDebug() << "";
    qDebug() << "【综合判断】";
    qDebug() << "信号方向:" << (signal.direction == SignalDirection::Bullish ? "看涨 📈" :
                              signal.direction == SignalDirection::Bearish ? "看跌 📉" : "中性 ➡️");
    qDebug() << "置信度:" << QString::number(signal.confidence, 'f', 1) << "%";
    qDebug() << "支持理论数:" << signal.theoryCount << "/ 4";
    qDebug() << "综合得分:" << QString::number(signal.score(), 'f', 1) << "分";
    qDebug() << "信号强度:" << (signal.isStrongSignal() ? "强信号 ⭐⭐⭐" : "普通信号");
    qDebug() << "";
    qDebug() << "【信号描述】";
    qDebug() << signal.description;
    qDebug() << "";

    if (!signal.sourceSignals.isEmpty()) {
        qDebug() << "【各理论分析详情】";
        for (const auto& srcSignal : signal.sourceSignals) {
            qDebug() << QString("  • %1: %2 (置信度: %3%)")
                .arg(srcSignal.theoryName())
                .arg(srcSignal.directionText())
                .arg(srcSignal.confidence, 0, 'f', 1);
        }
        qDebug() << "";
    }

    qDebug() << "【风险提示】";
    if (signal.confidence >= 80) {
        qDebug() << "  ✓ 高质量信号，可考虑入场";
    } else if (signal.confidence >= 60) {
        qDebug() << "  ⚠ 中等质量信号，建议结合其他指标";
    } else {
        qDebug() << "  ✗ 信号质量较低，建议观望";
    }
    qDebug() << "";
}

/**
 * @brief 主函数
 */
int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    qDebug() << "╔══════════════════════════════════════════════════╗";
    qDebug() << "║      WealthPilot 技术分析系统 - 使用示例          ║";
    qDebug() << "╚══════════════════════════════════════════════════╝";
    qDebug() << "";

    // 1. 初始化分析管理器
    qDebug() << "【步骤1】初始化分析管理器...";
    auto* manager = AnalysisManager::instance();
    manager->initialize();
    qDebug() << "✓ 初始化完成";
    qDebug() << "";

    // 2. 连接信号
    qDebug() << "【步骤2】连接信号处理...";
    QObject::connect(manager, &AnalysisManager::analysisCompleted,
        [](const QString& symbol, const CompositeSignal& signal) {
            qDebug() << QString("✓ %1 分析完成").arg(symbol);
        });

    QObject::connect(manager, &AnalysisManager::strongSignalFound,
        [](const QString& symbol, const CompositeSignal& signal) {
            qDebug() << "";
            qDebug() << "★★★★★ 发现强信号！★★★★★";
            qDebug() << QString("标的: %1").arg(symbol);
            qDebug() << QString("方向: %1").arg(signal.directionText());
            qDebug() << QString("置信度: %1%").arg(signal.confidence, 0, 'f', 1);
            qDebug() << "";
        });
    qDebug() << "✓ 信号连接完成";
    qDebug() << "";

    // 3. 生成模拟数据
    qDebug() << "【步骤3】生成模拟K线数据...";
    QString symbol = "IF2501"; // 沪深300股指期货
    auto klines = generateMockKlines(100);
    qDebug() << QString("✓ 已生成 %1 根K线数据").arg(klines.size());
    qDebug() << "";

    // 4. 执行分析
    qDebug() << "【步骤4】执行技术分析...";
    qDebug() << QString("分析标的: %1").arg(symbol);
    qDebug() << "";

    CompositeSignal signal = manager->analyze(symbol, klines);

    // 5. 打印分析结果
    printCompositeSignal(signal);

    // 6. 获取详细分析摘要
    qDebug() << "【步骤5】获取分析摘要...";
    auto summary = manager->getAnalysisSummary(symbol);
    qDebug() << "分析摘要:";
    for (auto it = summary.begin(); it != summary.end(); ++it) {
        qDebug() << QString("  %1: %2").arg(it.key()).arg(it.value().toString());
    }
    qDebug() << "";

    // 7. 演示订阅功能
    qDebug() << "【步骤6】演示信号订阅...";
    SignalSubscription subscription;
    subscription.symbol = symbol;
    subscription.theory = TheoryType::ChanLun; // 订阅所有理论
    subscription.minConfidence = 70.0;
    subscription.enableAlert = true;
    subscription.alertChannel = "default";

    manager->signalService()->addSubscription(subscription);
    qDebug() << QString("✓ 已添加订阅: %1").arg(symbol);
    qDebug() << "";

    // 8. 演示预警配置
    qDebug() << "【步骤7】配置预警...";
    AlertConfig alertConfig;
    alertConfig.enabled = true;
    alertConfig.minConfidence = 75.0;
    alertConfig.minTheoryCount = 3;
    alertConfig.alertOnStrongSignals = true;

    manager->signalService()->setAlertConfig(alertConfig);
    qDebug() << "✓ 预警配置完成";
    qDebug() << "";

    // 9. 演示K线图集成
    qDebug() << "【步骤8】获取K线图指标数据...";
    auto indicatorData = manager->signalService()->getIndicatorData(symbol);
    auto overlayConfig = manager->signalService()->getOverlayConfig(symbol);

    qDebug() << "指标数据:";
    qDebug() << QString("  信号方向: %1").arg(overlayConfig["signal_direction"].toInt());
    qDebug() << QString("  置信度: %1%").arg(overlayConfig["signal_confidence"].toDouble(), 0, 'f', 1);
    qDebug() << QString("  标记颜色: %1").arg(overlayConfig["marker_color"].toString());
    qDebug() << QString("  标记形状: %1").arg(overlayConfig["marker_shape"].toString());
    qDebug() << "";

    // 10. 演示单独使用各理论分析器
    qDebug() << "【步骤9】单独使用各理论分析器...";
    qDebug() << "";

    // 波浪理论
    auto* elliottWave = manager->getAnalyzer(TheoryType::ElliottWave);
    if (elliottWave) {
        auto result = elliottWave->analyze(klines);
        if (result.isValid && !result.signals.isEmpty()) {
            printSignal(result.signals.first());
        }
    }

    // 道氏理论
    auto* dowTheory = manager->getAnalyzer(TheoryType::DowTheory);
    if (dowTheory) {
        auto result = dowTheory->analyze(klines);
        if (result.isValid && !result.signals.isEmpty()) {
            printSignal(result.signals.first());
        }
    }

    // 量价形态
    auto* volumePattern = manager->getAnalyzer(TheoryType::VolumePattern);
    if (volumePattern) {
        auto result = volumePattern->analyze(klines);
        if (result.isValid && !result.signals.isEmpty()) {
            printSignal(result.signals.first());
        }
    }

    // 缠论
    auto* chanLun = manager->getAnalyzer(TheoryType::ChanLun);
    if (chanLun) {
        auto result = chanLun->analyze(klines);
        if (result.isValid && !result.signals.isEmpty()) {
            printSignal(result.signals.first());
        }
    }

    qDebug() << "╔══════════════════════════════════════════════════╗";
    qDebug() << "║              示例演示完成                          ║";
    qDebug() << "╚══════════════════════════════════════════════════╝";
    qDebug() << "";
    qDebug() << "提示: 实际使用时，请替换为真实的K线数据源";
    qDebug() << "文档: 参见 docs/analysis-system-guide.md";

    return 0;
}
