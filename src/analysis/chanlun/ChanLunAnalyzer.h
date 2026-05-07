/**
 * @file ChanLunAnalyzer.h
 * @brief 缠论分析器 - 核心算法实现
 *
 * @details 实现缠论的完整分析流程：
 * 1. K线包含处理
 * 2. 分型识别
 * 3. 笔划分
 * 4. 线段划分
 * 5. 中枢识别
 * 6. 背驰判断
 * 7. 买卖点识别
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef CHANLUN_ANALYZER_H
#define CHANLUN_ANALYZER_H

#include "../IAnalyzer.h"
#include "ChanLunTypes.h"
#include <QObject>
#include <QVector>
#include <memory>

namespace WealthPilot {
namespace ChanLun {

/**
 * @brief 缠论分析器
 *
 * @details 提供缠论完整分析功能，支持增量更新
 */
class ChanLunAnalyzer : public Analysis::IAnalyzer
{
    Q_OBJECT
    Q_INTERFACES(WealthPilot::Analysis::IAnalyzer)

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit ChanLunAnalyzer(QObject* parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~ChanLunAnalyzer() override;

    // ========== IAnalyzer 接口实现 ==========

    QString name() const override { return QStringLiteral("缠论"); }
    Analysis::TheoryType theoryType() const override { return Analysis::TheoryType::ChanLun; }

    Analysis::AnalysisResult analyze(const QVector<Analysis::KLine>& klines) override;
    void clear() override;
    QVector<Analysis::UnifiedSignal> currentSignals() const override;

    // ========== 主要接口 ==========
    
    /**
     * @brief 分析K线数据
     * @param klines 原始K线数据
     * @return 完整分析结果
     */
    ChanLunResult analyze(const QVector<RawKLine>& klines);
    
    /**
     * @brief 增量更新分析
     * @param newKlines 新增的K线数据
     * @return 更新后的分析结果
     */
    ChanLunResult update(const QVector<RawKLine>& newKlines);
    
    /**
     * @brief 获取当前分析结果
     */
    const ChanLunResult& result() const;

    // ========== 分步分析接口 ==========
    
    /**
     * @brief K线包含处理
     * @param klines 原始K线
     * @return 处理后的标准K线
     */
    QVector<StandardKLine> processContainment(const QVector<RawKLine>& klines);
    
    /**
     * @brief 识别分型
     * @param klines 标准K线
     * @return 分型列表
     */
    QVector<Fractal> identifyFractals(const QVector<StandardKLine>& klines);
    
    /**
     * @brief 划分笔
     * @param klines 标准K线
     * @param fractals 分型列表
     * @return 笔列表
     */
    QVector<Pen> identifyPens(const QVector<StandardKLine>& klines, 
                              const QVector<Fractal>& fractals);
    
    /**
     * @brief 划分线段
     * @param pens 笔列表
     * @return 线段列表
     */
    QVector<Segment> identifySegments(const QVector<Pen>& pens);
    
    /**
     * @brief 识别中枢
     * @param segments 线段列表
     * @param pens 笔列表（用于延伸判断）
     * @return 中枢列表
     */
    QVector<Pivot> identifyPivots(const QVector<Segment>& segments,
                                  const QVector<Pen>& pens);
    
    /**
     * @brief 判断背驰
     * @param klines 标准K线
     * @param pens 笔列表
     * @param pivots 中枢列表
     * @return 背驰列表
     */
    QVector<Divergence> detectDivergence(const QVector<StandardKLine>& klines,
                                         const QVector<Pen>& pens,
                                         const QVector<Pivot>& pivots);
    
    /**
     * @brief 识别买卖点
     * @param klines 标准K线
     * @param pens 笔列表
     * @param pivots 中枢列表
     * @param divergences 背驰列表
     * @return 买卖点信号列表
     */
    QVector<TradeSignal> identifySignals(const QVector<StandardKLine>& klines,
                                         const QVector<Pen>& pens,
                                         const QVector<Pivot>& pivots,
                                         const QVector<Divergence>& divergences);

signals:
    /**
     * @brief 分析完成信号
     */
    void analysisCompleted(const ChanLunResult& result);
    
    /**
     * @brief 发现买卖点信号
     */
    void signalFound(const TradeSignal& signal);

private:
    // ========== 内部方法 ==========
    
    /**
     * @brief 判断两根K线是否存在包含关系
     * @param k1 第一根K线
     * @param k2 第二根K线
     * @return 包含方向：1=k1包含k2，-1=k2包含k1，0=无包含
     */
    int checkContainment(const StandardKLine& k1, const StandardKLine& k2);
    
    /**
     * @brief 合并包含K线
     * @param k1 第一根K线
     * @param k2 第二根K线
     * @param direction 合并方向（1=向上，-1=向下）
     * @return 合并后的K线
     */
    StandardKLine mergeKLines(const StandardKLine& k1, const StandardKLine& k2, int direction);
    
    /**
     * @brief 判断是否形成顶分型
     */
    bool isTopFractal(const StandardKLine& k1, const StandardKLine& k2, const StandardKLine& k3);
    
    /**
     * @brief 判断是否形成底分型
     */
    bool isBottomFractal(const StandardKLine& k1, const StandardKLine& k2, const StandardKLine& k3);
    
    /**
     * @brief 计算MACD柱状图面积
     */
    double calculateMACDArea(const QVector<StandardKLine>& klines, int start, int end);
    
    /**
     * @brief 计算笔的MACD面积
     */
    double calculatePenMACD(const QVector<StandardKLine>& klines, const Pen& pen);
    
    /**
     * @brief 判断笔是否破坏前笔
     */
    bool isPenBroken(const Pen& current, const Pen& previous);
    
    /**
     * @brief 判断线段是否被破坏
     */
    bool isSegmentBroken(const QVector<Pen>& pens, int segStart, int segEnd, int newPenIndex);

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace ChanLun
} // namespace WealthPilot

#endif // CHANLUN_ANALYZER_H
