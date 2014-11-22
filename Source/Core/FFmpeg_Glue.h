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
#include <stdint.h>
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
class VideoStats;

class FFmpeg_Glue
{
public:
    // Constructor / Destructor
    enum outputmethod
    {
        Output_None,
        Output_QImage,
        Output_Jpeg,
        Output_Stats,
    };
    FFmpeg_Glue(const string &FileName, std::vector<VideoStats*>* Stats, bool WithStats=false);
    ~FFmpeg_Glue();

    // Images
    QImage*                     Image_Get(size_t Pos)                                                                   {if (Pos>=OutputDatas.size() || !OutputDatas[Pos] || !OutputDatas[Pos]->Enabled) return NULL; return OutputDatas[Pos]->Image;}
    struct bytes
    {
        unsigned char* Data;
        size_t         Size;

        bytes()
            :
            Data(NULL),
            Size(0)
        {
        }

        ~bytes()
        {
            delete[] Data;
        }
    };
    bytes*                      Thumbnail_Get(size_t Pos, size_t FramePos)                                              {if (Pos>=OutputDatas.size() || !OutputDatas[Pos] || !OutputDatas[Pos]->Enabled) return NULL; return OutputDatas[Pos]->Thumbnails[FramePos];}
    size_t                      Thumbnails_Size(size_t Pos)                                                             {if (Pos>=OutputDatas.size() || !OutputDatas[Pos] || !OutputDatas[Pos]->Enabled) return 0; return OutputDatas[Pos]->Thumbnails.size();}
    
    // Status
    size_t                      VideoFramePos_Get();

    // Data
    std::vector<VideoStats*>*   Stats;

    // Container information
    string                      ContainerFormat_Get();
    int                         StreamCount_Get();
	int                         BitRate_Get();

    // Video information
    string                      VideoFormat_Get();
    double                      VideoDuration_Get();
    double                      VideoFrameRate_Get();
    size_t                      VideoFrameCount_Get();
    int                         Width_Get();
    int                         Height_Get();
    double                      DAR_Get();
    string                      PixFormat_Get();
    
    // Audio information
    string                      AudioFormat_Get();
    string                      SampleFormat_Get();
    int                         SamplingRate_Get();
    string                      ChannelLayout_Get();
    int                         BitDepth_Get();
    
    // FFmpeg information
    string                      FFmpeg_Version();
    int                         FFmpeg_Year();
    string                      FFmpeg_Compiler();
    string                      FFmpeg_Configuration();
    string                      FFmpeg_LibsVersion();
 
    // Actions
    void                        AddOutput(int Scale_Width=0, int Scale_Height=0, outputmethod OutputMethod=Output_None, const string &Filter=string());
    void                        ModifyOutput(size_t Pos, int Scale_Width=0, int Scale_Height=0, outputmethod OutputMethod=Output_None, const string &Filter=string());
    void                        Seek(size_t Pos);
    void                        FrameAtPosition(size_t Pos);
    bool                        NextFrame();
    bool                        OutputFrame(AVPacket* Packet, bool Decode=true);
    void                        Filter_Change(const size_t Pos, const string &Filter);
    void                        Disable(const size_t Pos);
    void                        Scale_Change(int Scale_Width, int Scale_Height);

private:
    // Stream information
    struct inputdata
    {
        // Constructor / Destructor
        inputdata();
        ~inputdata();
        
        //Actions

        // In
        bool                    Enabled;

        // FFmpeg pointers - Input
        int                     Type;
        AVStream*               Stream;
        AVFrame*                DecodedFrame;

        // Status
        size_t                  FramePos;               // Current position of playback
        
        // General information
        size_t                  FrameCount;             // Total count of frames (may be estimated)
        size_t                  FrameCount_Max;         // Temporary usage for array max size
        double                  FirstTimeStamp;         // First PTS met in seconds
        double                  Duration;               // Duration in seconds
    };
    struct outputdata
    {
        // Constructor / Destructor
        outputdata();
        ~outputdata();
        
        //Actions
        void                    Process(AVFrame* DecodedFrame);
        void                    ApplyFilter();
        void                    ApplyScale();
        void                    ReplaceImage();
        void                    AddThumbnail();
        void                    DiscardScaledFrame();
        void                    DiscardFilteredFrame();

        // In
        bool                    Enabled;
        string                  Filter;

        // FFmpeg pointers - Input
        int                     Type;
        AVStream*               Stream;
        AVFrame*                DecodedFrame;

        // FFmpeg pointers - Filter
        AVFilterGraph*          FilterGraph;
        AVFilterContext*        FilterGraph_Source_Context;
        AVFilterContext*        FilterGraph_Sink_Context;
        AVFrame*                FilteredFrame;

        // FFmpeg pointers - Scale
        SwsContext*             ScaleContext;
        AVFrame*                ScaledFrame;

        // FFmpeg pointers - Output
        AVCodecContext*         JpegOutput_CodecContext;
        AVPacket*               JpegOutput_Packet;

        // Out
        outputmethod            OutputMethod;
        QImage*                 Image;
        std::vector<bytes*>     Thumbnails;
        VideoStats*             Stats;

        // Helpers
        bool                    InitThumnails();
        bool                    FilterGraph_Init();
        void                    FilterGraph_Free();
        bool                    Scale_Init();
        void                    Scale_Free();
        bool                    AdaptDAR();
        int                     Width;
        int                     Height;
    };
    std::vector<inputdata*>     InputDatas;
    std::vector<outputdata*>    OutputDatas;

    // FFmpeg pointers - Input
    AVFormatContext*            FormatContext;
    AVPacket*                   Packet;
    AVFrame*                    Frame;

    // In
    string                      FileName;
    bool                        WithStats;

    // Seek
    int64_t                     Seek_TimeStamp;
};

#endif // FFmpeg_Glue_H
