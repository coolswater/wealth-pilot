/**
 * @file SettingsPage.cpp
 * @brief 设置页面实现
 */

#include "SettingsPage.h"
#include "utils/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QSlider>
#include <QCheckBox>
#include <QMessageBox>
#include <QFileDialog>
#include <QApplication>

#include <views/widgets/CardWidget.h>
#include <core/ConfigManager.h>
#include <core/ThemeManager.h>
#include <core/Tokens.h>

using namespace Tokens;

struct SettingsPage::Impl {
    // 外观设置
    QComboBox* themeCombo = nullptr;
    QSlider* fontSlider = nullptr;
    QLabel* fontValueLabel = nullptr;
    QCheckBox* colorBlindCheck = nullptr;

    // 通知设置
    QCheckBox* priceAlertCheck = nullptr;
    QCheckBox* riskAlertCheck = nullptr;
    QCheckBox* tradeNotifyCheck = nullptr;
    QCheckBox* systemNotifyCheck = nullptr;
    QCheckBox* dailySummaryCheck = nullptr;

    // 安全设置
    QCheckBox* twoFactorCheck = nullptr;
    QCheckBox* bioCheck = nullptr;
};

SettingsPage::SettingsPage(QWidget *parent)
    : BasePage(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
    loadSettings();
}

SettingsPage::~SettingsPage()
{
    saveSettings();
}

QString SettingsPage::pageId() const
{
    return QStringLiteral("SettingsPage");
}

void SettingsPage::initializePage()
{

}

void SettingsPage::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(Spacing::LG, Spacing::LG, Spacing::LG, Spacing::LG);
    mainLayout->setSpacing(Spacing::LG);

    // 页面标题
    QLabel* titleLabel = new QLabel("系统设置", this);
    titleLabel->setStyleSheet(QString(
                                  "font-size: %1px; font-weight: 700; color: %2;")
                                  .arg(Font::Size::H1).arg(Colors::TextPrimary));
    mainLayout->addWidget(titleLabel);

    // 创建各设置区域
    createAppearanceSection();
    createNotificationSection();
    createSecuritySection();
    createAboutSection();

    mainLayout->addStretch();
}

