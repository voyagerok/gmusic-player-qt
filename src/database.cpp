#include "database.h"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QThread>

#include "utils.h"

static QString CONNECTION_NAME = QStringLiteral("DEFAULT_CONNECTION");

Database::Database(QObject *parent) : QObject(parent)
{
}

Database::~Database()
{
}

bool Database::openConnection(const QString &path, QString connName)
{
    if (db_.isOpen()) {
        db_.close();
    }
    if (!QSqlDatabase::contains(connName)) {
        db_ = QSqlDatabase::addDatabase("QSQLITE", connName);
    } else {
        db_ = QSqlDatabase::database(connName);
    }
    db_.setDatabaseName(path);
    if (!db_.open()) {
        qWarning() << "Could not open database" << path << ":" << db_.lastError().text();
        return false;
    }
    return true;
}

bool Database::createSchema_(QSqlDatabase &db)
{
    QSqlQuery query(db);
    DBTransaction transaction(db);

    if (!query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS Artist(id TEXT "
                                   "PRIMARY KEY, name TEXT, artUrl TEXT, bio "
                                   "TEXT)"))) {
        qWarning() << db.lastError();
        return false;
    }

    if (!query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS Album(id TEXT "
                                   "PRIMARY KEY, name TEXT, artUrl TEXT, descr "
                                   "TEXT, year INTEGER)"))) {
        qWarning() << db.lastError();
        return false;
    }

    if (!query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS Track(id TEXT PRIMARY KEY,"
                                   "albumId REFERENCES Album(id),"
                                   "name TEXT,"
                                   "genre TEXT,"
                                   "duration INTEGER,"
                                   "trackNumber INTEGER,"
                                   "year INTEGER,"
                                   "trackType TEXT,"
                                   "size INTEGER)"))) {
        qWarning() << db.lastError();
        return false;
    }

    if (!query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS "
                                   "Track2Artist("
                                   "trackId REFERENCES Track(id), "
                                   "artistId REFERENCES Artist(id),"
                                   "PRIMARY KEY(trackId, artistId))"))) {
        qWarning() << db.lastError();
        return false;
    }

    if (!query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS "
                                   "Artist2Album("
                                   "artistId REFERENCES Artist(id), "
                                   "albumId REFERENCES Album(id),"
                                   "PRIMARY KEY(artistId, albumId))"))) {
        qWarning() << db.lastError();
        return false;
    }

    transaction.commit();

    return true;
}

std::optional<GMTrackList> Database::tracks_(QSqlDatabase &db)
{
    QSqlQuery query(db);
    if (!query.exec(QStringLiteral(
            "SELECT id, albumId, name, genre, duration, trackNumber, year, trackType, size "
            "from Track"))) {
        qWarning() << query.lastError();
        return std::nullopt;
    }
    return extractTracks(db, query);
}

Opt<GMTrackList> Database::tracks_for_album(QSqlDatabase &db, const QString &albumId)
{
    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT id, albumId, name, genre, duration, trackNumber, year, trackType, size "
        "FROM Track WHERE albumId = :albumId"));
    query.bindValue(":albumId", albumId);
    if (!query.exec()) {
        qWarning() << query.lastError();
        return std::nullopt;
    }
    return extractTracks(db, query);
}

Opt<GMTrackList> Database::tracks_for_artist(QSqlDatabase &db, const QString &artistId)
{
    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT a.id, a.albumId, a.name, a.genre, a.duration, a.trackNumber, a.year, a.trackType, "
        "a.size "
        "FROM Track a JOIN Track2Artist b on (a.id = b.trackId) and b.artistId = :artistId"));
    query.bindValue(":artistId", artistId);
    if (!query.exec()) {
        qWarning() << query.lastError();
        return std::nullopt;
    }
    return extractTracks(db, query);
}

