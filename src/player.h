#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <QObject>
#include <QTemporaryFile>
#include <QUrl>

class QNetworkAccessManager;

class AudioSource : public QObject
{
    Q_OBJECT
public:
    AudioSource(const QString &url, QObject *parent = Q_NULLPTR);

public slots:
    void beginFetch();
    void read(qint64 maxlen);
    void seek(qint64 offset);

signals:
    void readBytes(QByteArray data);
    void errorOccured();
    void endOfFile();

private:
    QNetworkAccessManager *accessManager_;
    QUrl url_;
    QTemporaryFile tempFile_;
    bool fetched_;
};

extern "C" {
struct mpg123_handle;
}

class AudioDecoder : public QObject
{
    Q_OBJECT

public:
    AudioDecoder();
    ~AudioDecoder();

public slots:
    void open();
    void close();
    void decodeData(QByteArray data);
    void seekTime(int sec);
    void setVolume(int level);

signals:
    void dataDecoded(QByteArray data);
    void feedMore(qint64 size);
    void seek(qint64 offset);
    void newFormat(int channels, int encoding, qint64 rate);

private:
    mpg123_handle *handle_;
    int volume_;
};

class AudioOutput : public QObject
{

};

class AudioPlayer : public QObject
{
    Q_OBJECT

public:
    AudioPlayer(QObject *parent = Q_NULLPTR);
};

// namespace gmusic
//{

// class AudioPlayerException : public std::runtime_error
//{
//    using std::runtime_error::runtime_error;
//};

// enum {
//    PLAYER_COMMAND_PROCEED,
//    PLAYER_COMMAND_PAUSE,
//    PLAYER_COMMAND_STOP,
//    PLAYER_COMMAND_SEEK
//};

// enum { PLAYER_STATUS_IDLE, PLAYER_STATUS_PLAYING, PLAYER_STATUS_PAUSED };

// struct AudioPlayerDelegate {
//    virtual void updatePlaybackProgress() = 0;
//    virtual void updateCacheProgress()    = 0;
//    virtual void playbackFinished()       = 0;
//    virtual void playbackStarted()        = 0;
//    virtual ~AudioPlayerDelegate()        = default;
//};

// struct AudioOutput {
//    static void Initialize() { ao_initialize(); }
//    static void Destruct() { ao_shutdown(); }

//    ~AudioOutput();

//    bool Start(int bits, int channels, size_t rate);
//    bool Stop();
//    bool Play(char *data, size_t len);

//  private:
//    ao_device *device = nullptr;
//    ao_sample_format format;
//    int defaultDriver = -1;
//};

// class AudioPlayer
//{
//  public:
//    AudioPlayer();
//    ~AudioPlayer();
//    void playTrack(const std::string &trackUrl);
//    void stop();
//    void pause();
//    void resume();
//    bool inProgress() const;
//    void changeVolume(double volumeScale);
//    double getLastProgressValue();
//    void setDelegate(AudioPlayerDelegate *delegate)
//    {
//        this->delegate = delegate;
//    }
//    void seek(double seconds);

//  private:
//    void playRoutine();
//    void playRoutine2();
//    void downloadRoutine(const std::string &url);
//    void stopRoutines();
//    void resetDownloaderData();
//    void resetPlayerData();

//    OperationQueue playQueue;
//    std::atomic<double> currentVolumeScale{0.5};
//    std::atomic_int requestedCommand{PLAYER_COMMAND_PROCEED};
//    std::atomic_int playerStatus{PLAYER_STATUS_IDLE};
//    ThreadSafeQueue<double> progressQueue;
//    std::atomic<double> requestedSeekSeconds{-1};
//    std::mutex seekMutex;
//    std::atomic_bool shouldReportProgress{true};

//    OperationQueue downloadQueue;
//    std::atomic<size_t> totalSize{0};
//    std::atomic_bool downloadingFinished{false};
//    std::atomic_bool hasMoreData{false};
//    std::atomic<double> downloadProgress{0};

//    std::condition_variable condvar;
//    std::mutex cachefileMutex;
//    AudioOutput output;
//    AudioPlayerDelegate *delegate = nullptr;

//    FILE *cachefile = nullptr;
//};
//}

#endif // PLAYER_HPP
