/**
 * @file ViewModelTest.cpp
 * @brief ViewModel 单元测试
 * 
 * @details 测试 ViewModelBase 和 TradingViewModel 的核心功能
 * 
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include <QtTest/QtTest>
#include <QSignalSpy>
#include "viewmodels/ViewModelBase.h"
#include "viewmodels/TradingViewModel.h"
#include "viewmodels/OrderViewModel.h"

using namespace WealthPilot;

/**
 * @brief ViewModelBase 单元测试类
 */
class ViewModelBaseTest : public QObject
{
    Q_OBJECT

private
    slots :
    /**
     * @brief 测试初始化
     */

    void testInitialization();

    /**
     * @brief 测试加载状态
     */
    void testLoadingState();

    /**
     * @brief 测试错误处理
     */
    void testErrorHandling();

    /**
     * @brief 测试状态管理
     */
    void testStateManagement();

    /**
     * @brief 测试命令创建
     */
    void testCommandCreation();

    /**
     * @brief 测试命令执行
     */
    void testCommandExecution();

    /**
     * @brief 测试命令取消
     */
    void testCommandCancellation();

    /**
     * @brief 测试异步命令
     */
    void testAsyncCommand();
};

void ViewModelBaseTest::testInitialization()
{
    ViewModelBase viewModel;

    // 初始状态
    QVERIFY(!viewModel.isLoading());
    QVERIFY(viewModel.errorMessage().isEmpty());
    QVERIFY(viewModel.statusMessage().isEmpty());
    QVERIFY(!viewModel.hasError());

    // 初始化
    viewModel.initialize();

    // 初始化后状态应该保持不变
    QVERIFY(!viewModel.isLoading());
}

void ViewModelBaseTest::testLoadingState()
{
    ViewModelBase viewModel;
    QSignalSpy spy(&viewModel, &ViewModelBase::isLoadingChanged);

    // 设置加载状态
    viewModel.setLoading(true);
    QVERIFY(viewModel.isLoading());
    QCOMPARE(spy.count(), 1);

    // 清除加载状态
    viewModel.setLoading(false);
    QVERIFY(!viewModel.isLoading());
    QCOMPARE(spy.count(), 2);
}

void ViewModelBaseTest::testErrorHandling()
{
    ViewModelBase viewModel;
    QSignalSpy spy(&viewModel, &ViewModelBase::errorMessageChanged);
    QSignalSpy errorSpy(&viewModel, &ViewModelBase::errorOccurred);

    // 设置错误
    viewModel.setError("Test error");
    QVERIFY(viewModel.hasError());
    QCOMPARE(viewModel.errorMessage(), "Test error");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(errorSpy.count(), 1);

    // 清除错误
    viewModel.clearError();
    QVERIFY(!viewModel.hasError());
    QVERIFY(viewModel.errorMessage().isEmpty());
    QCOMPARE(spy.count(), 2);
}

void ViewModelBaseTest::testStateManagement()
{
    ViewModelBase viewModel;
    QSignalSpy spy(&viewModel, &ViewModelBase::stateChanged);

    // 设置状态
    viewModel.setState("key1", QVariant(42));
    QCOMPARE(viewModel.getState("key1").toInt(), 42);
    QCOMPARE(spy.count(), 1);

    // 更新状态
    viewModel.setState("key1", QVariant(100));
    QCOMPARE(viewModel.getState("key1").toInt(), 100);
    QCOMPARE(spy.count(), 2);

    // 获取不存在状态
    QVERIFY(!viewModel.getState("nonexistent").isValid());

    // 清除状态
    viewModel.clearState("key1");
    QVERIFY(!viewModel.getState("key1").isValid());

    // 清除所有状态
    viewModel.setState("key2", QVariant("value"));
    viewModel.setState("key3", QVariant(3.14));
    viewModel.clearAllStates();
    QVERIFY(!viewModel.getState("key2").isValid());
    QVERIFY(!viewModel.getState("key3").isValid());
}

void ViewModelBaseTest::testCommandCreation()
{
    ViewModelBase viewModel;

    // 创建同步命令
    Command* cmd = viewModel.createCommand(
        []() { return QVariant(42); },
        []() { return true; }
    );

    QVERIFY(cmd != nullptr);
    QVERIFY(cmd->canExecute());
    QVERIFY(!cmd->isExecuting());
    QCOMPARE(cmd->state(), CommandState::Ready);
}

