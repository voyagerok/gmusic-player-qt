#include "loginwidget.h"
#include "ui_loginwidget.h"

#include <QKeyEvent>

#include "settingsmodel.h"

LoginWidget::LoginWidget(QWidget *parent) : QWidget(parent), ui(new Ui::LoginWidget)
{
    ui->setupUi(this);

    connect(ui->txtEmail, &QLineEdit::textChanged, this, &LoginWidget::emailChanged);
    connect(ui->btnLogin, &QPushButton::clicked, this, &LoginWidget::onLoginRequested);
    connect(ui->txtEmail, &QLineEdit::returnPressed, this, &LoginWidget::onLoginRequested);
    connect(ui->txtPasswd, &QLineEdit::returnPressed, this, &LoginWidget::onLoginRequested);
    connect(ui->btnAutoLogin, &QCheckBox::toggled, this, &LoginWidget::autoLoginChanged);
}

void LoginWidget::setEmail(const QString &email)
{
    ui->txtEmail->setText(email);
}

void LoginWidget::onLoginRequested()
{
    if (ui->txtEmail->text().isEmpty()) {
        emit statusChanged(tr("Please provide your email address"));
        return;
    }
    if (ui->txtPasswd->text().isEmpty()) {
        emit statusChanged(tr("Please provide your password"));
        return;
    }
    emit login(ui->txtPasswd->text());
}

LoginWidget::~LoginWidget()
{
    delete ui;
}

void LoginWidget::setAutoLogin(bool autologin)
{
    ui->btnAutoLogin->setChecked(autologin);
}
