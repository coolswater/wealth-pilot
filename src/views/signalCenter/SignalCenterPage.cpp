#include "SignalCenterPage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QGridLayout>
#include <QProgressBar>

#include <views/widgets/CardWidget.h>

struct SignalCenterPage::Impl {};

SignalCenterPage::SignalCenterPage(QWidget *parent)
    : BasePage(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
}

SignalCenterPage::~SignalCenterPage() = default;

QString SignalCenterPage::pageId() const
{
    return QStringLiteral("SignalCenterPage");
}

void SignalCenterPage::initializePage()
{

}

void SignalCenterPage::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(24);

    // 头部
    QHBoxLayout* headerLayout = new QHBoxLayout();
    QLabel* titleLabel = new QLabel("信号中心", this);
    titleLabel->setStyleSheet("font-size: 28px; font-weight: 700; color: white;");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();

    QPushButton* mySignalsBtn = new QPushButton("我的订阅", this);
    mySignalsBtn->setStyleSheet(R"(
        QPushButton {
            background-color: rgba(59, 130, 246, 0.2);
            color: #3B82F6;
            border: none;
            border-radius: 8px;
            padding: 10px 20px;
        }
    )");
    headerLayout->addWidget(mySignalsBtn);
    mainLayout->addLayout(headerLayout);

    // 信号卡片网格
    QGridLayout* gridLayout = new QGridLayout();
    gridLayout->setSpacing(20);

    QStringList names = {"量化策略A", "趋势跟踪B", "价值投资C", "短线精灵D"};
    QList<double> returns = {156.8, 89.3, 45.2, 234.5};
    QList<int> winRates = {78, 65, 82, 71};
    QList<int> followers = {2340, 1890, 3200, 1560};

    for (int i = 0; i < names.size(); ++i) {
        CardWidget* card = new CardWidget(names[i], this);
        card->setMinimumSize(280, 200);

        QWidget* content = new QWidget();
        QVBoxLayout* layout = new QVBoxLayout(content);
        layout->setContentsMargins(0, 0, 0, 0);

        // 收益率
        QHBoxLayout* returnLayout = new QHBoxLayout();
        QLabel* returnLabel = new QLabel("累计收益");
        returnLabel->setStyleSheet("color: #9CA3AF;");
        returnLayout->addWidget(returnLabel);
        returnLayout->addStretch();
        QLabel* returnValue = new QLabel(QString("+%1%").arg(returns[i]));
        returnValue->setStyleSheet("color: #10B981; font-size: 24px; font-weight: 700;");
        returnLayout->addWidget(returnValue);
        layout->addLayout(returnLayout);

        // 胜率
        QHBoxLayout* winLayout = new QHBoxLayout();
        QLabel* winLabel = new QLabel("胜率");
        winLabel->setStyleSheet("color: #9CA3AF;");
        winLayout->addWidget(winLabel);
        winLayout->addStretch();
        QLabel* winValue = new QLabel(QString("%1%").arg(winRates[i]));
        winValue->setStyleSheet("color: white; font-weight: 600;");
        winLayout->addWidget(winValue);
        layout->addLayout(winLayout);

        // 订阅人数
        QHBoxLayout* followLayout = new QHBoxLayout();
        QLabel* followLabel = new QLabel("订阅人数");
        followLabel->setStyleSheet("color: #9CA3AF;");
        followLayout->addWidget(followLabel);
        followLayout->addStretch();
        QLabel* followValue = new QLabel(QString::number(followers[i]));
        followValue->setStyleSheet("color: white;");
        followLayout->addWidget(followValue);
        layout->addLayout(followLayout);

        // 订阅按钮
        QPushButton* subBtn = new QPushButton("订阅 ¥99/月");
        subBtn->setFixedHeight(40);
        subBtn->setStyleSheet(R"(
            QPushButton {
                background-color: #3B82F6;
                color: white;
                border: none;
                border-radius: 8px;
                font-weight: 600;
            }
        )");
        layout->addWidget(subBtn);

        card->setContent(content);
        gridLayout->addWidget(card, i / 2, i % 2);
    }

    mainLayout->addLayout(gridLayout);
    mainLayout->addStretch();
}
