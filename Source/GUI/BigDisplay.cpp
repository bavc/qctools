/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include "BigDisplay.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "GUI/BigDisplay.h"
#include "GUI/Control.h"
#include "GUI/Info.h"
#include "GUI/Help.h"
#include "GUI/FileInformation.h"
#include "Core/FFmpeg_Glue.h"

#include <QDesktopWidget>
#include <QGridLayout>
#include <QPixmap>
#include <QComboBox>
#include <QSlider>
#include <QMenu>
#include <QCheckBox>
#include <QLabel>
#include <QToolButton>
#include <QActionGroup>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QPainter>
#include <QSpacerItem>
#include <QPushButton>
#include <QRadioButton>
#include <QShortcut>

#ifdef _WIN32
    #include <string>
    #include <algorithm>
#endif
//---------------------------------------------------------------------------


//***************************************************************************
// Info
//***************************************************************************

//---------------------------------------------------------------------------
enum args_type
{
    Args_Type_None,
    Args_Type_Toggle,
    Args_Type_Slider,
    Args_Type_Radio,
};

struct args
{
    const args_type     Type;
    const int           Default;
    const int           Min;
    const int           Max;
    const double        Divisor;
    const char*         Name;
};

struct filter
{
    const char*         Name;
    const args          Args[4];
    const char*         Value[16]; //Max 2^4 toggles
};


