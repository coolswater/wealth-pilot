#ifndef CARDWIDGET_H
#define CARDWIDGET_H

#include "BaseWidget.h"
#include <QLabel>
#include <QVBoxLayout>

/**
 * @brief 卡片控件
 *
 * 带标题、内容和悬停动效的卡片组件
 */
class CardWidget : public BaseWidget
{
    Q_OBJECT

public:
    explicit CardWidget(const QString& title = QString(), QWidget *parent = nullptr);
    ~CardWidget();

    void setTitle(const QString& title);
    QString title() const;

    void setContent(QWidget* content);
    QWidget* content() const;

    void setIcon(const QString& iconPath);

    // 设置边框颜色
    void setBorderColor(const QColor& color);
    void resetBorderColor();

    // 设置背景色
    void setBackgroundColor(const QColor& color);
    void resetBackgroundColor();

protected:
    void onHoverEnter() override;
    void onHoverLeave() override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void setupUI();
    void applyStyle();

    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // CARDWIDGET_H
