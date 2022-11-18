#include "libavformat/avformat.h"

#include <QProcess>
#include <QFileInfo>
#include <QDebug>
#include <QTemporaryFile>
#include <QXmlStreamWriter>
#include <QUrl>
#include <QBuffer>
#include <QPair>
#include <QDir>
#include <zlib.h>
#include <zconf.h>

#include <cmath>
#include <string>
#include <sstream>
#include <iostream>
#include <cassert>
#ifdef _WIN32
#include <QEventLoop>
#include <QFuture>
#include <QThreadPool>
    #include <algorithm>
#include <qavaudiofilter_p.h>
#include <qaviodevice_p.h>
#include <qavvideofilter_p.h>
#endif

#include "MediaParser.h"
#include "qavdemuxer_p.h"
#include "qavpacket_p.h"
#include "qavfilter_p.h"
#include "qavplayer.h"
#include "qavfiltergraph_p.h"
#include "qavframe.h"
#include "qavdemuxer_p.h"
#include "qavstreamframe.h"
#include <QMutex>
#include <QWaitCondition>
#include <QList>
#include <math.h>
#include <QSharedPointer>
#include <QtConcurrent>

extern "C" {
#include <libavutil/time.h>
#include <libavcodec/avcodec.h>
}

#if 1

static QString err_str(int err)
{
    char errbuf[128];
    const char *errbuf_ptr = errbuf;
    if (av_strerror(err, errbuf, sizeof(errbuf)) < 0)
        errbuf_ptr = strerror(AVUNERROR(err));

    return QString::fromUtf8(errbuf_ptr);
}

Q_LOGGING_CATEGORY(lcAVPlayer, "qt.QtAVParser")

void QAVPlayerPrivate::setSource(const QString &url, QIODevice *dev)
{
    if (this->url == url)
        return;

    qCDebug(lcAVPlayer) << __FUNCTION__ << ":" << url;

    terminate();
    this->url = url;
    if(dev)
        this->dev.reset(new QAVIODevice(*dev));
    Q_EMIT sourceChanged(url);
    wait(true);
    quit = false;
    if (url.isEmpty())
        return;

    setPendingMediaStatus(LoadingMedia);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    loaderFuture = QtConcurrent::run(&threadPool, this, &QAVPlayerPrivate::doLoad);
#else
    loaderFuture = QtConcurrent::run(&threadPool, &QAVPlayerPrivate::doLoad, d);
#endif
}

QString QAVPlayerPrivate::source() const
{
    return url;
}

QList<QAVStream> QAVPlayerPrivate::videoStreams() const
{
    return demuxer.videoStreams();
}

QAVStream QAVPlayerPrivate::videoStream() const
{
    return demuxer.videoStream();
}

void QAVPlayerPrivate::setVideoStream(const QAVStream &stream)
{
    qCDebug(lcAVPlayer) << __FUNCTION__ << ":" << demuxer.videoStream().index() << "->" << stream.index();
    if (demuxer.setVideoStream(stream))
        Q_EMIT videoStreamChanged(stream);
}

QList<QAVStream> QAVPlayerPrivate::audioStreams() const
{
    return demuxer.audioStreams();
}

QAVStream QAVPlayerPrivate::audioStream() const
{
    return demuxer.audioStream();
}

void QAVPlayerPrivate::setAudioStream(const QAVStream &stream)
{
    qCDebug(lcAVPlayer) << __FUNCTION__ << ":" << demuxer.audioStream().index() << "->" << stream.index();
    if (demuxer.setAudioStream(stream))
        Q_EMIT audioStreamChanged(stream);
}

QAVPlayer::Error QAVPlayerPrivate::currentError() const {
    QMutexLocker locker(&stateMutex);
    return error;
}

bool QAVPlayerPrivate::setState(QAVPlayer::State s) {
    bool result = false;
    {
        QMutexLocker locker(&stateMutex);
        if (_state == s)
            return result;

        qCDebug(lcAVPlayer) << __FUNCTION__ << ":" << _state << "->" << s;
        _state = s;
        result = true;
    }

    Q_EMIT stateChanged(s);
    return result;
}

void QAVPlayerPrivate::resetPendingStatuses()
{
    QMutexLocker locker(&stateMutex);
    qCDebug(lcAVPlayer) << __FUNCTION__ << ":" << pendingMediaStatuses;
    pendingMediaStatuses.clear();
    wait(true);
}

