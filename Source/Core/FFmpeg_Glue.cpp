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
#include <QImage>
#include <QXmlStreamReader>

extern "C"
{
#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#endif

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>

#include <libavfilter/avfiltergraph.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>

#include <libavutil/imgutils.h>

#include <config.h>
#include <libavutil/ffversion.h>
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
    }                                                                               \

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
    FramesCache_Default(NULL),

    // Encode
    Encode_FormatContext(NULL),
    Encode_CodecContext(NULL),
    Encode_Stream(NULL),
    Encode_Packet(NULL),
    Encode_CodecID(AV_CODEC_ID_NONE)
{
}

FFmpeg_Glue::inputdata::~inputdata()
{
    // FFmpeg pointers - Input
    if (Stream)
        avcodec_close(Stream->codec);

    // Encode
    CloseEncode();

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

//---------------------------------------------------------------------------
bool FFmpeg_Glue::inputdata::InitEncode()
{
    if (Type==AVMEDIA_TYPE_VIDEO)
    {
        AVCodec *Encode_Codec;
        if (Encode_CodecID==AV_CODEC_ID_NONE)
            Encode_Codec=avcodec_find_encoder(Stream->codec->codec_id);
        else
            Encode_Codec=avcodec_find_encoder((AVCodecID)Encode_CodecID);
        if (!Encode_Codec)
            return false;

        Encode_Stream=avformat_new_stream(Encode_FormatContext, Encode_Codec);
        if (TimecodeBCD != -1)
        {
            char timecode[12];
            timecode[ 0] = '0' + ((TimecodeBCD>>28)&0xF);
            timecode[ 1] = '0' + ((TimecodeBCD>>24)&0xF);
            timecode[ 2] = ':';
            timecode[ 3] = '0' + ((TimecodeBCD>>20)&0xF);
            timecode[ 4] = '0' + ((TimecodeBCD>>16)&0xF);
            timecode[ 5] = ':';
            timecode[ 6] = '0' + ((TimecodeBCD>>12)&0xF);
            timecode[ 7] = '0' + ((TimecodeBCD>> 8)&0xF);
            timecode[ 8] = ';';
            timecode[ 9] = '0' + ((TimecodeBCD>> 4)&0xF);
            timecode[10] = '0' + ( TimecodeBCD     &0xF);
            timecode[11] = '\0';
            av_dict_set(&Encode_Stream->metadata, "timecode", timecode, 0);
        }
        Encode_Stream->id=Encode_FormatContext->nb_streams-1;

        Encode_CodecContext=Encode_Stream->codec;
        if (!Encode_CodecContext)
            return false;
        Encode_CodecContext->flags         = CODEC_FLAG_GLOBAL_HEADER;
        Encode_CodecContext->width         = Stream->codec->width;
        Encode_CodecContext->height        = Stream->codec->height;
        if (Encode_CodecID==AV_CODEC_ID_NONE)
            Encode_CodecContext->pix_fmt   = Stream->codec->pix_fmt;
        else if (Stream->codec->bits_per_raw_sample==10)
            Encode_CodecContext->pix_fmt   = AV_PIX_FMT_YUV422P10;
        else
            Encode_CodecContext->pix_fmt   = AV_PIX_FMT_YUV422P;
        Encode_CodecContext->time_base.num = Stream->codec->time_base.num;
        Encode_CodecContext->time_base.den = Stream->codec->time_base.den;
        Encode_CodecContext->sample_aspect_ratio.num = 9;
        Encode_CodecContext->sample_aspect_ratio.den = 10;
        Encode_CodecContext->field_order   = AV_FIELD_BT;
        if (avcodec_open2(Encode_CodecContext, Encode_Codec, NULL) < 0)
            return false;
    }

    if (Type==AVMEDIA_TYPE_AUDIO)
    {
        AVCodec *Encode_Codec;
        if (Encode_CodecID==AV_CODEC_ID_NONE)
            Encode_Codec=avcodec_find_encoder(Stream->codec->codec_id);
        else
            Encode_Codec=avcodec_find_encoder((AVCodecID)Encode_CodecID);
        if (!Encode_Codec)
            return false;

        Encode_Stream=avformat_new_stream(Encode_FormatContext, Encode_Codec);
        Encode_Stream->id=Encode_FormatContext->nb_streams-1;

        Encode_CodecContext=Encode_Stream->codec;
        if (!Encode_CodecContext)
            return false;
        Encode_CodecContext->bits_per_raw_sample=Stream->codec->bits_per_raw_sample;
        Encode_CodecContext->sample_rate   = Stream->codec->sample_rate;
        Encode_CodecContext->channels      = Stream->codec->channels;
        Encode_CodecContext->channel_layout= Stream->codec->channel_layout;
        Encode_CodecContext->sample_fmt    =  Stream->codec->sample_fmt;
        if (avcodec_open2(Encode_CodecContext, Encode_Codec, NULL) < 0)
            return false;
    }

    return true;
}

//---------------------------------------------------------------------------
void FFmpeg_Glue::inputdata::Encode(AVPacket* SourcePacket)
{
    if (!Encode_CodecContext && !InitEncode())
    {
        return;
    }
                                    
    AVPacket TempPacket=*SourcePacket;
    TempPacket.pts=AV_NOPTS_VALUE;
    TempPacket.dts=AV_NOPTS_VALUE;

    if (Encode_CodecID==AV_CODEC_ID_NONE)
        av_interleaved_write_frame(Encode_FormatContext, &TempPacket);
    else if (Encode_CodecID==AV_CODEC_ID_PCM_S24LE)
    {
        // 32 to 24-bit
        TempPacket.size=SourcePacket->size*3/4;
        TempPacket.data=new unsigned char[SourcePacket->size];
        unsigned char* Source=SourcePacket->data;
        unsigned char* Source_End=SourcePacket->data+SourcePacket->size;
        unsigned char* Dest=TempPacket.data;
        while (Source<Source_End)
        {
            ++Source;
            *(Dest++)=*(Source++);
            *(Dest++)=*(Source++);
            *(Dest++)=*(Source++);
        }
        av_interleaved_write_frame(Encode_FormatContext, &TempPacket);
    }
    else if (Encode_CodecID==AV_CODEC_ID_FFV1)
    {
        // FFV1
        AVFrame* SourceFrame = av_frame_alloc();
        unsigned char* SourceFrame_Data = NULL;
        int SourceFrame_Size = -1;
        if (Stream->codec->bits_per_raw_sample==10)
        {
            int SourceFrame_Size = avpicture_get_size(Stream->codec->pix_fmt, Stream->codec->width, Stream->codec->height);
            SourceFrame_Data = new unsigned char[SourceFrame_Size];
            avpicture_fill((AVPicture*)SourceFrame, SourceFrame_Data, Stream->codec->pix_fmt, Stream->codec->width, Stream->codec->height);
            int got_frame;
            int Bytes=avcodec_decode_video2(Stream->codec, SourceFrame, &got_frame, SourcePacket);
            got_frame=0;
        }
        else
        {
            SourceFrame->width = Stream->codec->width;
            SourceFrame->height = Stream->codec->height;
            SourceFrame->format = Stream->codec->pix_fmt;
            avpicture_fill((AVPicture*)SourceFrame, SourcePacket->data, (AVPixelFormat)SourceFrame->format, SourceFrame->width, SourceFrame->height);
        }

        AVFrame* DestFrame = av_frame_alloc();
        DestFrame->width = Stream->codec->width;
        DestFrame->height = Stream->codec->height;
        if (Stream->codec->bits_per_raw_sample==10)
            DestFrame->format = AV_PIX_FMT_YUV422P10;
        else
            DestFrame->format = AV_PIX_FMT_YUV422P;
        int DestFrame_Size = avpicture_get_size((AVPixelFormat)DestFrame->format, DestFrame->width, DestFrame->height);
        unsigned char* DestFrame_Data = new unsigned char[DestFrame_Size];
        avpicture_fill((AVPicture*)DestFrame, DestFrame_Data, (AVPixelFormat)DestFrame->format, DestFrame->width, DestFrame->height);

        struct SwsContext* Context = sws_getContext(SourceFrame->width, SourceFrame->height, (AVPixelFormat)SourceFrame->format, DestFrame->width, DestFrame->height, (AVPixelFormat)DestFrame->format, SWS_FAST_BILINEAR, NULL, NULL, NULL);
        int Result = sws_scale(Context, SourceFrame->data, SourceFrame->linesize, 0, SourceFrame->height, DestFrame->data, DestFrame->linesize);

        AVPacket TempPacket;

        av_init_packet(&TempPacket);
        TempPacket.data=NULL;
        TempPacket.size=0;
        TempPacket.stream_index=0;
        int got_packet=0;
        if (avcodec_encode_video2(Encode_CodecContext, &TempPacket, DestFrame, &got_packet) < 0 || !got_packet)
        {
            delete[] DestFrame_Data;
            return;
        }

        av_interleaved_write_frame(Encode_FormatContext, &TempPacket);
        delete[] SourceFrame_Data;
        delete[] DestFrame_Data;
    }
}

//---------------------------------------------------------------------------
void FFmpeg_Glue::inputdata::CloseEncode()
{
    Encode_CodecContext=NULL;
}

//***************************************************************************
// outputdata
//***************************************************************************

FFmpeg_Glue::outputdata::outputdata()
    :
    // In
    Enabled(true),
    FilterPos(false),

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
    ScaledFrame(NULL),

    // FFmpeg pointers - Output
    JpegOutput_CodecContext(NULL),
    JpegOutput_Packet(NULL),

    // Out
    OutputMethod(Output_None),
    Image(NULL),
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
    delete Image;
    for (size_t Pos=0; Pos<Thumbnails.size(); Pos++)
        delete Thumbnails[Pos];

    // FFmpeg pointers - Output
    if (JpegOutput_Packet)
    {
        av_free_packet(JpegOutput_Packet);
        delete JpegOutput_Packet;
    }
    if (JpegOutput_CodecContext)
        avcodec_free_context(&JpegOutput_CodecContext);

    // FFmpeg pointers - Scale
    if (ScaledFrame)
    {
        avpicture_free((AVPicture*)ScaledFrame);
        av_frame_free(&ScaledFrame);
    }
    if (ScaleContext)
        sws_freeContext(ScaleContext);

    // FFmpeg pointers - Filter
    if (FilterGraph)
        avfilter_graph_free(&FilterGraph);
}

