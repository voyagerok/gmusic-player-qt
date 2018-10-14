#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QKeyEvent>
#include <QLabel>
#include <QMediaPlayer>
#include <QShortcut>
#include <QTimer>
#include <QToolBar>

#include "playertoolbar.h"
#include "settingsmodel.h"
#include "user.h"

#define BACKUP_INTERVAL 10 * 60 * 1000

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), appStyle_(qApp->style())
{
    ui->setupUi(this);

    printFocusedTimer_ = new QTimer(this);
    printFocusedTimer_->setSingleShot(false);
    connect(printFocusedTimer_, &QTimer::timeout, this, [this] {
        qDebug() << "focused widget name:" << focusWidget()->metaObject()->className();
    });

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
    connect(ui->libraryPage, &LibraryWidget::play, this, &MainWindow::handlePlayRequest);
    connect(ui->libraryPage, &LibraryWidget::playerRewind, this, &MainWindow::handleRewindRequest);
    connect(ui->libraryPage, &LibraryWidget::playerResume, player_, &QMediaPlayer::play);
    connect(player_, &QMediaPlayer::stateChanged, ui->libraryPage,
            &LibraryWidget::handlePlayerStateChanged);
    connect(player_, &QMediaPlayer::positionChanged, ui->libraryPage,
            &LibraryWidget::handlePlayerPositionChanged);
    connect(player_, &QMediaPlayer::durationChanged, ui->libraryPage,
            &LibraryWidget::handlePLayerDurationChanged);
    connect(player_, &QMediaPlayer::stateChanged, this, &MainWindow::handlePlayerStateChanged);
    connect(player_, &QMediaPlayer::volumeChanged, this, &MainWindow::handlePlayerVolumeChanged);

    settingsModel_->load();
    user_->setAuthToken(settingsModel_->authToken());
    user_->setDeviceId(settingsModel_->deviceId());
    user_->setEmail(settingsModel_->email());
    user_->setMasterToken(settingsModel_->masterToken());

    backupTimer_ = new QTimer(this);
    connect(backupTimer_, &QTimer::timeout, settingsModel_, &SettingsModel::save);
    backupTimer_->start(BACKUP_INTERVAL);

    setupActions();
    setupToolbar();
    setupShortcuts();
    setupMenu();

    applyVolume(settingsModel_->volume());
    player_->setMuted(settingsModel_->muted());

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
        //        printFocusedTimer_->start(5 * 1000);
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
    toolbar_->setVolume(settingsModel_->volume());
    toolbar_->setMuted(settingsModel_->muted());
    connect(player_, &QMediaPlayer::stateChanged, toolbar_,
            &PlayerToolbar::handlePlayerStateChanged);
    connect(player_, &QMediaPlayer::durationChanged, toolbar_,
            &PlayerToolbar::handleDuratioChanged);
    connect(player_, &QMediaPlayer::positionChanged, toolbar_,
            &PlayerToolbar::handlePositionChanged);
    connect(player_, &QMediaPlayer::seekableChanged, toolbar_,
            &PlayerToolbar::handlePlayerIsSeekableChanged);
    connect(player_, &QMediaPlayer::mutedChanged, toolbar_, &PlayerToolbar::setMuted);
    connect(player_, &QMediaPlayer::mutedChanged, settingsModel_, &SettingsModel::setMuted);

    connect(user_, &User::databasePathChanged, toolbar_, &PlayerToolbar::setDatabasePath);

    connect(toolbar_, &PlayerToolbar::seek, player_, &QMediaPlayer::setPosition);
    connect(toolbar_, &PlayerToolbar::volumeChanged, this, &MainWindow::applyVolume);
    connect(toolbar_, &PlayerToolbar::mutedChanged, player_, &QMediaPlayer::setMuted);

    QAction *firstAction = toolbar_->actions().at(0);
    toolbar_->insertActions(firstAction, QList<QAction *>()
                                             << playAction_ << pauseAction_ << skipBackwardAction_
                                             << skipForwardAction_);

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
    if (player_->isSeekable()) {
        player_->setPosition(0);
        player_->play();
    } else {
        qDebug() << __FUNCTION__ << ": player not seekable";
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
}

void MainWindow::handlePlayerVolumeChanged(int volume)
{
    qreal logVol = QAudio::convertVolume(volume / qreal(100.0), QAudio::LinearVolumeScale,
                                         QAudio::LogarithmicVolumeScale);
    int value    = qRound(logVol * 100);
    settingsModel_->setVolume(value);
    toolbar_->setVolume(value);
}

void MainWindow::setupShortcuts()
{
}

void MainWindow::setupMenu()
{
    ui->menuPlayback->addActions(QList<QAction *>() << playAction_ << pauseAction_
                                                    << skipBackwardAction_ << skipForwardAction_);
    ui->menuPlayback->addSeparator();
    ui->menuPlayback->addActions(QList<QAction *>() << seekBackward1Action_ << seekBackward5Action_
                                                    << seekForward1Action_ << seekForward5Action_);
}

void MainWindow::setupActions()
{
    playAction_  = new QAction(appStyle_->standardIcon(QStyle::SP_MediaPlay), tr("Play"), this);
    pauseAction_ = new QAction(appStyle_->standardIcon(QStyle::SP_MediaPause), tr("Pause"), this);
    skipForwardAction_ =
        new QAction(appStyle_->standardIcon(QStyle::SP_MediaSkipForward), tr("Skip Forward"), this);
    skipBackwardAction_  = new QAction(appStyle_->standardIcon(QStyle::SP_MediaSkipBackward),
                                      tr("Skip Backward"), this);
    seekForward1Action_  = new QAction(tr("Seek Forward (1 second)"), this);
    seekForward5Action_  = new QAction(tr("Seek Forward (5 seconds)"), this);
    seekBackward1Action_ = new QAction(tr("Seek Backward (1 second)"), this);
    seekBackward5Action_ = new QAction(tr("Seek Backward (5 seconds)"), this);

    playAction_->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_S));
    pauseAction_->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_P));
    skipBackwardAction_->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Comma));
    skipForwardAction_->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Period));
    seekForward1Action_->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Right));
    seekForward5Action_->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Up));
    seekBackward1Action_->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Left));
    seekBackward5Action_->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Down));

    connect(playAction_, &QAction::triggered, ui->libraryPage, &LibraryWidget::playCurrent);
    connect(pauseAction_, &QAction::triggered, player_, &QMediaPlayer::pause);
    connect(skipForwardAction_, &QAction::triggered, ui->libraryPage, &LibraryWidget::playNext);
    connect(skipBackwardAction_, &QAction::triggered, ui->libraryPage, &LibraryWidget::playPrev);
    connect(seekBackward1Action_, &QAction::triggered,
            std::bind(&MainWindow::playerSeek, this, -1));
    connect(seekBackward5Action_, &QAction::triggered,
            std::bind(&MainWindow::playerSeek, this, -5));
    connect(seekForward1Action_, &QAction::triggered, std::bind(&MainWindow::playerSeek, this, 1));
    connect(seekForward5Action_, &QAction::triggered, std::bind(&MainWindow::playerSeek, this, 5));

    connect(player_, &QMediaPlayer::seekableChanged, seekBackward1Action_, &QAction::setEnabled);
    connect(player_, &QMediaPlayer::seekableChanged, seekBackward5Action_, &QAction::setEnabled);
    connect(player_, &QMediaPlayer::seekableChanged, seekForward1Action_, &QAction::setEnabled);
    connect(player_, &QMediaPlayer::seekableChanged, seekForward5Action_, &QAction::setEnabled);
}

void MainWindow::playerSeek(int seconds)
{
    if (player_->isSeekable()) {
        player_->setPosition(player_->position() + seconds * 1000);
    }
}
