/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include "BigDisplay.h"
#include "SelectionArea.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "GUI/BigDisplay.h"
#include "GUI/Control.h"
#include "GUI/Info.h"
#include "GUI/Help.h"
#include "GUI/imagelabel.h"
#include "GUI/config.h"
#include "GUI/Comments.h"
#include "GUI/Plots.h"
#include "Core/FileInformation.h"
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
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <QSplitter>
#include <qwt_scale_widget.h>
#include "playerwindow.h"

#include <sstream>
//---------------------------------------------------------------------------


//***************************************************************************
// Config
//***************************************************************************

//---------------------------------------------------------------------------
// Default filters (check Filters order)
const size_t Filters_Default1 = 2; // 2 = Normal
const size_t Filters_Default2 = 5; // 5 = Waveform

//***************************************************************************
// Info
//***************************************************************************

//---------------------------------------------------------------------------
enum args_type
{
    Args_Type_None,
    Args_Type_Toggle,
    Args_Type_Slider,
    Args_Type_Win_Func,
    Args_Type_Wave_Mode,
    Args_Type_Yuv,  // Y, U , V
    Args_Type_YuvA, // Y, U, V, All
    Args_Type_Ranges, // above whites, below black
    Args_Type_ColorMatrix, // bt601, bt709, smpte240m, fcc
    Args_Type_SampleRange, // broadcast, full, auto
    Args_Type_ClrPck, // Color picker
    Args_Type_LogLin,  // Logarithmic and linear
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
    const int           Type; // 0 = Video, 1 = Audio
    const args          Args[Args_Max];
    const char*         Formula[1<<Args_Max]; //Max 2^Args_Max toggles
};


