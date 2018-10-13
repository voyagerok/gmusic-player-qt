#include "player.h"

#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTemporaryFile>
#include <memory>
#include <mpg123.h>

#define MPG123_CHECK_AND_THROW(err_code, exception)                                                \
    if (err_code != MPG123_OK)                                                                     \
    throw exception(mpg123_plain_strerror(errCode))

AudioSource::AudioSource(const QString &url, QObject *parent)
    : QObject(parent), url_(url), fetched_(false)
{
    if (!tempFile_.open()) {
        qDebug() << __PRETTY_FUNCTION__ << ": could not open temp file";
    }
}

void AudioSource::beginFetch()
{
    if (!tempFile_.isOpen()) {
        qDebug() << __PRETTY_FUNCTION__ << ": temp file not opened";
        emit errorOccured();
        return;
    }
    tempFile_.reset();
    QNetworkReply *reply = accessManager_->get(QNetworkRequest(url_));
    connect(reply, &QNetworkReply::readyRead, this, [this] {
        auto reply = qobject_cast<QNetworkReply *>(sender());
        tempFile_.write(reply->readAll());
    });
    connect(reply, &QNetworkReply::finished, this, [this] {
        auto reply = qobject_cast<QNetworkReply *>(sender());
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << this->url_ << ": download error: " << reply->errorString();
            emit errorOccured();
            return;
        }
        fetched_ = true;
    });
}

void AudioSource::read(qint64 maxlen)
{
    if (tempFile_.atEnd()) {
        emit endOfFile();
    } else {
        emit readBytes(tempFile_.read(maxlen));
    }
}

void AudioSource::seek(qint64 offset)
{
    bool success = tempFile_.seek(offset);
    if (!success) {
        qDebug() << __PRETTY_FUNCTION__ << ": seek failed";
    }
}

AudioDecoder::AudioDecoder() : QObject(Q_NULLPTR), handle_(nullptr)
{
    mpg123_init();
    handle_ = mpg123_new(NULL, NULL);
}

AudioDecoder::~AudioDecoder()
{
    mpg123_delete(handle_);
}

void AudioDecoder::open()
{
    int err = mpg123_open_feed(handle_);
    if (err != MPG123_OK) {
        qDebug() << __PRETTY_FUNCTION__ << ": could not open feed";
    }
}

void AudioDecoder::close()
{
    mpg123_close(handle_);
}

#define DECODER_BUFFER_SIZE 4096

void AudioDecoder::decodeData(QByteArray buffer)
{
    if (!buffer.isEmpty()) {
        size_t done;
        unsigned char *output;
        off_t frameOffset;

        mpg123_feed(handle_, (unsigned char *)buffer.data(), buffer.size());
        mpg123_volume(handle_, (double)volume_ / 100);
        do {
            int status = mpg123_decode_frame(handle_, &frameOffset, &output, &done);
            switch (status) {
            case MPG123_NEW_FORMAT:
                long rate;
                int channels, encoding;
                mpg123_getformat(handle_, &rate, &channels, &encoding);
                emit newFormat(channels, encoding, rate);
                break;
            case MPG123_OK:
                emit decodeData(QByteArray::fromRawData((char *)output, done));
                break;
            }
        } while (done > 0);
    }
}

void AudioDecoder::seekTime(int secs)
{
    long rate;
    int status = mpg123_getformat2(handle_, &rate, NULL, NULL, 0);
    if (status == MPG123_OK) {
        off_t inputOffset;
        qint64 offset = rate * secs;
        if (mpg123_feedseek(handle_, offset, SEEK_SET, &inputOffset) >= 0) {
            emit seek(inputOffset);
        }
    }
}

void AudioDecoder::setVolume(int volume)
{
    volume_ = volume;
}

// AudioOutput::~AudioOutput()
//{
//    Stop();
//}

// bool AudioOutput::Start(int bits, int channels, size_t rate)
//{
//    if (device != nullptr) {
//        return true;
//    }
//    if (defaultDriver == -1) {
//        defaultDriver = ao_default_driver_id();
//    }
//    memset(&format, 0, sizeof(ao_sample_format));
//    format.bits        = bits;
//    format.byte_format = AO_FMT_NATIVE;
//    format.channels    = channels;
//    format.rate        = static_cast<int>(rate);
//    device             = ao_open_live(defaultDriver, &format, nullptr);
//    if (device == nullptr) {
//        char errBuf[1024];
//        char *err = strerror_r(errno, errBuf, 1024);
//        return false;
//    }
//    return true;
//}

