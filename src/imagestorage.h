#ifndef IMAGESTORAGE_H
#define IMAGESTORAGE_H

#include <QObject>
#include <QSet>
#include <QSqlDatabase>
#include <QTimer>
#include <QVector>

class ProxyResult;
class QNetworkAccessManager;

class ImageStorage : public QObject
{
    Q_OBJECT
public:
    static ImageStorage &instance();
    ImageStorage(const ImageStorage &) = delete;
    ImageStorage &operator=(const ImageStorage &) = delete;
    ~ImageStorage();

    QByteArray tryGetImage(const QString &url);

signals:
    void imageUpdated(const QString &url);

private:
    explicit ImageStorage(QObject *parent = nullptr);

    QSqlDatabase db_;
    QNetworkAccessManager *manager_;
    QString imageCacheDirPath_;
    QSet<QString> activeDownloads_;
    qint64 failureCounter_;
    bool initialized_;

    bool createDatabaseSchema();
    void insertCacheEntry(const QString &url, const QString &filepath);
    void removeCacheEntry(const QString &filePath, const QString &url);
    void downloadImage(const QString &url);
    void scheduleNextDownload(const QString &url);
};

#endif // IMAGESTORAGE_H