const filter Filters[]=
{
    {
        "Help",
        -1,
        {
            { Args_Type_None,   0, 0, 0, 0, nullptr },
            { Args_Type_None,   0, 0, 0, 0, nullptr },
            { Args_Type_None,   0, 0, 0, 0, nullptr },
            { Args_Type_None,   0, 0, 0, 0, nullptr },
            { Args_Type_None,   0, 0, 0, 0, nullptr },
            { Args_Type_None,   0, 0, 0, 0, nullptr },
            { Args_Type_None,   0, 0, 0, 0, nullptr },
        },
        {
            "",
        },
    },
    {
        "No Display",
        -1,
        {
            { Args_Type_None,   0, 0, 0, 0, nullptr },
            { Args_Type_None,   0, 0, 0, 0, nullptr },
            { Args_Type_None,   0, 0, 0, 0, nullptr },
            { Args_Type_None,   0, 0, 0, 0, nullptr },
            { Args_Type_None,   0, 0, 0, 0, nullptr },
            { Args_Type_None,   0, 0, 0, 0, nullptr },
            { Args_Type_None,   0, 0, 0, 0, nullptr },
        },
        {
            "",
        },
    },
    {
        "Normal",
        0,
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field" },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
            "",
            "il=l=d:c=d",
        },
    },
    {
        "(Separator)",
        -1,
        {
            { Args_Type_None,   0, 0, 0, 0, nullptr },
            { Args_Type_None,   0, 0, 0, 0, nullptr },
            { Args_Type_None,   0, 0, 0, 0, nullptr },
            { Args_Type_None,   0, 0, 0, 0, nullptr },
            { Args_Type_None,   0, 0, 0, 0, nullptr },
            { Args_Type_None,   0, 0, 0, 0, nullptr },
            { Args_Type_None,   0, 0, 0, 0, nullptr },
        },
        {
            "",
        },
    },
    {
        "Histogram",
        0,
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field" },
            { Args_Type_Toggle,   0,   0,   0,   0, "RGB" },
            { Args_Type_YuvA,     3,   0,   0,   0, "Plane" },
            { Args_Type_LogLin,   0,   0,   0,   0, "Levels" },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
            // field N, rgb, N, all planes N
            "histogram=level_height=${height}-12:components=${3}:levels_mode=${4}",
            // field N, rgb, N, all planes Y
            "histogram=level_height=${height}:levels_mode=${4}",
            // field N, rgb, Y, all planes N
            "format=rgb24,histogram=level_height=${height}:components=${3}:levels_mode=${4}",
            // field N, rgb, Y, all planes Y
            "format=rgb24,histogram=level_height=${height}:levels_mode=${4}",
            // field Y, rgb, N, all planes N
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]histogram=components=${3}:levels_mode=${4}[a2];[b1]histogram=components=${3}:levels_mode=${4}[b2];[a2][b2]vstack",
            // field Y, rgb, N, all planes Y
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]histogram=levels_mode=${4}[a2];[b1]histogram=levels_mode=${4}[b2];[a2][b2]hstack",
            // field Y, rgb, Y, all planes N
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]format=rgb24,histogram=components=${3}:levels_mode=${4}[a2];[b1]format=rgb24,histogram=components=${3}:levels_mode=${4}[b2];[a2][b2]vstack",
            // field Y, rgb, Y, all planes Y
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]format=rgb24,histogram=levels_mode=${4}[a2];[b1]format=rgb24,histogram=levels_mode=${4}[b2];[a2][b2]hstack",
        },
    },
    {
        "Waveform",
        0,
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field" },
            { Args_Type_Slider,  10,   0, 100, 100, "Intensity" },
            { Args_Type_YuvA,     0,   0,   0,   0, "Plane" },
            { Args_Type_Toggle,   0,   0,   0,   0, "Vertical" },
            { Args_Type_Slider,   0,   0,   6,   1, "Filter" },
            { Args_Type_Slider,   0,   0,   2,   1, "Scale" },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
            // field N, all planes N, vertical N
            "waveform=intensity=${2}:mode=column:mirror=1:c=${3}:f=${5}:graticule=green:flags=numbers+dots:scale=${6}",
            // field N, all planes N, vertical Y
            "waveform=intensity=${2}:mode=row:   mirror=1:c=${3}:f=${5}:graticule=green:flags=numbers+dots:scale=${6}",
            // field N, all planes Y, vertical N
            "waveform=intensity=${2}:mode=column:mirror=1:c=${3}:f=${5}:graticule=green:flags=numbers+dots:scale=${6}:display=overlay",
            // field N, all planes Y, vertical Y
            "waveform=intensity=${2}:mode=row:   mirror=1:c=${3}:f=${5}:graticule=green:flags=numbers+dots:scale=${6}:display=overlay",
            // field Y, all planes N, vertical N
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]waveform=intensity=${2}:mode=column:mirror=1:c=${3}:f=${5}:graticule=green:flags=numbers+dots:scale=${6}[a2];[b1]waveform=intensity=${2}:mode=column:mirror=1:c=${3}:f=${5}:graticule=green:flags=numbers+dots:scale=${6}[b2];[a2][b2]vstack",
            // field Y, all planes N, vertical Y
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]waveform=intensity=${2}:mode=row:   mirror=1:c=${3}:f=${5}:graticule=green:flags=numbers+dots:scale=${6}[a2];[b1]waveform=intensity=${2}:mode=row:   mirror=1:c=${3}:f=${5}:graticule=green:flags=numbers+dots:scale=${6}[b2];[a2][b2]hstack",
            // field Y, all planes Y, vertical N
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]waveform=intensity=${2}:mode=column:mirror=1:c=${3}:f=${5}:graticule=green:flags=numbers+dots:scale=${6}:display=overlay[a2];[b1]waveform=intensity=${2}:mode=column:mirror=1:c=${3}:f=${5}:graticule=green:flags=numbers+dots:scale=${6}:display=overlay[b2];[a2][b2]vstack",
            // field Y, all planes Y, vertical Y
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]waveform=intensity=${2}:mode=row:   mirror=1:c=${3}:f=${5}:graticule=green:flags=numbers+dots:scale=${6}:display=overlay[a2];[b1]waveform=intensity=${2}:mode=row:   mirror=1:c=${3}:f=${5}:graticule=green:flags=numbers+dots:scale=${6}:display=overlay[b2];[b2][a2]hstack",
        },
    },
    {
        "Waveform Target",
        0,
        {
            { Args_Type_Slider,  20,   0,   0,   1, "x" },
            { Args_Type_Slider,  20,   0,   0,   1, "y" },
            { Args_Type_Slider, 121,  16,   0,   1, "w" },
            { Args_Type_Slider, 121,  16,   0,   1, "h" },
            //{ Args_Type_Slider,   8,   0,  10,  10, "Intensity" },
            { Args_Type_Slider,   0,   0,   5,   1, "Filter" },
            { Args_Type_Slider,   0,   0,   2,   1, "Scale" },
            { Args_Type_Toggle,   1,   0,   0,   0, "Background"},
        },
        {
            "crop=${3}:${4}:${1}:${2},\
            waveform=intensity=0.8:mode=column:mirror=1:c=1:f=${5}:graticule=green:flags=numbers+dots:scale=${6},scale=${width}:${height},setsar=1/1",
            "split[a][b];\
            [a]lutyuv=y=val/4,scale=${width}:${height},setsar=1/1,format=yuv444p|yuv444p10le[a1];\
            [b]crop=${3}:${4}:${1}:${2},\
            waveform=intensity=0.8:mode=column:mirror=1:c=1:f=${5}:graticule=green:flags=numbers+dots:scale=${6},scale=${width}:${height},setsar=1/1[b1];\
            [a1][b1]blend=addition",
        },
    },
    {
        "Line Select",
        0,
        {
            { Args_Type_Slider,   1,   1,   0,   1, "Line" },
            { Args_Type_Slider,  10,   0,  10,  10, "Intensity" },
            { Args_Type_Toggle,   0,   0,   0,   0, "Vertical"},
            { Args_Type_Toggle,   0,   0,   0,   0, "Background"},
            { Args_Type_Slider,   0,   0,   2,   1, "Scale" },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
            "               crop=iw:1:0:${1}:0:1,waveform=intensity=${2}:mode=column:mirror=1:components=7:display=overlay:graticule=green:flags=numbers+dots:scale=${5}",
            "split[a][b];[a]crop=iw:1:0:${1}:0:1,waveform=intensity=${2}:mode=column:mirror=1:components=7:display=overlay:graticule=green:flags=numbers+dots:scale=${5},scale=iw:${height},drawbox=y=${1}:w=iw:h=1:color=yellow,setsar=1/1[a1];[b]lutyuv=y=val/2,setsar=1/1[b1];[a1][b1]blend=addition",
            "               crop=1:ih:${1}:0:0:1,waveform=intensity=${2}:mode=row:   mirror=1:components=7:display=overlay:graticule=green:flags=numbers+dots:scale=${5}",
            "split[a][b];[a]crop=1:ih:${1}:0:0:1,waveform=intensity=${2}:mode=row:   mirror=1:components=7:display=overlay:graticule=green:flags=numbers+dots:scale=${5},scale=${width}:${height},drawbox=x=${1}:w=1:h=ih:color=yellow,setsar=1/1[a1];[b]lutyuv=y=val/2,setsar=1/1[b1];[a1][b1]blend=addition",
        },
    },
    {
        "Oscilloscope",
        0,
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field" },
            { Args_Type_Slider,   500000,   0,   1000000,   1, "SX pos" },
            { Args_Type_Slider,   500000,   0,   1000000,   1, "SY pos" },
            { Args_Type_Slider,   500000,   0,   1000000,   1, "S size" },
            { Args_Type_Slider,   500000,   0,   1000000,   1, "S tilt" },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
            "oscilloscope=x=${2}/1000000:y=${3}/1000000:s=${4}/1000000:t=${5}/1000000",
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]oscilloscope=x=${2}/1000000:y=${3}/1000000:s=${4}/1000000:t=${5}/1000000[a2];[b1]oscilloscope=x=${2}/1000000:y=${3}/1000000:s=${4}/1000000:t=${5}/1000000[b2],[a2][b2]vstack",
        },
    },
    {
        "Pixel Scope",
        0,
        {
            { Args_Type_Toggle,      0,   0,       0,   0, "Field" },
            { Args_Type_Slider,     20,   0,       0,   1, "x" },
            { Args_Type_Slider,     20,   0,       0,   1, "y" },
            { Args_Type_Slider,      8,   1,      80,   1, "width" },
            { Args_Type_Slider,      8,   1,      80,   1, "height" },
            { Args_Type_None,        0,   0,       0,   0, nullptr },
            { Args_Type_None,        0,   0,       0,   0, nullptr },
        },
        {
            "pixscope=x=${2}/${width}:y=${3}/${height}:w=${4}:h=${5}",
            "il=l=d:c=d,pixscope=x=${2}/${width}:y=${3}/${height}:w=${4}:h=${5}",
        },
    },
    {
        "Vectorscope",
        0,
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field" },
            { Args_Type_Slider,   1,   0,  10,  10, "Intensity" },
            { Args_Type_Slider,   3,   0,   5,   1, "Mode" },
            { Args_Type_Slider,   0,   0,   3,   1, "Peak" },
            { Args_Type_Slider,   1,   0,   2,   1, "Colorspace" },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
            "vectorscope=i=${2}:mode=${3}:envelope=${4}:colorspace=${5}:graticule=green:flags=name,pad=ih*${dar}:ih:(ow-iw)/2:(oh-ih)/2",
            "format=yuv422p|yuv422p10le|yuv420p|yuv411p|yuv444p|yuv444p10le,split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]vectorscope=i=${2}:mode=${3}:envelope=${4}:colorspace=${5}:graticule=green:flags=name[a2];[b1]vectorscope=i=${2}:mode=${3}:envelope=${4}:colorspace=${5}:graticule=green:flags=name[b2];[a2][b2]vstack,pad=ih*${dar}:ih:(ow-iw)/2:(oh-ih)/2",
        },
    },
    {
        "Vectorscope High/Low",
        0,
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field" },
            { Args_Type_Slider,   1,   0,  10,  10, "Intensity" },
            { Args_Type_Slider,   3,   0,   5,   1, "Mode" },
            { Args_Type_Slider,   0,   0,   3,   1, "Peak" },
            { Args_Type_Slider,   1,   0,   2,   1, "Colorspace" },
            { Args_Type_Slider,  50,   0, 100, 100, "Threshold" },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
            "split[h][l];[l]vectorscope=i=${2}:mode=${3}:envelope=${4}:colorspace=${5}:graticule=green:flags=name:l=0:h=${6}[l1];[h]vectorscope=i=${2}:mode=${3}:envelope=${4}:colorspace=${5}:graticule=green:flags=name:l=${6}:h=1[h1];[l1][h1]hstack",
            "format=yuv422p|yuv422p10le|yuv420p|yuv411p|yuv444p|yuv444p10le,split[a][b];\
            [a]field=top,split[th][tl];\
            [b]field=bottom,split[bh][bl];\
            [th]vectorscope=i=${2}:mode=${3}:envelope=${4}:colorspace=${5}:graticule=green:flags=name:l=${6}:h=1[th1];\
            [tl]vectorscope=i=${2}:mode=${3}:envelope=${4}:colorspace=${5}:graticule=green:flags=name:l=0:h=${6}[tl1];\
            [bh]vectorscope=i=${2}:mode=${3}:envelope=${4}:colorspace=${5}:graticule=green:flags=name:l=${6}:h=1[bh1];\
            [bl]vectorscope=i=${2}:mode=${3}:envelope=${4}:colorspace=${5}:graticule=green:flags=name:l=0:h=${6}[bl1];\
            [tl1][th1]hstack[t];\
            [bl1][bh1]hstack[b];\
            [t][b]vstack",
        },
    },
    {
        "Vectorscope Target",
        0,
        {
            { Args_Type_Slider,  20,   0,   0,   1, "x" },
            { Args_Type_Slider,  20,   0,   0,   1, "y" },
            { Args_Type_Slider, 120,  16,   0,   1, "w" },
            { Args_Type_Slider, 120,  16,   0,   1, "h" },
            //{ Args_Type_Slider,   1,   0,  10,  10, "Intensity" },
            { Args_Type_Slider,   3,   0,   4,   1, "Mode" },
            { Args_Type_Slider,   0,   0,   3,   1, "Peak" },
            { Args_Type_Toggle,   1,   0,   0,   0, "Background"},
        },
        {
            "crop=${3}:${4}:${1}:${2},\
            format=yuv422p|yuv422p10le|yuv420p|yuv411p|yuv444p|yuv444p10le,vectorscope=i=0.1:mode=${5}:envelope=${6}:colorspace=601:graticule=green:flags=name,pad=ih*${dar}:ih:(ow-iw)/2:(oh-ih)/2",
            "split[a][b];\
            [a]lutyuv=y=val/4,scale=${width}:${height},setsar=1/1,format=yuv444p|yuv444p10le[a1];\
            [b]crop=${3}:${4}:${1}:${2},\
            format=yuv422p|yuv422p10le|yuv420p|yuv411p|yuv444p|yuv444p10le,vectorscope=i=0.1:mode=${5}:envelope=${6}:colorspace=601:graticule=green:flags=name,pad=ih*${dar}:ih:(ow-iw)/2:(oh-ih)/2,scale=${width}:${height},setsar=1/1[b1];\
            [a1][b1]blend=addition",
        },
    },
    {
        "Waveform / Vectorscope",
        0,
        {
            { Args_Type_Slider,   1,   0,  10,  10, "Waveform" },
            { Args_Type_Slider,   1,   0,  10,  10, "Vectorscope" },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
            "split[a][b];[a]format=yuv422p|yuv422p10le|yuv420p|yuv411p|yuv444p|yuv444p10le,\
            vectorscope=intensity=${2}:mode=4,pad=ih*${dar}:ih:(ow-iw)/2:(oh-ih)/2,scale=720:512,setsar=1/1[a1];\
            [b]waveform=intensity=${1}:mode=column:mirror=1:c=1,scale=720:512,setsar=1/1[b1];\
            [b1][a1]blend=c0_mode=addition:c1_mode=average:c2_mode=average,hue=s=2",
        },
    },
    {
        "CIE Scope",
        0,
        {
            { Args_Type_Slider,   1,   0,   8,   1, "System"},
            { Args_Type_Slider,   1,   0,   8,   1, "Gamut"},
            { Args_Type_Slider,   7,   0,  10,  10, "Contrast" },
            { Args_Type_Slider,   1,   0, 100, 100, "Intensity" },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
            "ciescope=system=${1}:gamuts=pow(2\\,${2}):contrast=${3}:intensity=${4}",
        },
    },
    {
        "Datascope",
        0,
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field" },
            { Args_Type_Slider,   0,   0,   0,   1, "x" },
            { Args_Type_Slider,   0,   0,   0,   1, "y" },
            { Args_Type_Slider,   1,   0,   1,   1, "Axis"},
            { Args_Type_Slider,   1,   0,   2,   1, "DataMode" },
            { Args_Type_Toggle,   0,   0,   0,   0, "Show" },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
            "datascope=x=${2}:y=${3}:mode=${5}:axis=${4}",
            "drawbox=x=${2}:y=${3}:color=yellow:thickness=4:width=32:height=4,drawbox=x=${2}:y=${3}:color=yellow:thickness=4:width=4:height=32",
            "il=l=d:c=d,datascope=x=${2}:y=${3}:mode=${5}:axis=${4}",
            "il=l=d:c=d,drawbox=x=${2}:y=${3}:color=yellow:thickness=4:width=32:height=4,drawbox=x=${2}:y=${3}:color=yellow:thickness=4:width=4:height=32",

        },
    },
    {
        "Extract Planes Equalized",
        0,
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field" },
            { Args_Type_Yuv,      2,   0,   0,   0, "Plane"},
            { Args_Type_Slider,   2,   0,  10,  10, "Strength" },
            { Args_Type_Slider,   2,   0,  10,  10, "Intensity" },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
            "format=yuv444p|yuv422p|yuv420p|yuv444p|yuv410p,extractplanes=${2},histeq=strength=${3}:intensity=${4}",
            "il=l=d:c=d,format=yuv444p|yuv422p|yuv420p|yuv444p|yuv410p,extractplanes=${2},histeq=strength=${3}:strength=${4}",
        },
    },
    {
        "Extract Planes UV Equal.",
        0,
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field" },
            { Args_Type_Slider,   2,   0,  10,  10, "Strength" },
            { Args_Type_Slider,   2,   0,  10,  10, "Intensity" },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
            "format=yuv444p|yuv422p|yuv420p|yuv444p|yuv410p,extractplanes=u+v,hstack,histeq=strength=${2}:intensity=${3}",
            "il=l=d:c=d,format=yuv444p|yuv422p|yuv420p|yuv444p|yuv410p,extractplanes=u+v,hstack,histeq=strength=${2}:strength=${3}",
        },
    },
    {
        "Bit Plane",
        0,
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field" },
            { Args_Type_Slider,   1,  -1,   10,   1, "Y bit position" },
            { Args_Type_Slider,   -1, -1,   10,   1, "U bit position" },
            { Args_Type_Slider,   -1, -1,   10,   1, "V bit position" },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
            "format=yuv420p10le|yuv422p10le|yuv444p10le|yuv440p10le,\
            lutyuv=\
                y=if(eq(${2}\\,-1)\\,512\\,if(eq(${2}\\,0)\\,val\\,bitand(val\\,pow(2\\,10-${2}))*pow(2\\,${2}))):\
                u=if(eq(${3}\\,-1)\\,512\\,if(eq(${3}\\,0)\\,val\\,bitand(val\\,pow(2\\,10-${3}))*pow(2\\,${3}))):\
                v=if(eq(${4}\\,-1)\\,512\\,if(eq(${4}\\,0)\\,val\\,bitand(val\\,pow(2\\,10-${4}))*pow(2\\,${4}))),format=yuv444p",
            "il=l=d:c=d,format=yuv420p10le|yuv422p10le|yuv444p10le|yuv440p10le,\
            lutyuv=\
                y=if(eq(${2}\\,-1)\\,512\\,if(eq(${2}\\,0)\\,val\\,bitand(val\\,pow(2\\,10-${2}))*pow(2\\,${2}))):\
                u=if(eq(${3}\\,-1)\\,512\\,if(eq(${3}\\,0)\\,val\\,bitand(val\\,pow(2\\,10-${3}))*pow(2\\,${3}))):\
                v=if(eq(${4}\\,-1)\\,512\\,if(eq(${4}\\,0)\\,val\\,bitand(val\\,pow(2\\,10-${4}))*pow(2\\,${4}))),format=yuv444p",
        },
    },
    {
        "Bit Plane (10 slices)",
        0,
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Rows" },
            { Args_Type_Yuv,      0,   0,   0,   0, "Plane"},
            { Args_Type_Slider,   0,   1,   0,   1, "x offset" },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
            "format=yuv420p10le|yuv422p10le|yuv444p10le|yuv440p10le,split[h1][h2];[h1][h2]hstack,crop=iw/2:ih:${3}:0,\
            split=10[b0][b1][b2][b3][b4][b5][b6][b7][b8][b9];\
            [b0]crop=iw/10:ih:(iw/10)*0:0,lutyuv=y=512:u=512:v=512:${2}=bitand(val\\,pow(2\\,10-1))*pow(2\\,1)[b0c];\
            [b1]crop=iw/10:ih:(iw/10)*1:0,lutyuv=y=512:u=512:v=512:${2}=bitand(val\\,pow(2\\,10-2))*pow(2\\,2)[b1c];\
            [b2]crop=iw/10:ih:(iw/10)*2:0,lutyuv=y=512:u=512:v=512:${2}=bitand(val\\,pow(2\\,10-3))*pow(2\\,3)[b2c];\
            [b3]crop=iw/10:ih:(iw/10)*3:0,lutyuv=y=512:u=512:v=512:${2}=bitand(val\\,pow(2\\,10-4))*pow(2\\,4)[b3c];\
            [b4]crop=iw/10:ih:(iw/10)*4:0,lutyuv=y=512:u=512:v=512:${2}=bitand(val\\,pow(2\\,10-5))*pow(2\\,5)[b4c];\
            [b5]crop=iw/10:ih:(iw/10)*5:0,lutyuv=y=512:u=512:v=512:${2}=bitand(val\\,pow(2\\,10-6))*pow(2\\,6)[b5c];\
            [b6]crop=iw/10:ih:(iw/10)*6:0,lutyuv=y=512:u=512:v=512:${2}=bitand(val\\,pow(2\\,10-7))*pow(2\\,7)[b6c];\
            [b7]crop=iw/10:ih:(iw/10)*7:0,lutyuv=y=512:u=512:v=512:${2}=bitand(val\\,pow(2\\,10-8))*pow(2\\,8)[b7c];\
            [b8]crop=iw/10:ih:(iw/10)*8:0,lutyuv=y=512:u=512:v=512:${2}=bitand(val\\,pow(2\\,10-9))*pow(2\\,9)[b8c];\
            [b9]crop=iw/10:ih:(iw/10)*9:0,lutyuv=y=512:u=512:v=512:${2}=bitand(val\\,pow(2\\,10-10))*pow(2\\,10)[b9c];\
            [b0c][b1c][b2c][b3c][b4c][b5c][b6c][b7c][b8c][b9c]hstack=10,format=yuv444p,drawgrid=w=iw/10:h=ih:t=2:c=green@0.5",
            "format=yuv420p10le|yuv422p10le|yuv444p10le|yuv440p10le,split[h1][h2];[h1][h2]hstack,crop=iw/2:ih:${3}:0,\
            split=10[b0][b1][b2][b3][b4][b5][b6][b7][b8][b9];\
            [b0]crop=iw:ih/10:0:(ih/10)*0,lutyuv=y=512:u=512:v=512:${2}=bitand(val\\,pow(2\\,10-1))*pow(2\\,1)[b0c];\
            [b1]crop=iw:ih/10:0:(ih/10)*1,lutyuv=y=512:u=512:v=512:${2}=bitand(val\\,pow(2\\,10-2))*pow(2\\,2)[b1c];\
            [b2]crop=iw:ih/10:0:(ih/10)*2,lutyuv=y=512:u=512:v=512:${2}=bitand(val\\,pow(2\\,10-3))*pow(2\\,3)[b2c];\
            [b3]crop=iw:ih/10:0:(ih/10)*3,lutyuv=y=512:u=512:v=512:${2}=bitand(val\\,pow(2\\,10-4))*pow(2\\,4)[b3c];\
            [b4]crop=iw:ih/10:0:(ih/10)*4,lutyuv=y=512:u=512:v=512:${2}=bitand(val\\,pow(2\\,10-5))*pow(2\\,5)[b4c];\
            [b5]crop=iw:ih/10:0:(ih/10)*5,lutyuv=y=512:u=512:v=512:${2}=bitand(val\\,pow(2\\,10-6))*pow(2\\,6)[b5c];\
            [b6]crop=iw:ih/10:0:(ih/10)*6,lutyuv=y=512:u=512:v=512:${2}=bitand(val\\,pow(2\\,10-7))*pow(2\\,7)[b6c];\
            [b7]crop=iw:ih/10:0:(ih/10)*7,lutyuv=y=512:u=512:v=512:${2}=bitand(val\\,pow(2\\,10-8))*pow(2\\,8)[b7c];\
            [b8]crop=iw:ih/10:0:(ih/10)*8,lutyuv=y=512:u=512:v=512:${2}=bitand(val\\,pow(2\\,10-9))*pow(2\\,9)[b8c];\
            [b9]crop=iw:ih/10:0:(ih/10)*9,lutyuv=y=512:u=512:v=512:${2}=bitand(val\\,pow(2\\,10-10))*pow(2\\,10)[b9c];\
            [b0c][b1c][b2c][b3c][b4c][b5c][b6c][b7c][b8c][b9c]vstack=10,format=yuv444p,drawgrid=w=iw:h=ih/10:t=2:c=green@0.5",
        },
    },
    {
        "Bit Plane Noise",
        0,
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field" },
            // TODO: Adjust slider max to bit depth.
            { Args_Type_Slider,   1,   1,  16,   1, "Bit position" },
            { Args_Type_YuvA,     0,   0,   0,   0, "Plane"},
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
            "bitplanenoise=bitplane=${2}:filter=1,format=yuv444p,extractplanes=${3}",
            "bitplanenoise=bitplane=${2}:filter=1",
            "il=l=d:c=d,bitplanenoise=bitplane=${2}:filter=1,format=yuv444p,extractplanes=${3}",
            "il=l=d:c=d,bitplanenoise=bitplane=${2}:filter=1",

        },
    },
    {
        "Bit Plane Noise Graph",
        0,
        {
            // TODO: Adjust slider max to bit depth.
            { Args_Type_Slider,   1,   1,  16,   1, "Bit position" },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
            "bitplanenoise=${1},drawgraph=fg1=0x006400:fg2=0x00008B:fg3=0x8B0000:m1=lavfi.bitplanenoise.0.${1}:m2=lavfi.bitplanenoise.1.${1}:m3=lavfi.bitplanenoise.2.${1}:min=0:max=1:slide=rscroll:s=${width}x${height}",
        },
    },
    /*
    {
        "Frame Metadata Play",
        0,
        {
            { Args_Type_ClrPck, 0xFFFF00,   0,   0,   0, ""},
            { Args_Type_None,   0, 0, 0, 0, },
            { Args_Type_None,   0, 0, 0, 0, },
            { Args_Type_None,   0, 0, 0, 0, },
            { Args_Type_None,   0, 0, 0, 0, },
            { Args_Type_None,   0, 0, 0, 0, },
            { Args_Type_None,   0, 0, 0, 0, },
        },
        {
            "cropdetect=reset=1:limit=16:round=1,signalstats=stat=brng+vrep+tout,drawtext=fontfile=/Users/rice/Downloads/Anonymous_Pro_B.ttf:x=8:y=8:fontcolor=${1}:shadowx=3:shadowy=2:fontsize=20:tabsize=8:textfile=/Users/rice/Downloads/drawtext.txt",
        },
    },
    */
    {
        "Value Highlight",
        0,
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field" },
            { Args_Type_Yuv,      0,   0,   0,   0, "Plane" },
            { Args_Type_Slider, 235,   0, 255,   1, "Min"},
            { Args_Type_Slider, 255,   0, 255,   1, "Max"},
            { Args_Type_ClrPck, 0xFFFF00,   0,   0,   0, ""},
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
            "extractplanes=${2},format=rgb24,lutrgb=r=if(between(val\\,${3}\\,${4})\\,${5R}\\,val):g=if(between(val\\,${3}\\,${4})\\,${5G}\\,val):b=if(between(val\\,${3}\\,${4})\\,${5B}\\,val)",
            "extractplanes=${2},format=rgb24,il=l=d:c=d,lutrgb=r=if(between(val\\,${3}\\,${4})\\,${5R}\\,val):g=if(between(val\\,${3}\\,${4})\\,${5G}\\,val):b=if(between(val\\,${3}\\,${4})\\,${5B}\\,val)",
        },
    },
    {
        "Saturation Highlight",
        0,
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field" },
            { Args_Type_Toggle,   0,   0,   0,   0, "Sat as Lum" },
            { Args_Type_Slider,  89,   0, 182,   1, "Min"},
            { Args_Type_Slider, 182,   0, 182,   1, "Max"},
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
            "format=yuv444p,geq=lum=lum(X\\,Y):cb=if(between(hypot(cb(X\\,Y)-128\\,cr(X\\,Y)-128)\\,${3}\\,${4})\\,32\\,128):cr=if(between(hypot(cb(X\\,Y)-128\\,cr(X\\,Y)-128)\\,${3}\\,${4})\\,220\\,128)",
            "format=yuv444p,geq=lum=hypot(cb(X\\,Y)-128\\,cr(X\\,Y)-128)*(256/189):cb=if(between(hypot(cb(X\\,Y)-128\\,cr(X\\,Y)-128)\\,${3}\\,${4})\\,32\\,128):cr=if(between(hypot(cb(X\\,Y)-128\\,cr(X\\,Y)-128)\\,${3}\\,${4})\\,220\\,128)",
            "il=l=d:c=d,format=yuv444p,geq=lum=lum(X\\,Y):cb=if(between(hypot(cb(X\\,Y)-128\\,cr(X\\,Y)-128)\\,${3}\\,${4})\\,32\\,128):cr=if(between(hypot(cb(X\\,Y)-128\\,cr(X\\,Y)-128)\\,${3}\\,${4})\\,220\\,128)",
            "il=l=d:c=d,format=yuv444p,geq=lum=hypot(cb(X\\,Y)-128\\,cr(X\\,Y)-128)*(256/189):cb=if(between(hypot(cb(X\\,Y)-128\\,cr(X\\,Y)-128)\\,${3}\\,${4})\\,32\\,128):cr=if(between(hypot(cb(X\\,Y)-128\\,cr(X\\,Y)-128)\\,${3}\\,${4})\\,220\\,128)"
        },
    },
    {
        "Chroma Adjust",
        0,
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field" },
            { Args_Type_Toggle,   0,   0,   0,   0, "Vectorscope" },
            { Args_Type_Slider,   0,-180, 180,   1, "Hue"},
            { Args_Type_Slider,  10,   0,  30,  10, "Saturation"},
            { Args_Type_Slider,   0,-128, 128,   1, "Cb Shift"},
            { Args_Type_Slider,   0,-128, 128,   1, "Cr Shift"},
            { Args_Type_Slider,   1,   0,   2,   1, "Colorspace" },
        },
        {
            "format=yuv444p|yuvj444p,lutyuv=y=val:u=mod(val+${5}\\,256):v=mod(val+${6}\\,256),hue=h=${3}:s=${4}",
            "format=yuv444p|yuvj444p,lutyuv=y=val:u=mod(val+${5}\\,256):v=mod(val+${6}\\,256),hue=h=${3}:s=${4},split[a][b];[a]vectorscope=mode=color2:colorspace=${7}:graticule=green:flags=name,\
            scale=512:512,pad=720:512:(ow-iw)/2:(oh-ih)/2,setsar=1/1[a1];\
            [b]lutyuv=y=val/2,scale=720:512,setsar=1/1[b1];[a1][b1]blend=addition",
            "il=l=d:c=d,format=yuv444p|yuvj444p,lutyuv=y=val:u=mod(val+${5}\\,256):v=mod(val+${6}\\,256),hue=h=${3}:s=${4}",
            "il=l=d:c=d,format=yuv444p|yuvj444p,lutyuv=y=val:u=mod(val+${5}\\,256):v=mod(val+${6}\\,256),hue=h=${3}:s=${4},split[a][b];[a]vectorscope=mode=color2:colorspace=${7}:graticule=green:flags=name,\
            scale=512:512,pad=720:512:(ow-iw)/2:(oh-ih)/2,setsar=1/1[a1];\
            [b]lutyuv=y=val/2,scale=720:512,setsar=1/1[b1];[a1][b1]blend=addition",
        },
    },
    {
        "Luma Adjust",
        0,
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field" },
            { Args_Type_Toggle,   0,   0,   0,   0, "Waveform" },
            { Args_Type_Slider,   0,-180, 180,   1, "Offset"},
            { Args_Type_Slider, 100,   0, 400, 100, "Contrast"},
            { Args_Type_Slider,   0,   0,   5,   1, "Filter" },
            { Args_Type_Slider,   0,   0,   2,   1, "Scale" },
            { Args_Type_Slider,   1,   0,  10,  10, "Intensity" },
        },
        {
            "format=yuv444p|yuvj444p,lutyuv=y=(val+${3})*${4}:u=val:v=val",
            "format=yuv444p|yuvj444p,lutyuv=y=(val+${3})*${4}:u=val:v=val,split[a][b];[a]waveform=intensity=${7}:graticule=green:flags=numbers+dots:f=${5}:scale=${6},\
            scale=${width}:${height},setsar=1/1[a1];[b]setsar=1/1[b1];\
            [b1][a1]vstack",
            "il=l=d:c=d,format=yuv444p|yuvj444p,lutyuv=y=(val+${3})*${4}:u=val:v=val",
            "format=yuv444p|yuvj444p,split[a][b];\
            [a]field=top,split[t1][t2];\
            [t1]lutyuv=y=(val+${3})*${4}:u=val:v=val,waveform=intensity=${7}:graticule=green:flags=numbers+dots:f=${5}:scale=${6}[t1w];\
            [b]field=bottom,split[b1][b2];\
            [b1]lutyuv=y=(val+${3})*${4}:u=val:v=val,waveform=intensity=${7}:graticule=green:flags=numbers+dots:f=${5}:scale=${6}[b1w];\
            [t2][t1w][b2][b1w]vstack=4",
        },
    },
    {
        "Chroma Delay",
        0,
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field" },
            { Args_Type_Slider,   0,-128, 128,   1, "Chroma Shift"},
            { Args_Type_Toggle,   1,   0,   0,   0, "Interleave" },
            { Args_Type_None,          0,   0,   0,   0, nullptr },
            { Args_Type_None,          0,   0,   0,   0, nullptr },
            { Args_Type_None,          0,   0,   0,   0, nullptr },
            { Args_Type_None,          0,   0,   0,   0, nullptr },
        },
        {
            "format=yuv444p,pad=w=iw+256:h=ih:x=128,geq=lum=lum(X\\,Y):cb=cb(X-${2}\\,Y):cr=cr(X-${2}\\,Y)",
            "format=yuv444p,split[y][u];[y]extractplanes=y,pad=w=iw+256:h=ih:x=128,format=yuv444p[y1];[u]extractplanes=u,histeq,pad=w=iw+256:h=ih:x=${2}+128:y=0,format=yuv444p[u1];[y1][u1]vstack,il=l=i:c=i",
            "il=l=d:c=d,format=yuv444p,pad=w=iw+256:h=ih:x=128,geq=lum=lum(X\\,Y):cb=cb(X-${2}\\,Y):cr=cr(X-${2}\\,Y)",
            "il=l=d:c=d,format=yuv444p,split[y][u];[y]extractplanes=y,pad=w=iw+256:h=ih:x=128,format=yuv444p[y1];[u]extractplanes=u,histeq,pad=w=iw+256:h=ih:x=${2}+128:y=0,format=yuv444p[u1];[y1][u1]vstack,il=l=i:c=i",
        },
    },
    {
        "Color Matrix",
        0,
        {
            { Args_Type_ColorMatrix,   0,   0,   0,   0, "Src" },
            { Args_Type_ColorMatrix,   1,   0,   0,   0, "Dst" },
            { Args_Type_Slider,        0,   0,   0,   1, "Reveal" },
            { Args_Type_None,          0,   0,   0,   0, nullptr },
            { Args_Type_None,          0,   0,   0,   0, nullptr },
            { Args_Type_None,          0,   0,   0,   0, nullptr },
            { Args_Type_None,          0,   0,   0,   0, nullptr },
        },
        {
            "split[a][b];[a]crop=${3}:${height}:0:0[a1];[b]colormatrix=${1}:${2}[b1];[b1][a1]overlay",
        },
    },
    {
        "Sample Range",
        0,
        {
            { Args_Type_SampleRange,   2,   0,   0,   0, "Src" },
            { Args_Type_SampleRange,   1,   0,   0,   0, "Dst" },
            { Args_Type_Slider,        0,   0,   0,   1, "Reveal" },
            { Args_Type_None,          0,   0,   0,   0, nullptr },
            { Args_Type_None,          0,   0,   0,   0, nullptr },
            { Args_Type_None,          0,   0,   0,   0, nullptr },
            { Args_Type_None,          0,   0,   0,   0, nullptr },
        },
        {
            "split[a][b];[a]crop=${3}:${height}:0:0[a1];[b]scale=iw+1:ih:in_range=${1}:out_range=${2},scale=iw-1:ih[b1];[b1][a1]overlay",
        },
    },
    {
        "Limiter",
        0,
        {
            { Args_Type_Toggle,        0,   0,   0,   0, "Field" },
            { Args_Type_YuvA,          0,   0,   0,   0, "Plane" },
            { Args_Type_Slider,        0,   0, 255,   1, "Min"},
            { Args_Type_Slider,      255,   0, 255,   1, "Max"},
            { Args_Type_Slider,        2,   0,  10,  10, "Strength" },
            { Args_Type_None,          0,   0,   0,   0, nullptr },
            { Args_Type_None,          0,   0,   0,   0, nullptr },
        },
        {
            "limiter=min=${3}:max=${4}:planes=${2},histeq=strength=${5}",
            "limiter=min=${3}:max=${4}:planes=7,histeq=strength=${5}",
            "il=l=d:c=d,limiter=min=${3}:max=${4}:planes=${2},histeq=strength=${5}",
            "il=l=d:c=d,limiter=min=${3}:max=${4}:planes=7,histeq=strength=${5}",
        },
    },
    {
        "Field Difference",
        0,
        {
            { Args_Type_YuvA,     3,   0,   0,   0, "Plane" },
            { Args_Type_Slider,   0,   0,  10,  10, "Strength" },
            { Args_Type_Slider,   0,   0,  10,  10, "Intensity" },
            { Args_Type_Toggle,   0,   0,   0,   0, "Columns" },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
            "extractplanes=${1},split[a][b];[a]field=bottom[a1];[b]field=top,negate[b2];[a1][b2]blend=all_mode=average,histeq=strength=${2}:intensity=${3}",
            "extractplanes=${1},transpose=1,split[a][b];[a]field=bottom[a1];[b]field=top,negate[b2];[a1][b2]blend=all_mode=average,histeq=strength=${2}:intensity=${3},transpose=2",
            "split[a][b];[a]field=bottom[a1];[b]field=top,negate[b2];[a1][b2]blend=all_mode=average,histeq=strength=${2}:intensity=${3}",
            "transpose=1,split[a][b];[a]field=bottom[a1];[b]field=top,negate[b2];[a1][b2]blend=all_mode=average,histeq=strength=${2}:intensity=${3},transpose=2",
        },
    },
    {
        "Temporal Difference",
        0,
        {
            { Args_Type_YuvA,     3,   0,   0,   0, "Plane" },
            { Args_Type_Slider,   2,   0,  10,  10, "Strength" },
            { Args_Type_Slider,   2,   0,  10,  10, "Intensity" },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
            "extractplanes=${1},tblend=all_mode=difference128,histeq=strength=${2}:intensity=${3}",
            "tblend=all_mode=difference128,histeq=strength=${2}:intensity=${3}",
        },
    },
    {
        "Pixel Offset Subtraction",
        0,
        {
            { Args_Type_Slider,   1,-120, 120,   1, "Y H" },
            { Args_Type_Slider,   0,-120, 120,   1, "Y V" },
            { Args_Type_Slider,   0,-120, 120,   1, "UV H" },
            { Args_Type_Slider,   0,-120, 120,   1, "UV V" },
            { Args_Type_Slider,   0,   0,  10,  10, "Strength" },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
            "geq=lum=lum(X\\,Y)-lum(X-${1}\\,Y-${2})+128:cb=cb(X\\,Y)-cb(X-${3}\\,Y-${4})+128:cr=cr(X\\,Y)-cr(X-${3}\\,Y-${4})+128,histeq=strength=${5}",
        },
    },
    {
        "Broadcast Range Pixels",
        0,
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field" },
            { Args_Type_ClrPck, 0xFFFF00,   0,   0,   0, ""},
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
            "signalstats=out=brng:c=${2},format=yuv444p|rgb24",
            "il=l=d:c=d,signalstats=out=brng:c=${2},format=yuv444p|rgb24",
        },
    },
    {
        "Broadcast Illegal Focus",
        0,
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field" },
            { Args_Type_Ranges,   1,   0,   0,   0, "Outer Range"},
            { Args_Type_Slider,   3,   0,  10,  10, "Strength" },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
                       "lutyuv=y=if(gt(val\\,maxval)\\,val-maxval\\,0):u=(maxval+minval)/2:v=(maxval+minval)/2,histeq=strength=${3}",
                       "lutyuv=y=if(lt(val\\,minval)\\,val+minval\\,0):u=(maxval+minval)/2:v=(maxval+minval)/2,histeq=strength=${3}",
            "il=l=d:c=d,lutyuv=y=if(gt(val\\,maxval)\\,val-maxval\\,0):u=(maxval+minval)/2:v=(maxval+minval)/2,histeq=strength=${3}",
            "il=l=d:c=d,lutyuv=y=if(lt(val\\,minval)\\,val+minval\\,0):u=(maxval+minval)/2:v=(maxval+minval)/2,histeq=strength=${3}",
        },
    },
    {
        "Temporal Outlier Pixels",
        0,
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field" },
            { Args_Type_ClrPck, 0xFFFF00,   0,   0,   0, ""},
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
            "signalstats=out=tout:c=${2}",
            "il=l=d:c=d,signalstats=out=tout:c=${2}",
        },
    },
    {
        "Vertical Line Repetition",
        0,
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field" },
            { Args_Type_ClrPck, 0xFFFF00,   0,   0,   0, ""},
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
            "signalstats=out=vrep:c=${2}",
            "signalstats=out=vrep:c=${2},il=l=d:c=d",
        },
    },
    {
        "Frame Tiles",
        0,
        {
            { Args_Type_Slider,   2,   1,   12,   1, "Width"},
            { Args_Type_Slider,   2,   1,   12,   1, "Height"},
            { Args_Type_Toggle,   0,   0,    0,   0, "Field" },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
            "scale=iw/${1}:ih/${2},tile=${1}x${2}",
            "il=l=d:c=d,scale=iw/${1}:ih/${2},tile=${1}x${2}"
        },
    },
    {
        "Line Over Time",
        0,
        {
            { Args_Type_Slider,   1,   1,   0,   1, "Line" },
            { Args_Type_Slider, 480,  20,1080,   1, "Count"},
            { Args_Type_Toggle,   0,   0,   0,   0, "Show Two Lines" },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
            "crop=iw:1:0:${1}:0:1,tile=1x${2}:overlap=${2}-1:init_padding=${2}-1",
            "split[a][b];[a]crop=iw:1:0:${1}:0:1,tile=1x${2}:overlap=${2}-1:init_padding=${2}-1,pad=iw:ih+1[a1];[b]crop=iw:1:0:${1}+1:0:1,tile=1x${2}:overlap=${2}-1:init_padding=${2}-1[b1];[a1][b1]vstack"
        },
    },
    {
        "Zoom",
        0,
        {
            { Args_Type_Slider,  20,   0,   0,   1, "x" },
            { Args_Type_Slider,  20,   0,   0,   1, "y" },
            { Args_Type_Slider, 120,  16,   0,   1, "w" },
            { Args_Type_Slider, 120,  16,   0,   1, "h" },
            { Args_Type_Slider,   0,   0,  10,  10, "Strength" },
            //{ Args_Type_Slider,   0,   0,  10,  10, "Intensity" },
            { Args_Type_Toggle,   0,   0,   0,   0, "Field" },
            { Args_Type_Toggle,   1,   0,   0,   0, "Zoom"},
        },
        {
            "scale=${width}:${height}:flags=neighbor,histeq=strength=${5},setsar=1/1",
            "crop=x=${1}:y=${2}:w=${3}:h=${4},scale=${width}:${height}:flags=neighbor,histeq=strength=${5},setsar=1/1",
            "il=l=d:c=d,scale=${width}:${height}:flags=neighbor,histeq=strength=${5},setsar=1/1",
            "il=l=d:c=d,crop=x=${1}:y=${2}:w=${3}:h=${4},scale=${width}:${height}:flags=neighbor,histeq=strength=${5},setsar=1/1",
        },
    },
    {
        "Corners",
        0,
        {
            { Args_Type_Slider,  24,   1,  48,   1, "W" },
            { Args_Type_Slider,  24,   1,  48,   1, "H" },
            { Args_Type_Slider,   0,   0,  10,  10, "Strength" },
            { Args_Type_Toggle,   0,   0,   0,   0, "Field" },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
            "split=4[a][b][c][d];[a]crop=w=${1}:h=${2}:x=0:y=0,histeq=strength=${3},scale=${1}*16:${2}*16:flags=neighbor,drawgrid=w=iw/${1}:h=ih/${2}:t=1:c=green@0.5[a1];[b]crop=w=${1}:h=${2}:x=iw-${1}:y=0,histeq=strength=${3},scale=${1}*16:${2}*16:flags=neighbor,drawgrid=w=iw/${1}:h=ih/${2}:t=1:c=green@0.5[b1];[c]crop=w=${1}:h=${2}:x=0:y=ih-${2},histeq=strength=${3},scale=${1}*16:${2}*16:flags=neighbor,drawgrid=w=iw/${1}:h=ih/${2}:t=1:c=green@0.5[c1];[d]crop=w=${1}:h=${2}:x=iw-${1}:y=ih-${2},histeq=strength=${3},scale=${1}*16:${2}*16:flags=neighbor,drawgrid=w=iw/${1}:h=ih/${2}:t=1:c=green@0.5[d1];[a1][b1]hstack[ab];[c1][d1]hstack[cd];[ab][cd]vstack,setsar=1/1,drawgrid=w=iw/2:h=ih/2:t=2:c=blue@0.5",
            "split=4[a][b][c][d];[a]crop=w=${1}:h=${2}:x=0:y=0,il=l=d:c=d,histeq=strength=${3},scale=${1}*16:${2}*16:flags=neighbor,drawgrid=w=iw/${1}:h=ih/${2}:t=1:c=green@0.5[a1];[b]crop=w=${1}:h=${2}:x=iw-${1}:y=0,il=l=d:c=d,histeq=strength=${3},scale=${1}*16:${2}*16:flags=neighbor,drawgrid=w=iw/${1}:h=ih/${2}:t=1:c=green@0.5[b1];[c]crop=w=${1}:h=${2}:x=0:y=ih-${2},il=l=d:c=d,histeq=strength=${3},scale=${1}*16:${2}*16:flags=neighbor,drawgrid=w=iw/${1}:h=ih/${2}:t=1:c=green@0.5[c1];[d]crop=w=${1}:h=${2}:x=iw-${1}:y=ih-${2},il=l=d:c=d,histeq=strength=${3},scale=${1}*16:${2}*16:flags=neighbor,drawgrid=w=iw/${1}:h=ih/${2}:t=1:c=green@0.5[d1];[a1][b1]hstack[ab];[c1][d1]hstack[cd];[ab][cd]vstack,setsar=1/1,drawgrid=w=iw/2:h=ih/4:t=2:c=blue@0.5",
        },
    },
    {
        "EIA608 VITC Viewer",
        0,
        {
            { Args_Type_Slider,   2,   0,  50, 100, "msd" },
            { Args_Type_Slider,  30,   1, 100,   1, "scan_max" },
            { Args_Type_Toggle,   0,   0,   0,   0, "parity" },
            { Args_Type_Toggle,   0,   0,   0,   0, "zoom" },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
            "readvitc=scan_max=${2},readeia608=scan_max=${2}:msd=${1},drawtext=fontfile=${fontfile}:fontcolor=white:fontsize=36:box=1:boxcolor=black@0.5:x=(w-tw)/2:y=h*3/4-ascent:text=Line %{metadata\\\\:lavfi.readeia608.0.line\\\\:-} %{metadata\\\\:lavfi.readeia608.0.cc\\\\:------} - Line %{metadata\\\\:lavfi.readeia608.1.line\\\\:-} %{metadata\\\\:lavfi.readeia608.1.cc\\\\:------},drawtext=fontfile=${fontfile}:fontcolor=white:fontsize=36:box=1:boxcolor=black@0.5:x=(w-tw)/2:y=h*3/4-ascent*3:text=VITC %{metadata\\\\:lavfi.readvitc.tc_str\\\\:-- -- -- --}",
            "readvitc=scan_max=${2},readeia608=scan_max=${2}:msd=${1},crop=iw:${2}:0:0,scale=${width}:${height}:flags=neighbor,drawtext=fontfile=${fontfile}:fontcolor=white:fontsize=36:box=1:boxcolor=black@0.5:x=(w-tw)/2:y=h*3/4-ascent:text=Line %{metadata\\\\:lavfi.readeia608.0.line\\\\:-} %{metadata\\\\:lavfi.readeia608.0.cc\\\\:------} - Line %{metadata\\\\:lavfi.readeia608.1.line\\\\:-} %{metadata\\\\:lavfi.readeia608.1.cc\\\\:------},drawtext=fontfile=${fontfile}:fontcolor=white:fontsize=36:box=1:boxcolor=black@0.5:x=(w-tw)/2:y=h*3/4-ascent*3:text=VITC %{metadata\\\\:lavfi.readvitc.tc_str\\\\:-- -- -- --}",
            "readvitc=scan_max=${2},readeia608=scan_max=${2}:msd=${1}:chp=1,drawtext=fontfile=${fontfile}:fontcolor=white:fontsize=36:box=1:boxcolor=black@0.5:x=(w-tw)/2:y=h*3/4-ascent:text=Line %{metadata\\\\:lavfi.readeia608.0.line\\\\:-} %{metadata\\\\:lavfi.readeia608.0.cc\\\\:------} - Line %{metadata\\\\:lavfi.readeia608.1.line\\\\:-} %{metadata\\\\:lavfi.readeia608.1.cc\\\\:------},drawtext=fontfile=${fontfile}:fontcolor=white:fontsize=36:box=1:boxcolor=black@0.5:x=(w-tw)/2:y=h*3/4-ascent*3:text=VITC %{metadata\\\\:lavfi.readvitc.tc_str\\\\:-- -- -- --}",
            "readvitc=scan_max=${2},readeia608=scan_max=${2}:msd=${1}:chp=1,crop=iw:${2}:0:0,scale=${width}:${height}:flags=neighbor,drawtext=fontfile=${fontfile}:fontcolor=white:fontsize=36:box=1:boxcolor=black@0.5:x=(w-tw)/2:y=h*3/4-ascent:text=Line %{metadata\\\\:lavfi.readeia608.0.line\\\\:-} %{metadata\\\\:lavfi.readeia608.0.cc\\\\:------} - Line %{metadata\\\\:lavfi.readeia608.1.line\\\\:-} %{metadata\\\\:lavfi.readeia608.1.cc\\\\:------},drawtext=fontfile=${fontfile}:fontcolor=white:fontsize=36:box=1:boxcolor=black@0.5:x=(w-tw)/2:y=h*3/4-ascent*3:text=VITC %{metadata\\\\:lavfi.readvitc.tc_str\\\\:-- -- -- --}",
        },
    },
    {
        "(Separator)",
        -1,
        {
            { Args_Type_None,   0, 0, 0, 0, nullptr },
            { Args_Type_None,   0, 0, 0, 0, nullptr },
            { Args_Type_None,   0, 0, 0, 0, nullptr },
            { Args_Type_None,   0, 0, 0, 0, nullptr },
            { Args_Type_None,   0, 0, 0, 0, nullptr },
            { Args_Type_None,   0, 0, 0, 0, nullptr },
            { Args_Type_None,   0, 0, 0, 0, nullptr },
        },
        {
            "",
        },
    },
    {
        "Audio Spectrum",
        1,
        {
            { Args_Type_Slider,   1, -10,  10,   1, "Saturation" },
            { Args_Type_Win_Func, 0,   0,   0,   0, "Win Func" },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
            "showspectrum=slide=scroll:mode=separate:color=intensity:saturation=${1}:win_func=${2}",
        },
    },
    {
        "Audio Waveform",
        1,
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Split Channels" },
            { Args_Type_Slider,   2,   1,  20,   1, "Samples per column"},
            { Args_Type_Wave_Mode,2,   0,   0,   0, "Mode" },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
            "showwaves=mode=${3}:n=${2}:s=${width}x${height}:split_channels=0,negate",
            "showwaves=mode=${3}:n=${2}:s=${width}x${height}:split_channels=1,negate",
        },
    },
    {
        "Show CQT",
        1,
        {
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
            "showcqt=fullhd=0",
        },
    },
    {
        "Audio Vectorscope",
        1,
        {
            { Args_Type_Slider,   1,   1,  10,   1, "Zoom" },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
            "avectorscope=m=lissajous:s=512x512:zoom=${1}",
        },
    },
    {
        "Audio Phase Meter",
        1,
        {
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
            "aphasemeter=mpc=red:video=1[out0][out1];[out0]anullsink;[out1]copy",
        },
    },
    {
        "Audio Frequency",
        1,
        {
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
            "showfreqs=mode=line:win_size=w1024",
        },
    },
    {
        "Audio Volume",
        1,
        {
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
            "showvolume",
        },
    },
    {
        "Audio Bit Scope",
        1,
        {
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
            "abitscope,drawgrid=w=iw:h=ih/8:t=1:c=gray@0.9",
        },
    },
    {
        "EBU R128 Loudness Meter",
        1,
        {
            { Args_Type_Slider,   9,   9,  18,   1, "Scale Meter"},
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
            "ebur128=video=1:meter=${1}[out0][out1];[out1]anullsink;[out0]copy",
        },
    },
    {
        "(End)",
        -1,
        {
            { Args_Type_None,   0, 0, 0, 0, nullptr },
            { Args_Type_None,   0, 0, 0, 0, nullptr },
            { Args_Type_None,   0, 0, 0, 0, nullptr },
            { Args_Type_None,   0, 0, 0, 0, nullptr },
            { Args_Type_None,   0, 0, 0, 0, nullptr },
            { Args_Type_None,   0, 0, 0, 0, nullptr },
            { Args_Type_None,   0, 0, 0, 0, nullptr },
        },
        {
            "",
        },
    },
};

