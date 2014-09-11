/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef FFmpeg_Glue_H
#define FFmpeg_Glue_H

#include "Core/Core.h"

#include <string>
#include <vector>
using namespace std;

struct AVFormatContext;
struct AVStream;
struct AVFrame;

struct AVCodec;
struct AVCodecContext;
struct SwsContext;
struct AVPacket;
struct AVFilterContext;
struct AVFilterGraph;
struct AVFilter;
struct AVFilterInOut;
struct AVFilterInOut;

class QImage;

class FFmpeg_Glue
{
public:
    // Constructor / Destructor
    enum outputmethod
    {
        Output_None,
        Output_QImage,
        Output_Jpeg,
        Output_JpegList,
    };
    FFmpeg_Glue(const string &FileName, int Scale_Width=0, int Scale_Height=0, outputmethod OutputMethod=Output_None, const string &Filter1=string(), const string &Filter2=string(), bool With1=true, bool With2=false, bool WithStats=false, const string &StatsFromExternalData=string());
    ~FFmpeg_Glue();

    // In
    string                      StatsFromExternalData;

    // Out
    QImage*                     Image1;
    QImage*                     Image2;
    struct bytes
    {
        unsigned char* Data;
        size_t         Size;
    };
    bytes                       Jpeg;
    std::vector<bytes*>         JpegList;
    double**                    x; //PTS of frame number
    double**                    y; //Data
    bool*                       key_frame; //May be useful for FFmpeg XML output
    double                      y_Max[PlotType_Max];
    size_t                      x_Max;
    size_t                      VideoFramePos;
    size_t                      VideoFrameCount;
    size_t                      VideoFrameCount_Max;
    double                      VideoDuration;

    // Infos
    size_t                      Frames_Total_Get()                                                                      {return VideoFrameCount;}
    double                      Duration_Get()                                                                          {return VideoDuration;}
    string                      ContainerFormat_Get();
    int                         StreamCount_Get();
    string                      VideoFormat_Get();
    int                         Width_Get();
    int                         Height_Get();
    int                         KeyFrame_Get(size_t FramePos);
    double                      DAR_Get();
    string                      PixFormat_Get();
    string                      AudioFormat_Get();
    string                      SampleFormat_Get();
    int                         SamplingRate_Get();
    string                      ChannelLayout_Get();
    int                         BitDepth_Get();
    string                      StatsToExternalData();
    string                      FFmpeg_Version();
    int                         FFmpeg_Year();
    string                      FFmpeg_Compiler();
    string                      FFmpeg_Configuration();
    string                      FFmpeg_LibsVersion();

    // Actions
    void                        Seek (size_t Pos);
    void                        FrameAtPosition(size_t Pos);
    void                        NextFrame();
    bool                        OutputFrame(AVPacket* Packet, bool Decode=true);
    void                        Filter1_Change(const string &Filter);
    void                        Filter2_Change(const string &Filter);
    void                        With1_Change(bool With);
    void                        With2_Change(bool With);
    void                        Scale_Change(int Scale_Width, int Scale_Height);

private:
    // FFmpeg related actions
    bool                        Scale_Adapt(AVFrame* Frame);
    bool                        FilterGraph_Init(AVFilterGraph* &FilterGraph, AVFilterContext* &FilterGraph_Source_Context, AVFilterContext* &FilterGraph_Sink_Context, const string &Filter);
    void                        FilterGraph_Free(AVFilterGraph* &FilterGraph);
    bool                        Scale_Init(AVFrame* FilteredFrame, SwsContext* &Scale_Context, AVFrame* &Scale_Frame);
    void                        Scale_Free(AVFrame* FilteredFrame, SwsContext* &Scale_Context, AVFrame* &Scale_Frame);
    bool                        Process(AVFrame* &FilteredFrame, AVFilterGraph* &FilterGraph, AVFilterContext* &FilterGraph_Source_Context, AVFilterContext* &FilterGraph_Sink_Context, const string &Filter, SwsContext* &Scale_Context, AVFrame* &Scale_Frame, QImage* &Image);
    bool                        JpegOutput_Init();
    
    // FFmpeg pointers - Input
    AVFormatContext*            FormatContext;
    AVStream*                   VideoStream;
    AVStream*                   AudioStream;
    int                         VideoStream_Index;
    AVFrame*                    Frame;
    AVPacket*                   Packet;

    // FFmpeg pointers - Filter
    AVFilterGraph*              FilterGraph1;
    AVFilterGraph*              FilterGraph2;
    AVFilterContext*            FilterGraph1_Source_Context;
    AVFilterContext*            FilterGraph2_Source_Context;
    AVFilterContext*            FilterGraph1_Sink_Context;
    AVFilterContext*            FilterGraph2_Sink_Context;
    AVFrame*                    Filtered1_Frame;
    AVFrame*                    Filtered2_Frame;

    // FFmpeg pointers - Scale
    SwsContext*                 Scale1_Context;
    SwsContext*                 Scale2_Context;
    AVFrame*                    Scale1_Frame;
    AVFrame*                    Scale2_Frame;

    // FFmpeg pointers - Output
    AVCodecContext*             JpegOutput_CodecContext;
    AVPacket*                   JpegOutput_Packet;

    // In
    string                      FileName;
    string                      Filter1;
    string                      Filter2;
    int                         Scale_Width;
    int                         Scale_Height;
    bool                        Scale_Adapted;
    outputmethod                OutputMethod;
    bool                        WithStats;
    bool                        With1;
    bool                        With2;

    // Temp
    int                         DTS_Target;
    size_t                      x_Line_Begin;
    size_t                      x_Line_EolSize;
    char                        x_Line_EolChar;
};

#endif // FFmpeg_Glue_H
