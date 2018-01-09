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

#include <functional>
#include <QDebug>

FFmpegVideoEncoder::FFmpegVideoEncoder(QObject *parent) : QObject(parent)
{
    av_register_all();
}

void FFmpegVideoEncoder::makeVideo(const QString &video, int width, int height, int bitrate, std::function<AVPacket* ()> getPacket)
{
    auto filename = video.toStdString();
    AVFormatContext *oc;

    /* allocate the output media context */
    avformat_alloc_output_context2(&oc, NULL, NULL, filename.c_str());
    if (!oc) {
        qDebug() << "Could not deduce output format from file extension: using MKV.\n";
        avformat_alloc_output_context2(&oc, NULL, "mkv", filename.c_str());
    }

    // Add the AV_CODEC_ID_MJPEG video stream
    auto codec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
    if(!codec)
    {
        qDebug() << "Error resolving AV_CODEC_ID_MJPEG codec";
        return;
    }

    AVStream* videoStream = avformat_new_stream(oc, codec);
    if (!videoStream) {
        qDebug() << "Could not allocate stream\n";
        return;
    }

    videoStream->id = oc->nb_streams-1;

    auto c = avcodec_alloc_context3(codec);
    if (!c) {
        qDebug() << "Could not alloc an encoding context\n";
        return;
    }

    auto enc = c;

    c->codec_id = AV_CODEC_ID_MJPEG;
    c->codec_type = AVMEDIA_TYPE_VIDEO;

    c->bit_rate = bitrate;
    /* Resolution must be a multiple of two. */
    c->width    = width;
    c->height   = height;

    /* timebase: This is the fundamental unit of time (in seconds) in terms
     * of which frame timestamps are represented. For fixed-fps content,
     * timebase should be 1/framerate and timestamp increments should be
     * identical to 1. */

    videoStream->time_base.num = 1;
    videoStream->time_base.den = 1;
    c->time_base = videoStream->time_base;

    c->gop_size      = 1; /* emit one intra frame every twelve frames at most */
    c->pix_fmt       = AV_PIX_FMT_YUVJ422P;
    c->max_b_frames  = 0;

    if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
        /* just for testing, we also add B-frames */
        c->max_b_frames = 2;
    }
    if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
        /* Needed to avoid using macroblocks in which some coeffs overflow.
         * This does not happen with normal video, it just happens here as
         * the motion of the chroma plane does not match the luma plane. */
        c->mb_decision = 2;
    }

    /* Some formats want stream headers to be separate. */
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    int ret = avcodec_open2(c, codec, NULL);
    if(ret < 0)
    {
        char errbuf[255];
        qDebug() << "Could not open codec: " << av_make_error_string(errbuf, sizeof errbuf, ret) << "\n";
        return;
    }

    /* copy the stream parameters to the muxer */
    ret = avcodec_parameters_from_context(videoStream->codecpar, c);

    av_dump_format(oc, 0, filename.c_str(), 1);

    /* open the output file, if needed */
    ret = avio_open(&oc->pb, filename.c_str(), AVIO_FLAG_WRITE);
    if (ret < 0) {
        char errbuf[255];
        fprintf(stderr, "Could not open '%s': %s\n", filename.c_str(), av_make_error_string(errbuf, sizeof errbuf, ret));
        return;
    }

    /* Write the stream header, if any. */
    ret = avformat_write_header(oc, NULL);

    int i = 0;

    while(true)  {
        if(!getPacket)
            break;

        AVPacket* packet = getPacket();
        if(!packet)
            break;

        // packet->flags |= AV_PKT_FLAG_KEY;
        packet->stream_index = videoStream->index;

        /*
        packet->pts = i;
        packet->dts = i;
        */

        ++i;
        // av_write_frame(oc, packet);
        av_interleaved_write_frame(oc, packet);
    }

    /* free the stream */
    avformat_free_context(oc);
}

