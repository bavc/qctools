/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include "Core/AudioStats.h"
#include "Core/AudioCore.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
extern "C"
{
#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#endif

#include <libavutil/frame.h>
}

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
AudioStats::AudioStats (size_t FrameCount, double Duration, AVStream* stream)
    :
    CommonStats(AudioPerItem, Type_Audio, Group_AudioMax, Item_AudioMax, FrameCount, Duration, stream)
{
}

//---------------------------------------------------------------------------
AudioStats::~AudioStats()
{
}

//***************************************************************************
// External data
//***************************************************************************

//---------------------------------------------------------------------------
void AudioStats::StatsFromExternalData(const char* Data, size_t Size)
{
    // AudioStats from external data
    // XML input
    XMLDocument Document;
    if (Document.Parse(Data, Size))
       return;

    XMLElement* Root=Document.FirstChildElement("ffprobe:ffprobe");
    if (Root)
    {
        XMLElement* Frames=Root->FirstChildElement("frames");
        if (Frames)
        {
            XMLElement* Frame=Frames->FirstChildElement();
            while (Frame)
            {
                bool statsMapInitialized = !statsValueInfoByKeys.empty();

                if (!strcmp(Frame->Value(), "frame"))
                {
                    const char* media_type=Frame->Attribute("media_type");
                    if (media_type && !strcmp(media_type, "audio"))
                    {
                        const char* stream_index_value = Frame->Attribute("stream_index");
                        if(stream_index_value)
                            streamIndex = std::stoi(stream_index_value);

                        if (x_Current >= Data_Reserved)
                            Data_Reserve(x_Current);

                        const char* Attribute;
                            
                        x[0][x_Current]=x_Current;

                        Attribute=Frame->Attribute("pkt_duration_time");
                        if (Attribute)
                            durations[x_Current]=std::atof(Attribute);

                        Attribute=Frame->Attribute("key_frame");
                        if (Attribute)
                            key_frames[x_Current]=std::atof(Attribute)?true:false;

                        Attribute = Frame->Attribute("pkt_pos");
                        if(Attribute)
                            pkt_pos[x_Current] = std::atoll(Attribute);

                        Attribute = Frame->Attribute("pkt_size");
                        if (Attribute)
                            pkt_size[x_Current] = std::atoi(Attribute);

                        Attribute = Frame->Attribute("pkt_pts");
                        if (Attribute)
                            pkt_pts[x_Current] = std::atoi(Attribute);

                        Attribute=Frame->Attribute("pkt_pts_time");
                        if (!Attribute || !strcmp(Attribute, "N/A"))
                            Attribute=Frame->Attribute("pkt_dts_time");
                        if (Attribute && strcmp(Attribute, "N/A"))
                        {
                            x[1][x_Current]=std::atof(Attribute);
                            if (FirstTimeStamp==DBL_MAX)
                                FirstTimeStamp=x[1][x_Current];
                            if (x[1][x_Current]<FirstTimeStamp)
                            {
                                double Difference=FirstTimeStamp-x[1][x_Current];
                                for (size_t Pos=0; Pos<x_Current; Pos++)
                                {
                                    x[1][Pos]-=Difference;
                                    x[2][Pos]=x[1][Pos]/60;
                                    x[3][Pos]=x[2][Pos]/60;
                                }
                            }
                            x[1][x_Current]-=FirstTimeStamp;
                            x[2][x_Current]=x[1][x_Current]/60;
                            x[3][x_Current]=x[2][x_Current]/60;
                        }

                        XMLElement* Tag=Frame->FirstChildElement();
                        while (Tag)
                        {
                            if (!strcmp(Tag->Value(), "tag"))
                            {
                                size_t j=Item_AudioMax;
                                const char* key=Tag->Attribute("key");
                                if (key)
                                    for (size_t Plot_Pos=0; Plot_Pos<Item_AudioMax; Plot_Pos++)
                                        if (!strcmp(key, PerItem[Plot_Pos].FFmpeg_Name))
                                        {
                                            j=Plot_Pos;
                                            break;
                                        }

                                if (j!=Item_AudioMax)
                                {
                                    double value;
                                    Attribute=Tag->Attribute("value");
                                    if (Attribute)
                                        value=std::atof(Attribute);
                                    else
                                        value=0;
                                    y[j][x_Current]=value;

                                    if (PerItem[j].Group1!=Group_AudioMax && y_Max[PerItem[j].Group1]<y[j][x_Current])
                                        y_Max[PerItem[j].Group1]=y[j][x_Current];
                                    if (PerItem[j].Group2!=Group_AudioMax && y_Max[PerItem[j].Group2]<y[j][x_Current])
                                        y_Max[PerItem[j].Group2]=y[j][x_Current];
                                    if (PerItem[j].Group1!=Group_AudioMax && y_Min[PerItem[j].Group1]>y[j][x_Current])
                                        y_Min[PerItem[j].Group1]=y[j][x_Current];
                                    if (PerItem[j].Group2!=Group_AudioMax && y_Min[PerItem[j].Group2]>y[j][x_Current])
                                        y_Min[PerItem[j].Group2]=y[j][x_Current];

                                    //AudioStats
                                    Stats_Totals[j]+=y[j][x_Current];
                                    if (PerItem[j].DefaultLimit!=DBL_MAX)
                                    {
                                        if (y[j][x_Current]>PerItem[j].DefaultLimit)
                                            Stats_Counts[j]++;
                                        if (PerItem[j].DefaultLimit2!=DBL_MAX && y[j][x_Current]>PerItem[j].DefaultLimit2)
                                            Stats_Counts2[j]++;
                                    }
                                } else {
                                    auto value = Tag->Attribute("value");
                                    processAdditionalStats(key, value ? value : "", statsMapInitialized);
                                }
                            }

                            Tag=Tag->NextSiblingElement();
                        }

                        if (x_Max[0]<=x[0][x_Current])
                        {
                            x_Max[0]=x[0][x_Current];
                            x_Max[1]=x[1][x_Current];
                            x_Max[2]=x[2][x_Current];
                            x_Max[3]=x[3][x_Current];
                        }
                        x_Current++;
                        if (x_Current_Max<=x_Current)
                            x_Current_Max=x_Current;
                    }
                }

                if(!statsMapInitialized) {
                    initializeAdditionalStats();
                }

                Frame=Frame->NextSiblingElement();
            }
        }
    }
}

