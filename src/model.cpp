#include "model.h"
#include <QDebug>
#include <QJsonObject>

#include "utils.h"

QString GMTrack::duration_string() const
{
    return Utils::TimeFormat(durationMillis / 1000);
}

std::optional<GMTrack> GMTrack::fromJson(const QJsonObject &json)
{
    GMTrack track;

    if (!json.contains(QStringLiteral("id"))) {
        qDebug() << __PRETTY_FUNCTION__ << ": could not find track id";
        return std::nullopt;
    }

    track.id             = json.value(QStringLiteral("id")).toString();
    track.title          = json.value(QStringLiteral("title")).toString();
    track.albumId        = json.value(QStringLiteral("albumId")).toString();
    track.genre          = json.value(QStringLiteral("genre")).toString();
    track.trackType      = json.value(QStringLiteral("trackType")).toString(QStringLiteral("8"));
    track.durationMillis = json.value(QStringLiteral("durationMillis")).toString().toLongLong();
    track.estimatedSize  = json.value(QStringLiteral("estimatedSize")).toString().toLongLong();
    track.trackNumber    = json.value(QStringLiteral("trackNumber")).toInt();
    track.year           = json.value(QStringLiteral("year")).toInt();

    QJsonArray artists = json.value(QStringLiteral("artistId")).toArray();
    for (int i = 0; i < artists.size(); ++i) {
        track.artistId.append(artists.at(i).toString());
    }

    return track;
}

std::optional<GMAlbum> GMAlbum::fromJson(const QJsonObject &json)
{
    GMAlbum album;

    if (!json.contains(QStringLiteral("albumId"))) {
        qDebug() << __PRETTY_FUNCTION__ << ": could not find album id";
        return std::nullopt;
    }

    album.albumId     = json.value(QStringLiteral("albumId")).toString();
    album.name        = json.value(QStringLiteral("name")).toString();
    album.albumArtRef = json.value(QStringLiteral("albumArtRef")).toString();
    album.description = json.value(QStringLiteral("description")).toString();
    album.year        = json.value(QStringLiteral("year")).toInt();

    QJsonArray artists = json.value(QStringLiteral("artistId")).toArray();
    for (int i = 0; i < artists.size(); ++i) {
        album.artistId.append(artists.at(i).toString());
    }

    return album;
}

std::optional<GMArtist> GMArtist::fromJson(const QJsonObject &json)
{
    GMArtist artist;

    if (!json.contains(QStringLiteral("artistId"))) {
        qDebug() << __PRETTY_FUNCTION__ << ": could not find artist id";
        return std::nullopt;
    }

    artist.artistId  = json.value(QStringLiteral("artistId")).toString();
    artist.name      = json.value(QStringLiteral("name")).toString();
    artist.artistBio = json.value(QStringLiteral("artistBio")).toString();

    bool found         = false;
    QJsonArray artRefs = json.value(QStringLiteral("artistArtRefs")).toArray();
    for (const auto artref : artRefs) {
        QJsonObject artrefObj = artref.toObject();
        QString aspectRatio   = artrefObj.value(QStringLiteral("aspectRatio")).toString();
        if (aspectRatio == QLatin1String("1")) {
            artist.artistArtRef = artrefObj.value(QStringLiteral("url")).toString();
            found               = true;
        }
    }
    if (artist.artistArtRef.isEmpty()) {
        artist.artistArtRef = json.value(QStringLiteral("artistArtRef")).toString();
    }

    return artist;
}

std::optional<GMDevice> GMDevice::fromJson(const QJsonObject &json)
{
    GMDevice device;

    if (!json.contains(QStringLiteral("id"))) {
        qDebug() << __PRETTY_FUNCTION__ << ": could not find device id";
        return std::nullopt;
    }

    device.id           = json.value(QStringLiteral("id")).toString();
    device.friendlyName = json.value(QStringLiteral("friendlyName")).toString();
    device.type         = json.value(QStringLiteral("type")).toString();
    device.lastAccessedTimeMs =
        json.value(QStringLiteral("lastAccessedTimeMs")).toString().toLongLong();

    return device;
}
