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
    VideoFrameCount_Max=0;
    VideoDuration=0;
    VideoFirstTimeStamp=(uint64_t)-1;
    IsComplete=false;
    
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

    //Stats
    memset(Stats_Totals, 0x00, PlotName_Max*sizeof(double));
    memset(Stats_Counts, 0x00, PlotName_Max*sizeof(uint64_t));
    memset(Stats_Counts2, 0x00, PlotName_Max*sizeof(uint64_t));

    // FFmpeg init
    av_register_all();
    avfilter_register_all();
    //av_log_set_callback(avlog_cb);

    // Container part
    VideoStream=NULL;
    AudioStream=NULL;
    if (avformat_open_input(&FormatContext, FileName.c_str(), NULL, NULL)>=0)
    {
        if (avformat_find_stream_info(FormatContext, NULL)>=0)
        {
            // Video stream
            VideoStream_Index=av_find_best_stream(FormatContext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
            if (VideoStream_Index>=0)
            {
                VideoStream=FormatContext->streams[VideoStream_Index];

                // Video codec
                if (VideoStream)
                {
                    AVCodec* VideoStream_Codec=avcodec_find_decoder(VideoStream->codec->codec_id);
                    if (VideoStream_Codec)
                        avcodec_open2(VideoStream->codec, VideoStream_Codec, NULL);
                }
            }

            // Audio stream
            int AudioStream_Index=av_find_best_stream(FormatContext, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
            if (AudioStream_Index>=0)
            {
                AudioStream=FormatContext->streams[AudioStream_Index];

                if (AudioStream)
                {
                    AVCodec* AudioStream_Codec=avcodec_find_decoder(AudioStream->codec->codec_id);
                    if (AudioStream_Codec)
                        avcodec_open2(AudioStream->codec, AudioStream_Codec, NULL);
                }
            }
        }
    }

    // Frame
    Frame = av_frame_alloc();
    if (!Frame)
        return;

    // Packet
    Packet = new AVPacket;
    av_init_packet(Packet);
    Packet->data = NULL;
    Packet->size = 0;

    // Output
    if (VideoStream)
    {
        VideoFrameCount_Max=VideoFrameCount=VideoStream->nb_frames;
        if (VideoStream->duration!=AV_NOPTS_VALUE)
            VideoDuration=((double)VideoStream->duration)*VideoStream->time_base.num/VideoStream->time_base.den;

        // If video duration is not known, estimating it
        if (VideoDuration==0 && FormatContext->duration!=AV_NOPTS_VALUE)
            VideoDuration=((double)FormatContext->duration)/AV_TIME_BASE;

        // If frame count is not known, estimating it
        if (VideoFrameCount==0 && VideoStream->avg_frame_rate.num && VideoStream->avg_frame_rate.den && VideoDuration)
            VideoFrameCount=VideoDuration*VideoStream->avg_frame_rate.num/VideoStream->avg_frame_rate.den;
        if (VideoFrameCount==0
         && ((VideoStream->time_base.num==1 && VideoStream->time_base.den>=24 && VideoStream->time_base.den<=60)
          || (VideoStream->time_base.num==1001 && VideoStream->time_base.den>=24000 && VideoStream->time_base.den<=60000)))
            VideoFrameCount=VideoStream->duration;
    }
    if (VideoFrameCount_Max==0)
    {
        VideoFrameCount_Max=3*60*60*30; // 3 hours max for the moment
    }
    if (OutputMethod==Output_JpegList)
    {
        JpegList.reserve(VideoFrameCount_Max);
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
        x = new double*[4];
        memset(x, 0, 4*sizeof(double*));
        d = new double[VideoFrameCount_Max];
        memset(d, 0x00, VideoFrameCount_Max*sizeof(double));
        y = new double*[PlotName_Max];
        memset(y, 0, PlotName_Max*sizeof(double*));
        memset(y_Max, 0, PlotType_Max*sizeof(double));

        for(size_t j=0; j<4; ++j)
        {
            x[j]=new double[VideoFrameCount_Max];
            memset(x[j], 0x00, VideoFrameCount_Max*sizeof(double));
        }
        for(size_t j=0; j<PlotName_Max; ++j)
        {
            y[j] = new double[VideoFrameCount_Max];
            memset(y[j], 0x00, VideoFrameCount_Max*sizeof(double));
        }
        for(size_t j=0; j<PlotType_Max; ++j)
            y_Max[j]=0; //PerPlotGroup[j].Max;

        key_frame=new bool[VideoFrameCount_Max];
        memset(key_frame, 0x00, VideoFrameCount_Max*sizeof(bool));
    }
    x_Max=0;
    x_Line_Begin=0;
    VideoFramePos=0;

    // Temp
    DTS_Target=-1;

    // Stats from external data
    // XML input
    QXmlStreamReader Xml(StatsFromExternalData.c_str());
    while (!Xml.atEnd())
    {
        if (Xml.readNextStartElement())
        {
            if (Xml.name()=="ffprobe")
            {
                while (Xml.readNextStartElement())
                {
                    if (Xml.name()=="frames")
                    {
                        while (Xml.readNextStartElement())
                        {
                            if (Xml.name()=="frame" && Xml.attributes().value("media_type")=="video")
                            {
                                x[0][x_Max]=x_Max;
                                d[x_Max]=std::atof(Xml.attributes().value("pkt_duration_time").toString().toUtf8().data());
                                string ts=Xml.attributes().value("pkt_pts_time").toString().toUtf8().data();
                                if (ts.empty() || ts=="N/A")
                                    ts=Xml.attributes().value("pkt_dts_time").toString().toUtf8().data(); // Using DTS is PTS is not available
                                if (!ts.empty() && ts!="N/A")
                                {
                                    x[1][x_Max]=std::atof(ts.c_str());
                                    x[2][x_Max]=x[1][x_Max]/60;
                                    x[3][x_Max]=x[2][x_Max]/60;
                                }

                                int Width=atoi(Xml.attributes().value("width").toString().toUtf8().data());
                                int Height=atoi(Xml.attributes().value("height").toString().toUtf8().data());

                                while (Xml.readNextStartElement())
                                {
                                    if (Xml.name()=="tag")
                                    {
                                        PlotName j=PlotName_Max;
                                        for (size_t Plot_Pos=0; Plot_Pos<PlotName_Max; Plot_Pos++)
                                            if (Xml.attributes().value("key")==PerPlotName[Plot_Pos].FFmpeg_Name_2_3)
                                                j=(PlotName)Plot_Pos;

                                        if (j!=PlotName_Max)
                                        {
                                            double value=std::atof(Xml.attributes().value("value").toString().toUtf8().data());
                                            
                                            // Special cases: crop: x2, y2
                                            if (Width && Xml.attributes().value("key")=="lavfi.cropdetect.x2")
                                                y[j][x_Max]=Width-value;
                                            else if (Height && Xml.attributes().value("key")=="lavfi.cropdetect.y2")
                                                y[j][x_Max]=Height-value;
                                            else if (Width && Xml.attributes().value("key")=="lavfi.cropdetect.w")
                                                y[j][x_Max]=Width-value;
                                            else if (Height && Xml.attributes().value("key")=="lavfi.cropdetect.h")
                                                y[j][x_Max]=Height-value;
                                            else
                                                y[j][x_Max]=value;

                                            if (PerPlotName[j].Group1!=PlotType_Max && y_Max[PerPlotName[j].Group1]<y[j][x_Max])
                                                y_Max[PerPlotName[j].Group1]=y[j][x_Max];
                                            if (PerPlotName[j].Group2!=PlotType_Max && y_Max[PerPlotName[j].Group2]<y[j][x_Max])
                                                y_Max[PerPlotName[j].Group2]=y[j][x_Max];

                                            //Stats
                                            Stats_Totals[j]+=y[j][x_Max];
                                            if (PerPlotName[j].DefaultLimit!=DBL_MAX)
                                            {
                                                if (y[j][x_Max]>PerPlotName[j].DefaultLimit)
                                                    Stats_Counts[j]++;
                                                if (PerPlotName[j].DefaultLimit2!=DBL_MAX && y[j][x_Max]>PerPlotName[j].DefaultLimit2)
                                                    Stats_Counts2[j]++;
                                            }
                                        }
                                    }
                                    Xml.skipCurrentElement();
                                }

                                QStringRef key_frame_String=Xml.attributes().value("key_frame");
                                if (key_frame_String.size()>0)
                                    key_frame[x_Max]=std::atof(key_frame_String.toString().toUtf8().data());
                                else
                                    key_frame[x_Max]=1; //Forcing key_frame to 1 if it is missing from the XML, for decoding

                                x_Max++;
                                if (x_Max>=VideoFrameCount)
                                {
                                    VideoFrameCount=x_Max+1;
                                    if (!ts.empty() && ts!="N/A")
                                        VideoDuration=x[1][x_Max-1]+d[x_Max-1];
                                    if (FormatContext==NULL)
                                        VideoFramePos=VideoFrameCount;
                                }
                            }
                            else
                                Xml.skipCurrentElement();
                        }
                    }
                    else
                        Xml.skipCurrentElement();
                }
            }
            else
                Xml.skipCurrentElement();
        }
    }
}

//---------------------------------------------------------------------------
FFmpeg_Glue::~FFmpeg_Glue()
{
    if (VideoFrameCount_Max==0)
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
// Stats
//***************************************************************************

//---------------------------------------------------------------------------
string FFmpeg_Glue::Stats_Average_Get(PlotName Pos)
{
    if (x_Max==0)
        return string();

    double Value=Stats_Totals[Pos]/x_Max;
    stringstream str;
    str<<fixed<<setprecision(PerPlotName[Pos].DigitsAfterComma)<<Value;
    return str.str();
}

//---------------------------------------------------------------------------
string FFmpeg_Glue::Stats_Average_Get(PlotName Pos, PlotName Pos2)
{
    if (x_Max==0)
        return string();

    double Value=(Stats_Totals[Pos]-Stats_Totals[Pos2])/x_Max;
    stringstream str;
    str<<fixed<<setprecision(PerPlotName[Pos].DigitsAfterComma)<<Value;
    return str.str();
}

//---------------------------------------------------------------------------
string FFmpeg_Glue::Stats_Count_Get(PlotName Pos)
{
    if (x_Max==0)
        return string();

    stringstream str;
    str<<Stats_Counts[Pos];
    return str.str();
}

//---------------------------------------------------------------------------
string FFmpeg_Glue::Stats_Count2_Get(PlotName Pos)
{
    if (x_Max==0)
        return string();

    stringstream str;
    str<<Stats_Counts2[Pos];
    return str.str();
}

//---------------------------------------------------------------------------
string FFmpeg_Glue::Stats_Percent_Get(PlotName Pos)
{
    if (x_Max==0)
        return string();

    double Value=Stats_Counts[Pos]/x_Max;
    stringstream str;
    str<<Value*100<<"%";
    return str.str();
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
    DTS/=VideoFrameCount;  // TODO: seek based on time stamp
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
        /* CSV input disabled
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
                x[0][x_Max]=x_Max;
        
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
                        x[1][x_Max]=std::atof(Value.data());
                        x[2][x_Max]=x[1][x_Max]/60;
                        x[3][x_Max]=x[2][x_Max]/60;
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
        */
    }
    
    if (!FormatContext)
        return;

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
    
    //Complete
    IsComplete=true;
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
            TempPacket->data+=TempPacket->size;
            TempPacket->size=0;
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
        if (VideoFramePos>VideoFrameCount)
            VideoFrameCount=VideoFramePos;
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
    Scale_Frame=av_frame_alloc();
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
        av_frame_free(&Scale_Frame);
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
            x[0][x_Max]=x_Max;
            int64_t ts=Frame->pkt_dts;
            if (ts==AV_NOPTS_VALUE && x_Max)
                ts=(int)((x[1][x_Max-1]*x_Max/(x_Max-1))/VideoStream->time_base.num*VideoStream->time_base.den)+VideoFirstTimeStamp; //TODO: understand how to do with first timestamp not being 0 and last timestamp being AV_NOPTS_VALUE e.g. op1a-mpeg2-wave_hd.mxf
            //int64_t ts=(Frame->pkt_pts==AV_NOPTS_VALUE)?Frame->pkt_dts:Frame->pkt_pts; // Using DTS is PTS is not available // TODO: check if stats are based on DTS or PTS
            if (VideoFirstTimeStamp==(uint64_t)-1)
                VideoFirstTimeStamp=ts;
            if (ts<VideoFirstTimeStamp)
            {
                for (size_t Pos=0; Pos<x_Max; Pos++)
                {
                    x[1][Pos]-=VideoFirstTimeStamp-ts;
                    x[2][Pos]=x[1][Pos]/60;
                    x[3][Pos]=x[2][Pos]/60;
                }
            }
            ts-=VideoFirstTimeStamp;
            if (ts!=AV_NOPTS_VALUE)
            {
                x[1][x_Max]=((double)ts)*VideoStream->time_base.num/VideoStream->time_base.den;
                if (x[1][x_Max]>VideoDuration)
                    VideoDuration=x[1][x_Max];
                x[2][x_Max]=x[1][x_Max]/60;
                x[3][x_Max]=x[2][x_Max]/60;
            }
            if (Frame->pkt_duration!=AV_NOPTS_VALUE)
                d[x_Max]=((double)Frame->pkt_duration)*VideoStream->time_base.num/VideoStream->time_base.den;

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
                    if (strcmp(e->key, PerPlotName[j].FFmpeg_Name_2_3)==0)
                        break;
                }

                if (j<PlotName_Max)
                {
                    double value=std::atof(e->value);
                                            
                    // Special cases: crop: x2, y2
                    if (string(e->key)=="lavfi.cropdetect.x2")
                        y[j][x_Max]=Width_Get()-value;
                    else if (string(e->key)=="lavfi.cropdetect.y2")
                        y[j][x_Max]=Height_Get()-value;
                    else if (string(e->key)=="lavfi.cropdetect.w")
                        y[j][x_Max]=Width_Get()-value;
                    else if (string(e->key)=="lavfi.cropdetect.h")
                        y[j][x_Max]=Height_Get()-value;
                    else
                        y[j][x_Max]=value;

                    if (PerPlotName[j].Group1!=PlotType_Max && y_Max[PerPlotName[j].Group1]<y[j][x_Max])
                        y_Max[PerPlotName[j].Group1]=y[j][x_Max];
                    if (PerPlotName[j].Group2!=PlotType_Max && y_Max[PerPlotName[j].Group2]<y[j][x_Max])
                        y_Max[PerPlotName[j].Group2]=y[j][x_Max];

                    //Stats
                    Stats_Totals[j]+=y[j][x_Max];
                    if (PerPlotName[j].DefaultLimit!=DBL_MAX)
                    {
                        if (y[j][x_Max]>PerPlotName[j].DefaultLimit)
                            Stats_Counts[j]++;
                        if (PerPlotName[j].DefaultLimit2!=DBL_MAX && y[j][x_Max]>PerPlotName[j].DefaultLimit2)
                            Stats_Counts2[j]++;
                    }
                }

                /*
                A+=e->key;
                A+=',';
                A+=e->value;
                A+="\r\n";
                */
            }

            key_frame[x_Max]=Frame->key_frame;

            x_Max++;
            if (x_Max>=VideoFrameCount)
                VideoFrameCount=x_Max+1;
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
// Real time information
//***************************************************************************

