#include "user.h"

#include <QCryptographicHash>
#include <QDir>
#include <QStandardPaths>
#include <QtMultimedia/QMediaPlayer>

#include "utils.h"

SyncWorker::SyncWorker(const QString &token, QObject *parent) : QObject(parent), token_(token)
{
    api_ = new GMApi(this);
}

void SyncWorker::run(QString dbPath)
{
    if (!db_.openConnection(dbPath, Utils::ThreadId())) {
        qDebug() << __PRETTY_FUNCTION__ << ": could not open database connection";
        return;
    }
    emit started();
    try {
        GMTrackList allTracks;
        do {
            allTracks.append(wait_result<GMTrackList>(api_->tracks(token_)));
            if (thread()->isInterruptionRequested()) {
                qDebug() << "SyncThread: interruption requested";
                emit finished();
                return;
            }
        } while (api_->hasMoreTracks());
        auto localtracks = db_.tracks();
        if (!localtracks) {
            qDebug() << __PRETTY_FUNCTION__ << ": could not extract tracks";
            emit errorOccured(tr("Could not load tracks from database"));
            return;
        }
        removeDeletedTracks(allTracks, *localtracks);
        if (thread()->isInterruptionRequested()) {
            qDebug() << "SyncThread: interruption requested";
            emit finished();
            return;
        }
        mergeRemoteTracks(allTracks, *localtracks);
        if (thread()->isInterruptionRequested()) {
            qDebug() << "SyncThread: interruption requested";
            emit finished();
            return;
        }
        emit finished();
    } catch (const std::exception &e) {
        emit errorOccured(e.what());
    } catch (...) {
        emit errorOccured(tr("Unknown error"));
    }
}

void SyncWorker::removeDeletedTracks(const GMTrackList &remoteTrackList,
                                     const GMTrackList &localTracks)
{
    QSet<QString> remoteTrackSet;
    remoteTrackSet.reserve(remoteTrackList.size());
    for (int i = 0; i < remoteTrackList.size(); ++i) {
        remoteTrackSet.insert(remoteTrackList[i].id);
    }
    if (thread()->isInterruptionRequested()) {
        return;
    }

    for (int i = 0; i < localTracks.size(); ++i) {
        const auto &localTrack = localTracks.at(i);
        if (!remoteTrackSet.contains(localTrack.id)) {
            db_.removeTrack(localTrack.id);
        }
        if (thread()->isInterruptionRequested()) {
            return;
        }
    }
}

void SyncWorker::processTrack(const GMTrack &track)
{
    if (!track.artistId.isEmpty()) {
        QString artistId = track.artistId.at(0);
        if (!artistsCache_.contains(artistId)) {
            GMArtist artist = wait_result<GMArtist>(api_->artist(token_, artistId));
            bool success    = db_.insertArtist(artist);
            if (success) {
                artistsCache_.insert(artist.artistId);
            }
        }
    }
    if (!albumsCache_.contains(track.albumId)) {
        GMAlbum album = wait_result<GMAlbum>(api_->album(token_, track.albumId));
        bool success  = db_.insertAlbum(album);
        if (success) {
            albumsCache_.insert(album.albumId);
        }
    }
    db_.insertTrack(track);
}

void SyncWorker::mergeRemoteTracks(const GMTrackList &remoteTrackList,
                                   const GMTrackList &localtracks)
{
    QSet<QString> localTrackSet;
    localTrackSet.reserve(localtracks.size());
    for (const auto &track : localtracks) {
        localTrackSet.insert(track.id);
    }

    for (int i = 0; i < remoteTrackList.size(); ++i) {
        if (thread()->isInterruptionRequested()) {
            return;
        }
        if (localTrackSet.contains(remoteTrackList[i].id)) {
            continue;
        }
        try {
            processTrack(remoteTrackList[i]);
        } catch (const GMApi::Error &e) {
            qDebug() << __FUNCTION__ << ": failed to prcoess track: " << e.what();
        }
        if (i % 50 == 0) {
            emit progressChanged((double)i / remoteTrackList.size());
        }
    }
    emit progressChanged(1.0);
}

User::User(QObject *parent) : QObject(parent), syncInProgress_(false)
{
    syncThread_ = new QThread(this);
    syncThread_->start();
}

User::~User()
{
    syncThread_->requestInterruption();
    syncThread_->quit();
    syncThread_->wait();
}

void User::login(const QString &passwd)
{
    QString deviceId = deviceId_;
    if (deviceId.isEmpty()) {
        deviceId = Utils::MacAddress();
    }
    login_(deviceId, passwd);
}

bool User::canRestore() const
{
    return !authToken_.isEmpty() || !masterToken_.isEmpty();
}