//---------------------------------------------------------------------------
void FFmpeg_Glue::outputdata::Process(AVFrame* DecodedFrame_)
{
    DecodedFrame=DecodedFrame_;
    
    //Filtering
    ApplyFilter();

    // Stats
    if (Stats && FilteredFrame && !Filter.empty())
    {
        Stats->TimeStampFromFrame(FilteredFrame, FramePos-1);
        Stats->StatsFromFrame(FilteredFrame, Stream->codec->width, Stream->codec->height);
    }

    // Scale
    ApplyScale();

    // Output
    switch (OutputMethod)
    {
        case Output_QImage  :   ReplaceImage(); break;
        case Output_Jpeg    :   AddThumbnail();  break;
        default             :   ;
    }

    // Clean up
    DiscardScaledFrame();

    // Clean up
    DiscardFilteredFrame();
}

//---------------------------------------------------------------------------
void FFmpeg_Glue::outputdata::ApplyFilter()
{
    if (Filter.empty())
    {
        FilteredFrame=DecodedFrame;
        FramePos++;
        return;
    }

    if (!FilterGraph && !FilterGraph_Init())
    {
        FilteredFrame=DecodedFrame;
        FramePos++;
        return;
    }
            
    // Push the decoded Frame into the filtergraph 
    //if (av_buffersrc_add_frame_flags(FilterGraph_Source_Context, DecodedFrame, AV_BUFFERSRC_FLAG_KEEP_REF)<0)
    if (av_buffersrc_add_frame_flags(FilterGraph_Source_Context, DecodedFrame, 0)<0)
    {
        FilteredFrame=DecodedFrame;
        FramePos++;
        return;
    }

    // Pull filtered frames from the filtergraph 
    FilteredFrame = av_frame_alloc();
    int GetAnswer = av_buffersink_get_frame(FilterGraph_Sink_Context, FilteredFrame); //TODO: handling of multiple output per input
    if (GetAnswer==AVERROR(EAGAIN) || GetAnswer==AVERROR_EOF)
    {
        av_frame_free(&FilteredFrame);
        FilteredFrame=NULL;
        return;
    }
    if (GetAnswer<0)
    {
        av_frame_free(&FilteredFrame);
        FilteredFrame=DecodedFrame;
        return;
    }

    FramePos++;
}