const filter Filters[]=
{
    {
        "Help",
        {
            { Args_Type_None,   0, 0, 0, 0, },
            { Args_Type_None,   0, 0, 0, 0, },
            { Args_Type_None,   0, 0, 0, 0, },
            { Args_Type_None,   0, 0, 0, 0, },
        },
        {
            "",
        },
    },
    {
        "No display",
        {
            { Args_Type_None,   0, 0, 0, 0, },
            { Args_Type_None,   0, 0, 0, 0, },
            { Args_Type_None,   0, 0, 0, 0, },
            { Args_Type_None,   0, 0, 0, 0, },
        },
        {
            "",
        },
    },
    {
        "Normal",
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field Split" },
            { Args_Type_None,     0,   0,   0,   0, },
            { Args_Type_None,     0,   0,   0,   0, },
            { Args_Type_None,     0,   0,   0,   0, },
        },
        {
            "",
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1][b1]framepack=tab",
        },
    },
    {
        "(Separator)",
        {
            { Args_Type_None,   0, 0, 0, 0, },
            { Args_Type_None,   0, 0, 0, 0, },
            { Args_Type_None,   0, 0, 0, 0, },
            { Args_Type_None,   0, 0, 0, 0, },
        },
        {
            "",
        },
    },
    {
        "Field Difference",
        {
            { Args_Type_None,   0, 0, 0, 0, },
            { Args_Type_None,   0, 0, 0, 0, },
            { Args_Type_None,   0, 0, 0, 0, },
            { Args_Type_None,   0, 0, 0, 0, },
        },
        {
            "split[a][b];[a]field=bottom[a1];[b]field=top,negate[b2];[a1][b2]blend=all_mode=average",
        },
    },
    {
        "Histogram",
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field Split" },
            { Args_Type_None,     0,   0,   0,   0, },
            { Args_Type_None,     0,   0,   0,   0, },
            { Args_Type_None,     0,   0,   0,   0, },
        },
        {
            "histogram",
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]histogram[a2];[b1]histogram[b2];[a2][b2]framepack",
        },
    },
    {
        "Waveform",
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field Split" },
            { Args_Type_Slider,  10,   1, 255,   1, "Intensity" },
            { Args_Type_Toggle,   0,   0,   0,   0, "Parade" },
            { Args_Type_Toggle,   0,   0,   0,   0, "Horizontal" },
        },
        {
            "histogram=step=$2:mode=waveform:waveform_mode=column:waveform_mirror=1,crop=iw:256:0:0,drawbox=y=(256-16):w=iw:h=16:color=aqua@0.2:t=16,drawbox=w=iw:h=(256-235):color=crimson@0.2:t=16,scale=iw:512",
            "transpose=1,histogram=step=$2:mode=waveform:waveform_mode=column:waveform_mirror=1,crop=iw:256:0:0,drawbox=y=(256-16):w=iw:h=16:color=aqua@0.2:t=16,drawbox=w=iw:h=(256-235):color=crimson@0.2:t=16,scale=iw:512",
            "histogram=step=$2:mode=waveform:waveform_mode=column:waveform_mirror=1,drawbox=y=(256-16):w=iw:h=16:color=aqua@0.2:t=16,drawbox=w=iw:h=(256-235):color=crimson@0.2:t=16,drawgrid=width=0:height=256:thickness=1:color=white@0.5",
            "transpose=1,histogram=step=$2:mode=waveform:waveform_mode=column:waveform_mirror=1,drawbox=y=(256-16):w=iw:h=16:color=aqua@0.2:t=16,drawbox=w=iw:h=(256-235):color=crimson@0.2:t=16,drawgrid=width=0:height=256:thickness=1:color=white@0.5",
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]histogram=step=$2:mode=waveform:waveform_mode=column:waveform_mirror=1,crop=iw:256:0:0,drawbox=y=(256-16):w=iw:h=16:color=aqua@0.2:t=16,drawbox=w=iw:h=(256-235):color=crimson@0.2:t=16[a2];[b1]histogram=step=$2:mode=waveform:waveform_mode=column:waveform_mirror=1,crop=iw:256:0:0,drawbox=y=(256-16):w=iw:h=16:color=aqua@0.2:t=16,drawbox=w=iw:h=(256-235):color=crimson@0.2:t=16[b2];[a2][b2]framepack=tab",
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]transpose=1,histogram=step=$2:mode=waveform:waveform_mode=column:waveform_mirror=1,crop=iw:256:0:0,drawbox=y=(256-16):w=iw:h=16:color=aqua@0.2:t=16,drawbox=w=iw:h=(256-235):color=crimson@0.2:t=16[a2];[b1]transpose=1,histogram=step=$2:mode=waveform:waveform_mode=column:waveform_mirror=1,crop=iw:256:0:0,drawbox=y=(256-16):w=iw:h=16:color=aqua@0.2:t=16,drawbox=w=iw:h=(256-235):color=crimson@0.2:t=16[b2];[a2][b2]framepack=tab",
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]histogram=step=$2:mode=waveform:waveform_mode=column:waveform_mirror=1,drawbox=y=(256-16):w=iw:h=16:color=aqua@0.2:t=16,drawbox=w=iw:h=(256-235):color=crimson@0.2:t=16,drawgrid=width=0:height=256:thickness=1:color=white@0.5[a2];[b1]histogram=step=$2:mode=waveform:waveform_mode=column:waveform_mirror=1,drawbox=y=(256-16):w=iw:h=16:color=aqua@0.2:t=16,drawbox=w=iw:h=(256-235):color=crimson@0.2:t=16,drawgrid=width=0:height=256:thickness=1:color=white@0.5[b2];[a2][b2]framepack",
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]transpose=1,histogram=step=$2:mode=waveform:waveform_mode=column:waveform_mirror=1,drawbox=y=(256-16):w=iw:h=16:color=aqua@0.2:t=16,drawbox=w=iw:h=(256-235):color=crimson@0.2:t=16,drawgrid=width=0:height=256:thickness=1:color=white@0.5[a2];[b1]transpose=1,histogram=step=$2:mode=waveform:waveform_mode=column:waveform_mirror=1,drawbox=y=(256-16):w=iw:h=16:color=aqua@0.2:t=16,drawbox=w=iw:h=(256-235):color=crimson@0.2:t=16,drawgrid=width=0:height=256:thickness=1:color=white@0.5[b2];[a2][b2]framepack",
        },
    },
    {
        "Vectorscope",
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field Split" },
            { Args_Type_Slider,  10,   1, 100,  10, "Scale" },
            { Args_Type_None,     0,   0,   0,   0, },
            { Args_Type_None,     0,   0,   0,   0, },
        },
        {
            "histogram=mode=color2,lutyuv=y=val*$2,transpose=dir=2,scale=512:512,drawgrid=w=32:h=32:t=1:c=white@0.2,drawgrid=w=256:h=256:t=1:c=white@0.3,drawgrid=w=8:h=8:t=1:c=white@0.1,drawbox=w=9:h=9:t=1:x=180-3:y=512-480-5:c=red@0.6,drawbox=w=9:h=9:t=1:x=108-3:y=512-68-5:c=green@0.6,drawbox=w=9:h=9:t=1:x=480-3:y=512-220-5:c=blue@0.6,drawbox=w=9:h=9:t=1:x=332-3:y=512-32-5:c=cyan@0.6,drawbox=w=9:h=9:t=1:x=404-3:y=512-444-5:c=magenta@0.6,drawbox=w=9:h=9:t=1:x=32-3:y=512-292-5:c=yellow@0.6,drawbox=w=9:h=9:t=1:x=199-3:y=512-424-5:c=red@0.8,drawbox=w=9:h=9:t=1:x=145-3:y=512-115-5:c=green@0.8,drawbox=w=9:h=9:t=1:x=424-3:y=512-229-5:c=blue@0.8,drawbox=w=9:h=9:t=1:x=313-3:y=512-88-5:c=cyan@0.8,drawbox=w=9:h=9:t=1:x=367-3:y=512-397-5:c=magenta@0.8,drawbox=w=9:h=9:t=1:x=88-3:y=512-283-5:c=yellow@0.8,drawbox=w=9:h=9:t=1:x=128-3:y=512-452-5:c=sienna@0.8,drawbox=w=9:h=9:t=1:x=160-3:y=512-404-5:c=sienna@0.8,drawbox=w=9:h=9:t=1:x=192-3:y=512-354-5:c=sienna@0.8,drawbox=w=9:h=9:t=1:x=224-3:y=512-304-5:c=sienna@0.8",
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]histogram=mode=color2,lutyuv=y=val*$2,transpose=dir=2,scale=512:512,drawgrid=w=32:h=32:t=1:c=white@0.2,drawgrid=w=256:h=256:t=1:c=white@0.3,drawgrid=w=8:h=8:t=1:c=white@0.1,drawbox=w=9:h=9:t=1:x=180-3:y=512-480-5:c=red@0.6,drawbox=w=9:h=9:t=1:x=108-3:y=512-68-5:c=green@0.6,drawbox=w=9:h=9:t=1:x=480-3:y=512-220-5:c=blue@0.6,drawbox=w=9:h=9:t=1:x=332-3:y=512-32-5:c=cyan@0.6,drawbox=w=9:h=9:t=1:x=404-3:y=512-444-5:c=magenta@0.6,drawbox=w=9:h=9:t=1:x=32-3:y=512-292-5:c=yellow@0.6,drawbox=w=9:h=9:t=1:x=199-3:y=512-424-5:c=red@0.8,drawbox=w=9:h=9:t=1:x=145-3:y=512-115-5:c=green@0.8,drawbox=w=9:h=9:t=1:x=424-3:y=512-229-5:c=blue@0.8,drawbox=w=9:h=9:t=1:x=313-3:y=512-88-5:c=cyan@0.8,drawbox=w=9:h=9:t=1:x=367-3:y=512-397-5:c=magenta@0.8,drawbox=w=9:h=9:t=1:x=88-3:y=512-283-5:c=yellow@0.8,drawbox=w=9:h=9:t=1:x=128-3:y=512-452-5:c=sienna@0.8,drawbox=w=9:h=9:t=1:x=160-3:y=512-404-5:c=sienna@0.8,drawbox=w=9:h=9:t=1:x=192-3:y=512-354-5:c=sienna@0.8,drawbox=w=9:h=9:t=1:x=224-3:y=512-304-5:c=sienna@0.8[a2];[b1]histogram=mode=color2,lutyuv=y=val*$2,transpose=dir=2,scale=512:512,drawgrid=w=32:h=32:t=1:c=white@0.2,drawgrid=w=256:h=256:t=1:c=white@0.3,drawgrid=w=8:h=8:t=1:c=white@0.1,drawbox=w=9:h=9:t=1:x=180-3:y=512-480-5:c=red@0.6,drawbox=w=9:h=9:t=1:x=108-3:y=512-68-5:c=green@0.6,drawbox=w=9:h=9:t=1:x=480-3:y=512-220-5:c=blue@0.6,drawbox=w=9:h=9:t=1:x=332-3:y=512-32-5:c=cyan@0.6,drawbox=w=9:h=9:t=1:x=404-3:y=512-444-5:c=magenta@0.6,drawbox=w=9:h=9:t=1:x=32-3:y=512-292-5:c=yellow@0.6,drawbox=w=9:h=9:t=1:x=199-3:y=512-424-5:c=red@0.8,drawbox=w=9:h=9:t=1:x=145-3:y=512-115-5:c=green@0.8,drawbox=w=9:h=9:t=1:x=424-3:y=512-229-5:c=blue@0.8,drawbox=w=9:h=9:t=1:x=313-3:y=512-88-5:c=cyan@0.8,drawbox=w=9:h=9:t=1:x=367-3:y=512-397-5:c=magenta@0.8,drawbox=w=9:h=9:t=1:x=88-3:y=512-283-5:c=yellow@0.8,drawbox=w=9:h=9:t=1:x=128-3:y=512-452-5:c=sienna@0.8,drawbox=w=9:h=9:t=1:x=160-3:y=512-404-5:c=sienna@0.8,drawbox=w=9:h=9:t=1:x=192-3:y=512-354-5:c=sienna@0.8,drawbox=w=9:h=9:t=1:x=224-3:y=512-304-5:c=sienna@0.8[b2];[a2][b2]framepack=tab",
        },
    },
    {
        "Extract Planes UV",
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field Split" },
            { Args_Type_None,     0,   0,   0,   0, },
            { Args_Type_None,     0,   0,   0,   0, },
            { Args_Type_None,     0,   0,   0,   0, },
        },
        {
            "format=yuv422p|yuv420p|yuv444p|yuv410p,extractplanes=u+v,framepack",
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]format=yuv444p,extractplanes=u+v,framepack[a2];[b1]format=yuv444p,extractplanes=u+v,framepack[b2];[a2][b2]framepack=tab",
        },
    },
    {
        "Extract Planes UV Equal.",
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field Split" },
            { Args_Type_Slider,   2,   0,  10,  10, "Strength" },
            { Args_Type_Slider,   2,   0,  10,  10, "Intensity" },
            { Args_Type_None,     0,   0,   0,   0, },
        },
        {
            "format=yuv422p|yuv420p|yuv444p|yuv410p,extractplanes=u+v,framepack,histeq=strength=$2:intensity=$3",
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]format=yuv422p|yuv420p|yuv444p|yuv410p,extractplanes=u+v,framepack,histeq=strength=$2:strength=$3[a2];[b1]format=yuv422p|yuv420p|yuv444p|yuv410p,extractplanes=u+v,framepack,histeq=strength=$2:intensity=$3[b2];[a2][b2]framepack=tab",
        },
    },
    {
        "Extract Planes",
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field Split" },
            { Args_Type_Radio,    2,   0,   0,   0, ""},
            { Args_Type_None,     0,   0,   0,   0, },
            { Args_Type_None,     0,   0,   0,   0, },
        },
        {
            "format=yuv422p|yuv420p|yuv444p|yuv410p,extractplanes=$2",
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]format=yuv422p|yuv420p|yuv444p|yuv410p,extractplanes=$2[a2];[b1]format=yuv422p|yuv420p|yuv444p|yuv410p,extractplanes=$2[b2];[a2][b2]framepack=tab",
        },
    },
    {
        "Extract Planes Equalized",
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field Split" },
            { Args_Type_Radio,    2,   0,   0,   0, ""},
            { Args_Type_Slider,   2,   0,  10,  10, "Strength" },
            { Args_Type_Slider,   2,   0,  10,  10, "Intensity" },
        },
        {
            "format=yuv422p|yuv420p|yuv444p|yuv410p,extractplanes=$2,histeq=strength=$3:intensity=$4",
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]format=yuv422p|yuv420p|yuv444p|yuv410p,extractplanes=$2,histeq=strength=$3:strength=$4[a2];[b1]format=yuv422p|yuv420p|yuv444p|yuv410p,extractplanes=$2,histeq=strength=$3:intensity=$4[b2];[a2][b2]framepack=tab",
        },
    },
    {
        "Bit Plane",
        {
            { Args_Type_Slider,   1,  -1,   8,   1, "Y bit position" },
            { Args_Type_Slider,   0,  -1,   8,   1, "U bit position" },
            { Args_Type_Slider,   0,  -1,   8,   1, "V bit position" },
            { Args_Type_None,     0,   0,   0,   0, },
        },
        {
            "lutyuv=y=if(eq($1\\,-1)\\,128\\,if(eq($1\\,0)\\,val\\,bitand(val\\,pow(2\\,8-$1))*pow(2\\,$1))):u=if(eq($2\\,-1)\\,128\\,if(eq($2\\,0)\\,val\\,bitand(val\\,pow(2\\,8-$2))*pow(2\\,$2))):v=if(eq($3\\,-1)\\,128\\,if(eq($3\\,0)\\,val\\,bitand(val\\,pow(2\\,8-$3))*pow(2\\,$3)))",
        },
    },
    {
        "Head Switching",
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field Split" },
            { Args_Type_None,     0,   0,   0,   0, },
            { Args_Type_None,     0,   0,   0,   0, },
            { Args_Type_None,     0,   0,   0,   0, },
        },
        {
            "signalstats=out=head",
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]signalstats=out=head[a2];[b1]signalstats=out=head[b2];[a2][b2]framepack=tab",
        },
    },
    {
        "Vertical Line Repetitions",
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field Split" },
            { Args_Type_None,     0,   0,   0,   0, },
            { Args_Type_None,     0,   0,   0,   0, },
            { Args_Type_None,     0,   0,   0,   0, },
        },
        {
            "signalstats=out=vrep",
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]signalstats=out=vrep[a2];[b1]signalstats=out=vrep[b2];[a2][b2]framepack=tab",
        },
    },
    {
        "Broadcast Range Pixels",
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field Split" },
            { Args_Type_None,     0,   0,   0,   0, },
            { Args_Type_None,     0,   0,   0,   0, },
            { Args_Type_None,     0,   0,   0,   0, },
        },
        {
            "signalstats=out=rang",
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]signalstats=out=rang[a2];[b1]signalstats=out=rang[b2];[a2][b2]framepack=tab",
        },
    },
    {
        "Temporal Outlier Pixels",
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field Split" },
            { Args_Type_None,     0,   0,   0,   0, },
            { Args_Type_None,     0,   0,   0,   0, },
            { Args_Type_None,     0,   0,   0,   0, },
        },
        {
            "signalstats=out=tout",
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]signalstats=out=tout[a2];[b1]signalstats=out=tout[b2];[a2][b2]framepack=tab",
        },
    },
    {
        "Value Highlight",
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field Split" },
            { Args_Type_Radio,    0,   0,   0,   0, "Plane" },
            { Args_Type_Slider, 128,   0, 255,   1, "Min"},
            { Args_Type_Slider, 128,   0, 255,   1, "Max"},
        },
        {
            "extractplanes=$2,lutrgb=r=if(between(val\\,$3\\,$4)\\,255\\,val):g=if(between(val\\,$3\\,$4)\\,255\\,val):b=if(between(val\\,$3\\,$4)\\,0\\,val)",
            "extractplanes=$2,split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]lutrgb=r=if(between(val\\,$3\\,$4)\\,255\\,val):g=if(between(val\\,$3\\,$4)\\,255\\,val):b=if(between(val\\,$3\\,$4)\\,0\\,val)[a2];[b1]lutrgb=r=if(between(val\\,$3\\,$4)\\,255\\,val):g=if(between(val\\,$3\\,$4)\\,255\\,val):b=if(between(val\\,$3\\,$4)\\,0\\,val)[b2];[a2][b2]framepack=tab",
        },
    },
    {
        "Chroma Adjust",
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field Split" },
            { Args_Type_Slider,   0,   0, 360,   1, "Hue"},
            { Args_Type_Slider,   1, -10,  10,   1, "Saturation"},
            { Args_Type_None,     0,   0,   0,   0, },
        },
        {
            "hue=h=$2:s=$3",
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]hue=h=$2:s=$3[a2];[b1]hue=h=$2:s=$3[b2];[a2][b2]framepack=tab",
        },
    },
    {
        "(End)",
        {
            { Args_Type_None,   0, 0, 0, 0, },
            { Args_Type_None,   0, 0, 0, 0, },
            { Args_Type_None,   0, 0, 0, 0, },
            { Args_Type_None,   0, 0, 0, 0, },
        },
        {
            "",
        },
    },
};

