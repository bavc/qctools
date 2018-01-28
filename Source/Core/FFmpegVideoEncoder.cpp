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
#include <functional>
#include <QDebug>

FFmpegVideoEncoder::FFmpegVideoEncoder(QObject *parent) : QObject(parent)
{
    av_register_all();
}

void FFmpegVideoEncoder::makeVideo(const QString &video, int width, int height, int bitrate, int num, int den, std::function<AVPacket* ()> getPacket,
                                   const QByteArray& attachment, const QString& attachmentName)
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
    auto videoCodec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
    if(!videoCodec) {
        qDebug() << "Error resolving AV_CODEC_ID_MJPEG codec";
        return;
    }

    auto videoEncCtx = avcodec_alloc_context3(videoCodec);
    if (!videoEncCtx) {
        qDebug() << "Could not alloc an encoding context\n";
        return;
    }

    /* Resolution must be a multiple of two. */
    videoEncCtx->width    = width;
    videoEncCtx->height   = height;
    videoEncCtx->bit_rate = bitrate;

    /* timebase: This is the fundamental unit of time (in seconds) in terms
     * of which frame timestamps are represented. For fixed-fps content,
     * timebase should be 1/framerate and timestamp increments should be
     * identical to 1. */

    videoEncCtx->time_base.num = num;
    videoEncCtx->time_base.den = den;

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

    ///////////////////////////////
    /// attachements

#if 1
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

    attachementEncCtx->codec_type = AVMEDIA_TYPE_ATTACHMENT;
    attachementEncCtx->codec_id = AV_CODEC_ID_BIN_DATA;
    attachementEncCtx->extradata_size = attachment.size();
    attachementEncCtx->extradata = static_cast<uint8_t*> (av_malloc(attachementEncCtx->extradata_size));
    memcpy(attachementEncCtx->extradata, attachment.data(), attachementEncCtx->extradata_size);

    /* copy the stream parameters to the muxer */
    ret = avcodec_parameters_from_context(attachmentStream->codecpar, attachementEncCtx);

    auto attachmentFileName = attachmentName.toStdString();
    const char* p = strrchr(attachmentFileName.c_str(), '/');
    av_dict_set(&attachmentStream->metadata, "filename", (p && *p) ? p + 1 : attachmentFileName.c_str(), AV_DICT_DONT_OVERWRITE);
#endif //

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

    AVPacket newPacket;

    while(true)  {
        if(!getPacket)
            break;

        AVPacket* packet = getPacket();
        if(!packet)
            break;

        av_init_packet(&newPacket);
        av_packet_ref(&newPacket, packet);

        newPacket.stream_index = videoStream->index;

        ++i;

        av_interleaved_write_frame(oc, &newPacket);
        av_packet_unref(&newPacket);
    }

    //Write file trailer
    av_write_trailer(oc);

    // dispose video encoder
    avcodec_free_context(&videoEncCtx);

    // dispose attachment encoder
    avcodec_free_context(&attachementEncCtx);

    /* Close the output file. */
    avio_closep(&oc->pb);

    /* free the stream */
    avformat_free_context(oc);
}

