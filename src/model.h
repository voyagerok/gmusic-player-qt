#ifndef MODEL_H
#define MODEL_H

#include <QJsonArray>
#include <QObject>
#include <QString>
#include <QStringList>
#include <optional>

struct GMTrack {

    QString duration_string() const;
    QString title;
    QString albumId;
    QStringList artistId;
    QString id;
    QString genre;
    QString trackType;
    qlonglong durationMillis;
    int trackNumber;
    int year;
    qlonglong estimatedSize;

    static std::optional<GMTrack> fromJson(const QJsonObject &json);
};

struct GMAlbum {
    QString albumId;
    QString name;
    QString albumArtRef;
    int year;
    QString description;
    QStringList artistId;

    static std::optional<GMAlbum> fromJson(const QJsonObject &json);
};

struct GMArtist {

    QString artistId;
    QString name;
    QString artistArtRef;
    QString artistBio;

    static std::optional<GMArtist> fromJson(const QJsonObject &json);
};

struct GMDevice {
    QString id;
    QString friendlyName;
    QString type;
    qlonglong lastAccessedTimeMs;

    static std::optional<GMDevice> fromJson(const QJsonObject &json);
};

using GMTrackList  = QList<GMTrack>;
using GMArtistList = QList<GMArtist>;
using GMAlbumList  = QList<GMAlbum>;
using GMDeviceList = QList<GMDevice>;

Q_DECLARE_METATYPE(GMTrack)
Q_DECLARE_METATYPE(GMAlbum)
Q_DECLARE_METATYPE(GMArtist)
Q_DECLARE_METATYPE(GMDevice)

#endif // MODEL_H