//---------------------------------------------------------------------------

//***************************************************************************
// Helper
//***************************************************************************

//***************************************************************************
// Constructor / Destructor
//***************************************************************************

//---------------------------------------------------------------------------
BigDisplay::BigDisplay(QWidget *parent, FileInformation* FileInformationData_) :
    QDialog(parent),
    FileInfoData(FileInformationData_)
{
    setlocale(LC_NUMERIC, "C");
    setWindowTitle("QCTools - "+FileInfoData->fileName());
    setWindowFlags(windowFlags() | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint);
    setWindowFlags(windowFlags() &(0xFFFFFFFF-Qt::WindowContextHelpButtonHint));
    resize(QDesktopWidget().screenGeometry().width()*2/5, QDesktopWidget().screenGeometry().height()*2/5);

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
        Options[Pos].FiltersList->setProperty("playerIndex", (int) Pos); // unfortunately QVariant doesn't support size_t
        Options[Pos].FiltersList->setFont(Font);

        typedef QPair<QString, int> FilterInfo;
        typedef QList<FilterInfo> FiltersGroup;
        typedef QVector<FiltersGroup> FiltersGroups;

        struct Sort
        {
            static bool filterInfoLessThan(const FilterInfo& i1, const FilterInfo& i2)
            {
                return i1.first < i2.first;
            }
        };

        FiltersGroups filtersGroups;

        for (size_t FilterPos=0; FilterPos<FiltersListDefault_Count; FilterPos++)
        {
            const char* filterName = Filters[FilterPos].Name;

            if (strcmp(filterName, "(Separator)") && strcmp(filterName, "(End)"))
            {
                if(filtersGroups.empty())
                    filtersGroups.push_back(FiltersGroup());

                filtersGroups.back().append(FilterInfo(filterName, FilterPos));
            }
            else if (strcmp(Filters[FilterPos].Name, "(End)"))
            {
                filtersGroups.push_back(FiltersGroup());
            }
        }

        for(int i = 0; i < filtersGroups.length(); ++i)
        {
            FiltersGroup & filterGroup = filtersGroups[i];
            qSort(filterGroup.begin(), filterGroup.end(), Sort::filterInfoLessThan);

            for(FiltersGroup::const_iterator it = filterGroup.cbegin(); it != filterGroup.cend(); ++it)
            {
                 Options[Pos].FiltersList->addItem(it->first, it->second);
            }

            if(i != (filtersGroups.length() - 1))
                Options[Pos].FiltersList->insertSeparator(FiltersListDefault_Count);
        };

        Options[Pos].FiltersList->setMinimumWidth(Options[Pos].FiltersList->minimumSizeHint().width());
        Options[Pos].FiltersList->setMaximumWidth(Options[Pos].FiltersList->minimumSizeHint().width());
        Options[Pos].FiltersList->setMaxVisibleItems(25);

    }

    splitter = new QSplitter;
    splitter->setStyleSheet("QSplitter::handle { background-color: gray }");
    splitter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Players
    for (size_t Pos=0; Pos<2; Pos++)
    {
        imageLabels[Pos] = new PlayerWindow();
        imageLabels[Pos]->setFile(FileInformationData_->fileName());

        if(Config::instance().getDebug())
            imageLabels[Pos]->setStyleSheet("background: yellow");

        imageLabels[Pos]->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        imageLabels[Pos]->setMinimumSize(20, 20);
//        imageLabels[Pos]->showDebugOverlay(Config::instance().getDebug());

        splitter->addWidget(imageLabels[Pos]);
//        imageLabels[Pos]->installEventFilter(this);
    }

    splitter->handle(1)->installEventFilter(this);
    splitter->installEventFilter(this);
    connect(splitter, &QSplitter::splitterMoved, this, [&] {
            qDebug() << "splitter moved";
            timer.start();
    });

    Layout->addWidget(splitter, 1, 0, 1, 3);

    // Info
    InfoArea=NULL;
    //InfoArea=new Info(this, FileInfoData, Info::Style_Columns);
    //Layout->addWidget(InfoArea, 1, 1, 1, 3, Qt::AlignHCenter);
    //Layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding), 2, 1, 1, 3, Qt::AlignHCenter);

    // Slider
    Slider=new QSlider(Qt::Horizontal);
    Slider->setMaximum(FileInfoData->Glue->VideoFrameCount_Get() - 1);

    connect(Slider, SIGNAL(valueChanged(int)), this, SLOT(onSliderValueChanged(int)));
    connect(Slider, SIGNAL(valueChanged(int)), this, SLOT(onCursorMoved(int)));

    Layout->addWidget(Slider, 2, 0, 1, 3);

    plot = createCommentsPlot(FileInformationData_, nullptr);
    plot->enableAxis(QwtPlot::yLeft, false);
    plot->enableAxis(QwtPlot::xBottom, true);
    plot->setAxisScale(QwtPlot::xBottom, Slider->minimum(), Slider->maximum());
    plot->setAxisAutoScale(QwtPlot::xBottom, false);

    plot->setFrameShape(QFrame::NoFrame);
    plot->setObjectName("commentsPlot");
    plot->setStyleSheet("#commentsPlot { border: 0px solid transparent; }");
    plot->canvas()->setObjectName("commentsPlotCanvas");
    dynamic_cast<QFrame*>(plot->canvas())->setFrameStyle( QFrame::NoFrame );
    dynamic_cast<QFrame*>(plot->canvas())->setContentsMargins(0, 0, 0, 0);

    connect( plot, SIGNAL( cursorMoved( int ) ), SLOT( onCursorMoved( int ) ) );
    plot->canvas()->installEventFilter( this );

    // Notes
    Layout->addWidget(
                plot,
                3, 0, 1, 3, Qt::AlignBottom);

    // Control
    ControlArea=new Control(this, FileInfoData, true);
    Layout->addWidget(ControlArea, 4, 0, 1, 3, Qt::AlignBottom);

    for (size_t Pos=0; Pos<2; Pos++)
    {
        connect(ControlArea, &Control::playClicked, imageLabels[Pos], &PlayerWindow::play);
        connect(ControlArea, &Control::stopClicked, imageLabels[Pos], &PlayerWindow::pause);
    }

    setLayout(Layout);

    // Picture
    Picture=NULL;
    Picture_Current[0] = Filters_Default1;
    Picture_Current[1] = Filters_Default2;

    for(int playerIndex = 0; playerIndex < 2; ++playerIndex)
    {
        for(int displayFilterIndex = 0; displayFilterIndex < Options[playerIndex].FiltersList->count(); ++displayFilterIndex)
        {
            int physicalFilterIndex = Options[playerIndex].FiltersList->itemData(displayFilterIndex).toInt();
            if((physicalFilterIndex == Filters_Default1 && playerIndex == 0) || (physicalFilterIndex == Filters_Default2 && playerIndex == 1))
                Options[playerIndex].FiltersList->setCurrentIndex(displayFilterIndex);
        }

        connect(Options[playerIndex].FiltersList, SIGNAL(currentIndexChanged(int)), this, SLOT(on_FiltersList_currentIndexChanged(int)));
    }

    // Info
    Frames_Pos=-1;
    ShouldUpate=false;

    // Shortcuts
    QShortcut *shortcutJ = new QShortcut(QKeySequence(Qt::Key_J), this);
    QObject::connect(shortcutJ, SIGNAL(activated()), ControlArea->M1, SLOT(click()));
    QShortcut *shortcutLeft = new QShortcut(QKeySequence(Qt::Key_Left), this);
    QObject::connect(shortcutLeft, SIGNAL(activated()), ControlArea->Minus, SLOT(click()));
    QShortcut *shortcutK = new QShortcut(QKeySequence(Qt::Key_K), this);
    QObject::connect(shortcutK, SIGNAL(activated()), ControlArea->Pause, SLOT(click()));
    QShortcut *shortcutRight = new QShortcut(QKeySequence(Qt::Key_Right), this);
    QObject::connect(shortcutRight, SIGNAL(activated()), ControlArea->Plus, SLOT(click()));
    QShortcut *shortcutL = new QShortcut(QKeySequence(Qt::Key_L), this);
    QObject::connect(shortcutL, SIGNAL(activated()), ControlArea->P1, SLOT(click()));
    QShortcut *shortcutSpace = new QShortcut(QKeySequence(Qt::Key_Space), this);
    QObject::connect(shortcutSpace, SIGNAL(activated()), ControlArea->PlayPause, SLOT(click()));
    QShortcut *shortcutF = new QShortcut(QKeySequence(Qt::Key_F), this);
    QObject::connect(shortcutF, SIGNAL(activated()), this, SLOT(on_Full_triggered()));

    timer.setInterval(10);
    timer.setSingleShot(true);
    connect(&timer, SIGNAL(timeout()), this, SLOT(onAfterResize()));
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
            //Special case : default for Color Matrix is Width/2
            if (Filters[FilterPos].Args[OptionPos].Name && string(Filters[FilterPos].Args[OptionPos].Name)=="Reveal")
                PreviousValues[Pos][FilterPos].Values[OptionPos]=FileInfoData->Glue->Width_Get()/2;
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
                                    QString MaxTemp(Filters[FilterPos].Args[OptionPos].Name);
                                    if(strcmp(Filters[FilterPos].Name, "Limiter") == 0)
                                    {
                                        int BitsPerRawSample = FileInfoData->Glue->BitsPerRawSample_Get();
                                        if (BitsPerRawSample == 0) {
                                            BitsPerRawSample = 8; //Workaround when BitsPerRawSample is unknown, we hope it is 8-bit.
                                        }
                                        Max = pow(2, BitsPerRawSample) - 1;
                                        if (Filters[FilterPos].Args[OptionPos].Name && string(Filters[FilterPos].Args[OptionPos].Name)=="Max")
                                            PreviousValues[Pos][FilterPos].Values[OptionPos]=pow(2, BitsPerRawSample) - 1;
                                    } else
                                    if (MaxTemp == "Line")
                                    {
                                        bool SelectWidth = false;
                                        for (size_t OptionPos2 = 0; OptionPos2 < Args_Max; OptionPos2++)
                                            if (Filters[FilterPos].Args[OptionPos2].Type != Args_Type_None && string(Filters[FilterPos].Args[OptionPos2].Name) == "Vertical")
                                                SelectWidth = Filters[FilterPos].Args[OptionPos2].Default ? true : false;
                                        Max = SelectWidth ? FileInfoData->Glue->Width_Get() : FileInfoData->Glue->Height_Get();
                                    }
                                    else if (MaxTemp == "x" || MaxTemp == "x offset" || MaxTemp == "Reveal" || MaxTemp == "w")
                                        Max = FileInfoData->Glue->Width_Get();
                                    else if (MaxTemp == "y" || MaxTemp == "s" || MaxTemp == "h")
                                        Max = FileInfoData->Glue->Height_Get();
                                    else if (MaxTemp.contains("bit position", Qt::CaseInsensitive) && FileInfoData->Glue->BitsPerRawSample_Get() != 0)
                                        Max = FileInfoData->Glue->BitsPerRawSample_Get();
                                    else
                                        Max=Filters[FilterPos].Args[OptionPos].Max;

                                    Options[Pos].Sliders_Label[OptionPos]=new QLabel(Filters[FilterPos].Args[OptionPos].Name+QString(": "));
                                    Options[Pos].Sliders_SpinBox[OptionPos]=new DoubleSpinBoxWithSlider(Filters[FilterPos].Args[OptionPos].Min, Max, Filters[FilterPos].Args[OptionPos].Divisor, PreviousValues[Pos][FilterPos].Values[OptionPos], Filters[FilterPos].Args[OptionPos].Name, Pos, QString(Filters[FilterPos].Args[OptionPos].Name).contains(" bit position"), QString(Filters[FilterPos].Args[OptionPos].Name).contains("Filter"), QString(Filters[FilterPos].Args[OptionPos].Name).contains("Peak"), QString(Filters[FilterPos].Args[OptionPos].Name).contains("Mode"), QString(Filters[FilterPos].Args[OptionPos].Name).contains("Scale"), QString(Filters[FilterPos].Args[OptionPos].Name).contains("Colorspace"), QString(Filters[FilterPos].Args[OptionPos].Name).contains("DataMode"), QString(Filters[FilterPos].Args[OptionPos].Name).contains("System") || QString(Filters[FilterPos].Args[OptionPos].Name).contains("Gamut"), this);
                                    hideOthersOnEntering(Options[Pos].Sliders_SpinBox[OptionPos], Options[Pos].Sliders_SpinBox);

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
            case Args_Type_Win_Func:
                                    Options[Pos].Radios_Group[OptionPos]=new QButtonGroup();
                                    for (size_t OptionPos2=0; OptionPos2<4; OptionPos2++)
                                    {
                                        Options[Pos].Radios[OptionPos][OptionPos2]=new QRadioButton();
                                        Options[Pos].Radios[OptionPos][OptionPos2]->setFont(Font);
                                        switch (OptionPos2)
                                        {
                                            case 0: Options[Pos].Radios[OptionPos][OptionPos2]->setText("none"); break;
                                            case 1: Options[Pos].Radios[OptionPos][OptionPos2]->setText("hann"); break;
                                            case 2: Options[Pos].Radios[OptionPos][OptionPos2]->setText("hamming"); break;
                                            case 3: Options[Pos].Radios[OptionPos][OptionPos2]->setText("blackman"); break;
                                            default:;
                                        }
                                        if (OptionPos2==PreviousValues[Pos][FilterPos].Values[OptionPos])
                                            Options[Pos].Radios[OptionPos][OptionPos2]->setChecked(true);
                                        connect(Options[Pos].Radios[OptionPos][OptionPos2], SIGNAL(toggled(bool)), this, Pos==0?(SLOT(on_FiltersOptions1_toggle(bool))):SLOT(on_FiltersOptions2_toggle(bool)));
                                        Layout0->addWidget(Options[Pos].Radios[OptionPos][OptionPos2], 0, Widget_XPox+OptionPos2);
                                        Options[Pos].Radios_Group[OptionPos]->addButton(Options[Pos].Radios[OptionPos][OptionPos2]);
                                    }
                                    Widget_XPox+=4;
                                    break;
            case Args_Type_Wave_Mode:
                                    Options[Pos].Radios_Group[OptionPos]=new QButtonGroup();
                                    for (size_t OptionPos2=0; OptionPos2<4; OptionPos2++)
                                    {
                                        Options[Pos].Radios[OptionPos][OptionPos2]=new QRadioButton();
                                        Options[Pos].Radios[OptionPos][OptionPos2]->setFont(Font);
                                        switch (OptionPos2)
                                        {
                                            case 0: Options[Pos].Radios[OptionPos][OptionPos2]->setText("point"); break;
                                            case 1: Options[Pos].Radios[OptionPos][OptionPos2]->setText("line"); break;
                                            case 2: Options[Pos].Radios[OptionPos][OptionPos2]->setText("p2p"); break;
                                            case 3: Options[Pos].Radios[OptionPos][OptionPos2]->setText("cline"); break;
                                            default:;
                                        }
                                        if (OptionPos2==PreviousValues[Pos][FilterPos].Values[OptionPos])
                                            Options[Pos].Radios[OptionPos][OptionPos2]->setChecked(true);
                                        connect(Options[Pos].Radios[OptionPos][OptionPos2], SIGNAL(toggled(bool)), this, Pos==0?(SLOT(on_FiltersOptions1_toggle(bool))):SLOT(on_FiltersOptions2_toggle(bool)));
                                        Layout0->addWidget(Options[Pos].Radios[OptionPos][OptionPos2], 0, Widget_XPox+OptionPos2);
                                        Options[Pos].Radios_Group[OptionPos]->addButton(Options[Pos].Radios[OptionPos][OptionPos2]);
                                    }
                                    Widget_XPox+=4;
                                    break;
            case Args_Type_Yuv:
            case Args_Type_YuvA:
                                    Options[Pos].Radios_Group[OptionPos]=new QButtonGroup();
                                    for (size_t OptionPos2=0; OptionPos2<(Filters[FilterPos].Args[OptionPos].Type==Args_Type_Yuv?3:4); OptionPos2++)
                                    {
                                        Options[Pos].Radios[OptionPos][OptionPos2]=new QRadioButton();
                                        Options[Pos].Radios[OptionPos][OptionPos2]->setFont(Font);
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
            case Args_Type_Ranges:
                                    Options[Pos].Radios_Group[OptionPos]=new QButtonGroup();
                                    for (size_t OptionPos2=0; OptionPos2<2; OptionPos2++)
                                    {
                                        Options[Pos].Radios[OptionPos][OptionPos2]=new QRadioButton();
                                        Options[Pos].Radios[OptionPos][OptionPos2]->setFont(Font);
                                        switch (OptionPos2)
                                        {
                                            case 0: Options[Pos].Radios[OptionPos][OptionPos2]->setText("above white"); break;
                                            case 1: Options[Pos].Radios[OptionPos][OptionPos2]->setText("below black"); break;
                                            default:;
                                        }
                                        if (OptionPos2==PreviousValues[Pos][FilterPos].Values[OptionPos])
                                            Options[Pos].Radios[OptionPos][OptionPos2]->setChecked(true);
                                        connect(Options[Pos].Radios[OptionPos][OptionPos2], SIGNAL(toggled(bool)), this, Pos==0?(SLOT(on_FiltersOptions1_toggle(bool))):SLOT(on_FiltersOptions2_toggle(bool)));
                                        Layout0->addWidget(Options[Pos].Radios[OptionPos][OptionPos2], 0, Widget_XPox+OptionPos2);
                                        Options[Pos].Radios_Group[OptionPos]->addButton(Options[Pos].Radios[OptionPos][OptionPos2]);
                                    }
                                    Widget_XPox+=2;
                                    break;
            case Args_Type_ColorMatrix:
                                    Options[Pos].Sliders_Label[OptionPos]=new QLabel(Filters[FilterPos].Args[OptionPos].Name+QString(": "));
                                    Layout0->addWidget(Options[Pos].Sliders_Label[OptionPos], 0, Widget_XPox);
                                    Options[Pos].Radios_Group[OptionPos]=new QButtonGroup();
                                    for (size_t OptionPos2=0; OptionPos2<4; OptionPos2++)
                                    {
                                        Options[Pos].Radios[OptionPos][OptionPos2]=new QRadioButton();
                                        Options[Pos].Radios[OptionPos][OptionPos2]->setFont(Font);
                                        switch (OptionPos2)
                                        {
                                            case 0: Options[Pos].Radios[OptionPos][OptionPos2]->setText("bt601"); break;
                                            case 1: Options[Pos].Radios[OptionPos][OptionPos2]->setText("bt709"); break;
                                            case 2: Options[Pos].Radios[OptionPos][OptionPos2]->setText("smpte240m"); break;
                                            case 3: Options[Pos].Radios[OptionPos][OptionPos2]->setText("fcc"); break;
                                            default:;
                                        }
                                        if (OptionPos2==PreviousValues[Pos][FilterPos].Values[OptionPos])
                                            Options[Pos].Radios[OptionPos][OptionPos2]->setChecked(true);
                                        connect(Options[Pos].Radios[OptionPos][OptionPos2], SIGNAL(toggled(bool)), this, Pos==0?(SLOT(on_FiltersOptions1_toggle(bool))):SLOT(on_FiltersOptions2_toggle(bool)));
                                        Layout0->addWidget(Options[Pos].Radios[OptionPos][OptionPos2], 0, Widget_XPox+1+OptionPos2);
                                        Options[Pos].Radios_Group[OptionPos]->addButton(Options[Pos].Radios[OptionPos][OptionPos2]);
                                    }
                                    Widget_XPox+=5;
                                    break;
            case Args_Type_SampleRange:
                                    Options[Pos].Sliders_Label[OptionPos]=new QLabel(Filters[FilterPos].Args[OptionPos].Name+QString(": "));
                                    Layout0->addWidget(Options[Pos].Sliders_Label[OptionPos], 0, Widget_XPox);
                                    Options[Pos].Radios_Group[OptionPos]=new QButtonGroup();
                                    for (size_t OptionPos2=0; OptionPos2<3; OptionPos2++)
                                    {
                                        Options[Pos].Radios[OptionPos][OptionPos2]=new QRadioButton();
                                        Options[Pos].Radios[OptionPos][OptionPos2]->setFont(Font);
                                        switch (OptionPos2)
                                        {
                                            case 0: Options[Pos].Radios[OptionPos][OptionPos2]->setText("auto"); break;
                                            case 1: Options[Pos].Radios[OptionPos][OptionPos2]->setText("full"); break;
                                            case 2: Options[Pos].Radios[OptionPos][OptionPos2]->setText("broadcast"); break;
                                            default:;
                                        }
                                        if (OptionPos2==PreviousValues[Pos][FilterPos].Values[OptionPos])
                                            Options[Pos].Radios[OptionPos][OptionPos2]->setChecked(true);
                                        connect(Options[Pos].Radios[OptionPos][OptionPos2], SIGNAL(toggled(bool)), this, Pos==0?(SLOT(on_FiltersOptions1_toggle(bool))):SLOT(on_FiltersOptions2_toggle(bool)));
                                        Layout0->addWidget(Options[Pos].Radios[OptionPos][OptionPos2], 0, Widget_XPox+1+OptionPos2);
                                        Options[Pos].Radios_Group[OptionPos]->addButton(Options[Pos].Radios[OptionPos][OptionPos2]);
                                    }
                                    Widget_XPox+=4;
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
            case Args_Type_LogLin:
                                    //Options[Pos].Sliders_Label[OptionPos]=new QLabel(Filters[FilterPos].Args[OptionPos].Name+QString(": "));
                                    Layout0->addWidget(Options[Pos].Sliders_Label[OptionPos], 0, Widget_XPox);
                                    Options[Pos].Radios_Group[OptionPos]=new QButtonGroup();
                                    for (size_t OptionPos2=0; OptionPos2<2; OptionPos2++)
                                    {
                                        Options[Pos].Radios[OptionPos][OptionPos2]=new QRadioButton();
                                        Options[Pos].Radios[OptionPos][OptionPos2]->setFont(Font);
                                        switch (OptionPos2)
                                        {
                                            case 0: Options[Pos].Radios[OptionPos][OptionPos2]->setText("linear"); break;
                                            case 1: Options[Pos].Radios[OptionPos][OptionPos2]->setText("log"); break;
                                            default:;
                                        }
                                        if (OptionPos2==PreviousValues[Pos][FilterPos].Values[OptionPos])
                                            Options[Pos].Radios[OptionPos][OptionPos2]->setChecked(true);
                                        connect(Options[Pos].Radios[OptionPos][OptionPos2], SIGNAL(toggled(bool)), this, Pos==0?(SLOT(on_FiltersOptions1_toggle(bool))):SLOT(on_FiltersOptions2_toggle(bool)));
                                        Layout0->addWidget(Options[Pos].Radios[OptionPos][OptionPos2], 0, Widget_XPox+1+OptionPos2);
                                        Options[Pos].Radios_Group[OptionPos]->addButton(Options[Pos].Radios[OptionPos][OptionPos2]);
                                    }
                                    Widget_XPox++;
                                    break;
            default:                ;
        }
    }
}

