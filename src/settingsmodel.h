#ifndef SETTINGSMODEL_H
#define SETTINGSMODEL_H

#include <QMap>
#include <QObject>

class QSettings;

class SettingsModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString authToken READ authToken WRITE setAuthToken NOTIFY authTokenChanged)
    Q_PROPERTY(QString masterToken READ masterToken WRITE setMasterToken NOTIFY masterTokenChanged)
    Q_PROPERTY(QString email READ email WRITE setEmail NOTIFY emailChanged)
    Q_PROPERTY(QString deviceId READ deviceId WRITE setDeviceId NOTIFY deviceIdChanged)
    Q_PROPERTY(bool autoLogin READ autoLogin WRITE setAutoLogin NOTIFY autoLoginChanged)
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY mutedChanged)
public:
    using SettingsValue = std::pair<QString, QVariant>;

    explicit SettingsModel(QObject *parent = nullptr);

    QString authToken() const;
    QString masterToken() const;
    QString email() const;
    QString deviceId() const;
    bool autoLogin() const;
    int volume() const;
    bool muted() const;

public slots:
    void setAuthToken(const QString &authToken);
    void setMasterToken(const QString &masterToken);
    void setEmail(const QString &email);
    void setDeviceId(const QString &deviceId);
    void setAutoLogin(bool);
    void setVolume(int volume);
    void setMuted(bool muted);

    void load();
    void save();

signals:
    void authTokenChanged(QString);
    void masterTokenChanged(QString);
    void emailChanged(QString);
    void deviceIdChanged(QString);
    void autoLoginChanged(bool);
    void volumeChanged(int volume);
    void mutedChanged(bool muted);

private:
    QSettings *settings_;

    QString authToken_;
    QString masterToken_;
    QString email_;
    QString deviceId_;
    bool autoLogin_;
    int volume_;
    bool muted_;

    static QVariantMap defaultValues_;
};

#endif // SETTINGSMODEL_H
