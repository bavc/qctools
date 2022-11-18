/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include "Core/FFmpeg_Glue.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "Core/VideoStats.h"
#include "Core/AudioStats.h"
#include "Core/StreamsStats.h"
#include "Core/FormatStats.h"

#include <QXmlStreamReader>
#include <QDebug>

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

#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <cfloat>
//---------------------------------------------------------------------------

void LibsVersion_Inject(stringstream &LibsVersion, const char* Name, int Value)
{
    LibsVersion<<' ' << Name << "=\"" << Value << '\"';
}
void LibsVersion_Inject(stringstream &LibsVersion, const char* Name, const char* Value)
{
    LibsVersion<<' ' << Name << "=\"" << Value << '\"';
}

#ifndef WITH_SYSTEM_FFMPEG
#define LIBSVERSION(libname, LIBNAME)                                               \
    if (CONFIG_##LIBNAME)                                                           \
    {                                                                               \
        unsigned int version = libname##_version();                                 \
        LibsVersion<<"        <library_version";                                    \
        LibsVersion_Inject(LibsVersion, "name",    "lib" #libname);                 \
        LibsVersion_Inject(LibsVersion, "major",   LIB##LIBNAME##_VERSION_MAJOR);   \
        LibsVersion_Inject(LibsVersion, "minor",   LIB##LIBNAME##_VERSION_MINOR);   \
        LibsVersion_Inject(LibsVersion, "micro",   LIB##LIBNAME##_VERSION_MICRO);   \
        LibsVersion_Inject(LibsVersion, "version", version);                        \
        LibsVersion_Inject(LibsVersion, "ident",   LIB##LIBNAME##_IDENT);           \
        LibsVersion<<"/>\n";                                                        \
    }
#else
#define LIBSVERSION(libname, LIBNAME)                                               \
    do {                                                                            \
        unsigned int version = libname##_version();                                 \
        LibsVersion<<"        <library_version";                                    \
        LibsVersion_Inject(LibsVersion, "name",    "lib" #libname);                 \
        LibsVersion_Inject(LibsVersion, "major",   LIB##LIBNAME##_VERSION_MAJOR);   \
        LibsVersion_Inject(LibsVersion, "minor",   LIB##LIBNAME##_VERSION_MINOR);   \
        LibsVersion_Inject(LibsVersion, "micro",   LIB##LIBNAME##_VERSION_MICRO);   \
        LibsVersion_Inject(LibsVersion, "version", version);                        \
        LibsVersion_Inject(LibsVersion, "ident",   LIB##LIBNAME##_IDENT);           \
        LibsVersion<<"/>\n";                                                        \
    } while (0)
#endif

//***************************************************************************
// Error manager of FFmpeg
//***************************************************************************

//---------------------------------------------------------------------------
/*
string Errors;
static void avlog_cb(void *, int level, const char * szFmt, va_list varg)
{
    char Temp[1000];
    vsnprintf(Temp, 1000, szFmt, varg);
    Errors+=Temp;
}
//*/

//***************************************************************************
// inputdata
//***************************************************************************

FFmpeg_Glue::inputdata::inputdata()
    :
    // In
    Enabled(true),

    // FFmpeg pointers - Input
    Type(-1),
    Stream(NULL),
    DecodedFrame(NULL),

    // Status
    FramePos(0),

    // General information
    FrameCount(0),
    FirstTimeStamp(DBL_MAX),
    Duration(0),

    // Cache
    FramesCache(NULL),
    FramesCache_Default(NULL)
{
}

FFmpeg_Glue::inputdata::~inputdata()
{
    // FFmpeg pointers - Input
    if (Stream)
        avcodec_close(Stream->codec);

    // FramesCache
    if (FramesCache)
    {
        for (size_t Pos = 0; Pos < FramesCache->size(); Pos++)
        {
            av_frame_free(&((*FramesCache)[Pos]));
        }
        delete FramesCache;
    }
}

//***************************************************************************
// outputdata
//***************************************************************************

FFmpeg_Glue::outputdata::outputdata(int index)
    : index(index),
    // In
    Enabled(true),

    // FFmpeg pointers - Input
    Stream(NULL),
    DecodedFrame(NULL),

    // FFmpeg pointers - Filter
    FilterGraph(NULL),
    FilterGraph_Source_Context(NULL),
    FilterGraph_Sink_Context(NULL),
    FilteredFrame(NULL),

    // FFmpeg pointers - Scale
    ScaleContext(NULL),
    Scale_OutputPixelFormat(AV_PIX_FMT_YUVJ420P),
    ScaledFrame(NULL),

    // FFmpeg pointers - Output
    Output_CodecContext(NULL),
    Output_PixelFormat(AV_PIX_FMT_YUVJ420P),
    Output_CodecID(AV_CODEC_ID_MJPEG),

    // Out
    OutputMethod(Output_None),
    Thumbnails_Modulo(1),
    Stats(NULL),
    
    // Helpers
    Width(0),
    Height(0),

    // Status
    FramePos(0)
{
}

//---------------------------------------------------------------------------
FFmpeg_Glue::outputdata::~outputdata()
{
    // Images
    image.free();

    Thumbnails.clear();

    if (Output_CodecContext)
        avcodec_free_context(&Output_CodecContext);

    // FFmpeg pointers - Scale
    if (ScaledFrame)
        ScaledFrame.reset();

    if (ScaleContext)
        sws_freeContext(ScaleContext);

    // FFmpeg pointers - Filter
    if (FilterGraph)
        avfilter_graph_free(&FilterGraph);
}

//---------------------------------------------------------------------------
void FFmpeg_Glue::outputdata::Process(AVFrame* DecodedFrame_)
{
    struct NoDeleter {
        static void free(AVFrame* frame) {
            Q_UNUSED(frame)
        }
    };

    DecodedFrame = AVFramePtr(DecodedFrame_, NoDeleter::free);
    OutputFrame = DecodedFrame;

    //Filtering
    ApplyFilter(OutputFrame);

    if(FilteredFrame)
    {
        ++FramePos; // only increase frame position if filter succeed, otherwise it breaks audio filters
        OutputFrame = FilteredFrame;
    }

    // Stats
    if (Stats && FilteredFrame && !Filter.empty())
    {
        Stats->TimeStampFromFrame(FilteredFrame.get(), FramePos-1);
        Stats->StatsFromFrame(FilteredFrame.get(), Stream->codec->width, Stream->codec->height);
    }

    // Scale
    ApplyScale(OutputFrame);

    if(ScaledFrame)
        OutputFrame = ScaledFrame;

    // Output
    switch (OutputMethod)
    {
        case Output_QImage  :   ReplaceImage(); break;
        case Output_Jpeg    :   AddThumbnail();  break;
        case Output_Panels  :   {
            if(OutputFrame == DecodedFrame) {

                auto frame = OutputFrame.get();
                if(frame) {
                    AVFrame *copyFrame = av_frame_alloc();
                    copyFrame->format = frame->format;
                    copyFrame->width = frame->width;
                    copyFrame->height = frame->height;
                    copyFrame->channels = frame->channels;
                    copyFrame->channel_layout = frame->channel_layout;
                    copyFrame->nb_samples = frame->nb_samples;
                    av_frame_get_buffer(copyFrame, 32);
                    av_frame_copy(copyFrame, frame);
                    av_frame_copy_props(copyFrame, frame);

                    struct FilteredFrameDeleter {
                        static void free(AVFrame* frame) {
                            if(frame) {
                                av_frame_unref(frame);
                                av_frame_free(&frame);
                            }
                        }
                    };

                    OutputFrame.reset(copyFrame, FilteredFrameDeleter::free);
                }
            }
            AddPanel(); break;

        }
        default             :   ;
    }    
}

//---------------------------------------------------------------------------
void FFmpeg_Glue::outputdata::ApplyFilter(const AVFramePtr& sourceFrame)
{
    FilteredFrame.reset();

    if (Filter.empty())
        return;

    if (!FilterGraph && !FilterGraph_Init())
        return;

    if (sourceFrame)
        AccumulatedDuration += sourceFrame->pkt_duration;

    // Push the decoded Frame into the filtergraph 
    //if (av_buffersrc_add_frame_flags(FilterGraph_Source_Context, DecodedFrame, AV_BUFFERSRC_FLAG_KEEP_REF)<0)
    if (av_buffersrc_add_frame_flags(FilterGraph_Source_Context, sourceFrame.get(), 0)<0)
        return;

    struct FilteredFrameDeleter {
        static void free(AVFrame* frame) {
            if(frame) {
                av_frame_unref(frame);
                av_frame_free(&frame);
            }
        }
    };

    // Pull filtered frames from the filtergraph 
    FilteredFrame = AVFramePtr(av_frame_alloc(), FilteredFrameDeleter::free);

    if(sourceFrame) {
        av_frame_copy_props(FilteredFrame.get(), sourceFrame.get());
    }

    int GetAnswer = av_buffersink_get_frame(FilterGraph_Sink_Context, FilteredFrame.get()); //TODO: handling of multiple output per input
    if (GetAnswer==AVERROR(EAGAIN) || GetAnswer==AVERROR_EOF)
    {
        FilteredFrame.reset();
        OutputFrame.reset();
        return;
    }

    if (GetAnswer<0)
    {
        FilteredFrame.reset();
        return;
    }

    FilteredFrame->pkt_duration = AccumulatedDuration;
    AccumulatedDuration = 0;
}

//---------------------------------------------------------------------------
void FFmpeg_Glue::outputdata::ApplyScale(const AVFramePtr& sourceFrame)
{
    ScaledFrame.reset();

    if (!sourceFrame || !sourceFrame->width || !sourceFrame->height)
        return;

    switch (OutputMethod)
    {
        case Output_Jpeg:
        case Output_QImage:
                            break;
        default:
                            return;
    }

    if (!ScaleContext && !Scale_Init(OutputFrame.get()))
        return;

    struct ScaledFrameDeleter {
        static void free(AVFrame* frame) {
            if(frame) {
                av_freep(&frame->data[0]);
                av_frame_free(&frame);
            }
        }
    };

    ScaledFrame = AVFramePtr(av_frame_alloc(), ScaledFrameDeleter::free);
    av_frame_copy_props(ScaledFrame.get(), sourceFrame.get());

    ScaledFrame->width=Width;
    ScaledFrame->height=Height;
    ScaledFrame->format=(AVPixelFormat)Scale_OutputPixelFormat; 

    av_image_alloc(ScaledFrame->data, ScaledFrame->linesize, Width, Height, (AVPixelFormat) Scale_OutputPixelFormat, 1);
    if (sws_scale(ScaleContext, sourceFrame->data, sourceFrame->linesize, 0, sourceFrame->height, ScaledFrame->data, ScaledFrame->linesize)<0)
    {
        ScaledFrame.reset();
    }
}

//---------------------------------------------------------------------------
void FFmpeg_Glue::outputdata::ReplaceImage()
{
    image.frame = OutputFrame;
}

void FFmpeg_Glue::outputdata::AVPacketDeleter::operator()(AVPacket* packet) {
    // FFmpeg pointers - Output
    if (packet)
    {
        av_packet_unref(packet);
        av_packet_free(&packet);
    }
};

//---------------------------------------------------------------------------
void FFmpeg_Glue::outputdata::AddThumbnail()
{
    if (Thumbnails.size()%Thumbnails_Modulo)
    {
        Thumbnails.push_back(NULL);
        return; // Not wanting to saturate memory. TODO: Find a smarter way to detect memory usage
    }

    Thumbnails.push_back(encodeFrame(OutputFrame.get()));
}

void FFmpeg_Glue::outputdata::AddPanel()
{
    if(OutputFrame) {

        Panels.push_back(OutputFrame);
        // qDebug() << "added panel: " << index << this << OutputFrame->width << " x " << OutputFrame->height << " duration " << OutputFrame->pkt_duration;
    }
}

//---------------------------------------------------------------------------
bool FFmpeg_Glue::outputdata::initEncoder(const QSize& size)
{
    if(Output_CodecContext)
        return true;

    //
    AVCodec *Output_Codec=avcodec_find_encoder((AVCodecID) Output_CodecID);
    if (!Output_Codec)
        return false;
    Output_CodecContext=avcodec_alloc_context3 (Output_Codec);
    if (!Output_CodecContext)
        return false;
    Output_CodecContext->qmin          = 8;
    Output_CodecContext->qmax          = 12;
    Output_CodecContext->width         = size.width();
    Output_CodecContext->height        = size.height();
    Output_CodecContext->pix_fmt       = (AVPixelFormat) Output_PixelFormat;
    Output_CodecContext->time_base.num = Stream->codec->time_base.num;
    Output_CodecContext->time_base.den = Stream->codec->time_base.den;
    if (avcodec_open2(Output_CodecContext, Output_Codec, NULL) < 0)
        return false;

    // All is OK
    return true;
}

std::unique_ptr<AVPacket, FFmpeg_Glue::outputdata::AVPacketDeleter> FFmpeg_Glue::outputdata::encodeFrame(AVFrame* frame, bool* ok /*= nullptr*/)
{
    if(ok)
        *ok = true;

    auto outPacket = std::unique_ptr<AVPacket, AVPacketDeleter>(av_packet_alloc(), AVPacketDeleter());
    av_init_packet (outPacket.get());

    outPacket->data = nullptr;
    outPacket->size = 0;

    struct NoDeleter {
        static void free(AVFrame* frame) {
            Q_UNUSED(frame)
        }
    };

    AVFramePtr Frame(frame, NoDeleter::free);

    if(scaleBeforeEncoding)
    {
        if (ScaleContext || Scale_Init(frame))
        {
            struct ScaledFrameDeleter {
                static void free(AVFrame* frame) {
                    if(frame) {
                        av_freep(&frame->data[0]);
                        av_frame_free(&frame);
                    }
                }
            };

            auto scaledFrame = AVFramePtr(av_frame_alloc(), ScaledFrameDeleter::free);
            av_frame_copy_props(scaledFrame.get(), Frame.get());

            scaledFrame->width = Width;
            scaledFrame->height= Height;
            scaledFrame->format=(AVPixelFormat)Scale_OutputPixelFormat;

            av_image_alloc(scaledFrame->data, scaledFrame->linesize, scaledFrame->width, scaledFrame->height, (AVPixelFormat) Scale_OutputPixelFormat, 1);
            if (sws_scale(ScaleContext, Frame->data, Frame->linesize, 0, Frame->height, scaledFrame->data, scaledFrame->linesize)<0)
            {
                scaledFrame.reset();
            }
            else
            {
                Frame = scaledFrame;
            }

        }
    }

    if (!Output_CodecContext && !initEncoder(QSize(Frame->width, Frame->height)))
    {
        if(ok)
            *ok = false;

        return outPacket;
    }

    int got_packet=0;
    int result = avcodec_send_frame(Output_CodecContext, Frame.get());
    if (result < 0)
    {
        char buffer[256];
        qDebug() << av_make_error_string(buffer, sizeof buffer, result);

        if(ok)
            *ok = false;

        return outPacket;
    }

    result = avcodec_receive_packet(Output_CodecContext, outPacket.get());
    if (result != 0)
    {
        char buffer[256];
        qDebug() << av_make_error_string(buffer, sizeof buffer, result);

        if(ok)
            *ok = false;

        return outPacket;
    }

    outPacket->duration = Frame->pkt_duration;
    return outPacket;
}

//---------------------------------------------------------------------------
bool FFmpeg_Glue::outputdata::FilterGraph_Init()
{
    // Alloc
    AVFilterInOut*  Outputs                     = avfilter_inout_alloc();
    AVFilterInOut*  Inputs                      = avfilter_inout_alloc();
                    FilterGraph                 = avfilter_graph_alloc();

    // Source
    stringstream    Args;
    const AVFilter*       Source;
    const AVFilter*       Sink;
    if (Type==AVMEDIA_TYPE_VIDEO)
    {
        Source                                  = avfilter_get_by_name("buffer");
        Sink                                    = avfilter_get_by_name("buffersink");
        Args    << "video_size="                << Stream->codec->width
                << "x"                          << Stream->codec->height
                <<":pix_fmt="                   << (int)Stream->codec->pix_fmt
                <<":time_base="                 << Stream->codec->time_base.num
                << "/"                          << Stream->codec->time_base.den
                <<":pixel_aspect="              << Stream->codec->sample_aspect_ratio.num
                << "/"                          << Stream->codec->sample_aspect_ratio.den;
    }
    if (Type==AVMEDIA_TYPE_AUDIO)
    {
        Source                                  = avfilter_get_by_name("abuffer");
        Sink                                    = avfilter_get_by_name(OutputMethod==Output_Stats?"abuffersink":"buffersink");
        Args    << "time_base="                 << Stream->codec->time_base.num
                << "/"                          << Stream->codec->time_base.den
                <<":sample_rate="               << Stream->codec->sample_rate
                <<":sample_fmt="                << av_get_sample_fmt_name(Stream->codec->sample_fmt)
                <<":channel_layout=0x"          << std::hex << (Stream->codec->channel_layout ? Stream->codec->channel_layout : av_get_default_channel_layout(Stream->codec->channels));
            ;
    }
    if (avfilter_graph_create_filter(&FilterGraph_Source_Context, Source, "in", Args.str().c_str(), NULL, FilterGraph)<0)
    {
        FilterGraph_Free();
        return false;
    }

    // Sink
    if (avfilter_graph_create_filter(&FilterGraph_Sink_Context, Sink, "out", NULL, NULL, FilterGraph)<0)
    {
        FilterGraph_Free();
        return false;
    }

    // Endpoints for the filter graph. 
    Outputs->name       = av_strdup("in");
    Outputs->filter_ctx = FilterGraph_Source_Context;
    Outputs->pad_idx    = 0;
    Outputs->next       = NULL;
    Inputs->name        = av_strdup("out");
    Inputs->filter_ctx  = FilterGraph_Sink_Context;
    Inputs->pad_idx     = 0;
    Inputs->next        = NULL;
    if (avfilter_graph_parse_ptr(FilterGraph, Filter.c_str(), &Inputs, &Outputs, NULL)<0)
    {
        FilterGraph_Free();
        return false;
    }
    if (avfilter_graph_config(FilterGraph, NULL)<0)
    {
        FilterGraph_Free();
        return false;
    }

    // All is OK
    return true;
}

//---------------------------------------------------------------------------
void FFmpeg_Glue::outputdata::FilterGraph_Free()
{
    if (FilterGraph)
    {
        avfilter_graph_free(&FilterGraph);
        FilterGraph=NULL;
    }
}

//---------------------------------------------------------------------------
bool FFmpeg_Glue::outputdata::Scale_Init(AVFrame* frame)
{
    if (!frame)
        return false;    
        
    if(OutputMethod != Output_Panels)
    {
        if (!AdaptDAR())
            return false;
    }

    // Init
    ScaleContext = sws_getContext(frame->width, frame->height,
                                    (AVPixelFormat)frame->format,
                                    Width, Height,
                                    (AVPixelFormat) Scale_OutputPixelFormat,
                                    Output_QImage?/* SWS_BICUBIC */ SWS_FAST_BILINEAR :SWS_FAST_BILINEAR, NULL, NULL, NULL);

    // All is OK
    return true;
}

//---------------------------------------------------------------------------
void FFmpeg_Glue::outputdata::Scale_Free()
{
    if (ScaleContext)
    {
        sws_freeContext(ScaleContext);
        ScaleContext=NULL;
    }

    ScaledFrame.reset();
}

//---------------------------------------------------------------------------
bool FFmpeg_Glue::outputdata::AdaptDAR()
{
    double DAR = GetDAR();

    int Target_Width=Height*DAR;
    int Target_Height=Width/DAR;
    if (Target_Width>Width)
        Height=Target_Height;
    if (Target_Height>Height)
        Width=Target_Width;

    return true;
}

double FFmpeg_Glue::GetDAR(const FFmpeg_Glue::AVFramePtr & frame) {
    if(frame->sample_aspect_ratio.num && frame->sample_aspect_ratio.den) {
        return ((double)frame->width)/frame->height*frame->sample_aspect_ratio.num/frame->sample_aspect_ratio.den;
    } else {
        return ((double)frame->width)/frame->height;
    }
}

double FFmpeg_Glue::outputdata::GetDAR()
{
    if(FilteredFrame)
        return FFmpeg_Glue::GetDAR(FilteredFrame);

    if(DecodedFrame)
        return FFmpeg_Glue::GetDAR(DecodedFrame);

    return 4.0/3.0;
}

void ensureFFMpegInitialized() {
    static bool initialized = false;

    if(!initialized)  {
        qDebug() << "initializing ffmpeg.. .";

        // FFmpeg init
        av_register_all();
        avfilter_register_all();

        initialized = true;
    }
}

//***************************************************************************
// Constructor / Destructor
//***************************************************************************

//---------------------------------------------------------------------------
FFmpeg_Glue::FFmpeg_Glue (const string &FileName_, activealltracks ActiveAllTracks, std::vector<CommonStats*>* Stats_, StreamsStats** streamsStats, FormatStats** formatStats, bool WithStats_) :
    Stats(Stats_),
    WithStats(WithStats_),
    FileName(FileName_),
    InputDatas_Copy(false),
    mutex(nullptr)
{
    ensureFFMpegInitialized();

    //av_log_set_callback(avlog_cb);
    av_log_set_level(AV_LOG_QUIET);

    // Open file
    FormatContext=NULL;
    if (!FileName.empty())
    {
        if (avformat_open_input(&FormatContext, FileName.c_str(), NULL, NULL)>=0)
        {
            if (avformat_find_stream_info(FormatContext, NULL)>=0)
            {
                for (int Pos=0; Pos<FormatContext->nb_streams; Pos++)
                {
                    switch (FormatContext->streams[Pos]->codec->codec_type)
                    {
                        case AVMEDIA_TYPE_VIDEO:
                        case AVMEDIA_TYPE_AUDIO:
                                                    {
                                                        inputdata* InputData=new inputdata;
                                                        InputData->Type=FormatContext->streams[Pos]->codec->codec_type;
                                                        InputData->Stream=FormatContext->streams[Pos];
                                                        AVCodec* Codec=avcodec_find_decoder(InputData->Stream->codec->codec_id);
                                                        if (Codec)
                                                            avcodec_open2(InputData->Stream->codec, Codec, NULL);

                                                        InputData->FrameCount=InputData->Stream->nb_frames;
                                                        if (InputData->Stream->duration!=AV_NOPTS_VALUE)
                                                            InputData->Duration=((double)InputData->Stream->duration)*InputData->Stream->time_base.num/InputData->Stream->time_base.den;

                                                        // If duration is not known, estimating it
                                                        if (InputData->Duration==0 && FormatContext->duration!=AV_NOPTS_VALUE)
                                                            InputData->Duration=((double)FormatContext->duration)/AV_TIME_BASE;

                                                        // If frame count is not known, estimating it
                                                        if (InputData->FrameCount==0 && InputData->Stream->avg_frame_rate.num && InputData->Stream->avg_frame_rate.den && InputData->Duration)
                                                            InputData->FrameCount=InputData->Duration*InputData->Stream->avg_frame_rate.num/InputData->Stream->avg_frame_rate.den;
                                                        if (InputData->FrameCount==0
                                                         && ((InputData->Stream->time_base.num==1 && InputData->Stream->time_base.den>=24 && InputData->Stream->time_base.den<=60)
                                                          || (InputData->Stream->time_base.num==1001 && InputData->Stream->time_base.den>=24000 && InputData->Stream->time_base.den<=60000)))
                                                            InputData->FrameCount=InputData->Stream->duration;

                                                        //
                                                        InputDatas.push_back(InputData);
                                                    }
                                                    break;
                        default: InputDatas.push_back(NULL);
                    }
                }
            }
        }
    }

    // Packet
    Packet = new AVPacket;
    av_init_packet(Packet);
    Packet->data = NULL;
    Packet->size = 0;

    // Frame
    Frame = av_frame_alloc();
    if (!Frame)
        return;

    // Stats
    if (WithStats)
    {
        if(streamsStats)
            *streamsStats = new StreamsStats(FormatContext);

        if(formatStats)
            *formatStats = new FormatStats(FormatContext);

        size_t VideoPos=0;
        size_t AudioPos=0;

        for (size_t Pos=0; Pos<InputDatas.size(); Pos++)
        {
            inputdata* InputData=InputDatas[Pos];

            CommonStats* Stat=NULL;
            if (InputData)
            {
                switch (InputData->Type)
                {
                    case AVMEDIA_TYPE_VIDEO:
                                                if (!VideoPos || ActiveAllTracks[Type_Video])
                                                    Stat=new VideoStats(InputData->FrameCount, InputData->Duration, InputData->Stream);
                                                VideoPos++;
                                                break;
                    case AVMEDIA_TYPE_AUDIO:
                                                if (!AudioPos || ActiveAllTracks[Type_Audio])
                                                    Stat=new AudioStats(InputData->FrameCount, InputData->Duration, InputData->Stream); AudioPos++; break;
                                                AudioPos++;
                                                break;
                }
            }
            
            Stats->push_back(Stat);
        }
    }

    // Temp
    Seek_TimeStamp=AV_NOPTS_VALUE;
}

//---------------------------------------------------------------------------
FFmpeg_Glue::~FFmpeg_Glue()
{
    if (Packet)
    {
        //av_packet_unref(Packet);
        delete Packet;
    }

    if (Frame)
        av_frame_free(&Frame);

    for (size_t Pos=0; Pos<InputDatas.size(); Pos++)
        delete InputDatas[Pos];
    for (size_t Pos=0; Pos<OutputDatas.size(); Pos++)
        delete OutputDatas[Pos];
    avformat_close_input(&FormatContext);
}

FFmpeg_Glue::Image FFmpeg_Glue::Image_Get(size_t Pos) const
{
    QMutexLocker locker(mutex);

    if (Pos>=OutputDatas.size() || !OutputDatas[Pos] || !OutputDatas[Pos]->Enabled)
        return Image();

    return OutputDatas[Pos]->image;
}

FFmpeg_Glue::AVPacketPtr FFmpeg_Glue::encodePanelFrame(int outputIndex, AVFrame *frame)
{
    auto output = OutputDatas[outputIndex];
    return output->encodeFrame(frame);
}

std::shared_ptr<AVPacket> FFmpeg_Glue::ThumbnailPacket_Get(size_t Pos, size_t FramePos)
{
    QMutexLocker locker(mutex);

    if (Pos>=OutputDatas.size() || !OutputDatas[Pos] || !OutputDatas[Pos]->Enabled)
        return NULL;

    auto avpacket = OutputDatas[Pos]->Thumbnails[FramePos];
    return avpacket;
}

QByteArray FFmpeg_Glue::Thumbnail_Get(size_t Pos, size_t FramePos)
{
    QMutexLocker locker(mutex);

    if (Pos>=OutputDatas.size() || !OutputDatas[Pos] || !OutputDatas[Pos]->Enabled)
        return NULL;

    auto avpacket = OutputDatas[Pos]->Thumbnails[FramePos].get();
    return QByteArray(reinterpret_cast<char*> (avpacket->data), avpacket->size);
}

size_t FFmpeg_Glue::Thumbnails_Size(size_t Pos)
{
    QMutexLocker locker(mutex);

    if (Pos>=OutputDatas.size() || !OutputDatas[Pos] || !OutputDatas[Pos]->Enabled)
        return 0;

    return OutputDatas[Pos]->Thumbnails.size();
}

//***************************************************************************
// Actions
//***************************************************************************

FFmpeg_Glue::outputdata* FFmpeg_Glue::AddOutput(size_t inputPos, int Scale_Width, int Scale_Height, outputmethod OutputMethod, int FilterType, const string &Filter)
{
    OutputDatas.push_back(NULL);
    ModifyOutput(inputPos, OutputDatas.size() - 1, Scale_Width, Scale_Height, OutputMethod, FilterType, Filter);

    return OutputDatas.back();
}


//---------------------------------------------------------------------------
void FFmpeg_Glue::AddOutput(int Scale_Width, int Scale_Height, outputmethod OutputMethod, int FilterType, const string &Filter)
{
    for (size_t InputPos=0; InputPos<InputDatas.size(); InputPos++)
    {
        inputdata* InputData=InputDatas[InputPos];

        if (InputData && InputData->Type==FilterType)
        {
            auto filter = Filter;
            qDebug() << "FFmpeg_Glue::AddOutput => original filter: " << QString::fromStdString(Filter);

            auto pos = -1;
            static std::string audioSampleRate = "${AUDIO_SAMPLE_RATE}";

            while((pos = filter.find(audioSampleRate)) != -1)
                filter.replace(pos, audioSampleRate.length(), std::to_string(sampleRate(InputPos)));

            qDebug() << "FFmpeg_Glue::AddOutput => modified filter: " << QString::fromStdString(filter);

            OutputDatas.push_back(NULL);
            ModifyOutput(InputPos, OutputDatas.size()-1, Scale_Width, Scale_Height, OutputMethod, FilterType, filter);
        }
    }
}

//---------------------------------------------------------------------------
void FFmpeg_Glue::CloseOutput()
{
    // Complete
    if (WithStats)
        for (size_t Pos=0; Pos<Stats->size(); Pos++)
            if ((*Stats)[Pos])
                (*Stats)[Pos]->StatsFinish();
}

//---------------------------------------------------------------------------
void FFmpeg_Glue::ModifyOutput(size_t InputPos, size_t OutputPos, int Scale_Width, int Scale_Height, outputmethod OutputMethod, int FilterType, const string &Filter)
{
    if (InputPos>=InputDatas.size())
        return;
    inputdata* InputData=InputDatas[InputPos];

    outputdata* OutputData=new outputdata(OutputPos);
    OutputData->Type=FilterType;
    OutputData->Width=Scale_Width;
    OutputData->Height=Scale_Height;
    OutputData->OutputMethod=OutputMethod;
    OutputData->Scale_OutputPixelFormat = (OutputMethod == Output_QImage ? AV_PIX_FMT_RGB24:AV_PIX_FMT_YUVJ420P);

    OutputData->Filter=Filter;

    qDebug() << "created output: " << OutputData << "input pos: " << InputPos << "output pos: " << OutputPos << "filter: " << Filter.c_str();

    OutputData->Stream=InputData->Stream;
    if (OutputMethod==Output_Stats && Stats && Stats->size() > InputPos)
        OutputData->Stats=(*Stats)[InputPos];

    delete OutputDatas[OutputPos];
    OutputDatas[OutputPos]=OutputData;
}

//---------------------------------------------------------------------------
void FFmpeg_Glue::Seek(size_t FramePos)
{
    QMutexLocker locker(mutex);
    qDebug() << "Seek " << FramePos;

    for (size_t Pos=0; Pos<InputDatas.size(); Pos++)
    {
        inputdata* InputData=InputDatas[Pos];

        if (InputData && InputData->Type==AVMEDIA_TYPE_VIDEO)
        {
            InputData->FramePos=FramePos;

            // Getting first time stamp
            if (InputData->FirstTimeStamp==DBL_MAX && Stats && !Stats->empty() && (*Stats)[0])
                InputData->FirstTimeStamp=(*Stats)[0]->FirstTimeStamp;

            // Finding the right source and time stamp computing
            if (Stats && !Stats->empty() && (*Stats)[0] && FramePos<(*Stats)[0]->x_Current)
            {
                double TimeStamp=(*Stats)[0]->x[1][FramePos];
                if (InputData->FirstTimeStamp!=DBL_MAX)
                    TimeStamp+=InputData->FirstTimeStamp;
                Seek_TimeStamp=(int64_t)(TimeStamp*InputData->Stream->time_base.den/InputData->Stream->time_base.num);
            }
            else
            {
                Seek_TimeStamp=FramePos;
                Seek_TimeStamp*=InputData->Stream->duration;

                if(InputData->FrameCount != 0)
                    Seek_TimeStamp/=InputData->FrameCount;  // TODO: seek based on time stamp
            }
    
            // Seek
            if (FormatContext)
            {
                if (Seek_TimeStamp)
                    avformat_seek_file(FormatContext, Pos, 0, Seek_TimeStamp, Seek_TimeStamp, 0);
                else
                    avformat_seek_file(FormatContext, Pos, 0, 1, 1, 0); //Found some cases such seek fails
            }

            // Flushing
            avcodec_flush_buffers(InputData->Stream->codec);

            break;
        }
    }
}

//---------------------------------------------------------------------------
void FFmpeg_Glue::FrameAtPosition(size_t FramePos)
{
    for (size_t Pos=0; Pos<InputDatas.size(); Pos++)
    {
        inputdata* InputData=InputDatas[Pos];

        if (InputData && InputData->Type==AVMEDIA_TYPE_VIDEO)
        {
            // qDebug() << "*** frameAtPosition: FramePos = " << FramePos << ", InputData->FramePos = " << InputData->FramePos;

            if (InputData->FramePos != FramePos)
            {
                if(FramePos == -1)
                {
                    // qDebug("Initial Seek");
                    Seek(FramePos);
                }
                else if(FramePos >= InputData->FramePos)
                {
                    // qDebug("Seek forward");
                    Seek(FramePos + 1);
                    InputData->FramePos = FramePos;
                }
                else
                {
                    // qDebug("Seek backward");
                    Seek(FramePos + 1);
                    InputData->FramePos = FramePos;
                }

                // qDebug() << "frameAtPosition after Seek: FramePos = " << FramePos << ", InputData->FramePos = " << InputData->FramePos;
            }

            NextFrame();

            // qDebug() << "frameAtPosition after NextFrame: FramePos = " << FramePos << ", InputData->FramePos = " << InputData->FramePos;
            break;
        }
    }
}

//---------------------------------------------------------------------------
bool FFmpeg_Glue::NextFrame()
{
    QMutexLocker locker(mutex);

	if (FileName.empty())
    {
        if (Seek_TimeStamp!=AV_NOPTS_VALUE)
            InputDatas[0]->FramePos=Seek_TimeStamp;
            
        AVPacket Packet;
        av_init_packet(&Packet);
        av_packet_from_data(&Packet, NULL, 0);
        Packet.stream_index=0;
        Packet.pts=InputDatas[0]->FramePos;
        if (InputDatas[0]->FramesCache && !(*InputDatas[0]->FramesCache).empty())
        {
            if (InputDatas[0]->FramePos>=(*InputDatas[0]->FramesCache).size())
            {
                if (!InputDatas[0]->FramesCache_Default)
                {
                    InputDatas[0]->FramesCache_Default = av_frame_alloc();
                    InputDatas[0]->FramesCache_Default->width = (*InputDatas[0]->FramesCache)[0]->width;
                    InputDatas[0]->FramesCache_Default->height = (*InputDatas[0]->FramesCache)[0]->height;
                    InputDatas[0]->FramesCache_Default->format = (*InputDatas[0]->FramesCache)[0]->format;
                    InputDatas[0]->FramesCache_Default->pkt_pts = (*InputDatas[0]->FramesCache)[0]->pkt_pts;
                    int size = av_image_get_buffer_size((AVPixelFormat)InputDatas[0]->FramesCache_Default->format, InputDatas[0]->FramesCache_Default->width, InputDatas[0]->FramesCache_Default->height, 1);
                    uint8_t* buffer = (uint8_t*)av_malloc(size);
                    av_image_fill_arrays(InputDatas[0]->FramesCache_Default->data, InputDatas[0]->FramesCache_Default->linesize, buffer, (AVPixelFormat)InputDatas[0]->FramesCache_Default->format, InputDatas[0]->FramesCache_Default->width, InputDatas[0]->FramesCache_Default->height, 1);
                    memset(InputDatas[0]->FramesCache_Default->data[0], 0xFF, InputDatas[0]->FramesCache_Default->linesize[0] * InputDatas[0]->FramesCache_Default->height);
                    memset(InputDatas[0]->FramesCache_Default->data[1], 0x80, InputDatas[0]->FramesCache_Default->linesize[1] * InputDatas[0]->FramesCache_Default->height / 2);
                    memset(InputDatas[0]->FramesCache_Default->data[2], 0x80, InputDatas[0]->FramesCache_Default->linesize[2] * InputDatas[0]->FramesCache_Default->height / 2);
                }
                Frame=InputDatas[0]->FramesCache_Default;
                Frame->pkt_pts=InputDatas[0]->FramePos;
            }
            else
                Frame=(*InputDatas[0]->FramesCache)[InputDatas[0]->FramePos];
            OutputFrame(&Packet, false);
        }
        return true;
    }
    
    if (!FormatContext)
        return false;

    // Next frame
    while (Packet->size || av_read_frame(FormatContext, Packet) >= 0)
    {
        AVPacket TempPacket=*Packet;
        auto packetStreamIndex = Packet->stream_index;
        // qDebug() << "streamIndex: " << packetStreamIndex;

        if (Packet->stream_index<InputDatas.size() && InputDatas[packetStreamIndex] && InputDatas[packetStreamIndex]->Enabled)
        {
            do
            {
                if (OutputFrame(Packet))
                {
                    if (Packet->size==0)
                        av_packet_unref(&TempPacket);
                    if (InputDatas[Packet->stream_index]->Type==AVMEDIA_TYPE_VIDEO)
                        return true;
                }
            }
            while (Packet->size > 0);
        }
        av_packet_unref(&TempPacket);
        Packet->size=0;
    }
    
    for(auto i = 0; i < InputDatas.size(); ++i)
    {
        if(!InputDatas[i])
            continue;

        auto streamIndex = InputDatas[i]->Stream->index;

        Packet->stream_index = streamIndex;
        // Flushing
        Packet->data=NULL;
        Packet->size=0;
        while (OutputFrame(Packet));
    }
    
    // Complete
    if (WithStats)
        for (size_t Pos=0; Pos<Stats->size(); Pos++)
            if ((*Stats)[Pos])
                (*Stats)[Pos]->StatsFinish();

    return false;
}

int DecodeVideo(FFmpeg_Glue::inputdata* InputData, AVFrame* Frame, int & got_frame, AVPacket* TempPacket)
{
	return avcodec_decode_video2(InputData->Stream->codec, Frame, &got_frame, TempPacket);
}
//---------------------------------------------------------------------------
bool FFmpeg_Glue::OutputFrame(AVPacket* TempPacket, bool Decode)
{
    if (TempPacket->stream_index>=InputDatas.size())
        return false;
    inputdata* InputData=InputDatas[TempPacket->stream_index];
    if (!InputData)
        return false;
    
    // Decoding
    int got_frame;
    if (Decode)
    {
        got_frame=0;
        int Bytes;
        switch(InputData->Type)
        {
            case AVMEDIA_TYPE_VIDEO : Bytes=DecodeVideo(InputData, Frame, got_frame, TempPacket); break;
            case AVMEDIA_TYPE_AUDIO : Bytes=avcodec_decode_audio4(InputData->Stream->codec, Frame, &got_frame, TempPacket); break;
            default                 : Bytes=0;
        }
        
        if (Bytes<=0 && !got_frame)
        {
            TempPacket->data+=TempPacket->size;
            TempPacket->size=0;

            for (size_t OutputPos=0; OutputPos<OutputDatas.size(); OutputPos++)
                if (OutputDatas[OutputPos] && OutputDatas[OutputPos]->Enabled && OutputDatas[OutputPos]->Stream==InputData->Stream) {
                    if(!OutputDatas[OutputPos]->Filter.empty()) // flush delayed filtered frames
                        OutputDatas[OutputPos]->Process(nullptr);
                }

            return false;
        }
        TempPacket->data+=Bytes;
        TempPacket->size-=Bytes;
    }
    else if (Frame && Frame->format!=-1)
        got_frame=1;
    else
        got_frame=0;

    // Analyzing frame
    if (got_frame && (Seek_TimeStamp==AV_NOPTS_VALUE || Frame->pkt_pts>=(Seek_TimeStamp?(Seek_TimeStamp-1):Seek_TimeStamp)))
    {
        Seek_TimeStamp=AV_NOPTS_VALUE;
        int64_t ts=(Frame->pkt_pts==AV_NOPTS_VALUE)?Frame->pkt_dts:Frame->pkt_pts;
        if (ts!=AV_NOPTS_VALUE && ts<InputData->FirstTimeStamp*InputData->Stream->time_base.den/InputData->Stream->time_base.num)
            InputData->FirstTimeStamp=((double)ts)*InputData->Stream->time_base.num/InputData->Stream->time_base.den;
        
        for (size_t OutputPos=0; OutputPos<OutputDatas.size(); OutputPos++)
        {
            auto output = OutputDatas[OutputPos];
            if(!output)
                continue;

            if(!output->Enabled)
                continue;

            if(output->Stream != InputData->Stream)
                continue;

            output->Process(Frame);
        }

        if(Decode)
            InputData->FramePos++;

        if (InputData->FramePos>InputData->FrameCount)
            InputData->FrameCount=InputData->FramePos;
        if (!InputDatas_Copy && FileName.empty() && (!InputData->FramesCache || InputData->FramesCache->size()<300)) // Value arbitrary choosen
        {
            AVFrame* NewFrame = av_frame_alloc();
            NewFrame->width = Frame->width;
            NewFrame->height = Frame->height;
            NewFrame->format = Frame->format;
            NewFrame->pkt_pts = Frame->pkt_pts;
            int size = av_image_get_buffer_size((AVPixelFormat)NewFrame->format, NewFrame->width, NewFrame->height, 1);
            uint8_t* buffer = (uint8_t*)av_malloc(size);
            av_image_fill_arrays(NewFrame->data, NewFrame->linesize, buffer, (AVPixelFormat)NewFrame->format, NewFrame->width, NewFrame->height, 1);
            memcpy(NewFrame->data[0], Frame->data[0], NewFrame->linesize[0] * NewFrame->height);
            memcpy(NewFrame->data[1], Frame->data[1], NewFrame->linesize[1] * NewFrame->height / 2);
            memcpy(NewFrame->data[2], Frame->data[2], NewFrame->linesize[2] * NewFrame->height / 2);

            if (!InputData->FramesCache)
                InputData->FramesCache=new std::vector<AVFrame*>;
            InputData->FramesCache->push_back(NewFrame);
        }

        return true;
    }

    return false;
}

//---------------------------------------------------------------------------
bool FFmpeg_Glue::OutputFrame(unsigned char* Data, size_t Size, int stream_index, int FramePos)
{
    AVPacket Packet;
    av_init_packet(&Packet);
    Packet.data=Data;
    Packet.size=Size;
    Packet.stream_index=stream_index;
    Packet.pts=FramePos;
    return OutputFrame(&Packet);
}

//***************************************************************************
// FFmpeg actions
//***************************************************************************

//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void FFmpeg_Glue::Disable(const size_t Pos)
{
    if (Pos>=OutputDatas.size())
        return;
    outputdata* OutputData=OutputDatas[Pos];
    if (!OutputData)
        return;

    OutputData->Enabled=false;
}

//---------------------------------------------------------------------------
double FFmpeg_Glue::TimeStampOfCurrentFrame(const size_t OutputPos)
{
    QMutexLocker locker(mutex);

    if (OutputPos >= OutputDatas.size())
        return DBL_MAX;

    outputdata *OutputData = OutputDatas[OutputPos];
    if (!OutputData || !OutputData->DecodedFrame)
        return DBL_MAX;

    double pkt_pts = OutputData->DecodedFrame->pkt_pts;
    int num = OutputData->Stream->time_base.num;
    int den = OutputData->Stream->time_base.den;

    return pkt_pts * num / den;
}

//---------------------------------------------------------------------------
void FFmpeg_Glue::Scale_Change(int Scale_Width_, int Scale_Height_, int index /* = -1*/)
{
    QMutexLocker locker(mutex);

    bool MustOutput=false;

    for (size_t Pos=0; Pos<OutputDatas.size(); Pos++)
    {
        if(index != -1 && index != Pos)
            continue;

        outputdata* OutputData=OutputDatas[Pos];

        if (OutputData)
        {
            int ScaleSave_Width=OutputData->Width;
            int ScaleSave_Height=OutputData->Height;
            OutputData->Width=Scale_Width_;
            OutputData->Height=Scale_Height_;
            OutputData->AdaptDAR();
            if (ScaleSave_Width!=OutputData->Width || ScaleSave_Height!=OutputData->Height)
            {
                OutputData->Scale_Free();
                OutputData->image.free();
                MustOutput=true;
            }
        }
    }

    if (MustOutput)
        OutputFrame(Packet, false);
}

//---------------------------------------------------------------------------
void FFmpeg_Glue::Thumbnails_Modulo_Change(size_t Modulo)
{
    QMutexLocker locker(mutex);

    for (size_t Pos=0; Pos<OutputDatas.size(); Pos++)
    {
        outputdata* OutputData=OutputDatas[Pos];

        if (OutputData)
        {
            OutputData->Thumbnails_Modulo=Modulo;
            size_t Thumbnails_Size=OutputData->Thumbnails.size();
            for (size_t Pos = 0; Pos < Thumbnails_Size; Pos++)
            {
                if (Pos%Modulo)
                    continue;

                OutputData->Thumbnails[Pos].reset();
            }
        }
    }
}

size_t FFmpeg_Glue::TotalFramesCountPerAllStreams() const
{
    QMutexLocker locker(mutex);

    size_t totalFrames = 0;
    for(size_t i = 0; i < InputDatas.size(); ++i)
    {
        if(!InputDatas.at(i))
            continue;

        totalFrames += InputDatas.at(i)->FrameCount;
    }

    return totalFrames;
}

size_t FFmpeg_Glue::TotalFramesProcessedPerAllStreams() const
{
    QMutexLocker locker(mutex);

    size_t totalFramesProcessed = 0;
    for(size_t i = 0; i < InputDatas.size(); ++i)
    {
        if(!InputDatas.at(i))
            continue;

        totalFramesProcessed += InputDatas.at(i)->FramePos;
    }

    return totalFramesProcessed;
}

size_t FFmpeg_Glue::FramesCountPerStream(size_t index) const
{
    QMutexLocker locker(mutex);

    if(!InputDatas.at(index))
        return 0;

    return InputDatas.at(index)->FrameCount;
}

size_t FFmpeg_Glue::FramesProcessedPerStream(size_t index) const
{
    QMutexLocker locker(mutex);

    if(!InputDatas.at(index))
        return 0;

    return InputDatas.at(index)->FramePos;
}

std::vector<size_t> FFmpeg_Glue::FramesCountForAllStreams() const
{
    QMutexLocker locker(mutex);

    std::vector<size_t> totalFramesCountPerStream;
    totalFramesCountPerStream.reserve(InputDatas.size());

    for(size_t i = 0; i < InputDatas.size(); ++i)
    {
        if(!InputDatas.at(i))
            continue;

        totalFramesCountPerStream.push_back(InputDatas.at(i)->FrameCount);
    }

    return totalFramesCountPerStream;
}

std::vector<size_t> FFmpeg_Glue::FramesProcessedForAllStreams() const
{
    QMutexLocker locker(mutex);

    std::vector<size_t> totalFramesProcessedPerStream;
    totalFramesProcessedPerStream.reserve(InputDatas.size());

    for(size_t i = 0; i < InputDatas.size(); ++i)
    {
        if(!InputDatas.at(i))
            continue;

        totalFramesProcessedPerStream.push_back(InputDatas.at(i)->FramePos);
    }

    return totalFramesProcessedPerStream;
}

FFmpeg_Glue::AVFramePtr FFmpeg_Glue::DecodedFrame(size_t index) const
{
    return OutputDatas[index]->DecodedFrame;
}

FFmpeg_Glue::AVFramePtr FFmpeg_Glue::FilteredFrame(size_t index) const
{
    return OutputDatas[index]->FilteredFrame;
}

FFmpeg_Glue::AVFramePtr FFmpeg_Glue::ScaledFrame(size_t index) const
{
    return OutputDatas[index]->ScaledFrame;
}

std::map<string, string> FFmpeg_Glue::getInputMetadata(int pos) const
{
    std::map<string, string> metadata;

    auto tags = InputDatas[pos]->Stream->metadata;
    if (!tags)
        return metadata;

    AVDictionaryEntry *tag = NULL;

    while ((tag = av_dict_get(tags, "", tag, AV_DICT_IGNORE_SUFFIX))) {
        if(tag->key && tag->value)
        {
            metadata[QString(tag->key).toLower().toStdString()] = tag->value;
        } else
        {
            break;
        }
    }

    return metadata;
}

void FFmpeg_Glue::setThreadSafe(bool enable)
{
    if(enable)
    {
        if(!mutex)
            mutex = new QMutex();
    }
    else
    {
        if(mutex)
        {
            delete mutex;
            mutex = nullptr;
        }
    }
}

//***************************************************************************
// Export
//***************************************************************************

//---------------------------------------------------------------------------
string FFmpeg_Glue::ContainerFormat_Get()
{
    if (FormatContext==NULL || FormatContext->iformat==NULL || FormatContext->iformat->long_name==NULL)
        return "";

    return FormatContext->iformat->long_name;
}

//---------------------------------------------------------------------------
int FFmpeg_Glue::StreamCount_Get()
{
    if (FormatContext==NULL)
        return 0;

    return FormatContext->nb_streams;
}

FFmpeg_Glue::FrameRate FFmpeg_Glue::getFrameRate(int streamIndex) const
{
    return FrameRate(InputDatas[streamIndex]->Stream->avg_frame_rate.num, InputDatas[streamIndex]->Stream->avg_frame_rate.den);
}

FFmpeg_Glue::FrameRate FFmpeg_Glue::getAvgVideoFrameRate() const
{
    inputdata* InputData=NULL;
    for (size_t Pos=0; Pos<InputDatas.size(); Pos++)
        if (InputDatas[Pos] && InputDatas[Pos]->Type==AVMEDIA_TYPE_VIDEO)
        {
            return getFrameRate(Pos);
        }

    return FrameRate(0, 0);
}

int FFmpeg_Glue::getStreamType(int streamIndex) const
{
    return InputDatas[streamIndex]->Stream->codecpar->codec_type;
}

int FFmpeg_Glue::sampleRate(int streamIndex) const
{
    return InputDatas[streamIndex]->Stream->codecpar->sample_rate;
}

std::vector<FFmpeg_Glue::StreamInfo> FFmpeg_Glue::findStreams(StreamType type)
{
    std::vector<StreamInfo> streamInfos;

    for(auto i = 0; i < FormatContext->nb_streams; ++i)
    {
        auto stream = FormatContext->streams[i];
        if(stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            auto e=av_dict_get(stream->metadata, "title", NULL, AV_DICT_IGNORE_SUFFIX);
            if(e) {
                qDebug() << "key: " << e->key << "value: " << e->value;
            }

            auto title = e ? std::string(e->value) : std::string();
            bool thumbnails = title == "Frame Thumbnails";

            if(type == Thumbnails) {
               if(thumbnails) {
                   streamInfos.push_back(FFmpeg_Glue::StreamInfo(title, i));
                   break;
               }
               else {
                   continue;
               }
            } else if(type == Panels) {
                if(!thumbnails && !title.empty())
                    streamInfos.push_back(FFmpeg_Glue::StreamInfo(title, i));
            }
        }
    }

    return streamInfos;
}

std::vector<int> FFmpeg_Glue::findStreams(const std::function<bool (AVStream *)> &criteria)
{
    std::vector<int> streamInfos;

    for(auto i = 0; i < FormatContext->nb_streams; ++i)
    {
        auto stream = FormatContext->streams[i];
        if(criteria(stream)) {
            streamInfos.push_back(i);
        }
    }

    return streamInfos;
}

std::vector<int> FFmpeg_Glue::findMediaStreams()
{
    return findStreams([](AVStream* stream) -> bool {
        return stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO || stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO;
    });
}

std::vector<int> FFmpeg_Glue::findVideoStreams()
{
    return findStreams([](AVStream* stream) -> bool {
        return stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO;
    });
}

std::vector<int> FFmpeg_Glue::findAudioStreams()
{
    return findStreams([](AVStream* stream) -> bool {
        return stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO;
    });
}

int FFmpeg_Glue::findInputStreamByOutput(size_t pos) const
{
    auto output = OutputDatas[pos];
    auto stream = output->Stream;

    for(auto i = 0; i < InputDatas.size(); ++i)
    {
        auto intput = InputDatas[i];
        if(intput->Stream == output->Stream)
            return i;
    }

    return -1;
}

//---------------------------------------------------------------------------
int FFmpeg_Glue::BitRate_Get()
{
    if (FormatContext==NULL)
        return 0;

    return FormatContext->bit_rate;
}

//---------------------------------------------------------------------------
string FFmpeg_Glue::VideoFormat_Get()
{
    inputdata* InputData=NULL;
    for (size_t Pos=0; Pos<InputDatas.size(); Pos++)
        if (InputDatas[Pos] && InputDatas[Pos]->Type==AVMEDIA_TYPE_VIDEO)
        {
            InputData=InputDatas[Pos];
            break;
        }

    if (InputData==NULL || InputData->Stream==NULL || InputData->Stream->codec==NULL || InputData->Stream->codec->codec==NULL || InputData->Stream->codec->codec->long_name==NULL)
        return "";

    return InputData->Stream->codec->codec->long_name;
}

//---------------------------------------------------------------------------
double FFmpeg_Glue::VideoDuration_Get()
{
    inputdata* InputData=NULL;
    for (size_t Pos=0; Pos<InputDatas.size(); Pos++)
        if (InputDatas[Pos] && InputDatas[Pos]->Type==AVMEDIA_TYPE_VIDEO)
        {
            InputData=InputDatas[Pos];
            break;
        }

    if (InputData==NULL)
        return 0;

    return InputData->Duration;
}

//---------------------------------------------------------------------------
double FFmpeg_Glue::FramesDivDuration_Get()
{
    inputdata* InputData=NULL;
    for (size_t Pos=0; Pos<InputDatas.size(); Pos++)
        if (InputDatas[Pos] && InputDatas[Pos]->Type==AVMEDIA_TYPE_VIDEO)
        {
            InputData=InputDatas[Pos];
            break;
        }

    if (InputData==NULL || InputData->Stream==NULL || InputData->Stream->codec==NULL || InputData->Stream->codec->codec==NULL || InputData->Stream->codec->codec->long_name==NULL)
        return 0;

    if (InputData->FrameCount && InputData->Duration)
        return InputData->FrameCount/InputData->Duration;

    return 0; // Unknown
}

//---------------------------------------------------------------------------
string FFmpeg_Glue::RVideoFrameRate_Get()
{
    inputdata* InputData=NULL;
    for (size_t Pos=0; Pos<InputDatas.size(); Pos++)
        if (InputDatas[Pos] && InputDatas[Pos]->Type==AVMEDIA_TYPE_VIDEO)
        {
            InputData=InputDatas[Pos];
            break;
        }

    if (InputData==NULL || InputData->Stream==NULL || InputData->Stream->codec==NULL || InputData->Stream->codec->codec==NULL || InputData->Stream->codec->codec->long_name==NULL)
        return string();
    
    if (InputData->Stream->r_frame_rate.num==0)
        return "Und";
    else
    {
        ostringstream convert;
        convert << InputData->Stream->r_frame_rate.num << "/" << InputData->Stream->r_frame_rate.den;
        return convert.str();
    }
}
//---------------------------------------------------------------------------
string FFmpeg_Glue::AvgVideoFrameRate_Get()
{
    inputdata* InputData=NULL;
    for (size_t Pos=0; Pos<InputDatas.size(); Pos++)
        if (InputDatas[Pos] && InputDatas[Pos]->Type==AVMEDIA_TYPE_VIDEO)
        {
            InputData=InputDatas[Pos];
            break;
        }

    if (InputData==NULL || InputData->Stream==NULL || InputData->Stream->codec==NULL || InputData->Stream->codec->codec==NULL || InputData->Stream->codec->codec->long_name==NULL)
        return string();
    
    ostringstream convert;
    if (InputData->Stream->avg_frame_rate.num==0)
        return "Und";
    else
    {
        convert << InputData->Stream->avg_frame_rate.num << "/" << InputData->Stream->avg_frame_rate.den;
        return convert.str();
    }
}

//---------------------------------------------------------------------------
size_t FFmpeg_Glue::VideoFramePos_Get()
{
    inputdata* InputData=NULL;
    for (size_t Pos=0; Pos<InputDatas.size(); Pos++)
        if (InputDatas[Pos] && InputDatas[Pos]->Type==AVMEDIA_TYPE_VIDEO)
        {
            InputData=InputDatas[Pos];
            break;
        }

    if (InputData==NULL)
        return 0;

    return InputData->FramePos;
}

bool FFmpeg_Glue::withStats() const
{
    return WithStats;
}

//---------------------------------------------------------------------------
size_t FFmpeg_Glue::VideoFrameCount_Get()
{
    inputdata* InputData=NULL;
    for (size_t Pos=0; Pos<InputDatas.size(); Pos++)
        if (InputDatas[Pos] && InputDatas[Pos]->Type==AVMEDIA_TYPE_VIDEO)
        {
            InputData=InputDatas[Pos];
            break;
        }

    if (InputData==NULL)
        return 0;

    return InputData->FrameCount;
}

//---------------------------------------------------------------------------
int FFmpeg_Glue::Width_Get()
{
    inputdata* InputData=NULL;
    for (size_t Pos=0; Pos<InputDatas.size(); Pos++)
        if (InputDatas[Pos] && InputDatas[Pos]->Type==AVMEDIA_TYPE_VIDEO)
        {
            InputData=InputDatas[Pos];
            break;
        }

    if (InputData==NULL || InputData->Stream==NULL || InputData->Stream->codec==NULL)
        return 0;

    return InputData->Stream->codec->width;
}

//---------------------------------------------------------------------------
int FFmpeg_Glue::Height_Get()
{
    inputdata* InputData=NULL;
    for (size_t Pos=0; Pos<InputDatas.size(); Pos++)
        if (InputDatas[Pos] && InputDatas[Pos]->Type==AVMEDIA_TYPE_VIDEO)
        {
            InputData=InputDatas[Pos];
            break;
        }

    if (InputData==NULL || InputData->Stream==NULL || InputData->Stream->codec==NULL)
        return 0;

    return InputData->Stream->codec->height;
}

//---------------------------------------------------------------------------
string FFmpeg_Glue::FieldOrder_Get()
{
    inputdata* InputData=NULL;
    for (size_t Pos=0; Pos<InputDatas.size(); Pos++)
        if (InputDatas[Pos] && InputDatas[Pos]->Type==AVMEDIA_TYPE_VIDEO)
        {
            InputData=InputDatas[Pos];
            break;
        }

    if (InputData==NULL || InputData->Stream==NULL || InputData->Stream->codec==NULL)
        return string();

    switch (InputData->Stream->codec->field_order)
    {
        case AV_FIELD_UNKNOWN: return "unknown";
        case AV_FIELD_PROGRESSIVE: return "progressive";
        case AV_FIELD_TT:   return "TFF: top displayed first, top coded first";
        case AV_FIELD_BB:   return "BFF: bottom displayed first, bottom coded first";
        case AV_FIELD_TB:   return "TFF: top displayed first, coded interleaved";
        case AV_FIELD_BT:   return "BFF: bottom displayed first, coded interleaved";
        default: return string();
    }
}

//---------------------------------------------------------------------------
double FFmpeg_Glue::DAR_Get()
{
    inputdata* InputData=NULL;
    for (size_t Pos=0; Pos<InputDatas.size(); Pos++)
        if (InputDatas[Pos] && InputDatas[Pos]->Type==AVMEDIA_TYPE_VIDEO)
        {
            InputData=InputDatas[Pos];
            break;
        }

    if (InputData==NULL || InputData->Stream==NULL || InputData->Stream->codec==NULL || InputData->Stream->codec->codec==NULL || InputData->Stream->codec->codec->long_name==NULL)
        return 0;

    double DAR;
    if (InputData->Stream->codec->sample_aspect_ratio.num && InputData->Stream->codec->sample_aspect_ratio.den)
        DAR=((double)InputData->Stream->codec->width)/InputData->Stream->codec->height*InputData->Stream->codec->sample_aspect_ratio.num/InputData->Stream->codec->sample_aspect_ratio.den;
    else
        DAR=((double)InputData->Stream->codec->width)/InputData->Stream->codec->height;
    return DAR;
}
//---------------------------------------------------------------------------
string FFmpeg_Glue::SAR_Get()
{
    inputdata* InputData=NULL;
    for (size_t Pos=0; Pos<InputDatas.size(); Pos++)
        if (InputDatas[Pos] && InputDatas[Pos]->Type==AVMEDIA_TYPE_VIDEO)
        {
            InputData=InputDatas[Pos];
            break;
        }

    if (InputData==NULL || InputData->Stream==NULL || InputData->Stream->codec==NULL)
        return string();

    if (InputData->Stream->sample_aspect_ratio.num && InputData->Stream->sample_aspect_ratio.num==0)
      return "Und";
    else if (InputData->Stream->sample_aspect_ratio.num)
    {
        ostringstream convert;
        convert << InputData->Stream->sample_aspect_ratio.num << "/" << InputData->Stream->sample_aspect_ratio.den;
        return convert.str();
    }
    else if (InputData->Stream->codec->sample_aspect_ratio.num && InputData->Stream->codec->sample_aspect_ratio.num==0)
      return "Und";
    else
    {
        ostringstream convert;
        convert << InputData->Stream->codec->sample_aspect_ratio.num << "/" << InputData->Stream->codec->sample_aspect_ratio.den;
        return convert.str();
    }
}

double FFmpeg_Glue::OutputDAR_Get(int Pos)
{
    double DAR = 1.0f;

    outputdata* OutputData=OutputDatas[Pos];

    if (OutputData)
    {
        DAR = OutputData->GetDAR();
    }

    return DAR;
}

int FFmpeg_Glue::OutputWidth_Get(int Pos)
{
    outputdata* OutputData=OutputDatas[Pos];

    if (OutputData)
    {
        return OutputData->Width;
    }

    return 0;
}

int FFmpeg_Glue::OutputHeight_Get(int Pos)
{
    outputdata* OutputData=OutputDatas[Pos];

    if (OutputData)
    {
        return OutputData->Height;
    }

    return 0;
}

int FFmpeg_Glue::OutputThumbnailWidth_Get() const
{
    outputdata* OutputData=OutputDatas[0];

    if (OutputData)
    {
        return OutputData->Output_CodecContext->width;
    }

    return 0;
}

int FFmpeg_Glue::OutputThumbnailHeight_Get() const
{
    outputdata* OutputData=OutputDatas[0];

    if (OutputData)
    {
        return OutputData->Output_CodecContext->height;
    }

    return 0;
}

int FFmpeg_Glue::OutputThumbnailBitRate_Get() const
{
    outputdata* OutputData=OutputDatas[0];

    if (OutputData)
    {
        return OutputData->Output_CodecContext->bit_rate;
    }

    return 0;
}

std::string FFmpeg_Glue::getOutputFilter(int pos) const
{
    return OutputDatas[pos]->Filter;
}

void FFmpeg_Glue::getOutputTimeBase(int pos, int &num, int &den) const
{
    num = OutputDatas[pos]->Stream->time_base.num;
    den = OutputDatas[pos]->Stream->time_base.den;
}

//---------------------------------------------------------------------------
string FFmpeg_Glue::PixFormat_Get()
{
    inputdata* InputData=NULL;
    for (size_t Pos=0; Pos<InputDatas.size(); Pos++)
        if (InputDatas[Pos] && InputDatas[Pos]->Type==AVMEDIA_TYPE_VIDEO)
        {
            InputData=InputDatas[Pos];
            break;
        }

    if (InputData==NULL || InputData->Stream==NULL || InputData->Stream->codec==NULL)
        return string();

    switch (InputData->Stream->codec->pix_fmt)
    {
        case AV_PIX_FMT_YUV420P: return "planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples)";
        case AV_PIX_FMT_YUYV422: return "packed YUV 4:2:2, 16bpp, Y0 Cb Y1 Cr";
        case AV_PIX_FMT_RGB24:   return "packed RGB 8:8:8, 24bpp, RGBRGB...";
        case AV_PIX_FMT_BGR24:   return "packed RGB 8:8:8, 24bpp, BGRBGR...";
        case AV_PIX_FMT_YUV422P: return "planar YUV 4:2:2, 16bpp, (1 Cr & Cb sample per 2x1 Y samples)";
        case AV_PIX_FMT_YUV444P: return "planar YUV 4:4:4, 24bpp, (1 Cr & Cb sample per 1x1 Y samples)";
        case AV_PIX_FMT_YUV410P: return "planar YUV 4:1:0: 9bpp, (1 Cr & Cb sample per 4x4 Y samples)";
        case AV_PIX_FMT_YUV411P: return "planar YUV 4:1:1, 12bpp, (1 Cr & Cb sample per 4x1 Y samples)";
        case AV_PIX_FMT_GRAY8:   return "Y 8bpp";
        case AV_PIX_FMT_MONOWHITE: return "Y 1bpp, 0 is white, 1 is black, in each byte pixels are ordered from the msb to the lsb";
        case AV_PIX_FMT_MONOBLACK: return " Y 1bpp, 0 is black, 1 is white, in each byte pixels are ordered from the msb to the lsb";
        case AV_PIX_FMT_PAL8:    return "8 bit with PIX_FMT_RGB32 palette";
        case AV_PIX_FMT_YUVJ420P: return "planar YUV 4:2:0, 12bpp, full scale (JPEG), deprecated in favor of PIX_FMT_YUV420P and setting color_range";
        case AV_PIX_FMT_YUVJ422P: return "planar YUV 4:2:2, 16bpp, full scale (JPEG), deprecated in favor of PIX_FMT_YUV422P and setting color_range";
        case AV_PIX_FMT_YUVJ444P: return "planar YUV 4:4:4, 24bpp, full scale (JPEG), deprecated in favor of PIX_FMT_YUV444P and setting color_range";
        case AV_PIX_FMT_UYVY422: return "packed YUV 4:2:2, 16bpp, Cb Y0 Cr Y1";
        case AV_PIX_FMT_UYYVYY411: return "packed YUV 4:1:1, 12bpp, Cb Y0 Y1 Cr Y2 Y3";
        case AV_PIX_FMT_BGR8:    return "packed RGB 3:3:2:8bpp, (msb)2B 3G 3R(lsb)";
        case AV_PIX_FMT_BGR4:    return "packed RGB 1:2:1 bitstream:4bpp, (msb)1B 2G 1R(lsb), a byte contains two pixels, the first pixel in the byte is the one composed by the 4 msb bits";
        case AV_PIX_FMT_BGR4_BYTE: return "packed RGB 1:2:1:8bpp, (msb)1B 2G 1R(lsb)";
        case AV_PIX_FMT_RGB8:    return "packed RGB 3:3:2:8bpp, (msb)2R 3G 3B(lsb)";
        case AV_PIX_FMT_RGB4:    return "packed RGB 1:2:1 bitstream:4bpp, (msb)1R 2G 1B(lsb), a byte contains two pixels, the first pixel in the byte is the one composed by the 4 msb bits";
        case AV_PIX_FMT_RGB4_BYTE: return "packed RGB 1:2:1:8bpp, (msb)1R 2G 1B(lsb)";
        case AV_PIX_FMT_NV12:    return "planar YUV 4:2:0, 12bpp, 1 plane for Y and 1 plane for the UV components, which are interleaved (first byte U and the following byte V)";
        case AV_PIX_FMT_NV21:    return "as above, but U and V bytes are swapped";

        case AV_PIX_FMT_ARGB:    return "packed ARGB 8:8:8:8, 32bpp, ARGBARGB...";
        case AV_PIX_FMT_RGBA:    return "packed RGBA 8:8:8:8, 32bpp, RGBARGBA...";
        case AV_PIX_FMT_ABGR:    return "packed ABGR 8:8:8:8, 32bpp, ABGRABGR...";
        case AV_PIX_FMT_BGRA:    return "packed BGRA 8:8:8:8, 32bpp, BGRABGRA...";

        case AV_PIX_FMT_GRAY16BE: return "Y, 16bpp, big-endian";
        case AV_PIX_FMT_GRAY16LE: return "Y, 16bpp, little-endian";
        case AV_PIX_FMT_YUV440P: return "planar YUV 4:4:0 (1 Cr & Cb sample per 1x2 Y samples)";
        case AV_PIX_FMT_YUVJ440P: return "planar YUV 4:4:0 full scale (JPEG), deprecated in favor of PIX_FMT_YUV440P and setting color_range";
        case AV_PIX_FMT_YUVA420P: return "planar YUV 4:2:0, 20bpp, (1 Cr & Cb sample per 2x2 Y & A samples)";
        case AV_PIX_FMT_RGB48BE: return "packed RGB 16:16:16, 48bpp, 16R, 16G, 16B, the 2-byte value for each R/G/B component is stored as big-endian";
        case AV_PIX_FMT_RGB48LE: return "packed RGB 16:16:16, 48bpp, 16R, 16G, 16B, the 2-byte value for each R/G/B component is stored as little-endian";

        case AV_PIX_FMT_RGB565BE: return "packed RGB 5:6:5, 16bpp, (msb)   5R 6G 5B(lsb), big-endian";
        case AV_PIX_FMT_RGB565LE: return "packed RGB 5:6:5, 16bpp, (msb)   5R 6G 5B(lsb), little-endian";
        case AV_PIX_FMT_RGB555BE: return "packed RGB 5:5:5, 16bpp, (msb)1A 5R 5G 5B(lsb), big-endian, most significant bit to 0";
        case AV_PIX_FMT_RGB555LE: return "packed RGB 5:5:5, 16bpp, (msb)1A 5R 5G 5B(lsb), little-endian, most significant bit to 0";

        case AV_PIX_FMT_BGR565BE: return "packed BGR 5:6:5, 16bpp, (msb)   5B 6G 5R(lsb), big-endian";
        case AV_PIX_FMT_BGR565LE: return "packed BGR 5:6:5, 16bpp, (msb)   5B 6G 5R(lsb), little-endian";
        case AV_PIX_FMT_BGR555BE: return "packed BGR 5:5:5, 16bpp, (msb)1A 5B 5G 5R(lsb), big-endian, most significant bit to 1";
        case AV_PIX_FMT_BGR555LE: return "packed BGR 5:5:5, 16bpp, (msb)1A 5B 5G 5R(lsb), little-endian, most significant bit to 1";

#if FF_API_VAAPI
        /** @name Deprecated pixel formats */
        /**@{*/
        case AV_PIX_FMT_VAAPI_MOCO: return "HW acceleration through VA API at motion compensation entry-point, Picture.data[3] contains a vaapi_render_state struct which contains macroblocks as well as various fields extracted from headers";
        case AV_PIX_FMT_VAAPI_IDCT: return "HW acceleration through VA API at IDCT entry-point, Picture.data[3] contains a vaapi_render_state struct which contains fields extracted from headers";
        case AV_PIX_FMT_VAAPI_VLD: return "HW decoding through VA API, Picture.data[3] contains a vaapi_render_state struct which contains the bitstream of the slices as well as various fields extracted from headers";
        /**@}*/
#else
        case AV_PIX_FMT_VAAPI: return "Hardware acceleration through VA-API, data[3] contains a VASurfaceID";
#endif

        case AV_PIX_FMT_YUV420P16LE: return "planar YUV 4:2:0, 24bpp, (1 Cr & Cb sample per 2x2 Y samples), little-endian";
        case AV_PIX_FMT_YUV420P16BE: return "planar YUV 4:2:0, 24bpp, (1 Cr & Cb sample per 2x2 Y samples), big-endian";
        case AV_PIX_FMT_YUV422P16LE: return "planar YUV 4:2:2, 32bpp, (1 Cr & Cb sample per 2x1 Y samples), little-endian";
        case AV_PIX_FMT_YUV422P16BE: return "planar YUV 4:2:2, 32bpp, (1 Cr & Cb sample per 2x1 Y samples), big-endian";
        case AV_PIX_FMT_YUV444P16LE: return "planar YUV 4:4:4, 48bpp, (1 Cr & Cb sample per 1x1 Y samples), little-endian";
        case AV_PIX_FMT_YUV444P16BE: return "planar YUV 4:4:4, 48bpp, (1 Cr & Cb sample per 1x1 Y samples), big-endian";
        case AV_PIX_FMT_DXVA2_VLD:  return "HW decoding through DXVA2, Picture.data[3] contains a LPDIRECT3DSURFACE9 pointer";

        case AV_PIX_FMT_RGB444LE: return "packed RGB 4:4:4, 16bpp, (msb)4A 4R 4G 4B(lsb), little-endian, most significant bits to 0";
        case AV_PIX_FMT_RGB444BE: return "packed RGB 4:4:4, 16bpp, (msb)4A 4R 4G 4B(lsb), big-endian, most significant bits to 0";
        case AV_PIX_FMT_BGR444LE: return "packed BGR 4:4:4, 16bpp, (msb)4A 4B 4G 4R(lsb), little-endian, most significant bits to 1";
        case AV_PIX_FMT_BGR444BE: return "packed BGR 4:4:4, 16bpp, (msb)4A 4B 4G 4R(lsb), big-endian, most significant bits to 1";
        case AV_PIX_FMT_YA8: 	return "8bit gray, 8bit alpha";

        case AV_PIX_FMT_BGR48BE: return "packed RGB 16:16:16, 48bpp, 16B, 16G, 16R, the 2-byte value for each R/G/B component is stored as big-endian";
        case AV_PIX_FMT_BGR48LE: return "packed RGB 16:16:16, 48bpp, 16B, 16G, 16R, the 2-byte value for each R/G/B component is stored as little-endian";

        /**
        * The following 12 formats have the disadvantage of needing 1 format for each bit depth.
        * Notice that each 9/10 bits sample is stored in 16 bits with extra padding.
        * If you want to support multiple bit depths, then using AV_PIX_FMT_YUV420P16* with the bpp stored separately is better.
        */
        case AV_PIX_FMT_YUV420P9BE: return "planar YUV 4:2:0, 13.5bpp, (1 Cr & Cb sample per 2x2 Y samples), big-endian";
        case AV_PIX_FMT_YUV420P9LE: return "planar YUV 4:2:0, 13.5bpp, (1 Cr & Cb sample per 2x2 Y samples), little-endian";
        case AV_PIX_FMT_YUV420P10BE: return "planar YUV 4:2:0, 15bpp, (1 Cr & Cb sample per 2x2 Y samples), big-endian";
        case AV_PIX_FMT_YUV420P10LE: return "planar YUV 4:2:0, 15bpp, (1 Cr & Cb sample per 2x2 Y samples), little-endian";
        case AV_PIX_FMT_YUV422P10BE: return "planar YUV 4:2:2, 20bpp, (1 Cr & Cb sample per 2x1 Y samples), big-endian";
        case AV_PIX_FMT_YUV422P10LE: return "planar YUV 4:2:2, 20bpp, (1 Cr & Cb sample per 2x1 Y samples), little-endian";
        case AV_PIX_FMT_YUV444P9BE: return "planar YUV 4:4:4, 27bpp, (1 Cr & Cb sample per 1x1 Y samples), big-endian";
        case AV_PIX_FMT_YUV444P9LE: return "planar YUV 4:4:4, 27bpp, (1 Cr & Cb sample per 1x1 Y samples), little-endian";
        case AV_PIX_FMT_YUV444P10BE: return "planar YUV 4:4:4, 30bpp, (1 Cr & Cb sample per 1x1 Y samples), big-endian";
        case AV_PIX_FMT_YUV444P10LE: return "planar YUV 4:4:4, 30bpp, (1 Cr & Cb sample per 1x1 Y samples), little-endian";
        case AV_PIX_FMT_YUV422P9BE: return "planar YUV 4:2:2, 18bpp, (1 Cr & Cb sample per 2x1 Y samples), big-endian";
        case AV_PIX_FMT_YUV422P9LE: return "planar YUV 4:2:2, 18bpp, (1 Cr & Cb sample per 2x1 Y samples), little-endian";
        case AV_PIX_FMT_GBRP: 		return "planar GBR 4:4:4 24bpp";
        case AV_PIX_FMT_GBRP9BE: return "planar GBR 4:4:4 27bpp, big-endian";
        case AV_PIX_FMT_GBRP9LE: return "planar GBR 4:4:4 27bpp, little-endian";
        case AV_PIX_FMT_GBRP10BE: return "planar GBR 4:4:4 30bpp, big-endian";
        case AV_PIX_FMT_GBRP10LE: return "planar GBR 4:4:4 30bpp, little-endian";
        case AV_PIX_FMT_GBRP16BE: return "planar GBR 4:4:4 48bpp, big-endian";
        case AV_PIX_FMT_GBRP16LE: return "planar GBR 4:4:4 48bpp, little-endian";
        case AV_PIX_FMT_YUVA422P: return "planar YUV 4:2:2 24bpp, (1 Cr & Cb sample per 2x1 Y & A samples)";
        case AV_PIX_FMT_YUVA444P: return "planar YUV 4:4:4 32bpp, (1 Cr & Cb sample per 1x1 Y & A samples)";
        case AV_PIX_FMT_YUVA420P9BE: return "planar YUV 4:2:0 22.5bpp, (1 Cr & Cb sample per 2x2 Y & A samples), big-endian";
        case AV_PIX_FMT_YUVA420P9LE: return "planar YUV 4:2:0 22.5bpp, (1 Cr & Cb sample per 2x2 Y & A samples), little-endian";
        case AV_PIX_FMT_YUVA422P9BE: return "planar YUV 4:2:2 27bpp, (1 Cr & Cb sample per 2x1 Y & A samples), big-endian";
        case AV_PIX_FMT_YUVA422P9LE: return "planar YUV 4:2:2 27bpp, (1 Cr & Cb sample per 2x1 Y & A samples), little-endian";
        case AV_PIX_FMT_YUVA444P9BE: return "planar YUV 4:4:4 36bpp, (1 Cr & Cb sample per 1x1 Y & A samples), big-endian";
        case AV_PIX_FMT_YUVA444P9LE: return "planar YUV 4:4:4 36bpp, (1 Cr & Cb sample per 1x1 Y & A samples), little-endian";
        case AV_PIX_FMT_YUVA420P10BE: return "planar YUV 4:2:0 25bpp, (1 Cr & Cb sample per 2x2 Y & A samples, big-endian)";
        case AV_PIX_FMT_YUVA420P10LE: return "planar YUV 4:2:0 25bpp, (1 Cr & Cb sample per 2x2 Y & A samples, little-endian)";
        case AV_PIX_FMT_YUVA422P10BE: return "planar YUV 4:2:2 30bpp, (1 Cr & Cb sample per 2x1 Y & A samples, big-endian)";
        case AV_PIX_FMT_YUVA422P10LE: return "planar YUV 4:2:2 30bpp, (1 Cr & Cb sample per 2x1 Y & A samples, little-endian)";
        case AV_PIX_FMT_YUVA444P10BE: return "planar YUV 4:4:4 40bpp, (1 Cr & Cb sample per 1x1 Y & A samples, big-endian)";
        case AV_PIX_FMT_YUVA444P10LE: return "planar YUV 4:4:4 40bpp, (1 Cr & Cb sample per 1x1 Y & A samples, little-endian)";
        case AV_PIX_FMT_YUVA420P16BE: return "planar YUV 4:2:0 40bpp, (1 Cr & Cb sample per 2x2 Y & A samples, big-endian)";
        case AV_PIX_FMT_YUVA420P16LE: return "planar YUV 4:2:0 40bpp, (1 Cr & Cb sample per 2x2 Y & A samples, little-endian)";
        case AV_PIX_FMT_YUVA422P16BE: return "planar YUV 4:2:2 48bpp, (1 Cr & Cb sample per 2x1 Y & A samples, big-endian)";
        case AV_PIX_FMT_YUVA422P16LE: return "planar YUV 4:2:2 48bpp, (1 Cr & Cb sample per 2x1 Y & A samples, little-endian)";
        case AV_PIX_FMT_YUVA444P16BE: return "planar YUV 4:4:4 64bpp, (1 Cr & Cb sample per 1x1 Y & A samples, big-endian)";
        case AV_PIX_FMT_YUVA444P16LE: return "planar YUV 4:4:4 64bpp, (1 Cr & Cb sample per 1x1 Y & A samples, little-endian)";

        case AV_PIX_FMT_VDPAU:   return "HW acceleration through VDPAU, Picture.data[3] contains a VdpVideoSurface";

        case AV_PIX_FMT_XYZ12LE:    return "packed XYZ 4:4:4, 36 bpp, (msb) 12X, 12Y, 12Z (lsb), the 2-byte value for each X/Y/Z is stored as little-endian, the 4 lower bits are set to 0";
        case AV_PIX_FMT_XYZ12BE:    return "packed XYZ 4:4:4, 36 bpp, (msb) 12X, 12Y, 12Z (lsb), the 2-byte value for each X/Y/Z is stored as big-endian, the 4 lower bits are set to 0";
        case AV_PIX_FMT_NV16:       return "interleaved chroma YUV 4:2:2, 16bpp, (1 Cr & Cb sample per 2x1 Y samples)";
        case AV_PIX_FMT_NV20LE:     return "interleaved chroma YUV 4:2:2, 20bpp, (1 Cr & Cb sample per 2x1 Y samples), little-endian";
        case AV_PIX_FMT_NV20BE:     return "interleaved chroma YUV 4:2:2, 20bpp, (1 Cr & Cb sample per 2x1 Y samples), big-endian";

        case AV_PIX_FMT_RGBA64BE: 	return "packed RGBA 16:16:16:16, 64bpp, 16R, 16G, 16B, 16A, the 2-byte value for each R/G/B/A component is stored as big-endian";
        case AV_PIX_FMT_RGBA64LE: 	return "packed RGBA 16:16:16:16, 64bpp, 16R, 16G, 16B, 16A, the 2-byte value for each R/G/B/A component is stored as little-endian";
        case AV_PIX_FMT_BGRA64BE: 	return "packed RGBA 16:16:16:16, 64bpp, 16B, 16G, 16R, 16A, the 2-byte value for each R/G/B/A component is stored as big-endian";
        case AV_PIX_FMT_BGRA64LE: 	return "packed RGBA 16:16:16:16, 64bpp, 16B, 16G, 16R, 16A, the 2-byte value for each R/G/B/A component is stored as little-endian";

        case AV_PIX_FMT_YVYU422: 	return "packed YUV 4:2:2, 16bpp, Y0 Cr Y1 Cb";

        case AV_PIX_FMT_YA16BE: 	return "16 bits gray, 16 bits alpha (big-endian)";
        case AV_PIX_FMT_YA16LE: 	return "16 bits gray, 16 bits alpha (little-endian)";

        case AV_PIX_FMT_GBRAP: 		return "planar GBRA 4:4:4:4 32bpp";
        case AV_PIX_FMT_GBRAP16BE: 	return "planar GBRA 4:4:4:4 64bpp, big-endian";
        case AV_PIX_FMT_GBRAP16LE: 	return "planar GBRA 4:4:4:4 64bpp, little-endian";
        /**
         *  HW acceleration through QSV, data[3] contains a pointer to the
         *  mfxFrameSurface1 structure.
         */
        case AV_PIX_FMT_QSV: 	return "HW acceleration through QSV";
        /**
         * HW acceleration though MMAL, data[3] contains a pointer to the
         * MMAL_BUFFER_HEADER_T structure.
         */
        case AV_PIX_FMT_MMAL: 	return "HW acceleration though MMAL";

        case AV_PIX_FMT_D3D11VA_VLD: return "HW decoding through Direct3D11 via old API";

        /**
         * HW acceleration through CUDA. data[i] contain CUdeviceptr pointers
         * exactly as for system memory frames.
         */
        case AV_PIX_FMT_CUDA: "HW acceleration through CUDA";

        case AV_PIX_FMT_0RGB: return "packed RGB 8:8:8, 32bpp, XRGBXRGB...   X=unused/undefined";
        case AV_PIX_FMT_RGB0: return "packed RGB 8:8:8, 32bpp, RGBXRGBX...   X=unused/undefined";
        case AV_PIX_FMT_0BGR: return "packed BGR 8:8:8, 32bpp, XBGRXBGR...   X=unused/undefined";
        case AV_PIX_FMT_BGR0: return "packed BGR 8:8:8, 32bpp, BGRXBGRX...   X=unused/undefined";

        case AV_PIX_FMT_YUV420P12BE: return "planar YUV 4:2:0,18bpp, (1 Cr & Cb sample per 2x2 Y samples), big-endian";
        case AV_PIX_FMT_YUV420P12LE: return "planar YUV 4:2:0,18bpp, (1 Cr & Cb sample per 2x2 Y samples), little-endian";
        case AV_PIX_FMT_YUV420P14BE: return "planar YUV 4:2:0,21bpp, (1 Cr & Cb sample per 2x2 Y samples), big-endian";
        case AV_PIX_FMT_YUV420P14LE: return "planar YUV 4:2:0,21bpp, (1 Cr & Cb sample per 2x2 Y samples), little-endian";
        case AV_PIX_FMT_YUV422P12BE: return "planar YUV 4:2:2,24bpp, (1 Cr & Cb sample per 2x1 Y samples), big-endian";
        case AV_PIX_FMT_YUV422P12LE: return "planar YUV 4:2:2,24bpp, (1 Cr & Cb sample per 2x1 Y samples), little-endian";
        case AV_PIX_FMT_YUV422P14BE: return "planar YUV 4:2:2,28bpp, (1 Cr & Cb sample per 2x1 Y samples), big-endian";
        case AV_PIX_FMT_YUV422P14LE: return "planar YUV 4:2:2,28bpp, (1 Cr & Cb sample per 2x1 Y samples), little-endian";
        case AV_PIX_FMT_YUV444P12BE: return "planar YUV 4:4:4,36bpp, (1 Cr & Cb sample per 1x1 Y samples), big-endian";
        case AV_PIX_FMT_YUV444P12LE: return "planar YUV 4:4:4,36bpp, (1 Cr & Cb sample per 1x1 Y samples), little-endian";
        case AV_PIX_FMT_YUV444P14BE: return "planar YUV 4:4:4,42bpp, (1 Cr & Cb sample per 1x1 Y samples), big-endian";
        case AV_PIX_FMT_YUV444P14LE: return "planar YUV 4:4:4,42bpp, (1 Cr & Cb sample per 1x1 Y samples), little-endian";
        case AV_PIX_FMT_GBRP12BE: return "planar GBR 4:4:4 36bpp, big-endian";
        case AV_PIX_FMT_GBRP12LE: return "planar GBR 4:4:4 36bpp, little-endian";
        case AV_PIX_FMT_GBRP14BE: return "planar GBR 4:4:4 42bpp, big-endian";
        case AV_PIX_FMT_GBRP14LE: return "planar GBR 4:4:4 42bpp, little-endian";
        case AV_PIX_FMT_YUVJ411P: return "planar YUV 4:1:1, 12bpp, (1 Cr & Cb sample per 4x1 Y samples) full scale (JPEG), deprecated in favor of AV_PIX_FMT_YUV411P and setting color_range";

        case AV_PIX_FMT_BAYER_BGGR8: return "bayer, BGBG..(odd line), GRGR..(even line), 8-bit samples";
        case AV_PIX_FMT_BAYER_RGGB8: return "bayer, RGRG..(odd line), GBGB..(even line), 8-bit samples";
        case AV_PIX_FMT_BAYER_GBRG8: return "bayer, GBGB..(odd line), RGRG..(even line), 8-bit samples";
        case AV_PIX_FMT_BAYER_GRBG8: return "bayer, GRGR..(odd line), BGBG..(even line), 8-bit samples";
        case AV_PIX_FMT_BAYER_BGGR16LE: return "bayer, BGBG..(odd line), GRGR..(even line), 16-bit samples, little-endian";
        case AV_PIX_FMT_BAYER_BGGR16BE: return "bayer, BGBG..(odd line), GRGR..(even line), 16-bit samples, big-endian";
        case AV_PIX_FMT_BAYER_RGGB16LE: return "bayer, RGRG..(odd line), GBGB..(even line), 16-bit samples, little-endian";
        case AV_PIX_FMT_BAYER_RGGB16BE: return "bayer, RGRG..(odd line), GBGB..(even line), 16-bit samples, big-endian";
        case AV_PIX_FMT_BAYER_GBRG16LE: return "bayer, GBGB..(odd line), RGRG..(even line), 16-bit samples, little-endian";
        case AV_PIX_FMT_BAYER_GBRG16BE: return "bayer, GBGB..(odd line), RGRG..(even line), 16-bit samples, big-endian";
        case AV_PIX_FMT_BAYER_GRBG16LE: return "bayer, GRGR..(odd line), BGBG..(even line), 16-bit samples, little-endian";
        case AV_PIX_FMT_BAYER_GRBG16BE: return "bayer, GRGR..(odd line), BGBG..(even line), 16-bit samples, big-endian";

        case AV_PIX_FMT_XVMC: return "XVideo Motion Acceleration via common packet passing";

        case AV_PIX_FMT_YUV440P10LE: return "planar YUV 4:4:0,20bpp, (1 Cr & Cb sample per 1x2 Y samples), little-endian";
        case AV_PIX_FMT_YUV440P10BE: return "planar YUV 4:4:0,20bpp, (1 Cr & Cb sample per 1x2 Y samples), big-endian";
        case AV_PIX_FMT_YUV440P12LE: return "planar YUV 4:4:0,24bpp, (1 Cr & Cb sample per 1x2 Y samples), little-endian";
        case AV_PIX_FMT_YUV440P12BE: return "planar YUV 4:4:0,24bpp, (1 Cr & Cb sample per 1x2 Y samples), big-endian";
        case AV_PIX_FMT_AYUV64LE: return "packed AYUV 4:4:4,64bpp (1 Cr & Cb sample per 1x1 Y & A samples), little-endian";
        case AV_PIX_FMT_AYUV64BE: return "packed AYUV 4:4:4,64bpp (1 Cr & Cb sample per 1x1 Y & A samples), big-endian";

        case AV_PIX_FMT_VIDEOTOOLBOX: return "hardware decoding through Videotoolbox";

        case AV_PIX_FMT_P010LE: return "NV12, with 10bpp per component, data in the high bits, zeros in the low bits, little-endian";
        case AV_PIX_FMT_P010BE: return "NV12, with 10bpp per component, data in the high bits, zeros in the low bits, big-endian";

        case AV_PIX_FMT_GBRAP12BE: return "planar GBR 4:4:4:4 48bpp, big-endian";
        case AV_PIX_FMT_GBRAP12LE: return "planar GBR 4:4:4:4 48bpp, little-endian";

        case AV_PIX_FMT_GBRAP10BE: return "planar GBR 4:4:4:4 40bpp, big-endian";
        case AV_PIX_FMT_GBRAP10LE: return "planar GBR 4:4:4:4 40bpp, little-endian";

        case AV_PIX_FMT_MEDIACODEC: return "hardware decoding through MediaCodec";

        case AV_PIX_FMT_GRAY12BE: return "Y, 12bpp, big-endian";
        case AV_PIX_FMT_GRAY12LE: return "Y, 12bpp, little-endian";
        case AV_PIX_FMT_GRAY10BE: return "Y, 10bpp, big-endian";
        case AV_PIX_FMT_GRAY10LE: return "Y, 10bpp, little-endian";

        case AV_PIX_FMT_P016LE: return "like NV12, with 16bpp per component, little-endian";
        case AV_PIX_FMT_P016BE: return "like NV12, with 16bpp per component, big-endian";

        /**
         * Hardware surfaces for Direct3D11.
         *
         * This is preferred over the legacy AV_PIX_FMT_D3D11VA_VLD. The new D3D11
         * hwaccel API and filtering support AV_PIX_FMT_D3D11 only.
         *
         * data[0] contains a ID3D11Texture2D pointer, and data[1] contains the
         * texture array index of the frame as intptr_t if the ID3D11Texture2D is
         * an array texture (or always 0 if it's a normal texture).
         */
        case AV_PIX_FMT_D3D11: return "Hardware surfaces for Direct3D11";

        case AV_PIX_FMT_GRAY9BE: return "Y, 9bpp, big-endian";
        case AV_PIX_FMT_GRAY9LE: return "Y, 9bpp, little-endian";

        case AV_PIX_FMT_GBRPF32BE: return "IEEE-754 single precision planar GBR 4:4:4, 96bpp, big-endian";
        case AV_PIX_FMT_GBRPF32LE: return "IEEE-754 single precision planar GBR 4:4:4, 96bpp, little-endian";
        case AV_PIX_FMT_GBRAPF32BE: return "IEEE-754 single precision planar GBRA 4:4:4:4, 128bpp, big-endian";
        case AV_PIX_FMT_GBRAPF32LE: return "IEEE-754 single precision planar GBRA 4:4:4:4, 128bpp, little-endian";

        /**
         * DRM-managed buffers exposed through PRIME buffer sharing.
         *
         * data[0] points to an AVDRMFrameDescriptor.
         */
        case AV_PIX_FMT_DRM_PRIME: return "DRM-managed buffers exposed through PRIME buffer sharing.";
        default: return string();
    }
}

//---------------------------------------------------------------------------
string FFmpeg_Glue::PixFormatName_Get()
{
    inputdata* InputData=NULL;
    for (size_t Pos=0; Pos<InputDatas.size(); Pos++)
        if (InputDatas[Pos] && InputDatas[Pos]->Type==AVMEDIA_TYPE_VIDEO)
        {
            InputData=InputDatas[Pos];
            break;
        }

    if (InputData==NULL || InputData->Stream==NULL || InputData->Stream->codec==NULL)
        return string();

    const AVPixFmtDescriptor* Desc=av_pix_fmt_desc_get(InputData->Stream->codec->pix_fmt);
    if (!Desc)
        return string();
    return Desc->name;
}

QString FFmpeg_Glue::FrameType_Get() const
{
    inputdata* InputData=NULL;
    for (size_t Pos=0; Pos<InputDatas.size(); Pos++)
        if (InputDatas[Pos] && InputDatas[Pos]->Type==AVMEDIA_TYPE_VIDEO)
        {
            InputData=InputDatas[Pos];
            break;
        }

    if (InputData==NULL || InputData->DecodedFrame==NULL)
        return QString();

    return QString("%1").arg(av_get_picture_type_char(InputData->DecodedFrame->pict_type));
}

//---------------------------------------------------------------------------
string FFmpeg_Glue::ColorSpace_Get()
{
    inputdata* InputData=NULL;
    for (size_t Pos=0; Pos<InputDatas.size(); Pos++)
        if (InputDatas[Pos] && InputDatas[Pos]->Type==AVMEDIA_TYPE_VIDEO)
        {
            InputData=InputDatas[Pos];
            break;
        }

    if (InputData==NULL || InputData->Stream==NULL || InputData->Stream->codec==NULL)
        return string();

    switch (InputData->Stream->codec->colorspace)
    {
        case AVCOL_SPC_RGB: return "RGB: order of coefficients is actually GBR, also IEC 61966-2-1 (sRGB)";
        case AVCOL_SPC_BT709: return "BT.709"; // full: "BT.709 / ITU-R BT1361 / IEC 61966-2-4 xvYCC709 / SMPTE RP177 Annex B"
        case AVCOL_SPC_UNSPECIFIED: return "Unspecified";
        case AVCOL_SPC_RESERVED: return "Reserved";
        case AVCOL_SPC_FCC: return "FCC Title 47 Code of Federal Regulations 73.682 (a)(20)";
        case AVCOL_SPC_BT470BG: return "BT.601 PAL";  // full: "BT.470bg / ITU-R BT601-6 625 / ITU-R BT1358 625 / ITU-R BT1700 625 PAL & SECAM / IEC 61966-2-4 xvYCC601"
        case AVCOL_SPC_SMPTE170M: return "BT.601 NTSC"; // full:"SMPTE 170m / ITU-R BT601-6 525 / ITU-R BT1358 525 / ITU-R BT1700 NTSC"
        case AVCOL_SPC_SMPTE240M: return "SMPTE 240m";
        case AVCOL_SPC_YCOCG: return "YCOCG: Used by Dirac / VC-2 and H.264 FRext, see ITU-T SG16";
        case AVCOL_SPC_BT2020_NCL: return "ITU-R BT2020 non-constant luminance system";
        case AVCOL_SPC_BT2020_CL: return "ITU-R BT2020 constant luminance system";
        case AVCOL_SPC_NB: return "Not part of ABI.";
        default: return string();
    }
}

//---------------------------------------------------------------------------
string FFmpeg_Glue::ColorRange_Get()
{
    inputdata* InputData=NULL;
    for (size_t Pos=0; Pos<InputDatas.size(); Pos++)
        if (InputDatas[Pos] && InputDatas[Pos]->Type==AVMEDIA_TYPE_VIDEO)
        {
            InputData=InputDatas[Pos];
            break;
        }

    if (InputData==NULL || InputData->Stream==NULL || InputData->Stream->codec==NULL)
        return string();

    switch (InputData->Stream->codec->color_range)
    {
        case AVCOL_RANGE_UNSPECIFIED: return "Unspecified";
        case AVCOL_RANGE_MPEG: return "Broadcast Range"; // full: "Broadcast Range (219*2^n-1)"
        case AVCOL_RANGE_JPEG: return "Full Range"; // full: "Full Range (2^n-1)"
        case AVCOL_RANGE_NB: return "Not part of ABI";
        default: return string();
    }
}

int FFmpeg_Glue::IsRGB_Get()
{
    inputdata* InputData=NULL;
    for (size_t Pos=0; Pos<InputDatas.size(); Pos++)
        if (InputDatas[Pos] && InputDatas[Pos]->Type==AVMEDIA_TYPE_VIDEO)
        {
            InputData=InputDatas[Pos];
            break;
        }

    if (InputData==NULL || InputData->Stream==NULL || InputData->Stream->codec==NULL)
        return 0;

    const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(InputData->Stream->codec->pix_fmt);
    return (desc->flags & AV_PIX_FMT_FLAG_RGB);
}

int FFmpeg_Glue::BitsPerRawSample_Get(int streamType)
{
    inputdata* InputData=NULL;
    for (size_t Pos=0; Pos<InputDatas.size(); Pos++)
        if (InputDatas[Pos] && InputDatas[Pos]->Type==streamType)
        {
            InputData=InputDatas[Pos];
            break;
        }

    if (InputData==NULL || InputData->Stream==NULL || InputData->Stream->codec==NULL)
        return 0;

    return InputData->Stream->codec->bits_per_raw_sample;
}

//---------------------------------------------------------------------------
string FFmpeg_Glue::AudioFormat_Get()
{
    inputdata* InputData=NULL;
    for (size_t Pos=0; Pos<InputDatas.size(); Pos++)
        if (InputDatas[Pos] && InputDatas[Pos]->Type==AVMEDIA_TYPE_AUDIO)
        {
            InputData=InputDatas[Pos];
            break;
        }

    if (InputData==NULL || InputData->Stream==NULL || InputData->Stream->codec==NULL || InputData->Stream->codec->codec==NULL || InputData->Stream->codec->codec->long_name==NULL)
        return string();

    return InputData->Stream->codec->codec->long_name;
}

//---------------------------------------------------------------------------
string FFmpeg_Glue::SampleFormat_Get()
{
    inputdata* InputData=NULL;
    for (size_t Pos=0; Pos<InputDatas.size(); Pos++)
        if (InputDatas[Pos] && InputDatas[Pos]->Type==AVMEDIA_TYPE_AUDIO)
        {
            InputData=InputDatas[Pos];
            break;
        }
        if (InputData==NULL || InputData->Stream==NULL || InputData->Stream->codec==NULL)
            return "";

        switch (InputData->Stream->codec->sample_fmt)
        {
            case AV_SAMPLE_FMT_NONE: return "none";
            case AV_SAMPLE_FMT_U8: return "unsigned 8 bits";
            case AV_SAMPLE_FMT_S16: return "signed 16 bits";
            case AV_SAMPLE_FMT_S32: return "signed 32 bits";
            case AV_SAMPLE_FMT_FLT: return "float";
            case AV_SAMPLE_FMT_DBL: return "double";
            case AV_SAMPLE_FMT_U8P: return "unsigned 8 bits, planar";
            case AV_SAMPLE_FMT_S16P: return "signed 16 bits, planar";
            case AV_SAMPLE_FMT_S32P: return "signed 32 bits, planar";
            case AV_SAMPLE_FMT_FLTP: return "float, planar";
            case AV_SAMPLE_FMT_DBLP: return "double, planar";
            case AV_SAMPLE_FMT_NB: return "number of sample formats";
            default: return string();
        }
}

int FFmpeg_Glue::sampleFormat()
{
    inputdata* InputData=NULL;
    for (size_t Pos=0; Pos<InputDatas.size(); Pos++)
        if (InputDatas[Pos] && InputDatas[Pos]->Type==AVMEDIA_TYPE_AUDIO)
        {
            InputData=InputDatas[Pos];
            break;
        }

    if (InputData==NULL || InputData->Stream==NULL || InputData->Stream->codec==NULL)
        return AV_SAMPLE_FMT_NONE;

    return InputData->Stream->codec->sample_fmt;
}

//---------------------------------------------------------------------------
int FFmpeg_Glue::SamplingRate_Get()
{
    inputdata* InputData=NULL;
    for (size_t Pos=0; Pos<InputDatas.size(); Pos++)
        if (InputDatas[Pos] && InputDatas[Pos]->Type==AVMEDIA_TYPE_AUDIO)
        {
            InputData=InputDatas[Pos];
            break;
        }

    if (InputData==NULL || InputData->Stream==NULL || InputData->Stream->codec->sample_rate==0)
        return 0;

    return InputData->Stream->codec->sample_rate;
}

//---------------------------------------------------------------------------
string FFmpeg_Glue::ChannelLayout_Get()
{
    inputdata* InputData=NULL;
    for (size_t Pos=0; Pos<InputDatas.size(); Pos++)
        if (InputDatas[Pos] && InputDatas[Pos]->Type==AVMEDIA_TYPE_AUDIO)
        {
            InputData=InputDatas[Pos];
            break;
        }

    if (InputData==NULL || InputData->Stream==NULL || InputData->Stream->codec==NULL || InputData->Stream->codec->codec==NULL)
        return "";

    switch (InputData->Stream->codec->channel_layout)
    {
        case AV_CH_LAYOUT_MONO: return "mono";
        case AV_CH_LAYOUT_STEREO: return "stereo"; 
        case AV_CH_LAYOUT_2POINT1: return "2.1"; 
        case AV_CH_LAYOUT_SURROUND: return "3.0"; 
        case AV_CH_LAYOUT_2_1: return "3.0(back)";
        case AV_CH_LAYOUT_4POINT0: return "4.0";
        case AV_CH_LAYOUT_QUAD: return "quad";
        case AV_CH_LAYOUT_2_2: return "quad(side)";
        case AV_CH_LAYOUT_3POINT1: return "3.1";
        case AV_CH_LAYOUT_5POINT0_BACK: return "5.0";
        case AV_CH_LAYOUT_5POINT0: return "5.0(side)";
        case AV_CH_LAYOUT_4POINT1: return "4.1";
        case AV_CH_LAYOUT_5POINT1_BACK: 
        case AV_CH_LAYOUT_5POINT1: return "5.1(side)";
        case AV_CH_LAYOUT_6POINT0: return "6.0";
        case AV_CH_LAYOUT_6POINT0_FRONT: return "6.0(front)";
        case AV_CH_LAYOUT_HEXAGONAL: return "hexagonal";
        case AV_CH_LAYOUT_6POINT1: return "6.1";
        case AV_CH_LAYOUT_6POINT1_BACK: return "6.1";
        case AV_CH_LAYOUT_6POINT1_FRONT: return "6.1(front)";
        case AV_CH_LAYOUT_7POINT0: return "7.0";
        case AV_CH_LAYOUT_7POINT0_FRONT: return "7.0(front)";
        case AV_CH_LAYOUT_7POINT1: return "7.1";
        case AV_CH_LAYOUT_7POINT1_WIDE_BACK: return "7.1(wide)";
        case AV_CH_LAYOUT_7POINT1_WIDE: return "7.1(wide-side)";
        case AV_CH_LAYOUT_OCTAGONAL: return "octagonal";
        case AV_CH_LAYOUT_STEREO_DOWNMIX: return "downmix";
        default: return string();
    }
}

//---------------------------------------------------------------------------
int FFmpeg_Glue::ABitDepth_Get()
{
    inputdata* InputData=NULL;
    for (size_t Pos=0; Pos<InputDatas.size(); Pos++)
        if (InputDatas[Pos] && InputDatas[Pos]->Type==AVMEDIA_TYPE_AUDIO)
        {
            InputData=InputDatas[Pos];
            break;
        }

    if (InputData==NULL || InputData->Stream==NULL || InputData->Stream->codec==NULL || InputData->Stream->codec->codec==NULL || InputData->Stream->codec->codec->long_name==NULL)
        return 0;

    return InputData->Stream->codec->bits_per_coded_sample;
}

//---------------------------------------------------------------------------
string FFmpeg_Glue::FFmpeg_Version()
{
    return FFMPEG_VERSION;
}

//---------------------------------------------------------------------------
int FFmpeg_Glue::FFmpeg_Year()
{
#ifdef WITH_SYSTEM_FFMPEG
    time_t t = std::time(0);
    struct tm * now = std::localtime(&t);
    return now->tm_year + 1900;
#else
    return CONFIG_THIS_YEAR;
#endif
}

//---------------------------------------------------------------------------
string FFmpeg_Glue::FFmpeg_Compiler()
{
#ifdef WITH_SYSTEM_FFMPEG
    return "not available";
#else
    return CC_IDENT;
#endif
}

//---------------------------------------------------------------------------
string FFmpeg_Glue::FFmpeg_Configuration()
{
#ifdef WITH_SYSTEM_FFMPEG
    return "not available";
#else
    return FFMPEG_CONFIGURATION;
#endif
}

//---------------------------------------------------------------------------
string FFmpeg_Glue::FFmpeg_LibsVersion()
{
    stringstream LibsVersion;
    LIBSVERSION(avutil,     AVUTIL);
    LIBSVERSION(avcodec,    AVCODEC);
    LIBSVERSION(avformat,   AVFORMAT);
    LIBSVERSION(avfilter,   AVFILTER);
    LIBSVERSION(swscale,    SWSCALE);
    return LibsVersion.str();
}

QByteArray FFmpeg_Glue::getAttachment(const QString &fileName, QString& attachmentFileName)
{
    ensureFFMpegInitialized();

    QByteArray attachment;

    // Open file
    AVFormatContext* formatContext = nullptr;
    auto fileNameString = fileName.toStdString();

    auto result = avformat_open_input(&formatContext, fileNameString.c_str(), NULL, NULL);
    if (result >= 0)
    {
        if (avformat_find_stream_info(formatContext, NULL)>=0)
        {
            for(auto i = 0; i < formatContext->nb_streams; ++i)
            {
                if(formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_ATTACHMENT) {
                    auto st = formatContext->streams[i];
                    if(st->codecpar->extradata_size != 0) {
                        attachment = QByteArray((const char*) st->codecpar->extradata, st->codecpar->extradata_size);
                        AVDictionaryEntry *e = av_dict_get(st->metadata, "filename", NULL, 0);
                        if(e) {
                            attachmentFileName = e->value;
                        }
                        break;
                    }
                }
            }
        }
    } else {
        char errbuf[255];
        qDebug() << "Could not open file: " << av_make_error_string(errbuf, sizeof errbuf, result) << "\n";
    }

    avformat_close_input(&formatContext);

    return attachment;
}

int FFmpeg_Glue::guessBitsPerRawSampleFromFormat(int pixelFormat)
{
    switch(pixelFormat) {
        case AV_PIX_FMT_MONOWHITE:
        case AV_PIX_FMT_MONOBLACK:
            return 1;

        case AV_PIX_FMT_RGB444LE:
        case AV_PIX_FMT_RGB444BE:
        case AV_PIX_FMT_BGR444LE:
        case AV_PIX_FMT_BGR444BE:
            return 4;

        case AV_PIX_FMT_RGB565BE:
        case AV_PIX_FMT_RGB565LE:
        case AV_PIX_FMT_RGB555BE:
        case AV_PIX_FMT_RGB555LE:
        case AV_PIX_FMT_BGR565BE:
        case AV_PIX_FMT_BGR565LE:
        case AV_PIX_FMT_BGR555BE:
        case AV_PIX_FMT_BGR555LE:
            return 6;

        case AV_PIX_FMT_YUV420P9BE:
        case AV_PIX_FMT_YUV420P9LE:
        case AV_PIX_FMT_YUV444P9BE:
        case AV_PIX_FMT_YUV444P9LE:
        case AV_PIX_FMT_YUV422P9BE:
        case AV_PIX_FMT_YUV422P9LE:
        case AV_PIX_FMT_GBRP9BE:
        case AV_PIX_FMT_GBRP9LE:
        case AV_PIX_FMT_YUVA420P9BE:
        case AV_PIX_FMT_YUVA420P9LE:
        case AV_PIX_FMT_YUVA422P9BE:
        case AV_PIX_FMT_YUVA422P9LE:
        case AV_PIX_FMT_YUVA444P9BE:
        case AV_PIX_FMT_YUVA444P9LE:
        case AV_PIX_FMT_GRAY9BE:
        case AV_PIX_FMT_GRAY9LE:
            return 9;

        case AV_PIX_FMT_YUV420P10BE:
        case AV_PIX_FMT_YUV420P10LE:
        case AV_PIX_FMT_YUV422P10BE:
        case AV_PIX_FMT_YUV422P10LE:
        case AV_PIX_FMT_YUV444P10BE:
        case AV_PIX_FMT_YUV444P10LE:
        case AV_PIX_FMT_GBRP10BE:
        case AV_PIX_FMT_GBRP10LE:
        case AV_PIX_FMT_YUVA420P10BE:
        case AV_PIX_FMT_YUVA420P10LE:
        case AV_PIX_FMT_YUVA422P10BE:
        case AV_PIX_FMT_YUVA422P10LE:
        case AV_PIX_FMT_YUVA444P10BE:
        case AV_PIX_FMT_YUVA444P10LE:
        case AV_PIX_FMT_YUV440P10LE:
        case AV_PIX_FMT_YUV440P10BE:
        case AV_PIX_FMT_GBRAP10BE:
        case AV_PIX_FMT_GBRAP10LE:
        case AV_PIX_FMT_GRAY10BE:
        case AV_PIX_FMT_GRAY10LE:
            return 10;

        case AV_PIX_FMT_XYZ12LE:
        case AV_PIX_FMT_XYZ12BE:
        case AV_PIX_FMT_YUV420P12BE:
        case AV_PIX_FMT_YUV420P12LE:
        case AV_PIX_FMT_YUV422P12BE:
        case AV_PIX_FMT_YUV422P12LE:
        case AV_PIX_FMT_YUV444P12BE:
        case AV_PIX_FMT_YUV444P12LE:
        case AV_PIX_FMT_GBRP12BE:
        case AV_PIX_FMT_GBRP12LE:
        case AV_PIX_FMT_YUV440P12LE:
        case AV_PIX_FMT_YUV440P12BE:
        case AV_PIX_FMT_GBRAP12BE:
        case AV_PIX_FMT_GBRAP12LE:
        case AV_PIX_FMT_GRAY12BE:
        case AV_PIX_FMT_GRAY12LE:
            return 12;

        case AV_PIX_FMT_YUV420P14BE:
        case AV_PIX_FMT_YUV420P14LE:
        case AV_PIX_FMT_YUV422P14BE:
        case AV_PIX_FMT_YUV422P14LE:
        case AV_PIX_FMT_YUV444P14BE:
        case AV_PIX_FMT_YUV444P14LE:
        case AV_PIX_FMT_GBRP14BE:
        case AV_PIX_FMT_GBRP14LE:
            return 14;

        case AV_PIX_FMT_GRAY16BE:
        case AV_PIX_FMT_YUV420P16LE:
        case AV_PIX_FMT_YUV420P16BE:
        case AV_PIX_FMT_YUV422P16LE:
        case AV_PIX_FMT_YUV422P16BE:
        case AV_PIX_FMT_YUV444P16LE:
        case AV_PIX_FMT_YUV444P16BE:
        case AV_PIX_FMT_GRAY16LE:
        case AV_PIX_FMT_RGB48BE:
        case AV_PIX_FMT_RGB48LE:
        case AV_PIX_FMT_BGR48BE:
        case AV_PIX_FMT_BGR48LE:
        case AV_PIX_FMT_GBRP16BE:
        case AV_PIX_FMT_GBRP16LE:
        case AV_PIX_FMT_YUVA420P16BE:
        case AV_PIX_FMT_YUVA420P16LE:
        case AV_PIX_FMT_YUVA422P16BE:
        case AV_PIX_FMT_YUVA422P16LE:
        case AV_PIX_FMT_YUVA444P16BE:
        case AV_PIX_FMT_YUVA444P16LE:
        case AV_PIX_FMT_RGBA64BE:
        case AV_PIX_FMT_RGBA64LE:
        case AV_PIX_FMT_BGRA64BE:
        case AV_PIX_FMT_BGRA64LE:
        case AV_PIX_FMT_YA16BE:
        case AV_PIX_FMT_YA16LE:
        case AV_PIX_FMT_GBRAP16BE:
        case AV_PIX_FMT_GBRAP16LE:
        case AV_PIX_FMT_BAYER_BGGR16LE:
        case AV_PIX_FMT_BAYER_BGGR16BE:
        case AV_PIX_FMT_BAYER_RGGB16LE:
        case AV_PIX_FMT_BAYER_RGGB16BE:
        case AV_PIX_FMT_BAYER_GBRG16LE:
        case AV_PIX_FMT_BAYER_GBRG16BE:
        case AV_PIX_FMT_BAYER_GRBG16LE:
        case AV_PIX_FMT_BAYER_GRBG16BE:
        case AV_PIX_FMT_AYUV64LE:
            return 16;
        default:
            return 8;
    }
}

int FFmpeg_Glue::bitsPerAudioSample(int audioFormat)
{
    return av_get_bytes_per_sample((AVSampleFormat) audioFormat);
}

bool FFmpeg_Glue::isFloatAudioSampleFormat(int audioFormat)
{
    return audioFormat == AV_SAMPLE_FMT_FLT
            || audioFormat == AV_SAMPLE_FMT_FLTP
            || audioFormat == AV_SAMPLE_FMT_DBL
            || audioFormat == AV_SAMPLE_FMT_DBLP;
}

bool FFmpeg_Glue::isSignedAudioSampleFormat(int audioFormat)
{
    return audioFormat == AV_SAMPLE_FMT_S16
            || audioFormat == AV_SAMPLE_FMT_S16P
            || audioFormat == AV_SAMPLE_FMT_S32
            || audioFormat == AV_SAMPLE_FMT_S32P
            || audioFormat == AV_SAMPLE_FMT_S64
            || audioFormat == AV_SAMPLE_FMT_S64P;
}

bool FFmpeg_Glue::isUnsignedAudioSampleFormat(int audioFormat)
{
    return audioFormat == AV_SAMPLE_FMT_U8
            || audioFormat == AV_SAMPLE_FMT_U8P;
}

QByteArray FFmpeg_Glue::toByteArray(AVPacket *avpacket)
{
    return QByteArray(reinterpret_cast<char*> (avpacket->data), avpacket->size);
}

FFmpeg_Glue::Image::Image()
{

}

const uchar *FFmpeg_Glue::Image::data() const
{
    return *frame->data;
}

int FFmpeg_Glue::Image::width() const
{
    return frame->width;
}

int FFmpeg_Glue::Image::height() const
{
    return frame->height;
}

int FFmpeg_Glue::Image::linesize() const
{
    return *frame->linesize;
}

void FFmpeg_Glue::Image::free()
{
    frame.reset();
}


