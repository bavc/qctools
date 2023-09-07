/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include "Core/CommonStats.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
extern "C"
{
#include <libavutil/frame.h>
#include <libavformat/avformat.h>
}
#include <qavplayer.h>
#include <qavcodec_p.h>

#include "Core/Core.h"
#include "tinyxml2.h"
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <cfloat>
using namespace tinyxml2;
//---------------------------------------------------------------------------

//***************************************************************************
// Constructor / Destructor
//***************************************************************************

//---------------------------------------------------------------------------
CommonStats::CommonStats (const struct per_item* PerItem_, int Type_, size_t CountOfGroups_, size_t CountOfItems_, size_t FrameCount, double Duration, QAVStream* stream)
    :
    Frequency(stream ? (((double)stream->stream()->time_base.den) / stream->stream()->time_base.num) : 0),
    streamIndex(stream ? stream->stream()->index : -1),
    Type(Type_),
    PerItem(PerItem_),
    CountOfGroups(CountOfGroups_),
    CountOfItems(CountOfItems_),
    additionalIntStats(nullptr),
    additionalDoubleStats(nullptr),
    additionalStringStats(nullptr)
{

    int sizeOfLastStatsIndex = sizeof lastStatsIndexByValueType;
    memset(lastStatsIndexByValueType, 0, sizeOfLastStatsIndex);

    // Status
    IsComplete=false;
    FirstTimeStamp=DBL_MAX;

    // Memory management
    if (FrameCount<10*3600*30)
    {
        Data_Reserved=1;
        while (Data_Reserved<FrameCount)
            Data_Reserved<<=1;
        if (Data_Reserved+128>=FrameCount)
            Data_Reserved<<=1;
    }
    else
        Data_Reserved=1<<16; //Frame count is not reliable (too huge, e.g. it is sample count instead of frame count), reserving only a default count of frames.

    // Data - Counts
    Stats_Totals = new double[CountOfItems];
    memset(Stats_Totals, 0x00, CountOfItems*sizeof(double));
    Stats_Counts = new uint64_t[CountOfItems];
    memset(Stats_Counts, 0x00, CountOfItems*sizeof(uint64_t));
    Stats_Counts2 = new uint64_t[CountOfItems];
    memset(Stats_Counts2, 0x00, CountOfItems*sizeof(uint64_t));

    // Data - x and y
    x = new double*[4];
    for (size_t j=0; j<4; ++j)
    {
        x[j]=new double[Data_Reserved];
        memset(x[j], 0x00, Data_Reserved*sizeof(double));
    }
    y = new double*[CountOfItems];
    for (size_t j=0; j<CountOfItems; ++j)
    {
        y[j]=new double[Data_Reserved];
        memset(y[j], 0x00, Data_Reserved*sizeof(double));
    }

    // Data - Extra
    durations=new double[Data_Reserved];
    memset(durations, 0x00, Data_Reserved*sizeof(double));

    key_frames=new bool[Data_Reserved];
    memset(key_frames, 0x00, Data_Reserved*sizeof(bool));

    pkt_pos = new int64_t[Data_Reserved];
    memset(pkt_pos, 0x00, Data_Reserved * sizeof(int64_t));

    pkt_pts = new int64_t[Data_Reserved];
    memset(pkt_pts, 0x00, Data_Reserved * sizeof(int64_t));

    pkt_size = new int[Data_Reserved];
    memset(pkt_size, 0x00, Data_Reserved * sizeof(int));

    pix_fmt = new int[Data_Reserved];
    memset(pix_fmt, 0x00, Data_Reserved * sizeof(int));

    pict_type_char = new char[Data_Reserved];
    memset(pict_type_char, 0x00, Data_Reserved * sizeof(char));

    comments = new char*[Data_Reserved];
    memset(comments, 0x00, Data_Reserved * sizeof(char*));

    // Data - Maximums
    x_Current=0;
    x_Current_Max=FrameCount;
    x_Max[0]=x_Current_Max;
    x_Max[1]=Duration;
    x_Max[2]=x_Max[1]/60;
    x_Max[3]=x_Max[2]/60;
    y_Min=new double[CountOfGroups];
    memset(y_Min, 0x00, CountOfGroups*sizeof(double));
    y_Max=new double[CountOfGroups];
    memset(y_Max, 0x00, CountOfGroups*sizeof(double));
}

