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

void FFmpegVideoEncoder::makeVideo(const QString &video, int width, int height, int bitrate, std::function<AVPacket* ()> getPacket,
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
    auto codec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
    if(!codec) {
        qDebug() << "Error resolving AV_CODEC_ID_MJPEG codec";
        return;
    }

    AVStream* videoStream = avformat_new_stream(oc, codec);
    if (!videoStream) {
        qDebug() << "Could not allocate stream\n";
        return;
    }

    videoStream->id = oc->nb_streams-1;

    auto videoEncCtx = avcodec_alloc_context3(codec);
    if (!videoEncCtx) {
        qDebug() << "Could not alloc an encoding context\n";
        return;
    }

    videoEncCtx->bit_rate = bitrate;
    /* Resolution must be a multiple of two. */
    videoEncCtx->width    = width;
    videoEncCtx->height   = height;

    /* timebase: This is the fundamental unit of time (in seconds) in terms
     * of which frame timestamps are represented. For fixed-fps content,
     * timebase should be 1/framerate and timestamp increments should be
     * identical to 1. */

    videoStream->time_base.num = 1;
    videoStream->time_base.den = 1;
    videoEncCtx->time_base = videoStream->time_base;

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

    /* Some formats want stream headers to be separate. */
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        videoEncCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    int ret =  0;

    ret = avcodec_open2(videoEncCtx, codec, NULL);
    if(ret < 0) {
        char errbuf[255];
        qDebug() << "Could not open codec: " << av_make_error_string(errbuf, sizeof errbuf, ret) << "\n";
        return;
    }

    /* copy the stream parameters to the muxer */
    ret = avcodec_parameters_from_context(videoStream->codecpar, videoEncCtx);

    av_dump_format(oc, 0, filename.c_str(), 1);

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
    attachementEncCtx->extradata = reinterpret_cast<uint8_t*> (const_cast<char*> (attachment.data()));
    attachementEncCtx->extradata_size = attachment.size();
    attachementEncCtx->subtitle_header = nullptr;
    attachementEncCtx->subtitle_header_size = 0;

    /* copy the stream parameters to the muxer */
    ret = avcodec_parameters_from_context(attachmentStream->codecpar, attachementEncCtx);

    auto attachmentFileName = attachmentName.toStdString();
    const char* p = strrchr(attachmentFileName.c_str(), '/');
    av_dict_set(&attachmentStream->metadata, "filename", (p && *p) ? p + 1 : attachmentFileName.c_str(), AV_DICT_DONT_OVERWRITE);
#endif //

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

        // packet->flags |= AV_PKT_FLAG_KEY;
        newPacket.stream_index = videoStream->index;

        /*
        packet->pts = i;
        packet->dts = i;
        */

        ++i;

        // av_write_frame(oc, packet);
        av_interleaved_write_frame(oc, &newPacket);
        av_packet_unref(&newPacket);
    }

    struct Local {
        static int writeFrame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt) {
            /* rescale output packet timestamp values from codec to stream timebase */
            av_packet_rescale_ts(pkt, *time_base, st->time_base);
            pkt->stream_index = st->index;

            /* Write the compressed frame to the media file. */
            // log_packet(fmt_ctx, pkt);

            return av_interleaved_write_frame(fmt_ctx, pkt);
        }
        static void flushStream(AVFormatContext* oc, AVCodecContext* enc, AVStream* st) {
            int ret;
                AVPacket pkt = { 0 };
                av_init_packet(&pkt);

                ret = avcodec_send_frame(enc, NULL);
                if (ret < 0) {
                    char errbuf[255];
                    qDebug() << "Error encoding video frame: " << av_make_error_string(errbuf, sizeof errbuf, ret);
                    return;
                }

                do {
                    ret = avcodec_receive_packet(enc, &pkt);
                    if (ret == 0) {
                        ret = writeFrame(oc, &enc->time_base, st, &pkt);
                        if (ret < 0) {
                            char errbuf[255];
                            fprintf(stderr, "Error while writing video frame: %s\n", av_make_error_string(errbuf, sizeof errbuf, ret));
                            return;
                        }
                    }
                    else if (ret == AVERROR(EINVAL)) {
                        char errbuf[255];
                        fprintf(stderr, "Error while getting video packet: %s\n", av_make_error_string(errbuf, sizeof errbuf, ret));
                        return;
                    }
                }
                while (ret != AVERROR_EOF) ;
            }
    };

    Local::flushStream(oc, videoEncCtx, videoStream);
    Local::flushStream(oc, attachementEncCtx, attachmentStream);

    //Write file trailer
    av_write_trailer(oc);

    // dispoe video encoder
    avcodec_free_context(&videoEncCtx);

    // dispose attachment encoder
    // can't do avcodec_free_context as we didn't allocate 'extradata' in ffmpeg way, so shouldn't dispose it
    // ... so just execute internals for avcodec_free_context one-by-one

    avcodec_close(attachementEncCtx);
    // av_freep(&avctx->extradata); // do not touch 'extradata' !
    av_freep(&attachementEncCtx->subtitle_header);
    av_freep(&attachementEncCtx->intra_matrix);
    av_freep(&attachementEncCtx->inter_matrix);
    av_freep(&attachementEncCtx->rc_override);
    av_freep(&attachementEncCtx);

    /* Close the output file. */
    avio_closep(&oc->pb);

    /* free the stream */
    avformat_free_context(oc);
}

