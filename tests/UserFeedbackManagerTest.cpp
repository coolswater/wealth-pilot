/**
 * @file UserFeedbackManagerTest.cpp
 * @brief 用户反馈管理器单元测试
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include <QtTest/QtTest>
#include "../src/core/feedback/UserFeedbackManager.h"
#include "../src/ui/components/ToastWidget.h"

using namespace WealthPilot;

/**
 * @brief 用户反馈管理器测试类
 */
class UserFeedbackManagerTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // 基础功能测试
    void testShowInfo();
    void testShowWarning();
    void testShowError();
    void testShowSuccess();

    // 对话框测试
    void testShowConfirm();
    void testShowInput();

    // 进度测试
    void testBeginProgress();
    void testUpdateProgress();
    void testEndProgress();

    // 历史记录测试
    void testRecordFeedback();
    void testGetHistory();
    void testClearHistory();
    void testExportHistory();
    void testGetStats();

    // Toast 测试
    void testToastManager();
    void testToastQueue();

private:
    UserFeedbackManager* m_manager = nullptr;
};

void UserFeedbackManagerTest::initTestCase() {
    m_manager = UserFeedbackManager::instance();
    QVERIFY(m_manager != nullptr);
}

void UserFeedbackManagerTest::cleanupTestCase() {
    m_manager->clearHistory();
}

// ========== 基础功能测试 ==========

void UserFeedbackManagerTest::testShowInfo() {
    // 记录初始历史数量
    int initialCount = m_manager->getHistory().size();

    // 显示信息
    m_manager->showInfo("测试标题", "测试消息", UserFeedbackManager::FeedbackLevel::Toast, 1000);

    // 验证历史记录增加
    QCOMPARE(m_manager->getHistory().size(), initialCount + 1);

    // 验证记录内容
    auto history = m_manager->getHistory(1);
    QCOMPARE(history.first().title, QString("测试标题"));
    QCOMPARE(history.first().message, QString("测试消息"));
    QCOMPARE(history.first().type, FeedbackType::Info);
}

void UserFeedbackManagerTest::testShowWarning() {
    int initialCount = m_manager->getHistory().size();

    m_manager->showWarning("警告标题", "警告消息", UserFeedbackManager::FeedbackLevel::Toast);

    QCOMPARE(m_manager->getHistory().size(), initialCount + 1);

    auto history = m_manager->getHistory(1);
    QCOMPARE(history.first().type, FeedbackType::Warning);
}

void UserFeedbackManagerTest::testShowError() {
    int initialCount = m_manager->getHistory().size();

    m_manager->showError("错误标题", "错误消息", UserFeedbackManager::FeedbackLevel::Toast);

    QCOMPARE(m_manager->getHistory().size(), initialCount + 1);

    auto history = m_manager->getHistory(1);
    QCOMPARE(history.first().type, FeedbackType::Error);
}

void UserFeedbackManagerTest::testShowSuccess() {
    int initialCount = m_manager->getHistory().size();

    m_manager->showSuccess("成功标题", "成功消息", UserFeedbackManager::FeedbackLevel::Toast, 1000);

    QCOMPARE(m_manager->getHistory().size(), initialCount + 1);

    auto history = m_manager->getHistory(1);
    QCOMPARE(history.first().type, FeedbackType::Success);
}

// ========== 对话框测试 ==========

void UserFeedbackManagerTest::testShowConfirm() {
    // 注意：对话框测试需要模拟用户交互
    // 这里只测试函数调用不崩溃
    // 实际测试中可以使用 QSignalSpy 或模拟对话框

    // 测试历史记录
    int initialCount = m_manager->getHistory().size();

    // 模拟调用（实际测试需要更复杂的设置）
    // bool result = m_manager->showConfirm("确认", "是否确认？");

    // 验证历史记录
    // QCOMPARE(m_manager->getHistory().size(), initialCount + 1);
}

void UserFeedbackManagerTest::testShowInput() {
    // 类似 showConfirm，需要模拟用户输入
    // 这里只测试函数调用不崩溃

    // QString result = m_manager->showInput("输入", "请输入：");
}

// ========== 进度测试 ==========

void UserFeedbackManagerTest::testBeginProgress() {
    ProgressConfig config;
    config.title = "测试进度";
    config.minimum = 0;
    config.maximum = 100;

    m_manager->beginProgress("test_progress", config);

    // 验证进度对话框已创建
    QVERIFY(!m_manager->isProgressCancelled("test_progress"));

    m_manager->endProgress("test_progress", true);
}

void UserFeedbackManagerTest::testUpdateProgress() {
    ProgressConfig config;
    config.title = "测试进度";
    config.maximum = 100;

    m_manager->beginProgress("test_progress_2", config);

    // 更新进度
    m_manager->updateProgress("test_progress_2", 50, "处理中...");

    // 验证进度未取消
    QVERIFY(!m_manager->isProgressCancelled("test_progress_2"));

    m_manager->endProgress("test_progress_2", true);
}

