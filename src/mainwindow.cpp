#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QLabel>
#include <QMediaPlayer>
#include <QTimer>
#include <QToolBar>

#include "playertoolbar.h"
#include "settingsmodel.h"
#include "user.h"

#define BACKUP_INTERVAL 10 * 60 * 1000

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    settingsModel_ = new SettingsModel(this);
    user_          = new User(this);

    connect(user_, &User::emailChanged, settingsModel_, &SettingsModel::setEmail);
    connect(user_, &User::authTokenChanged, settingsModel_, &SettingsModel::setAuthToken);
    connect(user_, &User::masterTokenChanged, settingsModel_, &SettingsModel::setMasterToken);
    connect(user_, &User::deviceIdChanged, settingsModel_, &SettingsModel::setDeviceId);

    connect(user_, &User::errorOccured, this, &MainWindow::handleErrorOccured);
    connect(user_, &User::authorizedChanged, this, &MainWindow::handleAuthorizedChanged);
    connect(user_, &User::syncStarted, this, &MainWindow::enterSyncState);
    connect(user_, &User::syncFinished, this, &MainWindow::openLibraryPage);
    connect(user_, &User::databasePathChanged, this, &MainWindow::setDatabasePath);

    connect(user_, &User::authorizedChanged, ui->actionSynchronize, &QAction::setEnabled);
    connect(ui->actionSynchronize, &QAction::triggered, user_, &User::sync);

    connect(ui->loginPage, &LoginWidget::emailChanged, user_, &User::setEmail);
    connect(ui->loginPage, &LoginWidget::statusChanged, this, &MainWindow::handleStatusChanged);
    connect(ui->loginPage, &LoginWidget::login, user_, &User::login);
    connect(ui->loginPage, &LoginWidget::autoLoginChanged, settingsModel_,
            &SettingsModel::setAutoLogin);

    connect(user_, &User::syncFinished, ui->libraryPage, &LibraryWidget::reloadData);
    connect(user_, &User::databasePathChanged, ui->libraryPage, &LibraryWidget::setDatabasePath);

    player_ = new QMediaPlayer(this);
    applyVolume(50);
    connect(ui->libraryPage, &LibraryWidget::play, this, &MainWindow::handlePlayRequest);
    connect(ui->libraryPage, &LibraryWidget::playerRewind, this, &MainWindow::handleRewindRequest);
    connect(player_, &QMediaPlayer::stateChanged, ui->libraryPage,
            &LibraryWidget::handlePlayerStateChanged);
    connect(player_, &QMediaPlayer::positionChanged, ui->libraryPage,
            &LibraryWidget::handlePlayerPositionChanged);
    connect(player_, &QMediaPlayer::durationChanged, ui->libraryPage,
            &LibraryWidget::handlePLayerDurationChanged);
    connect(player_, &QMediaPlayer::stateChanged, this, &MainWindow::handlePlayerStateChanged);

    settingsModel_->load();
    user_->setAuthToken(settingsModel_->authToken());
    user_->setDeviceId(settingsModel_->deviceId());
    user_->setEmail(settingsModel_->email());
    user_->setMasterToken(settingsModel_->masterToken());

    backupTimer_ = new QTimer(this);
    connect(backupTimer_, &QTimer::timeout, settingsModel_, &SettingsModel::save);
    backupTimer_->start(BACKUP_INTERVAL);

    setupToolbar();

    openLoginPage();
}

MainWindow::~MainWindow()
{
    settingsModel_->save();
    delete ui;
}

void MainWindow::handleStatusChanged(const QString &status)
{
    ui->statusbar->showMessage(status, 5 * 1000);
}

void MainWindow::handleAuthorizedChanged(bool isAuthorized)
{
    ui->statusbar->showMessage(isAuthorized ? "Success" : "Failure", 5 * 1000);
    if (isAuthorized) {
        user_->sync();
        openLibraryPage();
    }
}

void MainWindow::handleErrorOccured(const QString &error)
{
    ui->statusbar->showMessage(error, 5 * 1000);
}

