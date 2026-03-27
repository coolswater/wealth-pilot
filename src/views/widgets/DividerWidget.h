#ifndef DIVIDER_H
#define DIVIDER_H

#include <QWidget>
#include <QColor>
#include <QPen>
#include <QPainter>
#include <QStyleOption>
#include <QPaintEvent>

class DividerWidget : public QWidget
{
    Q_OBJECT
    // Qt属性系统，支持QSS和Designer
    Q_PROPERTY(QColor lineColor READ lineColor WRITE setLineColor NOTIFY lineColorChanged)
    Q_PROPERTY(int lineWidth READ lineWidth WRITE setLineWidth NOTIFY lineWidthChanged)
    Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation NOTIFY orientationChanged)
    Q_PROPERTY(int margin READ margin WRITE setMargin NOTIFY marginChanged)

public:
    explicit DividerWidget(Qt::Orientation orientation = Qt::Horizontal, QWidget *parent = nullptr);
    explicit DividerWidget(QWidget *parent = nullptr);

    ~DividerWidget() override = default;

    // 颜色相关
    QColor lineColor() const;
    void setLineColor(const QColor &color);

    // 线宽相关（水平时为高度，垂直时为宽度）
    int lineWidth() const;
    void setLineWidth(int width);

    // 方向相关
    Qt::Orientation orientation() const;
    void setOrientation(Qt::Orientation orientation);

    // 边距相关（距离两端的空白距离）
    int margin() const;
    void setMargin(int margin);

    // 便捷方法：创建水平分割线
    static DividerWidget* createHorizontal(QWidget *parent = nullptr,
                                     const QColor &color = QColor(200, 200, 200),
                                     int width = 1,
                                     int margin = 0);

    // 便捷方法：创建垂直分割线
    static DividerWidget* createVertical(QWidget *parent = nullptr,
                                   const QColor &color = QColor(200, 200, 200),
                                   int width = 1,
                                   int margin = 0);

signals:
    void lineColorChanged(const QColor &color);
    void lineWidthChanged(int width);
    void orientationChanged(Qt::Orientation orientation);
    void marginChanged(int margin);

protected:
    void paintEvent(QPaintEvent *event) override;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

private:
    QColor m_lineColor{QColor(200, 200, 200)};  // 默认浅灰色
    int m_lineWidth{1};                          // 默认1像素
    int m_margin{0};                             // 默认无边距
    Qt::Orientation m_orientation{Qt::Horizontal};

    void updateSizePolicy();
};

#endif // DIVIDER_H