void QAVPlayerPrivate::setError(QAVPlayer::Error err, const QString &str)
{
    {
        QMutexLocker locker(&stateMutex);
        error = err;
    }

    qWarning() << err << ":" << str;
    Q_EMIT errorOccurred(err, str);
    setMediaStatus(QAVPlayer::InvalidMedia);
    setState(QAVPlayer::StoppedState);
    resetPendingStatuses();
}

void QAVPlayerPrivate::wait(bool v)
{
    {
        QMutexLocker locker(&waitMutex);
        if (isWaiting != v)
            qCDebug(lcAVPlayer) << __FUNCTION__ << ":" << isWaiting << "->" << v;
        isWaiting = v;
    }

    if (!v)
        waitCond.wakeAll();
    videoQueue->wake(true);
    audioQueue->wake(true);
}

void QAVPlayerPrivate::setPendingMediaStatus(PendingMediaStatus status)
{
    QMutexLocker locker(&stateMutex);
    pendingMediaStatuses.push_back(status);
    qCDebug(lcAVPlayer) << __FUNCTION__ << ":" << _mediaStatus << "->" << pendingMediaStatuses;
}

bool QAVPlayerPrivate::isEndOfFile() const
{
    QMutexLocker locker(&stateMutex);
    return eof;
}

void QAVPlayerPrivate::endOfFile(bool v)
{
    QMutexLocker locker(&stateMutex);
    eof = v;
}

void QAVPlayerPrivate::stop() {
    if (currentError() == QAVPlayer::ResourceError)
        return;

    qCDebug(lcAVPlayer) << __FUNCTION__;
    if (setState(QAVPlayer::StoppedState)) {
        setPendingMediaStatus(StoppingMedia);
        wait(false);
    } else {
        wait(true);
    }

    if (mediaStatus() != QAVPlayer::NoMedia)
        applyFilter();
}

void QAVPlayerPrivate::doDemux()
{
    const int maxQueueBytes = 15 * 1024 * 1024;
    QMutex waiterMutex;
    QWaitCondition waiter;

    while (!quit) {
        if (videoQueue->bytes() + audioQueue->bytes() > maxQueueBytes
            || (videoQueue->enough() && audioQueue->enough()))
        {
            QMutexLocker locker(&waiterMutex);
            waiter.wait(&waiterMutex, 10);
            continue;
        }

        auto packet = demuxer.read();
        if (packet) {
            endOfFile(false);
            if (packet.packet()->stream_index == demuxer.videoStream().index())
                videoQueue->enqueue(packet);
            else if (packet.packet()->stream_index == demuxer.audioStream().index())
                audioQueue->enqueue(packet);
        } else {
            if (demuxer.eof() && videoQueue->isEmpty() && audioQueue->isEmpty() && !isEndOfFile()) {
                endOfFile(true);
                qCDebug(lcAVPlayer) << "EndOfMedia";
                setPendingMediaStatus(EndOfMedia);
                stop();
                wait(false);
            }

            QMutexLocker locker(&waiterMutex);
            waiter.wait(&waiterMutex, 10);
        }
    }
    qCDebug(lcAVPlayer) << __FUNCTION__ << "finished";
}

QAVPlayer::MediaStatus QAVPlayerPrivate::mediaStatus() const
{
    QMutexLocker locker(&stateMutex);
    return _mediaStatus;
}

void QAVPlayerPrivate::setMediaStatus(QAVPlayer::MediaStatus status)
{
    {
        QMutexLocker locker(&stateMutex);
        if (_mediaStatus == status)
            return;

        if (status != QAVPlayer::InvalidMedia)
            error = QAVPlayer::NoError;

        qCDebug(lcAVPlayer) << __FUNCTION__ << ":" << _mediaStatus << "->" << status;
        _mediaStatus = status;
    }

    Q_EMIT mediaStatusChanged(status);
}