void SettingsPage::createAppearanceSection()
{
    QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(layout());

    CardWidget* card = new CardWidget("外观设置", this);

    QWidget* content = new QWidget(card);
    QVBoxLayout* layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(Spacing::MD);

    // 主题选择
    QHBoxLayout* themeRow = new QHBoxLayout();
    themeRow->setSpacing(Spacing::MD);

    QLabel* themeLabel = new QLabel("主题模式", content);
    themeLabel->setFixedWidth(100);
    themeLabel->setStyleSheet(QString("color: %1;").arg(Colors::TextSecondary));
    themeRow->addWidget(themeLabel);

    d->themeCombo = new QComboBox(content);
    d->themeCombo->addItem("深色模式", "Dark");
    d->themeCombo->addItem("浅色模式", "Light");
    d->themeCombo->addItem("护眼模式", "EyeCare");
    d->themeCombo->setFixedWidth(200);
    d->themeCombo->setStyleSheet(QString(R"(
        QComboBox {
            background-color: %1;
            border: 1px solid %2;
            border-radius: %3px;
            padding: %4px %5px;
            color: %6;
        }
        QComboBox:hover {
            border-color: %7;
        }
        QComboBox::drop-down {
            border: none;
            width: 24px;
        }
        QComboBox QAbstractItemView {
            background-color: %8;
            border: 1px solid %2;
            selection-background-color: %9;
        }
    )").arg(Colors::BgHover)
                                     .arg(Colors::Border)
                                     .arg(Radius::MD)
                                     .arg(Spacing::SM)
                                     .arg(Spacing::SM)
                                     .arg(Colors::TextPrimary)
                                     .arg(Colors::BorderHover)
                                     .arg(Colors::BgSurface)
                                     .arg(Colors::BgActive));

    connect(d->themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsPage::onThemeChanged);
    themeRow->addWidget(d->themeCombo);
    themeRow->addStretch();

    layout->addLayout(themeRow);

    // 字体大小
    QHBoxLayout* fontRow = new QHBoxLayout();
    fontRow->setSpacing(Spacing::MD);

    QLabel* fontLabel = new QLabel("字体大小", content);
    fontLabel->setFixedWidth(100);
    fontLabel->setStyleSheet(QString("color: %1;").arg(Colors::TextSecondary));
    fontRow->addWidget(fontLabel);

    d->fontSlider = new QSlider(Qt::Horizontal, content);
    d->fontSlider->setRange(80, 120);
    d->fontSlider->setValue(100);
    d->fontSlider->setFixedWidth(200);
    d->fontSlider->setStyleSheet(QString(R"(
        QSlider::groove:horizontal {
            height: 4px;
            background: %1;
            border-radius: 2px;
        }
        QSlider::handle:horizontal {
            width: 16px;
            height: 16px;
            background: %2;
            border-radius: 8px;
            margin: -6px 0;
        }
        QSlider::handle:horizontal:hover {
            background: %3;
        }
    )").arg(Colors::Border).arg(Colors::Primary).arg(Colors::PrimaryHover));

    connect(d->fontSlider, &QSlider::valueChanged, this, &SettingsPage::onFontSizeChanged);
    fontRow->addWidget(d->fontSlider);

    d->fontValueLabel = new QLabel("100%", content);
    d->fontValueLabel->setFixedWidth(50);
    d->fontValueLabel->setStyleSheet(QString("color: %1;").arg(Colors::TextSecondary));
    fontRow->addWidget(d->fontValueLabel);
    fontRow->addStretch();

    layout->addLayout(fontRow);

    // 色盲模式
    QHBoxLayout* colorBlindRow = new QHBoxLayout();
    d->colorBlindCheck = new QCheckBox("启用色盲友好模式", content);
    d->colorBlindCheck->setStyleSheet(QString("color: %1;").arg(Colors::TextPrimary));
    colorBlindRow->addWidget(d->colorBlindCheck);
    colorBlindRow->addStretch();
    layout->addLayout(colorBlindRow);

    card->setContent(content);
    mainLayout->addWidget(card);
}

void SettingsPage::createNotificationSection()
{
    QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(layout());

    CardWidget* card = new CardWidget("通知设置", this);

    QWidget* content = new QWidget(card);
    QVBoxLayout* layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(Spacing::SM);

    QString checkStyle = QString("QCheckBox { color: %1; spacing: 8px; }"
                                 "QCheckBox::indicator { width: 18px; height: 18px; }")
                             .arg(Colors::TextPrimary);

    d->priceAlertCheck = new QCheckBox("价格预警通知", content);
    d->priceAlertCheck->setChecked(true);
    d->priceAlertCheck->setStyleSheet(checkStyle);
    connect(d->priceAlertCheck, &QCheckBox::checkStateChanged, this, &SettingsPage::onNotificationChanged);
    layout->addWidget(d->priceAlertCheck);

    d->riskAlertCheck = new QCheckBox("风险预警通知", content);
    d->riskAlertCheck->setChecked(true);
    d->riskAlertCheck->setStyleSheet(checkStyle);
    connect(d->riskAlertCheck, &QCheckBox::checkStateChanged, this, &SettingsPage::onNotificationChanged);
    layout->addWidget(d->riskAlertCheck);

    d->tradeNotifyCheck = new QCheckBox("交易完成通知", content);
    d->tradeNotifyCheck->setChecked(true);
    d->tradeNotifyCheck->setStyleSheet(checkStyle);
    connect(d->tradeNotifyCheck, &QCheckBox::checkStateChanged, this, &SettingsPage::onNotificationChanged);
    layout->addWidget(d->tradeNotifyCheck);

    d->systemNotifyCheck = new QCheckBox("系统公告通知", content);
    d->systemNotifyCheck->setStyleSheet(checkStyle);
    connect(d->systemNotifyCheck, &QCheckBox::checkStateChanged, this, &SettingsPage::onNotificationChanged);
    layout->addWidget(d->systemNotifyCheck);

    d->dailySummaryCheck = new QCheckBox("每日市场总结", content);
    d->dailySummaryCheck->setChecked(true);
    d->dailySummaryCheck->setStyleSheet(checkStyle);
    connect(d->dailySummaryCheck, &QCheckBox::checkStateChanged, this, &SettingsPage::onNotificationChanged);
    layout->addWidget(d->dailySummaryCheck);

    card->setContent(content);
    mainLayout->addWidget(card);
}

void SettingsPage::createSecuritySection()
{
    QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(layout());

    CardWidget* card = new CardWidget("安全设置", this);

    QWidget* content = new QWidget(card);
    QVBoxLayout* layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(Spacing::SM);

    QString checkStyle = QString("QCheckBox { color: %1; spacing: 8px; }")
                             .arg(Colors::TextPrimary);

    d->twoFactorCheck = new QCheckBox("启用双重验证", content);
    d->twoFactorCheck->setStyleSheet(checkStyle);
    layout->addWidget(d->twoFactorCheck);

    d->bioCheck = new QCheckBox("启用生物识别登录", content);
    d->bioCheck->setChecked(true);
    d->bioCheck->setStyleSheet(checkStyle);
    layout->addWidget(d->bioCheck);

    // 按钮
    QHBoxLayout* btnRow = new QHBoxLayout();
    btnRow->setSpacing(Spacing::SM);

    QPushButton* changePwdBtn = new QPushButton("修改密码", content);
    changePwdBtn->setFixedHeight(Size::ButtonHeightMD);
    changePwdBtn->setProperty("secondary", true);
    btnRow->addWidget(changePwdBtn);

    QPushButton* clearCacheBtn = new QPushButton("清除缓存", content);
    clearCacheBtn->setFixedHeight(Size::ButtonHeightMD);
    clearCacheBtn->setProperty("secondary", true);
    connect(clearCacheBtn, &QPushButton::clicked, this, &SettingsPage::onClearCacheClicked);
    btnRow->addWidget(clearCacheBtn);

    btnRow->addStretch();
    layout->addLayout(btnRow);

    card->setContent(content);
    mainLayout->addWidget(card);
}

void SettingsPage::createAboutSection()
{
    QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(layout());

    CardWidget* card = new CardWidget("关于", this);

    QWidget* content = new QWidget(card);
    QVBoxLayout* layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(Spacing::SM);

    QString labelStyle = QString("color: %1; font-size: %2px;")
                             .arg(Colors::TextSecondary).arg(Font::Size::Small);

    QLabel* nameLabel = new QLabel("WealthPilot 智能投资管理", content);
    nameLabel->setStyleSheet(QString("color: %1; font-size: %2px; font-weight: 600;")
                                 .arg(Colors::TextPrimary).arg(Font::Size::Body));
    layout->addWidget(nameLabel);

    QLabel* versionLabel = new QLabel("版本: 1.1.0", content);
    versionLabel->setStyleSheet(labelStyle);
    layout->addWidget(versionLabel);

    QLabel* techLabel = new QLabel("技术栈: Qt 6.10.2 / C++17", content);
    techLabel->setStyleSheet(labelStyle);
    layout->addWidget(techLabel);

    QLabel* devLabel = new QLabel("开发者: WealthPilot Team", content);
    devLabel->setStyleSheet(labelStyle);
    layout->addWidget(devLabel);

    // 导出数据按钮
    QHBoxLayout* btnRow = new QHBoxLayout();
    QPushButton* exportBtn = new QPushButton("导出数据", content);
    exportBtn->setFixedHeight(Size::ButtonHeightMD);
    exportBtn->setProperty("secondary", true);
    connect(exportBtn, &QPushButton::clicked, this, &SettingsPage::onExportDataClicked);
    btnRow->addWidget(exportBtn);
    btnRow->addStretch();
    layout->addLayout(btnRow);

    card->setContent(content);
    mainLayout->addWidget(card);
}

void SettingsPage::loadSettings()
{
    // 加载主题
    QString theme = ConfigManager::instance()->getString(ConfigKeys::Theme, "Dark");
    int index = d->themeCombo->findData(theme);
    if (index >= 0) {
        d->themeCombo->setCurrentIndex(index);
    }

    // 加载字体大小
    int fontSize = ConfigManager::instance()->getInt("appearance/font_size", 100);
    d->fontSlider->setValue(fontSize);
    d->fontValueLabel->setText(QString("%1%").arg(fontSize));

    // 加载通知设置
    d->priceAlertCheck->setChecked(ConfigManager::instance()->getBool("notify/price_alert", true));
    d->riskAlertCheck->setChecked(ConfigManager::instance()->getBool("notify/risk_alert", true));
    d->tradeNotifyCheck->setChecked(ConfigManager::instance()->getBool("notify/trade", true));
    d->systemNotifyCheck->setChecked(ConfigManager::instance()->getBool("notify/system", false));
    d->dailySummaryCheck->setChecked(ConfigManager::instance()->getBool("notify/daily_summary", true));
}

void SettingsPage::saveSettings()
{
    // 保存字体大小
    ConfigManager::instance()->set("appearance/font_size", d->fontSlider->value());

    // 保存通知设置
    ConfigManager::instance()->set("notify/price_alert", d->priceAlertCheck->isChecked());
    ConfigManager::instance()->set("notify/risk_alert", d->riskAlertCheck->isChecked());
    ConfigManager::instance()->set("notify/trade", d->tradeNotifyCheck->isChecked());
    ConfigManager::instance()->set("notify/system", d->systemNotifyCheck->isChecked());
    ConfigManager::instance()->set("notify/daily_summary", d->dailySummaryCheck->isChecked());
}

void SettingsPage::onThemeChanged(int index)
{
    QString theme = d->themeCombo->itemData(index).toString();
    // ThemeManager::instance()->setTheme(theme);
    LOG_INFO(QString("Theme changed to: %1").arg(theme));
}

void SettingsPage::onFontSizeChanged(int value)
{
    d->fontValueLabel->setText(QString("%1%").arg(value));

    // 应用字体缩放
    QFont font = qApp->font();
    font.setPointSize(qRound(10 * value / 100.0));
    qApp->setFont(font);

    ConfigManager::instance()->set("appearance/font_size", value);
}

void SettingsPage::onNotificationChanged()
{
    saveSettings();
}

void SettingsPage::onClearCacheClicked()
{
    int ret = QMessageBox::question(this, "确认",
                                    "确定要清除所有缓存数据吗？\n这将清除行情缓存、浏览历史等数据。",
                                    QMessageBox::Yes | QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        // 清除数据服务缓存
        // DataService::instance()->clearCache();

        // 清除浏览历史
        // DatabaseManager::instance()->clearBrowseHistory();

        LOG_INFO("Cache cleared");
        QMessageBox::information(this, "完成", "缓存已清除");
    }
}

void SettingsPage::onExportDataClicked()
{
    QString filePath = QFileDialog::getSaveFileName(this,
                                                    "导出数据", "", "JSON 文件 (*.json)");

    if (!filePath.isEmpty()) {
        // 导出配置
        ConfigManager::instance()->exportToFile(filePath, false);
        LOG_INFO(QString("Data exported to: %1").arg(filePath));
        QMessageBox::information(this, "完成", "数据已导出");
    }
}