void ViewModelBaseTest::testCommandExecution()
{
    ViewModelBase viewModel;

    QVariant resultValue;
    Command* cmd = viewModel.createCommand(
        []() { return QVariant(42); },
        nullptr
    );

    cmd->setOnCompleted([&resultValue](const QVariant& result)
    {
        resultValue = result;
    });

    QSignalSpy completedSpy(cmd, &Command::completed);

    // 执行命令
    cmd->execute();

    // 验证结果
    QCOMPARE(completedSpy.count(), 1);
    QCOMPARE(resultValue.toInt(), 42);
    QCOMPARE(cmd->state(), CommandState::Completed);
    QVERIFY(!cmd->isExecuting());
}

void ViewModelBaseTest::testCommandCancellation()
{
    ViewModelBase viewModel;

    Command* cmd = viewModel.createAsyncCommand(
        []()
        {
            // 模拟长时间操作
            QThread::msleep(100);
            return QFuture<QVariant>();
        },
        nullptr
    );

    // 执行命令
    cmd->execute();
    QVERIFY(cmd->isExecuting());

    // 取消命令
    cmd->cancel();
    QVERIFY(!cmd->isExecuting());
    QCOMPARE(cmd->state(), CommandState::Cancelled);
}

void ViewModelBaseTest::testAsyncCommand()
{
    ViewModelBase viewModel;

    bool completed = false;
    Command* cmd = viewModel.createCommand(
        [&completed]()
        {
            completed = true;
            return QVariant();
        },
        nullptr
    );

    QSignalSpy spy(cmd, &Command::completed);

    cmd->execute();

    QVERIFY(spy.wait(1000));
    QVERIFY(completed);
}

/**
 * @brief TradingViewModel 单元测试类
 */
class TradingViewModelTest : public QObject
{
    Q_OBJECT

private
    slots :
    /**
     * @brief 测试合约设置
     */

    void testSetInstrument();

    /**
     * @brief 测试价格更新
     */
    void testPriceUpdate();

    /**
     * @brief 测试下单参数
     */
    void testOrderParams();

    /**
     * @brief 测试保证金计算
     */
    void testMarginCalculation();

    /**
     * @brief 测试盈亏计算
     */
    void testProfitCalculation();

    /**
     * @brief 测试命令状态
     */
    void testCommandState();
};

void TradingViewModelTest::testSetInstrument()
{
    TradingViewModel viewModel;
    QSignalSpy spy(&viewModel, &TradingViewModel::instrumentChanged);

    viewModel.setInstrument("IF2506", "沪深300", 4000.0, 0.2, 300, 0.1);

    QCOMPARE(viewModel.instrumentId(), "IF2506");
    QCOMPARE(viewModel.instrumentName(), "沪深300");
    QCOMPARE(viewModel.currentPrice(), 4000.0);
    QCOMPARE(viewModel.tickSize(), 0.2);
    QCOMPARE(viewModel.volumeMultiple(), 300);
    QCOMPARE(viewModel.marginRatio(), 0.1);
    QCOMPARE(spy.count(), 1);
}

void TradingViewModelTest::testPriceUpdate()
{
    TradingViewModel viewModel;
    viewModel.setInstrument("IF2506", "沪深300", 4000.0);

    QSignalSpy spy(&viewModel, &TradingViewModel::priceChanged);

    viewModel.onPriceUpdated("IF2506", 4010.0);

    QCOMPARE(viewModel.currentPrice(), 4010.0);
    QCOMPARE(spy.count(), 1);
}

void TradingViewModelTest::testOrderParams()
{
    TradingViewModel viewModel;
    QSignalSpy spy(&viewModel, &TradingViewModel::orderParamsChanged);

    viewModel.setOrderPrice(4005.0);
    QCOMPARE(viewModel.orderPrice(), 4005.0);

    viewModel.setOrderVolume(2);
    QCOMPARE(viewModel.orderVolume(), 2);

    viewModel.setOrderType(1); // 市价
    QCOMPARE(viewModel.orderType(), 1);

    QCOMPARE(spy.count(), 3);
}

void TradingViewModelTest::testMarginCalculation()
{
    TradingViewModel viewModel;
    viewModel.setInstrument("IF2506", "沪深300", 4000.0, 0.2, 300, 0.1);
    viewModel.setOrderVolume(1);
    viewModel.setOrderPrice(4000.0);

    // 保证金 = 数量 × 价格 × 合约乘数 × 保证金比例
    // = 1 × 4000 × 300 × 0.1 = 120000
    double margin = viewModel.calculateMargin(1, 4000.0);
    QCOMPARE(margin, 120000.0);
}

