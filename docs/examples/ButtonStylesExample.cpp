/**
 * @file ButtonStylesExample.cpp
 * @brief 按钮样式使用示例 - 展示如何迁移到新样式系统
 * 
 * @details 本文件展示如何将现有页面的按钮迁移到统一的样式管理系统
 * 
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "ButtonStyles.h"
#include "ui/ThemeManager.h"
#include "core/config/Tokens.h"

#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QDialog>
#include <QMessageBox>

/**
 * @brief 示例：工具栏按钮配置
 * 
 * 展示如何为页面工具栏配置统一的按钮样式
 */
void setupToolbarButtons(QWidget* toolbar)
{
    QHBoxLayout* layout = new QHBoxLayout(toolbar);
    layout->setSpacing(8);
    layout->setContentsMargins(16, 8, 16, 8);

    // ========== 左侧操作按钮组 ==========

    // 添加按钮 - 使用 Add 类型
    QPushButton* addBtn = new QPushButton(QStringLiteral("添加"));
    ButtonStyles::setAdd(addBtn);
    layout->addWidget(addBtn);

    // 编辑按钮 - 使用 Edit 类型
    QPushButton* editBtn = new QPushButton(QStringLiteral("编辑"));
    ButtonStyles::setEdit(editBtn);
    layout->addWidget(editBtn);

    // 删除按钮 - 使用 Delete 类型
    QPushButton* deleteBtn = new QPushButton(QStringLiteral("删除"));
    ButtonStyles::setDelete(deleteBtn);
    layout->addWidget(deleteBtn);

    // 分隔线
    QFrame* separator = new QFrame();
    separator->setFrameShape(QFrame::VLine);
    separator->setProperty("lineType", "vertical");
    layout->addWidget(separator);

    // ========== 右侧功能按钮组 ==========

    layout->addStretch();

    // 刷新按钮 - 使用 Refresh 类型
    QPushButton* refreshBtn = new QPushButton(QStringLiteral("刷新"));
    ButtonStyles::setRefresh(refreshBtn);
    layout->addWidget(refreshBtn);

    // 导出按钮 - 使用 Export 类型
    QPushButton* exportBtn = new QPushButton(QStringLiteral("导出"));
    ButtonStyles::setExport(exportBtn);
    layout->addWidget(exportBtn);
}

/**
 * @brief 示例：对话框按钮配置
 *
 * 展示如何为对话框配置统一的按钮样式
 */
void setupDialogButtons(QDialog* dialog)
{
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(12);
    buttonLayout->addStretch();

    // 取消按钮 - 使用 Secondary 类型
    QPushButton* cancelBtn = new QPushButton(QStringLiteral("取消"));
    cancelBtn->setObjectName("cancelBtn");
    ButtonStyles::setSecondary(cancelBtn);
    cancelBtn->setMinimumWidth(90);
    buttonLayout->addWidget(cancelBtn);

    // 确定按钮 - 使用 Primary 类型
    QPushButton* okBtn = new QPushButton(QStringLiteral("确定"));
    okBtn->setObjectName("okBtn");
    ButtonStyles::setPrimary(okBtn);
    okBtn->setMinimumWidth(90);
    buttonLayout->addWidget(okBtn);

    // 添加到对话框底部
    QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(dialog->layout());
    if (mainLayout)
    {
        mainLayout->addSpacing(16);
        mainLayout->addLayout(buttonLayout);
    }
}

/**
 * @brief 示例：交易面板按钮配置
 *
 * 展示金融交易场景的按钮样式配置
 */
void setupTradingButtons(QWidget* tradingPanel)
{
    QVBoxLayout* layout = new QVBoxLayout(tradingPanel);
    layout->setSpacing(12);

    // ========== 交易操作按钮 ==========

    // 买入按钮 - 使用 Success 类型（绿色）
    QPushButton* buyBtn = new QPushButton(QStringLiteral("买入"));
    ButtonStyles::setSuccess(buyBtn);
    ButtonStyles::setLarge(buyBtn); // 交易按钮使用大尺寸
    buyBtn->setMinimumWidth(120);
    layout->addWidget(buyBtn);

    // 卖出按钮 - 使用 Danger 类型（红色）
    QPushButton* sellBtn = new QPushButton(QStringLiteral("卖出"));
    ButtonStyles::setDanger(sellBtn);
    ButtonStyles::setLarge(sellBtn);
    sellBtn->setMinimumWidth(120);
    layout->addWidget(sellBtn);

    // ========== 辅助操作按钮 ==========

    QHBoxLayout* helperLayout = new QHBoxLayout();
    helperLayout->setSpacing(8);

    // 计算按钮 - 使用 Info 类型
    QPushButton* calcBtn = new QPushButton(QStringLiteral("计算费用"));
    ButtonStyles::setInfo(calcBtn);
    helperLayout->addWidget(calcBtn);

    // 重置按钮 - 使用 Warning 类型
    QPushButton* resetBtn = new QPushButton(QStringLiteral("重置"));
    ButtonStyles::setWarning(resetBtn);
    helperLayout->addWidget(resetBtn);

    layout->addLayout(helperLayout);
}

