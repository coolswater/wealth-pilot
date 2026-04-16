/**
 * @file AboutUSPage.cpp
 * @brief About Page Implementation
 */

#include "AboutUSPage.h"
#include "core/config/Tokens.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QPixmap>

struct AboutUSPage::Impl {
    QLabel* logoLabel = nullptr;
    QLabel* titleLabel = nullptr;
    QLabel* versionLabel = nullptr;
    QLabel* infoLabel = nullptr;
    QLabel* devLabel = nullptr;
    QPushButton* checkUpdateBtn = nullptr;
};

AboutUSPage::AboutUSPage(QWidget* parent)
    : BasePage(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
}

AboutUSPage::~AboutUSPage() = default;

QString AboutUSPage::pageId() const
{
    return "AboutUS";
}

void AboutUSPage::initializePage()
{
}

void AboutUSPage::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignCenter);
    mainLayout->setSpacing(20);

    // Logo
    d->logoLabel = new QLabel(this);
    d->logoLabel->setFixedSize(120, 120);
    d->logoLabel->setStyleSheet(
        "QLabel {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
        "    stop:0 #4A90D9, stop:1 #357ABD);"
        "  border-radius: 20px;"
        "}"
    );
    d->logoLabel->setText("WP");
    d->logoLabel->setAlignment(Qt::AlignCenter);
    d->logoLabel->setStyleSheet(
        "QLabel {"
        "  font-size: 48px; font-weight: bold; color: white;"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
        "    stop:0 #4A90D9, stop:1 #357ABD);"
        "  border-radius: 20px;"
        "}"
    );
    mainLayout->addWidget(d->logoLabel, 0, Qt::AlignCenter);

    // Title
    d->titleLabel = new QLabel("WealthPilot", this);
    d->titleLabel->setStyleSheet("font-size: 28px; font-weight: bold;");
    mainLayout->addWidget(d->titleLabel, 0, Qt::AlignCenter);

    // Version
    d->versionLabel = new QLabel("Version 2.0.0", this);
    d->versionLabel->setStyleSheet("font-size: 14px; color: #888;");
    mainLayout->addWidget(d->versionLabel, 0, Qt::AlignCenter);

    // Info
    d->infoLabel = new QLabel(
        "WealthPilot is a financial information analysis platform designed for PC users. "
        "Built with Qt framework, it provides real-time stock and futures tracking, "
        "watchlist management, and comprehensive market analysis.",
        this);
    d->infoLabel->setWordWrap(true);
    d->infoLabel->setAlignment(Qt::AlignCenter);
    d->infoLabel->setMaximumWidth(500);
    d->infoLabel->setProperty("secondary", true);
    mainLayout->addWidget(d->infoLabel, 0, Qt::AlignCenter);

    // Developer
    d->devLabel = new QLabel("Developed by WealthPilot Team", this);
    d->devLabel->setStyleSheet("font-size: 12px; color: #888;");
    d->devLabel->setProperty("secondary", true);
    mainLayout->addWidget(d->devLabel, 0, Qt::AlignCenter);

    // Check update button
    d->checkUpdateBtn = new QPushButton("Check for Updates", this);
    d->checkUpdateBtn->setFixedWidth(150);
    QObject::connect(d->checkUpdateBtn, &QPushButton::clicked, this, []() {
        // TODO: Implement update check
    });
    mainLayout->addWidget(d->checkUpdateBtn, 0, Qt::AlignCenter);

    mainLayout->addStretch();
}
