#ifndef SYNCWIDGET_H
#define SYNCWIDGET_H

#include <QWidget>

namespace Ui
{
class SyncWidget;
}

class SyncWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SyncWidget(QWidget *parent = nullptr);
    ~SyncWidget();

public slots:
    void setProgress(double progress);

private:
    Ui::SyncWidget *ui;
};

#endif // SYNCWIDGET_H
