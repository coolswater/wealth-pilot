/**
 * @file BacktestViewModel.h
 * @brief 回测 ViewModel - MVVM 架构
 * 
 * @details 提供回测功能的数据绑定和命令�? * - 策略配置
 * - 回测参数设置
 * - 回测执行控制
 * - 结果展示
 * 
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef BACKTESTVIEWMODEL_H
#define BACKTESTVIEWMODEL_H

#include "ViewModelBase.h"
#include <QDateTime>
#include <QVariantMap>

namespace WealthPilot
{
    /**
 * @brief 回测状�? */
    enum class BacktestState
    {
        Idle, ///< 空闲
        Running, ///< 运行中
        Paused, ///< 已暂停
        Completed, ///< 已完成
        Error ///< 错误
    };

    /**
 * @brief 回测 ViewModel
 *
 * @details 用于 QML 回测面板�? * @code
 * // BacktestPanel.qml
 * BacktestViewModel {
 *     id: viewModel
 * }
 *
 * TextField {
 *     text: viewModel.startDate
 *     onTextChanged: viewModel.setStartDate(text)
 * }
 *
 * Button {
 *     text: "运行回测"
 *     enabled: viewModel.runCommand.canExecute
 *     onClicked: viewModel.runCommand.execute()
 * }
 * @endcode
 */
    class BacktestViewModel : public ViewModelBase
    {
        Q_OBJECT

        // ========== 回测参数 ==========

        /// 策略名称
        Q_PROPERTY(QString strategyName READ strategyName WRITE setStrategyName NOTIFY paramsChanged)
        /// 开始日�?    Q_PROPERTY(QString startDate READ startDate WRITE setStartDate NOTIFY paramsChanged)
    /// 结束日期
        Q_PROPERTY(QString endDate READ endDate WRITE setEndDate NOTIFY paramsChanged)
        /// 初始资金
        Q_PROPERTY(double initialCapital READ initialCapital WRITE setInitialCapital NOTIFY paramsChanged)
        /// 手续费率
        Q_PROPERTY(double commissionRate READ commissionRate WRITE setCommissionRate NOTIFY paramsChanged)
        /// 滑点
        Q_PROPERTY(double slippage READ slippage WRITE setSlippage NOTIFY paramsChanged)

        // ========== 回测状�?==========

        /// 回测状�?    Q_PROPERTY(int state READ state NOTIFY stateChanged)
    /// 进度 (0-100)
        Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
        /// 当前处理日期
        Q_PROPERTY(QString currentDate READ currentDate NOTIFY progressChanged)
        /// 已处理数据条�?    Q_PROPERTY(int processedBars READ processedBars NOTIFY progressChanged)
    /// 总数据条�?    Q_PROPERTY(int totalBars READ totalBars NOTIFY progressChanged)

        // ========== 回测结果 ==========

        /// 总收益率 (%)
        Q_PROPERTY(double totalReturn READ totalReturn NOTIFY resultChanged)
        /// 年化收益�?(%)
        Q_PROPERTY(double annualizedReturn READ annualizedReturn NOTIFY resultChanged)
        /// 最大回�?(%)
        Q_PROPERTY(double maxDrawdown READ maxDrawdown NOTIFY resultChanged)
        /// 夏普比率
        Q_PROPERTY(double sharpeRatio READ sharpeRatio NOTIFY resultChanged)
        /// 胜率 (%)
        Q_PROPERTY(double winRate READ winRate NOTIFY resultChanged)
        /// 盈亏�?    Q_PROPERTY(double profitFactor READ profitFactor NOTIFY resultChanged)
    /// 总交易次�?    Q_PROPERTY(int totalTrades READ totalTrades NOTIFY resultChanged)
    /// 盈利交易次数
        Q_PROPERTY(int winningTrades READ winningTrades NOTIFY resultChanged)
        /// 亏损交易次数
        Q_PROPERTY(int losingTrades READ losingTrades NOTIFY resultChanged)

        // ========== 资金曲线 ==========

        /// 最终资�?    Q_PROPERTY(double finalCapital READ finalCapital NOTIFY resultChanged)
    /// 最大资�?    Q_PROPERTY(double maxCapital READ maxCapital NOTIFY resultChanged)
    /// 最小资�?    Q_PROPERTY(double minCapital READ minCapital NOTIFY resultChanged)

        // ========== 命令 ==========

        /// 运行回测命令
        Q_PROPERTY(Command* runCommand READ runCommand CONSTANT)
        /// 暂停回测命令
        Q_PROPERTY(Command* pauseCommand READ pauseCommand CONSTANT)
        /// 停止回测命令
        Q_PROPERTY(Command* stopCommand READ stopCommand CONSTANT)
        /// 导出报告命令
        Q_PROPERTY(Command* exportCommand READ exportCommand CONSTANT)
        /// 重置命令
        Q_PROPERTY(Command* resetCommand READ resetCommand CONSTANT)

        // ========== 验证 ==========

        /// 参数是否有效
        Q_PROPERTY(bool paramsValid READ paramsValid NOTIFY validationChanged)
        /// 验证错误信息
        Q_PROPERTY(QString validationError READ validationError NOTIFY validationChanged)

    public:
        explicit BacktestViewModel(QObject* parent = nullptr);
        ~BacktestViewModel() override;

        void initialize() override;
        void cleanup() override;

        // ========== 属性访�?==========

        // 回测参数
        QString strategyName() const { return m_strategyName; }
        QString startDate() const { return m_startDate; }
        QString endDate() const { return m_endDate; }
        double initialCapital() const { return m_initialCapital; }
        double commissionRate() const { return m_commissionRate; }
        double slippage() const { return m_slippage; }

        // 回测状�?    int state() const { return static_cast<int>(m_state); }
        int progress() const { return m_progress; }
        QString currentDate() const { return m_currentDate; }
        int processedBars() const { return m_processedBars; }
        int totalBars() const { return m_totalBars; }

        // 回测结果
        double totalReturn() const { return m_totalReturn; }
        double annualizedReturn() const { return m_annualizedReturn; }
        double maxDrawdown() const { return m_maxDrawdown; }
        double sharpeRatio() const { return m_sharpeRatio; }
        double winRate() const { return m_winRate; }
        double profitFactor() const { return m_profitFactor; }
        int totalTrades() const { return m_totalTrades; }
        int winningTrades() const { return m_winningTrades; }
        int losingTrades() const { return m_losingTrades; }

        // 资金曲线
        double finalCapital() const { return m_finalCapital; }
        double maxCapital() const { return m_maxCapital; }
        double minCapital() const { return m_minCapital; }

        // 命令
        Command* runCommand() { return m_runCommand; }
        Command* pauseCommand() { return m_pauseCommand; }
        Command* stopCommand() { return m_stopCommand; }
        Command* exportCommand() { return m_exportCommand; }
        Command* resetCommand() { return m_resetCommand; }

        // 验证
        bool paramsValid() const { return m_paramsValid; }
        QString validationError() const { return m_validationError; }

        // ========== 参数设置 ==========

        void setStrategyName(const QString& name);
        void setStartDate(const QString& date);
        void setEndDate(const QString& date);
        void setInitialCapital(double capital);
        void setCommissionRate(double rate);
        void setSlippage(double slippage);

        // ========== 公共方法 ==========

        /**
     * @brief 获取回测配置
     */
    Q_INVOKABLE QVariantMap getConfig() const;

        /**
     * @brief 设置回测配置
     */
    Q_INVOKABLE void setConfig(const QVariantMap& config);

        /**
     * @brief 获取资金曲线数据
     */
    Q_INVOKABLE QVariantList getEquityCurve() const;

        /**
     * @brief 获取交易记录
     */
    Q_INVOKABLE QVariantList getTradeHistory() const;

        signals :

        void paramsChanged();
        void stateChanged();
        void progressChanged();
        void resultChanged();
        void validationChanged();

        /**
     * @brief 回测完成
     */
        void backtestCompleted(bool success, const QString& message);

        /**
     * @brief 回测进度更新
     */
        void progressUpdated(int percent, const QString& currentDate);

    private
        slots :

        void onBacktestProgress(int percent, const QString& date);
        void onBacktestCompleted(bool success, const QVariantMap& result);
        void onBacktestError(const QString& error);

    private:
        void setupCommands();
        void executeRun();
        void executePause();
        void executeStop();
        void executeExport();
        void executeReset();

        void validateParams();
        void updateResults(const QVariantMap& result);
        void resetResults();

        // ========== 回测参数 ==========
        QString m_strategyName;
        QString m_startDate;
        QString m_endDate;
        double m_initialCapital = 1000000.0;
        double m_commissionRate = 0.0003;
        double m_slippage = 0.0;

        // ========== 回测状�?==========
        BacktestState m_state = BacktestState::Idle;
        int m_progress = 0;
        QString m_currentDate;
        int m_processedBars = 0;
        int m_totalBars = 0;

        // ========== 回测结果 ==========
        double m_totalReturn = 0.0;
        double m_annualizedReturn = 0.0;
        double m_maxDrawdown = 0.0;
        double m_sharpeRatio = 0.0;
        double m_winRate = 0.0;
        double m_profitFactor = 0.0;
        int m_totalTrades = 0;
        int m_winningTrades = 0;
        int m_losingTrades = 0;

        // ========== 资金曲线 ==========
        double m_finalCapital = 0.0;
        double m_maxCapital = 0.0;
        double m_minCapital = 0.0;
        QVariantList m_equityCurve;
        QVariantList m_tradeHistory;

        // ========== 命令 ==========
        Command* m_runCommand = nullptr;
        Command* m_pauseCommand = nullptr;
        Command* m_stopCommand = nullptr;
        Command* m_exportCommand = nullptr;
        Command* m_resetCommand = nullptr;

        // ========== 验证 ==========
        bool m_paramsValid = false;
        QString m_validationError;

        // ========== 服务引用 ==========
        QObject* m_engine = nullptr;
    };
} // namespace WealthPilot

#endif // BACKTESTVIEWMODEL_H