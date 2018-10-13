#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QString>
#include <functional>
#include <future>
#include <type_traits>

#include "model.h"
#include "proxyresult.h"

class DBTransaction
{
public:
    DBTransaction(QSqlDatabase &db) : shouldRollback_(true), db_(db)
    {
        db_.transaction();
    }

    ~DBTransaction()
    {
        if (shouldRollback_) {
            db_.rollback();
        }
    }

    void commit()
    {
        shouldRollback_ = false;
        db_.commit();
    }

private:
    bool shouldRollback_;
    QSqlDatabase &db_;
};

template <class T> using Opt = std::optional<T>;

class Database : public QObject
{
    Q_OBJECT

public:
    Database(QObject *parent = nullptr);
    ~Database();

    bool openConnection(const QString &path, QString connName = QString());

    Opt<GMTrackList> tracks();
    Opt<GMTrackList> tracksForAlbum(const QString &albumId);
    Opt<GMTrackList> tracksForArtist(const QString &artistId);
    Opt<GMTrack> track(const QString &id);
    bool insertTrack(const GMTrack &track);
    bool removeTrack(const QString &id);
    Opt<GMArtistList> artists();
    Opt<GMArtist> artist(const QString &id);
    bool insertArtist(const GMArtist &artist);
    bool insertAlbum(const GMAlbum &album);
    Opt<GMAlbumList> albums();
    Opt<GMAlbumList> albumsForArtist(const QString &artistId);
    Opt<GMAlbum> album(const QString &id);

    bool createTables();

private:
    static bool createSchema_(QSqlDatabase &db);
    static Opt<GMTrackList> tracks_(QSqlDatabase &db);
    static Opt<GMTrackList> tracks_for_album(QSqlDatabase &db, const QString &albumId);
    static Opt<GMTrackList> tracks_for_artist(QSqlDatabase &db, const QString &artistId);
    static Opt<GMTrack> track_(QSqlDatabase &db, const QString &id);
    static bool insertTrack_(QSqlDatabase &db, const GMTrack &track);
    static bool removeTrack_(QSqlDatabase &db, const QString &id);

    static Opt<GMArtistList> artists_(QSqlDatabase &db);
    static Opt<GMArtist> artist_(QSqlDatabase &db, const QString &id);
    static bool insertArtist_(QSqlDatabase &db, const GMArtist &artist);

    static Opt<GMAlbumList> albums_(QSqlDatabase &db);
    static Opt<GMAlbumList> albums_for_artist(QSqlDatabase &db, const QString &artistId);
    static Opt<GMAlbum> album_(QSqlDatabase &db, const QString &id);
    static bool insertAlbum_(QSqlDatabase &db, const GMAlbum &album);

    static Opt<GMTrackList> extractTracks(QSqlDatabase &db, QSqlQuery &query);

    template <class Action> auto perform(std::mutex &mutex, Action &&action)
    {
        std::lock_guard<std::mutex> guard(mutex);
        return action(db_);
    }

    QSqlDatabase db_;
};

#endif // DATABASE_H