//***************************************************************************
// Helper
//***************************************************************************

//---------------------------------------------------------------------------
ImageLabel::ImageLabel(FFmpeg_Glue** Picture_, size_t Pos_, QWidget *parent) :
    QWidget(parent),
    Picture(Picture_),
    Pos(Pos_)
{
    Pixmap_MustRedraw=false;
    IsMain=true;
}

//---------------------------------------------------------------------------
void ImageLabel::paintEvent(QPaintEvent *event)
{
    //QWidget::paintEvent(event);

    QPainter painter(this);
    if (!*Picture)
    {
        painter.drawPixmap(0, 0, QPixmap().scaled(event->rect().width(), event->rect().height()));
        return;
    }

    /*
    painter.setRenderHint(QPainter::Antialiasing);

    QSize pixSize = Pixmap.size();
    pixSize.scale(event->rect().size(), Qt::KeepAspectRatio);

    QPixmap scaledPix = Pixmap.scaled(pixSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    */

    QImage* Image;
    switch (Pos)
    {
        case 1 : Image=(*Picture)->Image1; break;
        case 2 : Image=(*Picture)->Image2; break;
        default: return;
    }  
    if (!Image)
    {
        painter.drawPixmap(0, 0, QPixmap().scaled(event->rect().width(), event->rect().height()));
        return;
    }

    QSize Size = event->rect().size();
    if (Pixmap_MustRedraw || Size.width()!=Pixmap.width() || Size.height()!=Pixmap.height())
    {
        if (IsMain && (Size.width()!=Pixmap.width() || Size.height()!=Pixmap.height()))
        {
            (*Picture)->Scale_Change(Size.width(), Size.height());
            switch (Pos)
            {
                case 1 : Image=(*Picture)->Image1; break;
                case 2 : Image=(*Picture)->Image2; break;
                default: return;
            }
        }
        Pixmap.convertFromImage(*Image);
        Pixmap_MustRedraw=false;
    }

    painter.drawPixmap((event->rect().width()-Pixmap.size().width())/2, (event->rect().height()-Pixmap.size().height())/2, Pixmap);
}

