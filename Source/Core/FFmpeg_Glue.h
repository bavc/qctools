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
#include <memory>
#include <functional>
#include <stdint.h>
#include <QByteArray>
#include <QMutex>
#include <QString>
#include <QSize>
#include <map>
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

class CommonStats;
class StreamsStats;
class FormatStats;

class FFmpeg_Glue
{
public:

    typedef std::shared_ptr<AVFrame> AVFramePtr;
    typedef std::shared_ptr<AVPacket> AVPacketPtr;
    typedef std::pair<std::string, int> StreamInfo;

    struct FrameRate {
        FrameRate(int  num, int den) : num(num), den(den) {}

        int num;
        int den;

        double value() {
            return double(num) / den;
        }

        bool isValid() const {
            return num > 0 && den > 0;
        }
    };

    // Constructor / Destructor
    enum outputmethod
    {
        Output_None,
        Output_QImage,
        Output_Jpeg,
        Output_Panels,
        Output_Stats,
    };

    struct Image {
        Image();

        bool isNull() const {
            return frame == NULL;
        }

        const uchar* data() const;
        int width() const;
        int height() const;
        int linesize() const;

        void free();
        AVFramePtr frame;
    };

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
        int                     TimecodeBCD;            // Timecode in BCD format

        // Cache
        std::vector<AVFrame*>*  FramesCache;
        AVFrame*                FramesCache_Default;
    };
    struct outputdata
    {
        // Constructor / Destructor
        outputdata(int index);
        ~outputdata();

        int index = {0};

        //Actions
        void                    Process(AVFrame* DecodedFrame);
        void                    ApplyFilter(const AVFramePtr& sourceFrame);
        void                    ApplyScale(const AVFramePtr& sourceFrame);
        void                    ReplaceImage();
        void                    AddThumbnail();
        void                    AddPanel();

        // In
        bool                    Enabled;
        string                  Filter;
        string                  Title;

        // FFmpeg pointers - Input
        int                     Type;
        AVStream*               Stream;
        AVFramePtr              DecodedFrame;
        int64_t                 AccumulatedDuration = {0}; // durating of all the frames contributing to filtered frame

        // FFmpeg pointers - Filter
        AVFilterGraph*          FilterGraph;
        AVFilterContext*        FilterGraph_Source_Context;
        AVFilterContext*        FilterGraph_Sink_Context;
        AVFramePtr              FilteredFrame;

        // FFmpeg pointers - Scale
        SwsContext*             ScaleContext;
        AVFramePtr              ScaledFrame;
        int                     Scale_OutputPixelFormat;
        AVFramePtr              OutputFrame;


        // FFmpeg pointers - Output
        AVCodecContext*         Output_CodecContext;
        int                     Output_PixelFormat;
        int                     Output_CodecID;

        // Out
        outputmethod            OutputMethod;
        Image                   image;

        struct AVPacketDeleter {
            void operator()(AVPacket* packet);
        };

        std::vector<std::shared_ptr<AVPacket>>  Thumbnails;
        std::vector<AVFramePtr> Panels;

        size_t                  Thumbnails_Modulo;
        CommonStats*            Stats;

        // Status
        size_t                  FramePos;               // Current position of playback
        double                  TimeStamp;              // Current position of playback

        // Helpers
        bool                    initEncoder(const QSize& size);;
        bool                    FilterGraph_Init();
        void                    FilterGraph_Free();
        bool                    Scale_Init(AVFrame* frame);
        void                    Scale_Free();
        bool                    AdaptDAR();
        double                  GetDAR();

        int                     Width;
        int                     Height;
        bool scaleBeforeEncoding = { false }; // used to change format before encoding

        std::unique_ptr<AVPacket, AVPacketDeleter> encodeFrame(AVFrame* frame, bool* ok = nullptr);
    };

    FFmpeg_Glue(const string &FileName, activealltracks ActiveAllTracks, std::vector<CommonStats*>* Stats, StreamsStats** streamsStats, FormatStats** formatStats, bool WithStats=false);
    ~FFmpeg_Glue();

    // Images
    Image Image_Get(size_t Pos) const;
    FFmpeg_Glue::AVPacketPtr encodePanelFrame(int outputIndex, AVFrame* frame);

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

    std::shared_ptr<AVPacket>   ThumbnailPacket_Get(size_t Pos, size_t FramePos);
    QByteArray                  Thumbnail_Get(size_t Pos, size_t FramePos);
    size_t                      Thumbnails_Size(size_t Pos);
    
    // Status
    size_t                      VideoFramePos_Get();

    // Data
    std::vector<CommonStats*>*  Stats;

    bool                        withStats() const;
    // Container information
    string                      ContainerFormat_Get();
    int                         StreamCount_Get();

    enum StreamType {
        Thumbnails,
        Panels
    };

    FrameRate getFrameRate(int streamIndex) const;
    FrameRate getAvgVideoFrameRate() const;

    int getStreamType(int streamIndex) const;
    int sampleRate(int streamIndex) const;

    std::vector<StreamInfo>     findStreams(StreamType type);
    std::vector<int>            findStreams(const std::function<bool(AVStream* stream)>& criteria);
    std::vector<int>            findMediaStreams();
    std::vector<int>            findVideoStreams();
    std::vector<int>            findAudioStreams();
    int findInputStreamByOutput(size_t pos) const;
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

    double                      OutputDAR_Get(int Pos);
    int                         OutputWidth_Get(int Pos);
    int                         OutputHeight_Get(int Pos);

    int                         OutputThumbnailWidth_Get() const;
    int                         OutputThumbnailHeight_Get() const;
    int                         OutputThumbnailBitRate_Get() const;

    std::string getOutputFilter(int pos) const;
    void getOutputTimeBase(int pos, int& num, int& den) const;

    QString                     FrameType_Get() const;
    string                      PixFormat_Get();
    string                      PixFormatName_Get();
    string                      ColorSpace_Get();
    string                      ColorRange_Get();
    int                         IsRGB_Get();
    int                         BitsPerRawSample_Get(int streamType = Type_Video);

    // Audio information
    string                      AudioFormat_Get();
    string                      SampleFormat_Get();
    int                         sampleFormat();
    int                         SamplingRate_Get();
    string                      ChannelLayout_Get();
    int                         ABitDepth_Get();
    
    // FFmpeg information
    static string               FFmpeg_Version();
    static int                  FFmpeg_Year();
    static string               FFmpeg_Compiler();
    static string               FFmpeg_Configuration();
    static string               FFmpeg_LibsVersion();

    static QByteArray           getAttachment(const QString& fileName, QString& attachmentFileName);
    static int                  guessBitsPerRawSampleFromFormat(int pixelFormat);
    static int                  bitsPerAudioSample(int audioFormat);
    static bool                 isFloatAudioSampleFormat(int audioFormat);
    static bool                 isSignedAudioSampleFormat(int audioFormat);
    static bool                 isUnsignedAudioSampleFormat(int audioFormat);
    static QByteArray           toByteArray(AVPacket* packet);

    // Actions
    FFmpeg_Glue::outputdata*    AddOutput(size_t inputPos, int Scale_Width=0, int Scale_Height=0, outputmethod OutputMethod=Output_None, int FilterType=0, const string &Filter=string());
    void                        AddOutput(int Scale_Width=0, int Scale_Height=0, outputmethod OutputMethod=Output_None, int FilterType=0, const string &Filter=string());
    void                        AddOutput(const string &FileName, const string &Format);
    void                        CloseOutput();
    void                        ModifyOutput(size_t InputPos, size_t OutputPos, int Scale_Width=0, int Scale_Height=0, outputmethod OutputMethod=Output_None, int FilterType=0, const string &Filter=string());
    void                        Seek(size_t Pos);
    void                        FrameAtPosition(size_t Pos);
    bool                        NextFrame();
    bool                        OutputFrame(AVPacket* Packet, bool Decode=true);
    bool                        OutputFrame(unsigned char* Data, size_t Size, int stream_index, int FramePos);
    void                        Disable(const size_t Pos);
    double                      TimeStampOfCurrentFrame(size_t OutputPos);
    void                        Scale_Change(int Scale_Width, int Scale_Height, int index = -1 /* scale both left & right by default */);
    void                        Thumbnails_Modulo_Change(size_t Modulo);

    size_t                      TotalFramesCountPerAllStreams() const;
    size_t                      TotalFramesProcessedPerAllStreams() const;

    size_t                      FramesCountPerStream(size_t index) const;
    size_t                      FramesProcessedPerStream(size_t index) const;

    std::vector<size_t>         FramesCountForAllStreams() const;
    std::vector<size_t>         FramesProcessedForAllStreams() const;

    AVFramePtr                  DecodedFrame(size_t index) const;
    AVFramePtr                  FilteredFrame(size_t index) const;
    AVFramePtr                  ScaledFrame(size_t index) const;

    std::map<std::string, std::string> getInputMetadata(int pos) const;

    // Between different FFmpeg_Glue instances

    void*                       InputData_Get() { return InputDatas[0]; }
    void                        InputData_Set(void* InputData) {InputDatas.push_back((inputdata*)InputData); InputDatas_Copy=true;}

    void setThreadSafe(bool enable);

    static double               GetDAR(const FFmpeg_Glue::AVFramePtr & frame);

private:
    QMutex* mutex { nullptr };

    std::vector<inputdata*>     InputDatas;
    std::vector<outputdata*>    OutputDatas;
    bool                        InputDatas_Copy;

    // FFmpeg pointers - Input
    AVFormatContext*            FormatContext { nullptr };
    AVPacket*                   Packet { nullptr };
    AVFrame*                    Frame { nullptr };

    // In
    string                      FileName;
    bool                        WithStats;

    // Seek
    int64_t                     Seek_TimeStamp;

	friend int DecodeVideo(FFmpeg_Glue::inputdata* InputData, AVFrame* Frame, int & got_frame, AVPacket* TempPacket);

};

#endif // FFmpeg_Glue_H
