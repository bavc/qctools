#ifndef MEDIAPARSER_H
#define MEDIAPARSER_H

#include <QIODevice>
#include <QObject>
#include <QMutex>
#include <QWaitCondition>
#include <QThreadPool>
#include <QFuture>
#include <QList>
#include "qavplayer.h"
#include <qaviodevice_p.h>
#include <qavdemuxer_p.h>
#include <qavfiltergraph_p.h>
#include <qavfilter_p.h>

enum PendingMediaStatus
{
    LoadingMedia,
    PlayingMedia,
    PausingMedia,
    StoppingMedia,
    SteppingMedia,
    SeekingMedia,
    EndOfMedia
};

class QAVQueueClock
{
public:
    QAVQueueClock(double v = 1 / 24.0)
        : frameRate(v)
    {
    }

    bool sync(bool sync, double pts, double speed = 1.0, double master = -1);

    double frameRate = 0;
    double frameTimer = 0;
    double prevPts = 0;
    const double maxFrameDuration = 10.0;
    const double minThreshold = 0.04;
    const double maxThreshold = 0.1;
    const double frameDuplicationThreshold = 0.1;
    const double refreshRate = 0.01;
};

class QAVPacketQueue
{
public:
    QAVPacketQueue(const QAVDemuxer &demuxer) : m_demuxer(demuxer)
    {
    }

    virtual ~QAVPacketQueue()
    {
        abort();
    }

    bool isEmpty() const
    {
        QMutexLocker locker(&m_mutex);
        return m_packets.isEmpty() && !m_frame && isEmptyData();
    }

    void enqueue(const QAVPacket &packet);

    void waitForEmpty()
    {
        QMutexLocker locker(&m_mutex);
        clearPackets();
        if (!m_abort && !m_waitingForPackets)
            m_producerWaiter.wait(&m_mutex);
        clearTimers();
    }

    QAVPacket dequeue();

    void abort()
    {
        QMutexLocker locker(&m_mutex);
        m_abort = true;
        m_waitingForPackets = true;
        m_consumerWaiter.wakeAll();
        m_producerWaiter.wakeAll();
    }

    bool enough() const
    {
        QMutexLocker locker(&m_mutex);
        const int minFrames = 15;
        return m_packets.size() > minFrames && (!m_duration || m_duration > 1.0);
    }

    int bytes() const
    {
        QMutexLocker locker(&m_mutex);
        return m_bytes;
    }

    void clear()
    {
        QMutexLocker locker(&m_mutex);
        clearPackets();
        clearTimers();
        clearData();
    }

    virtual int frame(bool sync, double speed, double master, QAVStreamFrame &frame) = 0;

    double pts() const
    {
        QMutexLocker locker(&m_mutex);
        return m_clock.prevPts;
    }

    void setFrameRate(double v)
    {
        QMutexLocker locker(&m_mutex);
        m_clock.frameRate = v;
    }

    void wake(bool wake)
    {
        QMutexLocker locker(&m_mutex);
        if (wake)
            m_consumerWaiter.wakeAll();
        m_wake = wake;
    }

protected:
    Q_DISABLE_COPY(QAVPacketQueue)

    void clearPackets()
    {
        m_packets.clear();
        m_bytes = 0;
        m_duration = 0;
        m_frame.reset();
    }

    void clearTimers()
    {
        m_clock.prevPts = 0;
        m_clock.frameTimer = 0;
    }

    virtual void clearData()
    {
    }

    virtual bool isEmptyData() const
    {
        return true;
    }

    const QAVDemuxer &m_demuxer;
    QList<QAVPacket> m_packets;
    mutable QMutex m_mutex;
    QWaitCondition m_consumerWaiter;
    QWaitCondition m_producerWaiter;
    bool m_abort = false;
    bool m_waitingForPackets = true;
    bool m_wake = false;

    int m_bytes = 0;
    int m_duration = 0;

    QScopedPointer<QAVStreamFrame> m_frame;
    QAVQueueClock m_clock;
};



class QAVPacketFrameQueue : public QAVPacketQueue
{
public:
    QAVPacketFrameQueue(const QAVDemuxer &demuxer) : QAVPacketQueue(demuxer)
    {
    }

    int frame(bool sync, double speed, double master, QAVStreamFrame &baseFrame) override
    {
        QMutexLocker locker(&m_mutex);
        QAVFrame &frame = static_cast<QAVFrame &>(baseFrame);
        frame = m_frame ? static_cast<QAVFrame &>(*m_frame) : QAVFrame{};
        if (!frame) {
            const bool decode = !m_filter || m_filter->eof();
            if (decode) {
                locker.unlock();
                m_demuxer.decode(dequeue(), frame);
                locker.relock();
                if (m_filter && frame) {
                    m_ret = m_filter->write(frame);
                    if (m_ret < 0) {
                        if (m_ret != AVERROR_EOF)
                            return m_ret;
                    }
                }
            }
            if (m_filter) {
                m_ret = m_filter->read(frame);
                if (m_ret < 0) {
                    if (m_ret != AVERROR(EAGAIN))
                        return m_ret;
                }
            } else {
                m_ret = 0;
            }

            if (frame)
                m_frame.reset(new QAVFrame(frame));
        }
        locker.unlock();

        if (frame && m_clock.sync(sync, frame.pts(), speed, master)) {
            frame.frame()->sample_rate *= speed;
            locker.relock();
            m_frame.reset();
            return m_ret;
        }

        frame = {};
        return AVERROR(EAGAIN);
    }