//---------------------------------------------------------------------------
void ImageLabel::Remove ()
{
    Pixmap=QPixmap();
    resize(0, 0);
    repaint();
    setVisible(false);
}

//***************************************************************************
// Constructor / Destructor
//***************************************************************************

//---------------------------------------------------------------------------
BigDisplay::BigDisplay(QWidget *parent, FileInformation* FileInformationData_) :
    QDialog(parent),
    FileInfoData(FileInformationData_)
{
    setWindowTitle("QCTools - "+FileInfoData->FileName);
    setWindowFlags(windowFlags() &(0xFFFFFFFF-Qt::WindowContextHelpButtonHint));
    resize(QDesktopWidget().screenGeometry().width()*2/5, QDesktopWidget().screenGeometry().height()*2/5);

    QShortcut *shortcutJ = new QShortcut(QKeySequence(Qt::Key_J), this);
    QObject::connect(shortcutJ, SIGNAL(activated()), this, SLOT(on_M1_triggered()));
    QShortcut *shortcutLeft = new QShortcut(QKeySequence(Qt::Key_Left), this);
    QObject::connect(shortcutLeft, SIGNAL(activated()), this, SLOT(on_Minus_triggered()));
    QShortcut *shortcutK = new QShortcut(QKeySequence(Qt::Key_K), this);
    QObject::connect(shortcutK, SIGNAL(activated()), this, SLOT(on_Pause_triggered()));
    QShortcut *shortcutRight = new QShortcut(QKeySequence(Qt::Key_Right), this);
    QObject::connect(shortcutRight, SIGNAL(activated()), this, SLOT(on_Plus_triggered()));
    QShortcut *shortcutL = new QShortcut(QKeySequence(Qt::Key_L), this);
    QObject::connect(shortcutL, SIGNAL(activated()), this, SLOT(on_P1_triggered()));
    QShortcut *shortcutSpace = new QShortcut(QKeySequence(Qt::Key_Space), this);
    QObject::connect(shortcutSpace, SIGNAL(activated()), this, SLOT(on_PlayPause_triggered()));
    QShortcut *shortcutF = new QShortcut(QKeySequence(Qt::Key_F), this);
    QObject::connect(shortcutF, SIGNAL(activated()), this, SLOT(on_Full_triggered()));

    // FiltersListDefault_Count
    FiltersListDefault_Count=0;
    while (strcmp(Filters[FiltersListDefault_Count].Name, "(End)"))
        FiltersListDefault_Count++;
    for (size_t Pos=0; Pos<2; Pos++)
    {
        for (size_t OptionPos=0; OptionPos<4; OptionPos++)
        {
            Options[Pos].Checks[OptionPos]=NULL;
            Options[Pos].Sliders[OptionPos]=NULL;
            Options[Pos].Sliders_Label[OptionPos]=NULL;
            for (size_t OptionPos2=0; OptionPos2<3; OptionPos2++)
                Options[Pos].Radios[OptionPos][OptionPos2]=NULL;
        }
        Options[Pos].FiltersList_Fake=NULL;
    }

    QFont Font=QFont();
    #ifdef _WIN32
    #else //_WIN32
        Font.setPointSize(Font.pointSize()*3/4);
    #endif //_WIN32

    Layout=new QGridLayout();
    Layout->setContentsMargins(0, 0, 0, 0);

    // Filters
    /*
    FiltersList1=new QToolButton(this);
    FiltersList1->setText("< Filters");
    connect(FiltersList1, SIGNAL(pressed()), this, SLOT(on_FiltersList1_click()));
    Layout->addWidget(FiltersList1, 0, 0, 1, 1, Qt::AlignLeft);
    FiltersList2=new QToolButton();
    FiltersList2->setText("Filters >");
    connect(FiltersList2, SIGNAL(pressed()), this, SLOT(on_FiltersList2_click()));
    Layout->addWidget(FiltersList2, 0, 2, 1, 1, Qt::AlignRight);
    */
    for (size_t Pos=0; Pos<2; Pos++)
    {
        Options[Pos].FiltersList=new QComboBox(this);
        Options[Pos].FiltersList->setFont(Font);
        Layout->addWidget(Options[Pos].FiltersList, 0, 0, 1, 1, Pos==0?Qt::AlignLeft:Qt::AlignRight);
        for (size_t FilterPos=0; FilterPos<FiltersListDefault_Count; FilterPos++)
        {
            if (strcmp(Filters[FilterPos].Name, "(Separator)") && strcmp(Filters[FilterPos].Name, "(End)"))
                Options[Pos].FiltersList->addItem(Filters[FilterPos].Name);
            else if (strcmp(Filters[FilterPos].Name, "(End)"))
                Options[Pos].FiltersList->insertSeparator(FiltersListDefault_Count);
        }
        Options[Pos].FiltersList->setMinimumWidth(Options[Pos].FiltersList->minimumSizeHint().width());
        Options[Pos].FiltersList->setMaximumWidth(Options[Pos].FiltersList->minimumSizeHint().width());
        Options[Pos].FiltersList->setMaxVisibleItems(25);
    }

    //Image1
    Image1=new ImageLabel(&Picture, 1, this);
    Image1->IsMain=true;
    Image1->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    Layout->addWidget(Image1, 1, 0, 1, 1);
    Layout->setColumnStretch(0, 1);

    //Image2
    Image2=new ImageLabel(&Picture, 2, this);
    Image2->IsMain=false;
    Image2->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    Layout->addWidget(Image2, 1, 2, 1, 1);
    Layout->setColumnStretch(2, 1);

    // Info
    InfoArea=NULL;
    //InfoArea=new Info(this, FileInfoData, Info::Style_Columns);
    //Layout->addWidget(InfoArea, 1, 1, 1, 3, Qt::AlignHCenter);
    //Layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding), 2, 1, 1, 3, Qt::AlignHCenter);

    // Slider
    Slider=new QSlider(Qt::Horizontal);
    Slider->setMaximum(FileInfoData->Glue->VideoFrameCount);
    connect(Slider, SIGNAL(sliderMoved(int)), this, SLOT(on_Slider_sliderMoved(int)));
    connect(Slider, SIGNAL(actionTriggered(int)), this, SLOT(on_Slider_actionTriggered(int)));
    Layout->addWidget(Slider, 2, 0, 1, 3);

    // Control
    ControlArea=new Control(this, FileInfoData, Control::Style_Cols, true);
    Layout->addWidget(ControlArea, 3, 0, 1, 3, Qt::AlignBottom);

    setLayout(Layout);

    // Picture
    Picture=NULL;
    Picture_Current1=2;
    Picture_Current2=6;
    Options[0].FiltersList->setCurrentIndex(Picture_Current1);
    Options[1].FiltersList->setCurrentIndex(Picture_Current2);
    connect(Options[0].FiltersList, SIGNAL(currentIndexChanged(int)), this, SLOT(on_FiltersList1_currentIndexChanged(int)));
    connect(Options[1].FiltersList, SIGNAL(currentIndexChanged(int)), this, SLOT(on_FiltersList2_currentIndexChanged(int)));

    // Info
    Frames_Pos=-1;
    ShouldUpate=false;
}

