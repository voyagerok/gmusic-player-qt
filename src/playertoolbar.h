#ifndef PLAYERTOOLBAR_H
#define PLAYERTOOLBAR_H

#include <QToolBar>

class QLabel;
class QSlider;
class Database;
class ImageStorage;

namespace Ui
{
class PlayerToolbar;
}

class PlayerToolbar : public QToolBar
{
    Q_OBJECT

public:
    explicit PlayerToolbar(QWidget *parent = nullptr);
    ~PlayerToolbar();

public slots:
    void setCurrentTrack(const QString &id);
    void setDatabasePath(const QString &dbPath);
    void handlePlayerStateChanged(int state);
    void handleDuratioChanged(qint64 duration);
    void handlePositionChanged(qint64 position);

signals:
    void seek(qint64 msec);
    void play();
    void pause();
    void next();
    void prev();

private:
    Ui::PlayerToolbar *ui;

    QAction *playAction_;
    QAction *pauseAction_;
    QAction *nextAction_;
    QAction *prevAction_;

    //    QLabel *imgLabel_;
    QSlider *progressSlider_;
    QLabel *timeLabel_;

    Database *db_;
    QString currentTrackId_;
    ImageStorage &imageStorage_;
    int trackDuration_;

    void handlePlaybackStarted();
    void handlePlaybackStopped();
};

#endif // PLAYERTOOLBAR_H