// bool AudioOutput::Stop()
//{
//    if (device == nullptr) {
//        return true;
//    }
//    if (ao_close(device) == 0) {
//        return false;
//    }
//    device = nullptr;
//    return true;
//}

// bool AudioOutput::Play(char *data, size_t len)
//{
//    if (device == nullptr) {
//        return false;
//    }
//    if (ao_play(device, data, static_cast<uint_32>(len)) == 0) {
//        return false;
//    }
//    return true;
//}

// AudioPlayer::AudioPlayer()
//{
//    int errCode = mpg123_init();
//    MPG123_CHECK_AND_THROW(errCode, AudioPlayerException);
//    AudioOutput::Initialize();
//}

// AudioPlayer::~AudioPlayer()
//{
//    requestedCommand = PLAYER_COMMAND_STOP;
//    downloadQueue.unregister(this);
//    playQueue.unregister(this);
//    output.Stop();
//    mpg123_exit();
//    AudioOutput::Destruct();
//    if (this->cachefile != nullptr) {
//        fclose(this->cachefile);
//        cachefile = nullptr;
//    }
//}

// void AudioPlayer::stop()
//{
//    stopRoutines();
//    resetDownloaderData();
//    resetPlayerData();
//    if (this->cachefile) {
//        fclose(this->cachefile);
//        cachefile = nullptr;
//    }
//}

// void AudioPlayer::stopRoutines()
//{
//    requestedCommand.store(PLAYER_COMMAND_STOP);
//    condvar.notify_one();
//    playQueue.wait();
//    downloadQueue.wait();
//    requestedCommand.store(PLAYER_COMMAND_PROCEED);
//}

// void AudioPlayer::resetDownloaderData()
//{
//    downloadingFinished = false;
//    hasMoreData         = false;
//    totalSize           = 0;
//    downloadProgress    = 0;
//}

// void AudioPlayer::resetPlayerData()
//{
//    progressQueue.clear();
//    playerStatus         = PLAYER_STATUS_IDLE;
//    requestedSeekSeconds = -1;
//    shouldReportProgress = true;
//}

// void AudioPlayer::seek(double seconds)
//{
//    std::lock_guard<std::mutex> lock(seekMutex);
//    requestedCommand     = PLAYER_COMMAND_SEEK;
//    requestedSeekSeconds = seconds;
//    shouldReportProgress = false;
//    progressQueue.clear();
//}

//#define READBUF_SIZE 4096

// void AudioPlayer::playTrack(const std::string &url)
//{
//    stop();
//    playerStatus = PLAYER_STATUS_PLAYING;

//    this->cachefile = tmpfile();
//    if (this->cachefile == nullptr) {
//        playerStatus = PLAYER_STATUS_IDLE;
//        throw std::runtime_error("failed to create temp file");
//    }
//    downloadQueue.scheduleTask([this, url] { this->downloadRoutine(url); }, this);

//    playQueue.scheduleTask([this] { this->playRoutine(); }, this);
//}

// void AudioPlayer::downloadRoutine(const std::string &url)
//{
//    HttpRequest requst(HttpMethod::GET, url);
//    HttpSession session;

//    session.set_progress_callback([this](size_t total, size_t received, HttpSession *) -> int {
//        this->downloadProgress = total == 0 ? 0 : static_cast<double>(received) / total;
//        this->totalSize        = total;
//        if (delegate) {
//            delegate->updateCacheProgress();
//        }
//        return 0;
//    });

//    session.set_data_callback([this](char *data, size_t len) -> size_t {
//        if (requestedCommand == PLAYER_COMMAND_STOP) {
//            return len == 0 ? ++len : 0;
//        }
//        std::unique_lock<std::mutex> lock(this->cachefileMutex);
//        fseek(this->cachefile, 0, SEEK_END);
//        size_t written = fwrite(data, sizeof(char), len, this->cachefile);
//        if (written != len) {
//        }
//        lock.unlock();
//        hasMoreData = true;
//        condvar.notify_one();
//        return len;
//    });

//    auto result         = session.make_request(requst);
//    downloadingFinished = true;
//    condvar.notify_one();
//}

// void AudioPlayer::playRoutine()
//{
//    char readBuffer[READBUF_SIZE];
//    long currentOffset = 0;

//    mpg123_handle *decoder = mpg123_new(nullptr, nullptr);
//    mpg123_open_feed(decoder);

//    struct FormatInfo {
//        int channels, encoding;
//        long rate;
//        bool isSet = false;
//    } formatInfo;

