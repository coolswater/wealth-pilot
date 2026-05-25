/**
 * @file BacktestViewModel.cpp
 * @brief 回测 ViewModel 实现
 * 
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "BacktestViewModel.h"
#include "core/di/ServiceLocator.h"
#include "shared/utils/Logger.h"

#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QThread>

namespace WealthPilot
{
    BacktestViewModel::BacktestViewModel(QObject* parent)
        : ViewModelBase(parent)
    {
        setupCommands();
        LOG_DEBUG("BacktestViewModel created");
    }

    BacktestViewModel::~BacktestViewModel()
    {
        cleanup();
        LOG_DEBUG("BacktestViewModel destroyed");
    }

    void BacktestViewModel::initialize()
    {
        ViewModelBase::initialize();

        // BacktestEngine 暂未实现
        // m_engine = getService<BacktestEngine>();

        // 设置默认日期范围
        QDateTime now = QDateTime::currentDateTime();
        m_endDate = now.toString("yyyy-MM-dd");
        m_startDate = now.addMonths(-6).toString("yyyy-MM-dd");

        validateParams();

        LOG_INFO("BacktestViewModel initialized");
    }

    void BacktestViewModel::cleanup()
    {
        ViewModelBase::cleanup();

        // 停止回测（暂未实现）
        // if (m_state == BacktestState::Running && m_engine) {
        //     m_engine->stop();
        // }

        LOG_INFO("BacktestViewModel cleaned up");
    }

    void BacktestViewModel::setupCommands()
    {
        // 运行回测命令
        m_runCommand = createCommand(
            [this]()
            {
                executeRun();
                return QVariant();
            },
            [this]() { return m_paramsValid && m_state != BacktestState::Running; }
        );

        // 暂停命令
        m_pauseCommand = createCommand(
            [this]()
            {
                executePause();
                return QVariant();
            },
            [this]() { return m_state == BacktestState::Running; }
        );

        // 停止命令
        m_stopCommand = createCommand(
            [this]()
            {
                executeStop();
                return QVariant();
            },
            [this]() { return m_state == BacktestState::Running || m_state == BacktestState::Paused; }
        );

        // 导出命令
        m_exportCommand = createCommand(
            [this]()
            {
                executeExport();
                return QVariant();
            },
            [this]() { return m_state == BacktestState::Completed; }
        );

        // 重置命令
        m_resetCommand = createCommand(
            [this]()
            {
                executeReset();
                return QVariant();
            },
            [this]() { return true; }
        );
    }

    // ========== 参数设置 ==========

    void BacktestViewModel::setStrategyName(const QString& name)
    {
        if (m_strategyName != name)
        {
            m_strategyName = name;
            validateParams();
            emit paramsChanged();
        }
    }

    void BacktestViewModel::setStartDate(const QString& date)
    {
        if (m_startDate != date)
        {
            m_startDate = date;
            validateParams();
            emit paramsChanged();
        }
    }

    void BacktestViewModel::setEndDate(const QString& date)
    {
        if (m_endDate != date)
        {
            m_endDate = date;
            validateParams();
            emit paramsChanged();
        }
    }

    void BacktestViewModel::setInitialCapital(double capital)
    {
        if (qAbs(m_initialCapital - capital) > 0.01)
        {
            m_initialCapital = capital;
            validateParams();
            emit paramsChanged();
        }
    }

    void BacktestViewModel::setCommissionRate(double rate)
    {
        if (qAbs(m_commissionRate - rate) > 0.000001)
        {
            m_commissionRate = rate;
            emit paramsChanged();
        }
    }

    void BacktestViewModel::setSlippage(double slippage)
    {
        if (qAbs(m_slippage - slippage) > 0.001)
        {
            m_slippage = slippage;
            emit paramsChanged();
        }
    }

    // ========== 配置管理 ==========

    QVariantMap BacktestViewModel::getConfig() const
    {
        return {
            {"strategyName", m_strategyName},
            {"startDate", m_startDate},
            {"endDate", m_endDate},
            {"initialCapital", m_initialCapital},
            {"commissionRate", m_commissionRate},
            {"slippage", m_slippage}
        };
    }

    void BacktestViewModel::setConfig(const QVariantMap& config)
    {
        setStrategyName(config.value("strategyName").toString());
        setStartDate(config.value("startDate").toString());
        setEndDate(config.value("endDate").toString());
        setInitialCapital(config.value("initialCapital").toDouble());
        setCommissionRate(config.value("commissionRate").toDouble());
        setSlippage(config.value("slippage").toDouble());
    }

    QVariantList BacktestViewModel::getEquityCurve() const
    {
        return m_equityCurve;
    }

    QVariantList BacktestViewModel::getTradeHistory() const
    {
        return m_tradeHistory;
    }

    // ========== 命令执行 ==========

    void BacktestViewModel::executeRun()
    {
        if (!m_paramsValid)
        {
            setError(m_validationError);
            return;
        }

        setStatus("正在运行回测...");

        m_state = BacktestState::Running;
        emit stateChanged();

        // 更新命令状态
        m_runCommand->updateCanExecute();
        m_pauseCommand->updateCanExecute();
        m_stopCommand->updateCanExecute();

        // BacktestEngine 暂未实现，模拟完成
        // TODO: 实现真实回测引擎集成

        // 模拟进度
        for (int i = 0; i <= 100; i += 10)
        {
            QThread::msleep(100);
            onBacktestProgress(i, m_startDate);
        }

        // 模拟完成
        QVariantMap result;
        result["totalReturn"] = 15.5;
        result["annualizedReturn"] = 25.3;
        result["maxDrawdown"] = 8.2;
        result["sharpeRatio"] = 1.8;
        result["winRate"] = 65.0;
        result["profitFactor"] = 2.1;
        result["totalTrades"] = 50;
        result["winningTrades"] = 32;
        result["losingTrades"] = 18;
        result["finalCapital"] = m_initialCapital * 1.155;
        result["maxCapital"] = m_initialCapital * 1.2;
        result["minCapital"] = m_initialCapital * 0.92;

        onBacktestCompleted(true, result);

        LOG_INFO("Backtest started: " + m_strategyName);
    }

    void BacktestViewModel::executePause()
    {
        if (m_state == BacktestState::Running)
        {
            m_state = BacktestState::Paused;
            emit stateChanged();

            m_runCommand->updateCanExecute();
            m_pauseCommand->updateCanExecute();
            m_stopCommand->updateCanExecute();

            setStatus("回测已暂停");
            LOG_INFO("Backtest paused");
        }
    }

    void BacktestViewModel::executeStop()
    {
        m_state = BacktestState::Idle;
        emit stateChanged();

        m_runCommand->updateCanExecute();
        m_pauseCommand->updateCanExecute();
        m_stopCommand->updateCanExecute();

        setStatus("回测已停止");
        LOG_INFO("Backtest stopped");
    }

    void BacktestViewModel::executeExport()
    {
        setStatus("正在导出报告...");

        // 生成报告
        QString report;
        report += QString("回测报告 - %1\n").arg(m_strategyName);
        report += QString("================================\n");
        report += QString("回测区间: %1 ~ %2\n").arg(m_startDate, m_endDate);
        report += QString("初始资金: %1\n").arg(m_initialCapital, 0, 'f', 2);
        report += QString("最终资金: %1\n").arg(m_finalCapital, 0, 'f', 2);
        report += QString("总收益率: %1%%\n").arg(m_totalReturn, 0, 'f', 2);
        report += QString("年化收益: %1%%\n").arg(m_annualizedReturn, 0, 'f', 2);
        report += QString("最大回撤: %1%%\n").arg(m_maxDrawdown, 0, 'f', 2);
        report += QString("夏普比率: %1\n").arg(m_sharpeRatio, 0, 'f', 2);
        report += QString("胜率: %1%%\n").arg(m_winRate, 0, 'f', 2);
        report += QString("盈亏比: %1\n").arg(m_profitFactor, 0, 'f', 2);
        report += QString("总交易: %1 次\n").arg(m_totalTrades);
        report += QString("盈利: %1 次\n").arg(m_winningTrades);
        report += QString("亏损: %1 次\n").arg(m_losingTrades);

        // 保存到文件
        QString filePath = QString("backtest_report_%1.txt").arg(
            QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QTextStream out(&file);
            out << report;
            file.close();

            setStatus(QString("报告已导出: %1").arg(filePath));
            LOG_INFO(QString("Report exported: %1").arg(filePath));
        }
        else
        {
            setError("导出报告失败");
        }
    }

    void BacktestViewModel::executeReset()
    {
        // 停止回测
        if (m_state == BacktestState::Running)
        {
            // TODO: 实现停止逻辑
        }

        // 重置状态
        m_state = BacktestState::Idle;
        m_progress = 0;
        m_currentDate.clear();
        m_processedBars = 0;
        m_totalBars = 0;

        // 重置结果
        resetResults();

        // 更新命令状态
        m_runCommand->updateCanExecute();
        m_pauseCommand->updateCanExecute();
        m_stopCommand->updateCanExecute();
        m_exportCommand->updateCanExecute();

        clearError();
        clearStatus();

        emit stateChanged();
        emit progressChanged();
        emit resultChanged();

        LOG_DEBUG("Backtest reset");
    }

    // ========== 验证 ==========

    void BacktestViewModel::validateParams()
    {
        m_validationError.clear();
        m_paramsValid = true;

        // 检查策略名称
        if (m_strategyName.isEmpty())
        {
            m_validationError = "请选择策略";
            m_paramsValid = false;
        }

        // 检查日期
        QDate start = QDate::fromString(m_startDate, "yyyy-MM-dd");
        QDate end = QDate::fromString(m_endDate, "yyyy-MM-dd");

        if (!start.isValid())
        {
            m_validationError = "开始日期格式错误";
            m_paramsValid = false;
        }
        else if (!end.isValid())
        {
            m_validationError = "结束日期格式错误";
            m_paramsValid = false;
        }
        else if (start >= end)
        {
            m_validationError = "开始日期必须早于结束日期";
            m_paramsValid = false;
        }

        // 检查资金
        if (m_initialCapital <= 0)
        {
            m_validationError = "初始资金必须大于0";
            m_paramsValid = false;
        }

        emit validationChanged();
        m_runCommand->updateCanExecute();
    }

    void BacktestViewModel::updateResults(const QVariantMap& result)
    {
        m_totalReturn = result.value("totalReturn").toDouble();
        m_annualizedReturn = result.value("annualizedReturn").toDouble();
        m_maxDrawdown = result.value("maxDrawdown").toDouble();
        m_sharpeRatio = result.value("sharpeRatio").toDouble();
        m_winRate = result.value("winRate").toDouble();
        m_profitFactor = result.value("profitFactor").toDouble();
        m_totalTrades = result.value("totalTrades").toInt();
        m_winningTrades = result.value("winningTrades").toInt();
        m_losingTrades = result.value("losingTrades").toInt();
        m_finalCapital = result.value("finalCapital").toDouble();
        m_maxCapital = result.value("maxCapital").toDouble();
        m_minCapital = result.value("minCapital").toDouble();
        m_equityCurve = result.value("equityCurve").toList();
        m_tradeHistory = result.value("tradeHistory").toList();

        emit resultChanged();
    }

    void BacktestViewModel::resetResults()
    {
        m_totalReturn = 0.0;
        m_annualizedReturn = 0.0;
        m_maxDrawdown = 0.0;
        m_sharpeRatio = 0.0;
        m_winRate = 0.0;
        m_profitFactor = 0.0;
        m_totalTrades = 0;
        m_winningTrades = 0;
        m_losingTrades = 0;
        m_finalCapital = 0.0;
        m_maxCapital = 0.0;
        m_minCapital = 0.0;
        m_equityCurve.clear();
        m_tradeHistory.clear();
    }

    // ========== 信号处理 ==========

    void BacktestViewModel::onBacktestProgress(int percent, const QString& date)
    {
        m_progress = percent;
        m_currentDate = date;

        emit progressChanged();
        emit progressUpdated(percent, date);
    }

    void BacktestViewModel::onBacktestCompleted(bool success, const QVariantMap& result)
    {
        if (success)
        {
            m_state = BacktestState::Completed;
            updateResults(result);

            setStatus(QString("回测完成，总收益: %1%%").arg(m_totalReturn, 0, 'f', 2));
            emit backtestCompleted(true, "回测完成");

            LOG_INFO(QString("Backtest completed: return=%1%%").arg(m_totalReturn));
        }
        else
        {
            m_state = BacktestState::Error;
            setError("回测失败");
            emit backtestCompleted(false, "回测失败");
        }

        emit stateChanged();

        // 更新命令状态
        m_runCommand->updateCanExecute();
        m_pauseCommand->updateCanExecute();
        m_stopCommand->updateCanExecute();
        m_exportCommand->updateCanExecute();
    }

    void BacktestViewModel::onBacktestError(const QString& error)
    {
        m_state = BacktestState::Error;
        setError(error);

        emit stateChanged();
        emit backtestCompleted(false, error);

        LOG_ERROR(QString("Backtest error: %1").arg(error));
    }
} // namespace WealthPilot