void MainWindow::openLoginPage()
{
    if (user_->canRestore() && settingsModel_->autoLogin()) {
        user_->restore();
    } else {
        ui->loginPage->setEmail(user_->email());
        ui->stackedWidget->setCurrentWidget(ui->loginPage);
    }
}

void MainWindow::openSyncPage()
{
    ui->stackedWidget->setCurrentWidget(ui->syncPage);
}

void MainWindow::openLibraryPage()
{
    ui->stackedWidget->setCurrentWidget(ui->libraryPage);
}

void MainWindow::handlePlayRequest(const QString &trackId)
{
    ProxyResult *result = user_->getStreamUrl(trackId);
    connect(result, &ProxyResult::ready, this, [this, trackId](int status, QVariant result) {
        if (status == ProxyResult::Error) {
            qDebug() << "Failed to get stream url: " << result.toString();
            return;
        }
        if (player_->state() == QMediaPlayer::State::PlayingState) {
            player_->stop();
        }
        this->setCurrentTrack(trackId);
        player_->setMedia(QUrl(result.toString()));
        player_->play();
    });
}

void MainWindow::applyVolume(int volume)
{
    qreal linearVolume = QAudio::convertVolume(
        volume / qreal(100.0), QAudio::LogarithmicVolumeScale, QAudio::LinearVolumeScale);
    player_->setVolume(qRound(linearVolume * 100));
}

void MainWindow::enterSyncState()
{
    auto syncWidget = new SyncWidget(this);
    syncWidget->setAttribute(Qt::WA_DeleteOnClose);
    connect(user_, &User::errorOccured, syncWidget, &SyncWidget::close);
    connect(user_, &User::syncProgressChanged, syncWidget, &SyncWidget::setProgress);
    connect(user_, &User::syncFinished, syncWidget, &SyncWidget::close);
    syncWidget->setWindowFlags(Qt::Dialog);
    syncWidget->setWindowModality(Qt::WindowModal);
    syncWidget->show();
}

void MainWindow::setupToolbar()
{
    toolbar_ = new PlayerToolbar;
    toolbar_->setDatabasePath(user_->databasePath());
    connect(player_, &QMediaPlayer::stateChanged, toolbar_,
            &PlayerToolbar::handlePlayerStateChanged);
    connect(player_, &QMediaPlayer::durationChanged, toolbar_,
            &PlayerToolbar::handleDuratioChanged);
    connect(player_, &QMediaPlayer::positionChanged, toolbar_,
            &PlayerToolbar::handlePositionChanged);
    connect(user_, &User::databasePathChanged, toolbar_, &PlayerToolbar::setDatabasePath);
    connect(toolbar_, &PlayerToolbar::play, ui->libraryPage, &LibraryWidget::playCurrent);
    connect(toolbar_, &PlayerToolbar::pause, player_, &QMediaPlayer::pause);
    connect(toolbar_, &PlayerToolbar::next, ui->libraryPage, &LibraryWidget::playNext);
    connect(toolbar_, &PlayerToolbar::prev, ui->libraryPage, &LibraryWidget::playPrev);
    addToolBar(Qt::ToolBarArea::TopToolBarArea, toolbar_);
}

void MainWindow::setCurrentTrack(const QString &trackId)
{
    currentTrackId_ = trackId;
    ui->libraryPage->setCurrentTrackId(trackId);
    toolbar_->setCurrentTrack(trackId);
}

void MainWindow::setDatabasePath(const QString &path)
{
    db_.openConnection(path);
}

void MainWindow::handlePlayerStateChanged(int state)
{
    if (state == QMediaPlayer::PlayingState) {
        Opt<GMTrack> track = db_.track(currentTrackId_);
        if (!track) {
            return;
        }
        Opt<GMArtist> artist = db_.artist(track->artistId.at(0));
        if (!artist) {
            return;
        }
        setWindowTitle(QString("%1 - %2").arg(artist->name).arg(track->title));
    } else if (state == QMediaPlayer::StoppedState) {
        setWindowTitle(qApp->applicationName());
    }
}

void MainWindow::handleRewindRequest()
{
    player_->setPosition(0);
    player_->play();
}
