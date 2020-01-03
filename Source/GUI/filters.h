#ifndef FILTERS_H
#define FILTERS_H

#include <cstddef>

constexpr size_t Args_Max=7;

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

const filter Filters[] =
{
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
            { Args_Type_Toggle,   0,   0,   0,   0, "Metadata" },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
            "format=yuv444p,scale",
            "format=yuv444p,scale,drawtext=fontfile=${fontfile}:box=1:boxborderw=4:boxcolor=black@0.5:fontcolor=white:fontsize=16:x=4:y=4:text=PTS %{pts\\\\:hms}",
            "format=yuv444p,scale,il=l=d:c=d",
            "format=yuv444p,scale,il=l=d:c=d,drawtext=fontfile=${fontfile}:box=1:boxborderw=4:boxcolor=black@0.5:fontcolor=white:fontsize=16:x=4:y=4:text=PTS %{pts\\\\:hms}",
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
        "Histogram (Temporal)",
        0,
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field" },
            { Args_Type_Toggle,   0,   0,   0,   0, "Overlay" },
            { Args_Type_YuvA,     3,   0,   0,   0, "Plane" },
            { Args_Type_LogLin,   0,   0,   0,   0, "Levels" },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
            // field N, overlay N, all planes N
            "thistogram=components=${3}:levels_mode=${4}",
            // field N, overlay N, all planes Y
            "thistogram=levels_mode=${4}",
            // field N, overlay Y, all planes N
            "thistogram=display_mode=overlay:components=${3}:levels_mode=${4}",
            // field N, overlay Y, all planes Y
            "thistogram=display_mode=overlay:levels_mode=${4}",
            // field Y, overlay N, all planes N
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]thistogram=components=${3}:levels_mode=${4}[a2];[b1]thistogram=components=${3}:levels_mode=${4}[b2];[a2][b2]vstack",
            // field Y, overlay N, all planes Y
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]thistogram=levels_mode=${4}[a2];[b1]thistogram=levels_mode=${4}[b2];[a2][b2]hstack",
            // field Y, overlay Y, all planes N
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]thistogram=display_mode=overlay:components=${3}:levels_mode=${4}[a2];[b1]thistogram=display_mode=overlay:components=${3}:levels_mode=${4}[b2];[a2][b2]vstack",
            // field Y, overlay Y, all planes Y
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]thistogram=display_mode=overlay:levels_mode=${4}[a2];[b1]thistogram=display_mode=overlay:levels_mode=${4}[b2];[a2][b2]vstack",
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
            { Args_Type_Slider,   0,   0,   7,   1, "Filter" },
            { Args_Type_Slider,   0,   0,   2,   1, "Scale" },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
            // field N, all planes N, vertical N
            "waveform=intensity=${2}:mode=column:mirror=1:c=${3}:f=${5}:graticule=invert:flags=numbers+dots:scale=${6}",
            // field N, all planes N, vertical Y
            "waveform=intensity=${2}:mode=row:   mirror=1:c=${3}:f=${5}:graticule=invert:flags=numbers+dots:scale=${6}",
            // field N, all planes Y, vertical N
            "waveform=intensity=${2}:mode=column:mirror=1:c=${3}:f=${5}:graticule=invert:flags=numbers+dots:scale=${6}:display=overlay",
            // field N, all planes Y, vertical Y
            "waveform=intensity=${2}:mode=row:   mirror=1:c=${3}:f=${5}:graticule=invert:flags=numbers+dots:scale=${6}:display=overlay",
            // field Y, all planes N, vertical N
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]waveform=intensity=${2}:mode=column:mirror=1:c=${3}:f=${5}:graticule=invert:flags=numbers+dots:scale=${6}[a2];[b1]waveform=intensity=${2}:mode=column:mirror=1:c=${3}:f=${5}:graticule=invert:flags=numbers+dots:scale=${6}[b2];[a2][b2]vstack",
            // field Y, all planes N, vertical Y
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]waveform=intensity=${2}:mode=row:   mirror=1:c=${3}:f=${5}:graticule=invert:flags=numbers+dots:scale=${6}[a2];[b1]waveform=intensity=${2}:mode=row:   mirror=1:c=${3}:f=${5}:graticule=invert:flags=numbers+dots:scale=${6}[b2];[a2][b2]hstack",
            // field Y, all planes Y, vertical N
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]waveform=intensity=${2}:mode=column:mirror=1:c=${3}:f=${5}:graticule=invert:flags=numbers+dots:scale=${6}:display=overlay[a2];[b1]waveform=intensity=${2}:mode=column:mirror=1:c=${3}:f=${5}:graticule=invert:flags=numbers+dots:scale=${6}:display=overlay[b2];[a2][b2]vstack",
            // field Y, all planes Y, vertical Y
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]waveform=intensity=${2}:mode=row:   mirror=1:c=${3}:f=${5}:graticule=invert:flags=numbers+dots:scale=${6}:display=overlay[a2];[b1]waveform=intensity=${2}:mode=row:   mirror=1:c=${3}:f=${5}:graticule=invert:flags=numbers+dots:scale=${6}:display=overlay[b2];[b2][a2]hstack",
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
            { Args_Type_Slider,   0,   0,   7,   1, "Filter" },
            { Args_Type_Slider,   0,   0,   2,   1, "Scale" },
            { Args_Type_Toggle,   1,   0,   0,   0, "Background"},
        },
        {
            "crop=${3}:${4}:${1}:${2},\
            waveform=intensity=0.8:mode=column:mirror=1:c=1:f=${5}:graticule=invert:flags=numbers+dots:scale=${6},scale=${width}:${height},setsar=1/1",
            "split[a][b];\
            [a]lutyuv=y=val/4,scale=${width}:${height},setsar=1/1,format=yuv444p|yuv444p10le[a1];\
            [b]crop=${3}:${4}:${1}:${2},\
            waveform=intensity=0.8:mode=column:mirror=1:c=1:f=${5}:graticule=invert:flags=numbers+dots:scale=${6},scale=${width}:${height},setsar=1/1[b1];\
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
            "               crop=iw:1:0:${1}:0:1,waveform=intensity=${2}:mode=column:mirror=1:components=7:display=overlay:graticule=invert:flags=numbers+dots:scale=${5}",
            "split[a][b];[a]crop=iw:1:0:${1}:0:1,waveform=intensity=${2}:mode=column:mirror=1:components=7:display=overlay:graticule=invert:flags=numbers+dots:scale=${5},scale=iw:${height},drawbox=y=${1}:w=iw:h=1:color=yellow,setsar=1/1[a1];[b]lutyuv=y=val/2,setsar=1/1[b1];[a1][b1]blend=addition",
            "               crop=1:ih:${1}:0:0:1,waveform=intensity=${2}:mode=row:   mirror=1:components=7:display=overlay:graticule=invert:flags=numbers+dots:scale=${5}",
            "split[a][b];[a]crop=1:ih:${1}:0:0:1,waveform=intensity=${2}:mode=row:   mirror=1:components=7:display=overlay:graticule=invert:flags=numbers+dots:scale=${5},scale=${width}:${height},drawbox=x=${1}:w=1:h=ih:color=yellow,setsar=1/1[a1];[b]lutyuv=y=val/2,setsar=1/1[b1];[a1][b1]blend=addition",
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
            "vectorscope=i=${2}:mode=${3}:envelope=${4}:colorspace=${5}:graticule=invert:flags=name,pad=ih*${dar}:ih:(ow-iw)/2:(oh-ih)/2",
            "format=yuv422p|yuv422p10le|yuv420p|yuv411p|yuv444p|yuv444p10le,split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]vectorscope=i=${2}:mode=${3}:envelope=${4}:colorspace=${5}:graticule=invert:flags=name[a2];[b1]vectorscope=i=${2}:mode=${3}:envelope=${4}:colorspace=${5}:graticule=invert:flags=name[b2];[a2][b2]vstack,pad=ih*${dar}:ih:(ow-iw)/2:(oh-ih)/2",
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
            "split[h][l];[l]vectorscope=i=${2}:mode=${3}:envelope=${4}:colorspace=${5}:graticule=invert:flags=name:l=0:h=${6}[l1];[h]vectorscope=i=${2}:mode=${3}:envelope=${4}:colorspace=${5}:graticule=invert:flags=name:l=${6}:h=1[h1];[l1][h1]hstack",
            "format=yuv422p|yuv422p10le|yuv420p|yuv411p|yuv444p|yuv444p10le,split[a][b];\
            [a]field=top,split[th][tl];\
            [b]field=bottom,split[bh][bl];\
            [th]vectorscope=i=${2}:mode=${3}:envelope=${4}:colorspace=${5}:graticule=invert:flags=name:l=${6}:h=1[th1];\
            [tl]vectorscope=i=${2}:mode=${3}:envelope=${4}:colorspace=${5}:graticule=invert:flags=name:l=0:h=${6}[tl1];\
            [bh]vectorscope=i=${2}:mode=${3}:envelope=${4}:colorspace=${5}:graticule=invert:flags=name:l=${6}:h=1[bh1];\
            [bl]vectorscope=i=${2}:mode=${3}:envelope=${4}:colorspace=${5}:graticule=invert:flags=name:l=0:h=${6}[bl1];\
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
            format=yuv422p|yuv422p10le|yuv420p|yuv411p|yuv444p|yuv444p10le,vectorscope=i=0.1:mode=${5}:envelope=${6}:colorspace=601:graticule=invert:flags=name,pad=ih*${dar}:ih:(ow-iw)/2:(oh-ih)/2",
            "split[a][b];\
            [a]lutyuv=y=val/4,scale=${width}:${height},setsar=1/1,format=yuv444p|yuv444p10le[a1];\
            [b]crop=${3}:${4}:${1}:${2},\
            format=yuv422p|yuv422p10le|yuv420p|yuv411p|yuv444p|yuv444p10le,vectorscope=i=0.1:mode=${5}:envelope=${6}:colorspace=601:graticule=invert:flags=name,pad=ih*${dar}:ih:(ow-iw)/2:(oh-ih)/2,scale=${width}:${height},setsar=1/1[b1];\
            [a1][b1]blend=addition",
        },
    },
    {
        "CIE Scope",
        0,
        {
            { Args_Type_Slider,   1,   0,   9,   1, "System"},
            { Args_Type_Slider,   1,   0,   9,   1, "Gamut"},
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
            "il=l=d:c=d,format=yuv444p|yuv422p|yuv420p|yuv444p|yuv410p,extractplanes=${2},histeq=strength=${3}:intensity=${4}",
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
            "format=yuv444p|yuvj444p,lutyuv=y=val:u=mod(val+${5}\\,256):v=mod(val+${6}\\,256),hue=h=${3}:s=${4},split[a][b];[a]vectorscope=mode=color2:colorspace=${7}:graticule=invert:flags=name,\
            scale=512:512,pad=720:512:(ow-iw)/2:(oh-ih)/2,setsar=1/1[a1];\
            [b]lutyuv=y=val/2,scale=720:512,setsar=1/1[b1];[a1][b1]blend=addition",
            "il=l=d:c=d,format=yuv444p|yuvj444p,lutyuv=y=val:u=mod(val+${5}\\,256):v=mod(val+${6}\\,256),hue=h=${3}:s=${4}",
            "il=l=d:c=d,format=yuv444p|yuvj444p,lutyuv=y=val:u=mod(val+${5}\\,256):v=mod(val+${6}\\,256),hue=h=${3}:s=${4},split[a][b];[a]vectorscope=mode=color2:colorspace=${7}:graticule=invert:flags=name,\
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
            { Args_Type_Slider,   0,   0,   7,   1, "Filter" },
            { Args_Type_Slider,   0,   0,   2,   1, "Scale" },
            { Args_Type_Slider,   1,   0,  10,  10, "Intensity" },
        },
        {
            "format=yuv444p|yuvj444p,lutyuv=y=(val+${3})*${4}:u=val:v=val",
            "format=yuv444p|yuvj444p,lutyuv=y=(val+${3})*${4}:u=val:v=val,split[a][b];[a]waveform=intensity=${7}:graticule=invert:flags=numbers+dots:f=${5}:scale=${6},\
            scale=${width}:${height},setsar=1/1[a1];[b]setsar=1/1[b1];\
            [b1][a1]vstack",
            "il=l=d:c=d,format=yuv444p|yuvj444p,lutyuv=y=(val+${3})*${4}:u=val:v=val",
            "format=yuv444p|yuvj444p,split[a][b];\
            [a]field=top,split[t1][t2];\
            [t1]lutyuv=y=(val+${3})*${4}:u=val:v=val,waveform=intensity=${7}:graticule=invert:flags=numbers+dots:f=${5}:scale=${6}[t1w];\
            [b]field=bottom,split[b1][b2];\
            [b1]lutyuv=y=(val+${3})*${4}:u=val:v=val,waveform=intensity=${7}:graticule=invert:flags=numbers+dots:f=${5}:scale=${6}[b1w];\
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
/*
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
*/
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

extern int FiltersListDefault_Count;

#endif // FILTERS_H
