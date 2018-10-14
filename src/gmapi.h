#ifndef GMAPI_H
#define GMAPI_H

#include <QLocale>
#include <QNetworkAccessManager>
#include <QObject>
#include <functional>
#include <future>

#include "model.h"
#include "proxyresult.h"

class GMApi : public QObject
{
    Q_OBJECT

public:
    using Error = std::runtime_error;

    explicit GMApi(QObject *parent = nullptr);

    ProxyResult *masterToken(const QString &email, const QString &deviceId, const QString &passwd);

    ProxyResult *authToken(const QString &email, const QString &deviceId,
                           const QString &masterToken);

    ProxyResult *tracks(const QString &authToken);
    ProxyResult *devices(const QString &authToken);
    ProxyResult *artist(const QString &token, const QString &artistId);
    ProxyResult *album(const QString &token, const QString &albumId);
    ProxyResult *streamUrl(const QString &token, const QString &trackId, const QString &deviceId);

    bool hasMoreTracks() const;

private:
    QNetworkAccessManager *accessManager_;
    QString tracksNextPageToken_;
    QLocale systemLocale_;
};

Q_DECLARE_METATYPE(GMTrackList)
Q_DECLARE_METATYPE(GMDeviceList)

#endif // GMAPI_H