void QAVPlayerPrivate::doLoad() {
    demuxer.abort(false);
    demuxer.unload();
    int ret = demuxer.load(url, dev.data());
    if (ret < 0) {
        setError(QAVPlayer::ResourceError, err_str(ret));
        return;
    }

    if (!demuxer.videoStream() && !demuxer.audioStream()) {
        setError(QAVPlayer::ResourceError, QLatin1String("No codecs found"));
        return;
    }

//        applyFilter();
//        dispatch([this] {
//            qCDebug(lcAVPlayer) << "[" << url << "]: Loaded, seekable:" << demuxer.seekable() << ", duration:" << demuxer.duration();
//            setSeekable(demuxer.seekable());
//            setDuration(demuxer.duration());
//            setVideoFrameRate(demuxer.videoFrameRate());
//            step(false);
//        });

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    demuxerFuture = QtConcurrent::run(&threadPool, this, &QAVPlayerPrivate::doDemux);
    if (!videoStreams().isEmpty())
        videoPlayFuture = QtConcurrent::run(&threadPool, this, &QAVPlayerPrivate::doPlayVideo);
    if (!audioStreams().isEmpty())
        audioPlayFuture = QtConcurrent::run(&threadPool, this, &QAVPlayerPrivate::doPlayAudio);
#else
    demuxerFuture = QtConcurrent::run(&threadPool, &QAVPlayerPrivate::doDemux, this);
    if (!videoStreams().isEmpty())
        videoPlayFuture = QtConcurrent::run(&threadPool, &QAVPlayerPrivate::doPlayVideo, this);
    if (!audioStreams().isEmpty())
        audioPlayFuture = QtConcurrent::run(&threadPool, &QAVPlayerPrivate::doPlayAudio, this);
#endif
    qCDebug(lcAVPlayer) << __FUNCTION__ << "finished";
}

void QAVPlayerPrivate::applyFilter()
{
    applyFilter(false, {});
}

void QAVPlayerPrivate::applyFilter(bool reset, const QAVFrame &frame)
{
    QMutexLocker locker(&stateMutex);
    if ((filterDesc.isEmpty() && !filterGraph) || (!reset && filterGraph && filterGraph->desc() == filterDesc))
        return;

    qCDebug(lcAVPlayer) << __FUNCTION__ << ":" << filterDesc;
    videoQueue->setFilter(nullptr);
    audioQueue->setFilter(nullptr);
    QScopedPointer<QAVFilterGraph> graph(!filterDesc.isEmpty() ? new QAVFilterGraph : nullptr);
    if (graph) {
        auto setFilterError = [&](const QString &err, int ret) {
            locker.unlock();
            setError(QAVPlayer::FilterError, err + ": " + err_str(ret));
        };
        int ret = graph->parse(filterDesc);
        if (ret < 0) {
            setFilterError(QLatin1String("Could not parse filter desc"), ret);
            return;
        }
        QAVFrame videoFrame;
        QAVFrame audioFrame;
        videoFrame.setStream(demuxer.videoStream());
        audioFrame.setStream(demuxer.audioStream());
        auto stream = frame.stream().stream();
        if (stream) {
            switch (stream->codecpar->codec_type) {
            case AVMEDIA_TYPE_VIDEO:
                videoFrame = frame;
                break;
            case AVMEDIA_TYPE_AUDIO:
                audioFrame = frame;
                break;
            default:
                qWarning() << "Unsupported codec type:" << stream->codecpar->codec_type;
                return;
            }
        }
        ret = graph->apply(videoFrame);
        if (ret < 0) {
            setFilterError(QLatin1String("Could not create video filters"), ret);
            return;
        }
        ret = graph->apply(audioFrame);
        if (ret < 0) {
            setFilterError(QLatin1String("Could not create audio filters"), ret);
            return;
        }
        ret = graph->config();
        if (ret < 0) {
            setFilterError(QLatin1String("Could not configure filter grapth"), ret);
            return;
        }

        videoQueue->setFilter(new QAVVideoFilter(graph->videoInputFilters(), graph->videoOutputFilters()));
        audioQueue->setFilter(new QAVAudioFilter(graph->audioInputFilters(), graph->audioOutputFilters()));
    }

    filterGraph.reset(graph.take());
    if (error == QAVPlayer::FilterError) {
        locker.unlock();
        setMediaStatus(QAVPlayer::LoadedMedia);
    }
}

void QAVPlayerPrivate::doWait() {
    QMutexLocker lock(&waitMutex);
    if (isWaiting)
        waitCond.wait(&waitMutex);
}

static double streamDuration(const QAVStreamFrame &frame, const QAVDemuxer &demuxer)
{
    double duration = demuxer.duration();
    const double stream_duration = frame.stream().duration();
    if (stream_duration > 0 && stream_duration < duration)
        duration = stream_duration;
    return duration;
}

static bool isLastFrame(const QAVStreamFrame &frame, const QAVDemuxer &demuxer)
{
    bool result = false;
    if (!isnan(frame.duration()) && frame.duration() > 0) {
        const double requestedPos = streamDuration(frame, demuxer);
        const int frameNumber = frame.pts() / frame.duration();
        const int requestedFrameNumber = requestedPos / frame.duration();
        result = frameNumber + 1 >= requestedFrameNumber;
    }
    return result;
}

