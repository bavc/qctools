/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include "Core/FFmpeg_Glue.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include <QImage>

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
}

#include <sstream>
#include <iomanip>
#include <cstdlib>
//---------------------------------------------------------------------------

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
*/

//***************************************************************************
// Constructor / Destructor
//***************************************************************************

//---------------------------------------------------------------------------
FFmpeg_Glue::FFmpeg_Glue (const string &FileName_, int Scale_Width_, int Scale_Height_, outputmethod OutputMethod_, const string &Filter1_, const string &Filter2_, bool With1_, bool With2_, bool WithStats_, const string &StatsFromExternalData_) :
    FileName(FileName_),
    Scale_Width(Scale_Width_),
    Scale_Height(Scale_Height_),
    OutputMethod(OutputMethod_),
    Filter1(Filter1_),
    Filter2(Filter2_),
    WithStats(WithStats_),
    With1(With1_),
    With2(With2_),
    StatsFromExternalData(StatsFromExternalData_)
{
    // In
    Scale_Adapted=false;

    // Out
    Image1=NULL;
    Image2=NULL;
    Jpeg.Data=NULL;
    Jpeg.Size=0;
    VideoFrameCount=0;
    VideoDuration=0;
    
    // FFmpeg pointers
    FormatContext=NULL;
    JpegOutput_CodecContext=NULL;
    FilterGraph1=NULL;
    FilterGraph2=NULL;
    Filtered1_Frame=NULL;
    Filtered2_Frame=NULL;
    Scale1_Context=NULL;
    Scale2_Context=NULL;
    Scale1_Frame=NULL;
    Scale2_Frame=NULL;

    // FFmpeg init
    av_register_all();
    avfilter_register_all();
    //av_log_set_callback(avlog_cb);

    // Container part
    if (avformat_open_input(&FormatContext, FileName.c_str(), NULL, NULL)<0)
        return;
    if (avformat_find_stream_info(FormatContext, NULL)<0)
        return;

    // Video stream
    VideoStream_Index=av_find_best_stream(FormatContext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (VideoStream_Index<0)
        return;
    VideoStream=FormatContext->streams[VideoStream_Index];

    // Audio stream
    int AudioStream_Index=av_find_best_stream(FormatContext, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if (AudioStream_Index>=0)
        AudioStream=FormatContext->streams[AudioStream_Index];
    else
        AudioStream=NULL;

    // Video codec
    AVCodec* VideoStream_Codec=avcodec_find_decoder(VideoStream->codec->codec_id);
    if (!VideoStream_Codec)
        return;
    if (avcodec_open2(VideoStream->codec, VideoStream_Codec, NULL) < 0)
        return;

    // Audio codec
    if (AudioStream)
    {
        AVCodec* AudioStream_Codec=avcodec_find_decoder(AudioStream->codec->codec_id);
        if (AudioStream_Codec)
            avcodec_open2(AudioStream->codec, AudioStream_Codec, NULL);
    }

    // Frame
    Frame = avcodec_alloc_frame();
    if (!Frame)
        return;

    // Packet
    Packet = new AVPacket;
    av_init_packet(Packet);
    Packet->data = NULL;
    Packet->size = 0;

    // Output
    VideoFrameCount=VideoStream->nb_frames;
    VideoDuration=((double)VideoStream->duration)*VideoStream->time_base.num/VideoStream->time_base.den;
    if (OutputMethod==Output_JpegList)
    {
        JpegList.reserve(VideoFrameCount);
    }
    if (OutputMethod==Output_Jpeg || OutputMethod==Output_JpegList)
    {
        JpegOutput_CodecContext=NULL;
    }

    //Demux
    JpegOutput_Packet=new AVPacket;
    av_init_packet (JpegOutput_Packet);
    JpegOutput_Packet->data=NULL;
    JpegOutput_Packet->size=0;

    // frame info
    if (WithStats || !StatsFromExternalData.empty())
    {
        // Configure
        x = new double*[2];
        memset(x, 0, 2*sizeof(double*));
        y = new double*[PlotName_Max];
        memset(y, 0, PlotName_Max*sizeof(double*));
        memset(y_Max, 0, PlotType_Max*sizeof(double));

        for(size_t j=0; j<2; ++j)
        {
            x[j]=new double[VideoFrameCount];
            memset(x[j], 0x00, VideoFrameCount*sizeof(double));
        }
        for(size_t j=0; j<PlotName_Max; ++j)
        {
            y[j] = new double[VideoFrameCount];
            memset(y[j], 0x00, VideoFrameCount*sizeof(double));
        }
        for(size_t j=0; j<PlotType_Max; ++j)
            y_Max[j]=0; //PerPlotGroup[j].Max;
    }
    x_Max=0;
    x_Line_Begin=0;
    VideoFramePos=0;

    // Temp
    DTS_Target=-1;
}

//---------------------------------------------------------------------------
FFmpeg_Glue::~FFmpeg_Glue()
{
    if (VideoFrameCount==0)
        return; // Nothing was initialized    
        
    // Clear
    if (Packet)
    {
        av_free_packet(Packet);
        delete Packet;
    }
    if (JpegOutput_Packet)
    {
        av_free_packet(JpegOutput_Packet);
        delete JpegOutput_Packet;
    }
    if (Scale1_Frame)
        avpicture_free((AVPicture*)Scale1_Frame);
    if (Scale1_Context)
        sws_freeContext(Scale1_Context);
}

//***************************************************************************
// Actions
//***************************************************************************

//---------------------------------------------------------------------------
void FFmpeg_Glue::Seek(size_t Pos)
{
    // DTS computing
    long long DTS=Pos;
    DTS*=VideoStream->duration;
    DTS/=VideoStream->nb_frames;
    DTS_Target=(int)DTS;
    
    // Seek
    if (DTS_Target)
        avformat_seek_file(FormatContext,VideoStream_Index,0,DTS_Target,DTS_Target,0);
    else
        avformat_seek_file(FormatContext,VideoStream_Index,0,1,1,0); //Found some cases such seek fails
    VideoFramePos=Pos;

    // Flushing
    avcodec_flush_buffers(VideoStream->codec);
}

//---------------------------------------------------------------------------
void FFmpeg_Glue::FrameAtPosition(size_t Pos)
{
    if (VideoFramePos!=Pos)
        Seek(Pos);
    return NextFrame();
}

//---------------------------------------------------------------------------
void FFmpeg_Glue::NextFrame()
{
    if (!StatsFromExternalData.empty())
    {
        if (!x_Line_Begin)
        {
            size_t Line_End=StatsFromExternalData.find('\r');
            if (Line_End==-1)
            {
                Line_End=StatsFromExternalData.find('\n');
                if (Line_End==-1)
                    StatsFromExternalData.clear();
                else
                {
                    x_Line_EolChar='\n';
                    x_Line_EolSize=1;
                }
            }
            else
            {
                x_Line_EolChar='\r';
                if (Line_End+1>StatsFromExternalData.size() || StatsFromExternalData[Line_End]!='\n')
                    x_Line_EolSize=1;
                else
                    x_Line_EolSize=2;
            }
        }
            
        size_t Frames_Begin=x_Max;
        int Line_End;
        for (;;)
        {
            Line_End=StatsFromExternalData.find(x_Line_EolChar, x_Line_Begin);
            if (Line_End==-1 || x_Max>=Frames_Begin+100)
                break;

            if (x_Line_Begin) //If not the header
            {
                x[1][x_Max]=x_Max;
        
                int Col_Begin=x_Line_Begin;
                int Col_End;
                unsigned Pos2=0;
                for (;;)
                {
                    Col_End=StatsFromExternalData.find(',', Col_Begin);
                    if (Col_End>Line_End || Col_End==-1)
                        Col_End=Line_End;
                    string Value(StatsFromExternalData.substr(Col_Begin, Col_End-Col_Begin));
                    if (Pos2==6)
                    {
                        x[0][x_Max]=std::atof(Value.data());
                    }
                    else if (Pos2>=PlotName_Begin)
                    {
                        size_t j=Pos2-PlotName_Begin;
                        y[j][x_Max]=std::atof(Value.data());

                        //TODO
                        if (PerPlotName[j].Group1!=PlotType_Max && y_Max[PerPlotName[j].Group1]<y[j][x_Max])
                            y_Max[PerPlotName[j].Group1]=y[j][x_Max];
                        if (PerPlotName[j].Group2!=PlotType_Max && y_Max[PerPlotName[j].Group2]<y[j][x_Max])
                            y_Max[PerPlotName[j].Group2]=y[j][x_Max];
                    }

                    if (Col_End>=Line_End)
                        break;
                    Col_Begin=Col_End+1;
                    Pos2++;
                }

                x_Max++;
            }

            x_Line_Begin=Line_End+x_Line_EolSize;
        }

        if (Line_End==-1)
            StatsFromExternalData.clear();
    }
    
    // Clear
    Jpeg.Data=NULL;
    Jpeg.Size=0;
    
    // Next frame
    while (Packet->size || av_read_frame(FormatContext, Packet) >= 0)
    {
        AVPacket TempPacket=*Packet;
        if (Packet->stream_index == VideoStream_Index)
        {
            do
            {
                if (OutputFrame(Packet))
                    return;
            }
            while (Packet->size > 0);
        }
        else
            Packet->size=0;
        av_free_packet(&TempPacket);
    }
    
    // Flushing
    Packet->data=NULL;
    Packet->size=0;
    while (!OutputFrame(Packet));
}

//---------------------------------------------------------------------------
bool FFmpeg_Glue::OutputFrame(AVPacket* TempPacket, bool Decode)
{
    // Decoding
    int got_frame;
    if (Decode)
    {
        got_frame=0;
        int Bytes=avcodec_decode_video2(VideoStream->codec, Frame, &got_frame, TempPacket);
        if (Bytes<=0 && !got_frame)
        {
            // Should not happen, NULL image
            Image1=NULL;
            Image2=NULL;
            Jpeg.Data=NULL;
            Jpeg.Size=0;
            return true;
        }
        TempPacket->data+=Bytes;
        TempPacket->size-=Bytes;
    }
    else
        got_frame=1;

    // Analyzing frame
    if (got_frame && (DTS_Target==-1 || Frame->pkt_pts>=(DTS_Target?(DTS_Target-1):DTS_Target)))
    {
        if (With1)
            Process(Filtered1_Frame, FilterGraph1, FilterGraph1_Source_Context, FilterGraph1_Sink_Context, Filter1, Scale1_Context, Scale1_Frame, Image1);

        if (With2)
            Process(Filtered1_Frame, FilterGraph2, FilterGraph2_Source_Context, FilterGraph2_Sink_Context, Filter2, Scale2_Context, Scale2_Frame, Image2);
        
        VideoFramePos++;
        return true;
    }

    return false;
}

//***************************************************************************
// FFmpeg actions
//***************************************************************************

//---------------------------------------------------------------------------
bool FFmpeg_Glue::FilterGraph_Init(AVFilterGraph* &FilterGraph, AVFilterContext* &FilterGraph_Source_Context, AVFilterContext* &FilterGraph_Sink_Context, const string &Filter)
{
    // Alloc
    AVFilter*       Source      = avfilter_get_by_name("buffer");
    AVFilter*       Sink        = avfilter_get_by_name("buffersink");
    AVFilterInOut*  Outputs     = avfilter_inout_alloc();
    AVFilterInOut*  Inputs      = avfilter_inout_alloc();
                    FilterGraph = avfilter_graph_alloc();


    // Source
    stringstream args; //"video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d"
    args    << "video_size="  <<VideoStream->codec->width<<"x"<<VideoStream->codec->height
            <<":pix_fmt="     <<(int)VideoStream->codec->pix_fmt
            <<":time_base="   <<VideoStream->codec->time_base.num<<"/"<<VideoStream->codec->time_base.den
            <<":pixel_aspect="<<VideoStream->codec->sample_aspect_ratio.num<<"/"<<VideoStream->codec->sample_aspect_ratio.den;
    if (avfilter_graph_create_filter(&FilterGraph_Source_Context, Source, "in", args.str().c_str(), NULL, FilterGraph)<0)
        return false;

    // Sink
    if (avfilter_graph_create_filter(&FilterGraph_Sink_Context, Sink, "out", NULL, NULL, FilterGraph)<0)
        return false;

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
        return false;
    if (avfilter_graph_config(FilterGraph, NULL)<0)
        return false;

    // All is OK
    return true;
}

//---------------------------------------------------------------------------
void FFmpeg_Glue::FilterGraph_Free(AVFilterGraph* &FilterGraph)
{
    if (FilterGraph)
    {
        avfilter_graph_free(&FilterGraph);
        FilterGraph=NULL;
    }
}

//---------------------------------------------------------------------------
void FFmpeg_Glue::Filter1_Change(const string &Filter)
{
    With1_Change(true);
    
    Scale_Free(Scale1_Frame, Scale1_Context, Scale1_Frame);
    FilterGraph_Free(FilterGraph1);
    if (!FilterGraph_Init(FilterGraph1, FilterGraph1_Source_Context, FilterGraph1_Sink_Context, Filter))
    {
        FilterGraph_Free(FilterGraph1);
        Filter1.clear();
        return;
    }
    Filter1=Filter;
}

//---------------------------------------------------------------------------
void FFmpeg_Glue::Filter2_Change(const string &Filter)
{
    With2_Change(true);
    
    Scale_Free(Scale2_Frame, Scale2_Context, Scale2_Frame);
    FilterGraph_Free(FilterGraph2);
    if (!FilterGraph_Init(FilterGraph2, FilterGraph2_Source_Context, FilterGraph2_Sink_Context, Filter))
    {
        FilterGraph_Free(FilterGraph2);
        Filter2.clear();
        return;
    }
    Filter2=Filter;
}

//---------------------------------------------------------------------------
void FFmpeg_Glue::With1_Change(bool With)
{
    With1=With;
    if (!With)
    {
        delete Image1; Image1=NULL;
    }
}

//---------------------------------------------------------------------------
void FFmpeg_Glue::With2_Change(bool With)
{
    With2=With;
    if (!With)
    {
        delete Image2; Image2=NULL;
    }
}

//---------------------------------------------------------------------------
void FFmpeg_Glue::Scale_Change(int Scale_Width_, int Scale_Height_)
{
    if (Scale_Width_ && Scale_Height_)
    {
        int ScaleSave_Width=Scale_Width;
        int ScaleSave_Height=Scale_Height;
        Scale_Width=Scale_Width_;
        Scale_Height=Scale_Height_;
        Scale_Adapted=false;
        if (Frame)
            Scale_Adapt(Frame);
        if (ScaleSave_Width!=Scale_Width || ScaleSave_Height!=Scale_Height)
        {
            Scale_Free(Scale1_Frame, Scale1_Context, Scale1_Frame);
            Scale_Free(Scale2_Frame, Scale2_Context, Scale2_Frame);
            delete Image1; Image1=NULL;
            delete Image2; Image2=NULL;
            OutputFrame(Packet, false);
        }
    }
    else
    {
        With1_Change(false);
        With2_Change(false);
    }
}

//---------------------------------------------------------------------------
bool FFmpeg_Glue::Scale_Adapt(AVFrame* FilteredFrame)
{
    // Display aspect ratio
    double DAR;
    if (FilteredFrame->sample_aspect_ratio.num && FilteredFrame->sample_aspect_ratio.den)
        DAR=((double)FilteredFrame->width)/FilteredFrame->height*FilteredFrame->sample_aspect_ratio.num/FilteredFrame->sample_aspect_ratio.den;
    else if ((FilteredFrame->width>=704 && FilteredFrame->width<=720) //NTSC / PAL
          && ((FilteredFrame->height>=480 && FilteredFrame->height<=486)
           || FilteredFrame->height==576))
        DAR=((double)4)/3;
    else
        DAR=((double)FilteredFrame->width)/FilteredFrame->height;
    if (DAR)
    {
        int Target_Width=Scale_Height*DAR;
        int Target_Height=Scale_Width/DAR;
        if (Target_Width>Scale_Width)
            Scale_Height=Target_Height;
        if (Target_Height>Scale_Height)
            Scale_Width=Target_Width;

        Scale_Adapted=true;
    }

    return true;
}

//---------------------------------------------------------------------------
bool FFmpeg_Glue::Scale_Init(AVFrame* FilteredFrame, SwsContext* &Scale_Context, AVFrame* &Scale_Frame)
{
    if (!Scale_Adapted && !Scale_Adapt(FilteredFrame))
        return false;
    
    // Init
    Scale_Context = sws_getContext(FilteredFrame->width, FilteredFrame->height,
                                    (AVPixelFormat)FilteredFrame->format,
                                    Scale_Width, Scale_Height,
                                    OutputMethod==Output_QImage?PIX_FMT_RGB24:PIX_FMT_YUVJ420P,
                                    SWS_BICUBIC, NULL, NULL, NULL);
    Scale_Frame=avcodec_alloc_frame();
    Scale_Frame->width=Scale_Width;
    Scale_Frame->height=Scale_Height;
    avpicture_alloc((AVPicture*)Scale_Frame, OutputMethod==Output_QImage?PIX_FMT_RGB24:PIX_FMT_YUVJ420P, Scale_Width, Scale_Height);

    // All is OK
    return true;
}

//---------------------------------------------------------------------------
void FFmpeg_Glue::Scale_Free(AVFrame* FilteredFrame, SwsContext* &Scale_Context, AVFrame* &Scale_Frame)
{
    if (FilteredFrame)
    {
        sws_freeContext(Scale_Context);
        Scale_Context=NULL;
    }

    if (Scale_Frame)
    {
        avcodec_free_frame(&Scale_Frame);
        Scale_Frame=NULL;
    }
}

//---------------------------------------------------------------------------
bool FFmpeg_Glue::JpegOutput_Init()
{
    AVCodec *JpegOutput_Codec=avcodec_find_encoder(CODEC_ID_MJPEG);
    if (!JpegOutput_Codec)
        return false;
    JpegOutput_CodecContext=avcodec_alloc_context3 (JpegOutput_Codec);
    if (!JpegOutput_CodecContext)
        return false;
    JpegOutput_CodecContext->qmax          = 12;
    JpegOutput_CodecContext->width         = Scale_Width;
    JpegOutput_CodecContext->height        = Scale_Height;
    JpegOutput_CodecContext->pix_fmt       = PIX_FMT_YUVJ420P;
    JpegOutput_CodecContext->time_base.num = VideoStream->codec->time_base.num;
    JpegOutput_CodecContext->time_base.den = VideoStream->codec->time_base.den;
    if (avcodec_open2(JpegOutput_CodecContext, JpegOutput_Codec, NULL) < 0)
        return false;

    // All is OK
    return true;
}

//---------------------------------------------------------------------------
bool FFmpeg_Glue::Process(AVFrame* &FilteredFrame, AVFilterGraph* &FilterGraph, AVFilterContext* &FilterGraph_Source_Context, AVFilterContext* &FilterGraph_Sink_Context, const string &Filter, SwsContext* &Scale_Context, AVFrame* &Scale_Frame, QImage* &Image)
{
    //Filtering
    if (Filter.empty())
        FilteredFrame=Frame; // The filtered frame is the decoded frame
    else
    {
        if (!FilterGraph && !FilterGraph_Init(FilterGraph, FilterGraph_Source_Context, FilterGraph_Sink_Context, Filter))
            return true;
            
        // Push the decoded Frame into the filtergraph 
        if (av_buffersrc_add_frame_flags(FilterGraph_Source_Context, Frame, AV_BUFFERSRC_FLAG_KEEP_REF)<0)
            return true;

        // Pull filtered frames from the filtergraph 
        FilteredFrame = av_frame_alloc();
        int GetAnswer = av_buffersink_get_frame(FilterGraph_Sink_Context, FilteredFrame); //TODO: handling of multiple output per input
            
        // Stats
        if (WithStats)
        {
            x[0][x_Max]=((double)Frame->pkt_pts)*VideoStream->time_base.num/VideoStream->time_base.den;
            x[1][x_Max]=x_Max;

            AVDictionary * m=av_frame_get_metadata (FilteredFrame);
            AVDictionaryEntry* e=NULL;
            string A;
            for (;;)
            {
                e=av_dict_get 	(m, "", e, AV_DICT_IGNORE_SUFFIX);
                if (!e)
                    break;
                size_t j=0;
                for (; j<PlotName_Max; j++)
                {
                    if (strcmp(e->key, PerPlotName[j].FFmpeg_Name)==0)
                        break;
                }

                if (j<PlotName_Max)
                {
                    y[j][x_Max]=std::atof(e->value);

                    if (PerPlotName[j].Group1!=PlotType_Max && y_Max[PerPlotName[j].Group1]<y[j][x_Max])
                        y_Max[PerPlotName[j].Group1]=y[j][x_Max];
                    if (PerPlotName[j].Group2!=PlotType_Max && y_Max[PerPlotName[j].Group2]<y[j][x_Max])
                        y_Max[PerPlotName[j].Group2]=y[j][x_Max];
                }

                /*
                A+=e->key;
                A+=',';
                A+=e->value;
                A+="\r\n";
                */
            }

            x_Max++;
        }

        if (GetAnswer==AVERROR(EAGAIN) || GetAnswer==AVERROR_EOF)
            return true; // TODO: handle such cases
        if (GetAnswer<0)
            return true; // Error

        //TODO: add a real flag for skipping filter
        if (OutputMethod!=Output_QImage) //Awful hack: we detect it is not the BigBisplay
        {
            av_frame_unref(FilteredFrame);
            FilteredFrame=Frame; // The filtered frame is the decoded frame
        }
    }

    //Scale
    if (true)
    {
        if (!Scale_Context && !Scale_Init(FilteredFrame, Scale_Context, Scale_Frame))
            return true;

        sws_scale(Scale_Context, FilteredFrame->data, FilteredFrame->linesize, 0, FilteredFrame->height, Scale_Frame->data, Scale_Frame->linesize);
    }
    else
    {
        Scale_Frame=FilteredFrame;
    }

    //Encode
    switch (OutputMethod)
    {
        case Output_QImage :
                            // Convert the Frame to QImage
                            if (Image==NULL)
                                Image=new QImage(Scale_Frame->width, Scale_Frame->height, QImage::Format_RGB888);
                            for(int y=0;y<Scale_Frame->height;y++)
                                memcpy(Image->scanLine(y), Scale_Frame->data[0]+y*Scale_Frame->linesize[0], Scale_Frame->width*3);
                            break;
        case Output_Jpeg :
        case Output_JpegList :
                            {
                            int got_packet=0;
                            JpegOutput_Packet->data=NULL;
                            JpegOutput_Packet->size=0;
                            if (!JpegOutput_CodecContext && !JpegOutput_Init())
                                return true;
                                    
                            int temp = avcodec_encode_video2(JpegOutput_CodecContext, JpegOutput_Packet, Scale_Frame, &got_packet);

                            if (got_packet)
                            {
                                if (OutputMethod==Output_JpegList)
                                {
                                    bytes* JpegItem=new bytes;
                                    JpegItem->Data=new unsigned char[JpegOutput_Packet->size];
                                    memcpy(JpegItem->Data, JpegOutput_Packet->data, JpegOutput_Packet->size);
                                    JpegItem->Size=JpegOutput_Packet->size;
                                    JpegList.push_back(JpegItem);
                                }
                                else
                                {
                                    Jpeg.Data=JpegOutput_Packet->data;
                                    Jpeg.Size=JpegOutput_Packet->size;
                                }
                                av_free_packet(Packet);
                            }
                            }
                            break;
        default          :  ;
    }

    //Filtering
    if (!Filter.empty())
        av_frame_unref(FilteredFrame);
    if (!true)
        Scale_Frame=NULL;

    // All is OK
    return true;
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
string FFmpeg_Glue::VideoFormat_Get()
{
    if (VideoStream==NULL || VideoStream->codec==NULL || VideoStream->codec->codec==NULL || VideoStream->codec->codec->long_name==NULL)
        return "";

    return VideoStream->codec->codec->long_name;
}

//---------------------------------------------------------------------------
int FFmpeg_Glue::Width_Get()
{
    if (VideoStream==NULL || VideoStream->codec==NULL)
        return 0;

    return VideoStream->codec->width;
}

//---------------------------------------------------------------------------
int FFmpeg_Glue::Height_Get()
{
    if (VideoStream==NULL || VideoStream->codec==NULL)
        return 0;

    return VideoStream->codec->height;
}

//---------------------------------------------------------------------------
double FFmpeg_Glue::DAR_Get()
{
    if (VideoStream==NULL || VideoStream->codec==NULL || VideoStream->codec->codec==NULL || VideoStream->codec->codec->long_name==NULL)
        return 0;

    double DAR;
    if (VideoStream->codec->sample_aspect_ratio.num && VideoStream->codec->sample_aspect_ratio.den)
        DAR=((double)VideoStream->codec->width)/VideoStream->codec->height*VideoStream->codec->sample_aspect_ratio.num/VideoStream->codec->sample_aspect_ratio.den;
    else if ((VideoStream->codec->width>=704 && VideoStream->codec->width<=720) //NTSC / PAL
          && ((VideoStream->codec->height>=480 && VideoStream->codec->height<=486)
           || VideoStream->codec->height==576))
        DAR=((double)4)/3;
    else
        DAR=((double)VideoStream->codec->width)/VideoStream->codec->height;
    return DAR;
}

//---------------------------------------------------------------------------
string FFmpeg_Glue::PixFormat_Get()
{
    if (VideoStream==NULL || VideoStream->codec==NULL)
        return "";

    switch (VideoStream->codec->pix_fmt)
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
string FFmpeg_Glue::AudioFormat_Get()
{
    if (AudioStream==NULL || AudioStream->codec==NULL || AudioStream->codec->codec==NULL || AudioStream->codec->codec->long_name==NULL)
        return "";

    return AudioStream->codec->codec->long_name;
}

//---------------------------------------------------------------------------
string FFmpeg_Glue::SampleFormat_Get()
{
    if (AudioStream==NULL || AudioStream->codec==NULL || AudioStream->codec->codec==NULL || AudioStream->codec->codec->long_name==NULL)
        return "";

    return "";
}

//---------------------------------------------------------------------------
int FFmpeg_Glue::SamplingRate_Get()
{
    if (AudioStream==NULL || AudioStream->time_base.den==0)
        return 0;

    return AudioStream->time_base.den;
}

//---------------------------------------------------------------------------
string FFmpeg_Glue::ChannelLayout_Get()
{
    if (AudioStream==NULL || AudioStream->codec==NULL || AudioStream->codec->codec==NULL || AudioStream->codec->codec->long_name==NULL)
        return "";

    return "";
}

//---------------------------------------------------------------------------
int FFmpeg_Glue::BitDepth_Get()
{
    if (AudioStream==NULL || AudioStream->codec==NULL || AudioStream->codec->codec==NULL || AudioStream->codec->codec->long_name==NULL)
        return 0;

    return 0;
}

//---------------------------------------------------------------------------
string FFmpeg_Glue::StatsToExternalData()
{
    stringstream Value;
    Value<<",,,,,,,,,,,,,,,,,,,,,YMIN,YLOW,YAVG,YHIGH,YMAX,UMIN,ULOW,UAVG,UHIGH,UMAX,VMIN,VLOW,VAVG,VHIGH,VMAX,YDIF,UDIF,VDIF,YDIF1,YDIF2,TOUT,HEAD,VREP,BRNG";
    #ifdef _WIN32
        Value<<"\r\n";
    #else
        #ifdef __APPLE__
            Value<<"\r";
        #else
            Value<<"\n";
        #endif
    #endif
    for (size_t Pos=0; Pos<x_Max; Pos++)
    {
        Value<<",,,,,,";
        Value<<fixed<<setprecision(6)<<x[0][Pos];
        Value<<",,,,,,,,,,,,,,";
        for (size_t Pos2=0; Pos2<PlotName_Max; Pos2++)
        {
            Value<<','<<fixed<<setprecision(PerPlotName[Pos2].DigitsAfterComma)<<y[Pos2][Pos];
        }
        #ifdef _WIN32
            Value<<"\r\n";
        #else
            #ifdef __APPLE__
                Value<<"\r";
            #else
                Value<<"\n";
            #endif
        #endif
    }

    return Value.str();
}