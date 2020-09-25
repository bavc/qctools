#include "FFmpegVideoEncoder.h"

extern "C"
{
#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#endif

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>

#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>

#include <libavutil/imgutils.h>
#include <libavutil/ffversion.h>

#ifndef WITH_SYSTEM_FFMPEG
#include <config.h>
#else
#include <ctime>
#endif
}

#include <cassert>
#include <memory>
#include <functional>
#include <QDebug>
#include <QFileInfo>

FFmpegVideoEncoder::FFmpegVideoEncoder(QObject *parent) : QObject(parent)
{
    av_register_all();
}

void FFmpegVideoEncoder::setMetadata(const Metadata &metadata)
{
    m_metadata = metadata;
}

void FFmpegVideoEncoder::makeVideo(const QString &video, const QVector<Source>& sources,
                                   const QByteArray& attachment, const QString& attachmentName)
{
    struct ContextDeleter {
        void operator()(AVCodecContext* context) {
            avcodec_free_context(&context);
        }
    };

    std::vector<std::unique_ptr<AVCodecContext, ContextDeleter>> contexts;
    QVector<AVStream*> streams;

    auto filename = video.toStdString();
    AVFormatContext *oc;

    /* allocate the output media context */
    avformat_alloc_output_context2(&oc, NULL, NULL, filename.c_str());
    if (!oc) {
        qDebug() << "Could not deduce output format from file extension: using MKV.\n";
        avformat_alloc_output_context2(&oc, NULL, "mkv", filename.c_str());
    }

    // Add the AV_CODEC_ID_MJPEG video stream
    auto videoCodec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
    if(!videoCodec) {
        qDebug() << "Error resolving AV_CODEC_ID_MJPEG codec";
        return;
    }

    for(auto & source : sources) {
        auto videoEncCtx = avcodec_alloc_context3(videoCodec);
        if (!videoEncCtx) {
            qDebug() << "Could not alloc an encoding context\n";
            return;
        }

        std::unique_ptr<AVCodecContext, ContextDeleter> videoEncCtxPtr(videoEncCtx, ContextDeleter());
        contexts.push_back(std::move(videoEncCtxPtr));

        /* Resolution must be a multiple of two. */
        videoEncCtx->width    = source.width;
        videoEncCtx->height   = source.height;
        videoEncCtx->bit_rate = source.bitrate;

        /* timebase: This is the fundamental unit of time (in seconds) in terms
         * of which frame timestamps are represented. For fixed-fps content,
         * timebase should be 1/framerate and timestamp increments should be
         * identical to 1. */

        videoEncCtx->time_base.num = source.num;
        videoEncCtx->time_base.den = source.den;

        videoEncCtx->gop_size      = 1; /* emit one intra frame every twelve frames at most */
        videoEncCtx->pix_fmt       = AV_PIX_FMT_YUVJ422P;
        videoEncCtx->max_b_frames  = 0;

        if (videoEncCtx->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
            /* just for testing, we also add B-frames */
            videoEncCtx->max_b_frames = 2;
        }
        if (videoEncCtx->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
            /* Needed to avoid using macroblocks in which some coeffs overflow.
             * This does not happen with normal video, it just happens here as
             * the motion of the chroma plane does not match the luma plane. */
            videoEncCtx->mb_decision = 2;
        }

        AVStream* videoStream = avformat_new_stream(oc, videoCodec);
        if (!videoStream) {
            qDebug() << "Could not allocate stream\n";
            return;
        }

        // this seems to be needed for certain codecs, as otherwise they don't have relevant options set
        avcodec_get_context_defaults3(videoStream->codec, videoCodec);

        videoStream->id = oc->nb_streams-1;

        /* copy the stream parameters to the muxer */
        int ret = avcodec_parameters_from_context(videoStream->codecpar, videoEncCtx);
        if (ret < 0 ) {
            qDebug() << "error on avcodec_parameters_from_context\n";
            return;
        }

        videoStream->avg_frame_rate.den = videoEncCtx->time_base.num;
        videoStream->avg_frame_rate.num = videoEncCtx->time_base.den;

        ret = avcodec_open2(videoEncCtx, videoCodec, NULL);
        if(ret < 0) {
            char errbuf[255];
            qDebug() << "Could not open codec: " << av_make_error_string(errbuf, sizeof errbuf, ret) << "\n";
            return;
        }

        /* Some formats want stream headers to be separate. */
        if (oc->oformat->flags & AVFMT_GLOBALHEADER)
            videoEncCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

        streams.push_back(videoStream);
    }

    ///////////////////////////////
    /// attachements

    AVStream* attachmentStream = avformat_new_stream(oc, NULL);
    if (!attachmentStream) {
        qDebug() << "Could not allocate attachment stream\n";
        return;
    }

    attachmentStream->id = oc->nb_streams-1;

    AVCodecContext* attachementEncCtx = avcodec_alloc_context3(NULL);
    if (!attachementEncCtx) {
        qDebug() << "Error allocating the encoding context.\n";
        return;
    }

    std::unique_ptr<AVCodecContext, ContextDeleter> attachementEncCtxPtr(attachementEncCtx, ContextDeleter());
    contexts.push_back(std::move(attachementEncCtxPtr));

    attachementEncCtx->codec_type = AVMEDIA_TYPE_ATTACHMENT;
    attachementEncCtx->codec_id = AV_CODEC_ID_BIN_DATA;
    attachementEncCtx->extradata_size = attachment.size();
    attachementEncCtx->extradata = static_cast<uint8_t*> (av_malloc(attachementEncCtx->extradata_size));
    memcpy(attachementEncCtx->extradata, attachment.data(), attachementEncCtx->extradata_size);

    /* copy the stream parameters to the muxer */
    int ret = avcodec_parameters_from_context(attachmentStream->codecpar, attachementEncCtx);

    auto attachmentFileName = attachmentName.toStdString();
    const char* p = strrchr(attachmentFileName.c_str(), '/');
    av_dict_set(&attachmentStream->metadata, "filename", (p && *p) ? p + 1 : attachmentFileName.c_str(), AV_DICT_DONT_OVERWRITE);
    av_dict_set(&attachmentStream->metadata, "mimetype", "application/x-gzip", AV_DICT_DONT_OVERWRITE);

    av_dump_format(oc, 0, filename.c_str(), 1);

    /* open the output file, if needed */
    ret = avio_open(&oc->pb, filename.c_str(), AVIO_FLAG_WRITE);
    if (ret < 0) {
        char errbuf[255];
        fprintf(stderr, "Could not open '%s': %s\n", filename.c_str(), av_make_error_string(errbuf, sizeof errbuf, ret));
        return;
    }

    for(auto & entry : m_metadata) {
        auto res = av_dict_set(&oc->metadata, entry.first.toStdString().c_str(), entry.second.toStdString().c_str(), 0);
        qDebug() << "global metadata: " << res;
    }

    for(auto i = 0; i < streams.length(); ++i) {
        auto source = sources[i];
        auto stream = streams[i];

        for(auto & entry : source.metadata) {
            qDebug() << "setting stream metadata for stream: " << i << "key: " << entry.first << "value: " << entry.second;

            auto res = av_dict_set(&stream->metadata, entry.first.toStdString().c_str(), entry.second.toStdString().c_str(), 0);
            qDebug() << "stream metadata: " << res;
        }
    }

    /* Write the stream header, if any. */
    ret = avformat_write_header(oc, NULL);

    AVPacket newPacket;

    for(auto i = 0; i < streams.length(); ++i)
    {
        auto source = sources[i];
        auto stream = streams[i];

        std::shared_ptr<AVPacket> packet;
        while(source.getPacket && (packet = source.getPacket())) {

            av_init_packet(&newPacket);
            av_packet_ref(&newPacket, packet.get());

            newPacket.stream_index = stream->index;

            av_interleaved_write_frame(oc, &newPacket);
            av_packet_unref(&newPacket);
        }
    }

    //Write file trailer
    av_write_trailer(oc);

    /* Close the output file. */
    avio_closep(&oc->pb);

    /* free the stream */
    avformat_free_context(oc);
}

