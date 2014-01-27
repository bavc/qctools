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

    // Video codec
    AVCodec* VideoStream_Codec=avcodec_find_decoder(VideoStream->codec->codec_id);
    if (!VideoStream_Codec)
        return;
    if (avcodec_open2(VideoStream->codec, VideoStream_Codec, NULL) < 0)
        return;

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
    
        y_Max[PlotType_Y]=255;
        y_Max[PlotType_U]=255;
        y_Max[PlotType_V]=255;
        y_Max[PlotType_YDiff]=0;
        y_Max[PlotType_YDiffX]=0;
        y_Max[PlotType_UDiff]=0;
        y_Max[PlotType_VDiff]=0;
        y_Max[PlotType_Diffs]=0;
        y_Max[PlotType_TOUT]=0;
        y_Max[PlotType_VREP]=0;
        y_Max[PlotType_HEAD]=0;
        y_Max[PlotType_BRNG]=0;
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

                        switch (j)
                        {
                            case PlotName_YDIF :    if (y_Max[PlotType_YDiff]<y[j][x_Max])
                                                        y_Max[PlotType_YDiff]=y[j][x_Max];
                                                    if (y_Max[PlotType_Diffs]<y[j][x_Max])
                                                        y_Max[PlotType_Diffs]=y[j][x_Max];
                                                    break;
                            case PlotName_UDIF :    if (y_Max[PlotType_UDiff]<y[j][x_Max])
                                                        y_Max[PlotType_UDiff]=y[j][x_Max];
                                                    if (y_Max[PlotType_Diffs]<y[j][x_Max])
                                                        y_Max[PlotType_Diffs]=y[j][x_Max];
                                                    break;
                            case PlotName_VDIF :    if (y_Max[PlotType_VDiff]<y[j][x_Max])
                                                        y_Max[PlotType_VDiff]=y[j][x_Max];
                                                    if (y_Max[PlotType_Diffs]<y[j][x_Max])
                                                        y_Max[PlotType_Diffs]=y[j][x_Max];
                                                    break;
                            case PlotName_YDIF1 :   if (y_Max[PlotType_YDiffX]<y[j][x_Max])
                                                        y_Max[PlotType_YDiffX]=y[j][x_Max];
                                                    break;
                            case PlotName_YDIF2 :   if (y_Max[PlotType_YDiffX]<y[j][x_Max])
                                                        y_Max[PlotType_YDiffX]=y[j][x_Max];
                                                    break;
                            case PlotName_TOUT :    if (y_Max[PlotType_TOUT]<y[j][x_Max])
                                                        y_Max[PlotType_TOUT]=y[j][x_Max];
                                                    break;
                            case PlotName_VREP :    if (y_Max[PlotType_VREP]<y[j][x_Max])
                                                        y_Max[PlotType_VREP]=y[j][x_Max];
                                                    break;
                            case PlotName_BRNG :    if (y_Max[PlotType_BRNG]<y[j][x_Max])
                                                        y_Max[PlotType_BRNG]=y[j][x_Max];
                                                    break;
                            case PlotName_HEAD :    if (y_Max[PlotType_HEAD]<y[j][x_Max])
                                                        y_Max[PlotType_HEAD]=y[j][x_Max];
                                                    break;
                            default:                ;
                        }
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
            //string A;
            size_t j=0;
            for (;;)
            {
                e=av_dict_get 	(m, "", e, AV_DICT_IGNORE_SUFFIX);
                if (!e)
                    break;
                y[j][x_Max]=std::atof(e->value);

                switch (j)
                {
                    case PlotName_YDIF :    if (y_Max[PlotType_YDiff]<y[j][x_Max])
                                                y_Max[PlotType_YDiff]=y[j][x_Max];
                                            if (y_Max[PlotType_Diffs]<y[j][x_Max])
                                                y_Max[PlotType_Diffs]=y[j][x_Max];
                                            break;
                    case PlotName_UDIF :    if (y_Max[PlotType_UDiff]<y[j][x_Max])
                                                y_Max[PlotType_UDiff]=y[j][x_Max];
                                            if (y_Max[PlotType_Diffs]<y[j][x_Max])
                                                y_Max[PlotType_Diffs]=y[j][x_Max];
                                            break;
                    case PlotName_VDIF :    if (y_Max[PlotType_VDiff]<y[j][x_Max])
                                                y_Max[PlotType_VDiff]=y[j][x_Max];
                                            if (y_Max[PlotType_Diffs]<y[j][x_Max])
                                                y_Max[PlotType_Diffs]=y[j][x_Max];
                                            break;
                    case PlotName_YDIF1 :   if (y_Max[PlotType_YDiffX]<y[j][x_Max])
                                                y_Max[PlotType_YDiffX]=y[j][x_Max];
                                            break;
                    case PlotName_YDIF2 :   if (y_Max[PlotType_YDiffX]<y[j][x_Max])
                                                y_Max[PlotType_YDiffX]=y[j][x_Max];
                                            break;
                    case PlotName_TOUT :    if (y_Max[PlotType_TOUT]<y[j][x_Max])
                                                y_Max[PlotType_TOUT]=y[j][x_Max];
                                            break;
                    case PlotName_VREP :    if (y_Max[PlotType_VREP]<y[j][x_Max])
                                                y_Max[PlotType_VREP]=y[j][x_Max];
                                            break;
                    case PlotName_BRNG :    if (y_Max[PlotType_BRNG]<y[j][x_Max])
                                                y_Max[PlotType_BRNG]=y[j][x_Max];
                                            break;
                    case PlotName_HEAD :    if (y_Max[PlotType_HEAD]<y[j][x_Max])
                                                y_Max[PlotType_HEAD]=y[j][x_Max];
                                            break;
                    default:                ;
                }

                j++;
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
string FFmpeg_Glue::StatsToExternalData()
{
    stringstream Value;
    Value<<",,,,,,,,,,,,,,,,,,,,,YMIN,YLOW,YAVG,YHIGH,YMAX,UMIN,ULOW,UAVG,UHIGH,UMAX,VMIN,VLOW,VAVG,VHIGH,VMAX,YDIF,UDIF,VDIF,YDIF1,YDIF2,TOUT,VREP,BRNG,HEAD";
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
            Value<<','<<fixed<<setprecision(PlotValues_DigitsAfterComma[Pos2])<<y[Pos2][Pos];
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