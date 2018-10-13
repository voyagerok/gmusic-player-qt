#ifndef LIBRARYMODEL_H
#define LIBRARYMODEL_H

#include <QAbstractItemModel>
#include <QSet>

#include "database.h"

using TracksLoader = std::function<Opt<GMTrackList>(Database &)>;

struct LibraryModelNode {
    LibraryModelNode(int level, int index, const QVariant &data, LibraryModelNode *parent,
                     bool is_leaf)
        : level(level), index(index), data(data), is_leaf(is_leaf), parent(parent)
    {
    }
    virtual ~LibraryModelNode();

    void clear_children()
    {
        qDeleteAll(children);
        children.clear();
    }

    virtual void load_children(Database * /*db*/)
    {
    }

    virtual TracksLoader tracksLoader() const
    {
        return [](Database &db) { return db.tracks(); };
    }

    virtual QVariant presentation_value() const = 0;
    virtual QString imageUrl() const
    {
        return QString();
    }

    int level;
    int index;
    QVariant data;
    bool is_leaf;
    QList<LibraryModelNode *> children;
    LibraryModelNode *parent;
};

struct RootLibraryNode : public LibraryModelNode {
    RootLibraryNode(int level, int index, const QVariant &data, LibraryModelNode *parent)
        : LibraryModelNode(level, index, data, parent, false)
    {
    }
    QVariant presentation_value() const override
    {
        return QVariant();
    }
    void load_children(Database *db) override;
};

struct ArtistLibraryNode : public LibraryModelNode {
    ArtistLibraryNode(int level, int index, const QVariant &data, LibraryModelNode *parent)
        : LibraryModelNode(level, index, data, parent, false)
    {
    }
    QVariant presentation_value() const override;
    QString imageUrl() const override;
    TracksLoader tracksLoader() const override;
    void load_children(Database *db) override;
};

struct AlbumLibraryNode : public LibraryModelNode {
    AlbumLibraryNode(int level, int index, const QVariant &data, LibraryModelNode *parent)
        : LibraryModelNode(level, index, data, parent, true)
    {
    }
    QVariant presentation_value() const override;
    QString imageUrl() const override;
    TracksLoader tracksLoader() const override;
};

class ImageStorage;
class QTimer;

class LibraryModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    LibraryModel(QObject *parent = Q_NULLPTR);
    ~LibraryModel() override;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    void reloadData();

public slots:
    void setDatabasePath(const QString &path);

private slots:
    void reloadDecoationData();

private:
    void build_tree(LibraryModelNode *root, Database *db);
    LibraryModelNode *_root;
    Database db_;
    ImageStorage &imageStorage_;
    QTimer *deferredUpdateTimer_;
    mutable QSet<QPersistentModelIndex> pendingItems_;
};

#endif // LIBRARYMODEL_H