//---------------------------------------------------------------------------
void BigDisplay::FiltersList_currentIndexChanged(size_t playerIndex, size_t FilterPos)
{
    // todo: consider consolidation this parts
    if(playerIndex == 0)
    {
        QGridLayout* Layout0=new QGridLayout();
        Layout0->setContentsMargins(0, 0, 0, 0);
        Layout0->setSpacing(8);

        Layout0->addWidget(Options[playerIndex].FiltersList, 0, 0, Qt::AlignLeft);
        FiltersList_currentIndexChanged(playerIndex, FilterPos, Layout0);

        Options[playerIndex].FiltersList_Fake=new QLabel(" ");
        Layout0->addWidget(Options[playerIndex].FiltersList_Fake, 1, 0, Qt::AlignLeft);
        Layout0->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum), 0, 14);
        Layout->addLayout(Layout0, 0, 0, 1, 1, Qt::AlignLeft|Qt::AlignTop);
    }
    else
    {
        QGridLayout* Layout0=new QGridLayout();
        Layout0->setContentsMargins(0, 0, 0, 0);
        Layout0->setSpacing(8);
        Layout0->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum), 0, 0);

        FiltersList_currentIndexChanged(playerIndex, FilterPos, Layout0);
        Layout0->addWidget(Options[playerIndex].FiltersList, 0, 14, Qt::AlignRight);

        Options[playerIndex].FiltersList_Fake=new QLabel(" ");
        Layout0->addWidget(Options[playerIndex].FiltersList_Fake, 1, 14, Qt::AlignRight);
        Layout->addLayout(Layout0, 0, 2, 1, 1, Qt::AlignRight|Qt::AlignTop);
    }

    if (FilterPos >= 2)
    {
        imageLabels[playerIndex]->setVisible(true);
    }

    Picture_Current[playerIndex] = FilterPos;
    on_FiltersList_currentOptionChanged(playerIndex, FilterPos);

    // updateSelection(FilterPos, imageLabels[playerIndex], Options[playerIndex]);
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
                {
                    Modified = true;
                    double value = Options[Pos].Sliders_SpinBox[OptionPos]->value();
                    double divisor = Filters[Picture_Current].Args[OptionPos].Divisor;
                    WithSliders[OptionPos] = value;
                    PreviousValues[Pos][Picture_Current].Values[OptionPos] = value * divisor;
                }
                break;
            case Args_Type_Win_Func:
            case Args_Type_Wave_Mode:
                Modified=true;
                for (size_t OptionPos2=0; OptionPos2<4; OptionPos2++)
                {
                    if (Options[Pos].Radios[OptionPos][OptionPos2] && Options[Pos].Radios[OptionPos][OptionPos2]->isChecked())
                    {
                        WithRadios[OptionPos]=Options[Pos].Radios[OptionPos][OptionPos2]->text().toUtf8().data();
                        size_t X_Pos=WithRadios[OptionPos].find('x');
                        if (X_Pos!=string::npos)
                            WithRadios[OptionPos].resize(X_Pos);
                        PreviousValues[Pos][Picture_Current].Values[OptionPos]=OptionPos2;
                        break;
                    }
                }
                break;
            case Args_Type_YuvA:
                                    Value_Pos<<=1;
                                    Value_Pos|=Options[Pos].Radios[OptionPos][3]->isChecked()?1:0; // 3 = pos of "all"
                                    //No break
            case Args_Type_Yuv:
                                    Modified=true;
                                    for (size_t OptionPos2=0; OptionPos2<(Filters[Picture_Current].Args[OptionPos].Type?4:3); OptionPos2++)
                                    {
                                        if (Options[Pos].Radios[OptionPos][OptionPos2] && Options[Pos].Radios[OptionPos][OptionPos2]->isChecked())
                                        {
                                            if (string(Filters[Picture_Current].Name)=="Extract Planes Equalized" || string(Filters[Picture_Current].Name)=="Bit Plane Noise" || string(Filters[Picture_Current].Name)=="Value Highlight" || string(Filters[Picture_Current].Name)=="Field Difference" || string(Filters[Picture_Current].Name)=="Temporal Difference" || string(Filters[Picture_Current].Name)=="Bit Plane (10 slices)")
                                            {
                                                switch (OptionPos2)
                                                {
                                                    case 0: WithRadios[OptionPos]="y"; break;
                                                    case 1: WithRadios[OptionPos]="u"; break;
                                                    case 2: WithRadios[OptionPos]="v"; break;
                                                    default:;
                                                }
                                            }
                                            else
                                            {
                                                switch (OptionPos2)
                                                {
                                                    case 0: WithRadios[OptionPos]="1"; break;
                                                    case 1: WithRadios[OptionPos]="2"; break;
                                                    case 2: WithRadios[OptionPos]="4"; break;
                                                    case 3: WithRadios[OptionPos]="7"; break; //Special case: remove plane
                                                    default:;
                                                }
                                            }
                                            PreviousValues[Pos][Picture_Current].Values[OptionPos]=OptionPos2;
                                            break;
                                        }
                                    }
                                    break;
            case Args_Type_Ranges:
                                    Value_Pos<<=1;
                                    Value_Pos|=Options[Pos].Radios[OptionPos][1]->isChecked()?1:0;
                                    //No break
            case Args_Type_ClrPck:
                                    Modified=true;
                                    WithSliders[OptionPos]=Options[Pos].ColorValue[OptionPos];
                                    PreviousValues[Pos][Picture_Current].Values[OptionPos]=Options[Pos].ColorValue[OptionPos];
                                    break ;
            case Args_Type_ColorMatrix:
                                    Modified=true;
                                    for (size_t OptionPos2=0; OptionPos2<(Filters[Picture_Current].Args[OptionPos].Type?4:3); OptionPos2++)
                                    {
                                        if (Options[Pos].Radios[OptionPos][OptionPos2] && Options[Pos].Radios[OptionPos][OptionPos2]->isChecked())
                                        {
                                            switch (OptionPos2)
                                            {
                                                case 0: WithRadios[OptionPos]="bt601"; break;
                                                case 1: WithRadios[OptionPos]="bt709"; break;
                                                case 2: WithRadios[OptionPos]="smpte240m"; break;
                                                case 3: WithRadios[OptionPos]="fcc"; break;
                                                default:;
                                            }
                                            PreviousValues[Pos][Picture_Current].Values[OptionPos]=OptionPos2;
                                            break;
                                        }
                                    }
                                    break;
            case Args_Type_SampleRange:
                                    Modified=true;
                                    for (size_t OptionPos2=0; OptionPos2<(Filters[Picture_Current].Args[OptionPos].Type?4:3); OptionPos2++)
                                    {
                                        if (Options[Pos].Radios[OptionPos][OptionPos2] && Options[Pos].Radios[OptionPos][OptionPos2]->isChecked())
                                        {
                                            switch (OptionPos2)
                                            {
                                                case 0: WithRadios[OptionPos]="auto"; break;
                                                case 1: WithRadios[OptionPos]="full"; break;
                                                case 2: WithRadios[OptionPos]="tv"; break;
                                                default:;
                                            }
                                            PreviousValues[Pos][Picture_Current].Values[OptionPos]=OptionPos2;
                                            break;
                                        }
                                    }
                                    break;
            case Args_Type_LogLin:
                                    Modified=true;
                                    for (size_t OptionPos2=0; OptionPos2<(Filters[Picture_Current].Args[OptionPos].Type?4:3); OptionPos2++)
                                    {
                                        if (Options[Pos].Radios[OptionPos][OptionPos2] && Options[Pos].Radios[OptionPos][OptionPos2]->isChecked())
                                        {
                                            switch (OptionPos2)
                                            {
                                                case 0: WithRadios[OptionPos]="linear"; break;
                                                case 1: WithRadios[OptionPos]="logarithmic"; break;
                                                default:;
                                            }
                                            PreviousValues[Pos][Picture_Current].Values[OptionPos]=OptionPos2;
                                            break;
                                        }
                                    }
                                    break;
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
                case Args_Type_Win_Func:
                case Args_Type_Wave_Mode:
                case Args_Type_Yuv:
                case Args_Type_YuvA:
                case Args_Type_ClrPck:
                case Args_Type_ColorMatrix:
                case Args_Type_SampleRange:
                case Args_Type_LogLin:
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
    QString str = QString::fromStdString(Modified_String);

    str.replace(QString("${width}"), QString::number(FileInfoData->Glue->Width_Get()));
    str.replace(QString("${height}"), QString::number(FileInfoData->Glue->Height_Get()));
    str.replace(QString("${dar}"), QString::number(FileInfoData->Glue->DAR_Get()));