//---------------------------------------------------------------------------
double FFmpeg_Glue::State_Get()
{
    if (IsComplete || VideoFrameCount==0)
        return 1;

    double Value=((double)VideoFramePos)/VideoFrameCount;
    if (Value>=1)
        Value=0.99999; // It is not yet complete, so not 100%

    return Value;
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
double FFmpeg_Glue::VideoFrameRate_Get()
{
    if (VideoStream==NULL || VideoStream->codec==NULL || VideoStream->codec->codec==NULL || VideoStream->codec->codec->long_name==NULL)
        return 0;

    if (VideoStream->avg_frame_rate.num && VideoStream->avg_frame_rate.den)
        return ((double)VideoStream->avg_frame_rate.num)/VideoStream->avg_frame_rate.den;
    
    if (VideoFrameCount && VideoDuration)
        return VideoFrameCount/VideoDuration;

    return 0; // Unknown
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
int FFmpeg_Glue::KeyFrame_Get(size_t FramePos)
{
    if (FramePos>=VideoFrameCount)
        return 1;

    return key_frame[FramePos];
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
    Value<<",,,,,,pts,,,,,,,,,,,,,,,YMIN,YLOW,YAVG,YHIGH,YMAX,UMIN,ULOW,UAVG,UHIGH,UMAX,VMIN,VLOW,VAVG,VHIGH,VMAX,YDIF,UDIF,VDIF,SATMIN,SATLOW,SATAVG,SATHIGH,SATMAX,HUEMED,HUEAVG,TOUT,VREP,BRNG,CROPx1,CROPx2,CROPy1,CROPy2,CROPw,CROPh,MSEy,MSEu,MSEv,PSNRy,PSNRu,PSNRv";
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
        Value<<fixed<<setprecision(6)<<x[1][Pos];
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