//---------------------------------------------------------------------------
CommonStats::~CommonStats()
{
    // Data - Counts
    delete[] Stats_Totals;
    delete[] Stats_Counts;
    delete[] Stats_Counts2;

    // Data - x and y
    for (size_t j=0; j<4; ++j)
        delete[] x[j];
    delete[] x;

    // Data - Extra
    for (size_t j=0; j<CountOfItems; ++j)
        delete[] y[j];
    delete[] y;

    // Data - Maximums
    delete[] durations;
    delete[] key_frames;
    delete[] y_Min;
    delete[] y_Max;

    delete[] pkt_pos;
    delete[] pkt_pts;
    delete[] pkt_size;
    delete[] pix_fmt;
    delete[] pict_type_char;

    for (size_t j = 0; j < Data_Reserved; ++j)
        delete [] comments[j];

    delete [] comments;

    auto numberOfIntValues = lastStatsIndexByValueType[StatsValueInfo::Int];

    for (size_t j = 0; j < numberOfIntValues; ++j) {
        delete[] additionalIntStats[j];
    }
    delete[] additionalIntStats;

    auto numberOfDoubleValues = lastStatsIndexByValueType[StatsValueInfo::Double];

    for (size_t j = 0; j < numberOfDoubleValues; ++j) {
        delete[] additionalDoubleStats[j];
    }
    delete[] additionalDoubleStats;

    auto numberOfStringValues = lastStatsIndexByValueType[StatsValueInfo::String];

    for (size_t j = 0; j < numberOfStringValues; ++j) {
        for(size_t i = 0; i < Data_Reserved; ++i) {
            free(additionalStringStats[j][i]);
        }

        delete[] additionalStringStats[j];
    }
    delete[] additionalStringStats;
}

void CommonStats::processAdditionalStats(const char* key, const char* value, bool statsMapInitialized)
{
    if (strcmp(key, "qctools.comment") == 0)
        return;

    if(!statsMapInitialized) {
        auto type = StatsValueInfo::typeFromKey(key, value);
        auto stats = StatsValueInfo {
            lastStatsIndexByValueType[type]++, type, value
        };
        statsValueInfoByKeys[key] = stats;
        statsKeysByIndexByValueType[type][stats.index] = key;
    } else if(statsValueInfoByKeys.find(key)==statsValueInfoByKeys.end()) {
        auto type = StatsValueInfo::typeFromKey(key, value);
        auto oldSize = lastStatsIndexByValueType[type];

        auto stats = StatsValueInfo {
            lastStatsIndexByValueType[type]++, type, value
        };
        statsValueInfoByKeys[key] = stats;
        statsKeysByIndexByValueType[type][stats.index] = key;

        auto size = lastStatsIndexByValueType[type];
        updateAdditionalStats(type, oldSize, size);
    } else {
        auto stats = statsValueInfoByKeys[key];
        if(stats.type == StatsValueInfo::Int) {
            additionalIntStats[stats.index][x_Current] = std::stoi(value);
        } else if(stats.type == StatsValueInfo::Double) {
            additionalDoubleStats[stats.index][x_Current] = std::stod(value);
        } else {
            additionalStringStats[stats.index][x_Current] = strdup(value);
        }
        statsKeysByIndexByValueType[stats.type][stats.index] = key;
    }
}