//    QSize windowSize = imageLabels[Pos]->pixmapSize();
    QSize windowSize = imageLabels[Pos]->size();

    str.replace(QString("${window_width}"), QString::number(windowSize.width()));
    str.replace(QString("${window_height}"), QString::number(windowSize.height()));

    QString tempLocation = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QDir tempDir(tempLocation);

    QString qctoolsTmpSubDir = "qctools";
    QString fontFileName = "Anonymous_Pro_B.ttf";

    if(tempDir.exists())
    {
        QDir qctoolsTmpDir(tempLocation + "/" + qctoolsTmpSubDir);
        if(!qctoolsTmpDir.exists())
            tempDir.mkdir(qctoolsTmpSubDir);

        QFile fontFile(qctoolsTmpDir.path() + "/" + fontFileName);
        if(!fontFile.exists())
        {
            QFile::copy(":/" + fontFileName, fontFile.fileName());
        }

        if(fontFile.exists())
        {
            QString fontFileName(fontFile.fileName());
            fontFileName = fontFileName.replace(":", "\\\\:"); // ":" is a reserved character, it must be escaped
            str.replace(QString("${fontfile}"), fontFileName);
        }
    }

    Modified_String = str.toStdString();

    return Modified_String;
}

//---------------------------------------------------------------------------
void BigDisplay::on_FiltersList_currentOptionChanged(size_t playerIndex, size_t filterIndex)
{
    string Modified_String=FiltersList_currentOptionChanged(playerIndex, filterIndex);
    Picture->Filter_Change(playerIndex, Filters[filterIndex].Type, Modified_String.c_str());

    Picture->FrameAtPosition(Frames_Pos);

    imageLabels[playerIndex]->setFilter(QString::fromStdString(Modified_String));
    /*
    if(imageLabels[playerIndex]->isVisible())
        imageLabels[playerIndex]->adjustScale();
        */
}

