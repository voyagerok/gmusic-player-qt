#include "refreshauthwidget.h"
#include "ui_refreshauthwidget.h"

RefreshAuthWidget::RefreshAuthWidget(QWidget *parent)
    : QWidget(parent), ui(new Ui::RefreshAuthWidget)
{
    ui->setupUi(this);

    connect(ui->btnForget, &QPushButton::clicked, this, &RefreshAuthWidget::forget);
    connect(ui->btnRefresh, &QPushButton::clicked, this, &RefreshAuthWidget::refresh);
}

RefreshAuthWidget::~RefreshAuthWidget()
{
    delete ui;
}

void RefreshAuthWidget::setDeviceName(const QString &deviceName)
{
    deviceName_ = deviceName;
    updateActiveAccountName();
}

void RefreshAuthWidget::setEmail(const QString &email)
{
    email_ = email;
    updateActiveAccountName();
}

void RefreshAuthWidget::updateActiveAccountName()
{
    ui->accountLineEdit->setText(QString("%1 (%2)").arg(email_).arg(
        deviceName_.isEmpty() ? tr("Unknown device") : deviceName_));
}
