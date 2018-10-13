#include "imagestorage.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>
#include <memory>

#include "proxyresult.h"

#define MS_THRESHOLD 24 * 3600 * 1000
#define TIMER_POOL_SIZE 20

ImageStorage &ImageStorage::instance()
{
    static ImageStorage storage;
    return storage;
}

ImageStorage::ImageStorage(QObject *parent) : QObject(parent), failureCounter_(0)
{
    manager_ = new QNetworkAccessManager(this);

    QDir dataDir;
    QString dataPath = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).at(0);
    if (!dataDir.exists(dataPath) && !dataDir.mkdir(dataPath)) {
        qDebug() << __PRETTY_FUNCTION__ << ": could not create data dir";
    }
    dataDir.cd(dataPath);

    QDir cacheDir;
    QString cachePath = QStandardPaths::standardLocations(QStandardPaths::CacheLocation).at(0);
    if (!cacheDir.exists(cachePath) && !cacheDir.mkdir(cachePath)) {
        qDebug() << __PRETTY_FUNCTION__ << ": could not create cache dir";
    }
    cacheDir.cd(cachePath);
    if (!cacheDir.mkdir("images")) {
        qDebug() << __PRETTY_FUNCTION__ << ": could not create image cache dir";
    }
    cacheDir.cd("images");
    imageCacheDirPath_ = cacheDir.absolutePath();

    db_ = QSqlDatabase::addDatabase("QSQLITE", "IMAGE_STORAGE");
    db_.setDatabaseName(dataDir.filePath("image_storage.sqlite"));
    if (!db_.open()) {
        qDebug() << __PRETTY_FUNCTION__ << ": could not open database:" << db_.lastError();
    }
    createDatabaseSchema();
}

ImageStorage::~ImageStorage()
{
}

void ImageStorage::createDatabaseSchema()
{
    QSqlQuery query(db_);
    query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS ImageCacheMetadata(url TEXT "
                              "PRIMARY KEY, localPath TEXT NOT NULL, timestamp INTEGER NOT NULL)"));
}

QByteArray ImageStorage::tryGetImage(const QString &url)
{
    QSqlQuery query(db_);
    query.prepare(QStringLiteral("SELECT localPath FROM ImageCacheMetadata WHERE url = :url"));
    query.bindValue(":url", url);
    query.exec();
    if (query.next()) {
        QString filePath = query.value(0).toString();
        QFile imageFile(filePath);
        if (!imageFile.open(QFile::ReadOnly)) {
            removeCacheEntry(filePath, url);
            scheduleNextDownload(url);
            return QByteArray();
        }
        return imageFile.readAll();
    }

    scheduleNextDownload(url);
    return QByteArray();
}

void ImageStorage::scheduleNextDownload(const QString &url)
{
    if (activeDownloads_.contains(url)) {
        return;
    }

    qint64 interval = 0;
    if (failureCounter_ > 100) {
        interval = 3600;
    } else if (failureCounter_ > 75) {
        interval = 1800;
    } else if (failureCounter_ > 50) {
        interval = 1200;
    } else if (failureCounter_ > 25) {
        interval = 600;
    }
    activeDownloads_.insert(url);
    QTimer::singleShot(interval * 1000, this, [=] { downloadImage(url); });
}

void ImageStorage::downloadImage(const QString &url)
{
    QString filename = QCryptographicHash::hash(url.toUtf8(), QCryptographicHash::Md5).toHex();
    QString filepath = QDir(imageCacheDirPath_).filePath(filename);
    auto imageFile   = std::make_shared<QFile>(filepath);
    if (!imageFile->open(QFile::WriteOnly | QFile::Unbuffered)) {
        qDebug() << __PRETTY_FUNCTION__ << ": could not open image file";
        return;
    }
    QNetworkReply *reply = manager_->get(QNetworkRequest(QUrl{url}));
    connect(reply, &QNetworkReply::readyRead, this, [this, imageFile] {
        auto reply = qobject_cast<QNetworkReply *>(sender());
        imageFile->write(reply->readAll());
        imageFile->flush();
    });
    connect(reply, &QNetworkReply::finished, this, [this, filepath, url] {
        activeDownloads_.remove(url);
        auto reply = qobject_cast<QNetworkReply *>(sender());
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "downloadImage error:" << reply->errorString();
            QFile::remove(filepath);
            ++failureCounter_;
            return;
        }
        failureCounter_ = 0;
        insertCacheEntry(url, filepath);
        QTimer::singleShot(1000, this, [this, url] { emit imageUpdated(url); });
    });
}

void ImageStorage::insertCacheEntry(const QString &url, const QString &filepath)
{
    QSqlQuery query(db_);
    query.prepare(
        QStringLiteral("INSERT OR REPLACE INTO ImageCacheMetadata(url, localPath, timestamp) "
                       "VALUES(:url, :localPath, :timestamp)"));
    query.bindValue(":url", url);
    query.bindValue(":localPath", filepath);
    query.bindValue(":timestamp", QDateTime::currentMSecsSinceEpoch());
    if (!query.exec()) {
        qDebug() << __PRETTY_FUNCTION__ << ": error:" << query.lastError();
    }
}

void ImageStorage::removeCacheEntry(const QString &path, const QString &url)
{
    if (QFile::exists(path)) {
        QFile::remove(path);
    }
    QSqlQuery query(db_);
    query.prepare(QStringLiteral("DELETE FROM ImageCacheMetadata WHERE url = :url"));
    query.bindValue(":url", url);
    if (!query.exec()) {
        qDebug() << __PRETTY_FUNCTION__ << ":" << query.lastError();
    }
}
