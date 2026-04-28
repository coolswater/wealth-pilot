#ifndef STATUSBARWIDGET_H
#define STATUSBARWIDGET_H

#include "BaseWidget.h"
#include <QVector>

class QLineEdit;
class QTableWidget;
class QWidget;

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

signals:
    void stockSelected(const QString& code, const QString& name, const QString& exchange);

private slots:
    void onSearchTextChanged(const QString& text);
    void onSearchResultClicked(int row, int column);

private:
    void setupUI();
    void updateTime();
    void setupSearch();

    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // STATUSBARWIDGET_H