    void setFilter(QAVFilter *filter)
    {
        QMutexLocker locker(&m_mutex);
        m_filter.reset(filter);
        m_frame.reset();
    }

private:
    void clearData() override
    {
        m_filter.reset();
    }

    bool isEmptyData() const override
    {
        return !m_filter || m_filter->eof();
    }

    QScopedPointer<QAVFilter> m_filter;
    int m_ret = 0;
};

class QAVPlayerPrivate : public QObject {

    Q_OBJECT

public:
    QAVPlayerPrivate(QObject* parent = nullptr) : QObject(parent) {

    }
    void setSource(const QString &url, QIODevice *dev = nullptr);

    void wait(bool v);
    QString source() const;
    QList<QAVStream> videoStreams() const;
    QAVStream videoStream() const;
    void setVideoStream(const QAVStream &stream);
    QList<QAVStream> audioStreams() const;
    QAVStream audioStream() const;
    void setAudioStream(const QAVStream &stream);
    QAVPlayer::Error currentError() const;
    bool setState(QAVPlayer::State s);
    void resetPendingStatuses();
    void setError(QAVPlayer::Error err, const QString &str);
    void setPendingMediaStatus(PendingMediaStatus status);
    bool isEndOfFile() const;
    void endOfFile(bool v);
    void stop();
    void doDemux();
    QAVPlayer::MediaStatus mediaStatus() const;
    void setMediaStatus(QAVPlayer::MediaStatus status);
    void doLoad();
    void applyFilter();
    void applyFilter(bool reset, const QAVFrame &frame);
    void doWait();
    bool skipFrame(const QAVStreamFrame &frame, const QAVPacketQueue &queue, bool master = true);
    bool isSeeking() const;
    qint64 duration() const;
    double pts() const;
    qint64 position() const;
    QAVPlayer::State state() const;
    bool doStep(PendingMediaStatus status, bool hasFrame);
    void step(bool hasFrame);
    bool doPlayStep(double refPts, bool master, QAVPacketQueue &queue, bool &sync, QAVStreamFrame &frame);
    void doPlayVideo();
    void doPlayAudio();
Q_SIGNALS:

    void sourceChanged(const QString &url);
    void stateChanged(QAVPlayer::State newState);
    void mediaStatusChanged(QAVPlayer::MediaStatus status);
    void errorOccurred(QAVPlayer::Error, const QString &str);
    void durationChanged(qint64 duration);
    void seekableChanged(bool seekable);
    void speedChanged(qreal rate);
    void videoFrameRateChanged(double rate);
    void videoStreamChanged(const QAVStream &stream);
    void audioStreamChanged(const QAVStream &stream);
    void subtitleStreamChanged(const QAVStream &stream);
    void played(qint64 pos);
    void paused(qint64 pos);
    void stopped(qint64 pos);
    void stepped(qint64 pos);
    void seeked(qint64 pos);
    void filterChanged(const QString &desc);
    void bitstreamFilterChanged(const QString &desc);
    void syncedChanged(bool sync);

    void videoFrame(const QAVVideoFrame &frame);
    void audioFrame(const QAVAudioFrame &frame);
    void subtitleFrame(const QAVSubtitleFrame &frame);

private:
    QString url;
    QScopedPointer<QAVIODevice> dev;
    QAVPlayer::MediaStatus _mediaStatus = QAVPlayer::NoMedia;
    QList<PendingMediaStatus> pendingMediaStatuses;
    QAVPlayer::State _state = QAVPlayer::StoppedState;
    mutable QMutex stateMutex;

    bool seekable = false;
    qreal speed = 1.0;
    mutable QMutex speedMutex;
    double videoFrameRate = 0.0;

    double _duration = 0;
    double pendingPosition = 0;
    bool pendingSeek = false;
    mutable QMutex positionMutex;
    bool synced = true;

    int _videoStream = -1;
    int _audioStream = -1;
    int subtitleStream = -1;

    QAVPlayer::Error error = QAVPlayer::NoError;

    QAVDemuxer demuxer;

    QThreadPool threadPool;
    QFuture<void> loaderFuture;
    QFuture<void> demuxerFuture;

    QFuture<void> videoPlayFuture;
    QScopedPointer<QAVPacketFrameQueue> videoQueue { nullptr };

    QFuture<void> audioPlayFuture;
    QScopedPointer<QAVPacketFrameQueue> audioQueue { nullptr };

    bool quit = 0;
    bool isWaiting = false;
    mutable QMutex waitMutex;
    QWaitCondition waitCond;
    bool eof = false;

    QString filterDesc;
    QScopedPointer<QAVFilterGraph> filterGraph;
};

class MediaParser : public QAVPlayerPrivate {
public:
    MediaParser(QObject* parent = nullptr) : QAVPlayerPrivate(parent) {}

};

#endif // 