void CommonStats::writeAdditionalStats(std::stringstream &stream, size_t index)
{
    if(additionalIntStats) {
        for(size_t i = 0; i < statsKeysByIndexByValueType[StatsValueInfo::Int].size(); ++i) {
            auto key = statsKeysByIndexByValueType[StatsValueInfo::Int][i];
            auto value = additionalIntStats[i][index];

            stream<<"            <tag key=\"" << key << "\" value=\"" << value << "\"/>\n";
        }
    }

    if(additionalDoubleStats) {
        for(size_t i = 0; i < statsKeysByIndexByValueType[StatsValueInfo::Double].size(); ++i) {
            auto key = statsKeysByIndexByValueType[StatsValueInfo::Double][i];
            auto value = additionalDoubleStats[i][index];

            stream<<"            <tag key=\"" << key << "\" value=\"" << std::to_string(value) << "\"/>\n";
        }
    }

    if(additionalStringStats) {
        for(size_t i = 0; i < statsKeysByIndexByValueType[StatsValueInfo::String].size(); ++i) {
            auto key = statsKeysByIndexByValueType[StatsValueInfo::String][i];
            auto value = additionalStringStats[i][index];

            stream<<"            <tag key=\"" << key << "\" value=\"" << (value != nullptr ? value : "N/A") << "\"/>\n";
        }
    }
}

void CommonStats::updateAdditionalStats(StatsValueInfo::Type type, size_t oldSize, size_t size)
{
    if (type==StatsValueInfo::Int)
    {
        auto additionalIntStats_Old = additionalIntStats;
        additionalIntStats = new int*[size];
        for (size_t j = 0; j < size; ++j)
        {
            additionalIntStats[j] = new int[Data_Reserved];
            memset(&additionalIntStats[j][0], 0x00, Data_Reserved * sizeof(int));
            if (size < oldSize)
            {
                memcpy(additionalIntStats[j], additionalIntStats_Old[j], Data_Reserved * sizeof(int));
                delete[] additionalIntStats_Old[j];
            }
        }
        delete[] additionalIntStats_Old;
    }
    else if (type==StatsValueInfo::Double)
    {
        auto additionalDoubleStats_Old = additionalDoubleStats;
        additionalDoubleStats = new double*[size];
        for (size_t j = 0; j < size; ++j)
        {
            additionalDoubleStats[j] = new double[Data_Reserved];
            memset(&additionalDoubleStats[j][0], 0x00, Data_Reserved * sizeof(double));
            if (size < oldSize)
            {
                memcpy(additionalDoubleStats[j], additionalDoubleStats_Old[j], Data_Reserved * sizeof(double));
                delete[] additionalDoubleStats_Old[j];
            }
        }
        delete[] additionalDoubleStats_Old;
    }
    else if (type==StatsValueInfo::String)
    {
        auto additionalStringStats_Old = additionalStringStats;
        additionalStringStats = new char**[size];
        for (size_t j = 0; j < size; ++j)
        {
            additionalStringStats[j] = new char*[Data_Reserved];
            memset(&additionalStringStats[j][0], 0x00, Data_Reserved * sizeof(char*));
            if (size < oldSize)
            {
                memcpy(additionalStringStats[j], additionalStringStats_Old[j], Data_Reserved * sizeof(char*));
                delete[] additionalStringStats_Old[j];
            }
        }
        delete[] additionalStringStats_Old;
    }
}