//    while (true) {
//        size_t read = 0;
//        do {
//            {
//                std::lock_guard<std::mutex> lock(seekMutex);
//                if (requestedCommand == PLAYER_COMMAND_SEEK) {
//                    if (this->totalSize > 0) {
//                        if (formatInfo.isSet) {
//                            off_t sampleOffset =
//                                static_cast<off_t>(formatInfo.rate * requestedSeekSeconds);
//                            off_t inputOffset;
//                            off_t success =
//                                mpg123_feedseek(decoder, sampleOffset, SEEK_SET, &inputOffset);
//                            if (success >= 0 &&
//                                inputOffset <= this->totalSize * this->downloadProgress) {
//                                currentOffset = inputOffset;
//                            }
//                        }
//                    }
//                    requestedCommand     = PLAYER_COMMAND_PROCEED;
//                    shouldReportProgress = true;
//                }
//            }

//            std::unique_lock<std::mutex> lock(this->cachefileMutex);
//            condvar.wait(lock, [this] {
//                if (requestedCommand == PLAYER_COMMAND_STOP) {
//                    return true;
//                }
//                if (hasMoreData || downloadingFinished) {
//                    if (playerStatus == PLAYER_STATUS_PAUSED &&
//                        requestedCommand != PLAYER_COMMAND_PROCEED) {
//                        return false;
//                    }
//                    return true;
//                }
//                return false;
//            });

//            if (requestedCommand == PLAYER_COMMAND_STOP) {
//                goto exit;
//            } else if (requestedCommand == PLAYER_COMMAND_PAUSE) {
//                playerStatus = PLAYER_STATUS_PAUSED;
//                continue;
//            } else if (requestedCommand == PLAYER_COMMAND_PROCEED) {
//                playerStatus = PLAYER_STATUS_PLAYING;
//            }

//            fseek(this->cachefile, currentOffset, SEEK_SET);
//            read = fread(readBuffer, sizeof(char), READBUF_SIZE, this->cachefile);
//            lock.unlock();
//            currentOffset += read;
//            mpg123_feed(decoder, reinterpret_cast<unsigned char *>(readBuffer), read);

//            size_t done;
//            unsigned char *audioData;
//            off_t frameOffset;
//            do {

//                mpg123_volume(decoder, currentVolumeScale * VOLUME_MAX_LEVEL);
//                int err = mpg123_decode_frame(decoder, &frameOffset, &audioData, &done);
//                switch (err) {
//                case MPG123_NEW_FORMAT:
//                    int encoding;
//                    mpg123_getformat(decoder, &formatInfo.rate, &formatInfo.channels, &encoding);
//                    formatInfo.encoding = mpg123_encsize(encoding);
//                    formatInfo.isSet    = true;
//                    output.Start(formatInfo.encoding * 8, formatInfo.channels,
//                                 static_cast<size_t>(formatInfo.rate));
//                    playerStatus = PLAYER_STATUS_PLAYING;
//                    if (delegate != nullptr) {
//                        delegate->playbackStarted();
//                    }
//                    break;
//                case MPG123_OK:
//                    output.Play(reinterpret_cast<char *>(audioData), done);
//                    break;
//                }
//            } while (done > 0);

//            if (shouldReportProgress && this->totalSize > 0 && delegate) {
//                double playbackProgress = static_cast<double>(currentOffset) / this->totalSize;
//                progressQueue.push(playbackProgress);
//                delegate->updatePlaybackProgress();
//            }

//        } while (read == READBUF_SIZE);

//        hasMoreData = false;
//        if (downloadingFinished) {
//            break;
//        }
//    }

// exit:
//    output.Stop();
//    mpg123_delete(decoder);
//    playerStatus = PLAYER_STATUS_IDLE;
//    if (delegate != nullptr) {
//        delegate->playbackFinished();
//    }
//}

// void AudioPlayer::changeVolume(double volumeScale)
//{
//    if (fabs(volumeScale - currentVolumeScale) < 0.001) {
//        return;
//    }
//    this->currentVolumeScale.store(volumeScale);
//}

// double AudioPlayer::getLastProgressValue()
//{
//    if (progressQueue.empty()) {
//        return 0;
//    }
//    return progressQueue.pop();
//}

// bool AudioPlayer::inProgress() const
//{
//    return playerStatus != PLAYER_STATUS_IDLE;
//}

// void AudioPlayer::pause()
//{
//    requestedCommand = PLAYER_COMMAND_PAUSE;
//}

// void AudioPlayer::resume()
//{
//    requestedCommand = PLAYER_COMMAND_PROCEED;
//    condvar.notify_one();
//}
