#include "NewsPage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QTextBrowser>

#include <ui/components/CardWidget.h>

struct NewsPage::Impl {};

NewsPage::NewsPage(QWidget *parent)
    : BasePage(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
}

NewsPage::~NewsPage() = default;

QString NewsPage::pageId() const
{
    // 使用QStringLiteral避免运行时字符串分配
    return QStringLiteral("NewsPage");
}

void NewsPage::initializePage()
{

}

void NewsPage::setupUI()
{
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(24);

    // 左侧：新闻列表
    CardWidget* listCard = new CardWidget("资讯列表", this);
    listCard->setFixedWidth(400);

    QWidget* listContent = new QWidget();
    QVBoxLayout* listLayout = new QVBoxLayout(listContent);
    listLayout->setContentsMargins(0, 0, 0, 0);

    // 筛选标签
    QHBoxLayout* filterLayout = new QHBoxLayout();
    QStringList filters = {"全部", "要闻", "公告", "研报"};
    for (const QString& f : filters) {
        QPushButton* btn = new QPushButton(f);
        btn->setCheckable(true);
        btn->setFixedHeight(32);
        btn->setStyleSheet(R"(
            QPushButton {
                background-color: transparent;
                color: #9CA3AF;
                border: none;
                border-radius: 4px;
                padding: 0 16px;
            }
            QPushButton:checked {
                background-color: rgba(59, 130, 246, 0.2);
                color: #3B82F6;
            }
        )");
        filterLayout->addWidget(btn);
    }
    filterLayout->addStretch();
    listLayout->addLayout(filterLayout);

    // 新闻列表
    QListWidget* newsList = new QListWidget();
    newsList->setStyleSheet(R"(
        QListWidget {
            background-color: transparent;
            border: none;
        }
        QListWidget::item {
            padding: 16px;
            border-bottom: 1px solid rgba(255, 255, 255, 0.05);
        }
    )");

    QStringList titles = {
        "美联储宣布维持利率不变，市场反应积极",
        "苹果公司发布Q4财报，营收超预期",
        "新能源汽车销量持续增长，行业前景看好",
        "央行降准释放流动性，利好股市"
    };

    for (const QString& title : titles) {
        QListWidgetItem* item = new QListWidgetItem(title);
        newsList->addItem(item);
    }

    listLayout->addWidget(newsList);
    listCard->setContent(listContent);
    mainLayout->addWidget(listCard);

    // 右侧：新闻详情
    CardWidget* detailCard = new CardWidget("", this);

    QWidget* detailContent = new QWidget();
    QVBoxLayout* detailLayout = new QVBoxLayout(detailContent);
    detailLayout->setContentsMargins(0, 0, 0, 0);

    QLabel* titleLabel = new QLabel("美联储宣布维持利率不变，市场反应积极");
    titleLabel->setStyleSheet("font-size: 20px; font-weight: 600; color: white;");
    titleLabel->setWordWrap(true);
    detailLayout->addWidget(titleLabel);

    QHBoxLayout* metaLayout = new QHBoxLayout();
    QLabel* sourceLabel = new QLabel("来源: 财经网");
    sourceLabel->setStyleSheet("color: #9CA3AF; font-size: 12px;");
    metaLayout->addWidget(sourceLabel);
    QLabel* timeLabel = new QLabel("2024-01-15 10:30");
    timeLabel->setStyleSheet("color: #9CA3AF; font-size: 12px;");
    metaLayout->addWidget(timeLabel);
    metaLayout->addStretch();
    detailLayout->addLayout(metaLayout);

    QTextBrowser* contentBrowser = new QTextBrowser();
    contentBrowser->setStyleSheet(R"(
        QTextBrowser {
            background-color: transparent;
            border: none;
            color: #E4E6EB;
            font-size: 14px;
            line-height: 1.8;
        }
    )");
    contentBrowser->setText("美联储在最新的货币政策会议上宣布维持基准利率不变，符合市场预期。这一决定反映出美联储对当前经济形势的审慎态度...");
    detailLayout->addWidget(contentBrowser);

    detailCard->setContent(detailContent);
    mainLayout->addWidget(detailCard, 1);
}