bool QAVPlayerPrivate::skipFrame(const QAVStreamFrame &frame, const QAVPacketQueue &queue, bool master)
{
    QMutexLocker locker(&positionMutex);
    bool result = pendingSeek;
    if (!pendingSeek && pendingPosition > 0) {
        const bool isQueueEOF = demuxer.eof() && queue.isEmpty();
        // Assume that no frames will be sent after this duration
        const double duration = streamDuration(frame, demuxer);
        const double requestedPos = qMin(pendingPosition, duration);
        double pos = frame.pts();
        // Show last frame if seeked to duration
        bool lastFrame = false;
        if (pendingPosition >= duration) {
            pos += frame.duration();
            // Additional check if frame rate has been changed,
            // thus last frame could be far away from duration by pts,
            // but frame number points to the latest frame.
            lastFrame = isLastFrame(frame, demuxer);
        }
        result = pos < requestedPos && !isQueueEOF && !lastFrame;
        if (master) {
            if (result)
                qCDebug(lcAVPlayer) << __FUNCTION__ << pos << "<" << requestedPos;
            else
                pendingPosition = 0;
        }
    }

    return result;
}

bool QAVPlayerPrivate::isSeeking() const
{
    QMutexLocker locker(&positionMutex);
    return pendingSeek;
}

qint64 QAVPlayerPrivate::duration() const
{
    return _duration * 1000;
}

double QAVPlayerPrivate::pts() const
{
    return !videoStreams().isEmpty() ? videoQueue->pts() : audioQueue->pts();
}

qint64 QAVPlayerPrivate::position() const
{
    QMutexLocker locker(&positionMutex);
    if (pendingSeek)
        return pendingPosition * 1000 + (pendingPosition < 0 ? duration() : 0);

    if (mediaStatus() == QAVPlayer::EndOfMedia)
        return duration();

    return pts() * 1000;
}

QAVPlayer::State QAVPlayerPrivate::state() const
{
    QMutexLocker locker(&stateMutex);
    return _state;
}

bool QAVPlayerPrivate::doStep(PendingMediaStatus status, bool hasFrame)
{
    bool result = false;
    const bool valid = hasFrame && !isSeeking() && mediaStatus() != QAVPlayer::NoMedia;
    switch (status) {
        case PlayingMedia:
            if (valid) {
                result = true;
                qCDebug(lcAVPlayer) << "Played from pos:" << position();
                Q_EMIT played(position());
                wait(false);
            }
            break;

        case PausingMedia:
            if (valid) {
                result = true;
                qCDebug(lcAVPlayer) << "Paused to pos:" << position();
                Q_EMIT paused(position());
                wait(true);
            }
            break;

        case SeekingMedia:
            if (valid) {
                result = true;
                if (mediaStatus() == QAVPlayer::EndOfMedia)
                    setMediaStatus(QAVPlayer::LoadedMedia);
                qCDebug(lcAVPlayer) << "Seeked to pos:" << position();
                Q_EMIT seeked(position());
                QAVPlayer::State currState = state();
                if (currState == QAVPlayer::PausedState || currState == QAVPlayer::StoppedState)
                    wait(true);
            }
            break;

        case StoppingMedia:
            if (mediaStatus() != QAVPlayer::NoMedia) {
                result = true;
                qCDebug(lcAVPlayer) << "Stopped to pos:" << position();
                Q_EMIT stopped(position());
                wait(true);
            }
            break;

        case SteppingMedia:
            result = isEndOfFile();
            if (valid) {
                result = true;
                qCDebug(lcAVPlayer) << "Stepped to pos:" << position();
                Q_EMIT stepped(position());
                wait(true);
            }
            break;

        case LoadingMedia:
            result = true;
            setMediaStatus(QAVPlayer::LoadedMedia);
            break;

        case EndOfMedia:
            result = true;
            setMediaStatus(QAVPlayer::EndOfMedia);
            break;

        default:
            break;
    }

    // The step is finished but queues are empty => no more frames will be sent.
    // Need to skip current status and move to next to prevent the blocking.
    if (!result && demuxer.eof() && videoQueue->isEmpty() && audioQueue->isEmpty() && !isSeeking()) {
        result = true;
        qCDebug(lcAVPlayer) << __FUNCTION__ << ": EndOfMedia -> skipping:" << status;
    }

    return result;
}

void QAVPlayerPrivate::step(bool hasFrame)
{
    QMutexLocker locker(&stateMutex);
    while (!pendingMediaStatuses.isEmpty()) {
        auto status = pendingMediaStatuses.first();
        locker.unlock();
        if (!doStep(status, hasFrame))
            break;
        locker.relock();
        if (!pendingMediaStatuses.isEmpty()) {
            pendingMediaStatuses.removeFirst();
            qCDebug(lcAVPlayer) << "Step done:" << status << ", pending" << pendingMediaStatuses;
        }
    }

    if (pendingMediaStatuses.isEmpty()) {
        videoQueue->wake(false);
        audioQueue->wake(false);
    } else {
        wait(false);
    }
}