/**
 * @brief 示例：设置页面按钮配置
 *
 * 展示设置页面的按钮样式配置
 */
void setupSettingsButtons(QWidget* settingsPage)
{
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(12);

    // ========== 左侧危险操作 ==========

    // 重置按钮 - 使用 Warning 类型
    QPushButton* resetBtn = new QPushButton(QStringLiteral("重置为默认"));
    ButtonStyles::setWarning(resetBtn);
    buttonLayout->addWidget(resetBtn);

    buttonLayout->addStretch();

    // ========== 右侧主要操作 ==========

    // 保存按钮 - 使用 Primary 类型
    QPushButton* saveBtn = new QPushButton(QStringLiteral("保存设置"));
    ButtonStyles::setPrimary(saveBtn);
    saveBtn->setMinimumWidth(100);
    buttonLayout->addWidget(saveBtn);
}

/**
 * @brief 示例：回测页面按钮配置
 *
 * 展示回测功能的按钮样式配置
 */
void setupBacktestButtons(QWidget* backtestPanel)
{
    QHBoxLayout* layout = new QHBoxLayout(backtestPanel);
    layout->setSpacing(12);

    // 运行回测 - 主要操作
    QPushButton* runBtn = new QPushButton(QStringLiteral("运行回测"));
    ButtonStyles::setPrimary(runBtn);
    ButtonStyles::setLarge(runBtn);
    runBtn->setMinimumWidth(120);
    layout->addWidget(runBtn);

    // 停止回测 - 危险操作
    QPushButton* stopBtn = new QPushButton(QStringLiteral("停止"));
    ButtonStyles::setDanger(stopBtn);
    stopBtn->setEnabled(false); // 初始禁用
    layout->addWidget(stopBtn);

    layout->addStretch();

    // 导出报告 - 功能操作
    QPushButton* exportBtn = new QPushButton(QStringLiteral("导出报告"));
    ButtonStyles::setExport(exportBtn);
    layout->addWidget(exportBtn);
}

/**
 * @brief 示例：预警中心按钮配置
 *
 * 展示预警管理功能的按钮样式配置
 */
void setupAlertCenterButtons(QWidget* alertPanel)
{
    QHBoxLayout* layout = new QHBoxLayout(alertPanel);
    layout->setSpacing(8);

    // 添加预警 - Add 类型
    QPushButton* addBtn = new QPushButton(QStringLiteral("添加预警"));
    ButtonStyles::setAdd(addBtn);
    layout->addWidget(addBtn);

    // 删除 - Delete 类型
    QPushButton* deleteBtn = new QPushButton(QStringLiteral("删除"));
    ButtonStyles::setDelete(deleteBtn);
    deleteBtn->setEnabled(false); // 未选中时禁用
    layout->addWidget(deleteBtn);

    // 启用/禁用 - Toggle 类型
    QPushButton* toggleBtn = new QPushButton(QStringLiteral("启用/禁用"));
    ButtonStyles::applyStyle(toggleBtn, ButtonType::Toggle);
    toggleBtn->setEnabled(false);
    layout->addWidget(toggleBtn);

    layout->addStretch();

    // 刷新 - Refresh 类型
    QPushButton* refreshBtn = new QPushButton(QStringLiteral("刷新"));
    ButtonStyles::setRefresh(refreshBtn);
    layout->addWidget(refreshBtn);

    // 清空历史 - Danger 类型
    QPushButton* clearBtn = new QPushButton(QStringLiteral("清空历史"));
    ButtonStyles::setDanger(clearBtn);
    layout->addWidget(clearBtn);
}

/**
 * @brief 示例：使用宏创建按钮
 *
 * 展示如何使用便捷宏快速创建带样式的按钮
 */
void createButtonsWithMacros(QWidget* parent)
{
    QVBoxLayout* layout = new QVBoxLayout(parent);

    // 使用宏创建主要按钮
    QPushButton* saveBtn = CREATE_PRIMARY_BUTTON(QStringLiteral("保存"), parent);
    layout->addWidget(saveBtn);

    // 使用宏创建次要按钮
    QPushButton* cancelBtn = CREATE_SECONDARY_BUTTON(QStringLiteral("取消"), parent);
    layout->addWidget(cancelBtn);

    // 使用宏创建危险按钮
    QPushButton* deleteBtn = CREATE_DANGER_BUTTON(QStringLiteral("删除"), parent);
    layout->addWidget(deleteBtn);

    // 使用宏创建成功按钮
    QPushButton* buyBtn = CREATE_SUCCESS_BUTTON(QStringLiteral("买入"), parent);
    layout->addWidget(buyBtn);
}

