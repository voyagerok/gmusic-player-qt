#include "settingsstore.h"

#include <QSettings>

static SettingsStore *globalInstance = nullptr;

SettingsStore::SettingsStore(QObject *parent) : QObject(parent)
{
    settings_ = new QSettings(this);
}

SettingsStore *SettingsStore::instance()
{
    if (!globalInstance) {
        globalInstance = new SettingsStore;
    }
    return globalInstance;
}

QVariant SettingsStore::getValue(const QString &key)
{
    return settings_->value(key);
}

void SettingsStore::setValue(const QString &key, const QVariant &value)
{
    settings_->setValue(key, value);
}

void SettingsStore::sync()
{
    settings_->sync();
}