//---------------------------------------------------------------------------
BigDisplay::~BigDisplay()
{
    delete Picture;
}

//---------------------------------------------------------------------------
void BigDisplay::FiltersList_currentIndexChanged(size_t Pos, size_t FilterPos, QGridLayout* Layout0)
{
    // Options
    for (size_t OptionPos=0; OptionPos<4; OptionPos++)
    {
        delete Options[Pos].Checks[OptionPos]; Options[Pos].Checks[OptionPos]=NULL;
        delete Options[Pos].Sliders[OptionPos]; Options[Pos].Sliders[OptionPos]=NULL;
        delete Options[Pos].Sliders_Label[OptionPos]; Options[Pos].Sliders_Label[OptionPos]=NULL;
        for (size_t OptionPos2=0; OptionPos2<3; OptionPos2++)
        {
            delete Options[Pos].Radios[OptionPos][OptionPos2]; Options[Pos].Radios[OptionPos][OptionPos2]=NULL;
        }
    }
    delete Options[Pos].FiltersList_Fake; Options[Pos].FiltersList_Fake=NULL;
    Layout0->setContentsMargins(0, 0, 0, 0);
    QFont Font=QFont();
    #ifdef _WIN32
    #else //_WIN32
        Font.setPointSize(Font.pointSize()*3/4);
    #endif //_WIN32
    size_t Options_Checks_Pos=0;
    size_t Options_Sliders_Pos=0;
    size_t Options_Radios_Pos=0;
    for (size_t OptionPos=0; OptionPos<4; OptionPos++)
    {
        if (FilterPos>=PreviousValues[Pos].size())
            PreviousValues[Pos].resize(FilterPos+1);
        if (PreviousValues[Pos][FilterPos].Values[OptionPos]==-2)
            PreviousValues[Pos][FilterPos].Values[OptionPos]=Filters[FilterPos].Args[OptionPos].Default;
        switch (Filters[FilterPos].Args[OptionPos].Type)
        {
            case Args_Type_Toggle:
                                    {
                                    Options[Pos].Checks[Options_Checks_Pos]=new QCheckBox(Filters[FilterPos].Args[OptionPos].Name);
                                    Options[Pos].Checks[Options_Checks_Pos]->setFont(Font);
                                    Options[Pos].Checks[Options_Checks_Pos]->setChecked(PreviousValues[Pos][FilterPos].Values[OptionPos]);
                                    connect(Options[Pos].Checks[Options_Checks_Pos], SIGNAL(stateChanged(int)), this, Pos==0?(SLOT(on_FiltersOptions1_click())):SLOT(on_FiltersOptions2_click()));
                                    Layout0->addWidget(Options[Pos].Checks[Options_Checks_Pos], 0, 1+Options_Checks_Pos+Options_Sliders_Pos+Options_Radios_Pos*3);
                                    Options_Checks_Pos++;
                                    }
                                    break;
            case Args_Type_Slider:
                                    {
                                    Options[Pos].Sliders[Options_Sliders_Pos]=new QSlider(Qt::Horizontal);
                                    Options[Pos].Sliders[Options_Sliders_Pos]->setMinimum(Filters[FilterPos].Args[OptionPos].Min);
                                    Options[Pos].Sliders[Options_Sliders_Pos]->setMaximum(Filters[FilterPos].Args[OptionPos].Max);
                                    Options[Pos].Sliders[Options_Sliders_Pos]->setValue(PreviousValues[Pos][FilterPos].Values[OptionPos]);
                                    Options[Pos].Sliders[Options_Sliders_Pos]->setToolTip(Filters[FilterPos].Args[OptionPos].Name);
                                    connect(Options[Pos].Sliders[Options_Sliders_Pos], SIGNAL(sliderMoved(int)), this, Pos==0?(SLOT(on_FiltersOptions1_click())):SLOT(on_FiltersOptions2_click()));
                                    connect(Options[Pos].Sliders[Options_Sliders_Pos], SIGNAL(actionTriggered(int)), this, Pos==0?(SLOT(on_FiltersOptions1_click())):SLOT(on_FiltersOptions2_click()));
                                    Layout0->addWidget(Options[Pos].Sliders[Options_Sliders_Pos], 0, 1+Options_Checks_Pos+Options_Sliders_Pos+Options_Radios_Pos*3);
                                    if (QString(Filters[FilterPos].Args[OptionPos].Name).contains(" bit position") && PreviousValues[Pos][FilterPos].Values[OptionPos]<1)
                                    {
                                        if (PreviousValues[Pos][FilterPos].Values[OptionPos]==0)
                                            Options[Pos].Sliders_Label[Options_Sliders_Pos]=new QLabel(Filters[FilterPos].Args[OptionPos].Name+QString(": all"));
                                        else
                                            Options[Pos].Sliders_Label[Options_Sliders_Pos]=new QLabel(Filters[FilterPos].Args[OptionPos].Name+QString(": none"));
                                    }
                                    else
                                        Options[Pos].Sliders_Label[Options_Sliders_Pos]=new QLabel(Filters[FilterPos].Args[OptionPos].Name+QString(": ")+QString::number(((double)PreviousValues[Pos][FilterPos].Values[OptionPos])/Filters[FilterPos].Args[OptionPos].Divisor));
                                    Options[Pos].Sliders_Label[Options_Sliders_Pos]->setFont(Font);
                                    Layout0->addWidget(Options[Pos].Sliders_Label[Options_Sliders_Pos], 1, 1+Options_Checks_Pos+Options_Sliders_Pos+Options_Radios_Pos*3);
                                    Options_Sliders_Pos++;
                                    }
                                    break;
            case Args_Type_Radio:
                                    for (size_t OptionPos2=0; OptionPos2<3; OptionPos2++)
                                    {
                                        Options[Pos].Radios[Options_Radios_Pos][OptionPos2]=new QRadioButton();
                                        Options[Pos].Radios[Options_Radios_Pos][OptionPos2]->setFont(Font);
                                        switch (OptionPos2)
                                        {
                                            case 0: Options[Pos].Radios[Options_Radios_Pos][OptionPos2]->setText("Y"); break;
                                            case 1: Options[Pos].Radios[Options_Radios_Pos][OptionPos2]->setText("U"); break;
                                            case 2: Options[Pos].Radios[Options_Radios_Pos][OptionPos2]->setText("V"); break;
                                            default:;
                                        }
                                        if (OptionPos2==PreviousValues[Pos][FilterPos].Values[OptionPos])
                                            Options[Pos].Radios[Options_Radios_Pos][OptionPos2]->setChecked(true);
                                        connect(Options[Pos].Radios[Options_Radios_Pos][OptionPos2], SIGNAL(toggled(bool)), this, Pos==0?(SLOT(on_FiltersOptions1_click())):SLOT(on_FiltersOptions2_click()));
                                        Layout0->addWidget(Options[Pos].Radios[Options_Radios_Pos][OptionPos2], 0, 1+Options_Checks_Pos+Options_Sliders_Pos+Options_Radios_Pos*3+OptionPos2);
                                    }
                                    Options_Radios_Pos++;
                                    break;
            default:                ;
        }
    }
}