//---------------------------------------------------------------------------
void AudioStats::StatsFromFrame (struct AVFrame* Frame, int, int)
{
    AVDictionary * m=av_frame_get_metadata (Frame);
    AVDictionaryEntry* e=NULL;
    bool statsMapInitialized = !statsValueInfoByKeys.empty();

    for (;;)
    {
        e=av_dict_get     (m, "", e, AV_DICT_IGNORE_SUFFIX);
        if (!e)
            break;
        size_t j=0;
        for (; j<Item_AudioMax; j++)
        {
            if (strcmp(e->key, PerItem[j].FFmpeg_Name)==0)
                break;
        }

        if (j<Item_AudioMax)
        {
            y[j][x_Current]=std::atof(e->value);
                                            
            if (PerItem[j].Group1!=Group_AudioMax && y_Max[PerItem[j].Group1]<y[j][x_Current])
                y_Max[PerItem[j].Group1]=y[j][x_Current];
            if (PerItem[j].Group2!=Group_AudioMax && y_Max[PerItem[j].Group2]<y[j][x_Current])
                y_Max[PerItem[j].Group2]=y[j][x_Current];
            if (PerItem[j].Group1!=Group_AudioMax && y_Min[PerItem[j].Group1]>y[j][x_Current])
                y_Min[PerItem[j].Group1]=y[j][x_Current];
            if (PerItem[j].Group2!=Group_AudioMax && y_Min[PerItem[j].Group2]>y[j][x_Current])
                y_Min[PerItem[j].Group2]=y[j][x_Current];

            //Stats
            Stats_Totals[j]+=y[j][x_Current];
            if (PerItem[j].DefaultLimit!=DBL_MAX)
            {
                if (y[j][x_Current]>PerItem[j].DefaultLimit)
                    Stats_Counts[j]++;
                if (PerItem[j].DefaultLimit2!=DBL_MAX && y[j][x_Current]>PerItem[j].DefaultLimit2)
                    Stats_Counts2[j]++;
            }
        } else {

            // not found among plot groups
            auto key = e->key;
            auto value = e->value;

            processAdditionalStats(key, value, statsMapInitialized);
        }
    }

    if(!statsMapInitialized) {
        initializeAdditionalStats();
    }

    key_frames[x_Current]=Frame->key_frame?true:false;

    pkt_pos[x_Current] = Frame->pkt_pos;
    pkt_size[x_Current] = Frame->pkt_size;
    pkt_pts[x_Current] = Frame->pkt_pts;

    if (x_Max[0]<=x[0][x_Current])
    {
        x_Max[0]=x[0][x_Current];
        x_Max[1]=x[1][x_Current];
        x_Max[2]=x[2][x_Current];
        x_Max[3]=x[3][x_Current];
    }
    x_Current++;
    if (x_Current_Max<=x_Current)
        x_Current_Max=x_Current;
}

