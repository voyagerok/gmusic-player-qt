#include "librarywidget.h"
#include "ui_librarywidget.h"

#include <QHeaderView>
#include <QLayout>
#include <QListView>
#include <QMediaPlayer>
#include <QSortFilterProxyModel>
#include <QSplitter>
#include <QTableView>
#include <QToolBar>
#include <QTreeView>

#include "librarymodel.h"
#include "tracklistmodel.h"

LibraryWidget::LibraryWidget(QWidget *parent) : QWidget(parent), ui(new Ui::LibraryWidget)
{
    ui->setupUi(this);

    mainSplitter_ = new QSplitter;

    libraryModel_  = new LibraryModel(this);
    auto lTreeView = new QTreeView;
    lTreeView->setModel(libraryModel_);
    connect(lTreeView, &QTreeView::doubleClicked, this, [this](const QModelIndex &index) {
        auto node = static_cast<LibraryModelNode *>(index.internalPointer());
        if (node->is_leaf) {
            trackListModel_->setLoaderFunc(node->tracksLoader());
        }
    });

    trackListModel_      = new TrackListModel(this);
    auto sortFilterModel = new TrackListSortingModel(this);
    sortFilterModel->sort(0);
    sortFilterModel->setSourceModel(trackListModel_);
    trackListTableView_ = new QTableView;
    trackListTableView_->setModel(sortFilterModel);
    trackListTableView_->setSelectionBehavior(QAbstractItemView::SelectRows);
    trackListTableView_->verticalHeader()->hide();
    trackListTableView_->horizontalHeader()->setHighlightSections(false);
    trackListTableView_->horizontalHeader()->setStretchLastSection(true);
    trackListTableView_->setSelectionMode(QAbstractItemView::SingleSelection);
    trackListTableView_->setColumnWidth(0, 35);
    trackListTableView_->setShowGrid(false);
    connect(trackListTableView_, &QTableView::doubleClicked, this,
            [this](const QModelIndex &index) {
                QString trackId = index.data(TrackListModel::TrackId).toString();
                emit play(trackId);
            });

    //    auto rightSplitter = new QSplitter;
    //    rightSplitter->setOrientation(Qt::Vertical);
    //    auto remotePlaylistView  = new QListView;
    //    auto remotePodscastsView = new QListView;
    //    rightSplitter->addWidget(remotePlaylistView);
    //    rightSplitter->addWidget(remotePodscastsView);

    mainSplitter_->addWidget(lTreeView);
    mainSplitter_->addWidget(trackListTableView_);
    //    mainSplitter_->addWidget(rightSplitter);

    mainSplitter_->setSizes(QList<int>() << 200 << INT_MAX);

    mainSplitter_->setCollapsible(1, false);
    mainSplitter_->setStretchFactor(0, 0);
    mainSplitter_->setStretchFactor(1, 1);
    mainSplitter_->setStretchFactor(2, 0);

    this->layout()->addWidget(mainSplitter_);
}

LibraryWidget::~LibraryWidget()
{
    delete ui;
}

void LibraryWidget::setDatabasePath(const QString &dbPath)
{
    libraryModel_->setDatabasePath(dbPath);
    trackListModel_->setDatabasePath(dbPath);
}

void LibraryWidget::reloadData()
{
    libraryModel_->reloadData();
    trackListModel_->reloadTracks();
}

void LibraryWidget::playCurrent()
{
    QModelIndexList indexes = trackListTableView_->selectionModel()->selectedIndexes();
    if (!indexes.isEmpty()) {
        QString trackId = indexes.at(0).data(TrackListModel::TrackId).toString();
        if (trackId != currentTrackId_) {
            emit play(trackId);
        }
    }
}

void LibraryWidget::playNext()
{
    if (currentPlayerState_ == QMediaPlayer::StoppedState) {
        return;
    }

    auto proxyModel = qobject_cast<TrackListSortingModel *>(trackListTableView_->model());
    QModelIndex current =
        proxyModel->mapFromSource(trackListModel_->getIndexForId(currentTrackId_));
    QModelIndex next = proxyModel->index(current.row() + 1, current.column());
    if (next.isValid()) {
        QString nextTrackId = next.data(TrackListModel::TrackId).toString();
        emit play(nextTrackId);
    }
}

void LibraryWidget::playPrev()
{
    if (currentPlayerState_ == QMediaPlayer::StoppedState) {
        return;
    }

    if (currentPlayerPos_ > 2 * 1000) {
        emit playerRewind();
        return;
    }

    auto proxyModel = qobject_cast<TrackListSortingModel *>(trackListTableView_->model());
    QModelIndex current =
        proxyModel->mapFromSource(trackListModel_->getIndexForId(currentTrackId_));
    QModelIndex prev = proxyModel->index(current.row() - 1, current.column());
    if (prev.isValid()) {
        QString prevTrackId = prev.data(TrackListModel::TrackId).toString();
        emit play(prevTrackId);
    }
}

void LibraryWidget::setCurrentTrackId(const QString &currentTrack)
{
    currentTrackId_ = currentTrack;
}

void LibraryWidget::handlePLayerDurationChanged(qint64 duration)
{
}

void LibraryWidget::handlePLayerVolumeChanged(int volume)
{
}

void LibraryWidget::handlePlayerMutedChanged(bool muted)
{
}

void LibraryWidget::handlePlayerPositionChanged(qint64 position)
{
    currentPlayerPos_ = position;
}

void LibraryWidget::handlePlayerSeekableChanged(bool seekable)
{
}

void LibraryWidget::handlePlayerStateChanged(int state)
{
    if (state == QMediaPlayer::PlayingState) {
        auto proxyModel = qobject_cast<TrackListSortingModel *>(trackListTableView_->model());
        QModelIndex index =
            proxyModel->mapFromSource(trackListModel_->getIndexForId(currentTrackId_));
        if (index.isValid()) {
            trackListTableView_->selectionModel()->setCurrentIndex(
                index, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
        }
    }
    currentPlayerState_ = state;
}

void LibraryWidget::setupToolbar(QToolBar *appToolbar)
{
}
