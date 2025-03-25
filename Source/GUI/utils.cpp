#ifndef QT_AVPLAYER_MULTIMEDIA
#include "GUI/utils.h"

#include <QVideoFrame>
#include <QVideoFrameFormat>
#include <QtAVPlayer/qavplayer.h>

//---------------------------------------------------------------------------
QVideoFrame QAVVideoFrame2QVideoFrame(const QAVVideoFrame& frame)
{
    const QAVVideoFrame* in = &frame;
    QAVVideoFrame convertedFrame;

    QVideoFrameFormat::PixelFormat pixelFormat;
    switch (in->frame()->format)
    {
    case AV_PIX_FMT_RGB32:
        pixelFormat = QVideoFrameFormat::Format_BGRA8888;
        break;
    default:
        // TODO: Add more supported formats instead of converting
        convertedFrame = in->convertTo(AV_PIX_FMT_RGB32);
        in = &convertedFrame;
        pixelFormat = QVideoFrameFormat::Format_BGRA8888;
        break;
    }

    QVideoFrameFormat format(in->size(), pixelFormat);
    QVideoFrame videoFrame(format);
    if (!videoFrame.map(QVideoFrame::WriteOnly))
        return QVideoFrame();

    for (int plane = 0; plane < 4; plane++)
    {
        if (!videoFrame.mappedBytes(plane))
            continue;

        QAVVideoFrame::MapData mappedData = in->map();
        int bufferSize = qMin(in->size().height() * mappedData.bytesPerLine[plane], videoFrame.mappedBytes(plane));
        memcpy(videoFrame.bits(plane), mappedData.data[plane], bufferSize);
    }

    videoFrame.unmap();

    return videoFrame;
}
#endif // QT_AVPLAYER_MULTIMEDIA
