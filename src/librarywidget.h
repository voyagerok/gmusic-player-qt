#ifndef LIBRARYWIDGET_H
#define LIBRARYWIDGET_H

#include <QPersistentModelIndex>
#include <QWidget>

class QSplitter;
class LibraryModel;
class TrackListModel;
class QToolBar;
class QTableView;

namespace Ui
{
class LibraryWidget;
}

class LibraryWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LibraryWidget(QWidget *parent = nullptr);
    ~LibraryWidget();

signals:
    void play(const QString &trackId);

public slots:
    void setDatabasePath(const QString &);
    void reloadData();
    void setupToolbar(QToolBar *appToolbar);

    void handlePlayerStateChanged(int state);
    void handlePLayerDurationChanged(qint64 duration);
    void handlePlayerPositionChanged(qint64 pos);
    void handlePlayerSeekableChanged(bool seekable);
    void handlePLayerVolumeChanged(int volume);
    void handlePlayerMutedChanged(bool muted);

    void playCurrent();
    void playNext();
    void playPrev();

    void setCurrentTrackId(const QString &trackId);

private:
    Ui::LibraryWidget *ui;

    QSplitter *mainSplitter_;
    LibraryModel *libraryModel_;
    TrackListModel *trackListModel_;
    QTableView *trackListTableView_;
    QString currentTrackId_;
};

#endif // LIBRARYWIDGET_H
