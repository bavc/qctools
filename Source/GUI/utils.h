#ifndef UTILS_H
#define UTILS_H

#ifdef QT_AVPLAYER_MULTIMEDIA
#define QAVV_QV(x) x
#else
class QVideoFrame;
class QAVVideoFrame;
QVideoFrame QAVVideoFrame2QVideoFrame(const QAVVideoFrame& frame);
#define QAVV_QV(x) QAVVideoFrame2QVideoFrame(x)
#endif // QT_AVPLAYER_MULTIMEDIA

#endif // UTILS_H
