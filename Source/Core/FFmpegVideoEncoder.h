#ifndef FFMPEGVIDEOENCODER_H
#define FFMPEGVIDEOENCODER_H

#include <QObject>
#include <QPair>
#include <QString>
#include <QList>
#include <functional>
#include <memory>

extern "C" {
#include <libavcodec/codec_id.h>
#include <libavformat/avformat.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>

#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>

#include <libavutil/pixfmt.h>
#include <libavutil/imgutils.h>
#include <libavutil/ffversion.h>
}

class FFmpegVideoEncoder : public QObject
{
    Q_OBJECT
public:
    typedef QPair<QString, QString> MetadataEntry;
    typedef QList<MetadataEntry> Metadata;

    struct Source {
        int width;
        int height;
        int bitrate = { 0 };
        int num;
        int den;
        Metadata metadata;
        std::function<std::shared_ptr<AVPacket>()> getPacket;
    };

    explicit FFmpegVideoEncoder(QObject *parent = nullptr);
    void setMetadata(const QList<MetadataEntry>& metadata);
signals:

public slots:
    void makeVideo(const QString& video, const QVector<Source>& sources, const QByteArray& attachment, const QString& attachmentName);

private:
    Metadata m_metadata;
};

#endif // FFMPEGVIDEOENCODER_H
