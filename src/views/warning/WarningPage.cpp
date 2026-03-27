#include "WarningPage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QCheckBox>

#include <views/widgets/CardWidget.h>

struct WarningPage::Impl {};

WarningPage::WarningPage(QWidget *parent)
    : BasePage(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
}

WarningPage::~WarningPage() = default;

QString WarningPage::pageId() const
{
    return QStringLiteral("WarningPage");
}

void WarningPage::initializePage()
{

}

void WarningPage::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(24);

    // 头部
    QHBoxLayout* headerLayout = new QHBoxLayout();
    QLabel* titleLabel = new QLabel("预警通知", this);
    titleLabel->setStyleSheet("font-size: 28px; font-weight: 700; color: white;");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();

    QPushButton* markAllBtn = new QPushButton("全部已读", this);
    markAllBtn->setStyleSheet(R"(
        QPushButton {
            background-color: rgba(255, 255, 255, 0.1);
            color: white;
            border: none;
            border-radius: 8px;
            padding: 10px 20px;
        }
    )");
    headerLayout->addWidget(markAllBtn);
    mainLayout->addLayout(headerLayout);

    // 筛选
    QHBoxLayout* filterLayout = new QHBoxLayout();
    QStringList types = {"全部", "价格预警", "风险预警", "系统通知"};
    for (const QString& t : types) {
        QPushButton* btn = new QPushButton(t);
        btn->setCheckable(true);
        btn->setFixedHeight(36);
        btn->setStyleSheet(R"(
            QPushButton {
                background-color: transparent;
                color: #9CA3AF;
                border: none;
                border-bottom: 2px solid transparent;
                padding: 0 20px;
            }
            QPushButton:checked {
                color: #3B82F6;
                border-bottom: 2px solid #3B82F6;
            }
        )");
        filterLayout->addWidget(btn);
    }
    filterLayout->addStretch();
    mainLayout->addLayout(filterLayout);

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

        QString borderColor = alert.type == "risk" ? "#EF4444" :
                                  alert.type == "price" ? "#3B82F6" : "#9CA3AF";
        item->setStyleSheet(QString(R"(
            QWidget {
                background-color: rgba(255, 255, 255, 0.03);
                border-left: 3px solid %1;
                border-radius: 8px;
            }
        )").arg(borderColor));

        QVBoxLayout* textLayout = new QVBoxLayout();
        QLabel* titleLabel = new QLabel(alert.title);
        titleLabel->setStyleSheet("font-weight: 600; color: white;");
        textLayout->addWidget(titleLabel);

        QLabel* contentLabel = new QLabel(alert.content);
        contentLabel->setStyleSheet("color: #9CA3AF; font-size: 13px;");
        textLayout->addWidget(contentLabel);

        itemLayout->addLayout(textLayout, 1);

        QLabel* timeLabel = new QLabel(alert.time);
        timeLabel->setStyleSheet("color: #6B7280; font-size: 12px;");
        itemLayout->addWidget(timeLabel);

        listLayout->addWidget(item);
    }

    listLayout->addStretch();
    listCard->setContent(listContent);
    mainLayout->addWidget(listCard);
}