/**
 * @brief 示例：表格内按钮配置
 *
 * 展示表格单元格内的按钮样式配置
 */
void setupTableButtons()
{
    // 表格内按钮使用小尺寸和轻量样式
    QPushButton* viewBtn = new QPushButton(QStringLiteral("详情"));
    ButtonStyles::setInfo(viewBtn);
    ButtonStyles::setSmall(viewBtn);

    QPushButton* editBtn = new QPushButton(QStringLiteral("编辑"));
    ButtonStyles::setEdit(editBtn);
    ButtonStyles::setSmall(editBtn);

    QPushButton* deleteBtn = new QPushButton(QStringLiteral("删除"));
    ButtonStyles::setDelete(deleteBtn);
    ButtonStyles::setSmall(deleteBtn);
}

/**
 * @brief 示例：图标按钮配置
 *
 * 展示纯图标按钮的样式配置
 */
void setupIconButton(QPushButton* button, const QString& iconPath)
{
    button->setIcon(QIcon(iconPath));
    button->setText(""); // 纯图标按钮不显示文字
    ButtonStyles::setIcon(button);
    button->setFixedSize(32, 32);
}

/**
 * @brief 示例：链接样式按钮配置
 *
 * 展示链接样式按钮的配置
 */
void setupLinkButtons(QWidget* parent)
{
    QHBoxLayout* layout = new QHBoxLayout(parent);

    // 链接样式按钮
    QPushButton* helpLink = new QPushButton(QStringLiteral("帮助"));
    ButtonStyles::setLink(helpLink);
    layout->addWidget(helpLink);

    QPushButton* moreLink = new QPushButton(QStringLiteral("了解更多"));
    ButtonStyles::setLink(moreLink);
    layout->addWidget(moreLink);
}

/**
 * @brief 示例：完整页面按钮配置
 *
 * 展示一个完整页面的按钮布局配置
 */
class ExamplePage : public QWidget
{
public:
    ExamplePage(QWidget* parent = nullptr) : QWidget(parent)
    {
        setupUI();
    }

private:
    void setupUI()
    {
        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setSpacing(16);
        mainLayout->setContentsMargins(24, 16, 24, 16);

        // ========== 标题栏 ==========
        QHBoxLayout* headerLayout = new QHBoxLayout();

        QLabel* titleLabel = new QLabel(QStringLiteral("示例页面"));
        titleLabel->setObjectName("pageTitleLabel");
        headerLayout->addWidget(titleLabel);

        headerLayout->addStretch();

        // 刷新按钮
        QPushButton* refreshBtn = new QPushButton(QStringLiteral("刷新"));
        ButtonStyles::setRefresh(refreshBtn);
        headerLayout->addWidget(refreshBtn);

        mainLayout->addLayout(headerLayout);

        // ========== 工具栏 ==========
        QFrame* toolbar = new QFrame();
        toolbar->setObjectName("pageToolbar");
        setupToolbarButtons(toolbar);
        mainLayout->addWidget(toolbar);

        // ========== 内容区域 ==========
        // ... 添加实际内容

        // ========== 底部操作栏 ==========
        QHBoxLayout* footerLayout = new QHBoxLayout();
        footerLayout->setSpacing(12);

        // 左侧危险操作
        QPushButton* clearBtn = new QPushButton(QStringLiteral("清空数据"));
        ButtonStyles::setDanger(clearBtn);
        footerLayout->addWidget(clearBtn);

        footerLayout->addStretch();

        // 右侧主要操作
        QPushButton* saveBtn = new QPushButton(QStringLiteral("保存"));
        ButtonStyles::setPrimary(saveBtn);
        saveBtn->setMinimumWidth(100);
        footerLayout->addWidget(saveBtn);

        QPushButton* exportBtn = new QPushButton(QStringLiteral("导出"));
        ButtonStyles::setExport(exportBtn);
        footerLayout->addWidget(exportBtn);

        mainLayout->addLayout(footerLayout);
    }
};

// ============================================================================
// 迁移指南示例
// ============================================================================

/**
 * @brief 迁移前：硬编码样式
 */
void beforeMigration()
{
    QPushButton* saveBtn = new QPushButton(QStringLiteral("保存"));
    // 问题：样式硬编码，不统一，难以维护
    saveBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #58A6FF;"
        "  color: white;"
        "  border-radius: 6px;"
        "  padding: 8px 20px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #4A96FF;"
        "}"
    );
}

/**
 * @brief 迁移后：使用统一样式系统
 */
void afterMigration()
{
    QPushButton* saveBtn = new QPushButton(QStringLiteral("保存"));
    // 优点：样式统一，易于维护，支持主题切换
    ButtonStyles::setPrimary(saveBtn);
    ButtonStyles::setMinWidth(saveBtn, 4); // 防止文字截断
}