Opt<GMTrackList> Database::extractTracks(QSqlDatabase &db, QSqlQuery &query)
{
    GMTrackList tracklist;
    while (query.next()) {
        GMTrack track;
        track.id             = query.value(0).toString();
        track.albumId        = query.value(1).toString();
        track.title          = query.value(2).toString();
        track.genre          = query.value(3).toString();
        track.durationMillis = query.value(4).toLongLong();
        track.trackNumber    = query.value(5).toInt();
        track.year           = query.value(6).toInt();
        track.trackType      = query.value(7).toString();
        track.estimatedSize  = query.value(8).toLongLong();

        QSqlQuery artistQuery(db);
        artistQuery.prepare(
            QStringLiteral("SELECT artistId FROM Track2Artist WHERE trackId = :trackId"));
        artistQuery.bindValue(":trackId", track.id);
        if (!artistQuery.exec()) {
            qWarning() << "could not extract artists for track:" << db.lastError();
        }
        while (artistQuery.next()) {
            track.artistId.append(artistQuery.value(0).toString());
        }
        tracklist.append(track);
    }
    return std::move(tracklist);
}

std::optional<GMTrack> Database::track_(QSqlDatabase &db, const QString &id)
{
    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT id, albumId, name, genre, duration, trackNumber, year, trackType, size "
        "from Track WHERE id = :trackId"));
    query.bindValue(":trackId", id);
    if (!query.exec()) {
        qWarning() << query.lastError();
        return std::nullopt;
    }
    GMTrack track;
    if (query.next()) {
        track.id             = query.value(0).toString();
        track.albumId        = query.value(1).toString();
        track.title          = query.value(2).toString();
        track.genre          = query.value(3).toString();
        track.durationMillis = query.value(4).toLongLong();
        track.trackNumber    = query.value(5).toInt();
        track.year           = query.value(6).toInt();
        track.trackType      = query.value(7).toString();
        track.estimatedSize  = query.value(8).toLongLong();

        QSqlQuery artistQuery(db);
        artistQuery.prepare(
            QStringLiteral("SELECT artistId FROM Track2Artist WHERE trackId = :trackId"));
        artistQuery.bindValue(":trackId", track.id);
        if (!artistQuery.exec()) {
            qWarning() << "could not get artists for tracks" << artistQuery.lastError();
        }
        while (artistQuery.next()) {
            track.artistId.append(artistQuery.value(0).toString());
        }
    }
    return std::move(track);
}

bool Database::insertTrack_(QSqlDatabase &db, const GMTrack &track)
{
    DBTransaction transaction(db);
    QSqlQuery query(db);
    query.prepare(
        QStringLiteral("INSERT OR REPLACE INTO Track (id, albumId, name, genre, duration, "
                       "trackNumber, year, trackType, size) VALUES (:id, :albumId, :name, :genre, "
                       ":duration, :trackNumber, :year, :trackType, :size)"));
    query.bindValue(":id", track.id);
    query.bindValue(":albumId", track.albumId);
    query.bindValue(":name", track.title);
    query.bindValue(":genre", track.genre);
    query.bindValue(":duration", track.durationMillis);
    query.bindValue(":trackNumber", track.trackNumber);
    query.bindValue(":year", track.year);
    query.bindValue(":trackType", track.trackType);
    query.bindValue(":size", track.estimatedSize);

    if (!query.exec()) {
        qWarning() << query.lastError();
        return false;
    }

    for (int i = 0; i < track.artistId.size(); ++i) {
        QSqlQuery artistQuery(db);
        artistQuery.prepare(QStringLiteral(
            "INSERT OR REPLACE INTO Track2Artist (trackId, artistId) VALUES(:trackId, :artistId)"));
        artistQuery.bindValue(":trackId", track.id);
        artistQuery.bindValue(":artistId", track.artistId[i]);
        if (!artistQuery.exec()) {
            qWarning() << artistQuery.lastError();
            return false;
        }
    }

    transaction.commit();
    return true;
}

bool Database::removeTrack_(QSqlDatabase &db, const QString &id)
{
    DBTransaction transaction(db);
    QSqlQuery query(db);

    query.prepare(QStringLiteral("DELETE FROM Track2Artist WHERE trackId = :trackId"));
    query.bindValue(":trackId", id);
    if (!query.exec()) {
        qWarning() << query.lastError();
        return false;
    }

    query.finish();
    query.prepare(QStringLiteral("DELETE FROM Track WHERE id = :trackId"));
    query.bindValue(":trackId", id);
    if (!query.exec()) {
        qWarning() << query.lastError();
        return false;
    }

    transaction.commit();

    return true;
}

