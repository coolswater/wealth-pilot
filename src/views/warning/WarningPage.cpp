/**
 * @file WarningPage.cpp
 * @brief 预警页面实现 - 预警通知管理
 *
 * @details 功能：
 * - 预警消息列表
 * - 预警类型分类
 * - 预警处理状态
 * - 预警历史记录
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "WarningPage.h"
#include "core/config/Tokens.h"
#include "ui/components/StyleHelper.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QCheckBox>

#include <ui/components/CardWidget.h>

// 使用 WealthPilot 命名空间
using WealthPilot::WarningPage;

struct WarningPage::Impl {};

WarningPage::WarningPage(QWidget *parent)
    : BasePage(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
}

WarningPage::~WarningPage() = default;

void WarningPage::initializePage()
{

}

void WarningPage::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 页面头部
    auto* header = StyleHelper::createPageHeader(this, QStringLiteral("预警通知"));
    mainLayout->addWidget(header);

    // 内容区域
    QWidget* contentWidget = new QWidget(this);
    QVBoxLayout* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(24, 24, 24, 24);
    contentLayout->setSpacing(24);

    // 工具栏
    QHBoxLayout* toolbarLayout = new QHBoxLayout();
    QPushButton* markAllBtn = new QPushButton("全部已读", this);
    markAllBtn->setStyleSheet(QString(R"(
        QPushButton {
            background-color: %1;
            color: %2;
            border: none;
            border-radius: 8px;
            padding: 10px 20px;
        }
    )").arg(Tokens::Colors::BgHover, Tokens::Colors::TextPrimary));
    toolbarLayout->addStretch();
    toolbarLayout->addWidget(markAllBtn);
    contentLayout->addLayout(toolbarLayout);

    // 筛选
    QHBoxLayout* filterLayout = new QHBoxLayout();
    QStringList types = {"全部", "价格预警", "风险预警", "系统通知"};
    for (const QString& t : types) {
        QPushButton* btn = new QPushButton(t);
        btn->setCheckable(true);
        btn->setFixedHeight(36);
        btn->setStyleSheet(QString(R"(
            QPushButton {
                background-color: transparent;
                color: %1;
                border: none;
                border-bottom: 2px solid transparent;
                padding: 0 20px;
            }
            QPushButton:checked {
                color: %2;
                border-bottom: 2px solid %2;
            }
        )").arg(Tokens::Colors::TextSecondary, Tokens::Colors::Primary));
        filterLayout->addWidget(btn);
    }
    filterLayout->addStretch();
    contentLayout->addLayout(filterLayout);

    // 通知列表
    CardWidget* listCard = new CardWidget("", this);

    QWidget* listContent = new QWidget();
    QVBoxLayout* listLayout = new QVBoxLayout(listContent);
    listLayout->setContentsMargins(0, 0, 0, 0);
    listLayout->setSpacing(12);

    struct AlertItem {
        QString title;
        QString content;
        QString time;
        QString type;
    };

    QList<AlertItem> alerts = {
        {"价格预警", "贵州茅台(600519)已达到您设置的目标价 ¥1,850", "10分钟前", "price"},
        {"风险预警", "您的投资组合波动率已超过设定阈值", "30分钟前", "risk"},
        {"系统通知", "账户安全登录提醒", "1小时前", "system"},
        {"价格预警", "比亚迪(002594)跌破止损价 ¥240", "2小时前", "price"}
    };

    for (const auto& alert : alerts) {
        QWidget* item = new QWidget();
        QHBoxLayout* itemLayout = new QHBoxLayout(item);
        itemLayout->setContentsMargins(16, 12, 16, 12);

        QString borderColor = alert.type == "risk" ? Tokens::Colors::Danger :
                                  alert.type == "price" ? Tokens::Colors::Primary : Tokens::Colors::TextSecondary;
        item->setStyleSheet(QString(R"(
            QWidget {
                background-color: rgba(255, 255, 255, 0.03);
                border-left: 3px solid %1;
                border-radius: 8px;
            }
        )").arg(borderColor));

        QVBoxLayout* textLayout = new QVBoxLayout();
        QLabel* titleLabel = new QLabel(alert.title);
        titleLabel->setStyleSheet(QString("font-weight: 600; color: %1;").arg(Tokens::Colors::TextPrimary));
        textLayout->addWidget(titleLabel);

        QLabel* contentLabel = new QLabel(alert.content);
        contentLabel->setStyleSheet(QString("color: %1; font-size: 13px;").arg(Tokens::Colors::TextSecondary));
        textLayout->addWidget(contentLabel);

        itemLayout->addLayout(textLayout, 1);

        QLabel* timeLabel = new QLabel(alert.time);
        timeLabel->setStyleSheet(QString("color: %1; font-size: 12px;").arg(Tokens::Colors::TextTertiary));
        itemLayout->addWidget(timeLabel);

        listLayout->addWidget(item);
    }

    listLayout->addStretch();
    listCard->setContent(listContent);
    contentLayout->addWidget(listCard);
    mainLayout->addWidget(contentWidget, 1);
}
