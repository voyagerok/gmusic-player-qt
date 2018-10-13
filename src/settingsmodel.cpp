#include "settingsmodel.h"

#include <QMetaProperty>
#include <QSettings>

#include "settingsstore.h"

SettingsModel::SettingsModel(QObject *parent) : QObject(parent), autoLogin_(true)
{
    settings_ = new QSettings(this);
}

void SettingsModel::load()
{
    const QMetaObject *metaobject = this->metaObject();
    int count                     = metaobject->propertyCount();
    int offset                    = metaobject->propertyOffset();
    for (int i = offset; i < count; ++i) {
        QMetaProperty property = metaobject->property(i);
        const char *name       = property.name();
        this->setProperty(name, settings_->value(QString::fromLatin1(name)));
    }
}

void SettingsModel::save()
{
    const QMetaObject *metaobject = this->metaObject();
    int count                     = metaobject->propertyCount();
    int offset                    = metaobject->propertyOffset();
    for (int i = offset; i < count; ++i) {
        QMetaProperty property = metaobject->property(i);
        const char *name       = property.name();
        settings_->setValue(QString::fromLatin1(name), this->property(name));
    }
    settings_->sync();
}

void SettingsModel::setAuthToken(const QString &authToken)
{
    if (authToken_ != authToken) {
        authToken_ = authToken;
        emit authTokenChanged(authToken);
    }
}

void SettingsModel::setMasterToken(const QString &masterToken)
{
    if (masterToken_ != masterToken) {
        masterToken_ = masterToken;
        emit masterTokenChanged(masterToken);
    }
}

void SettingsModel::setEmail(const QString &email)
{
    if (email_ != email) {
        email_ = email;
        emit emailChanged(email);
    }
}

void SettingsModel::setDeviceId(const QString &deviceId)
{
    if (deviceId_ != deviceId) {
        deviceId_ = deviceId;
        emit deviceIdChanged(deviceId);
    }
}

void SettingsModel::setAutoLogin(bool autologin)
{
    if (autoLogin_ != autologin) {
        autoLogin_ = autologin;
        emit autoLoginChanged(autologin);
    }
}

QString SettingsModel::authToken() const
{
    return authToken_;
}

QString SettingsModel::masterToken() const
{
    return masterToken_;
}

QString SettingsModel::email() const
{
    return email_;
}

QString SettingsModel::deviceId() const
{
    return deviceId_;
}

bool SettingsModel::autoLogin() const
{
    return autoLogin_;
}
