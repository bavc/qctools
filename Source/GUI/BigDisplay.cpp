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
#include <QHBoxLayout>
#include <QPixmap>
#include <QComboBox>
#include <QSlider>
#include <QDoubleSpinBox>
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
#include <QButtonGroup>
#include <QPushButton>
#include <QColorDialog>
#include <QShortcut>
#include <QApplication>

#include <sstream>
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
    Args_Type_Yuv,  // Y, U , V
    Args_Type_YuvA, // Y, U, V, All
    Args_Type_Tile, // 4x4, 6x6, 8x8, 10x10
    Args_Type_ClrPck, // Color picker
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
    const args          Args[Args_Max];
    const char*         Formula[1<<Args_Max]; //Max 2^Args_Max toggles
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
            { Args_Type_None,   0, 0, 0, 0, },
        },
        {
            "",
        },
    },
    {
        "Field Difference",
        {
            { Args_Type_YuvA,     3,   0,   0,   0, "Plane" },
            { Args_Type_Slider,   2,   0,  10,  10, "Strength" },
            { Args_Type_Slider,   2,   0,  10,  10, "Intensity" },
            { Args_Type_Toggle,   0,   0,   0,   0, "Columns" },
            { Args_Type_None,     0,   0,   0,   0, },
        },
        {
            "extractplanes=${1},split[a][b];[a]field=bottom[a1];[b]field=top,negate[b2];[a1][b2]blend=all_mode=average,histeq=strength=${2}:intensity=${3}",
            "extractplanes=${1},transpose=1,split[a][b];[a]field=bottom[a1];[b]field=top,negate[b2];[a1][b2]blend=all_mode=average,histeq=strength=${2}:intensity=${3},transpose=2",
            "split[a][b];[a]field=bottom[a1];[b]field=top,negate[b2];[a1][b2]blend=all_mode=average,histeq=strength=${2}:intensity=${3}",
            "transpose=1,split[a][b];[a]field=bottom[a1];[b]field=top,negate[b2];[a1][b2]blend=all_mode=average,histeq=strength=${2}:intensity=${3},transpose=2",
        },
    },
    /*
    {
        "Frame Metadata Play",
        {
            { Args_Type_None,   0, 0, 0, 0, },
            { Args_Type_None,   0, 0, 0, 0, },
            { Args_Type_None,   0, 0, 0, 0, },
            { Args_Type_None,   0, 0, 0, 0, },
            { Args_Type_None,   0, 0, 0, 0, },
        },
        {
            "cropdetect=reset=1:limit=16:round=1,signalstats=stat=brng+vrep+tout,drawtext=fontfile=Anonymous_Pro_B.ttf:x=8:y=8:fontcolor=yellow:shadowx=3:shadowy=2:fontsize=20:tabsize=8:textfile=drawtext.txt",
        },
    },
    */
    {
        "Histogram",
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field Split" },
            { Args_Type_Toggle,   0,   0,   0,   0, "RGB" },
            { Args_Type_YuvA,     3,   0,   0,   0, "Plane" },
            { Args_Type_None,     0,   0,   0,   0, },
            { Args_Type_None,     0,   0,   0,   0, },
        },
        {
            "extractplanes=${3},histogram",
            "histogram",
            "format=rgb48,extractplanes=${3},histogram",
            "format=rgb48,histogram",
            "extractplanes=${3},split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]histogram[a2];[b1]histogram[b2];[a2][b2]framepack",
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]histogram[a2];[b1]histogram[b2];[a2][b2]framepack",
            "format=rgb48,extractplanes=${3},split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]histogram[a2];[b1]histogram[b2];[a2][b2]framepack",
            "format=rgb48,split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]histogram[a2];[b1]histogram[b2];[a2][b2]framepack",
        },
    },
    {
        "Waveform",
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field Split" },
            { Args_Type_Slider,  20,   1, 255,   1, "Brightness" },
            { Args_Type_YuvA,     0,   0,   0,   0, "Plane" },
            { Args_Type_Toggle,   0,   0,   0,   0, "Vertical" },
            { Args_Type_None,     0,   0,   0,   0, },
        },
        {
            "histogram=step=${2}:mode=waveform:waveform_mode=column:waveform_mirror=1,crop=iw:256:0:${3},drawbox=y=(256-16):w=iw:h=16:color=aqua@0.3:t=16,drawbox=w=iw:h=(256-235):color=crimson@0.3:t=16",
            "transpose=0,histogram=step=${2}:mode=waveform:waveform_mode=column:waveform_mirror=1,crop=iw:256:0:${3},drawbox=y=(256-16):w=iw:h=16:color=aqua@0.3:t=16,drawbox=w=iw:h=(256-235):color=crimson@0.3:t=16",
            "histogram=step=${2}:mode=waveform:waveform_mode=column:waveform_mirror=1,drawbox=y=(256-16):w=iw:h=16:color=aqua@0.3:t=16,drawbox=w=iw:h=(256-235):color=crimson@0.3:t=16,drawgrid=width=0:height=256:thickness=1:color=white@0.5",
            "transpose=0,histogram=step=${2}:mode=waveform:waveform_mode=column:waveform_mirror=1,drawbox=y=(256-16):w=iw:h=16:color=aqua@0.3:t=16,drawbox=w=iw:h=(256-235):color=crimson@0.3:t=16,drawgrid=width=0:height=256:thickness=1:color=white@0.5",
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]histogram=step=${2}:mode=waveform:waveform_mode=column:waveform_mirror=1,crop=iw:256:0:${3},drawbox=y=(256-16):w=iw:h=16:color=aqua@0.3:t=16,drawbox=w=iw:h=(256-235):color=crimson@0.3:t=16[a2];[b1]histogram=step=${2}:mode=waveform:waveform_mode=column:waveform_mirror=1,crop=iw:256:0:${3},drawbox=y=(256-16):w=iw:h=16:color=aqua@0.3:t=16,drawbox=w=iw:h=(256-235):color=crimson@0.3:t=16[b2];[a2][b2]framepack=tab",
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]transpose=0,histogram=step=${2}:mode=waveform:waveform_mode=column:waveform_mirror=1,crop=iw:256:0:${3},drawbox=y=(256-16):w=iw:h=16:color=aqua@0.3:t=16,drawbox=w=iw:h=(256-235):color=crimson@0.3:t=16[a2];[b1]transpose=0,histogram=step=${2}:mode=waveform:waveform_mode=column:waveform_mirror=1,crop=iw:256:0:${3},drawbox=y=(256-16):w=iw:h=16:color=aqua@0.3:t=16,drawbox=w=iw:h=(256-235):color=crimson@0.3:t=16[b2];[a2][b2]framepack=tab",
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]histogram=step=${2}:mode=waveform:waveform_mode=column:waveform_mirror=1,drawbox=y=(256-16):w=iw:h=16:color=aqua@0.3:t=16,drawbox=w=iw:h=(256-235):color=crimson@0.3:t=16,drawgrid=width=0:height=256:thickness=1:color=white@0.5[a2];[b1]histogram=step=${2}:mode=waveform:waveform_mode=column:waveform_mirror=1,drawbox=y=(256-16):w=iw:h=16:color=aqua@0.3:t=16,drawbox=w=iw:h=(256-235):color=crimson@0.3:t=16,drawgrid=width=0:height=256:thickness=1:color=white@0.5[b2];[a2][b2]framepack",
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]transpose=0,histogram=step=${2}:mode=waveform:waveform_mode=column:waveform_mirror=1,drawbox=y=(256-16):w=iw:h=16:color=aqua@0.3:t=16,drawbox=w=iw:h=(256-235):color=crimson@0.3:t=16,drawgrid=width=0:height=256:thickness=1:color=white@0.5[a2];[b1]transpose=0,histogram=step=${2}:mode=waveform:waveform_mode=column:waveform_mirror=1,drawbox=y=(256-16):w=iw:h=16:color=aqua@0.3:t=16,drawbox=w=iw:h=(256-235):color=crimson@0.3:t=16,drawgrid=width=0:height=256:thickness=1:color=white@0.5[b2];[a2][b2]framepack",
        },
    },
    {
        "Line Select",
        {
            { Args_Type_Slider,   1,   1,   0,   1, "Line" },
            { Args_Type_Slider, 255,   1, 255,   1, "Brightness" },
            { Args_Type_Toggle,   0,   0,   0,   0, "Columns"},
            { Args_Type_Toggle,   0,   0,   0,   0, "Background"},
            { Args_Type_None,     0,   0,   0,   0, },
        },
        {
            "crop=iw:2:0:${1},histogram=step=${2}:mode=waveform:waveform_mode=column:display_mode=overlay:waveform_mirror=1,drawbox=y=(256-16):w=iw:h=16:color=aqua@0.3:t=16,drawbox=w=iw:h=(256-235):color=crimson@0.3:t=16",
            "split[a][b];[a]crop=iw:2:0:${1},histogram=step=${2}:mode=waveform:waveform_mode=column:display_mode=overlay:waveform_mirror=1,drawbox=y=(256-16):w=iw:h=16:color=aqua@0.3:t=16,drawbox=w=iw:h=(256-235):color=crimson@0.3:t=16,scale=iw:${height},drawbox=y=${1}:w=iw:h=2:color=yellow,setsar=1/1[a1];[b]setsar=1/1[b1];[a1][b1]blend=addition",
            "transpose=0,crop=iw:2:0:${1},histogram=step=${2}:mode=waveform:waveform_mode=column:display_mode=overlay:waveform_mirror=1,drawbox=y=(256-16):w=iw:h=16:color=aqua@0.3:t=16,drawbox=w=iw:h=(256-235):color=crimson@0.3:t=16,transpose=1",
            "split[a][b];[a]transpose=0,crop=iw:2:0:${1},histogram=step=${2}:mode=waveform:waveform_mode=column:display_mode=overlay:waveform_mirror=1,drawbox=y=(256-16):w=iw:h=16:color=aqua@0.3:t=16,drawbox=w=iw:h=(256-235):color=crimson@0.3:t=16,transpose=1,scale=${width}:ih,drawbox=x=${1}:w=2:h=ih:color=yellow,setsar=1/1[a1];[b]lutyuv=y=val/2,setsar=1/1[b1];[a1][b1]blend=addition",
        },
    },
    {
        "Vectorscope",
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field Split" },
            { Args_Type_Slider,   5,   1,  10,   1, "Brightness" },
            { Args_Type_None,     0,   0,   0,   0, },
            { Args_Type_None,     0,   0,   0,   0, },
            { Args_Type_None,     0,   0,   0,   0, },
        },
        {
            "histogram=mode=color2,lutyuv=y=val*${2},transpose=dir=2,scale=512:512,drawgrid=w=32:h=32:t=1:c=white@0.1,drawgrid=w=256:h=256:t=1:c=white@0.2,drawbox=w=9:h=9:t=1:x=180-3:y=512-480-5:c=red@0.6,drawbox=w=9:h=9:t=1:x=108-3:y=512-68-5:c=green@0.6,drawbox=w=9:h=9:t=1:x=480-3:y=512-220-5:c=blue@0.6,drawbox=w=9:h=9:t=1:x=332-3:y=512-32-5:c=cyan@0.6,drawbox=w=9:h=9:t=1:x=404-3:y=512-444-5:c=magenta@0.6,drawbox=w=9:h=9:t=1:x=32-3:y=512-292-5:c=yellow@0.6,drawbox=w=9:h=9:t=1:x=199-3:y=512-424-5:c=red@0.8,drawbox=w=9:h=9:t=1:x=145-3:y=512-115-5:c=green@0.8,drawbox=w=9:h=9:t=1:x=424-3:y=512-229-5:c=blue@0.8,drawbox=w=9:h=9:t=1:x=313-3:y=512-88-5:c=cyan@0.8,drawbox=w=9:h=9:t=1:x=367-3:y=512-397-5:c=magenta@0.8,drawbox=w=9:h=9:t=1:x=88-3:y=512-283-5:c=yellow@0.8,drawbox=w=9:h=9:t=1:x=128-3:y=512-452-5:c=sienna@0.8,drawbox=w=9:h=9:t=1:x=160-3:y=512-404-5:c=sienna@0.8,drawbox=w=9:h=9:t=1:x=192-3:y=512-354-5:c=sienna@0.8,drawbox=w=9:h=9:t=1:x=224-3:y=512-304-5:c=sienna@0.8,pad=ih*${dar}:ih:(ow-iw)/2:(oh-ih)/2",
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]histogram=mode=color2,lutyuv=y=val*${2},transpose=dir=2,scale=512:512,drawgrid=w=32:h=32:t=1:c=white@0.1,drawgrid=w=256:h=256:t=1:c=white@0.2,drawbox=w=9:h=9:t=1:x=180-3:y=512-480-5:c=red@0.6,drawbox=w=9:h=9:t=1:x=108-3:y=512-68-5:c=green@0.6,drawbox=w=9:h=9:t=1:x=480-3:y=512-220-5:c=blue@0.6,drawbox=w=9:h=9:t=1:x=332-3:y=512-32-5:c=cyan@0.6,drawbox=w=9:h=9:t=1:x=404-3:y=512-444-5:c=magenta@0.6,drawbox=w=9:h=9:t=1:x=32-3:y=512-292-5:c=yellow@0.6,drawbox=w=9:h=9:t=1:x=199-3:y=512-424-5:c=red@0.8,drawbox=w=9:h=9:t=1:x=145-3:y=512-115-5:c=green@0.8,drawbox=w=9:h=9:t=1:x=424-3:y=512-229-5:c=blue@0.8,drawbox=w=9:h=9:t=1:x=313-3:y=512-88-5:c=cyan@0.8,drawbox=w=9:h=9:t=1:x=367-3:y=512-397-5:c=magenta@0.8,drawbox=w=9:h=9:t=1:x=88-3:y=512-283-5:c=yellow@0.8,drawbox=w=9:h=9:t=1:x=128-3:y=512-452-5:c=sienna@0.8,drawbox=w=9:h=9:t=1:x=160-3:y=512-404-5:c=sienna@0.8,drawbox=w=9:h=9:t=1:x=192-3:y=512-354-5:c=sienna@0.8,drawbox=w=9:h=9:t=1:x=224-3:y=512-304-5:c=sienna@0.8[a2];[b1]histogram=mode=color2,lutyuv=y=val*${2},transpose=dir=2,scale=512:512,drawgrid=w=32:h=32:t=1:c=white@0.1,drawgrid=w=256:h=256:t=1:c=white@0.2,drawbox=w=9:h=9:t=1:x=180-3:y=512-480-5:c=red@0.6,drawbox=w=9:h=9:t=1:x=108-3:y=512-68-5:c=green@0.6,drawbox=w=9:h=9:t=1:x=480-3:y=512-220-5:c=blue@0.6,drawbox=w=9:h=9:t=1:x=332-3:y=512-32-5:c=cyan@0.6,drawbox=w=9:h=9:t=1:x=404-3:y=512-444-5:c=magenta@0.6,drawbox=w=9:h=9:t=1:x=32-3:y=512-292-5:c=yellow@0.6,drawbox=w=9:h=9:t=1:x=199-3:y=512-424-5:c=red@0.8,drawbox=w=9:h=9:t=1:x=145-3:y=512-115-5:c=green@0.8,drawbox=w=9:h=9:t=1:x=424-3:y=512-229-5:c=blue@0.8,drawbox=w=9:h=9:t=1:x=313-3:y=512-88-5:c=cyan@0.8,drawbox=w=9:h=9:t=1:x=367-3:y=512-397-5:c=magenta@0.8,drawbox=w=9:h=9:t=1:x=88-3:y=512-283-5:c=yellow@0.8,drawbox=w=9:h=9:t=1:x=128-3:y=512-452-5:c=sienna@0.8,drawbox=w=9:h=9:t=1:x=160-3:y=512-404-5:c=sienna@0.8,drawbox=w=9:h=9:t=1:x=192-3:y=512-354-5:c=sienna@0.8,drawbox=w=9:h=9:t=1:x=224-3:y=512-304-5:c=sienna@0.8[b2];[a2][b2]framepack=tab,pad=ih*${dar}:ih:(ow-iw)/2:(oh-ih)/2",
        },
    },
    {
        "Extract Planes UV Equal.",
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field Split" },
            { Args_Type_Slider,   2,   0,  10,  10, "Strength" },
            { Args_Type_Slider,   2,   0,  10,  10, "Intensity" },
            { Args_Type_None,     0,   0,   0,   0, },
            { Args_Type_None,     0,   0,   0,   0, },
        },
        {
            "format=yuv422p|yuv420p|yuv444p|yuv410p,extractplanes=u+v,framepack,histeq=strength=${2}:intensity=${3}",
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]format=yuv422p|yuv420p|yuv444p|yuv410p,extractplanes=u+v,framepack,histeq=strength=${2}:strength=${3}[a2];[b1]format=yuv422p|yuv420p|yuv444p|yuv410p,extractplanes=u+v,framepack,histeq=strength=${2}:intensity=${3}[b2];[a2][b2]framepack=tab",
        },
    },
    {
        "Extract Planes Equalized",
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field Split" },
            { Args_Type_Yuv,      2,   0,   0,   0, "Plane"},
            { Args_Type_Slider,   2,   0,  10,  10, "Strength" },
            { Args_Type_Slider,   2,   0,  10,  10, "Intensity" },
            { Args_Type_None,     0,   0,   0,   0, },
        },
        {
            "format=yuv422p|yuv420p|yuv444p|yuv410p,extractplanes=${2},histeq=strength=${3}:intensity=${4}",
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]format=yuv422p|yuv420p|yuv444p|yuv410p,extractplanes=${2},histeq=strength=${3}:strength=${4}[a2];[b1]format=yuv422p|yuv420p|yuv444p|yuv410p,extractplanes=${2},histeq=strength=${3}:intensity=${4}[b2];[a2][b2]framepack=tab",
        },
    },
    {
        "Bit Plane",
        {
            { Args_Type_Slider,   1,  -1,   8,   1, "Y bit position" },
            { Args_Type_Slider,   -1, -1,   8,   1, "U bit position" },
            { Args_Type_Slider,   -1, -1,   8,   1, "V bit position" },
            { Args_Type_None,     0,   0,   0,   0, },
            { Args_Type_None,     0,   0,   0,   0, },
        },
        {
            "lutyuv=y=if(eq(${1}\\,-1)\\,128\\,if(eq(${1}\\,0)\\,val\\,bitand(val\\,pow(2\\,8-${1}))*pow(2\\,${1}))):u=if(eq(${2}\\,-1)\\,128\\,if(eq(${2}\\,0)\\,val\\,bitand(val\\,pow(2\\,8-${2}))*pow(2\\,${2}))):v=if(eq(${3}\\,-1)\\,128\\,if(eq(${3}\\,0)\\,val\\,bitand(val\\,pow(2\\,8-${3}))*pow(2\\,${3})))",
        },
    },
    {
        "Value Highlight",
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field Split" },
            { Args_Type_Yuv,      0,   0,   0,   0, "Plane" },
            { Args_Type_Slider, 235,   0, 255,   1, "Min"},
            { Args_Type_Slider, 255,   0, 255,   1, "Max"},
            { Args_Type_ClrPck, 0xFFFF00,   0,   0,   0, ""},
        },
        {
            "extractplanes=${2},lutrgb=r=if(between(val\\,${3}\\,${4})\\,${5R}\\,val):g=if(between(val\\,${3}\\,${4})\\,${5G}\\,val):b=if(between(val\\,${3}\\,${4})\\,${5B}\\,val)",
            "extractplanes=${2},split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]lutrgb=r=if(between(val\\,${3}\\,${4})\\,${5R}\\,val):g=if(between(val\\,${3}\\,${4})\\,${5G}\\,val):b=if(between(val\\,${3}\\,${4})\\,${5B}\\,val)[a2];[b1]lutrgb=r=if(between(val\\,${3}\\,${4})\\,${5R}\\,val):g=if(between(val\\,${3}\\,${4})\\,${5G}\\,val):b=if(between(val\\,${3}\\,${4})\\,${5B}\\,val)[b2];[a2][b2]framepack=tab",
        },
    },
    {
        "Chroma Adjust",
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field Split" },
            { Args_Type_Slider,   0,   0, 360,   1, "Hue"},
            { Args_Type_Slider,   1, -10,  10,   1, "Saturation"},
            { Args_Type_None,     0,   0,   0,   0, },
            { Args_Type_None,     0,   0,   0,   0, },
        },
        {
            "hue=h=${2}:s=${3}",
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]hue=h=${2}:s=${3}[a2];[b1]hue=h=${2}:s=${3}[b2];[a2][b2]framepack=tab",
        },
    },
    {
        "Broadcast Range Pixels",
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field Split" },
            { Args_Type_ClrPck, 0xFFFF00,   0,   0,   0, ""},
            { Args_Type_None,     0,   0,   0,   0, },
            { Args_Type_None,     0,   0,   0,   0, },
            { Args_Type_None,     0,   0,   0,   0, },
        },
        {
            "signalstats=out=brng:c=${2}",
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]signalstats=out=brng:c=${2}[a2];[b1]signalstats=out=brng:c=${2}[b2];[a2][b2]framepack=tab",
        },
    },
    {
        "Temporal Outlier Pixels",
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field Split" },
            { Args_Type_ClrPck, 0xFFFF00,   0,   0,   0, ""},
            { Args_Type_None,     0,   0,   0,   0, },
            { Args_Type_None,     0,   0,   0,   0, },
            { Args_Type_None,     0,   0,   0,   0, },
        },
        {
            "signalstats=out=tout:c=${2}",
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]signalstats=out=tout:c=${2}[a2];[b1]signalstats=out=tout:c=${2}[b2];[a2][b2]framepack=tab",
        },
    },
    {
        "Vertical Repetition Pixels",
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field Split" },
            { Args_Type_ClrPck, 0xFFFF00,   0,   0,   0, ""},
            { Args_Type_None,     0,   0,   0,   0, },
            { Args_Type_None,     0,   0,   0,   0, },
            { Args_Type_None,     0,   0,   0,   0, },
        },
        {
            "signalstats=out=vrep:c=${2}",
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]signalstats=out=vrep:c=${2}[a2];[b1]signalstats=out=vrep:c=${2}[b2];[a2][b2]framepack=tab",
        },
    },
    /*
    {
        "Tile",
        {
            { Args_Type_Tile,     1,   0,   0,   0, "Frames" },
            { Args_Type_None,     0,   0,   0,   0, },
            { Args_Type_None,     0,   0,   0,   0, },
            { Args_Type_None,     0,   0,   0,   0, },
            { Args_Type_None,     0,   0,   0,   0, },
        },
        {
            "scale=iw/${1}:ih/${1}"
        },
    },
    */
    /*
    {
        "Zoom",
        {
            { Args_Type_Slider,   0,   0,   0,   1, "x" },
            { Args_Type_Slider,   0,   0,   0,   1, "y" },
            { Args_Type_Slider,  12,   0,   0,   1, "s" },
            { Args_Type_Slider,   0,   0,  10,  10, "Strength" },
            { Args_Type_Slider,   0,   0,  10,  10, "Intensity" },
        },
        {
            "crop=${3}:${3}/dar:${1}:${2},histeq=strength=${4}:intensity=${5}",
        },
    },
    */
    {
        "(End)",
        {
            { Args_Type_None,   0, 0, 0, 0, },
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
// Helper
//***************************************************************************

//---------------------------------------------------------------------------
DoubleSpinBoxWithSlider::DoubleSpinBoxWithSlider(DoubleSpinBoxWithSlider** Others_, int Min_, int Max_, int Divisor_, int Current, const char* Name, BigDisplay* Display_, size_t Pos_, bool IsBitSlice_, QWidget *parent) :
    Others(Others_),
    Divisor(Divisor_),
    Min(Min_),
    Max(Max_),
    Display(Display_),
    Pos(Pos_),
    IsBitSlice(IsBitSlice_),
    QDoubleSpinBox(parent)
{
    Popup=NULL;
    Slider=NULL;

    setMinimum(((double)Min)/Divisor);
    setMaximum(((double)Max)/Divisor);
    setValue(((double)Current)/Divisor);
    setToolTip(Name);
    if (Divisor<=1)
    {
        setDecimals(0);
        setSingleStep(1);
    }
    else if (Divisor<=10)
    {
        setDecimals(1);
        setSingleStep(0.1);
    }

    if (IsBitSlice)
        setPrefix("Bit ");

    QFont Font;
    #ifdef _WIN32
    #else //_WIN32
        Font.setPointSize(Font.pointSize()*3/4);
    #endif //_WIN32
    setFont(Font);

    setFocusPolicy(Qt::NoFocus);
}

//---------------------------------------------------------------------------
DoubleSpinBoxWithSlider::~DoubleSpinBoxWithSlider()
{
    //delete Popup; //Popup=NULL;
    delete Slider; //Slider=NULL;
}

//---------------------------------------------------------------------------
void DoubleSpinBoxWithSlider::enterEvent (QEvent* event)
{
    if (Slider==NULL)
    {
        //Popup=new QWidget((QWidget*)parent(), Qt::Popup | Qt::Window);
        //Popup=new QWidget((QWidget*)parent(), Qt::FramelessWindowHint);
        //Popup->setGeometry(((QWidget*)parent())->geometry().x()+x()+width()-(255+30), ((QWidget*)parent())->geometry().y()+y()+height(), 255+30, height());
        //Popup->setGeometry(x()+width()-(255+30), y()+height(), 255+30, height());
        //Popup->setWindowModality(Qt::NonModal);
        //Popup->setFocusPolicy(Qt::NoFocus);
        //QLayout* Layout=new QGridLayout();
        //Layout->setContentsMargins(0, 0, 0, 0);
        //Layout->setSpacing(0);
        Slider=new QSlider(Qt::Horizontal, (QWidget*)parent());
        Slider->setFocusPolicy(Qt::NoFocus);
        Slider->setMinimum(Min);
        Slider->setMaximum(Max);
        Slider->setToolTip(toolTip());
        Slider->setGeometry(x()+width()-(255+30), y()+height(), 255+30, height());
        connect(Slider, SIGNAL(valueChanged(int)), this, SLOT(on_sliderMoved(int)));
        connect(Slider, SIGNAL(sliderMoved(int)), this, SLOT(on_sliderMoved(int)));
        Slider->setFocusPolicy(Qt::NoFocus);
        //Layout->addWidget(Slider);
        //Popup->setFocusPolicy(Qt::NoFocus);
        //Popup->setLayout(Layout);
        connect(this, SIGNAL(valueChanged(double)), this, SLOT(on_valueChanged(double)));
    }
    for (size_t Pos=0; Pos<Args_Max; Pos++)
        if (Others[Pos] && Others[Pos]!=this)
            Others[Pos]->hidePopup();
    //if (!Popup->isVisible())
    if (!Slider->isVisible())
    {
        on_valueChanged(value());
        Slider->show();
        ((QWidget*)parent())->repaint();
    }
}

//---------------------------------------------------------------------------
void DoubleSpinBoxWithSlider::ChangeMax(int Max_)
{
    Max=Max_;
    setMaximum(Max);
    if (Slider)
        Slider->setMaximum(Max);
}

//---------------------------------------------------------------------------
void DoubleSpinBoxWithSlider::leaveEvent (QEvent* event)
{
    //Popup->hide();
    //((QWidget*)parent())->repaint();
}

//---------------------------------------------------------------------------
void DoubleSpinBoxWithSlider::keyPressEvent (QKeyEvent* event)
{
    if (event->key()==Qt::Key_Up || event->key()==Qt::Key_Down)
    {
        QDoubleSpinBox::keyPressEvent(event);
    }

    event->ignore();
}

//---------------------------------------------------------------------------
void DoubleSpinBoxWithSlider::moveEvent (QMoveEvent* event)
{
    if (Slider)
        //Popup->setGeometry(x()+width()-(255+30), y()+height(), 255+30, height());
        Slider->setGeometry(x()+width()-(255+30), y()+height(), 255+30, height());
}

//---------------------------------------------------------------------------
void DoubleSpinBoxWithSlider::hidePopup ()
{
    //if (Popup)
    //    Popup->hide();
    if (Slider)
        Slider->hide();
}

//---------------------------------------------------------------------------
void DoubleSpinBoxWithSlider::on_valueChanged (double value)
{
    if (IsBitSlice)
    {
        if (value<1)
            setPrefix(QString());
        else
            setPrefix("Bit ");
    }

    if (Slider)
    {
        double Value=value*Divisor;
        int ValueInt=(int)Value;
        if(Value-0.5>=ValueInt)
            ValueInt++;
        Slider->setValue(Value);
    }
}

//---------------------------------------------------------------------------
void DoubleSpinBoxWithSlider::on_sliderMoved (int value)
{
    setValue(((double)value)/Divisor);
}

//---------------------------------------------------------------------------
void DoubleSpinBoxWithSlider::showEvent (QShowEvent* event)
{
    if (IsBitSlice)
    {
        if (value()<1)
            setPrefix(QString());
        else
            setPrefix("Bit ");
    }

    QDoubleSpinBox::showEvent(event);
}

//---------------------------------------------------------------------------
QString DoubleSpinBoxWithSlider::textFromValue (double value) const
{
    if (IsBitSlice && value==0)
        return "All";
    else if (IsBitSlice && value==-1)
        return "None";
    else
        return QDoubleSpinBox::textFromValue(value);
}

//---------------------------------------------------------------------------
double DoubleSpinBoxWithSlider::valueFromText (const QString& text) const
{
    if (IsBitSlice && text=="All")
        return -1;
    else if (IsBitSlice && text=="None")
        return 0;
    else
        return QDoubleSpinBox::valueFromText(text);
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
    setWindowFlags(windowFlags() | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint);
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
        for (size_t OptionPos=0; OptionPos<Args_Max; OptionPos++)
        {
            Options[Pos].Checks[OptionPos]=NULL;
            Options[Pos].Sliders_Label[OptionPos]=NULL;
            Options[Pos].Sliders_SpinBox[OptionPos]=NULL;
            for (size_t OptionPos2=0; OptionPos2<4; OptionPos2++)
                Options[Pos].Radios[OptionPos][OptionPos2]=NULL;
            Options[Pos].Radios_Group[OptionPos]=NULL;
            Options[Pos].ColorButton[OptionPos]=NULL;
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
    Layout->setSpacing(0);

    // Filters
    for (size_t Pos=0; Pos<2; Pos++)
    {
        Options[Pos].FiltersList=new QComboBox(this);
        Options[Pos].FiltersList->setFont(Font);
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
    Image1->setMinimumSize(20, 20);
    //Layout->addWidget(Image1, 1, 0, 1, 1);
    //Layout->setColumnStretch(0, 1);

    //Image2
    Image2=new ImageLabel(&Picture, 2, this);
    Image2->IsMain=false;
    Image2->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    Image2->setMinimumSize(20, 20);
    //Layout->addWidget(Image2, 1, 2, 1, 1);
    //Layout->setColumnStretch(2, 1);

    // Mixing Image1 and Image2 in one widget
    QHBoxLayout* ImageLayout=new QHBoxLayout();
    ImageLayout->setContentsMargins(0, -1, 0, 0);
    ImageLayout->setSpacing(0);
    ImageLayout->addWidget(Image1);
    ImageLayout->addWidget(Image2);
    Layout->addLayout(ImageLayout, 1, 0, 1, 3);

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
    ControlArea=new Control(this, FileInfoData, NULL, Control::Style_Cols, true);
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
    for (size_t OptionPos=0; OptionPos<Args_Max; OptionPos++)
    {
        delete Options[Pos].Checks[OptionPos]; Options[Pos].Checks[OptionPos]=NULL;
        delete Options[Pos].Sliders_Label[OptionPos]; Options[Pos].Sliders_Label[OptionPos]=NULL;
        delete Options[Pos].Sliders_SpinBox[OptionPos]; Options[Pos].Sliders_SpinBox[OptionPos]=NULL;
        for (size_t OptionPos2=0; OptionPos2<4; OptionPos2++)
        {
            delete Options[Pos].Radios[OptionPos][OptionPos2]; Options[Pos].Radios[OptionPos][OptionPos2]=NULL;
        }
        delete Options[Pos].Radios_Group[OptionPos]; Options[Pos].Radios_Group[OptionPos]=NULL;
        delete Options[Pos].ColorButton[OptionPos]; Options[Pos].ColorButton[OptionPos]=NULL;
    }
    delete Options[Pos].FiltersList_Fake; Options[Pos].FiltersList_Fake=NULL;
    Layout0->setContentsMargins(0, 0, 0, 0);
    QFont Font=QFont();
    #ifdef _WIN32
    #else //_WIN32
        Font.setPointSize(Font.pointSize()*3/4);
    #endif //_WIN32
    size_t Widget_XPox=1;
    for (size_t OptionPos=0; OptionPos<Args_Max; OptionPos++)
    {
        if (FilterPos>=PreviousValues[Pos].size())
            PreviousValues[Pos].resize(FilterPos+1);
        if (PreviousValues[Pos][FilterPos].Values[OptionPos]==-2)
        {
            PreviousValues[Pos][FilterPos].Values[OptionPos]=Filters[FilterPos].Args[OptionPos].Default;

            //Special case : default for Line Select is Height/2
            if (Filters[FilterPos].Args[OptionPos].Name && string(Filters[FilterPos].Args[OptionPos].Name)=="Line")
                PreviousValues[Pos][FilterPos].Values[OptionPos]=FileInfoData->Glue->Height_Get()/2;
        }

        switch (Filters[FilterPos].Args[OptionPos].Type)
        {
            case Args_Type_Toggle:
                                    {
                                    Options[Pos].Checks[OptionPos]=new QCheckBox(Filters[FilterPos].Args[OptionPos].Name);
                                    Options[Pos].Checks[OptionPos]->setFont(Font);
                                    Options[Pos].Checks[OptionPos]->setChecked(PreviousValues[Pos][FilterPos].Values[OptionPos]);
                                    connect(Options[Pos].Checks[OptionPos], SIGNAL(stateChanged(int)), this, Pos==0?(SLOT(on_FiltersOptions1_click())):SLOT(on_FiltersOptions2_click()));
                                    Layout0->addWidget(Options[Pos].Checks[OptionPos], 0, Widget_XPox);
                                    Widget_XPox++;
                                    }
                                    break;
            case Args_Type_Slider:
                                    {
                                    // Special case: "Line", max is source width or height
                                    int Max;
                                    string MaxTemp(Filters[FilterPos].Args[OptionPos].Name);
                                    if (MaxTemp=="Line")
                                    {
                                        bool SelectWidth=false;
                                        for (size_t OptionPos2=0; OptionPos2<Args_Max; OptionPos2++)
                                            if (Filters[FilterPos].Args[OptionPos2].Type!=Args_Type_None && string(Filters[FilterPos].Args[OptionPos2].Name)=="Columns")
                                                SelectWidth=Filters[FilterPos].Args[OptionPos2].Default?true:false;
                                        Max=SelectWidth?FileInfoData->Glue->Width_Get():FileInfoData->Glue->Height_Get();
                                    }
                                    else if (MaxTemp=="x" || MaxTemp=="s")
                                        Max=FileInfoData->Glue->Width_Get();
                                    else if (MaxTemp=="y")
                                        Max=FileInfoData->Glue->Height_Get();
                                    else
                                        Max=Filters[FilterPos].Args[OptionPos].Max;

                                    Options[Pos].Sliders_Label[OptionPos]=new QLabel(Filters[FilterPos].Args[OptionPos].Name+QString(": "));
                                    Options[Pos].Sliders_SpinBox[OptionPos]=new DoubleSpinBoxWithSlider(Options[Pos].Sliders_SpinBox, Filters[FilterPos].Args[OptionPos].Min, Max, Filters[FilterPos].Args[OptionPos].Divisor, PreviousValues[Pos][FilterPos].Values[OptionPos], Filters[FilterPos].Args[OptionPos].Name, this, Pos, QString(Filters[FilterPos].Args[OptionPos].Name).contains(" bit position"), this);
                                    connect(Options[Pos].Sliders_SpinBox[OptionPos], SIGNAL(valueChanged(double)), this, Pos==0?(SLOT(on_FiltersSpinBox1_click())):SLOT(on_FiltersSpinBox2_click()));
                                    Options[Pos].Sliders_Label[OptionPos]->setFont(Font);
                                    if (Options[Pos].Sliders_SpinBox[OptionPos])
                                    {
                                        Layout0->addWidget(Options[Pos].Sliders_Label[OptionPos], 0, Widget_XPox, 1, 1, Qt::AlignRight);
                                        Layout0->addWidget(Options[Pos].Sliders_SpinBox[OptionPos], 0, Widget_XPox+1);
                                    }
                                    else
                                        Layout0->addWidget(Options[Pos].Sliders_Label[OptionPos], 1, Widget_XPox, 1, 2);
                                    Widget_XPox+=2;
                                    }
                                    break;
            case Args_Type_Yuv:
            case Args_Type_YuvA:
            case Args_Type_Tile:
                                    Options[Pos].Radios_Group[OptionPos]=new QButtonGroup();
                                    for (size_t OptionPos2=0; OptionPos2<(Filters[FilterPos].Args[OptionPos].Type==Args_Type_Yuv?3:4); OptionPos2++)
                                    {
                                        Options[Pos].Radios[OptionPos][OptionPos2]=new QRadioButton();
                                        Options[Pos].Radios[OptionPos][OptionPos2]->setFont(Font);
                                        if (Filters[FilterPos].Args[OptionPos].Type==Args_Type_Tile)
                                            switch (OptionPos2)
                                            {
                                                case 0: Options[Pos].Radios[OptionPos][OptionPos2]->setText("4x4"); break;
                                                case 1: Options[Pos].Radios[OptionPos][OptionPos2]->setText("6x6"); break;
                                                case 2: Options[Pos].Radios[OptionPos][OptionPos2]->setText("8x8"); break;
                                                case 3: Options[Pos].Radios[OptionPos][OptionPos2]->setText("10x10"); break;
                                                default:;
                                            }
                                        else
                                            switch (OptionPos2)
                                            {
                                                case 0: Options[Pos].Radios[OptionPos][OptionPos2]->setText("Y"); break;
                                                case 1: Options[Pos].Radios[OptionPos][OptionPos2]->setText("U"); break;
                                                case 2: Options[Pos].Radios[OptionPos][OptionPos2]->setText("V"); break;
                                                case 3: Options[Pos].Radios[OptionPos][OptionPos2]->setText("A"); break;
                                                default:;
                                            }
                                        if (OptionPos2==PreviousValues[Pos][FilterPos].Values[OptionPos])
                                            Options[Pos].Radios[OptionPos][OptionPos2]->setChecked(true);
                                        connect(Options[Pos].Radios[OptionPos][OptionPos2], SIGNAL(toggled(bool)), this, Pos==0?(SLOT(on_FiltersOptions1_toggle(bool))):SLOT(on_FiltersOptions2_toggle(bool)));
                                        Layout0->addWidget(Options[Pos].Radios[OptionPos][OptionPos2], 0, Widget_XPox+OptionPos2);
                                        Options[Pos].Radios_Group[OptionPos]->addButton(Options[Pos].Radios[OptionPos][OptionPos2]);
                                    }
                                    Widget_XPox+=Filters[FilterPos].Args[OptionPos].Type==Args_Type_Yuv?3:4;
                                    break;
            case Args_Type_ClrPck:
                                    {
                                    Options[Pos].ColorValue[OptionPos]=PreviousValues[Pos][FilterPos].Values[OptionPos];
                                    Options[Pos].ColorButton[OptionPos]=new QPushButton("Color");
                                    Options[Pos].ColorButton[OptionPos]->setFont(Font);
                                    Options[Pos].ColorButton[OptionPos]->setMaximumHeight(Options[Pos].FiltersList->height());
                                    connect(Options[Pos].ColorButton[OptionPos], SIGNAL(clicked (bool)), this, Pos==0?(SLOT(on_Color1_click(bool))):SLOT(on_Color2_click(bool)));
                                    Layout0->addWidget(Options[Pos].ColorButton[OptionPos], 0, Widget_XPox);
                                    Widget_XPox++;
                                    }
                                    break;
            default:                ;
        }
    }

    // Reorder focus order
    // Does not work as expected
    /*
    if (Pos==1) //Only if left and right blocks are designed
    {
        QWidget* First=Options[0].FiltersList;
        QWidget* Second=NULL;
        for (Pos=0; Pos<2; Pos++)
            for (size_t OptionPos=0; OptionPos<Args_Max; OptionPos++)
            {
                Second=Options[Pos].Checks[OptionPos];
                if (Second==NULL)
                    Second=Options[Pos].Sliders_SpinBox[OptionPos];
                if (Second==NULL)
                    Second=Options[Pos].Radios[OptionPos][1];
                if (Second==NULL)
                    Second=Options[Pos].ColorButton[OptionPos];
                if (Second)
                {
                    setTabOrder(First, Second);
                    First=Second;
                    Second=NULL;
                }
            }
        setTabOrder(First, Options[1].FiltersList);
    }
    */
}

//---------------------------------------------------------------------------
void BigDisplay::FiltersList1_currentIndexChanged(size_t FilterPos)
{
    QGridLayout* Layout0=new QGridLayout();
    Layout0->setContentsMargins(0, 0, 0, 0);
    Layout0->setSpacing(8);
    Layout0->addWidget(Options[0].FiltersList, 0, 0, Qt::AlignLeft);
    FiltersList_currentIndexChanged(0, FilterPos, Layout0);
    Options[0].FiltersList_Fake=new QLabel(" ");
    Options[0].FiltersList_Fake->setMinimumHeight(24);
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
    Layout0->setContentsMargins(0, 0, 0, 0);
    Layout0->setSpacing(8);
    Layout0->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum), 0, 0);
    FiltersList_currentIndexChanged(1, FilterPos, Layout0);
    Layout0->addWidget(Options[1].FiltersList, 0, 14, Qt::AlignRight);
    Options[1].FiltersList_Fake=new QLabel(" ");
    Options[1].FiltersList_Fake->setMinimumHeight(24);
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
    bool Modified=false;
    double WithSliders[Args_Max];
    WithSliders[0]=-2;
    WithSliders[1]=-2;
    WithSliders[2]=-2;
    WithSliders[3]=-2;
    string WithRadios[Args_Max];
    string Modified_String;
    for (size_t OptionPos=0; OptionPos<Args_Max; OptionPos++)
    {
        switch (Filters[Picture_Current].Args[OptionPos].Type)
        {
            case Args_Type_Toggle:
                                    Value_Pos<<=1;
                                    Value_Pos|=(Options[Pos].Checks[OptionPos]->isChecked()?1:0);
                                    PreviousValues[Pos][Picture_Current].Values[OptionPos]=Options[Pos].Checks[OptionPos]->isChecked()?1:0;

                                    // Special case: "Line", max is source width or height
                                    if (string(Filters[Picture_Current].Args[OptionPos].Name)=="Columns")
                                        for (size_t OptionPos2=0; OptionPos2<Args_Max; OptionPos2++)
                                            if (Filters[Picture_Current].Args[OptionPos2].Type!=Args_Type_None && string(Filters[Picture_Current].Args[OptionPos2].Name)=="Line")
                                            {
                                                Options[Pos].Sliders_SpinBox[OptionPos2]->ChangeMax(Options[Pos].Checks[OptionPos]->isChecked()?FileInfoData->Glue->Width_Get():FileInfoData->Glue->Height_Get());
                                                // Infinite loop in that case
                                                //Options[Pos].Sliders_SpinBox[OptionPos2]->setValue(Options[Pos].Sliders_SpinBox[OptionPos2]->maximum()/2);
                                            }

                                    // Special case: RGB
                                    if (string(Filters[Picture_Current].Name)=="Histogram" && Options[Pos].Checks[1])
                                    {
                                        if (Options[Pos].Checks[1]->isChecked() && Options[Pos].Radios[2][0]->text()!="R") //RGB
                                        {
                                            Options[Pos].Radios[2][0]->setText("R");
                                            Options[Pos].Radios[2][1]->setText("G");
                                            Options[Pos].Radios[2][2]->setText("B");
                                            Options[Pos].Radios[2][3]->setChecked(true);
                                        }
                                        if (!Options[Pos].Checks[1]->isChecked() && Options[Pos].Radios[2][0]->text()!="Y") //YUV
                                        {
                                            Options[Pos].Radios[2][0]->setText("Y");
                                            Options[Pos].Radios[2][1]->setText("U");
                                            Options[Pos].Radios[2][2]->setText("V");
                                            Options[Pos].Radios[2][3]->setChecked(true);
                                        }
                                    }

                                    break;
            case Args_Type_Slider:
                                    Modified=true;
                                    WithSliders[OptionPos]=Options[Pos].Sliders_SpinBox[OptionPos]->value();
                                    PreviousValues[Pos][Picture_Current].Values[OptionPos]=Options[Pos].Sliders_SpinBox[OptionPos]->value();
                                    break;
            case Args_Type_YuvA:
                                    Value_Pos<<=1;
                                    Value_Pos|=Options[Pos].Radios[OptionPos][3]->isChecked()?1:0; // 3 = pos of "all"
                                    //No break
            case Args_Type_Yuv:
            case Args_Type_Tile:
                                    Modified=true;
                                    for (size_t OptionPos2=0; OptionPos2<(Filters[Picture_Current].Args[OptionPos].Type?4:3); OptionPos2++)
                                    {
                                        if (Options[Pos].Radios[OptionPos][OptionPos2] && Options[Pos].Radios[OptionPos][OptionPos2]->isChecked())
                                        {
                                            if (Filters[Picture_Current].Args[OptionPos].Type==Args_Type_Tile)
                                            {
                                                WithRadios[OptionPos]=Options[Pos].Radios[OptionPos][OptionPos2]->text().toUtf8().data();
                                                size_t X_Pos=WithRadios[OptionPos].find('x');
                                                if (X_Pos!=string::npos)
                                                    WithRadios[OptionPos].resize(X_Pos);
                                            }
                                            else if (string(Filters[Picture_Current].Name)=="Waveform")
                                                switch (OptionPos2)
                                                {
                                                    case 0: WithRadios[OptionPos]="0"; break;
                                                    case 1: WithRadios[OptionPos]="256"; break;
                                                    case 2: WithRadios[OptionPos]="512"; break;
                                                    case 3: WithRadios[OptionPos]="all"; break; //Special case: remove plane
                                                    default:;
                                                }
                                            else if (string(Filters[Picture_Current].Name)=="Histogram" && Options[Pos].Checks[1] && Options[Pos].Checks[1]->isChecked()) //RGB
                                                switch (OptionPos2)
                                                {
                                                    case 0: WithRadios[OptionPos]="r"; break;
                                                    case 1: WithRadios[OptionPos]="g"; break;
                                                    case 2: WithRadios[OptionPos]="b"; break;
                                                    case 3: WithRadios[OptionPos]="all"; break; //Special case: remove plane
                                                    default:;
                                                }
                                            else
                                                switch (OptionPos2)
                                                {
                                                    case 0: WithRadios[OptionPos]="y"; break;
                                                    case 1: WithRadios[OptionPos]="u"; break;
                                                    case 2: WithRadios[OptionPos]="v"; break;
                                                    case 3: WithRadios[OptionPos]="all"; break; //Special case: remove plane
                                                    default:;
                                                }
                                            PreviousValues[Pos][Picture_Current].Values[OptionPos]=OptionPos2;
                                            break;
                                        }
                                    }
                                    break;
            case Args_Type_ClrPck:
                                    Modified=true;
                                    WithSliders[OptionPos]=Options[Pos].ColorValue[OptionPos];
                                    PreviousValues[Pos][Picture_Current].Values[OptionPos]=Options[Pos].ColorValue[OptionPos];
                                    break ;
            default:                ;
        }
    }

    Modified_String=Filters[Picture_Current].Formula[Value_Pos];
    if (Modified)
    {
        for (size_t OptionPos=0; OptionPos<Args_Max; OptionPos++)
            switch (Filters[Picture_Current].Args[OptionPos].Type)
            {
                case Args_Type_Slider:
                                        {
                                        char ToFind1[3];
                                        ToFind1[0]='$';
                                        ToFind1[1]='1'+OptionPos;
                                        ToFind1[2]='\0';
                                        char ToFind2[5];
                                        ToFind2[0]='$';
                                        ToFind2[1]='{';
                                        ToFind2[2]='1'+OptionPos;
                                        ToFind2[3]='}';
                                        ToFind2[4]='\0';
                                        for (;;)
                                        {
                                            size_t InsertPos=Modified_String.find(ToFind2);
                                            size_t BytesCount=4;
                                            if (InsertPos==string::npos)
                                            {
                                                InsertPos=Modified_String.find(ToFind1);
                                                if (InsertPos!=string::npos)
                                                    BytesCount=2;
                                            }
                                            if (InsertPos==string::npos)
                                                break;
                                            Modified_String.erase(InsertPos, BytesCount);
                                            Modified_String.insert(InsertPos, QString::number(WithSliders[OptionPos]).toUtf8());
                                        }
                                        }
                                        break;
                case Args_Type_Yuv:
                case Args_Type_YuvA:
                case Args_Type_Tile:
                case Args_Type_ClrPck:
                                        {
                                        char ToFind1[3];
                                        ToFind1[0]='$';
                                        ToFind1[1]='1'+OptionPos;
                                        ToFind1[2]='\0';
                                        char ToFind2[5];
                                        ToFind2[0]='$';
                                        ToFind2[1]='{';
                                        ToFind2[2]='1'+OptionPos;
                                        ToFind2[3]='}';
                                        ToFind2[4]='\0';
                                        for (;;)
                                        {
                                            size_t InsertPos=Modified_String.find(ToFind2);
                                            size_t BytesCount=4;
                                            if (InsertPos==string::npos)
                                            {
                                                InsertPos=Modified_String.find(ToFind1);
                                                if (InsertPos!=string::npos)
                                                    BytesCount=2;
                                            }
                                            if (InsertPos==string::npos)
                                            {
                                                // Special case RGB
                                                for (;;)
                                                {
                                                    char ToFind3[4];
                                                    ToFind3[0]='$';
                                                    ToFind3[1]='{';
                                                    ToFind3[2]='1'+OptionPos;
                                                    ToFind3[3]='\0';
                                                    InsertPos=Modified_String.find(ToFind3);
                                                    if (InsertPos==string::npos)
                                                        break;

                                                    if (InsertPos+6<Modified_String.size() && (Modified_String[InsertPos+3]=='R' || Modified_String[InsertPos+3]=='G' ||Modified_String[InsertPos+3]=='B') && Modified_String[InsertPos+4]=='}')
                                                    {
                                                        int Value=(int)WithSliders[OptionPos];
                                                        switch(Modified_String[InsertPos+3])
                                                        {
                                                            case 'R' : Value>>=16;            ; break;
                                                            case 'G' : Value>>= 8; Value&=0xFF; break;
                                                            case 'B' :             Value&=0xFF; break;
                                                            default  : ;
                                                        }

                                                        Modified_String.erase(InsertPos, 5);
                                                        Modified_String.insert(InsertPos, QString::number(Value).toUtf8());
                                                    }
                                                }
                                                break;
                                            }
                                            Modified_String.erase(InsertPos, BytesCount);
                                            if (Filters[Picture_Current].Args[OptionPos].Type==Args_Type_ClrPck)
                                                Modified_String.insert(InsertPos, ("0x"+QString::number(Options[Pos].ColorValue[OptionPos], 16)).toUtf8().data());
                                            else
                                                Modified_String.insert(InsertPos, WithRadios[OptionPos]);
                                        }

                                        // Special case: removing fake "extractplanes=all,"
                                        size_t RemovePos=Modified_String.find("crop=iw:256:0:all,");
                                        if (RemovePos==string::npos)
                                            RemovePos=Modified_String.find("extractplanes=all,");
                                        if (RemovePos!=string::npos)
                                            Modified_String.erase(RemovePos, 18);
                                        }
                                        break;
                default:                ;
            }
    }

    // Variables
    Pos=Modified_String.find("${width}");
    if (Pos!=string::npos)
    {
        Modified_String.erase(Pos, 8);
        stringstream ss;
        ss<<Image_Width;
        Modified_String.insert(Pos, ss.str());
    }
    Pos=Modified_String.find("${height}");
    if (Pos!=string::npos)
    {
        Modified_String.erase(Pos, 9);
        stringstream ss;
        ss<<Image_Height;
        Modified_String.insert(Pos, ss.str());
    }
    Pos=Modified_String.find("${dar}");
    if (Pos!=string::npos)
    {
        Modified_String.erase(Pos, 6);
        stringstream ss;
        ss<<1.333;
        Modified_String.insert(Pos, ss.str());
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
        int width=QDesktopWidget().screenGeometry().width()*2/5;
        if (width%2)
            width--; //odd number is wanted for filters
        int height=QDesktopWidget().screenGeometry().height()*2/5;
        if (height%2)
            height--; //odd number is wanted for filters
        Picture=new FFmpeg_Glue(FileName_string.c_str(), width, height, FFmpeg_Glue::Output_QImage, "", "", Picture_Current1<FiltersListDefault_Count, Picture_Current2<FiltersListDefault_Count); ///*Filters[Picture_Current1].Value[0], Filters[Picture_Current2].Value[0] removed else there is random crashs in libavfilters, maybe due to bad libavfilter config
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
void BigDisplay::on_FiltersOptions1_toggle(bool checked)
{
    if (checked)
        FiltersList1_currentOptionChanged(Picture_Current1);
}

//---------------------------------------------------------------------------
void BigDisplay::on_FiltersOptions2_toggle(bool checked)
{
    if (checked)
        FiltersList2_currentOptionChanged(Picture_Current2);
}

//---------------------------------------------------------------------------
void BigDisplay::on_FiltersSpinBox1_click()
{
    FiltersList1_currentOptionChanged(Picture_Current1);
}

//---------------------------------------------------------------------------
void BigDisplay::on_FiltersSpinBox2_click()
{
    FiltersList2_currentOptionChanged(Picture_Current2);
}

//---------------------------------------------------------------------------
void BigDisplay::on_Color1_click(bool checked)
{
    QObject* Sender=sender();
    size_t OptionPos=0;
    while (Sender!=Options[0].ColorButton[OptionPos])
        OptionPos++;


    QColor Color=QColorDialog::getColor(QColor(Options[0].ColorValue[OptionPos]), this);
    if (Color.isValid())
    {
        Options[0].ColorValue[OptionPos]=Color.rgb()&0x00FFFFFF;
        FiltersList2_currentOptionChanged(Picture_Current2);
    }
    hide();
    show();
}

//---------------------------------------------------------------------------
void BigDisplay::on_Color2_click(bool checked)
{
    QObject* Sender=sender();
    size_t OptionPos=0;
    while (Sender!=Options[1].ColorButton[OptionPos])
        OptionPos++;

    QColor Color=QColorDialog::getColor(QColor(Options[1].ColorValue[OptionPos]), this);
    if (Color.isValid())
    {
        Options[1].ColorValue[OptionPos]=Color.rgb()&0x00FFFFFF;
        FiltersList2_currentOptionChanged(Picture_Current2);
    }
    hide();
    show();
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
            //Picture->Filter1_Change(Filters[Pos].Formula[0]);
            Picture->Filter1_Change(FiltersList_currentOptionChanged(Pos, 0));

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