void CommonStats::initializeAdditionalStats()
{
    auto numberOfIntValues = lastStatsIndexByValueType[StatsValueInfo::Int];
    if(numberOfIntValues != 0) {
        additionalIntStats = new int*[numberOfIntValues];
        for(size_t i = 0; i < numberOfIntValues; ++i) {
            additionalIntStats[i] = new int[Data_Reserved];
        }
    }
    auto numberOfDoubleValues = lastStatsIndexByValueType[StatsValueInfo::Double];
    if(numberOfDoubleValues != 0) {
        additionalDoubleStats = new double*[numberOfDoubleValues];
        for(size_t i = 0; i < numberOfDoubleValues; ++i) {
            additionalDoubleStats[i] = new double[Data_Reserved];
        }
    }
    auto numberOfStringValues = lastStatsIndexByValueType[StatsValueInfo::String];
    if(numberOfStringValues != 0) {
        additionalStringStats = new char**[numberOfStringValues];
        for(size_t i = 0; i < numberOfStringValues; ++i) {
            additionalStringStats[i] = new char*[Data_Reserved];
            memset(additionalStringStats[i], 0, sizeof(char*) * Data_Reserved);
        }
    }

    for(auto entry : statsValueInfoByKeys) {
        auto& stats = entry.second;
        if(stats.type == StatsValueInfo::Int) {
            additionalIntStats[stats.index][x_Current] = std::stoi(stats.initialValue);
        } else if(stats.type == StatsValueInfo::Double) {
            auto doubleValue = std::stod(stats.initialValue);
            additionalDoubleStats[stats.index][x_Current] = doubleValue;
        } else {
            additionalStringStats[stats.index][x_Current] = strdup(stats.initialValue.c_str());
        }
    }
}

//***************************************************************************
// Status
//***************************************************************************

//---------------------------------------------------------------------------
int CommonStats::Type_Get()
{
    return Type;
}

//---------------------------------------------------------------------------
double CommonStats::State_Get()
{
    if (IsComplete || x_Current_Max==0)
        return 1;

    double Value=((double)x_Current)/x_Current_Max;
    if (Value>=1)
        Value=0.99; // It is not yet complete, so not 100%

    return Value;
}

//---------------------------------------------------------------------------
void CommonStats::StatsFinish ()
{
    // Adaptation
    if (x_Current==1)
    {
        x[0][1]=1;
        x[1][1]=durations[0]?durations[0]:1; //forcing to 1 in case duration is not available
        x[2][1]=durations[0]?(durations[0]/60):1; //forcing to 1 in case duration is not available
        x[3][1]=durations[0]?(durations[0]/3600):1; //forcing to 1 in case duration is not available
        for (size_t Plot_Pos=0; Plot_Pos<CountOfItems; Plot_Pos++)
            y[Plot_Pos][1]= y[Plot_Pos][0];
    }

    // Forcing max values to the last real ones, in case max values were estimated
    if (x_Current)
    {
        x_Max[0]=x[0][x_Current-1];
        if (x[1][x_Current-1])
        {
            x_Max[1]=x[1][x_Current-1];
            x_Max[2]=x[2][x_Current-1];
            x_Max[3]=x[3][x_Current-1];
        }
    }

    x_Current_Max=x_Current;
    IsComplete=true;
}

//***************************************************************************
// Stats
//***************************************************************************

//---------------------------------------------------------------------------
std::string CommonStats::Average_Get(size_t Pos)
{
    if (x_Current == 0 || Pos >= CountOfItems) {
        return std::string();
    }

    double Value = Stats_Totals[Pos] / x_Current;
    std::stringstream str;
    str << std::fixed;
    str << std::setprecision(PerItem[Pos].DigitsAfterComma);
    str << Value;
    return str.str();
}

//---------------------------------------------------------------------------
std::string CommonStats::Average_Get(size_t Pos, size_t Pos2)
{
    if (x_Current == 0 || Pos >= CountOfItems) {
        return std::string();
    }

    double Value = (Stats_Totals[Pos] - Stats_Totals[Pos2]) / x_Current;
    std::stringstream str;
    str << std::fixed;
    str << std::setprecision(PerItem[Pos].DigitsAfterComma);
    str << Value;
    return str.str();
}

//---------------------------------------------------------------------------
std::string CommonStats::Count_Get(size_t Pos)
{
    if (x_Current==0)
        return std::string();

    std::stringstream str;
    str<<Stats_Counts[Pos];
    return str.str();
}

