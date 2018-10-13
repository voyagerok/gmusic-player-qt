#include "playertoolbar.h"
#include "ui_playertoolbar.h"

#include <QLabel>
#include <QLayout>
#include <QMediaPlayer>
#include <QProxyStyle>
#include <QSlider>

#include "database.h"
#include "imagestorage.h"
#include "utils.h"

class MyStyle : public QProxyStyle
{
public:
    using QProxyStyle::QProxyStyle;

    int styleHint(QStyle::StyleHint hint, const QStyleOption *option = 0, const QWidget *widget = 0,
                  QStyleHintReturn *returnData = 0) const
    {
        if (hint == QStyle::SH_Slider_AbsoluteSetButtons)
            return (Qt::LeftButton | Qt::MidButton | Qt::RightButton);
        return QProxyStyle::styleHint(hint, option, widget, returnData);
    }
};

PlayerToolbar::PlayerToolbar(QWidget *parent)
    : QToolBar(parent), ui(new Ui::PlayerToolbar), imageStorage_(ImageStorage::instance()),
      trackDuration_(0)
{
    ui->setupUi(this);

    db_ = new Database(this);

    QStyle *appStyle = qApp->style();

    playAction_  = addAction(appStyle->standardIcon(QStyle::SP_MediaPlay), tr("Play"));
    pauseAction_ = addAction(appStyle->standardIcon(QStyle::SP_MediaPause), tr("Pause"));
    nextAction_  = addAction(appStyle->standardIcon(QStyle::SP_MediaSkipForward), tr("Play next"));
    prevAction_ =
        addAction(appStyle->standardIcon(QStyle::SP_MediaSkipBackward), tr("Play previous"));
    connect(playAction_, &QAction::triggered, this, &PlayerToolbar::play);
    connect(pauseAction_, &QAction::triggered, this, &PlayerToolbar::pause);
    connect(nextAction_, &QAction::triggered, this, &PlayerToolbar::next);
    connect(prevAction_, &QAction::triggered, this, &PlayerToolbar::prev);

    //    imgLabel_ = new QLabel(this);
    //    imgLabel_->setMinimumSize(64, 64);
    //    imgLabel_->setMaximumSize(64, 64);
    //    addWidget(imgLabel_);

    progressSlider_ = new QSlider;
    progressSlider_->setStyle(new MyStyle(progressSlider_->style()));
    progressSlider_->setOrientation(Qt::Horizontal);

    addWidget(progressSlider_);
    timeLabel_ = new QLabel;
    timeLabel_->setMinimumWidth(50);
    addWidget(timeLabel_);
}

PlayerToolbar::~PlayerToolbar()
{
    delete ui;
}

void PlayerToolbar::setDatabasePath(const QString &dbPath)
{
    db_->openConnection(dbPath);
}

void PlayerToolbar::setCurrentTrack(const QString &id)
{
    currentTrackId_ = id;
}

void PlayerToolbar::handlePlayerStateChanged(int state)
{
    switch (state) {
    case QMediaPlayer::PlayingState:
        handlePlaybackStarted();
        break;
    case QMediaPlayer::StoppedState:
        handlePlaybackStopped();
        break;
    }
}

void PlayerToolbar::handlePlaybackStarted()
{
    //    imgLabel_->setHidden(false);
    progressSlider_->setHidden(false);
    timeLabel_->setHidden(false);

    //    if (auto track = db_->track(currentTrackId_)) {
    //        if (auto album = db_->album(track->albumId)) {

    //            QByteArray imgData = imageStorage_.tryGetImage(album->albumArtRef);
    //            if (!imgData.isEmpty()) {
    //                QPixmap img;
    //                if (img.loadFromData(imgData)) {
    //                    imgLabel_->setPixmap(img.scaled(64, 64, Qt::KeepAspectRatio));
    //                }
    //            }
    //        }
    //    }
}

void PlayerToolbar::handlePlaybackStopped()
{
    //    imgLabel_->setHidden(true);
    progressSlider_->setHidden(true);
    timeLabel_->setHidden(true);
}

void PlayerToolbar::handleDuratioChanged(qint64 duration)
{
    int durationSec = (int)(duration / 1000);
    progressSlider_->setMinimum(0);
    progressSlider_->setMaximum(durationSec);
    timeLabel_->setText(QString("%1 / %2").arg("00:00").arg(Utils::TimeFormat(durationSec)));
    trackDuration_ = durationSec;
}

void PlayerToolbar::handlePositionChanged(qint64 pos)
{
    int posSec = (int)pos / 1000;
    progressSlider_->setValue(posSec);
    timeLabel_->setText(
        QString("%1 / %2").arg(Utils::TimeFormat(posSec)).arg(Utils::TimeFormat(trackDuration_)));
}