std::optional<GMArtistList> Database::artists_(QSqlDatabase &db)
{
    QSqlQuery query(db);
    if (!query.exec(QStringLiteral("SELECT id, name, artUrl, bio FROM Artist"))) {
        qWarning() << query.lastError();
        return std::nullopt;
    }

    GMArtistList result;
    while (query.next()) {
        GMArtist artist;
        artist.artistId     = query.value(0).toString();
        artist.name         = query.value(1).toString();
        artist.artistArtRef = query.value(2).toString();
        artist.artistBio    = query.value(3).toString();
        result.push_back(artist);
    }

    return std::move(result);
}

std::optional<GMArtist> Database::artist_(QSqlDatabase &db, const QString &id)
{
    QSqlQuery query(db);
    query.prepare(QStringLiteral("SELECT id, name, artUrl, bio FROM Artist WHERE id = :id"));
    query.bindValue(":id", id);
    if (!query.exec()) {
        qWarning() << query.lastError();
        return std::nullopt;
    }

    if (!query.next()) {
        qWarning() << query.lastError();
        return std::nullopt;
    }

    GMArtist artist;
    artist.artistId     = query.value(0).toString();
    artist.name         = query.value(1).toString();
    artist.artistArtRef = query.value(2).toString();
    artist.artistBio    = query.value(3).toString();

    return std::move(artist);
}

bool Database::insertArtist_(QSqlDatabase &db, const GMArtist &artist)
{
    QSqlQuery query(db);
    query.prepare(QStringLiteral("INSERT OR REPLACE INTO Artist (id, name, artUrl, bio) VALUES "
                                 "(:id, :name, :artUrl, :bio)"));
    query.bindValue(":id", artist.artistId);
    query.bindValue(":name", artist.name);
    query.bindValue(":artUrl", artist.artistArtRef);
    query.bindValue(":bio", artist.artistBio);
    if (!query.exec()) {
        qWarning() << query.lastError();
        return false;
    }

    return true;
}

std::optional<GMAlbumList> Database::albums_(QSqlDatabase &db)
{
    QSqlQuery query(db);
    query.prepare(QStringLiteral("SELECT id, name, artUrl, descr, year FROM Album"));
    if (!query.exec()) {
        qWarning() << query.lastError();
        return std::nullopt;
    }

    GMAlbumList albums;
    while (query.next()) {
        GMAlbum album;

        album.albumId     = query.value(0).toString();
        album.name        = query.value(1).toString();
        album.albumArtRef = query.value(2).toString();
        album.description = query.value(3).toString();
        album.year        = query.value(4).toInt();

        QSqlQuery artistQuery(db);
        artistQuery.prepare(
            QStringLiteral("SELECT artistId FROM Artist2Album WHERE albumId = :albumId"));
        artistQuery.bindValue(":albumId", album.albumId);
        if (artistQuery.exec()) {
            qWarning() << artistQuery.lastError();
        }
        while (artistQuery.next()) {
            album.artistId.append(artistQuery.value(0).toString());
        }

        albums.push_back(album);
    }

    return std::move(albums);
}

std::optional<GMAlbumList> Database::albums_for_artist(QSqlDatabase &db, const QString &artistId)
{
    QSqlQuery query(db);
    query.prepare(
        QStringLiteral("SELECT a.id, a.name, a.artUrl, a.descr, a.year FROM Album a JOIN "
                       "Artist2Album b ON (a.id = b.albumId) and (b.artistId = :artistId)"));
    query.bindValue(":artistId", artistId);
    if (!query.exec()) {
        qWarning() << query.lastError();
        return std::nullopt;
    }

    GMAlbumList albums;
    while (query.next()) {
        GMAlbum album;

        album.albumId     = query.value(0).toString();
        album.name        = query.value(1).toString();
        album.albumArtRef = query.value(2).toString();
        album.description = query.value(3).toString();
        album.year        = query.value(4).toInt();

        QSqlQuery artistQuery(db);
        artistQuery.prepare(
            QStringLiteral("SELECT artistId FROM Artist2Album WHERE albumId = :albumId"));
        artistQuery.bindValue(":albumId", album.albumId);
        if (!artistQuery.exec()) {
            qWarning() << artistQuery.lastError();
        }
        while (artistQuery.next()) {
            album.artistId.append(artistQuery.value(0).toString());
        }

        albums.push_back(album);
    }

    return std::move(albums);
}