//---------------------------------------------------------------------------
void BigDisplay::FiltersList1_currentIndexChanged(size_t FilterPos)
{
    QGridLayout* Layout0=new QGridLayout();
    Layout0->addWidget(Options[0].FiltersList, 0, 0, Qt::AlignLeft);
    FiltersList_currentIndexChanged(0, FilterPos, Layout0);
    Options[0].FiltersList_Fake=new QLabel(" ");
    Layout0->addWidget(Options[0].FiltersList_Fake, 1, 0, Qt::AlignLeft);
    Layout0->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum), 0, 14);
    Layout->addLayout(Layout0, 0, 0, 1, 1, Qt::AlignLeft|Qt::AlignTop);

    if (Picture_Current1<2)
    {
        Image1->setVisible(true);
        Layout->setColumnStretch(0, 1);
        //resize(width()+Image_Width, height());
    }
    Picture_Current1=FilterPos;
    FiltersList1_currentOptionChanged(Picture_Current1);
}

//---------------------------------------------------------------------------
void BigDisplay::FiltersList2_currentIndexChanged(size_t FilterPos)
{
    QGridLayout* Layout0=new QGridLayout();
    Layout0->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum), 0, 0);
    FiltersList_currentIndexChanged(1, FilterPos, Layout0);
    Layout0->addWidget(Options[1].FiltersList, 0, 14, Qt::AlignRight);
    Options[1].FiltersList_Fake=new QLabel(" ");
    Layout0->addWidget(Options[1].FiltersList_Fake, 1, 14, Qt::AlignRight);
    Layout->addLayout(Layout0, 0, 2, 1, 1, Qt::AlignRight|Qt::AlignTop);

    if (Picture_Current2<2)
    {
        Image2->setVisible(true);
        Layout->setColumnStretch(2, 1);
        //resize(width()+Image_Width, height());
    }
    Picture_Current2=FilterPos;
    FiltersList2_currentOptionChanged(Picture_Current2);
}

//---------------------------------------------------------------------------
string BigDisplay::FiltersList_currentOptionChanged(size_t Pos, size_t Picture_Current)
{
    size_t Value_Pos=0;
    size_t Options_Checks_Pos=0;
    size_t Options_Sliders_Pos=0;
    size_t Options_Radios_Pos=0;
    bool Modified=false;
    double WithSliders[4];
    WithSliders[0]=-2;
    WithSliders[1]=-2;
    WithSliders[2]=-2;
    WithSliders[3]=-2;
    string WithRadios[4];
    WithRadios[0]="";
    WithRadios[1]="";
    WithRadios[2]="";
    WithRadios[3]="";
    string Modified_String;
    for (size_t OptionPos=0; OptionPos<4; OptionPos++)
    {
        switch (Filters[Picture_Current].Args[OptionPos].Type)
        {
            case Args_Type_Toggle:
                                    Value_Pos<<=1;
                                    Value_Pos|=(Options[Pos].Checks[Options_Checks_Pos]->isChecked()?1:0);
                                    PreviousValues[Pos][Picture_Current].Values[OptionPos]=Options[Pos].Checks[Options_Checks_Pos]->isChecked()?1:0;
                                    Options_Checks_Pos++;
                                    break;
            case Args_Type_Slider:
                                    Modified=true;
                                    WithSliders[OptionPos]=((double)Options[Pos].Sliders[Options_Sliders_Pos]->sliderPosition())/Filters[Picture_Current].Args[OptionPos].Divisor;
                                    PreviousValues[Pos][Picture_Current].Values[OptionPos]=Options[Pos].Sliders[Options_Sliders_Pos]->sliderPosition();
                                    if (QString(Filters[Picture_Current].Args[OptionPos].Name).contains(" bit position") && PreviousValues[Pos][Picture_Current].Values[OptionPos]<1)
                                    {
                                        if (PreviousValues[Pos][Picture_Current].Values[OptionPos]==0)
                                            Options[Pos].Sliders_Label[Options_Sliders_Pos]->setText(Filters[Picture_Current].Args[OptionPos].Name+QString(": all"));
                                        else
                                            Options[Pos].Sliders_Label[Options_Sliders_Pos]->setText(Filters[Picture_Current].Args[OptionPos].Name+QString(": none"));
                                    }
                                    else
                                        Options[Pos].Sliders_Label[Options_Sliders_Pos]->setText(Filters[Picture_Current].Args[OptionPos].Name+QString(": ")+QString::number(WithSliders[OptionPos]));
                                    Options_Sliders_Pos++;
                                    break;
            case Args_Type_Radio:
                                    Modified=true;
                                    for (size_t OptionPos2=0; OptionPos2<3; OptionPos2++)
                                    {
                                        if (Options[Pos].Radios[Options_Radios_Pos][OptionPos2] && Options[Pos].Radios[Options_Radios_Pos][OptionPos2]->isChecked())
                                        {
                                            switch (OptionPos2)
                                            {
                                                case 0: WithRadios[OptionPos]="y"; break;
                                                case 1: WithRadios[OptionPos]="u"; break;
                                                case 2: WithRadios[OptionPos]="v"; break;
                                                default:;
                                            }
                                            PreviousValues[Pos][Picture_Current].Values[OptionPos]=OptionPos2;
                                            Options_Radios_Pos++;
                                            break;
                                        }
                                    }
                                    break ;
            default:                ;
        }
    }
    
    Modified_String=Filters[Picture_Current].Value[Value_Pos];
    if (Modified)
    {
        for (size_t OptionPos=0; OptionPos<4; OptionPos++)
            switch (Filters[Picture_Current].Args[OptionPos].Type)
            {
                case Args_Type_Slider:
                                        {
                                        char ToFind[3];
                                        ToFind[0]='$';
                                        ToFind[1]='1'+OptionPos;
                                        ToFind[2]='\0';
                                        for (;;)
                                        {
                                            size_t InsertPos=Modified_String.find(ToFind);
                                            if (InsertPos==string::npos)
                                                break;
                                            Modified_String.erase(InsertPos, 2);
                                            Modified_String.insert(InsertPos, QString::number(WithSliders[OptionPos]).toUtf8());
                                        }
                                        }
                                        break;
                case Args_Type_Radio:
                                        {
                                        char ToFind[3];
                                        ToFind[0]='$';
                                        ToFind[1]='1'+OptionPos;
                                        ToFind[2]='\0';
                                        for (;;)
                                        {
                                            size_t InsertPos=Modified_String.find(ToFind);
                                            if (InsertPos==string::npos)
                                                break;
                                            Modified_String.erase(InsertPos, 2);
                                            Modified_String.insert(InsertPos, WithRadios[OptionPos]);
                                        }
                                        }
                                        break;
                default:                ;
            }
    }

    return Modified_String;
}

