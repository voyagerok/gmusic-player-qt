#include "tracklistmodel.h"

#include <QDebug>
#include <QPixmapCache>
#include <QStringBuilder>
#include <QTimer>

#include "imagestorage.h"
#include "utils.h"

TrackListModel::TrackListModel(QObject *parent)
    : QAbstractTableModel(parent), imageStorage_(ImageStorage::instance())
{
    deferredUpdateTimer_ = new QTimer(this);
    deferredUpdateTimer_->setSingleShot(true);
    deferredUpdateTimer_->setInterval(3000);
    connect(deferredUpdateTimer_, &QTimer::timeout, this, &TrackListModel::reloadDecoationData);
    connect(&imageStorage_, SIGNAL(imageUpdated(QString)), deferredUpdateTimer_, SLOT(start()));
}

int TrackListModel::rowCount(const QModelIndex &parent) const
{
    return tracks_.size();
}

int TrackListModel::columnCount(const QModelIndex &parent) const
{
    return 5;
}

QVariant TrackListModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid()) {
        const auto &track = tracks_[index.row()];
        if (role == Qt::DisplayRole) {
            switch (index.column()) {
            case 0:
                return track.trackNumber;
            case 1:
                return track.title;
            case 2:
                return albumString(track);
            case 3:
                return artistString(track);
            case 4:
                return track.duration_string();
            default:
                return QVariant();
            }
        } else if (role == Qt::SizeHintRole) {
            return QSize(INT_MAX, 30);
        } else if (role == Roles::Number) {
            return track.trackNumber;
        } else if (role == Roles::Title) {
            return track.title;
        } else if (role == Roles::Album) {
            return albumString(track);
        } else if (role == Roles::Artist) {
            return artistString(track);
        } else if (role == Roles::Duration) {
            return track.duration_string();
        } else if (role == Roles::TrackId) {
            return track.id;
        }
    }

    return QVariant();
}

QVariant TrackListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case 0:
            return tr("#");
        case 1:
            return tr("Title");
        case 2:
            return tr("Album");
        case 3:
            return tr("Artist");
        case 4:
            return tr("Duration");
        }
    }

    return QVariant();
}

QHash<int, QByteArray> TrackListModel::roleNames() const
{
    QHash<int, QByteArray> result;
    result[Roles::Number]   = "number";
    result[Roles::Title]    = "title";
    result[Roles::Album]    = "album";
    result[Roles::Artist]   = "artist";
    result[Roles::Duration] = "duration";
    result[Roles::TrackId]  = "trackId";
    return result;
}

void TrackListModel::reloadTracks()
{
    if (loader_) {
        beginResetModel();
        if (auto tracks = loader_(db_)) {
            tracks_ = *tracks;
        }
        endResetModel();
    }
}

void TrackListModel::setDatabasePath(const QString &dbPath)
{
    if (!db_.openConnection(dbPath, Utils::ThreadId())) {
        qWarning() << "could not open database connection";
    }
}

inline QVariant TrackListModel::albumString(const GMTrack &track) const
{
    if (auto album = db_.album(track.albumId)) {
        return album->name;
    }
    return QVariant();
}

inline QVariant TrackListModel::artistString(const GMTrack &track) const
{
    if (!track.artistId.isEmpty()) {
        Opt<GMArtist> artist = db_.artist(track.artistId.at(0));
        if (artist) {
            return artist->name;
        }
    }
    return QString();
}

Opt<GMTrackList> TrackListModel::loadAllTracks(Database &db)
{
    return db.tracks();
}

void TrackListModel::setLoaderFunc(const Loader &loader)
{
    loader_ = loader;
    reloadTracks();
}

void TrackListModel::reloadDecoationData()
{
    emit dataChanged(index(0, 2), index(tracks_.size() - 1, 2), QVector<int>{Qt::DecorationRole});
    emit dataChanged(index(0, 3), index(tracks_.size() - 1, 3), QVector<int>{Qt::DecorationRole});
}

QPixmap TrackListModel::getPixmap(const QString &url) const
{
    QPixmap *cachedImg = QPixmapCache::find(url);
    if (cachedImg)
        return *cachedImg;
    QByteArray imageData = imageStorage_.tryGetImage(url);
    if (!imageData.isEmpty()) {
        QPixmap img;
        if (img.loadFromData(imageData)) {
            QPixmap scaledImg = img.scaled(24, 24, Qt::KeepAspectRatio);
            QPixmapCache::insert(url, scaledImg);
            return scaledImg;
        }
    }
    return QPixmap();
}

QModelIndex TrackListModel::getIndexForId(const QString &trackId)
{
    for (int i = 0; i < tracks_.size(); ++i) {
        if (tracks_[i].id == trackId) {
            return index(i, 0);
        }
    }
    return QModelIndex();
}

QModelIndex TrackListModel::getNext(const QString &trackId)
{
    for (int i = 0; i < tracks_.size(); ++i) {
        if (tracks_[i].id == trackId) {
            return index((i + 1) % tracks_.size(), 0);
        }
    }
    return QModelIndex();
}

QModelIndex TrackListModel::getPrev(const QString &trackId)
{
    for (int i = 0; i < tracks_.size(); ++i) {
        if (tracks_[i].id == trackId) {
            int prevIndex = i == 0 ? tracks_.size() - 1 : i - 1;
            return index(prevIndex, 0);
        }
    }
    return QModelIndex();
}

TrackListSortingModel::TrackListSortingModel(QObject *parent) : QSortFilterProxyModel(parent)
{
}

bool TrackListSortingModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    auto artistLeft  = left.data(TrackListModel::Artist).toString();
    auto artistRight = right.data(TrackListModel::Artist).toString();
    if (artistLeft != artistRight) {
        return artistLeft < artistRight;
    }
    auto albumLeft  = left.data(TrackListModel::Album).toString();
    auto albumRight = right.data(TrackListModel::Album).toString();
    if (albumLeft != albumRight) {
        return albumLeft < albumRight;
    }
    auto songNumLeft  = left.data(TrackListModel::Number).toInt();
    auto songNumRight = right.data(TrackListModel::Number).toInt();
    return songNumLeft < songNumRight;
}
