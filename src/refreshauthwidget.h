#ifndef REFRESHAUTHWIDGET_H
#define REFRESHAUTHWIDGET_H

#include <QAbstractListModel>
#include <QWidget>

namespace Ui
{
class RefreshAuthWidget;
}

class RefreshAuthWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RefreshAuthWidget(QWidget *parent = nullptr);
    ~RefreshAuthWidget();

public slots:
    void setEmail(const QString &email);
    void setDeviceName(const QString &deviceName);

signals:
    void refresh();
    void forget();

private:
    Ui::RefreshAuthWidget *ui;

    QString email_;
    QString deviceName_;

private:
    void updateActiveAccountName();
};

#endif // REFRESHAUTHWIDGET_H