//---------------------------------------------------------------------------
void BigDisplay::FiltersList1_currentOptionChanged(size_t Picture_Current)
{
    string Modified_String=FiltersList_currentOptionChanged(0, Picture_Current);
    Picture->Filter1_Change(Modified_String.c_str());

    Frames_Pos=(size_t)-1;
    ShowPicture ();
}

//---------------------------------------------------------------------------
void BigDisplay::FiltersList2_currentOptionChanged(size_t Picture_Current)
{
    string Modified_String=FiltersList_currentOptionChanged(1, Picture_Current);
    Picture->Filter2_Change(Modified_String.c_str());

    Frames_Pos=(size_t)-1;
    ShowPicture ();
}

//***************************************************************************
// Actions
//***************************************************************************

//---------------------------------------------------------------------------
void BigDisplay::ShowPicture ()
{
    if (!isVisible())
        return;

    if ((!ShouldUpate && Frames_Pos==FileInfoData->Frames_Pos_Get())
     || ( ShouldUpate && false)) // ToDo: try to optimize
        return;
    Frames_Pos=FileInfoData->Frames_Pos_Get();
    ShouldUpate=false;
    
    // Stats
    if (ControlArea)
        ControlArea->Update();
    if (InfoArea)
        InfoArea->Update();

    // Picture
    if (!Picture)
    {
        string FileName_string=FileInfoData->FileName.toUtf8().data();
        #ifdef _WIN32
            replace(FileName_string.begin(), FileName_string.end(), '/', '\\' );
        #endif
        Picture=new FFmpeg_Glue(FileName_string.c_str(), QDesktopWidget().screenGeometry().width()*2/5, QDesktopWidget().screenGeometry().height()*2/5, FFmpeg_Glue::Output_QImage, "", "", Picture_Current1<FiltersListDefault_Count, Picture_Current2<FiltersListDefault_Count); ///*Filters[Picture_Current1].Value[0], Filters[Picture_Current2].Value[0] removed else there is random crashs in libavfilters, maybe due to bad libavfilter config
        FiltersList1_currentIndexChanged(Picture_Current1);
        FiltersList2_currentIndexChanged(Picture_Current2);
    }
    Picture->FrameAtPosition(Frames_Pos);
    if (Picture->Image1)
    {
        Image_Width=Picture->Image1->width();
        Image_Height=Picture->Image1->height();
    }

    if (Slider->sliderPosition()!=Frames_Pos)
        Slider->setSliderPosition(Frames_Pos);

    Image1->Pixmap_MustRedraw=true;
    Image1->repaint();
    Image2->Pixmap_MustRedraw=true;
    Image2->repaint();
}

//***************************************************************************
// Events
//***************************************************************************

//---------------------------------------------------------------------------
void BigDisplay::on_Slider_sliderMoved(int value)
{
    ControlArea->Pause->click();
        
    FileInfoData->Frames_Pos_Set(value);
}

//---------------------------------------------------------------------------
void BigDisplay::on_Slider_actionTriggered(int action )
{
    if (action==QAbstractSlider::SliderMove)
        return;

    ControlArea->Pause->click();
        
    FileInfoData->Frames_Pos_Set(Slider->sliderPosition());
}

//---------------------------------------------------------------------------
void BigDisplay::on_FiltersSource_stateChanged(int state)
{
    ShowPicture ();
}

//---------------------------------------------------------------------------
void BigDisplay::on_FiltersList1_click()
{
    QMenu* Menu=new QMenu(this);
    QActionGroup* ActionGroup=new QActionGroup(this);

    for (size_t Pos=0; Pos<FiltersListDefault_Count; Pos++)
    {
        if (strcmp(Filters[Pos].Name, "(Separator)"))
        {
            QAction* Action=new QAction(Filters[Pos].Name, this);
            Action->setCheckable(true);
            if (Pos==Picture_Current1)
                Action->setChecked(true);
            if (Pos)
                ActionGroup->addAction(Action);
            Menu->addAction(Action);
        }
        else
            Menu->addSeparator();
    }

    connect(Menu, SIGNAL(triggered(QAction*)), this, SLOT(on_FiltersList1_currentIndexChanged(QAction*)));
    Menu->exec(Options[0].FiltersList->mapToGlobal(QPoint(-Menu->width(), Options[0].FiltersList->height())));
}

