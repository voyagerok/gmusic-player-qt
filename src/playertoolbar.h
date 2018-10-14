#ifndef PLAYERTOOLBAR_H
#define PLAYERTOOLBAR_H

#include <QToolBar>

class QLabel;
class QSlider;
class Database;
class ImageStorage;
class SettingsModel;

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
    void handlePlayerIsSeekableChanged(bool seekable);

    void setVolume(int volume);
    void setMuted(bool muted);

signals:
    void seek(qint64 msec);
    void volumeChanged(int volume);
    void mutedChanged(bool muted);
    void play();
    void pause();
    void next();
    void prev();

private:
    Ui::PlayerToolbar *ui;
    QStyle *appStyle_;

    QAction *playAction_;
    QAction *pauseAction_;
    QAction *nextAction_;
    QAction *prevAction_;
    QAction *muteAction_;

    QSlider *progressSlider_;
    QLabel *timeLabel_;
    QSlider *volumeSlider_;

    bool playerIsSeekable_;

    Database *db_;
    QString currentTrackId_;
    ImageStorage &imageStorage_;
    int trackDuration_;

    bool muted_;

    void handlePlaybackStarted();
    void handlePlaybackStopped();
};

#endif // PLAYERTOOLBAR_H
