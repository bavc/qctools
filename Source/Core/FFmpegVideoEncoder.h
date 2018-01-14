#ifndef FFMPEGVIDEOENCODER_H
#define FFMPEGVIDEOENCODER_H

#include <QObject>
#include <functional>

#include "FFmpeg_Glue.h"

class FFmpegVideoEncoder : public QObject
{
    Q_OBJECT
public:
    explicit FFmpegVideoEncoder(QObject *parent = nullptr);

signals:

public slots:
    void makeVideo(const QString& video, int width, int height, int bitrate, std::function<AVPacket* ()> getPacket, const QByteArray& attachment, const QString& attachmentName);
};

#endif // FFMPEGVIDEOENCODER_H