//---------------------------------------------------------------------------
void BigDisplay::on_FiltersList2_click()
{
    QMenu* Menu=new QMenu(this);
    QActionGroup* ActionGroup=new QActionGroup(this);

    for (size_t Pos=0; Pos<FiltersListDefault_Count; Pos++)
    {
        if (strcmp(Filters[Pos].Name, "(Separator)"))
        {
            QAction* Action=new QAction(Filters[Pos].Name, this);
            Action->setCheckable(true);
            if (Pos==Picture_Current2)
                Action->setChecked(true);
            if (Pos)
                ActionGroup->addAction(Action);
            Menu->addAction(Action);
        }
        else
            Menu->addSeparator();
    }

    connect(Menu, SIGNAL(triggered(QAction*)), this, SLOT(on_FiltersList2_currentIndexChanged(QAction*)));
    Menu->exec(Options[1].FiltersList->mapToGlobal(QPoint(Options[1].FiltersList->width(), Options[1].FiltersList->height())));
}

//---------------------------------------------------------------------------
void BigDisplay::on_FiltersOptions1_click()
{
    FiltersList1_currentOptionChanged(Picture_Current1);
}

//---------------------------------------------------------------------------
void BigDisplay::on_FiltersOptions2_click()
{
    FiltersList2_currentOptionChanged(Picture_Current2);
}

//---------------------------------------------------------------------------
void BigDisplay::on_FiltersList1_currentIndexChanged(QAction * action)
{
    // Help
    if (action->text()=="Help")
    {
        Help* Frame=new Help(this);
        Frame->PlaybackFilters();
        return;
    }

    // None
    if (action->text()=="No display")
    {
        Picture->With1_Change(false);
        Image1->Remove();
        Layout->setColumnStretch(0, 0);
        //move(pos().x()+Image_Width, pos().y());
        //adjustSize();
        Picture_Current1=1;
        Image1->IsMain=false;
        Image2->IsMain=true;
        repaint();
        return;
    }

    // Filters
    size_t Pos=0;
    for (; Pos<FiltersListDefault_Count; Pos++)
    {
        if (action->text()==Filters[Pos].Name)
        {
            if (Picture_Current1<2)
            {
                Image1->setVisible(true);
                Image1->IsMain=true;
                Image2->IsMain=false;
                Layout->setColumnStretch(0, 1);
                //move(pos().x()-Image_Width, pos().y());
                //resize(width()+Image_Width, height());
            }
            Picture_Current1=Pos;
            Picture->Filter1_Change(Filters[Pos].Value[0]);

            Frames_Pos=(size_t)-1;
            ShowPicture ();
            return;
        }
    }
}

//---------------------------------------------------------------------------
void BigDisplay::on_FiltersList1_currentIndexChanged(int Pos)
{
    // Help
    if (Pos==0)
    {
        Help* Frame=new Help(this);
        Frame->PlaybackFilters();
        return;
    }

    // None
    if (Pos==1)
    {
        Picture->With1_Change(false);
        Image1->Remove();
        Layout->setColumnStretch(0, 0);
        //move(pos().x()+Image_Width, pos().y());
        //adjustSize();
        Image1->IsMain=false;
        Image2->IsMain=true;
        repaint();
    }

    if (Picture_Current1<2)
    {
        Image1->setVisible(true);
        Image1->IsMain=true;
        Image2->IsMain=false;
        Layout->setColumnStretch(0, 1);
        //move(pos().x()-Image_Width, pos().y());
        //resize(width()+Image_Width, height());
    }
    FiltersList1_currentIndexChanged(Pos);

    Frames_Pos=(size_t)-1;
    ShowPicture ();
}

//---------------------------------------------------------------------------
void BigDisplay::on_FiltersList2_currentIndexChanged(int Pos)
{
    // Help
    if (Pos==0)
    {
        Help* Frame=new Help(this);
        Frame->PlaybackFilters();
        return;
    }

    // None
    if (Pos==1)
    {
        Picture->With2_Change(false);
        Image2->Remove();
        Layout->setColumnStretch(2, 0);
        //adjustSize();
        repaint();
    }

    FiltersList2_currentIndexChanged(Pos);
}

//---------------------------------------------------------------------------
void BigDisplay::on_FiltersList2_currentIndexChanged(QAction * action)
{
    // Help
    if (action->text()=="Help")
    {
        Help* Frame=new Help(this);
        Frame->PlaybackFilters();
        return;
    }

    // None
    if (action->text()=="No display")
    {
        Picture->With2_Change(false);
        Image2->Remove();
        Layout->setColumnStretch(2, 0);
        //adjustSize();
        Picture_Current2=1;
        repaint();
        return;
    }

    // Filters
    size_t Pos=0;
    for (; Pos<FiltersListDefault_Count; Pos++)
    {
        if (action->text()==Filters[Pos].Name)
        {
            FiltersList2_currentIndexChanged(Pos);
            return;
        }
    }
}

//---------------------------------------------------------------------------
void BigDisplay::resizeEvent(QResizeEvent* Event)
{
    if (Event->oldSize().width()<0 || Event->oldSize().height()<0)
        return;

    /*int DiffX=(Event->size().width()-Event->oldSize().width())/2;
    int DiffY=(Event->size().width()-Event->oldSize().width())/2;
    Picture->Scale_Change(Image_Width+DiffX, Image_Height+DiffY);

    Frames_Pos=(size_t)-1;
    ShowPicture ();*/
    int SizeX=(Event->size().width()-(InfoArea?InfoArea->width():0))/2-25;
    int SizeY=(Event->size().height()-Slider->height())-50;

    /*if (InfoArea->height()+FiltersList1->height()+Slider->height()+ControlArea->height()+50>=Event->size().height())
    {
        InfoArea->hide();
        Layout->removeWidget(InfoArea);
    }
    else
    {
        Layout->addWidget(InfoArea, 0, 1, 1, 3, Qt::AlignLeft);
        InfoArea->show();
    }*/

    //adjust();
    //Picture->Scale_Change(Image1->width(), Image1->height());

    //Frames_Pos=(size_t)-1;
    //ShowPicture ();
    Image1->Pixmap_MustRedraw=true;
    Image2->Pixmap_MustRedraw=true;
}

//---------------------------------------------------------------------------
void BigDisplay::on_M1_triggered()
{
    if (ControlArea)
        ControlArea->M1->clicked(true);
}

//---------------------------------------------------------------------------
void BigDisplay::on_Minus_triggered()
{
    on_Pause_triggered();
    if (ControlArea)
        ControlArea->Minus->clicked(true);
}

//---------------------------------------------------------------------------
void BigDisplay::on_PlayPause_triggered()
{
    if (ControlArea)
        ControlArea->PlayPause->clicked(true);
}

//---------------------------------------------------------------------------
void BigDisplay::on_Pause_triggered()
{
    if (ControlArea)
        ControlArea->Pause->clicked(true);
}

//---------------------------------------------------------------------------
void BigDisplay::on_Plus_triggered()
{
    on_Pause_triggered();
    if (ControlArea)
        ControlArea->Plus->clicked(true);
}

//---------------------------------------------------------------------------
void BigDisplay::on_P1_triggered()
{
    if (ControlArea)
        ControlArea->P1->clicked(true);
}

//---------------------------------------------------------------------------
void BigDisplay::on_Full_triggered()
{
    if (isMaximized())
        setWindowState(Qt::WindowActive);
    else
        setWindowState(Qt::WindowMaximized);
}