void TradingViewModelTest::testProfitCalculation()
{
    TradingViewModel viewModel;
    viewModel.setInstrument("IF2506", "沪深300", 4000.0, 0.2, 300, 0.1);

    // 多头盈亏 = (平仓价 - 开仓价) × 数量 × 合约乘数
    double profit = viewModel.calculateProfit(1, 4000.0, 4010.0, PositionDirection::Long);
    QCOMPARE(profit, 3000.0); // (4010 - 4000) × 1 × 300 = 3000

    // 空头盈亏 = (开仓价 - 平仓价) × 数量 × 合约乘数
    profit = viewModel.calculateProfit(1, 4000.0, 3990.0, PositionDirection::Short);
    QCOMPARE(profit, 3000.0); // (4000 - 3990) × 1 × 300 = 3000
}

void TradingViewModelTest::testCommandState()
{
    TradingViewModel viewModel;
    viewModel.setInstrument("IF2506", "沪深300", 4000.0);

    // 初始状态：可以买入开仓
    QVERIFY(viewModel.buyOpenCommand()->canExecute());

    // 设置账户资金不足
    viewModel.onAccountUpdated(1000.0, 1000.0, 0.0); // 可用资金只有 1000

    // 保证金需求 120000，资金不足
    viewModel.buyOpenCommand()->updateCanExecute();
    QVERIFY(!viewModel.buyOpenCommand()->canExecute());
}

/**
 * @brief OrderViewModel 单元测试类
 */
class OrderViewModelTest : public QObject
{
    Q_OBJECT

private
    slots :
    /**
     * @brief 测试合约设置
     */

    void testSetInstrument();

    /**
     * @brief 测试计算功能
     */
    void testCalculation();

    /**
     * @brief 测试验证功能
     */
    void testValidation();

    /**
     * @brief 测试止损止盈
     */
    void testStopProfit();
};

void OrderViewModelTest::testSetInstrument()
{
    OrderViewModel viewModel;

    viewModel.setInstrument("IF2506", "沪深300", 4000.0, 0.2, 300, 0.1);

    QCOMPARE(viewModel.instrumentId(), "IF2506");
    QCOMPARE(viewModel.lastPrice(), 4000.0);
    QCOMPARE(viewModel.orderPrice(), 4000.0); // 默认价格设为最新价
}

void OrderViewModelTest::testCalculation()
{
    OrderViewModel viewModel;
    viewModel.setInstrument("IF2506", "沪深300", 4000.0, 0.2, 300, 0.1);
    viewModel.setOrderVolume(1);
    viewModel.setOrderPrice(4000.0);

    // 执行计算
    viewModel.calculateCommand()->execute();

    // 验证计算结果
    QCOMPARE(viewModel.requiredMargin(), 120000.0);
    QVERIFY(viewModel.estimatedCommission() > 0);
    QCOMPARE(viewModel.totalRequirement(), viewModel.requiredMargin() + viewModel.estimatedCommission());
}

void OrderViewModelTest::testValidation()
{
    OrderViewModel viewModel;

    // 未设置合约时不能提交
    QVERIFY(!viewModel.canSubmit());
    QCOMPARE(viewModel.validationError(), "请先选择合约");

    // 设置合约
    viewModel.setInstrument("IF2506", "沪深300", 4000.0, 0.2, 300, 0.1);
    viewModel.setOrderPrice(4000.0);
    viewModel.setOrderVolume(1);
    viewModel.setAccount(200000.0, 0.0, 0.0); // 资金充足

    // 可以提交
    QVERIFY(viewModel.canSubmit());
    QVERIFY(viewModel.validationError().isEmpty());

    // 资金不足
    viewModel.setAccount(10000.0, 0.0, 0.0);
    QVERIFY(!viewModel.canSubmit());
    QVERIFY(viewModel.validationError().contains("资金不足"));
}

void OrderViewModelTest::testStopProfit()
{
    OrderViewModel viewModel;
    viewModel.setInstrument("IF2506", "沪深300", 4000.0, 0.2, 300, 0.1);
    viewModel.setDirection(0); // 买入
    viewModel.setOrderVolume(1);
    viewModel.setOrderPrice(4000.0);

    // 启用止盈
    viewModel.setEnableTakeProfit(true);
    viewModel.setTakeProfitPrice(4010.0);

    // 启用止损
    viewModel.setEnableStopLoss(true);
    viewModel.setStopLossPrice(3990.0);

    // 执行计算
    viewModel.calculateCommand()->execute();

    // 验证风险和盈利计算
    // 盈利 = (4010 - 4000) × 1 × 300 = 3000
    QCOMPARE(viewModel.profitAmount(), 3000.0);

    // 风险 = (4000 - 3990) × 1 × 300 = 3000
    QCOMPARE(viewModel.riskAmount(), 3000.0);
}

// 主函数
QTEST_MAIN(ViewModelBaseTest)
QTEST_APPLESS_MAIN(TradingViewModelTest)
QTEST_APPLESS_MAIN(OrderViewModelTest)

// 包含 moc 生成的代码
#include "ViewModelTest.moc"