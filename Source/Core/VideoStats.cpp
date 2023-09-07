/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include "Core/VideoStats.h"
#include "Core/VideoCore.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
extern "C"
{
#include <libavutil/frame.h>
#include <libavutil/pixdesc.h>
}
#include <qavplayer.h>

#include "tinyxml2.h"
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <cfloat>
#include <QString>
using namespace tinyxml2;
//---------------------------------------------------------------------------

//***************************************************************************
// Constructor / Destructor
//***************************************************************************

//---------------------------------------------------------------------------
VideoStats::VideoStats (size_t FrameCount, double Duration, QAVStream* stream)
    :
    CommonStats(VideoPerItem, Type_Video, Group_VideoMax, Item_VideoMax, FrameCount, Duration, stream)
{
}

VideoStats::VideoStats(int streamIndex) : VideoStats(0, 0, nullptr)
{
    this->streamIndex = streamIndex;
}

//---------------------------------------------------------------------------
VideoStats::~VideoStats()
{
}

void VideoStats::parseFrame(tinyxml2::XMLElement *Frame)
{
    bool statsMapInitialized = !statsValueInfoByKeys.empty();

    if (x_Current>=Data_Reserved)
        Data_Reserve(x_Current);

    const char* Attribute;

    x[0][x_Current]=x_Current;

    Attribute=Frame->Attribute("pkt_duration_time");
    if (Attribute)
    {
        durations[x_Current]=std::atof(Attribute);
        y[Item_pkt_duration_time][x_Current] = durations[x_Current];

        {
            double& group1Max = y_Max[PerItem[Item_pkt_duration_time].Group1];
            double& group1Min = y_Min[PerItem[Item_pkt_duration_time].Group1];
            double& current = durations[x_Current];

            if(group1Max < current)
                group1Max = current;
            if(group1Min > current)
                group1Min = current;
        }
    }

    Attribute=Frame->Attribute("key_frame");
    if (Attribute)
        key_frames[x_Current]=std::atof(Attribute)?true:false;

    Attribute = Frame->Attribute("pkt_pos");
    if(Attribute)
        pkt_pos[x_Current] = std::atoll(Attribute);

    Attribute = Frame->Attribute("pkt_size");
    if (Attribute)
    {
        pkt_size[x_Current] = std::atoi(Attribute);
        y[Item_pkt_size][x_Current] = pkt_size[x_Current];

        {
            double& group1Max = y_Max[PerItem[Item_pkt_size].Group1];
            double& group1Min = y_Min[PerItem[Item_pkt_size].Group1];
            int& current = pkt_size[x_Current];

            if(group1Max < current)
                group1Max = current;
            if(group1Min > current)
                group1Min = current;
        }
    }

    Attribute = Frame->Attribute("pkt_pts");
    if (Attribute)
        pkt_pts[x_Current] = std::atoi(Attribute);

    Attribute = Frame->Attribute("pix_fmt");
    if (Attribute)
        pix_fmt[x_Current] = av_get_pix_fmt(Attribute);

    Attribute = Frame->Attribute("pict_type");
    if (Attribute)
        pict_type_char[x_Current] = *Attribute;

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

    int Width;
    Attribute=Frame->Attribute("width");
    if (Attribute)
        Width=std::atoi(Attribute);
    else
        Width=0;

    setWidth(Width);

    int Height;
    Attribute=Frame->Attribute("height");
    if (Attribute)
        Height=std::atoi(Attribute);
    else
        Height=0;

    setHeight(Height);

    auto Tag=Frame->FirstChildElement();
    while (Tag)
    {
        if (!strcmp(Tag->Value(), "tag"))
        {
            size_t j=Item_VideoMax;
            const char* key=Tag->Attribute("key");
            if (key)
            {
                if(strcmp(key, "qctools.comment") == 0)
                {
                    const char* value = Tag->Attribute("value");
                    if(value)
                    {
                        comments[x_Current] = strdup(QString::fromUtf8(value).toHtmlEscaped().toUtf8().data());
                    }
                }
                else
                {
                    for (size_t Plot_Pos=0; Plot_Pos<Item_VideoMax; Plot_Pos++)
                        if (!strcmp(key, PerItem[Plot_Pos].FFmpeg_Name))
                        {
                            j=Plot_Pos;
                            break;
                        }
                }
            }

            if (j!=Item_VideoMax)
            {
                double value;
                Attribute=Tag->Attribute("value");
                if (Attribute)
                    value=std::atof(Attribute);
                else
                    value=0;

                // Special cases: crop: x2, y2
                if (Width && !strcmp(key, "lavfi.cropdetect.x2"))
                    y[j][x_Current]=Width-value;
                else if (Height && !strcmp(key, "lavfi.cropdetect.y2"))
                    y[j][x_Current]=Height-value;
                else if (Width && !strcmp(key, "lavfi.cropdetect.w"))
                    y[j][x_Current]=Width-value;
                else if (Height && !strcmp(key, "lavfi.cropdetect.h"))
                    y[j][x_Current]=Height-value;
                else
                    y[j][x_Current]=value;

                if (!std::isinf(value)) {
                    if (PerItem[j].Group1 != Group_VideoMax && y_Max[PerItem[j].Group1] < y[j][x_Current])
                        y_Max[PerItem[j].Group1] = y[j][x_Current];
                    if (PerItem[j].Group2 != Group_VideoMax && y_Max[PerItem[j].Group2] < y[j][x_Current])
                        y_Max[PerItem[j].Group2] = y[j][x_Current];
                    if (PerItem[j].Group1 != Group_VideoMax && y_Min[PerItem[j].Group1] > y[j][x_Current])
                        y_Min[PerItem[j].Group1] = y[j][x_Current];
                    if (PerItem[j].Group2 != Group_VideoMax && y_Min[PerItem[j].Group2] > y[j][x_Current])
                        y_Min[PerItem[j].Group2] = y[j][x_Current];
                }

                //VideoStats
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

void VideoStats::StatsFromFrame (const QAVFrame& frame, int Width, int Height)
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
        for (; j<Item_VideoMax; j++)
        {
            if (strcmp(e->key, PerItem[j].FFmpeg_Name)==0)
                break;
        }

        if (j<Item_VideoMax)
        {
            double value=std::atof(e->value);

            // Special cases: crop: x2, y2
            if (std::string(e->key)=="lavfi.cropdetect.x2")
                y[j][x_Current]=Width-value;
            else if (std::string(e->key)=="lavfi.cropdetect.y2")
                y[j][x_Current]=Height-value;
            else if (std::string(e->key)=="lavfi.cropdetect.w")
                y[j][x_Current]=Width-value;
            else if (std::string(e->key)=="lavfi.cropdetect.h")
                y[j][x_Current]=Height-value;
            else
                y[j][x_Current]=value;

            if (!std::isinf(value)) {
                if (PerItem[j].Group1 != Group_VideoMax && y_Max[PerItem[j].Group1] < y[j][x_Current])
                    y_Max[PerItem[j].Group1] = y[j][x_Current];
                if (PerItem[j].Group2 != Group_VideoMax && y_Max[PerItem[j].Group2] < y[j][x_Current])
                    y_Max[PerItem[j].Group2] = y[j][x_Current];
                if (PerItem[j].Group1 != Group_VideoMax && y_Min[PerItem[j].Group1] > y[j][x_Current])
                    y_Min[PerItem[j].Group1] = y[j][x_Current];
                if (PerItem[j].Group2 != Group_VideoMax && y_Min[PerItem[j].Group2] > y[j][x_Current])
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

    y[Item_pkt_duration_time][x_Current] = durations[x_Current];

    {
        double& group1Max = y_Max[PerItem[Item_pkt_duration_time].Group1];
        double& group1Min = y_Min[PerItem[Item_pkt_duration_time].Group1];
        double& current = durations[x_Current];

        if(group1Max < current)
            group1Max = current;
        if(group1Min > current)
            group1Min = current;
    }

    key_frames[x_Current]=Frame->key_frame?true:false;

    pkt_pos[x_Current] = Frame->pkt_pos;
    pkt_size[x_Current] = Frame->pkt_size;
    pkt_pts[x_Current] = Frame->pts;

    y[Item_pkt_size][x_Current] = pkt_size[x_Current];

    {
        double& group1Max = y_Max[PerItem[Item_pkt_size].Group1];
        double& group1Min = y_Min[PerItem[Item_pkt_size].Group1];
        int& current = pkt_size[x_Current];

        if(group1Max < current)
            group1Max = current;
        if(group1Min > current)
            group1Min = current;
    }
    pix_fmt[x_Current] = Frame->format;
    pict_type_char[x_Current] = av_get_picture_type_char(Frame->pict_type);

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
void VideoStats::TimeStampFromFrame (const QAVFrame& frame, size_t FramePos)
{
    auto Frame = frame.frame();

    if (Frequency==0)
        return; // Not supported

    if (FramePos>=x_Current_Max)
    {
        x_Current_Max=FramePos+1;
        if (x_Current_Max>Data_Reserved)
            Data_Reserve(x_Current_Max);
    }

    x[0][FramePos]=FramePos;

    int64_t ts=(Frame->pts == AV_NOPTS_VALUE) ? Frame->pkt_dts : Frame->pts; // Using DTS is PTS is not available
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
    if (Frame->pkt_duration!=AV_NOPTS_VALUE)
        durations[FramePos]=((double)Frame->pkt_duration)/Frequency;
}

//---------------------------------------------------------------------------
int VideoStats::getWidth() const
{
    return width;
}

void VideoStats::setWidth(int width)
{
    this->width = width;
}

int VideoStats::getHeight() const
{
    return height;
}

void VideoStats::setHeight(int height)
{
    this->height = height;
}

//---------------------------------------------------------------------------
std::string VideoStats::StatsToXML (const activefilters& filters)
{
    std::stringstream Data;

    // Per frame (note: the XML header and footer are not created here)
    std::stringstream widthStream; widthStream<<width; // Note: we use the same value for all frame, we should later use the right value per frame
    std::stringstream heightStream; heightStream<<height; // Note: we use the same value for all frame, we should later use the right value per frame
    for (size_t x_Pos=0; x_Pos<x_Current; ++x_Pos)
    {
        std::stringstream pkt_pts_time; pkt_pts_time<<std::fixed<<std::setprecision(7)<<(x[1][x_Pos]+FirstTimeStamp);
        std::stringstream pkt_duration_time; pkt_duration_time<<std::fixed<<std::setprecision(7)<<durations[x_Pos];
        std::stringstream key_frame; key_frame<<key_frames[x_Pos]?'1':'0';
        Data<<"        <frame media_type=\"video\"";
        Data << " stream_index=\"" << streamIndex << "\"";

        Data<<" key_frame=\"" << key_frame.str() << "\"";
        Data << " pkt_pts=\"" << pkt_pts[x_Pos] << "\"";
        Data<<" pkt_pts_time=\"" << pkt_pts_time.str() << "\"";
        if (pkt_duration_time)
            Data<<" pkt_duration_time=\"" << pkt_duration_time.str() << "\"";
        Data << " pkt_pos=\"" << pkt_pos[x_Pos] << "\"";
        Data << " pkt_size=\"" << pkt_size[x_Pos] << "\"";
        Data<<" width=\"" << widthStream.str() << "\" height=\"" << heightStream.str() <<"\"";
        Data << " pix_fmt=\"" << av_get_pix_fmt_name((AVPixelFormat) pix_fmt[x_Pos]) << "\"";
        Data << " pict_type=\"" << pict_type_char[x_Pos] << "\"";

        Data << ">\n";

        for (size_t Plot_Pos=0; Plot_Pos<Item_VideoMax; Plot_Pos++)
        {
            const activefilter filter = PerItem[Plot_Pos].Filter;
            if(filter == activefilter(-1))
                continue;

            if(!filters.test(filter))
                continue;

            const std::string& key = PerItem[Plot_Pos].FFmpeg_Name;
            std::string value;

            switch (Plot_Pos)
            {
            case Item_Crop_x2 :
            case Item_Crop_w :
                // Special case, values are from width
                value = std::to_string(width-y[Plot_Pos][x_Pos]);
                break;
            case Item_Crop_y2 :
            case Item_Crop_h :
                // Special case, values are from height
                value = std::to_string(height-y[Plot_Pos][x_Pos]);
                break;
            default:
                value = std::to_string(y[Plot_Pos][x_Pos]);
            }

            Data<<"            <tag key=\""+key+"\" value=\""+value+"\"/>\n";
        }

        writeAdditionalStats(Data, x_Pos);

        if(comments[x_Pos])
            Data<<"            <tag key=\"qctools.comment\" value=\"" << comments[x_Pos] << "\"/>\n";

        Data<<"        </frame>\n";
    }

    return Data.str();
}
