#ifndef LOGINWIDGET_H
#define LOGINWIDGET_H

#include <QWidget>
#include <memory>

class User;

using UserPtr = std::shared_ptr<User>;

namespace Ui
{
class LoginWidget;
}

class LoginWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LoginWidget(QWidget *parent = nullptr);
    ~LoginWidget();

public slots:
    void setEmail(const QString &email);
    void setAutoLogin(bool);

private slots:
    void onLoginRequested();

signals:
    void emailChanged(const QString &email);
    void login(const QString &passwd);
    void statusChanged(const QString &text);
    void autoLoginChanged(bool);

private:
    Ui::LoginWidget *ui;
};

#endif // LOGINWIDGET_H
