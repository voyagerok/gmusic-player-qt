#ifndef CONTROLPANELWIDGET_H
#define CONTROLPANELWIDGET_H

#include <QWidget>

namespace Ui {
class ControlPanelWidget;
}

class ControlPanelWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ControlPanelWidget(QWidget *parent = nullptr);
    ~ControlPanelWidget();

private:
    Ui::ControlPanelWidget *ui;
};

#endif // CONTROLPANELWIDGET_H
