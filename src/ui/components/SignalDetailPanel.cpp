/**
 * @file SignalDetailPanel.cpp
 * @brief 信号详情面板实现
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "SignalDetailPanel.h"
#include "core/config/Tokens.h"
#include <QScrollArea>
#include <QProgressBar>
#include <QHeaderView>
#include <QDebug>

namespace WealthPilot {
namespace UI {

struct SignalDetailPanel::Impl {
    Analysis::CompositeSignal compositeSignal;
    Analysis::UnifiedSignal unifiedSignal;
    bool hasCompositeSignal = false;

    // UI组件
    QLabel* symbolLabel = nullptr;
    QLabel* timeLabel = nullptr;
    QLabel* directionLabel = nullptr;
    QLabel* confidenceLabel = nullptr;
    QProgressBar* confidenceBar = nullptr;
    QLabel* theoryCountLabel = nullptr;
    QLabel* scoreLabel = nullptr;
    QLabel* descriptionLabel = nullptr;

    QTableWidget* theoryTable = nullptr;
    QTextEdit* riskText = nullptr;

    QPushButton* subscribeBtn = nullptr;
    QPushButton* historyBtn = nullptr;

    bool compactMode = false;
};

SignalDetailPanel::SignalDetailPanel(QWidget* parent)
    : QWidget(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
}

SignalDetailPanel::~SignalDetailPanel() = default;

void SignalDetailPanel::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // 摘要区域
    auto* summaryWidget = createSummaryWidget();
    mainLayout->addWidget(summaryWidget);

    // 理论详情区域
    auto* theoryWidget = createTheoryDetailsWidget();
    mainLayout->addWidget(theoryWidget);

    // 风险提示区域
    auto* riskWidget = createRiskWidget();
    mainLayout->addWidget(riskWidget);

    // 操作按钮区域
    auto* actionWidget = createActionWidget();
    mainLayout->addWidget(actionWidget);

    mainLayout->addStretch();
}

QWidget* SignalDetailPanel::createSummaryWidget()
{
    auto* group = new QGroupBox(QStringLiteral("信号摘要"));
    auto* layout = new QVBoxLayout(group);

    // 标的和时间
    auto* headerLayout = new QHBoxLayout();
    d->symbolLabel = new QLabel("--");
    d->symbolLabel->setStyleSheet("font-size: 16px; font-weight: bold;");
    headerLayout->addWidget(d->symbolLabel);

    d->timeLabel = new QLabel("--");
    d->timeLabel->setStyleSheet("color: #888;");
    headerLayout->addStretch();
    headerLayout->addWidget(d->timeLabel);

    layout->addLayout(headerLayout);

    // 方向和置信度
    auto* infoLayout = new QHBoxLayout();

    auto* dirLayout = new QVBoxLayout();
    dirLayout->addWidget(new QLabel(QStringLiteral("信号方向:")));
    d->directionLabel = new QLabel("--");
    d->directionLabel->setStyleSheet("font-size: 14px; font-weight: bold;");
    dirLayout->addWidget(d->directionLabel);
    infoLayout->addLayout(dirLayout);

    auto* confLayout = new QVBoxLayout();
    confLayout->addWidget(new QLabel(QStringLiteral("置信度:")));
    d->confidenceBar = new QProgressBar();
    d->confidenceBar->setRange(0, 100);
    d->confidenceBar->setTextVisible(true);
    d->confidenceBar->setFormat("%p%");
    confLayout->addWidget(d->confidenceBar);
    infoLayout->addLayout(confLayout);

    layout->addLayout(infoLayout);

    // 理论数量和得分
    auto* scoreLayout = new QHBoxLayout();
    scoreLayout->addWidget(new QLabel(QStringLiteral("支持理论:")));
    d->theoryCountLabel = new QLabel("0 / 4");
    scoreLayout->addWidget(d->theoryCountLabel);

    scoreLayout->addSpacing(20);

    scoreLayout->addWidget(new QLabel(QStringLiteral("综合得分:")));
    d->scoreLabel = new QLabel("0");
    d->scoreLabel->setStyleSheet("font-weight: bold;");
    scoreLayout->addWidget(d->scoreLabel);

    scoreLayout->addStretch();
    layout->addLayout(scoreLayout);

    // 描述
    d->descriptionLabel = new QLabel(QStringLiteral("暂无信号"));
    d->descriptionLabel->setWordWrap(true);
    d->descriptionLabel->setStyleSheet(QString(
        "color: %1; padding: 5px; background: %2; border-radius: 3px;"
    ).arg(Tokens::Colors::TextSecondary, Tokens::Colors::BgHover));
    layout->addWidget(d->descriptionLabel);

    return group;
}

QWidget* SignalDetailPanel::createTheoryDetailsWidget()
{
    auto* group = new QGroupBox(QStringLiteral("各理论分析详情"));
    auto* layout = new QVBoxLayout(group);

    d->theoryTable = new QTableWidget();
    d->theoryTable->setColumnCount(4);
    d->theoryTable->setHorizontalHeaderLabels({
        QStringLiteral("理论"),
        QStringLiteral("方向"),
        QStringLiteral("强度"),
        QStringLiteral("置信度")
    });

    d->theoryTable->horizontalHeader()->setStretchLastSection(true);
    d->theoryTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    d->theoryTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    d->theoryTable->setMaximumHeight(150);

    layout->addWidget(d->theoryTable);

    return group;
}

QWidget* SignalDetailPanel::createRiskWidget()
{
    auto* group = new QGroupBox(QStringLiteral("风险提示"));
    auto* layout = new QVBoxLayout(group);

    d->riskText = new QTextEdit();
    d->riskText->setReadOnly(true);
    d->riskText->setMaximumHeight(80);
    d->riskText->setPlainText(QStringLiteral("暂无风险提示"));

    layout->addWidget(d->riskText);

    return group;
}

QWidget* SignalDetailPanel::createActionWidget()
{
    auto* widget = new QWidget();
    auto* layout = new QHBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);

    d->subscribeBtn = new QPushButton(QStringLiteral("订阅信号"));
    d->subscribeBtn->setIcon(QIcon(":/icons/subscribe.png"));
    layout->addWidget(d->subscribeBtn);

    d->historyBtn = new QPushButton(QStringLiteral("历史信号"));
    d->historyBtn->setIcon(QIcon(":/icons/history.png"));
    layout->addWidget(d->historyBtn);

    layout->addStretch();

    // 连接信号
    connect(d->subscribeBtn, &QPushButton::clicked, this, [this]() {
        emit subscribeRequested(d->compositeSignal.symbol);
    });

    connect(d->historyBtn, &QPushButton::clicked, this, [this]() {
        emit historyRequested(d->compositeSignal.symbol);
    });

    return widget;
}

void SignalDetailPanel::setCompositeSignal(const Analysis::CompositeSignal& signal)
{
    d->compositeSignal = signal;
    d->hasCompositeSignal = true;

    updateSummary();
    updateTheoryDetails();
    updateRiskInfo();
    updateActions();
}

void SignalDetailPanel::setUnifiedSignal(const Analysis::UnifiedSignal& signal)
{
    d->unifiedSignal = signal;
    d->hasCompositeSignal = false;

    // 转换为综合信号显示
    Analysis::CompositeSignal composite;
    composite.symbol = signal.symbol;
    composite.time = signal.time;
    composite.price = signal.price;
    composite.direction = signal.direction;
    composite.confidence = signal.confidence;
    composite.theoryCount = 1;
    composite.sourceSignals.append(signal);
    composite.description = signal.description;

    setCompositeSignal(composite);
}

void SignalDetailPanel::clear()
{
    d->symbolLabel->setText("--");
    d->timeLabel->setText("--");
    d->directionLabel->setText("--");
    d->confidenceBar->setValue(0);
    d->theoryCountLabel->setText("0 / 4");
    d->scoreLabel->setText("0");
    d->descriptionLabel->setText(QStringLiteral("暂无信号"));
    d->theoryTable->setRowCount(0);
    d->riskText->setPlainText(QStringLiteral("暂无风险提示"));
}

void SignalDetailPanel::setCompactMode(bool compact)
{
    d->compactMode = compact;
    // 可以根据模式调整布局
}

void SignalDetailPanel::updateSummary()
{
    const auto& signal = d->compositeSignal;

    // 标的
    d->symbolLabel->setText(signal.symbol.isEmpty() ? "--" : signal.symbol);

    // 时间
    d->timeLabel->setText(signal.time.toString("yyyy-MM-dd hh:mm:ss"));

    // 方向
    QString directionText;
    QColor directionColor;
    if (signal.direction == Analysis::SignalDirection::Bullish) {
        directionText = QStringLiteral("📈 看涨");
        directionColor = QColor(Tokens::Colors::Success);
    } else if (signal.direction == Analysis::SignalDirection::Bearish) {
        directionText = QStringLiteral("📉 看跌");
        directionColor = QColor(Tokens::Colors::Danger);
    } else {
        directionText = QStringLiteral("➡️ 中性");
        directionColor = QColor(Tokens::Colors::TextSecondary);
    }
    d->directionLabel->setText(directionText);
    d->directionLabel->setStyleSheet(QString("font-size: %1px; font-weight: bold; color: %2;")
        .arg(Tokens::Font::Size::Body)
        .arg(directionColor.name()));

    // 置信度
    d->confidenceBar->setValue(static_cast<int>(signal.confidence));

    // 设置置信度颜色
    QString barStyle;
    if (signal.confidence >= 80) {
        barStyle = QString("QProgressBar::chunk { background-color: %1; }").arg(Tokens::Colors::Success);
    } else if (signal.confidence >= 60) {
        barStyle = QString("QProgressBar::chunk { background-color: %1; }").arg(Tokens::Colors::Warning);
    } else {
        barStyle = QString("QProgressBar::chunk { background-color: %1; }").arg(Tokens::Colors::Danger);
    }
    d->confidenceBar->setStyleSheet(barStyle);

    // 理论数量
    d->theoryCountLabel->setText(QString("%1 / 4").arg(signal.theoryCount));

    // 得分
    d->scoreLabel->setText(QString::number(signal.score(), 'f', 1));

    // 描述
    d->descriptionLabel->setText(signal.description.isEmpty() ?
        QStringLiteral("暂无描述") : signal.description);
}

void SignalDetailPanel::updateTheoryDetails()
{
    const auto& signalList = d->compositeSignal.sourceSignals;

    d->theoryTable->setRowCount(signalList.size());

    for (int i = 0; i < signalList.size(); ++i) {
        const auto& signal = signalList[i];

        // 理论名称
        auto* theoryItem = new QTableWidgetItem(signal.theoryName());
        d->theoryTable->setItem(i, 0, theoryItem);

        // 方向
        QString dirText;
        QColor dirColor;
        if (signal.direction == Analysis::SignalDirection::Bullish) {
            dirText = QStringLiteral("看涨");
            dirColor = QColor(Tokens::Colors::Success);
        } else if (signal.direction == Analysis::SignalDirection::Bearish) {
            dirText = QStringLiteral("看跌");
            dirColor = QColor(Tokens::Colors::Danger);
        } else {
            dirText = QStringLiteral("中性");
            dirColor = QColor(Tokens::Colors::TextSecondary);
        }
        auto* dirItem = new QTableWidgetItem(dirText);
        dirItem->setForeground(dirColor);
        d->theoryTable->setItem(i, 1, dirItem);

        // 强度
        QString strengthText;
        switch (signal.strength) {
            case Analysis::SignalStrength::VeryStrong:
                strengthText = QStringLiteral("极强");
                break;
            case Analysis::SignalStrength::Strong:
                strengthText = QStringLiteral("强");
                break;
            case Analysis::SignalStrength::Moderate:
                strengthText = QStringLiteral("中");
                break;
            default:
                strengthText = QStringLiteral("弱");
                break;
        }
        auto* strengthItem = new QTableWidgetItem(strengthText);
        d->theoryTable->setItem(i, 2, strengthItem);

        // 置信度
        auto* confItem = new QTableWidgetItem(QString::number(signal.confidence, 'f', 1) + "%");
        d->theoryTable->setItem(i, 3, confItem);
    }

    d->theoryTable->resizeColumnsToContents();
}

void SignalDetailPanel::updateRiskInfo()
{
    QString riskText;

    const auto& signal = d->compositeSignal;

    if (signal.confidence >= 80) {
        riskText = QStringLiteral("✓ 高质量信号，可考虑入场\n");
        riskText += QStringLiteral("建议：设置止损位，控制仓位在30%以内");
    } else if (signal.confidence >= 60) {
        riskText = QStringLiteral("⚠ 中等质量信号，建议结合其他指标\n");
        riskText += QStringLiteral("建议：轻仓试探，严格止损");
    } else {
        riskText = QStringLiteral("✗ 信号质量较低，建议观望\n");
        riskText += QStringLiteral("建议：等待更明确的信号");
    }

    if (signal.theoryCount >= 3) {
        riskText += QStringLiteral("\n✓ 多理论共振，信号可靠性较高");
    } else if (signal.theoryCount < 2) {
        riskText += QStringLiteral("\n⚠ 支持理论较少，注意风险");
    }

    d->riskText->setPlainText(riskText);
}

void SignalDetailPanel::updateActions()
{
    d->subscribeBtn->setEnabled(!d->compositeSignal.symbol.isEmpty());
    d->historyBtn->setEnabled(!d->compositeSignal.symbol.isEmpty());
}

QString SignalDetailPanel::getDirectionIcon(Analysis::SignalDirection direction)
{
    switch (direction) {
        case Analysis::SignalDirection::Bullish:
            return "📈";
        case Analysis::SignalDirection::Bearish:
            return "📉";
        default:
            return "➡️";
    }
}

QString SignalDetailPanel::getStrengthIcon(Analysis::SignalStrength strength)
{
    switch (strength) {
        case Analysis::SignalStrength::VeryStrong:
            return "⭐⭐⭐";
        case Analysis::SignalStrength::Strong:
            return "⭐⭐";
        case Analysis::SignalStrength::Moderate:
            return "⭐";
        default:
            return "☆";
    }
}

QColor SignalDetailPanel::getDirectionColor(Analysis::SignalDirection direction)
{
    switch (direction) {
        case Analysis::SignalDirection::Bullish:
            return QColor(Tokens::Colors::Success);
        case Analysis::SignalDirection::Bearish:
            return QColor(Tokens::Colors::Danger);
        default:
            return QColor(Tokens::Colors::TextSecondary);
    }
}

} // namespace UI
} // namespace WealthPilot