std::optional<GMAlbum> Database::album_(QSqlDatabase &db, const QString &id)
{
    QSqlQuery query(db);
    query.prepare(
        QStringLiteral("SELECT id, name, artUrl, descr, year FROM Album WHERE id = :albumId"));
    query.bindValue(":albumId", id);
    if (!query.exec()) {
        qWarning() << query.lastError();
        return std::nullopt;
    }

    GMAlbum album;
    if (query.next()) {

        album.albumId     = query.value(0).toString();
        album.name        = query.value(1).toString();
        album.albumArtRef = query.value(2).toString();
        album.description = query.value(3).toString();
        album.year        = query.value(4).toInt();

        QSqlQuery artistQuery(db);
        artistQuery.prepare(
            QStringLiteral("SELECT artistId FROM Artist2Album WHERE albumId = :albumId"));
        artistQuery.bindValue(":albumId", album.albumId);
        if (!artistQuery.exec()) {
            qWarning() << artistQuery.lastError();
        }
        while (artistQuery.next()) {
            album.artistId.append(artistQuery.value(0).toString());
        }
    }

    return std::move(album);
}

bool Database::insertAlbum_(QSqlDatabase &db, const GMAlbum &album)
{
    DBTransaction transaction(db);

    QSqlQuery query(db);
    query.prepare(
        QStringLiteral("INSERT OR REPLACE INTO Album (id, name, artUrl, descr, year) VALUES (:id, "
                       ":name, :artUrl, :descr, :year)"));
    query.bindValue(":id", album.albumId);
    query.bindValue(":name", album.name);
    query.bindValue(":artUrl", album.albumArtRef);
    query.bindValue(":descr", album.description);
    query.bindValue(":year", album.year);
    if (!query.exec()) {
        qWarning() << query.lastError();
        return false;
    }
    query.finish();

    for (int i = 0; i < album.artistId.size(); ++i) {
        query.prepare(QStringLiteral("INSERT OR REPLACE INTO Artist2Album (artistId, albumId) "
                                     "VALUES (:artistId, :albumId)"));
        query.bindValue(":artistId", album.artistId[i]);
        query.bindValue(":albumId", album.albumId);
        if (!query.exec()) {
            qWarning() << query.lastError();
        }
        query.finish();
    }

    transaction.commit();

    return true;
}

using namespace std::placeholders;

static std::mutex db_mutex_;

bool Database::createTables()
{
    return perform(db_mutex_, Database::createSchema_);
}

std::optional<GMTrackList> Database::tracks()
{
    return perform(db_mutex_, Database::tracks_);
}

Opt<GMTrackList> Database::tracksForAlbum(const QString &albumId)
{
    return perform(db_mutex_, std::bind(Database::tracks_for_album, _1, albumId));
}

Opt<GMTrackList> Database::tracksForArtist(const QString &artistId)
{
    return perform(db_mutex_, std::bind(Database::tracks_for_artist, _1, artistId));
}

std::optional<GMTrack> Database::track(const QString &id)
{
    return perform(db_mutex_, std::bind(Database::track_, _1, id));
}

bool Database::insertTrack(const GMTrack &track)
{
    return perform(db_mutex_, std::bind(Database::insertTrack_, _1, track));
}

bool Database::removeTrack(const QString &id)
{
    return perform(db_mutex_, std::bind(Database::removeTrack_, _1, id));
}

std::optional<GMArtistList> Database::artists()
{
    return perform(db_mutex_, Database::artists_);
}

std::optional<GMArtist> Database::artist(const QString &id)
{
    return perform(db_mutex_, std::bind(Database::artist_, _1, id));
}

bool Database::insertArtist(const GMArtist &artist)
{
    return perform(db_mutex_, std::bind(Database::insertArtist_, _1, artist));
}

std::optional<GMAlbumList> Database::albums()
{
    return perform(db_mutex_, Database::albums_);
}

std::optional<GMAlbumList> Database::albumsForArtist(const QString &artistId)
{
    return perform(db_mutex_, std::bind(Database::albums_for_artist, _1, artistId));
}

std::optional<GMAlbum> Database::album(const QString &id)
{
    return perform(db_mutex_, std::bind(Database::album_, _1, id));
}

bool Database::insertAlbum(const GMAlbum &album)
{
    return perform(db_mutex_, std::bind(Database::insertAlbum_, _1, album));
}
