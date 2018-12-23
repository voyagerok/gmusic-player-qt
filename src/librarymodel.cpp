#include "librarymodel.h"

#include <QDebug>
#include <QPixmap>
#include <QPixmapCache>
#include <QTimer>

#include "database.h"
#include "imagestorage.h"
#include "model.h"

LibraryModelNode::~LibraryModelNode()
{
    qDeleteAll(children);
}

void RootLibraryNode::load_children(Database *db)
{
    if (!children.empty()) {
        qDeleteAll(children);
        children.clear();
    }
    if (auto artists = db->artists()) {
        for (int i = 0; i < artists->size(); ++i) {
            children.push_back(
                new ArtistLibraryNode(0, i, QVariant::fromValue(artists->at(i)), this));
        }
    } else {
        qWarning() << "RootLibraryNode: failed to read artists from database";
    }
}

void ArtistLibraryNode::load_children(Database *db)
{
    if (!children.empty()) {
        qDeleteAll(children);
        children.clear();
    }
    auto artist = data.value<GMArtist>();

    if (auto albums = db->albumsForArtist(artist.artistId)) {
        for (int i = 0; i < albums->size(); ++i) {
            children.push_back(
                new AlbumLibraryNode(1, i, QVariant::fromValue(albums->at(i)), this));
        }
    } else {
        qWarning() << "ArtistLibraryNode: failed to load albums for artist with id: "
                   << artist.artistId << ", name: " << artist.name;
    }
}

TracksLoader ArtistLibraryNode::tracksLoader() const
{
    auto artistId = data.value<GMArtist>().artistId;
    return [artistId](Database &db) { return db.tracksForArtist(artistId); };
}

QVariant ArtistLibraryNode::presentation_value() const
{
    return data.value<GMArtist>().name;
}

QString ArtistLibraryNode::imageUrl() const
{
    return data.value<GMArtist>().artistArtRef;
}

TracksLoader AlbumLibraryNode::tracksLoader() const
{
    auto albumId = data.value<GMAlbum>().albumId;
    return [albumId](Database &db) { return db.tracksForAlbum(albumId); };
}

QVariant AlbumLibraryNode::presentation_value() const
{
    return data.value<GMAlbum>().name;
}

QString AlbumLibraryNode::imageUrl() const
{
    return data.value<GMAlbum>().albumArtRef;
}

LibraryModel::LibraryModel(QObject *parent)
    : QAbstractItemModel(parent), imageStorage_(ImageStorage::instance())
{
    deferredUpdateTimer_ = new QTimer(this);
    deferredUpdateTimer_->setSingleShot(true);
    deferredUpdateTimer_->setInterval(3000);
    connect(deferredUpdateTimer_, &QTimer::timeout, this, &LibraryModel::reloadDecoationData);
    _root = new RootLibraryNode(-1, 0, QVariant(), nullptr);
    connect(&imageStorage_, SIGNAL(imageUpdated(QString)), deferredUpdateTimer_, SLOT(start()));
}

LibraryModel::~LibraryModel()
{
    delete _root;
}

void LibraryModel::reloadData()
{
    Q_EMIT beginResetModel();
    _root->clear_children();
    build_tree(_root, &db_);
    Q_EMIT endResetModel();
}

void LibraryModel::build_tree(LibraryModelNode *root, Database *db)
{
    if (root->is_leaf) {
        return;
    }
    root->load_children(db);
    for (int i = 0; i < root->children.size(); ++i) {
        build_tree(root->children[i], db);
    }
}

QModelIndex LibraryModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent)) {
        return QModelIndex();
    }

    LibraryModelNode *node;
    if (parent.isValid()) {
        node = static_cast<LibraryModelNode *>(parent.internalPointer());
    } else {
        node = _root;
    }
    LibraryModelNode *child = node->childAt(row);
    if (child) {
        return createIndex(row, column, child);
    }
    return QModelIndex();
}

QModelIndex LibraryModel::parent(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return QModelIndex();
    }
    auto node = static_cast<LibraryModelNode *>(index.internalPointer());
    if (node->level == 0 || !node->parent) {
        return QModelIndex();
    }
    return createIndex(node->parent->index, 0, node->parent);
}

int LibraryModel::rowCount(const QModelIndex &parent) const
{
    LibraryModelNode *node;
    if (!parent.isValid()) {
        node = _root;
    } else {
        node = static_cast<LibraryModelNode *>(parent.internalPointer());
    }
    return node->children.size();
}

int LibraryModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 1;
}

QVariant LibraryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }
    auto node = static_cast<LibraryModelNode *>(index.internalPointer());
    if (role == Qt::DisplayRole) {
        return node->presentation_value();
    } else if (role == Qt::DecorationRole) {
        QString imageUrl = node->imageUrl();
        if (!imageUrl.isEmpty()) {
            QPixmap *cachedImg = QPixmapCache::find(imageUrl);
            if (cachedImg)
                return *cachedImg;
            QByteArray imageData = imageStorage_.tryGetImage(imageUrl);
            if (!imageData.isEmpty()) {
                QPixmap img;
                if (img.loadFromData(imageData)) {
                    QPixmap scaledImg = img.scaled(24, 24, Qt::KeepAspectRatio);
                    QPixmapCache::insert(imageUrl, scaledImg);
                    return scaledImg;
                }
            }
        }
    } else if (role == Qt::SizeHintRole) {
        return QSize(INT_MAX, 30);
    }

    return QVariant();
}

Qt::ItemFlags LibraryModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return Q_NULLPTR;
    }

    return QAbstractItemModel::flags(index);
}

QVariant LibraryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(section);
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        return QString::fromLocal8Bit("Library");
    }
    return QVariant();
}

void LibraryModel::setDatabasePath(const QString &dbPath)
{
    if (!db_.openConnection(dbPath)) {
        qWarning() << "could not open database at" << dbPath;
        return;
    }
}

void LibraryModel::reloadDecoationData()
{
    emit dataChanged(index(0, 0), index(_root->children.size() - 1, 0),
                     QVector<int>{Qt::DecorationRole});
}