//---------------------------------------------------------------------------
std::string CommonStats::Count2_Get(size_t Pos)
{
    if (x_Current==0)
        return std::string();

    std::stringstream str;
    str<<Stats_Counts2[Pos];
    return str.str();
}

//---------------------------------------------------------------------------
std::string CommonStats::Percent_Get(size_t Pos)
{
    if (x_Current==0)
        return std::string();

    double Value=((double)Stats_Counts[Pos])/x_Current;
    std::stringstream str;
    str<<Value*100<<"%";
    return str.str();
}

void CommonStats::statsFromExternalData(const char *Data, size_t Size, const std::function<CommonStats*(int, int)>& statsGetter)
{
    // AudioStats from external data
    // XML input
    XMLDocument Document;
    if (Document.Parse(Data, Size))
       return;

    auto Root=Document.FirstChildElement("ffprobe:ffprobe");
    if (Root)
    {
        auto Frames=Root->FirstChildElement("frames");
        if (Frames)
        {
            auto Frame=Frames->FirstChildElement();
            while (Frame)
            {
                if (!strcmp(Frame->Value(), "frame"))
                {
                    const char* media_type=Frame->Attribute("media_type");
                    if (media_type)
                    {
                        const char* stream_index_value = Frame->Attribute("stream_index");
                        if(stream_index_value)
                        {
                            auto streamIndex = std::stoi(stream_index_value);
                            CommonStats* stats = nullptr;

                            if(!strcmp(media_type, "video"))
                            {
                                stats = statsGetter(Type_Video, streamIndex);
                            }
                            else if(!strcmp(media_type, "audio"))
                            {
                                stats = statsGetter(Type_Audio, streamIndex);
                            }

                            stats->parseFrame(Frame);
                        }
                    }
                }

                Frame=Frame->NextSiblingElement();
            }
        }
    }
}

//***************************************************************************
// Memory management
//***************************************************************************