//---------------------------------------------------------------------------
void FFmpeg_Glue::outputdata::ApplyScale()
{
    if (!FilteredFrame)
        return;

    switch (OutputMethod)
    {
        case Output_Jpeg:
        case Output_QImage:
                            break;
        default: 
                            return;
    }

    if (!ScaleContext && !Scale_Init())
        return;

    ScaledFrame = av_frame_alloc();
    ScaledFrame->width=Width;
    ScaledFrame->height=Height;
    avpicture_alloc((AVPicture*)ScaledFrame, OutputMethod==Output_QImage?PIX_FMT_RGB24:PIX_FMT_YUVJ420P, Width, Height);
    if (sws_scale(ScaleContext, FilteredFrame->data, FilteredFrame->linesize, 0, FilteredFrame->height, ScaledFrame->data, ScaledFrame->linesize)<0)
    {
        avpicture_free((AVPicture*)ScaledFrame);
        av_frame_free(&ScaledFrame);
        ScaledFrame=FilteredFrame;
    }
}

//---------------------------------------------------------------------------
void FFmpeg_Glue::outputdata::ReplaceImage()
{
    if (!ScaledFrame)
        return;    
        
    // Convert the Frame to QImage
    if (Image==NULL)
        Image=new QImage(ScaledFrame->width, ScaledFrame->height, QImage::Format_RGB888);
    for(int y=0;y<ScaledFrame->height;y++)
        memcpy(Image->scanLine(y), ScaledFrame->data[0]+y*ScaledFrame->linesize[0], ScaledFrame->width*3);
}

//---------------------------------------------------------------------------
void FFmpeg_Glue::outputdata::AddThumbnail()
{
    if (Thumbnails.size()%Thumbnails_Modulo)
    {
        Thumbnails.push_back(NULL);
        return; // Not wanting to saturate memory. TODO: Find a smarter way to detect memory usage
    }
        
    int got_packet=0;
    if (!JpegOutput_CodecContext && !InitThumnails())
    {
        Thumbnails.push_back(new bytes());
        return;
    }
                                    
    JpegOutput_Packet->data=NULL;
    JpegOutput_Packet->size=0;
    if (avcodec_encode_video2(JpegOutput_CodecContext, JpegOutput_Packet, ScaledFrame, &got_packet) < 0 || !got_packet)
    {
        Thumbnails.push_back(new bytes());
        return;
    }

    bytes* JpegItem=new bytes;
    JpegItem->Data=new unsigned char[JpegOutput_Packet->size];
    memcpy(JpegItem->Data, JpegOutput_Packet->data, JpegOutput_Packet->size);
    JpegItem->Size=JpegOutput_Packet->size;
    Thumbnails.push_back(JpegItem);
}

//---------------------------------------------------------------------------
void FFmpeg_Glue::outputdata::DiscardScaledFrame()
{
    if (!ScaledFrame)
        return;

    if (ScaledFrame!=FilteredFrame)
    {
        avpicture_free((AVPicture*)ScaledFrame);
        av_frame_free(&ScaledFrame);
    }
    ScaledFrame=NULL;
}

//---------------------------------------------------------------------------
void FFmpeg_Glue::outputdata::DiscardFilteredFrame()
{
    if (!FilteredFrame)
        return;

    if (FilteredFrame!=DecodedFrame)
    {
        av_frame_unref(FilteredFrame);
        av_frame_free(&FilteredFrame);
    }
    FilteredFrame=NULL;
}

