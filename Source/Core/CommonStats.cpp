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
#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#endif

#include <libavutil/frame.h>
#include <libavformat/avformat.h>
}

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
CommonStats::CommonStats (const struct per_item* PerItem_, int Type_, size_t CountOfGroups_, size_t CountOfItems_, size_t FrameCount, double Duration, AVStream* stream)
    :
    Frequency(stream ? (((double)stream->time_base.den) / stream->time_base.num) : 0),
	streamIndex(stream ? stream->index : -1),
    PerItem(PerItem_),
    Type(Type_),
    CountOfGroups(CountOfGroups_),
    CountOfItems(CountOfItems_)
{
    // Adaptation for having a graph even with 1 frame
    if (FrameCount<2)
        FrameCount=2;

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

    pkt_size = new int[Data_Reserved];
    memset(pkt_size, 0x00, Data_Reserved * sizeof(int));

    pix_fmt = new int[Data_Reserved];
    memset(pix_fmt, 0x00, Data_Reserved * sizeof(int));

    pict_type_char = new char[Data_Reserved];
    memset(pict_type_char, 0x00, Data_Reserved * sizeof(char));

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
    delete[] pkt_size;
    delete[] pix_fmt;
    delete[] pict_type_char;
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
        x_Current++;
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
string CommonStats::Average_Get(size_t Pos)
{
    if (x_Current == 0 || Pos >= CountOfItems) {
        return string();
    }

    double Value = Stats_Totals[Pos] / x_Current;
    stringstream str;
    str << fixed;
    str << setprecision(PerItem[Pos].DigitsAfterComma);
    str << Value;
    return str.str();
}

//---------------------------------------------------------------------------
string CommonStats::Average_Get(size_t Pos, size_t Pos2)
{
    if (x_Current == 0 || Pos >= CountOfItems) {
        return string();
    }

    double Value = (Stats_Totals[Pos] - Stats_Totals[Pos2]) / x_Current;
    stringstream str;
    str << fixed;
    str << setprecision(PerItem[Pos].DigitsAfterComma);
    str << Value;
    return str.str();
}

//---------------------------------------------------------------------------
string CommonStats::Count_Get(size_t Pos)
{
    if (x_Current==0)
        return string();

    stringstream str;
    str<<Stats_Counts[Pos];
    return str.str();
}

//---------------------------------------------------------------------------
string CommonStats::Count2_Get(size_t Pos)
{
    if (x_Current==0)
        return string();

    stringstream str;
    str<<Stats_Counts2[Pos];
    return str.str();
}

//---------------------------------------------------------------------------
string CommonStats::Percent_Get(size_t Pos)
{
    if (x_Current==0)
        return string();

    double Value=((double)Stats_Counts[Pos])/x_Current;
    stringstream str;
    str<<Value*100<<"%";
    return str.str();
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
    int*                        pkt_size_Old = pkt_size;
    int*                        pix_fmt_Old = pix_fmt;
    char*                       pict_type_char_Old = pict_type_char;

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

    pkt_size = new int[Data_Reserved];
    memcpy(pkt_size, pkt_size_Old, Data_Reserved_Old * sizeof(int));
    memset(&pkt_size[Data_Reserved_Old], 0x00, diff * sizeof(int));

    pix_fmt = new int[Data_Reserved];
    memcpy(pix_fmt, pix_fmt_Old, Data_Reserved_Old * sizeof(int));
    memset(&pix_fmt[Data_Reserved_Old], 0x00, diff * sizeof(int));

    pict_type_char = new char[Data_Reserved];
    memcpy(pict_type_char, pict_type_char_Old, Data_Reserved_Old * sizeof(char));
    memset(&pict_type_char[Data_Reserved_Old], 0x00, diff * sizeof(char));

    delete[] x_Old;
    delete[] y_Old;
    delete[] durations_Old;
    delete[] key_frames_Old;
    delete[] pkt_pos_Old;
    delete[] pkt_size_Old;
    delete[] pix_fmt_Old;
    delete[] pict_type_char_Old;
}
