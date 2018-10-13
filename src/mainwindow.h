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

    void setupToolbar();
};

#endif // MAINWINDOW_H
