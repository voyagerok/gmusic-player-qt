#ifndef SETTINGSSTORE_H
#define SETTINGSSTORE_H

#include <QObject>
#include <QVariant>

class QSettings;

class SettingsStore : public QObject
{
    Q_OBJECT
public:
    static SettingsStore *instance();
    QVariant getValue(const QString &key);
    void setValue(const QString &key, const QVariant &value);

public slots:
    void sync();

private:
    explicit SettingsStore(QObject *parent = nullptr);
    QSettings *settings_;
};

#endif // SETTINGSSTORE_H
