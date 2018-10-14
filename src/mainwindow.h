#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "database.h"

class User;
class SettingsModel;
class QTimer;
class QMediaPlayer;
class AudioPlayer;
class PlayerToolbar;

namespace Ui
{
class MainWindow;
}

class QShortcut;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void handleStatusChanged(const QString &status);
    void handleAuthorizedChanged(bool authorized);
    void handleErrorOccured(const QString &error);
    void setDatabasePath(const QString &path);
    void enterSyncState();

    void openLoginPage();
    void openSyncPage();
    void openLibraryPage();

    void handlePlayRequest(const QString &trackId);
    void handleRewindRequest();
    void applyVolume(int volume);
    void setCurrentTrack(const QString &id);

    void handlePlayerStateChanged(int state);
    void handlePlayerVolumeChanged(int volume);

    void playerSeek(int seconds);

signals:
    void playbackStarted(const QString &trackId);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    Ui::MainWindow *ui;
    User *user_;
    QMediaPlayer *player_;
    SettingsModel *settingsModel_;
    QTimer *backupTimer_;
    QString currentTrackId_;
    PlayerToolbar *toolbar_;
    Database db_;
    QStyle *appStyle_;

    QAction *playAction_;
    QAction *pauseAction_;
    QAction *skipForwardAction_;
    QAction *skipBackwardAction_;
    QAction *seekForward1Action_;
    QAction *seekForward5Action_;
    QAction *seekBackward1Action_;
    QAction *seekBackward5Action_;

    QTimer *printFocusedTimer_;

    void setupMenu();
    void setupToolbar();
    void setupShortcuts();
    void setupActions();
};

#endif // MAINWINDOW_H