bool QAVPlayerPrivate::doPlayStep(double refPts, bool master, QAVPacketQueue &queue, bool &sync, QAVStreamFrame &frame)
{
    doWait();
    bool hasFrame = false;
    const int ret = queue.frame(synced ? sync : synced, 1, refPts, frame);
    if (ret < 0) {
        if (ret == AVERROR(EAGAIN) || ret == AVERROR(ENOTSUP)) {
            if (ret == AVERROR(ENOTSUP))
                applyFilter(true, static_cast<QAVFrame &>(frame));
            hasFrame = isLastFrame(frame, demuxer); // Always flush events on the latest frame if filters don't have enough though
            frame = {}; // Don't send the latest frame
        } else {
            setError(QAVPlayer::FilterError, err_str(ret));
        }
    } else {
        if (frame) {
            sync = !skipFrame(frame, queue, master);
            hasFrame = sync;
        }
    }
    if (master)
        step(hasFrame);
    return hasFrame;
}

void QAVPlayerPrivate::doPlayVideo()
{
    videoQueue->setFrameRate(demuxer.videoFrameRate());
    bool sync = true;
    QAVVideoFrame frame;
    const bool master = true;

    while (!quit) {
        if (doPlayStep(audioQueue->pts(), master, *videoQueue, sync, frame)) {
            if (frame)
                Q_EMIT videoFrame(frame);
        }
    }

    videoQueue->clear();
    setMediaStatus(QAVPlayer::NoMedia);
    qCDebug(lcAVPlayer) << __FUNCTION__ << "finished";
}

void QAVPlayerPrivate::doPlayAudio()
{
    const bool master = videoStreams().isEmpty();
    const double ref = -1;
    bool sync = true;
    QAVAudioFrame frame;

    while (!quit) {
        if (doPlayStep(ref, master, *audioQueue, sync, frame)) {
            if (frame)
                Q_EMIT audioFrame(frame);
        }
    }

    audioQueue->clear();
    if (master)
        setMediaStatus(QAVPlayer::NoMedia);
    qCDebug(lcAVPlayer) << __FUNCTION__ << "finished";
}

#endif

bool QAVQueueClock::sync(bool sync, double pts, double speed, double master)
{
    double delay = pts - prevPts;
    if (isnan(delay) || delay <= 0 || delay > maxFrameDuration)
        delay = frameRate;

    if (master > 0) {
        double diff = pts - master;
        double sync_threshold = qMax(minThreshold, qMin(maxThreshold, delay));
        if (!isnan(diff) && fabs(diff) < maxFrameDuration) {
            if (diff <= -sync_threshold)
                delay = qMax(0.0, delay + diff);
            else if (diff >= sync_threshold && delay > frameDuplicationThreshold)
                delay = delay + diff;
            else if (diff >= sync_threshold)
                delay = 2 * delay;
        }
    }

    delay /= speed;
    double time = av_gettime_relative() / 1000000.0;
    if (sync) {
        if (time < frameTimer + delay) {
            double remaining_time = qMin(frameTimer + delay - time, refreshRate);
            av_usleep((int64_t)(remaining_time * 1000000.0));
            return false;
        }
    }

    prevPts = pts;
    frameTimer += delay;
    if ((delay > 0 && time - frameTimer > maxThreshold) || !sync)
        frameTimer = time;

    return true;
}

void QAVPacketQueue::enqueue(const QAVPacket &packet)
{
    QMutexLocker locker(&m_mutex);
    m_packets.append(packet);
    m_bytes += packet.packet()->size + sizeof(packet);
    m_duration += packet.duration();
    m_consumerWaiter.wakeAll();
    m_abort = false;
    m_waitingForPackets = false;
}

QAVPacket QAVPacketQueue::dequeue()
{
    QMutexLocker locker(&m_mutex);
    if (m_packets.isEmpty()) {
        m_producerWaiter.wakeAll();
        if (!m_abort && !m_wake) {
            m_waitingForPackets = true;
            m_consumerWaiter.wait(&m_mutex);
            m_waitingForPackets = false;
        }
    }
    if (m_packets.isEmpty())
        return {};

    auto packet = m_packets.takeFirst();
    m_bytes -= packet.packet()->size + sizeof(packet);
    m_duration -= packet.duration();
    return packet;
}