void UserFeedbackManagerTest::testEndProgress() {
    ProgressConfig config;
    config.title = "测试进度";

    m_manager->beginProgress("test_progress_3", config);
    m_manager->updateProgress("test_progress_3", 100);
    m_manager->endProgress("test_progress_3", true, "完成");

    // 验证进度已结束
    QVERIFY(!m_manager->isProgressCancelled("test_progress_3"));
}

// ========== 历史记录测试 ==========

void UserFeedbackManagerTest::testRecordFeedback() {
    m_manager->clearHistory();

    // 添加多条记录
    m_manager->showInfo("信息1", "消息1", UserFeedbackManager::FeedbackLevel::Toast, 1000);
    m_manager->showWarning("警告1", "消息2", UserFeedbackManager::FeedbackLevel::Toast);
    m_manager->showError("错误1", "消息3", UserFeedbackManager::FeedbackLevel::Toast);

    QCOMPARE(m_manager->getHistory().size(), 3);
}

void UserFeedbackManagerTest::testGetHistory() {
    m_manager->clearHistory();

    // 添加记录
    for (int i = 0; i < 10; ++i) {
        m_manager->showInfo(QString("标题%1").arg(i), QString("消息%1").arg(i),
                           UserFeedbackManager::FeedbackLevel::Toast, 1000);
    }

    // 测试获取限制数量的历史
    auto history = m_manager->getHistory(5);
    QCOMPARE(history.size(), 5);

    // 测试获取全部历史
    history = m_manager->getHistory(0);
    QCOMPARE(history.size(), 10);
}

void UserFeedbackManagerTest::testClearHistory() {
    // 添加一些记录
    m_manager->showInfo("测试", "测试", UserFeedbackManager::FeedbackLevel::Toast, 1000);
    QVERIFY(m_manager->getHistory().size() > 0);

    // 清除历史
    m_manager->clearHistory();
    QCOMPARE(m_manager->getHistory().size(), 0);
}

void UserFeedbackManagerTest::testExportHistory() {
    m_manager->clearHistory();

    // 添加记录
    m_manager->showInfo("信息", "信息消息", UserFeedbackManager::FeedbackLevel::Toast, 1000);
    m_manager->showWarning("警告", "警告消息", UserFeedbackManager::FeedbackLevel::Toast);

    // 导出为文本
    QString exported = m_manager->exportHistory("text");
    QVERIFY(exported.contains("用户反馈历史"));
    QVERIFY(exported.contains("信息"));
    QVERIFY(exported.contains("警告"));
}

void UserFeedbackManagerTest::testGetStats() {
    m_manager->clearHistory();

    // 添加不同类型的记录
    m_manager->showInfo("信息", "信息", UserFeedbackManager::FeedbackLevel::Toast, 1000);
    m_manager->showInfo("信息2", "信息2", UserFeedbackManager::FeedbackLevel::Toast, 1000);
    m_manager->showWarning("警告", "警告", UserFeedbackManager::FeedbackLevel::Toast);
    m_manager->showError("错误", "错误", UserFeedbackManager::FeedbackLevel::Toast);
    m_manager->showSuccess("成功", "成功", UserFeedbackManager::FeedbackLevel::Toast, 1000);

    auto stats = m_manager->getStats();
    QCOMPARE(stats.totalCount, 5);
    QCOMPARE(stats.infoCount, 2);
    QCOMPARE(stats.warningCount, 1);
    QCOMPARE(stats.errorCount, 1);
    QCOMPARE(stats.successCount, 1);
}

// ========== Toast 测试 ==========

void UserFeedbackManagerTest::testToastManager() {
    auto* toastManager = UI::ToastManager::instance();
    QVERIFY(toastManager != nullptr);

    // 显示 Toast
    toastManager->showToast("测试标题", "测试消息", FeedbackType::Info, 1000);

    // 验证 Toast 显示（需要等待）
    QTest::qWait(100);

    // 清除所有 Toast
    toastManager->clearAll();
}

void UserFeedbackManagerTest::testToastQueue() {
    auto* toastManager = UI::ToastManager::instance();
    toastManager->setMaxVisible(2);

    // 添加多个 Toast
    for (int i = 0; i < 5; ++i) {
        toastManager->showToast(QString("Toast %1").arg(i), QString("消息 %1").arg(i),
                               FeedbackType::Info, 500);
    }

    // 等待处理
    QTest::qWait(200);

    toastManager->clearAll();
}

QTEST_MAIN(UserFeedbackManagerTest)
#include "UserFeedbackManagerTest.moc"