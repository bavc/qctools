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
#include <libavutil/frame.h>
#include <libavformat/avformat.h>
}
#include <qavplayer.h>

#include "tinyxml2.h"
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <cfloat>

#include <tinyxml2.h>
using namespace tinyxml2;
//---------------------------------------------------------------------------

//***************************************************************************
// Constructor / Destructor
//***************************************************************************

//---------------------------------------------------------------------------
AudioStats::AudioStats (size_t FrameCount, double Duration, QAVStream* stream)
    :
    CommonStats(AudioPerItem, Type_Audio, Group_AudioMax, Item_AudioMax, FrameCount, Duration, stream)
{
}

AudioStats::AudioStats(int streamIndex) : AudioStats(0, 0, nullptr)
{
    this->streamIndex = streamIndex;
}

//---------------------------------------------------------------------------
AudioStats::~AudioStats()
{
}

void AudioStats::parseFrame(tinyxml2::XMLElement *Frame)
{
    bool statsMapInitialized = !statsValueInfoByKeys.empty();

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

    auto Tag=Frame->FirstChildElement();
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

                if (!std::isinf(value)) {
                    if (PerItem[j].Group1 != Group_AudioMax && y_Max[PerItem[j].Group1] < y[j][x_Current])
                        y_Max[PerItem[j].Group1] = y[j][x_Current];
                    if (PerItem[j].Group2 != Group_AudioMax && y_Max[PerItem[j].Group2] < y[j][x_Current])
                        y_Max[PerItem[j].Group2] = y[j][x_Current];
                    if (PerItem[j].Group1 != Group_AudioMax && y_Min[PerItem[j].Group1] > y[j][x_Current])
                        y_Min[PerItem[j].Group1] = y[j][x_Current];
                    if (PerItem[j].Group2 != Group_AudioMax && y_Min[PerItem[j].Group2] > y[j][x_Current])
                        y_Min[PerItem[j].Group2] = y[j][x_Current];
                }

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

    if(!statsMapInitialized) {
        initializeAdditionalStats();
        statsMapInitialized=true;
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

//***************************************************************************
// External data
//***************************************************************************

//---------------------------------------------------------------------------
void AudioStats::StatsFromFrame (const QAVFrame& frame, int, int)
{
    auto Frame = frame.frame();
    AVDictionary * m= Frame->metadata;
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
            double value = std::atof(e->value);
            y[j][x_Current]=value;

            if (!std::isinf(value)) {
                if (PerItem[j].Group1 != Group_AudioMax && y_Max[PerItem[j].Group1] < y[j][x_Current])
                    y_Max[PerItem[j].Group1] = y[j][x_Current];
                if (PerItem[j].Group2 != Group_AudioMax && y_Max[PerItem[j].Group2] < y[j][x_Current])
                    y_Max[PerItem[j].Group2] = y[j][x_Current];
                if (PerItem[j].Group1 != Group_AudioMax && y_Min[PerItem[j].Group1] > y[j][x_Current])
                    y_Min[PerItem[j].Group1] = y[j][x_Current];
                if (PerItem[j].Group2 != Group_AudioMax && y_Min[PerItem[j].Group2] > y[j][x_Current])
                    y_Min[PerItem[j].Group2] = y[j][x_Current];
            }

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
    pkt_pts[x_Current] = Frame->pts;

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
void AudioStats::TimeStampFromFrame (const QAVFrame& frame, size_t FramePos)
{
    auto Frame = frame.frame();

    if (Frequency==0)
        return; // Not supported

    if (FramePos >= Data_Reserved)
        Data_Reserve(FramePos + 1);

    x[0][FramePos]=FramePos;

    /* can't use old way, take pts directly from frame
    int64_t ts=(Frame->pts == AV_NOPTS_VALUE)?Frame->pkt_dts : Frame->pts; // Using DTS is PTS is not available
    if (ts==AV_NOPTS_VALUE && FramePos)
        ts=(int64_t)((FirstTimeStamp+x[1][FramePos-1]+durations[FramePos-1])*Frequency); // If time stamp is not present, creating a fake one from last frame duration
    */

    auto time_base = frame.stream().stream()->time_base;
    int64_t ts = frame.pts() * time_base.den / time_base.num;

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

std::string AudioStats::StatsToXML (const activefilters& filters)
{
    std::string Data;

    // Per frame (note: the XML header and footer are not created here)
    for (size_t x_Pos=0; x_Pos<x_Current; ++x_Pos)
    {
        std::stringstream Frame;
        std::stringstream pkt_pts_time; pkt_pts_time<<std::fixed<<std::setprecision(7)<<(x[1][x_Pos]+FirstTimeStamp);
        std::stringstream pkt_duration_time; pkt_duration_time<<std::fixed<<std::setprecision(7)<<durations[x_Pos];
        std::stringstream key_frame; key_frame<< (key_frames[x_Pos]? '1' : '0');

        Frame<<"        <frame media_type=\"audio\"";
        Frame << " stream_index=\"" << streamIndex << "\"";

        Frame<<" key_frame=\"" << key_frame.str() << "\"";
        Frame << " pkt_pts=\"" << pkt_pts[x_Pos] << "\"";
        Frame<<" pkt_pts_time=\"" << pkt_pts_time.str() << "\"";
        if (pkt_duration_time)
            Frame<<" pkt_duration_time=\"" << pkt_duration_time.str() << "\"";
        Frame << " pkt_pos=\"" << pkt_pos[x_Pos] << "\"";
        Frame << " pkt_size=\"" << pkt_size[x_Pos] << "\"";

        Frame << ">\n";

        for (size_t Plot_Pos=0; Plot_Pos<Item_AudioMax; Plot_Pos++)
        {
            const activefilter filter = PerItem[Plot_Pos].Filter;
            if(filter == activefilter(-1))
                continue;

            if(!filters.test(filter))
                continue;

            const std::string& key = PerItem[Plot_Pos].FFmpeg_Name;
            auto value = std::to_string(y[Plot_Pos][x_Pos]);

            Frame<<"            <tag key=\""+key+"\" value=\""+value+"\"/>\n";
        }

        writeAdditionalStats(Frame, x_Pos);

        Frame<<"        </frame>\n";
        Data+=Frame.str();
    }

    return Data;
}