void User::restore()
{
    if (!authToken_.isEmpty()) {
        qDebug() << __FUNCTION__ << ": found auth token - nothing to do";
        createUserData();
        emit authorizedChanged(true);
    } else if (!masterToken_.isEmpty()) {
        qDebug() << __FUNCTION__ << ": "
                 << "Found master token: using it instead of password";
        getAuthToken();
    }
}

void User::login_(const QString &deviceId, const QString &passwd)
{
    auto proxy = api_.masterToken(email_, deviceId, passwd);
    connect(proxy, &ProxyResult::ready, this, [this](int status, QVariant result) {
        if (status == ProxyResult::OK) {
            this->setMasterToken(result.toString());
            this->getAuthToken();
        } else {
            qDebug() << result.toString();
            emit this->errorOccured(tr("Could not login: %1").arg(result.toString()));
        }
    });
}

void User::getAuthToken()
{
    ProxyResult *proxy = api_.authToken(email_, deviceId_, masterToken_);
    connect(proxy, &ProxyResult::ready, this, [this](int status, QVariant result) {
        if (status == ProxyResult::OK) {
            setAuthToken(result.toString());
            if (!deviceId_.isEmpty()) {
                this->createUserData();
                this->authorizedChanged(this->authorized());
            } else {
                this->extractDeviceId();
            }
        } else {
            qDebug() << result.toString();
            this->setMasterToken(QString());
            emit this->errorOccured(tr("Could not login: %1").arg(result.toString()));
        }
    });
}

void User::extractDeviceId()
{
    ProxyResult *proxy = api_.devices(authToken_);
    connect(proxy, &ProxyResult::ready, this, [this](int status, QVariant result) {
        if (status == ProxyResult::OK) {
            GMDeviceList devices = result.value<GMDeviceList>();
            QString id           = devices[0].id.replace(QRegExp("^0x"), "");
            setDeviceId(id);
            this->createUserData();
            this->authorizedChanged(this->authorized());
        } else {
            qDebug() << "Could not extract device id: " << result.toString();
            emit this->errorOccured(tr("Could not extract registered devices"));
        }
    });
}

void User::sync()
{
    auto syncWorker = new SyncWorker(authToken_);
    syncWorker->moveToThread(syncThread_);
    connect(syncWorker, &SyncWorker::progressChanged, this, &User::syncProgressChanged);
    connect(syncWorker, &SyncWorker::errorOccured, this, &User::errorOccured);
    connect(syncWorker, &SyncWorker::finished, this, &User::syncFinished);
    connect(syncWorker, &SyncWorker::started, this, &User::syncStarted);
    connect(syncThread_, &QThread::finished, syncWorker, &QObject::deleteLater);
    QMetaObject::invokeMethod(syncWorker, "run", Qt::QueuedConnection,
                              Q_ARG(QString, databasePath_));
}

void User::createUserData()
{
    QDir dataDir;
    QString appDataLocation = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!dataDir.exists(appDataLocation) && !dataDir.mkpath(appDataLocation)) {
        qDebug() << __FUNCTION__ << ": "
                 << "could not create app data directory";
    }
    dataDir.cd(appDataLocation);

    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(email_.toUtf8());
    hash.addData(deviceId_.toUtf8());
    QByteArray result = hash.result();
    QString userDir   = QString::fromLatin1(result.toHex());

    if (!dataDir.exists(userDir) && !dataDir.mkdir(userDir)) {
        qDebug() << __FUNCTION__ << ": "
                 << "could not create user data directory";
    }
    dataDir.cd(userDir);

    QString dbPath = dataDir.filePath(QStringLiteral("storage.sqlite"));
    if (!db_.openConnection(dbPath, Utils::ThreadId())) {
        qDebug() << __FUNCTION__ << ": could not open database connection";
        return;
    }
    if (!db_.createTables()) {
        qDebug() << __FUNCTION__ << ": could not create tables";
    }
    setDatabasePath(dbPath);
}

void User::requestSyncInterruption()
{
    syncThread_->requestInterruption();
}

// void User::play(const QString &trackId)
//{
//    ProxyResult *proxy = api_.streamUrl(authToken_, trackId, deviceId_);
//    connect(proxy, &ProxyResult::ready, this, [this](int status, QVariant result) {
//        if (status == ProxyResult::Error) {
//            qDebug() << "Failed to get stream url: " << result.toString();
//            emit this->errorOccured(tr("Could not play song"));
//            return;
//        }
//        if (player_->state() == QMediaPlayer::State::PlayingState) {
//            player_->stop();
//        }
//        player_->setMedia(QUrl(result.toString()));
//        player_->play();
//    });
//}
