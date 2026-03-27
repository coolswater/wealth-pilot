#ifndef STATUSBARWIDGET_H
#define STATUSBARWIDGET_H

#include "BaseWidget.h"


class StatusBarWidget : public BaseWidget
{
    Q_OBJECT
public:
    StatusBarWidget(QWidget *parent = nullptr);
    ~StatusBarWidget();

public slots:
    void onCTPStatusChanged();
private:
    // 初始化UI布局
    void setupUI();

    // 初始化信号槽连接
    void initConnections();
    /**
     * @brief PIMPL实现结构体
     */
    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // STATUSBARWIDGET_H