//---------------------------------------------------------------------------
void AudioStats::TimeStampFromFrame (struct AVFrame* Frame, size_t FramePos)
{
    if (Frequency==0)
        return; // Not supported

    if (FramePos >= Data_Reserved)
        Data_Reserve(FramePos + 1);

    x[0][FramePos]=FramePos;

    int64_t ts=(Frame->pkt_pts==AV_NOPTS_VALUE)?Frame->pkt_dts:Frame->pkt_pts; // Using DTS is PTS is not available
    if (ts==AV_NOPTS_VALUE && FramePos)
        ts=(int64_t)((FirstTimeStamp+x[1][FramePos-1]+durations[FramePos-1])*Frequency); // If time stamp is not present, creating a fake one from last frame duration
    if (ts!=AV_NOPTS_VALUE)
    {
        if (FirstTimeStamp==DBL_MAX)
            FirstTimeStamp=ts/Frequency;
        x[1][FramePos]=((double)ts)/Frequency;
        if (x[1][FramePos]<FirstTimeStamp)
        {
            double Difference=FirstTimeStamp-x[1][x_Current];
            for (size_t Pos=0; Pos<x_Current; Pos++)
            {
                x[1][Pos]-=Difference;
                x[2][Pos]=x[1][Pos]/60;
                x[3][Pos]=x[2][Pos]/60;
            }
        }
        x[1][FramePos]-=FirstTimeStamp;
        x[2][FramePos]=x[1][FramePos]/60;
        x[3][FramePos]=x[2][FramePos]/60;
    }
    if (Frame->pkt_duration != AV_NOPTS_VALUE)
        durations[FramePos]=((double)Frame->pkt_duration)/Frequency;
}

//---------------------------------------------------------------------------
string AudioStats::StatsToXML (const activefilters& filters)
{
    stringstream Data;

    // Per frame (note: the XML header and footer are not created here)
    for (size_t x_Pos=0; x_Pos<x_Current; ++x_Pos)
    {
        stringstream pkt_pts_time; pkt_pts_time<<fixed<<setprecision(7)<<(x[1][x_Pos]+FirstTimeStamp);
        stringstream pkt_duration_time; pkt_duration_time<<fixed<<setprecision(7)<<durations[x_Pos];
        stringstream key_frame; key_frame<<key_frames[x_Pos]?'1':'0';
        Data<<"        <frame media_type=\"audio\"";
        Data << " stream_index=\"" << streamIndex << "\"";

        Data<<" key_frame=\"" << key_frame.str() << "\"";
        Data << " pkt_pts=\"" << pkt_pts[x_Pos] << "\"";
        Data<<" pkt_pts_time=\"" << pkt_pts_time.str() << "\"";
        if (pkt_duration_time)
            Data<<" pkt_duration_time=\"" << pkt_duration_time.str() << "\"";
        Data << " pkt_pos=\"" << pkt_pos[x_Pos] << "\"";
        Data << " pkt_size=\"" << pkt_size[x_Pos] << "\"";

        Data << ">\n";

        for (size_t Plot_Pos=0; Plot_Pos<Item_AudioMax; Plot_Pos++)
        {
            const activefilter filter = PerItem[Plot_Pos].Filter;
            if(filter == activefilter(-1))
                continue;

            if(!filters.test(filter))
                continue;

            const std::string& key = PerItem[Plot_Pos].FFmpeg_Name;

            stringstream value;
            //switch (Plot_Pos)
            //{
            //    default:
            value<<y[Plot_Pos][x_Pos];
            //}

            Data<<"            <tag key=\""+key+"\" value=\""+value.str()+"\"/>\n";
        }

        if(additionalIntStats) {
            for(auto i = 0; i < statsKeysByIndexByValueType[StatsValueInfo::Int].size(); ++i) {
                auto key = statsKeysByIndexByValueType[StatsValueInfo::Int][i];
                auto value = additionalIntStats[i];

                Data<<"            <tag key=\"" << key << "\" value=\"" << value << "\"/>\n";
            }
        }

        if(additionalDoubleStats) {
            for(auto i = 0; i < statsKeysByIndexByValueType[StatsValueInfo::Double].size(); ++i) {
                auto key = statsKeysByIndexByValueType[StatsValueInfo::Double][i];
                auto value = additionalDoubleStats[i];

                Data<<"            <tag key=\"" << key << "\" value=\"" << value << "\"/>\n";
            }
        }

        if(additionalStringStats) {
            for(auto i = 0; i < statsKeysByIndexByValueType[StatsValueInfo::String].size(); ++i) {
                auto key = statsKeysByIndexByValueType[StatsValueInfo::String][i];
                auto value = additionalStringStats[i];

                Data<<"            <tag key=\"" << key << "\" value=\"" << value << "\"/>\n";
            }
        }

        Data<<"        </frame>\n";
    }

    return Data.str();
}
