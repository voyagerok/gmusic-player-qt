#include "gmapi.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QtCore/QStringBuilder>

#include "utils.h"

static const char *content_type = "application/x-www-form-urlencoded";

static const QString base_url = QStringLiteral("https://mclients.googleapis.com/sj/v2.5/");

static bool get_string_value(QNetworkReply *reply, const QString &key, QString &value)
{
    while (true) {
        QByteArray rawNextLine = reply->readLine();
        if (rawNextLine.isEmpty()) {
            break;
        }
        QString line   = QString::fromLatin1(rawNextLine.data());
        int splitIndex = line.indexOf('=');
        if (line.leftRef(splitIndex) == key) {
            value = line.section('=', 1).trimmed();
            return true;
        }
    }
    return false;
}

ProxyResult *GMApi::masterToken(const QString &email, const QString &deviceId,
                                const QString &passwd)
{
    QString encPasswd     = Utils::EncryptPasswd(email, passwd);
    QString localeCountry = systemLocale_.countryToString(systemLocale_.country());
    QString localeLang    = systemLocale_.languageToString(systemLocale_.language());

    QUrlQuery post_data;
    post_data.addQueryItem("accountType", "HOSTED_OR_GOOGLE");
    post_data.addQueryItem("has_permission", "1");
    post_data.addQueryItem("add_account", "1");
    post_data.addQueryItem("service", "ac2dm");
    post_data.addQueryItem("source", "android");
    post_data.addQueryItem("device_country", localeCountry);
    post_data.addQueryItem("operatorCountry", localeCountry);
    post_data.addQueryItem("lang", localeLang);
    post_data.addQueryItem("sdk_version", "17");
    post_data.addQueryItem("Email", email);
    post_data.addQueryItem("EncryptedPasswd", encPasswd);
    post_data.addQueryItem("androidId", deviceId);

    QNetworkRequest req(QUrl("https://android.clients.google.com/auth"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, content_type);
    QNetworkReply *reply = accessManager_->post(req, post_data.toString().toUtf8());

    auto proxyResult = new ProxyResult(this);
    connect(reply, &QNetworkReply::finished, this, [=]() mutable {
        auto reply = qobject_cast<QNetworkReply *>(sender());
        reply->deleteLater();
        auto error = reply->error();
        if (error != QNetworkReply::NoError) {
            auto errMessage =
                QStringLiteral("Network error: %1").arg(reply->errorString()).toLocal8Bit();
            emit proxyResult->ready(ProxyResult::Error, errMessage);
            return;
        }
        QString master_token;
        if (!get_string_value(reply, QStringLiteral("Token"), master_token)) {
            emit proxyResult->ready(ProxyResult::Error, "could not extract token field from reply");
            return;
        }
        emit proxyResult->ready(ProxyResult::OK, master_token);
    });

    return proxyResult;
}

ProxyResult *GMApi::authToken(const QString &email, const QString &deviceId,
                              const QString &masterToken)
{
    QString localeCountry = systemLocale_.countryToString(systemLocale_.country());
    QString localeLang    = systemLocale_.languageToString(systemLocale_.language());

    QUrlQuery post_data;
    post_data.addQueryItem("accountType", "HOSTED_OR_GOOGLE");
    post_data.addQueryItem("has_permission", "1");
    post_data.addQueryItem("service", "sj");
    post_data.addQueryItem("source", "android");
    post_data.addQueryItem("app", "com.google.android.music");
    post_data.addQueryItem("client_sig", "38918a453d07199354f8b19af05ec6562ced5788");
    post_data.addQueryItem("device_country", localeCountry);
    post_data.addQueryItem("operatorCountry", localeCountry);
    post_data.addQueryItem("lang", localeLang);
    post_data.addQueryItem("sdk_version", "17");
    post_data.addQueryItem("Email", email);
    post_data.addQueryItem("EncryptedPasswd", masterToken);
    post_data.addQueryItem("androidId", deviceId);

    QNetworkRequest req(QUrl("https://android.clients.google.com/auth"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, content_type);

    QNetworkReply *reply = accessManager_->post(req, post_data.toString().toUtf8());

    auto proxyResult = new ProxyResult(this);
    connect(reply, &QNetworkReply::finished, this, [=]() mutable {
        auto reply = qobject_cast<QNetworkReply *>(sender());
        reply->deleteLater();
        auto error = reply->error();
        if (error != QNetworkReply::NoError) {
            auto errMessage =
                QStringLiteral("network error: %1").arg(reply->errorString()).toLocal8Bit();
            emit proxyResult->ready(ProxyResult::Error, errMessage);
            return;
        }
        QString auth_token;
        if (!get_string_value(reply, QString::fromLatin1("Auth"), auth_token)) {
            emit proxyResult->ready(ProxyResult::Error, "failed to extract auth field from reply");
            return;
        }
        emit proxyResult->ready(ProxyResult::OK, auth_token);
    });

    return proxyResult;
}

static std::optional<GMTrackList> parse_tracks(const QByteArray &rawData, QString &nextPageToken)
{
    QJsonParseError error;
    auto doc = QJsonDocument::fromJson(rawData, &error);
    if (!doc.isObject()) {
        qWarning() << error.errorString();
        return std::nullopt;
    }

    QJsonObject root = doc.object();
    if (!root.contains("data") || !root["data"].isObject()) {
        return std::nullopt;
    }
    QJsonObject data = root.value("data").toObject();
    if (!data.contains("items") || !data["items"].isArray()) {
        return std::nullopt;
    }
    nextPageToken = root.value("nextPageToken").toString();

    QJsonArray items = data["items"].toArray();

    GMTrackList tracks;
    tracks.reserve(items.size());

    for (int i = 0; i < items.size(); ++i) {
        QJsonValue item = items[i];
        if (auto track = GMTrack::fromJson(item.toObject())) {
            tracks.append(*track);
        }
    }
    return std::move(tracks);
}

static std::optional<GMDeviceList> parse_devices(const QByteArray &rawData)
{
    QJsonParseError error;
    auto doc = QJsonDocument::fromJson(rawData, &error);
    if (!doc.isObject()) {
        qWarning() << error.errorString();
        return std::nullopt;
    }

    QJsonObject root = doc.object();
    if (!root.contains("data") || !root["data"].isObject()) {
        return std::nullopt;
    }
    QJsonObject data = root.value("data").toObject();
    if (!data.contains("items") || !data["items"].isArray()) {
        return std::nullopt;
    }
    QJsonArray items = data["items"].toArray();

    GMDeviceList devices;
    devices.reserve(items.size());

    for (auto item : items) {
        if (auto device = GMDevice::fromJson(item.toObject())) {
            devices.append(*device);
        }
    }

    return std::move(devices);
}

static std::optional<GMAlbum> parse_album(const QByteArray &rawData)
{
    QJsonParseError error;
    auto doc = QJsonDocument::fromJson(rawData, &error);
    if (!doc.isObject()) {
        qWarning() << error.errorString();
        return std::nullopt;
    }

    return GMAlbum::fromJson(doc.object());
}

static std::optional<GMArtist> parse_artist(const QByteArray &rawData)
{
    QJsonParseError error;
    auto doc = QJsonDocument::fromJson(rawData, &error);
    if (!doc.isObject()) {
        qWarning() << error.errorString();
        return std::nullopt;
    }

    return GMArtist::fromJson(doc.object());
}

ProxyResult *GMApi::tracks(const QString &token)
{
    static QUrl target_url(base_url % QStringLiteral("trackfeed"));

    QUrlQuery query;
    query.addQueryItem("dv", "0");
    query.addQueryItem("hl", systemLocale_.name());
    query.addQueryItem("tier", "fr");
    target_url.setQuery(query);

    QNetworkRequest req(target_url);
    req.setHeader(QNetworkRequest::KnownHeaders::UserAgentHeader, "gmusic-cpp/1.0");
    req.setRawHeader("Authorization", QString("GoogleLogin auth=%1").arg(token).toLatin1());
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonDocument jsonDoc;
    QJsonObject rootObj;
    if (!tracksNextPageToken_.isEmpty()) {
        rootObj.insert("start-token", tracksNextPageToken_);
    }
    jsonDoc.setObject(rootObj);

    QNetworkReply *reply = accessManager_->post(req, jsonDoc.toJson());

    auto proxyResult = new ProxyResult(this);
    connect(reply, &QNetworkReply::finished, this, [=]() mutable {
        auto reply = qobject_cast<QNetworkReply *>(sender());
        reply->deleteLater();
        auto error = reply->error();
        if (error != QNetworkReply::NoError) {
            auto errMessage =
                QStringLiteral("network error: %1").arg(reply->errorString()).toLocal8Bit();
            emit proxyResult->ready(ProxyResult::Error, errMessage);
            return;
        }
        QString nextPageToken;
        if (auto tracklist = parse_tracks(reply->readAll(), nextPageToken)) {
            if (nextPageToken.isEmpty() || nextPageToken == tracksNextPageToken_) {
                tracksNextPageToken_.clear();
            } else {
                tracksNextPageToken_ = nextPageToken;
            }
            emit proxyResult->ready(ProxyResult::OK, QVariant::fromValue(*tracklist));
        } else {
            emit proxyResult->ready(ProxyResult::Error, "could not parse payload with tracks");
        }
    });

    return proxyResult;
}

bool GMApi::hasMoreTracks() const
{
    return !tracksNextPageToken_.isEmpty();
}

ProxyResult *GMApi::devices(const QString &token)
{
    static QUrl target_url(base_url % QStringLiteral("devicemanagementinfo"));

    QUrlQuery query;
    query.addQueryItem("dv", "0");
    query.addQueryItem("hl", systemLocale_.name());
    query.addQueryItem("tier", "fr");
    target_url.setQuery(query);

    QNetworkRequest req(target_url);
    req.setHeader(QNetworkRequest::KnownHeaders::UserAgentHeader, "gmusic-cpp/1.0");
    req.setRawHeader("Authorization", QString("GoogleLogin auth=%1").arg(token).toLatin1());
    req.setHeader(QNetworkRequest::ContentTypeHeader, content_type);

    auto reply = accessManager_->get(req);

    auto proxyResult = new ProxyResult(this);
    connect(reply, &QNetworkReply::finished, this, [=]() mutable {
        auto reply = qobject_cast<QNetworkReply *>(sender());
        reply->deleteLater();
        auto error = reply->error();
        if (error != QNetworkReply::NoError) {
            auto errMessage =
                QStringLiteral("network error: %1").arg(reply->errorString()).toLocal8Bit();
            emit proxyResult->ready(ProxyResult::Error, errMessage);
            return;
        }
        if (auto devicelist = parse_devices(reply->readAll())) {
            emit proxyResult->ready(ProxyResult::OK, QVariant::fromValue(*devicelist));
        } else {
            emit proxyResult->ready(ProxyResult::Error, "could not parse device list payload");
        }
    });

    return proxyResult;
}

ProxyResult *GMApi::album(const QString &token, const QString &albumId)
{
    static QUrl target_url(base_url % QStringLiteral("fetchalbum"));

    QUrlQuery query;
    query.addQueryItem("dv", "0");
    query.addQueryItem("hl", systemLocale_.name());
    query.addQueryItem("tier", "fr");
    query.addQueryItem("alt", "json");
    query.addQueryItem("nid", albumId);
    query.addQueryItem("include-tracks", "false");

    target_url.setQuery(query);

    QNetworkRequest req(target_url);
    req.setHeader(QNetworkRequest::KnownHeaders::UserAgentHeader, "gmusic-cpp/1.0");
    req.setRawHeader("Authorization", QString("GoogleLogin auth=%1").arg(token).toLatin1());
    req.setHeader(QNetworkRequest::ContentTypeHeader, content_type);

    auto reply = accessManager_->get(req);

    auto proxyResult = new ProxyResult(this);
    connect(reply, &QNetworkReply::finished, this, [=]() mutable {
        auto reply = qobject_cast<QNetworkReply *>(sender());
        reply->deleteLater();
        auto error = reply->error();
        if (error != QNetworkReply::NoError) {
            auto errMessage =
                QStringLiteral("network error: %1").arg(reply->errorString()).toLocal8Bit();
            emit proxyResult->ready(ProxyResult::Error, errMessage);
            return;
        }
        if (auto album = parse_album(reply->readAll())) {
            emit proxyResult->ready(ProxyResult::OK, QVariant::fromValue(*album));
        } else {
            emit proxyResult->ready(ProxyResult::Error, "could not parse album payload");
        }
    });

    return proxyResult;
}

ProxyResult *GMApi::artist(const QString &token, const QString &artistId)
{
    static QUrl target_url(base_url % QStringLiteral("fetchartist"));

    QUrlQuery query;
    query.addQueryItem("dv", "0");
    query.addQueryItem("hl", systemLocale_.name());
    query.addQueryItem("tier", "fr");
    query.addQueryItem("nid", artistId);
    query.addQueryItem("include-albums", "true");

    target_url.setQuery(query);

    QNetworkRequest req(target_url);
    req.setHeader(QNetworkRequest::KnownHeaders::UserAgentHeader, "gmusic-cpp/1.0");
    req.setRawHeader("Authorization", QString("GoogleLogin auth=%1").arg(token).toLatin1());
    req.setHeader(QNetworkRequest::ContentTypeHeader, content_type);

    auto reply = accessManager_->get(req);

    auto proxyResult = new ProxyResult(this);
    connect(reply, &QNetworkReply::finished, this, [=]() mutable {
        auto reply = qobject_cast<QNetworkReply *>(sender());
        reply->deleteLater();
        auto error = reply->error();
        if (error != QNetworkReply::NoError) {
            auto errMessage =
                QStringLiteral("network error: %1").arg(reply->errorString()).toLocal8Bit();
            emit proxyResult->ready(ProxyResult::Error, errMessage);
            return;
        }
        if (auto artist = parse_artist(reply->readAll())) {
            emit proxyResult->ready(ProxyResult::OK, QVariant::fromValue(*artist));
        } else {
            emit proxyResult->ready(ProxyResult::Error, "could not parse artist payload");
        }
    });

    return proxyResult;
}

GMApi::GMApi(QObject *parent) : QObject(parent), systemLocale_(QLocale::system())
{
    qRegisterMetaType<GMTrackList>();
    qRegisterMetaType<GMDeviceList>();
    qRegisterMetaType<GMTrack>();
    qRegisterMetaType<GMDevice>();
    qRegisterMetaType<GMArtist>();
    qRegisterMetaType<GMAlbum>();

    accessManager_ = new QNetworkAccessManager(this);
}

ProxyResult *GMApi::streamUrl(const QString &token, const QString &trackId, const QString &deviceId)
{
    static QUrl target_url = QStringLiteral("https://mclients.googleapis.com/music/mplay");

    auto [hash, salt] = Utils::EncryptTrackId(trackId);

    QUrlQuery query;
    query.addQueryItem("opt", "hi");
    query.addQueryItem("net", "mob");
    query.addQueryItem("pt", "e");
    query.addQueryItem("sig", hash);
    query.addQueryItem("slt", salt);
    query.addQueryItem("songid", trackId);

    QUrl url(target_url);
    url.setQuery(query);

    auto req = QNetworkRequest(url);
    req.setRawHeader("Authorization", QString("GoogleLogin auth=%1").arg(token).toLatin1());
    req.setRawHeader("X-Device-ID", deviceId.toLatin1());

    auto proxyResult = new ProxyResult(this);
    auto reply       = accessManager_->get(req);
    connect(reply, &QNetworkReply::finished, this, [=] {
        auto reply = qobject_cast<QNetworkReply *>(sender());
        reply->deleteLater();
        auto error = reply->error();
        if (error != QNetworkReply::NoError) {
            auto errMessage =
                QStringLiteral("Failed to get stream url: %1").arg(reply->errorString());
            emit proxyResult->ready(ProxyResult::Error, errMessage);
            return;
        }
        auto location = reply->header(QNetworkRequest::LocationHeader);
        if (!location.isValid()) {
            auto errMessage =
                QStringLiteral("Could not find location header in stream url repsonse");
            emit proxyResult->ready(ProxyResult::Error, errMessage);
            return;
        }
        emit proxyResult->ready(ProxyResult::OK, location);
    });

    return proxyResult;
}