//***************************************************************************
// Actions
//***************************************************************************

//---------------------------------------------------------------------------
void BigDisplay::InitPicture()
{
    if (!Picture)
    {
        string FileName_string=FileInfoData->fileName().toUtf8().data();
        #ifdef _WIN32
            replace(FileName_string.begin(), FileName_string.end(), '/', '\\' );
        #endif
        int width=QDesktopWidget().screenGeometry().width()*2/5;
        if (width%2)
            width--; //odd number is wanted for filters
        int height=QDesktopWidget().screenGeometry().height()*2/5;
        if (height%2)
            height--; //odd number is wanted for filters
        Picture=new FFmpeg_Glue(FileName_string.c_str(), FileInfoData->ActiveAllTracks, &FileInfoData->Stats, NULL, NULL);
        Picture->setThreadSafe(true);

        if (FileName_string.empty())
            Picture->InputData_Set(FileInfoData->Glue->InputData_Get()); // Using data from the analyzed file
        Picture->AddOutput(0, width, height, FFmpeg_Glue::Output_QImage);
        Picture->AddOutput(1, width, height, FFmpeg_Glue::Output_QImage);

        setCurrentFilter(0, Picture_Current[0]);
        setCurrentFilter(1, Picture_Current[1]);
    }
}

QPixmap fromImage(const FFmpeg_Glue::Image& image)
{
    if(image.isNull())
        return QPixmap();

    return QPixmap::fromImage(QImage(image.data(), image.width(), image.height(), image.linesize(), QImage::Format_RGB888));
}

