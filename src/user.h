#ifndef USER_H
#define USER_H

#include <QEventLoop>
#include <QObject>
#include <QThread>

#include "database.h"
#include "gmapi.h"

class SyncWorker : public QObject
{
    Q_OBJECT

public:
    SyncWorker(const QString &token, QObject *parent = nullptr);
signals:
    void progressChanged(double progress);
    void errorOccured(QString error);
    void started();
    void finished();

public slots:
    void run(QString dbPath);

private:
    void removeDeletedTracks(const GMTrackList &remoteTrackList, const GMTrackList &localtracks);
    void mergeRemoteTracks(const GMTrackList &remoteTrackList, const GMTrackList &localtracks);
    void processTrack(const GMTrack &track);

    template <class T> T wait_result(ProxyResult *proxy)
    {
        QEventLoop loop;
        int status;
        QVariant value;
        connect(proxy, &ProxyResult::ready, this,
                [&status, &value, &loop](int _status, QVariant _value) {
                    status = _status;
                    value  = _value;
                    loop.quit();
                });
        loop.exec();
        if (status != ProxyResult::OK) {
            throw std::runtime_error(value.toString().toStdString());
        }
        return value.value<T>();
    }

    QString token_;
    Database db_;
    GMApi *api_;

    QSet<QString> albumsCache_;
    QSet<QString> artistsCache_;
};

class QThreadPool;

class User : public QObject
{
    Q_OBJECT

public:
    User(QObject *parent = nullptr);
    ~User() override;

    QString email() const
    {
        return email_;
    }
    QString deviceId() const
    {
        return deviceId_;
    }
    bool authorized() const
    {
        return !authToken_.isEmpty();
    }
    QString masterToken() const
    {
        return masterToken_;
    }
    QString authToken() const
    {
        return authToken_;
    }
    QString databasePath() const
    {
        return databasePath_;
    }

    Q_INVOKABLE void login(const QString &passwd);
    Q_INVOKABLE void sync();
    Q_INVOKABLE void restore();

    bool canRestore() const;

    ProxyResult *getStreamUrl(const QString &trackId)
    {
        return api_.streamUrl(authToken_, trackId, deviceId_);
    }

signals:
    void emailChanged(QString);
    void deviceIdChanged(QString);
    void authorizedChanged(bool);
    void masterTokenChanged(QString);
    void authTokenChanged(QString);
    void databasePathChanged(QString);

    void errorOccured(const QString &msg);
    void syncProgressChanged(double progress);
    void syncStarted();
    void syncFinished();

public slots:
    void requestSyncInterruption();
    void setEmail(const QString &email)
    {
        if (email_ != email) {
            email_ = email;
            emit emailChanged(email);
        }
    }
    void setDeviceId(const QString &deviceId)
    {
        if (deviceId_ != deviceId) {
            deviceId_ = deviceId;
            emit deviceIdChanged(deviceId);
        }
    }
    void setMasterToken(const QString &masterToken)
    {
        if (masterToken_ != masterToken) {
            masterToken_ = masterToken;
            emit masterTokenChanged(masterToken);
        }
    }
    void setAuthToken(const QString &authToken)
    {
        if (authToken_ != authToken) {
            authToken_ = authToken;
            emit authTokenChanged(authToken);
        }
    }
    void setDatabasePath(const QString &databasePath)
    {
        if (databasePath_ != databasePath) {
            databasePath_ = databasePath;
            emit databasePathChanged(databasePath);
        }
    }

private:
    void extractDeviceId();
    void login_(const QString &deviceId, const QString &passwd);

    QString email_;
    QString deviceId_;
    QString masterToken_;
    QString authToken_;
    QString databasePath_;

    void getAuthToken();

    GMApi api_;
    Database db_;
    bool syncInProgress_;
    QThread *syncThread_;

    void createUserData();
};

#endif // USER_H