//---------------------------------------------------------------------------
void CommonStats::Data_Reserve(size_t NewValue)
{
    // Saving old data
    size_t                      Data_Reserved_Old = Data_Reserved;
    double**                    x_Old = x;
    double**                    y_Old = y;
    double*                     durations_Old = durations;
    bool*                       key_frames_Old = key_frames;
    int64_t*                    pkt_pos_Old = pkt_pos;
    int64_t*                    pkt_pts_Old = pkt_pts;
    int*                        pkt_size_Old = pkt_size;
    int*                        pix_fmt_Old = pix_fmt;
    char*                       pict_type_char_Old = pict_type_char;
    char**                      comments_Old = comments;

    auto                        additionalIntStats_Old = additionalIntStats;
    auto                        additionalDoubleStats_Old = additionalDoubleStats;
    auto                        additionalStringStats_Old = additionalStringStats;

    // Computing new value
    while (Data_Reserved < NewValue + (1 << 18)) //We reserve extra space, minimum 2^18 frames added
        Data_Reserved <<= 1;

    size_t diff = Data_Reserved - Data_Reserved_Old;

    // Creating new data - x and y
    x = new double*[4];
    for (size_t j = 0; j < 4; ++j)
    {
        x[j] = new double[Data_Reserved];
        memcpy(x[j], x_Old[j], Data_Reserved_Old * sizeof(double));
        memset(&x[j][Data_Reserved_Old], 0x00, diff * sizeof(double));
        delete[] x_Old[j];
    }

    y = new double*[CountOfItems];
    for (size_t j = 0; j < CountOfItems; ++j)
    {
        y[j] = new double[Data_Reserved];
        memcpy(y[j], y_Old[j], Data_Reserved_Old * sizeof(double));
        memset(&y[j][Data_Reserved_Old], 0x00, diff * sizeof(double));
        delete[] y_Old[j];
    }

    // Creating new data - Extra
    durations = new double[Data_Reserved];
    memcpy(durations, durations_Old, Data_Reserved_Old * sizeof(double));
    memset(&durations[Data_Reserved_Old], 0x00, diff * sizeof(double));

    key_frames = new bool[Data_Reserved];
    memcpy(key_frames, key_frames_Old, Data_Reserved_Old * sizeof(bool));
    memset(&key_frames[Data_Reserved_Old], 0x00, diff * sizeof(bool));

    pkt_pos = new int64_t[Data_Reserved];
    memcpy(pkt_pos, pkt_pos_Old, Data_Reserved_Old * sizeof(int64_t));
    memset(&pkt_pos[Data_Reserved_Old], 0x00, diff * sizeof(int64_t));

    pkt_pts = new int64_t[Data_Reserved];
    memcpy(pkt_pts, pkt_pts_Old, Data_Reserved_Old * sizeof(int64_t));
    memset(&pkt_pts[Data_Reserved_Old], 0x00, diff * sizeof(int64_t));

    pkt_size = new int[Data_Reserved];
    memcpy(pkt_size, pkt_size_Old, Data_Reserved_Old * sizeof(int));
    memset(&pkt_size[Data_Reserved_Old], 0x00, diff * sizeof(int));

    pix_fmt = new int[Data_Reserved];
    memcpy(pix_fmt, pix_fmt_Old, Data_Reserved_Old * sizeof(int));
    memset(&pix_fmt[Data_Reserved_Old], 0x00, diff * sizeof(int));

    pict_type_char = new char[Data_Reserved];
    memcpy(pict_type_char, pict_type_char_Old, Data_Reserved_Old * sizeof(char));
    memset(&pict_type_char[Data_Reserved_Old], 0x00, diff * sizeof(char));

    comments = new char*[Data_Reserved];
    memcpy(comments, comments_Old, Data_Reserved_Old * sizeof(char*));
    memset(&comments[Data_Reserved_Old], 0x00, diff * sizeof(char*));

    auto numberOfIntValues = lastStatsIndexByValueType[StatsValueInfo::Int];
    additionalIntStats = new int*[numberOfIntValues];
    for (size_t j = 0; j < numberOfIntValues; ++j)
    {
        additionalIntStats[j] = new int[Data_Reserved];
        memcpy(additionalIntStats[j], additionalIntStats_Old[j], Data_Reserved_Old * sizeof(int));
        memset(&additionalIntStats[j][Data_Reserved_Old], 0x00, diff * sizeof(int));
        delete[] additionalIntStats_Old[j];
    }

    auto numberOfDoubleValues = lastStatsIndexByValueType[StatsValueInfo::Double];
    additionalDoubleStats = new double*[numberOfDoubleValues];
    for (size_t j = 0; j < numberOfDoubleValues; ++j)
    {
        additionalDoubleStats[j] = new double[Data_Reserved];
        memcpy(additionalDoubleStats[j], additionalDoubleStats_Old[j], Data_Reserved_Old * sizeof(double));
        memset(&additionalDoubleStats[j][Data_Reserved_Old], 0x00, diff * sizeof(double));
        delete[] additionalDoubleStats_Old[j];
    }

    auto numberOfStringValues = lastStatsIndexByValueType[StatsValueInfo::String];
    additionalStringStats = new char**[numberOfStringValues];
    for (size_t j = 0; j < numberOfStringValues; ++j)
    {
        additionalStringStats[j] = new char*[Data_Reserved];
        memcpy(additionalStringStats[j], additionalStringStats_Old[j], Data_Reserved_Old * sizeof(char*));
        memset(&additionalStringStats[j][Data_Reserved_Old], 0x00, diff * sizeof(char*));
        delete[] additionalStringStats_Old[j];
    }

    delete[] x_Old;
    delete[] y_Old;
    delete[] durations_Old;
    delete[] key_frames_Old;
    delete[] pkt_pos_Old;
    delete[] pkt_size_Old;
    delete[] pix_fmt_Old;
    delete[] pict_type_char_Old;
    delete[] comments_Old;
}
