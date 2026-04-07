#ifndef ASSETCARDWIDGET_H
#define ASSETCARDWIDGET_H

#include <QFrame>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPropertyAnimation>

    /**
 * @brief 统计卡片组件
 * 用于显示资产、盈亏等关键指标
 */
    class AssetCardWidget : public QFrame
{
    Q_OBJECT
    // 声明动画属性
    Q_PROPERTY(qreal value READ value WRITE setValue NOTIFY valueChanged)

public:
    enum CardTheme {
        Default,
        Success,    // 绿色 - 盈利
        Danger,     // 红色 - 亏损/风险
        Warning,    // 橙色 - 警告
        Primary     // 蓝色 - 主要信息
    };
    Q_ENUM(CardTheme)

    explicit AssetCardWidget(QWidget* parent = nullptr);
    ~AssetCardWidget();

    // 设置卡片数据
    void setTitle(const QString& title) const;
    void setValue(const QString& value, bool animated = false);
    void setSubValue(const QString& subValue) const;
    void setTrend(double percent, const QString& label = QString()) const;
    void setIcon(const QString& iconPath) const;
    void setTheme(CardTheme theme);

    // 动画支持
    qreal value() const { return m_currentValue; }
    void setValue(qreal value);

    // 启用/禁用悬停效果
    void setHoverEffect(bool enabled);

signals:
    void valueChanged(qreal value);
    void clicked();

protected:
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    void setupUI();
    void updateStyle();
    QString getTrendIcon(double percent) const;

    // UI 组件
    QLabel* m_iconLabel = nullptr;
    QLabel* m_titleLabel = nullptr;
    QLabel* m_valueLabel = nullptr;
    QLabel* m_subValueLabel = nullptr;
    QLabel* m_trendLabel = nullptr;

    // 数据
    CardTheme m_theme = Default;
    qreal m_currentValue = 0.0;
    qreal m_targetValue = 0.0;
    bool m_hoverEffect = true;

    // 动画
    QPropertyAnimation* m_valueAnimation = nullptr;
    QPropertyAnimation* m_scaleAnimation = nullptr;
};

#endif // ASSETCARDWIDGET_H
