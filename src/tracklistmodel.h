#ifndef TRACKLISTMODEL_H
#define TRACKLISTMODEL_H

#include <QAbstractTableModel>
#include <QSet>
#include <QSortFilterProxyModel>

#include "database.h"
#include "model.h"

class ImageStorage;
class QTimer;

class TrackListModel : public QAbstractTableModel
{
    Q_OBJECT

    Q_PROPERTY(QString databasePath WRITE setDatabasePath)

public:
    using Loader = std::function<Opt<GMTrackList>(Database &)>;

    TrackListModel(QObject *parent = nullptr);

    enum Roles { Number = Qt::UserRole + 1, Title, Album, Artist, Duration, TrackId };

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void reloadTracks();
    void setLoaderFunc(const Loader &loader);

    QModelIndex getIndexForId(const QString &id);
    QModelIndex getNext(const QString &id);
    QModelIndex getPrev(const QString &id);

public slots:
    void setDatabasePath(const QString &dbPath);

private slots:
    void reloadDecoationData();

private:
    inline QVariant albumString(const GMTrack &track) const;
    inline QVariant artistString(const GMTrack &track) const;

    static Opt<GMTrackList> loadAllTracks(Database &);

    GMTrackList tracks_;
    mutable Database db_;
    Loader loader_;
    QTimer *deferredUpdateTimer_;
    ImageStorage &imageStorage_;

    QPixmap getPixmap(const QString &url) const;
};

class TrackListSortingModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    TrackListSortingModel(QObject *parent = Q_NULLPTR);

protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};

#endif // TRACKLISTMODEL_H