void BigDisplay::ShowPicture ()
{
    if (!isVisible())
        return;

	if (!Picture)
		return;

    if ((!ShouldUpate && Frames_Pos==FileInfoData->Frames_Pos_Get())
     || ( ShouldUpate && false)) // ToDo: try to optimize
        return;

    Frames_Pos=FileInfoData->Frames_Pos_Get();
    ShouldUpate=false;
    Picture->FrameAtPosition(Frames_Pos);

    if (QThread::currentThread() == thread())
    {
        updateImagesAndSlider(fromImage(Picture->Image_Get(0)), fromImage(Picture->Image_Get(1)), Frames_Pos);
    }
    else
    {
        QMetaObject::invokeMethod(this, "updateImagesAndSlider", Qt::BlockingQueuedConnection,
                                  Q_ARG(const QPixmap&, fromImage(Picture->Image_Get(0))),
                                  Q_ARG(const QPixmap&, fromImage(Picture->Image_Get(1))),
                                  Q_ARG(const int, Frames_Pos));
    }

    // Stats
    if (ControlArea)
        ControlArea->Update();
    if (InfoArea)
        InfoArea->Update();
}

//***************************************************************************
// Events
//***************************************************************************

//---------------------------------------------------------------------------
void BigDisplay::onSliderValueChanged(int value)
{
    Q_EMIT rewind(Slider->value());
}

