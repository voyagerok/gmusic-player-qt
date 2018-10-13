#include "controlpanelwidget.h"
#include "ui_controlpanelwidget.h"

ControlPanelWidget::ControlPanelWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ControlPanelWidget)
{
    ui->setupUi(this);
}

ControlPanelWidget::~ControlPanelWidget()
{
    delete ui;
}
