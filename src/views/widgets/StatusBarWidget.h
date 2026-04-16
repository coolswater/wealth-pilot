#ifndef STATUSBARWIDGET_H
#define STATUSBARWIDGET_H

#include "BaseWidget.h"

class StatusBarWidget : public BaseWidget
{
    Q_OBJECT
public:
    explicit StatusBarWidget(QWidget *parent = nullptr);
    ~StatusBarWidget();

public slots:
    void setAIStatus(const QString& status);
    void setCTPStatus(const QString& status);
    void setLatency(const QString& latency);
    void setVersion(const QString& version);

private:
    void setupUI();
    void updateTime();

    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // STATUSBARWIDGET_H
