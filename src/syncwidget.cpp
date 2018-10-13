#include "syncwidget.h"
#include "ui_syncwidget.h"

#include <cmath>

SyncWidget::SyncWidget(QWidget *parent) : QWidget(parent), ui(new Ui::SyncWidget)
{
    ui->setupUi(this);
}

SyncWidget::~SyncWidget()
{
    delete ui;
}

void SyncWidget::setProgress(double value)
{
    ui->progressBar->setValue((int)floor(value * 100));
}