//---------------------------------------------------------------------------
bool FFmpeg_Glue::outputdata::InitThumnails()
{
    //
    //Thumbnails.reserve(FrameCount_Max);

    //
    JpegOutput_Packet=new AVPacket;
    av_init_packet (JpegOutput_Packet);
   
    //
    AVCodec *JpegOutput_Codec=avcodec_find_encoder(CODEC_ID_MJPEG);
    if (!JpegOutput_Codec)
        return false;
    JpegOutput_CodecContext=avcodec_alloc_context3 (JpegOutput_Codec);
    if (!JpegOutput_CodecContext)
        return false;
    JpegOutput_CodecContext->qmin          = 8;
    JpegOutput_CodecContext->qmax          = 12;
    JpegOutput_CodecContext->width         = Width;
    JpegOutput_CodecContext->height        = Height;
    JpegOutput_CodecContext->pix_fmt       = PIX_FMT_YUVJ420P;
    JpegOutput_CodecContext->time_base.num = Stream->codec->time_base.num;
    JpegOutput_CodecContext->time_base.den = Stream->codec->time_base.den;
    if (avcodec_open2(JpegOutput_CodecContext, JpegOutput_Codec, NULL) < 0)
        return false;

    // All is OK
    return true;
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
    AVFilter*       Source;
    AVFilter*       Sink;
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
                <<":channel_layout=0x"          << std::hex << Stream->codec->channel_layout;
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
bool FFmpeg_Glue::outputdata::Scale_Init()
{
    if (!FilteredFrame)
        return false;    
        
    if (!AdaptDAR())
        return false;
    
    // Init
    ScaleContext = sws_getContext(FilteredFrame->width, FilteredFrame->height,
                                    (AVPixelFormat)FilteredFrame->format,
                                    Width, Height,
                                    OutputMethod==Output_QImage?PIX_FMT_RGB24:PIX_FMT_YUVJ420P,
                                    Output_QImage?SWS_BICUBIC:SWS_FAST_BILINEAR, NULL, NULL, NULL);
    ScaledFrame=av_frame_alloc();
    ScaledFrame->width=Width;
    ScaledFrame->height=Height;
    avpicture_alloc((AVPicture*)ScaledFrame, OutputMethod==Output_QImage?PIX_FMT_RGB24:PIX_FMT_YUVJ420P, Width, Height);

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

    if (ScaledFrame)
    {
        avpicture_free((AVPicture*)ScaledFrame);
        av_frame_free(&ScaledFrame);
        ScaledFrame=NULL;
    }
}

//---------------------------------------------------------------------------
bool FFmpeg_Glue::outputdata::AdaptDAR()
{
    // Display aspect ratio
    double DAR;
    if (!DecodedFrame)
        DAR=4.0/3.0; // TODO: video frame DAR
    else if (DecodedFrame->sample_aspect_ratio.num && DecodedFrame->sample_aspect_ratio.den)
        DAR=((double)DecodedFrame->width)/DecodedFrame->height*DecodedFrame->sample_aspect_ratio.num/DecodedFrame->sample_aspect_ratio.den;
    else
        DAR=((double)DecodedFrame->width)/DecodedFrame->height;
    if (DAR)
    {
        int Target_Width=Height*DAR;
        int Target_Height=Width/DAR;
        if (Target_Width>Width)
            Height=Target_Height;
        if (Target_Height>Height)
            Width=Target_Width;
    }

    return true;
}

//***************************************************************************
// Constructor / Destructor
//***************************************************************************

//---------------------------------------------------------------------------
FFmpeg_Glue::FFmpeg_Glue (const string &FileName_, std::vector<CommonStats*>* Stats_, bool WithStats_) :
    Stats(Stats_),
    WithStats(WithStats_),
    FileName(FileName_),
    InputDatas_Copy(false),

    // Encode
    Encode_FormatContext(NULL)
{
    // FFmpeg init
    av_register_all();
    avfilter_register_all();
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
        for (size_t Pos=0; Pos<InputDatas.size(); Pos++)
        {
            inputdata* InputData=InputDatas[Pos];

            CommonStats* Stat=NULL;
            if (InputData)
            {
                switch (InputData->Type)
                {
                    case AVMEDIA_TYPE_VIDEO: Stat=new VideoStats(InputData->FrameCount, InputData->Duration, InputData->Stream?(((double)InputData->Stream->time_base.den)/InputData->Stream->time_base.num):0); break;
                    case AVMEDIA_TYPE_AUDIO: Stat=new AudioStats(InputData->FrameCount, InputData->Duration, InputData->Stream?(((double)InputData->Stream->time_base.den)/InputData->Stream->time_base.num):0); break;
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
    CloseEncode();

    if (Packet)
    {
        //av_free_packet(Packet);
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

//***************************************************************************
// Actions
//***************************************************************************

//---------------------------------------------------------------------------
void FFmpeg_Glue::AddInput_Video(size_t FrameCount, int time_base_num, int time_base_den, int Width, int Height, int BitDepth, bool Compression, int TimecodeBCD)
{
    if (!FormatContext && avformat_alloc_output_context2(&FormatContext, NULL, "mpeg", NULL)<0)
        return;

    enum AVCodecID codec_id;
    enum AVPixelFormat pix_fmt;
    switch (BitDepth)
    {
        case  8: codec_id=AV_CODEC_ID_RAWVIDEO; pix_fmt=AV_PIX_FMT_UYVY422; break;
        case 10: codec_id=AV_CODEC_ID_V210;     pix_fmt=AV_PIX_FMT_NONE; break;
        default: return; // Not supported
    }
    inputdata* InputData=new inputdata;
    InputData->Type=AVMEDIA_TYPE_VIDEO;
    InputData->Stream=avformat_new_stream(FormatContext, NULL);
    InputData->Stream->time_base.num=time_base_num;
    InputData->Stream->time_base.den=time_base_den;
    InputData->Stream->duration=FrameCount;
    AVCodec* Codec=avcodec_find_decoder(codec_id);
    AVCodecContext* CodecContext=avcodec_alloc_context3(Codec);
    CodecContext->pix_fmt=pix_fmt;
    CodecContext->width=Width;
    CodecContext->height=Height;
    CodecContext->time_base.num=time_base_num;
    CodecContext->time_base.den=time_base_den;
    CodecContext->field_order=AV_FIELD_BT;
    if (avcodec_open2(CodecContext, Codec, NULL)<0)
        return;
    InputData->Stream->codec=CodecContext;

    InputData->FrameCount=FrameCount;
    InputData->Duration=((double)FrameCount)*time_base_num/time_base_den;
    InputData->TimecodeBCD=TimecodeBCD;

    // Stats
    if (WithStats)
        Stats->push_back(new VideoStats(InputData->FrameCount, InputData->Duration, InputData->Stream?(((double)InputData->Stream->time_base.den)/InputData->Stream->time_base.num):0));

    //
    InputDatas.push_back(InputData);

    // Encode
    if (Compression)
        InputData->Encode_CodecID=AV_CODEC_ID_FFV1;
}

//---------------------------------------------------------------------------
void FFmpeg_Glue::AddInput_Audio(size_t FrameCount, int time_base_num, int time_base_den, int Samplerate, int BitDepth, int OutputBitDepth, int Channels)
{
    if (!FormatContext && avformat_alloc_output_context2(&FormatContext, NULL, "mpeg", NULL)<0)
        return;

    enum AVCodecID codec_id;
    switch (BitDepth)
    {
        case 16: codec_id=AV_CODEC_ID_PCM_S16LE; break;
        case 32: codec_id=AV_CODEC_ID_PCM_S32LE; break;
        default: return; // Not supported
    }

    inputdata* InputData=new inputdata;
    InputData->Type=AVMEDIA_TYPE_AUDIO;
    InputData->Stream=avformat_new_stream(FormatContext, NULL);
    InputData->Stream->time_base.num=time_base_num;
    InputData->Stream->time_base.den=time_base_den;
    InputData->Stream->duration=FrameCount;
    AVCodec* Codec=avcodec_find_decoder(codec_id);
    AVCodecContext* CodecContext=avcodec_alloc_context3(Codec);
    CodecContext->sample_rate=Samplerate;
    CodecContext->channels=Channels;
    if (avcodec_open2(CodecContext, Codec, NULL)<0)
        return;
    InputData->Stream->codec=CodecContext;
    InputData->Stream->codec->channel_layout=av_get_default_channel_layout(CodecContext->channels);

    InputData->FrameCount=FrameCount;
    InputData->Duration=((double)FrameCount)*time_base_num/time_base_den;

    // Stats
    if (WithStats)
        Stats->push_back(new AudioStats(InputData->FrameCount, InputData->Duration, InputData->Stream?(((double)InputData->Stream->time_base.den)/InputData->Stream->time_base.num):0));

    //
    InputDatas.push_back(InputData);

    // Encode
    switch (OutputBitDepth)
    {
        case 24: InputData->Encode_CodecID=AV_CODEC_ID_PCM_S24LE; break;
        default: ;
    }
 }

//---------------------------------------------------------------------------
void FFmpeg_Glue::AddOutput(size_t FilterPos, int Scale_Width, int Scale_Height, outputmethod OutputMethod, int FilterType, const string &Filter)
{
    for (size_t InputPos=0; InputPos<InputDatas.size(); InputPos++)
    {
        inputdata* InputData=InputDatas[InputPos];

        if (InputData && InputData->Type==FilterType)
        {
            OutputDatas.push_back(NULL);
            ModifyOutput(InputPos, OutputDatas.size()-1, FilterPos, Scale_Width, Scale_Height, OutputMethod, FilterType, Filter);
        }
    }
}

//---------------------------------------------------------------------------
void FFmpeg_Glue::AddOutput(const string &FileName, const string &Format)
{
    Encode_FileName=FileName;
    Encode_Format=Format;
}

//---------------------------------------------------------------------------
void FFmpeg_Glue::CloseOutput()
{
    CloseEncode();

    // Complete
    if (WithStats)
        for (size_t Pos=0; Pos<Stats->size(); Pos++)
            if ((*Stats)[Pos])
                (*Stats)[Pos]->StatsFinish();
}

//---------------------------------------------------------------------------
void FFmpeg_Glue::ModifyOutput(size_t InputPos, size_t OutputPos, size_t FilterPos, int Scale_Width, int Scale_Height, outputmethod OutputMethod, int FilterType, const string &Filter)
{
    if (InputPos>=InputDatas.size())
        return;
    inputdata* InputData=InputDatas[InputPos];

    outputdata* OutputData=new outputdata;
    OutputData->Type=FilterType;
    OutputData->Width=Scale_Width;
    OutputData->Height=Scale_Height;
    OutputData->OutputMethod=OutputMethod;
    OutputData->Filter=Filter;
    OutputData->FilterPos=FilterPos;

    OutputData->Stream=InputData->Stream;
    if (OutputMethod==Output_Stats && Stats)
        OutputData->Stats=(*Stats)[InputPos];

    delete OutputDatas[OutputPos];
    OutputDatas[OutputPos]=OutputData;
}

//---------------------------------------------------------------------------
void FFmpeg_Glue::Seek(size_t FramePos)
{
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

        if (InputData->Type==AVMEDIA_TYPE_VIDEO)
        {
            if (InputData->FramePos!=FramePos)
                Seek(FramePos);
            NextFrame();

            break;
        }
    }
}

//---------------------------------------------------------------------------
bool FFmpeg_Glue::NextFrame()
{
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
                    int size = avpicture_get_size((AVPixelFormat)InputDatas[0]->FramesCache_Default->format, InputDatas[0]->FramesCache_Default->width, InputDatas[0]->FramesCache_Default->height);
                    uint8_t* buffer = (uint8_t*)av_malloc(size);
                    avpicture_fill((AVPicture*)InputDatas[0]->FramesCache_Default, buffer, (AVPixelFormat)InputDatas[0]->FramesCache_Default->format, InputDatas[0]->FramesCache_Default->width, InputDatas[0]->FramesCache_Default->height);
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
        if (Packet->stream_index<InputDatas.size() && InputDatas[Packet->stream_index] && InputDatas[Packet->stream_index]->Enabled)
        {
            do
            {
                if (OutputFrame(Packet))
                {
                    if (Packet->size==0)
                        av_free_packet(&TempPacket);
                    if (InputDatas[Packet->stream_index]->Type==AVMEDIA_TYPE_VIDEO)
                        return true;
                }
            }
            while (Packet->size > 0);
        }
        av_free_packet(&TempPacket);
        Packet->size=0;
    }
    
    // Flushing
    Packet->data=NULL;
    Packet->size=0;
    while (OutputFrame(Packet));
    
    // Complete
    if (WithStats)
        for (size_t Pos=0; Pos<Stats->size(); Pos++)
            if ((*Stats)[Pos])
                (*Stats)[Pos]->StatsFinish();

    return false;
}

//---------------------------------------------------------------------------
bool FFmpeg_Glue::OutputFrame(AVPacket* TempPacket, bool Decode)
{
    if (TempPacket->stream_index>=InputDatas.size())
        return false;
    inputdata* InputData=InputDatas[TempPacket->stream_index];

    // Encode
    if (!Encode_FileName.empty())
    {
        if (!Encode_FormatContext)
            InitEncode();
        if (Encode_FormatContext)
            InputData->Encode(TempPacket);
    }
    
    // Decoding
    int got_frame;
    if (Decode)
    {
        got_frame=0;
        int Bytes;
        switch(InputDatas[TempPacket->stream_index]->Type)
        {
            case AVMEDIA_TYPE_VIDEO : Bytes=avcodec_decode_video2(InputData->Stream->codec, Frame, &got_frame, TempPacket); break;
            case AVMEDIA_TYPE_AUDIO : Bytes=avcodec_decode_audio4(InputData->Stream->codec, Frame, &got_frame, TempPacket); break;
            default                 : Bytes=0;
        }
        
        if (Bytes<=0 && !got_frame)
        {
            TempPacket->data+=TempPacket->size;
            TempPacket->size=0;
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
            if (OutputDatas[OutputPos] && OutputDatas[OutputPos]->Stream==InputData->Stream)
                OutputDatas[OutputPos]->Process(Frame);

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
            int size = avpicture_get_size((AVPixelFormat)NewFrame->format, NewFrame->width, NewFrame->height);
            uint8_t* buffer = (uint8_t*)av_malloc(size);
            avpicture_fill((AVPicture*)NewFrame, buffer, (AVPixelFormat)NewFrame->format, NewFrame->width, NewFrame->height);
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

//---------------------------------------------------------------------------
bool FFmpeg_Glue::InitEncode()
{
    //
    if (avformat_alloc_output_context2(&Encode_FormatContext, NULL, Encode_Format.c_str(), Encode_FileName.c_str())<0)
        return false;

    //
    for (size_t InputPos=0; InputPos<InputDatas.size(); InputPos++)
    {
        inputdata* InputData=InputDatas[InputPos];

        InputData->Encode_FormatContext=Encode_FormatContext;
        InputData->InitEncode();
    }

    //
    if (avio_open(&Encode_FormatContext->pb, Encode_FileName.c_str(), AVIO_FLAG_WRITE)<0)
        return false;
     if (avformat_write_header(Encode_FormatContext, NULL))
    {
        avio_close(Encode_FormatContext->pb);
        avformat_free_context(Encode_FormatContext);
        Encode_FormatContext=NULL;
        return false;
    }

    // All is OK
    return true;
}


//---------------------------------------------------------------------------
void FFmpeg_Glue::CloseEncode()
{
    if (!Encode_FormatContext)
        return;

    av_write_trailer(Encode_FormatContext);

    //
    for (size_t InputPos=0; InputPos<InputDatas.size(); InputPos++)
    {
        inputdata* InputData=InputDatas[InputPos];

        InputData->CloseEncode();
    }

    avio_close(Encode_FormatContext->pb);

    avformat_free_context(Encode_FormatContext);

    Encode_FormatContext=NULL;
}

//***************************************************************************
// FFmpeg actions
//***************************************************************************

//---------------------------------------------------------------------------
void FFmpeg_Glue::Filter_Change(size_t FilterPos, int FilterType, const string &Filter)
{
    for (size_t InputPos=0; InputPos<InputDatas.size(); InputPos++)
    {
        inputdata* InputData=InputDatas[InputPos];

        if (InputData && InputData->Type==FilterType)
        {
            for (size_t OutputPos=0; OutputPos<OutputDatas.size(); OutputPos++)
            {
                outputdata* OutputData=OutputDatas[OutputPos];

                if (OutputData->FilterPos==FilterPos)
                    ModifyOutput(InputPos, OutputPos, FilterPos, OutputData->Width, OutputData->Height, OutputData->OutputMethod, FilterType, Filter);
            }
        }
    }
}

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
    if (OutputPos>=OutputDatas.size())
        return DBL_MAX;
    outputdata* OutputData=OutputDatas[OutputPos];
    if (!OutputData)
        return DBL_MAX;

    return ((double)OutputData->DecodedFrame->pkt_pts)*OutputData->Stream->time_base.num/OutputData->Stream->time_base.den;
}

//---------------------------------------------------------------------------
void FFmpeg_Glue::Scale_Change(int Scale_Width_, int Scale_Height_)
{
    bool MustOutput=false;
    for (size_t Pos=0; Pos<OutputDatas.size(); Pos++)
    {
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
                delete OutputData->Image; OutputData->Image=NULL;
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
                bytes* OldThumbnail = OutputData->Thumbnails[Pos];
                OutputData->Thumbnails[Pos]=NULL;
                delete OldThumbnail; // pointer is deleted after deferencing it in order to avoid a race condition (e.g. vector is read during the deletion)
            }
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
        case AV_FIELD_TT:   return "TFF: top coded first, top displayed first";
        case AV_FIELD_BB:   return "BFF: bottom coded_first, bottom displayed first";
        case AV_FIELD_TB:   return "TFF: top coded_first, bottom displayed first";
        case AV_FIELD_BT:   return "BFF: bottom coded_first, top displayed first";
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

    if (InputData->Stream->codec->sample_aspect_ratio.num==0)
        return "Und";
    else
    {
        ostringstream convert;
        convert << InputData->Stream->codec->sample_aspect_ratio.num << "/" << InputData->Stream->codec->sample_aspect_ratio.den;
        return convert.str();
    }
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
        case AV_PIX_FMT_XVMC_MPEG2_MC: return "XVideo Motion Acceleration via common packet passing";
        case AV_PIX_FMT_XVMC_MPEG2_IDCT: return "XVideo Motion Acceleration";
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
//#if FF_API_VDPAU
        case AV_PIX_FMT_VDPAU_H264: return "H.264 HW decoding with VDPAU, data[0] contains a vdpau_render_state struct which contains the bitstream of the slices as well as various fields extracted from headers";
        case AV_PIX_FMT_VDPAU_MPEG1: return "MPEG-1 HW decoding with VDPAU, data[0] contains a vdpau_render_state struct which contains the bitstream of the slices as well as various fields extracted from headers";
        case AV_PIX_FMT_VDPAU_MPEG2: return "MPEG-2 HW decoding with VDPAU, data[0] contains a vdpau_render_state struct which contains the bitstream of the slices as well as various fields extracted from headers";
        case AV_PIX_FMT_VDPAU_WMV3: return "WMV3 HW decoding with VDPAU, data[0] contains a vdpau_render_state struct which contains the bitstream of the slices as well as various fields extracted from headers";
        case AV_PIX_FMT_VDPAU_VC1: return "VC-1 HW decoding with VDPAU, data[0] contains a vdpau_render_state struct which contains the bitstream of the slices as well as various fields extracted from headers";
//#endif
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

        case AV_PIX_FMT_VAAPI_MOCO: return "HW acceleration through VA API at motion compensation entry-point, Picture.data[3] contains a vaapi_render_state struct which contains macroblocks as well as various fields extracted from headers";
        case AV_PIX_FMT_VAAPI_IDCT: return "HW acceleration through VA API at IDCT entry-point, Picture.data[3] contains a vaapi_render_state struct which contains fields extracted from headers";
        case AV_PIX_FMT_VAAPI_VLD: return "HW decoding through VA API, Picture.data[3] contains a vaapi_render_state struct which contains the bitstream of the slices as well as various fields extracted from headers";

        case AV_PIX_FMT_YUV420P16LE: return "planar YUV 4:2:0, 24bpp, (1 Cr & Cb sample per 2x2 Y samples), little-endian";
        case AV_PIX_FMT_YUV420P16BE: return "planar YUV 4:2:0, 24bpp, (1 Cr & Cb sample per 2x2 Y samples), big-endian";
        case AV_PIX_FMT_YUV422P16LE: return "planar YUV 4:2:2, 32bpp, (1 Cr & Cb sample per 2x1 Y samples), little-endian";
        case AV_PIX_FMT_YUV422P16BE: return "planar YUV 4:2:2, 32bpp, (1 Cr & Cb sample per 2x1 Y samples), big-endian";
        case AV_PIX_FMT_YUV444P16LE: return "planar YUV 4:4:4, 48bpp, (1 Cr & Cb sample per 1x1 Y samples), little-endian";
        case AV_PIX_FMT_YUV444P16BE: return "planar YUV 4:4:4, 48bpp, (1 Cr & Cb sample per 1x1 Y samples), big-endian";
//#if FF_API_VDPAU
        case AV_PIX_FMT_VDPAU_MPEG4: return "MPEG4 HW decoding with VDPAU, data[0] contains a vdpau_render_state struct which contains the bitstream of the slices as well as various fields extracted from headers";
//#endif
        case AV_PIX_FMT_DXVA2_VLD:  return "HW decoding through DXVA2, Picture.data[3] contains a LPDIRECT3DSURFACE9 pointer";

        case AV_PIX_FMT_RGB444LE: return "packed RGB 4:4:4, 16bpp, (msb)4A 4R 4G 4B(lsb), little-endian, most significant bits to 0";
        case AV_PIX_FMT_RGB444BE: return "packed RGB 4:4:4, 16bpp, (msb)4A 4R 4G 4B(lsb), big-endian, most significant bits to 0";
        case AV_PIX_FMT_BGR444LE: return "packed BGR 4:4:4, 16bpp, (msb)4A 4B 4G 4R(lsb), little-endian, most significant bits to 1";
        case AV_PIX_FMT_BGR444BE: return "packed BGR 4:4:4, 16bpp, (msb)4A 4B 4G 4R(lsb), big-endian, most significant bits to 1";
        case AV_PIX_FMT_GRAY8A:  return "8bit gray, 8bit alpha";
        case AV_PIX_FMT_BGR48BE: return "packed RGB 16:16:16, 48bpp, 16B, 16G, 16R, the 2-byte value for each R/G/B component is stored as big-endian";
        case AV_PIX_FMT_BGR48LE: return "packed RGB 16:16:16, 48bpp, 16B, 16G, 16R, the 2-byte value for each R/G/B component is stored as little-endian";

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
        case AV_PIX_FMT_VDA_VLD:  return "hardware decoding through VDA";

//#ifdef AV_PIX_FMT_ABI_GIT_MASTER
        case AV_PIX_FMT_RGBA64BE: return "packed RGBA 16:16:16:16, 64bpp, 16R, 16G, 16B, 16A, the 2-byte value for each R/G/B/A component is stored as big-endian";
        case AV_PIX_FMT_RGBA64LE: return "packed RGBA 16:16:16:16, 64bpp, 16R, 16G, 16B, 16A, the 2-byte value for each R/G/B/A component is stored as little-endian";
        case AV_PIX_FMT_BGRA64BE: return "packed RGBA 16:16:16:16, 64bpp, 16B, 16G, 16R, 16A, the 2-byte value for each R/G/B/A component is stored as big-endian";
        case AV_PIX_FMT_BGRA64LE: return "packed RGBA 16:16:16:16, 64bpp, 16B, 16G, 16R, 16A, the 2-byte value for each R/G/B/A component is stored as little-endian";
//#endif
        case AV_PIX_FMT_GBRP:    return "planar GBR 4:4:4 24bpp";
        case AV_PIX_FMT_GBRP9BE: return "planar GBR 4:4:4 27bpp, big-endian";
        case AV_PIX_FMT_GBRP9LE: return "planar GBR 4:4:4 27bpp, little-endian";
        case AV_PIX_FMT_GBRP10BE: return "planar GBR 4:4:4 30bpp, big-endian";
        case AV_PIX_FMT_GBRP10LE: return "planar GBR 4:4:4 30bpp, little-endian";
        case AV_PIX_FMT_GBRP16BE: return "planar GBR 4:4:4 48bpp, big-endian";
        case AV_PIX_FMT_GBRP16LE: return "planar GBR 4:4:4 48bpp, little-endian";

        case AV_PIX_FMT_YUVA422P_LIBAV: return "planar YUV 4:2:2 24bpp, (1 Cr & Cb sample per 2x1 Y & A samples)";
        case AV_PIX_FMT_YUVA444P_LIBAV: return "planar YUV 4:4:4 32bpp, (1 Cr & Cb sample per 1x1 Y & A samples)";

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

        case AV_PIX_FMT_0RGB:    return "packed RGB 8:8:8, 32bpp, 0RGB0RGB...";
        case AV_PIX_FMT_RGB0:    return "packed RGB 8:8:8, 32bpp, RGB0RGB0...";
        case AV_PIX_FMT_0BGR:    return "packed BGR 8:8:8, 32bpp, 0BGR0BGR...";
        case AV_PIX_FMT_BGR0:    return "packed BGR 8:8:8, 32bpp, BGR0BGR0...";
        case AV_PIX_FMT_YUVA444P: return "planar YUV 4:4:4 32bpp, (1 Cr & Cb sample per 1x1 Y & A samples)";
        case AV_PIX_FMT_YUVA422P: return "planar YUV 4:2:2 24bpp, (1 Cr & Cb sample per 2x1 Y & A samples)";

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
        case AV_PIX_FMT_GBRP12BE:  return "planar GBR 4:4:4 36bpp, big-endian";
        case AV_PIX_FMT_GBRP12LE:  return "planar GBR 4:4:4 36bpp, little-endian";
        case AV_PIX_FMT_GBRP14BE:  return "planar GBR 4:4:4 42bpp, big-endian";
        case AV_PIX_FMT_GBRP14LE:  return "planar GBR 4:4:4 42bpp, little-endian";
        case AV_PIX_FMT_GBRAP:     return "planar GBRA 4:4:4:4 32bpp";
        case AV_PIX_FMT_GBRAP16BE: return "planar GBRA 4:4:4:4 64bpp, big-endian";
        case AV_PIX_FMT_GBRAP16LE: return "planar GBRA 4:4:4:4 64bpp, little-endian";
        case AV_PIX_FMT_YUVJ411P:  return "planar YUV 4:1:1, 12bpp, (1 Cr & Cb sample per 4x1 Y samples) full scale (JPEG), deprecated in favor of PIX_FMT_YUV411P and setting color_range";

        case AV_PIX_FMT_BAYER_BGGR8:  return "bayer, BGBG..(odd line), GRGR..(even line), 8-bit samples";
        case AV_PIX_FMT_BAYER_RGGB8:  return "bayer, RGRG..(odd line), GBGB..(even line), 8-bit samples";
        case AV_PIX_FMT_BAYER_GBRG8:  return "bayer, GBGB..(odd line), RGRG..(even line), 8-bit samples";
        case AV_PIX_FMT_BAYER_GRBG8:  return "bayer, GRGR..(odd line), BGBG..(even line), 8-bit samples";
        case AV_PIX_FMT_BAYER_BGGR16LE: return "bayer, BGBG..(odd line), GRGR..(even line), 16-bit samples, little-endian";
        case AV_PIX_FMT_BAYER_BGGR16BE: return "bayer, BGBG..(odd line), GRGR..(even line), 16-bit samples, big-endian";
        case AV_PIX_FMT_BAYER_RGGB16LE: return "bayer, RGRG..(odd line), GBGB..(even line), 16-bit samples, little-endian";
        case AV_PIX_FMT_BAYER_RGGB16BE: return "bayer, RGRG..(odd line), GBGB..(even line), 16-bit samples, big-endian";
        case AV_PIX_FMT_BAYER_GBRG16LE: return "bayer, GBGB..(odd line), RGRG..(even line), 16-bit samples, little-endian";
        case AV_PIX_FMT_BAYER_GBRG16BE: return "bayer, GBGB..(odd line), RGRG..(even line), 16-bit samples, big-endian";
        case AV_PIX_FMT_BAYER_GRBG16LE: return "bayer, GRGR..(odd line), BGBG..(even line), 16-bit samples, little-endian";
        case AV_PIX_FMT_BAYER_GRBG16BE: return "bayer, GRGR..(odd line), BGBG..(even line), 16-bit samples, big-endian";
        default: return string();
    }
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
        case AVCOL_SPC_BT709: return "BT.709 / ITU-R BT1361 / IEC 61966-2-4 xvYCC709 / SMPTE RP177 Annex B";
        case AVCOL_SPC_UNSPECIFIED: return "Unspecified";
        case AVCOL_SPC_RESERVED: return "Reserved";
        case AVCOL_SPC_FCC: return "FCC Title 47 Code of Federal Regulations 73.682 (a)(20)";
        case AVCOL_SPC_BT470BG: return "BT.470bg / ITU-R BT601-6 625 / ITU-R BT1358 625 / ITU-R BT1700 625 PAL & SECAM / IEC 61966-2-4 xvYCC601";
        case AVCOL_SPC_SMPTE170M: return "SMPTE 170m / ITU-R BT601-6 525 / ITU-R BT1358 525 / ITU-R BT1700 NTSC";
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
        case AVCOL_RANGE_MPEG: return "Broadcast Range (219*2^n-1)";
        case AVCOL_RANGE_JPEG: return "Full Range (2^n-1)";
        case AVCOL_RANGE_NB: return "Not part of ABI";
        default: return string();
    }
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

    if (InputData==NULL || InputData->Stream==NULL || InputData->Stream->time_base.den==0)
        return 0;

    return InputData->Stream->time_base.den;
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
    return CONFIG_THIS_YEAR;
}

//---------------------------------------------------------------------------
string FFmpeg_Glue::FFmpeg_Compiler()
{
    return CC_IDENT;
}

//---------------------------------------------------------------------------
string FFmpeg_Glue::FFmpeg_Configuration()
{
    return FFMPEG_CONFIGURATION;
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

