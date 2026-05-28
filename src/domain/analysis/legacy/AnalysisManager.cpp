/**
 * @file AnalysisManager.cpp
 * @brief 技术分析管理器实现
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "AnalysisManager.h"
#include "elliottwave/ElliottWaveAnalyzer.h"
#include "dowtheory/DowTheoryAnalyzer.h"
#include "volumepattern/VolumePatternAnalyzer.h"
#include "chanlun/ChanLunAnalyzer.h"
#include <QDebug>

#include "utils/Logger.h"

namespace WealthPilot {
namespace Analysis {

struct AnalysisManager::Impl {
    QMap<TheoryType, IAnalyzer*> analyzers;
    SignalFilter* filter = nullptr;
    SignalService* service = nullptr;

    QMap<QString, CompositeSignal> lastSignals;
};

AnalysisManager* AnalysisManager::instance()
{
    static AnalysisManager instance;
    return &instance;
}

AnalysisManager::AnalysisManager(QObject* parent)
    : QObject(parent)
    , d(std::make_unique<Impl>())
{
}

AnalysisManager::~AnalysisManager()
{
    // 清理分析器
    for (auto* analyzer : d->analyzers) {
        delete analyzer;
    }

    delete d->filter;
    delete d->service;
}

void AnalysisManager::initialize()
{
    // 创建信号过滤器
    d->filter = new SignalFilter(this);

    // 创建信号服务
    d->service = new SignalService(this);
    d->service->setSignalFilter(d->filter);
    d->service->initialize();

    // 注册分析器
    registerAnalyzers();

    // 设置信号连接
    setupSignalConnections();

    LOG_DEBUG("AnalysisManager initialized");
}

void AnalysisManager::registerAnalyzers()
{
    // 注册波浪理论分析器
    auto* elliottWave = new ElliottWave::ElliottWaveAnalyzer(this);
    d->analyzers[TheoryType::ElliottWave] = elliottWave;
    d->service->registerAnalyzer(elliottWave);

    // 注册缠论分析器
    auto* chanLun = new ChanLun::ChanLunAnalyzer(this);
    d->analyzers[TheoryType::ChanLun] = chanLun;
    d->service->registerAnalyzer(chanLun);

    // 注册道氏理论分析器
    auto* dowTheory = new DowTheory::DowTheoryAnalyzer(this);
    d->analyzers[TheoryType::DowTheory] = dowTheory;
    d->service->registerAnalyzer(dowTheory);

    // 注册量价形态分析器
    auto* volumePattern = new VolumePattern::VolumePatternAnalyzer(this);
    d->analyzers[TheoryType::VolumePattern] = volumePattern;
    d->service->registerAnalyzer(volumePattern);

    LOG_DEBUG(QString("Registered %1 analyzers").arg(d->analyzers.size()));
}

void AnalysisManager::setupSignalConnections()
{
    // 连接信号服务的信号
    connect(d->service, &SignalService::compositeSignalGenerated,
            this, [this](const CompositeSignal& signal) {
        d->lastSignals[signal.symbol] = signal;

        if (signal.isStrongSignal()) {
            emit strongSignalFound(signal.symbol, signal);
        }

        emit analysisCompleted(signal.symbol, signal);
    });

    connect(d->service, &SignalService::alertTriggered,
            this, [this](const QString& symbol, const CompositeSignal& signal) {
        LOG_DEBUG(QString("Alert for %1 : %2").arg(symbol, signal.description));
    });
}

CompositeSignal AnalysisManager::analyze(const QString& symbol, const QVector<KLine>& klines)
{
    // 使用信号服务进行分析
    d->service->analyzeSymbol(symbol, klines);

    // 返回最新的综合信号
    return d->lastSignals.value(symbol);
}

IAnalyzer* AnalysisManager::getAnalyzer(TheoryType type)
{
    return d->analyzers.value(type, nullptr);
}

SignalFilter* AnalysisManager::signalFilter()
{
    return d->filter;
}

SignalService* AnalysisManager::signalService()
{
    return d->service;
}

void AnalysisManager::setAnalysisParams(TheoryType type, const QMap<QString, QVariant>& params)
{
    Q_UNUSED(type)
    Q_UNUSED(params)
    // 可以根据需要设置各分析器的参数
}

QMap<QString, QVariant> AnalysisManager::getAnalysisSummary(const QString& symbol)
{
    QMap<QString, QVariant> summary;

    auto signal = d->lastSignals.value(symbol);

    summary["symbol"] = symbol;
    summary["direction"] = static_cast<int>(signal.direction);
    summary["confidence"] = signal.confidence;
    summary["theory_count"] = signal.theoryCount;
    summary["is_strong"] = signal.isStrongSignal();
    summary["description"] = signal.description;

    // 添加各理论详情
    QStringList theoryDetails;
    for (const auto& srcSignal : signal.sourceSignals) {
        theoryDetails << QString("%1: %2 (%3%)")
            .arg(srcSignal.theoryName())
            .arg(srcSignal.directionText())
            .arg(srcSignal.confidence, 0, 'f', 1);
    }
    summary["theory_details"] = theoryDetails.join("; ");

    return summary;
}

} // namespace Analysis
} // namespace WealthPilot
