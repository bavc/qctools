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

    auto pOutputFormat = av_guess_format(NULL, filename.c_str(), NULL);
    if (!pOutputFormat) {
       printf("Could not deduce output format from file extension: using mkv.\n");
       pOutputFormat = av_guess_format("mkv", NULL, NULL);
    }

    std::unique_ptr<AVFormatContext, std::function<void (AVFormatContext *)>> pFormatCtx(avformat_alloc_context(), avformat_free_context);
    if(!pFormatCtx)
    {
       qDebug() << "Error allocating format context\n";
       return;
    }

    pFormatCtx->oformat = pOutputFormat;
    snprintf(pFormatCtx->filename, sizeof(pFormatCtx->filename), "%s", filename.c_str());

    // Add the AV_CODEC_ID_MJPEG video stream
    auto codec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
    if(!codec)
    {
        qDebug() << "Error resolving AV_CODEC_ID_MJPEG codec";
        return;
    }

    std::unique_ptr<AVStream, std::function<void (void*)>> pVideoStream(avformat_new_stream(pFormatCtx.get(), codec), av_freep);
    if(!pVideoStream )
    {
       qDebug() << "Could not allocate stream\n";
       return;
    }

    pVideoStream->id = pFormatCtx->nb_streams - 1;

    auto c = avcodec_alloc_context3(codec);
    if (!c) {
        fprintf(stderr, "Could not alloc an encoding context\n");
        exit(1);
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
    pVideoStream->time_base.num = 1;
    pVideoStream->time_base.den = 1;
    c->time_base       = pVideoStream->time_base;

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
    if (pFormatCtx->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    int ret = avcodec_open2(c, codec, NULL);
    if(ret < 0)
    {
        char errbuf[255];
        qDebug() << "Could not open codec: " << av_make_error_string(errbuf, sizeof errbuf, ret) << "\n";
        return;
    }

    std::unique_ptr<AVFrame, std::function<void (AVFrame*)>> frame(av_frame_alloc(), [&](AVFrame* frame) { av_frame_free(&frame); });

    frame->format = AV_PIX_FMT_RGB24;
    frame->width  = c->width;
    frame->height = c->height;

    av_image_alloc(frame->data, frame->linesize, frame->width, frame->height, c->pix_fmt, 32);

    auto f = fopen(pFormatCtx->filename, "wb");
    if (!f) {
        qDebug() << "Could not open" << pFormatCtx->filename << "\n";
        return;
    }

    int got_output = 0;

    while(true)  {
        if(!getPacket)
            break;

        auto packet = getPacket();
        if(!packet)
            break;

        fwrite(packet->data, 1, packet->size, f);

        /* prepare a dummy image */
        /* Y */
        /*
        for (int y = 0; y < c->height; y++) {
            for (int x = 0; x < c->width; x++) {
                frame->data[0][y * frame->linesize[0] + x] = x + y + i * 3;
            }
        }
        */

        /* Cb and Cr */
        /*
        for (int y = 0; y < c->height/2; y++) {
            for (int x = 0; x < c->width/2; x++) {
                frame->data[1][y * frame->linesize[1] + x] = 128 + y + i * 2;
                frame->data[2][y * frame->linesize[2] + x] = 64 + x + i * 5;
            }
        }

        /* Which frame is it ? */
        /*
        frame->pts = i;
        */

        /* encode the image */
        /*
        auto ret = avcodec_encode_video2(c, &pkt, frame.get(), &got_output);
        if (ret < 0) {
            qDebug() << "Error encoding frame\n";
            return;
        }
        */

        if (got_output) {
            fwrite(packet->data, 1, packet->size, f);
        }
    }

    /* get the delayed frames */
    AVPacket pkt;
    av_init_packet(&pkt);

    pkt.data = NULL;    // packet data will be allocated by the encoder
    pkt.size = 0;

    got_output = 1;
    ret = avcodec_encode_video2(c, &pkt, NULL, &got_output);
    if (ret < 0) {
        qDebug() << "Error encoding frame\n";
        return;
    }

    if (got_output) {
        fwrite(pkt.data, 1, pkt.size, f);
        av_packet_unref(&pkt);
    }

    uint8_t endcode[] = { 0, 0, 1, 0xb7 };

    /* add sequence end code to have a real mpeg file */
    fwrite(endcode, 1, sizeof(endcode), f);
    fclose(f);
}

