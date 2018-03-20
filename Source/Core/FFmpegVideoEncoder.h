#ifndef FFMPEGVIDEOENCODER_H
#define FFMPEGVIDEOENCODER_H

#include <QObject>
#include <QPair>
#include <QString>
#include <QList>
#include <functional>

#include "FFmpeg_Glue.h"

class FFmpegVideoEncoder : public QObject
{
    Q_OBJECT
public:
    typedef QPair<QString, QString> MetadataEntry;
    typedef QList<MetadataEntry> Metadata;

    explicit FFmpegVideoEncoder(QObject *parent = nullptr);
    void setMetadata(const QList<MetadataEntry>& metadata);
signals:

public slots:
    void makeVideo(const QString& video, int width, int height, int bitrate, int num, int den, std::function<AVPacket* ()> getPacket, const QByteArray& attachment, const QString& attachmentName);

private:
    Metadata m_metadata;
};

#endif // FFMPEGVIDEOENCODER_H
