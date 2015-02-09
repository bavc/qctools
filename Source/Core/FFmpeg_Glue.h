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
class CommonStats;

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
    FFmpeg_Glue(const string &FileName, std::vector<CommonStats*>* Stats, bool WithStats=false);
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
    std::vector<CommonStats*>*  Stats;

    // Container information
    string                      ContainerFormat_Get();
    int                         StreamCount_Get();
    int                         BitRate_Get();

    // Video information
    string                      VideoFormat_Get();
    double                      VideoDuration_Get();
    double                      FramesDivDuration_Get();
    string                      RVideoFrameRate_Get();
    string                      AvgVideoFrameRate_Get();
    size_t                      VideoFrameCount_Get();
    int                         Width_Get();
    int                         Height_Get();
    string                      FieldOrder_Get();
    double                      DAR_Get();
    string                      SAR_Get();
    string                      PixFormat_Get();
    string                      ColorSpace_Get();
    string                      ColorRange_Get();
    
    // Audio information
    string                      AudioFormat_Get();
    string                      SampleFormat_Get();
    int                         SamplingRate_Get();
    string                      ChannelLayout_Get();
    int                         ABitDepth_Get();
    
    // FFmpeg information
    string                      FFmpeg_Version();
    int                         FFmpeg_Year();
    string                      FFmpeg_Compiler();
    string                      FFmpeg_Configuration();
    string                      FFmpeg_LibsVersion();
 
    // Actions
    void                        AddInput_Video(size_t FrameCount, int time_base_num, int time_base_den, int Width, int Height, int BitDepth);
    void                        AddInput_Audio(size_t FrameCount, int time_base_num, int time_base_den, int Samplerate, int BitDepth, int OutputBitDepth, int Channels);
    void                        AddOutput(size_t FilterPos, int Scale_Width=0, int Scale_Height=0, outputmethod OutputMethod=Output_None, int FilterType=0, const string &Filter=string());
    void                        AddOutput(const string &FileName);
    void                        CloseOutput();
    void                        ModifyOutput(size_t InputPos, size_t OutputPos, size_t FilterPos, int Scale_Width=0, int Scale_Height=0, outputmethod OutputMethod=Output_None, int FilterType=0, const string &Filter=string());
    void                        Seek(size_t Pos);
    void                        FrameAtPosition(size_t Pos);
    bool                        NextFrame();
    bool                        OutputFrame(AVPacket* Packet, bool Decode=true);
    bool                        OutputFrame(unsigned char* Data, size_t Size, int stream_index, int FramePos);
    void                        Filter_Change(size_t FilterPos, int FilterType, const string &Filter);
    void                        Disable(const size_t Pos);
    void                        Scale_Change(int Scale_Width, int Scale_Height);
    void                        Thumbnails_Modulo_Change(size_t Modulo);

    // Between different FFmpeg_Glue instances
    void*                       InputData_Get() { return InputDatas[0]; }
    void                        InputData_Set(void* InputData) {InputDatas.push_back((inputdata*)InputData); InputDatas_Copy=true;}

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
        double                  FirstTimeStamp;         // First PTS met in seconds
        double                  Duration;               // Duration in seconds

        // Cache
        std::vector<AVFrame*>*  FramesCache;
        AVFrame*                FramesCache_Default;

        // Encode
        bool                    InitEncode();
        void                    Encode(AVPacket* SourcePacket);
        void                    CloseEncode();
        AVFormatContext*        Encode_FormatContext;   // copy of pointer, do not delete
        AVCodecContext*         Encode_CodecContext;
        AVStream*               Encode_Stream;
        AVPacket*               Encode_Packet;
        int                     Encode_CodecID;
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
        size_t                  FilterPos;

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
        size_t                  Thumbnails_Modulo;
        CommonStats*            Stats;

        // Status
        size_t                  FramePos;               // Current position of playback

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
    bool                        InputDatas_Copy;

    // FFmpeg pointers - Input
    AVFormatContext*            FormatContext;
    AVPacket*                   Packet;
    AVFrame*                    Frame;

    // In
    string                      FileName;
    bool                        WithStats;

    // Encode
    bool                        InitEncode();
    void                        Encode();
    void                        CloseEncode();
    string                      Encode_FileName;
    AVFormatContext*            Encode_FormatContext;

    // Seek
    int64_t                     Seek_TimeStamp;
};

#endif // FFmpeg_Glue_H
