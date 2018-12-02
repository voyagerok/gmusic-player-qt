#include "playertoolbar.h"
#include "ui_playertoolbar.h"

#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMediaPlayer>
#include <QProxyStyle>
#include <QSlider>
#include <QSpacerItem>

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
    : QToolBar(parent), ui(new Ui::PlayerToolbar), appStyle_(qApp->style()),
      imageStorage_(ImageStorage::instance()), trackDuration_(0)
{
    ui->setupUi(this);

    db_ = new Database(this);

    progressSlider_ = new QSlider;
    progressSlider_->setStyle(new MyStyle(progressSlider_->style()));
    progressSlider_->setOrientation(Qt::Horizontal);
    progressSlider_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    connect(progressSlider_, &QSlider::actionTriggered, this, [this](int action) {
        if (playerIsSeekable_) {
            if (action == QAbstractSlider::SliderMove ||
                action == QAbstractSlider::SliderSingleStepAdd ||
                action == QAbstractSlider::SliderSingleStepSub) {
                emit seek(progressSlider_->sliderPosition() * 1000);
            }
        }
    });
    addWidget(progressSlider_);

    auto spacer = new QWidget;
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    spacer->setMaximumWidth(10);
    addWidget(spacer);

    timeLabel_ = new QLabel;
    timeLabel_->setText(QStringLiteral("00:00 / 00:00"));
    timeLabel_->setMinimumWidth(50);
    addWidget(timeLabel_);

    muteAction_ = addAction(appStyle_->standardIcon(QStyle::SP_MediaVolume), tr("Mute"));
    connect(muteAction_, &QAction::triggered, this, [this] { emit mutedChanged(!muted_); });

    volumeSlider_ = new QSlider;
    volumeSlider_->setStyle(new MyStyle(volumeSlider_->style()));
    volumeSlider_->setOrientation(Qt::Horizontal);
    volumeSlider_->setMinimum(0);
    volumeSlider_->setMaximum(100);
    volumeSlider_->setMaximumWidth(100);
    connect(volumeSlider_, &QSlider::actionTriggered, this, [this](int action) {
        if (action == QAbstractSlider::SliderMove ||
            action == QAbstractSlider::SliderSingleStepAdd ||
            action == QAbstractSlider::SliderSingleStepSub) {
            emit volumeChanged(volumeSlider_->sliderPosition());
        }
    });
    addWidget(volumeSlider_);

    addSeparator();

    searchLineEdit_ = new QLineEdit;
    searchLineEdit_->setMinimumWidth(50);
    searchLineEdit_->setMaximumWidth(450);
    searchLineEdit_->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    searchLineEdit_->setPlaceholderText(tr("Search"));
    searchLineEdit_->addAction(appStyle_->standardIcon(QStyle::SP_ComputerIcon),
                               QLineEdit::ActionPosition::LeadingPosition);
    searchLineEdit_->setFocusPolicy(Qt::StrongFocus);
    searchLineEdit_->setFocus();
    addWidget(searchLineEdit_);
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
    progressSlider_->setHidden(false);
    timeLabel_->setHidden(false);
}

void PlayerToolbar::handlePlaybackStopped()
{
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

void PlayerToolbar::handlePlayerIsSeekableChanged(bool seekable)
{
    playerIsSeekable_ = seekable;
}

void PlayerToolbar::setVolume(int volume)
{
    if (volumeSlider_->value() != volume) {
        volumeSlider_->setValue(volume);
    }
}

void PlayerToolbar::setMuted(bool muted)
{
    if (muted_ != muted) {
        muted_ = muted;
        muteAction_->setIcon(
            appStyle_->standardIcon(muted_ ? QStyle::SP_MediaVolumeMuted : QStyle::SP_MediaVolume));
    }
}