//---------------------------------------------------------------------------
void BigDisplay::on_FiltersSource_stateChanged(int state)
{
    ShowPicture ();
}

//---------------------------------------------------------------------------
void BigDisplay::on_FiltersOptions1_click()
{
    on_FiltersList_currentOptionChanged(0, Picture_Current[0]);
}

//---------------------------------------------------------------------------
void BigDisplay::on_FiltersOptions2_click()
{
    on_FiltersList_currentOptionChanged(1, Picture_Current[1]);
}

//---------------------------------------------------------------------------
void BigDisplay::on_FiltersOptions1_toggle(bool checked)
{
    if (checked)
            on_FiltersList_currentOptionChanged(0, Picture_Current[0]);
}

//---------------------------------------------------------------------------
void BigDisplay::on_FiltersOptions2_toggle(bool checked)
{
    if (checked)
            on_FiltersList_currentOptionChanged(1, Picture_Current[1]);
}

//---------------------------------------------------------------------------
void BigDisplay::on_FiltersSpinBox1_click()
{
    on_FiltersList_currentOptionChanged(0, Picture_Current[0]);
}

//---------------------------------------------------------------------------
void BigDisplay::on_FiltersSpinBox2_click()
{
    on_FiltersList_currentOptionChanged(1, Picture_Current[1]);
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
        on_FiltersList_currentOptionChanged(0, Picture_Current[0]);
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
        on_FiltersList_currentOptionChanged(1, Picture_Current[1]);
    }
    hide();
    show();
}

//---------------------------------------------------------------------------
void BigDisplay::updateSelection(int Pos, ImageLabel* image, options& opts)
{
    image->disconnect();

    if(strcmp(Filters[Pos].Name, "Waveform Target") == 0 ||
            strcmp(Filters[Pos].Name, "Vectorscope Target") ==  0 ||
            strcmp(Filters[Pos].Name, "Zoom") ==  0 ||
            strcmp(Filters[Pos].Name, "Pixel Scope") == 0)
    {
        int xIndex = 0;
        int yIndex = 1;
        int wIndex = 2;
        int hIndex = 3;

        if(strcmp(Filters[Pos].Name, "Pixel Scope") == 0)
        {
            xIndex = 1;
            yIndex = 2;
            wIndex = 3;
            hIndex = 4;
        }

        auto& xSpinBox = opts.Sliders_SpinBox[xIndex];
        auto& ySpinBox = opts.Sliders_SpinBox[yIndex];
        auto& wSpinBox = opts.Sliders_SpinBox[wIndex];
        auto& hSpinBox = opts.Sliders_SpinBox[hIndex];

        image->setSelectionArea(xSpinBox->value(), ySpinBox->value(), wSpinBox->value(), hSpinBox->value());

        image->setMinSelectionSize(QSizeF(wSpinBox->minimum(), hSpinBox->minimum()));
        image->setMaxSelectionSize(QSizeF(wSpinBox->maximum(), hSpinBox->maximum()));

        connect(xSpinBox, &DoubleSpinBoxWithSlider::controlValueChanged, image, &ImageLabel::moveSelectionX);
        connect(ySpinBox, &DoubleSpinBoxWithSlider::controlValueChanged, image, &ImageLabel::moveSelectionY);
        connect(wSpinBox, &DoubleSpinBoxWithSlider::controlValueChanged, image, &ImageLabel::changeSelectionWidth);
        connect(hSpinBox, &DoubleSpinBoxWithSlider::controlValueChanged, image, &ImageLabel::changeSelectionHeight);

        connect(image, &ImageLabel::selectionChangeFinished, [&](const QRectF& geometry) {
            qDebug() << "selectionChangeFinished: "
                     << ", x: " << geometry.x() << ", y: " << geometry.y()
                     << ", w: " << geometry.width() << ", h: " << geometry.height();

            xSpinBox->applyValue(geometry.topLeft().x(), false);
            ySpinBox->applyValue(geometry.topLeft().y(), false);
            wSpinBox->applyValue(geometry.width(), false);
            hSpinBox->applyValue(geometry.height(), false);

        });

        connect(image, &ImageLabel::selectionChanged, [&](const QRectF& geometry) {
            qDebug() << "selectionChanged: "
                     << ", x: " << geometry.x() << ", y: " << geometry.y()
                     << ", w: " << geometry.width() << ", h: " << geometry.height();

            xSpinBox->applyValue(geometry.x(), false);
            ySpinBox->applyValue(geometry.y(), false);
            wSpinBox->applyValue(geometry.width(), false);
            hSpinBox->applyValue(geometry.height(), false);
        });
    }
    else
    {
        image->clearSelectionArea();
    }
}

void BigDisplay::setCurrentFilter(size_t playerIndex, size_t filterIndex)
{
    // Help
    if (filterIndex == 0)
    {
        Help* Frame=new Help(this);
        Frame->PlaybackFilters();
        return;
    }

    auto* imageLabel = imageLabels[playerIndex];

    // None
    if (filterIndex == 1)
    {
        Picture->Disable(playerIndex);
        imageLabel->Remove();
    }

    FiltersList_currentIndexChanged(playerIndex, filterIndex);
    // updateSelection(filterIndex, imageLabel, Options[playerIndex]);
}

void BigDisplay::hideOthersOnEntering(DoubleSpinBoxWithSlider *doubleSpinBox, DoubleSpinBoxWithSlider **others)
{
    connect(doubleSpinBox, &DoubleSpinBoxWithSlider::entered, [others](DoubleSpinBoxWithSlider* control) {
        for (size_t Pos=0; Pos<Args_Max; Pos++)
            if (others[Pos] && others[Pos] != control)
                others[Pos]->hidePopup();
    });
}

bool BigDisplay::eventFilter(QObject *watched, QEvent *event)
{
    if(watched == splitter && event->type() == QEvent::Resize)
    {
        qDebug() << "entering splitter: resize event";
        bool result = QWidget::eventFilter(watched, event);
        qDebug() << "leaving splitter: resize event";

        return result;
    }

    if((watched == imageLabels[0] || watched == imageLabels[1]) && event->type() == QEvent::Resize)
    {
        qDebug() << "entering imagelabel: resize event";
        bool result = QWidget::eventFilter(watched, event);
        qDebug() << "leaving imagelabel: resize event";
        return result;
    }

    if((watched == splitter || watched == splitter->handle(1)) && event->type() == QEvent::MouseButtonDblClick)
    {
        QList<int> sizes;

        int left = (width() - splitter->handle(0)->width()) / 2;
        int right = width() - splitter->handle(0)->width() - left;

        sizes << left << right;

        splitter->setSizes(sizes);
    } else if(watched == plot->canvas()) {

        if(event->type() == QEvent::MouseButtonDblClick)
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            if(mouseEvent->button() == Qt::LeftButton)
            {
                showEditFrameCommentsDialog(parentWidget(), FileInfoData, FileInfoData->ReferenceStat(), Slider->value());
            }
        } else if(event->type() == QEvent::KeyPress)
        {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            if(keyEvent->key() == Qt::Key_M)
            {
                showEditFrameCommentsDialog(parentWidget(), FileInfoData, FileInfoData->ReferenceStat(), Slider->value());
            }
        }
    }

    return QWidget::eventFilter(watched, event);
}

void BigDisplay::onCursorMoved(int x)
{
    plot->setCursorPos(x);
    Slider->setValue(x);
}

void BigDisplay::updateImagesAndSlider(const QPixmap &pixmap1, const QPixmap &pixmap2, int sliderPos)
{
    if (Slider->sliderPosition() != sliderPos)
        Slider->setSliderPosition(sliderPos);

    /*
    if(!pixmap1.isNull())
        imageLabels[0]->setPixmap(pixmap1);

    if(!pixmap2.isNull())
        imageLabels[1]->setPixmap(pixmap2);
        */
}

void BigDisplay::on_FiltersList_currentIndexChanged(int Pos)
{
    QComboBox* combo = qobject_cast<QComboBox*>(sender());
    if(combo)
    {
        if(!combo->itemData(Pos).isNull())
            Pos = combo->itemData(Pos).toInt();

        int playerIndex = combo->property("playerIndex").toInt();
        setCurrentFilter(playerIndex, Pos);
    }
}

//---------------------------------------------------------------------------
void BigDisplay::on_Full_triggered()
{
    if (isMaximized())
        setWindowState(Qt::WindowActive);
    else
        setWindowState(Qt::WindowMaximized);
}

void BigDisplay::onAfterResize()
{
    for(int playerIndex = 0; playerIndex < 2; ++playerIndex)
    {
        int filterIndex = Picture_Current[playerIndex];

        string Modified_String=FiltersList_currentOptionChanged(playerIndex, filterIndex);
        Picture->Filter_Change(playerIndex, Filters[filterIndex].Type, Modified_String.c_str());
    }

    Picture->FrameAtPosition(Frames_Pos);

    /*
    for(int playerIndex = 0; playerIndex < 2; ++playerIndex)
    {
        if(imageLabels[playerIndex]->isVisible())
            imageLabels[playerIndex]->adjustScale(true);
    }
    */
}

void BigDisplay::resizeEvent(QResizeEvent  *e)
{
    qDebug() << "entering BigDisplay::resizeEvent";

    QDialog::resizeEvent(e);
    timer.start();

    qDebug() << "leaving BigDisplay::resizeEvent";
}
