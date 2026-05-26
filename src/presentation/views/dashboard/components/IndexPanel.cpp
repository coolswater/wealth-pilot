/**
 * @file IndexPanel.cpp
 * @brief 指数面板组件实现
 */

#include "IndexPanel.h"
#include "infrastructure/config/Tokens.h"
#include "presentation/styles/ThemeManager.h"

#include <QHBoxLayout>
#include <QVBoxLayout>

namespace WealthPilot {

// ============================================================================
// Impl 定义
// ============================================================================

struct IndexPanel::Impl {
    QVector<QLabel*> nameLabels;      ///< 名称标签
    QVector<QLabel*> priceLabels;     ///< 价格标签
    QVector<QLabel*> changeLabels;    ///< 涨跌标签
    QVector<IndexInfo> data;          ///< 数据缓存
    QVector<QFrame*> cards;           ///< 卡片容器
};

// ============================================================================
// IndexPanel 实现
// ============================================================================

IndexPanel::IndexPanel(QWidget* parent)
    : QFrame(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
    setObjectName("IndexPanel");
}

void IndexPanel::setupUI()
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    // 默认创建 7 个卡片槽位
    ThemeColors theme = ThemeManager::instance()->currentTheme();

    for (int i = 0; i < 7; ++i) {
        auto* card = new QFrame(this);
        card->setStyleSheet(QString(R"(
            QFrame {
                background-color: %1;
                border-radius: 6px;
                border: 1px solid %2;
            }
        )").arg(theme.bgPrimary, theme.border));

        auto* cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(10, 10, 10, 10);
        cardLayout->setSpacing(4);

        // 名称标签
        auto* nameLabel = new QLabel(QStringLiteral("--"), card);
        nameLabel->setStyleSheet(QString("color: %1; font-size: 15px; font-weight: bold;")
            .arg(theme.textPrimary));
        cardLayout->addWidget(nameLabel);
        d->nameLabels.append(nameLabel);

        // 价格行
        auto* priceLayout = new QHBoxLayout();

        auto* priceLabel = new QLabel(QStringLiteral("0.00"), card);
        priceLabel->setStyleSheet(QString("color: %1; font-size: 15px; font-weight: bold;")
            .arg(theme.textPrimary));
        priceLayout->addWidget(priceLabel, 1);
        d->priceLabels.append(priceLabel);

        auto* changeLabel = new QLabel(QStringLiteral("0.00%"), card);
        changeLabel->setStyleSheet(QString("color: %1; font-size: 13px;")
            .arg(theme.textSecondary));
        priceLayout->addWidget(changeLabel);
        d->changeLabels.append(changeLabel);

        cardLayout->addLayout(priceLayout);
        layout->addWidget(card, 1);
        d->cards.append(card);

        // 点击事件
        card->installEventFilter(this);
    }
}

void IndexPanel::setData(const QVector<IndexInfo>& data)
{
    d->data = data;

    for (int i = 0; i < d->data.size() && i < d->nameLabels.size(); ++i) {
        const auto& info = d->data[i];

        // 更新名称
        d->nameLabels[i]->setText(info.name);

        // 更新价格
        d->priceLabels[i]->setText(QString::number(info.current, 'f', 2));

        // 更新涨跌幅
        QString changeText = QString("%1%2%")
            .arg(info.changePercent >= 0 ? "+" : "")
            .arg(info.changePercent, 0, 'f', 2);
        d->changeLabels[i]->setText(changeText);

        // 设置涨跌颜色
        QString color = info.changePercent >= 0
            ? Tokens::Color::danger().name()
            : Tokens::Color::success().name();
        d->changeLabels[i]->setStyleSheet(
            QString("color: %1; font-size: 13px;").arg(color));
    }
}

void IndexPanel::updateIndex(const QString& code, const IndexInfo& data)
{
    for (int i = 0; i < d->data.size(); ++i) {
        if (d->data[i].code == code) {
            d->data[i] = data;

            // 更新 UI
            if (i < d->priceLabels.size()) {
                d->priceLabels[i]->setText(QString::number(data.current, 'f', 2));
            }
            if (i < d->changeLabels.size()) {
                QString changeText = QString("%1%2%")
                    .arg(data.changePercent >= 0 ? "+" : "")
                    .arg(data.changePercent, 0, 'f', 2);
                d->changeLabels[i]->setText(changeText);

                QString color = data.changePercent >= 0
                    ? Tokens::Color::danger().name()
                    : Tokens::Color::success().name();
                d->changeLabels[i]->setStyleSheet(
                    QString("color: %1; font-size: 13px;").arg(color));
            }
            break;
        }
    }
}

void IndexPanel::clear()
{
    d->data.clear();

    for (auto* label : d->nameLabels) {
        label->setText(QStringLiteral("--"));
    }
    for (auto* label : d->priceLabels) {
        label->setText(QStringLiteral("0.00"));
    }
    for (auto* label : d->changeLabels) {
        label->setText(QStringLiteral("0.00%"));
    }
}

} // namespace WealthPilot
