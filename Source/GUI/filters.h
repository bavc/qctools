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
        "Normal",
        0,
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field" },
            { Args_Type_Toggle,   0,   0,   0,   0, "Metadata" },
            { Args_Type_Toggle,   0,   0,   0,   0, "Title/Action Safe" },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
            // field=0, metadata=0, safe=0
            "format=yuv444p,scale",
            // field=0, metadata=0, safe=1
            "format=yuv444p,scale,\
             drawbox=color=yellow:x='iw*(((100-93)/2)/100)':y='ih*(((100-93)/2)/100)':width='iw*(93/100)':height='ih*(93/100)':thickness=1,drawbox=color=green:x='iw*(((100-90)/2)/100)':y='ih*(((100-90)/2)/100)':width='iw*(90/100)':height='ih*(90/100)':thickness=1",
            // field=0, metadata=1, safe=0
                       "format=yuv444p,scale,drawtext=fontfile=${fontfile}:box=1:boxborderw=4:boxcolor=black@0.5:fontcolor=white:fontsize=16:x=4:y=4:text=PTS=%{pts\\\\:hms}\
                                            ,drawtext=fontfile=${fontfile}:box=1:boxborderw=2:boxcolor=black@0.5:fontcolor=white:fontsize=16:x=4:y=20:text=size=%{eif\\\\:w\\\\:d}x%{eif\\\\:h\\\\:d} dar=${dar}\
           ,cropdetect=reset_count=1:round=1,drawtext=fontfile=${fontfile}:box=1:boxborderw=2:boxcolor=black@0.5:fontcolor=white:fontsize=16:x=4:y=36:text=cropdetect wxh=%{metadata\\\\:lavfi.cropdetect.w}x%{metadata\\\\:lavfi.cropdetect.h} x\\,y=%{metadata\\\\:lavfi.cropdetect.x}\\,%{metadata\\\\:lavfi.cropdetect.y}\
                           ,idet=half_life=1,drawtext=fontfile=${fontfile}:box=1:boxborderw=2:boxcolor=black@0.5:fontcolor=white:fontsize=16:x=4:y=52:text=interlacement \(single\)\\\\: %{metadata\\\\:lavfi.idet.single.current_frame}\
                                            ,drawtext=fontfile=${fontfile}:box=1:boxborderw=2:boxcolor=black@0.5:fontcolor=white:fontsize=16:x=4:y=68:text=interlacement \(multiple\)\\\\: %{metadata\\\\:lavfi.idet.multiple.current_frame}\
                                            ,drawtext=fontfile=${fontfile}:box=1:boxborderw=2:boxcolor=black@0.5:fontcolor=white:fontsize=16:x=4:y=84:text=interlacement \(repeat field\)\\\\: %{metadata\\\\:lavfi.idet.repeated.current_frame}",
            // field=0, metadata=1, safe=1
                       "format=yuv444p,scale,drawtext=fontfile=${fontfile}:box=1:boxborderw=4:boxcolor=black@0.5:fontcolor=white:fontsize=16:x=4:y=4:text=PTS=%{pts\\\\:hms}\
                                            ,drawtext=fontfile=${fontfile}:box=1:boxborderw=2:boxcolor=black@0.5:fontcolor=white:fontsize=16:x=4:y=20:text=size=%{eif\\\\:w\\\\:d}x%{eif\\\\:h\\\\:d} dar=${dar}\
           ,cropdetect=reset_count=1:round=1,drawtext=fontfile=${fontfile}:box=1:boxborderw=2:boxcolor=black@0.5:fontcolor=white:fontsize=16:x=4:y=36:text=cropdetect wxh=%{metadata\\\\:lavfi.cropdetect.w}x%{metadata\\\\:lavfi.cropdetect.h} x\\,y=%{metadata\\\\:lavfi.cropdetect.x}\\,%{metadata\\\\:lavfi.cropdetect.y}\
                           ,idet=half_life=1,drawtext=fontfile=${fontfile}:box=1:boxborderw=2:boxcolor=black@0.5:fontcolor=white:fontsize=16:x=4:y=52:text=interlacement \(single\)\\\\: %{metadata\\\\:lavfi.idet.single.current_frame}\
                                            ,drawtext=fontfile=${fontfile}:box=1:boxborderw=2:boxcolor=black@0.5:fontcolor=white:fontsize=16:x=4:y=68:text=interlacement \(multiple\)\\\\: %{metadata\\\\:lavfi.idet.multiple.current_frame}\
                                            ,drawtext=fontfile=${fontfile}:box=1:boxborderw=2:boxcolor=black@0.5:fontcolor=white:fontsize=16:x=4:y=84:text=interlacement \(repeat field\)\\\\: %{metadata\\\\:lavfi.idet.repeated.current_frame}\
                                            ,drawbox=color=yellow:x='iw*(((100-93)/2)/100)':y='ih*(((100-93)/2)/100)':width='iw*(93/100)':height='ih*(93/100)':thickness=1,drawbox=color=green:x='iw*(((100-90)/2)/100)':y='ih*(((100-90)/2)/100)':width='iw*(90/100)':height='ih*(90/100)':thickness=1",
            // field=1, metadata=0, safe=0
            "format=yuv444p,scale,il=l=d:c=d",
            // field=1, metadata=0, safe=1
            "format=yuv444p,scale,split[t][b]\
                ;[t]field=top,drawbox=color=yellow:x='iw*(((100-93)/2)/100)':y='ih*(((100-93)/2)/100)':width='iw*(93/100)':height='ih*(93/100)':thickness=1,drawbox=color=green:x='iw*(((100-90)/2)/100)':y='ih*(((100-90)/2)/100)':width='iw*(90/100)':height='ih*(90/100)':thickness=1[t2]\
                ;[b]field=bottom,drawbox=color=yellow:x='iw*(((100-93)/2)/100)':y='ih*(((100-93)/2)/100)':width='iw*(93/100)':height='ih*(93/100)':thickness=1,drawbox=color=green:x='iw*(((100-90)/2)/100)':y='ih*(((100-90)/2)/100)':width='iw*(90/100)':height='ih*(90/100)':thickness=1[b2]\
           ;[t2][b2]vstack",
            // field=1, metadata=1, safe=0
            "format=yuv444p,scale,split[t][b],[t]field=top[t1];[b]field=bottom[b1]\
                                        ;[t1]drawtext=fontfile=${fontfile}:box=1:boxborderw=4:boxcolor=black@0.5:fontcolor=white:fontsize=16:x=4:y=4:text=PTS=%{pts\\\\:hms}\
                                            ,drawtext=fontfile=${fontfile}:box=1:boxborderw=2:boxcolor=black@0.5:fontcolor=white:fontsize=16:x=4:y=20:text=size=%{eif\\\\:w\\\\:d}x%{eif\\\\:h\\\\:d}\
           ,cropdetect=reset_count=1:round=1,drawtext=fontfile=${fontfile}:box=1:boxborderw=2:boxcolor=black@0.5:fontcolor=white:fontsize=16:x=4:y=36:text=cropdetect wxh=%{metadata\\\\:lavfi.cropdetect.w}x%{metadata\\\\:lavfi.cropdetect.h} x\\,y=%{metadata\\\\:lavfi.cropdetect.x}\\,%{metadata\\\\:lavfi.cropdetect.y}[t2]\
                                        ;[b1]drawtext=fontfile=${fontfile}:box=1:boxborderw=4:boxcolor=black@0.5:fontcolor=white:fontsize=16:x=4:y=4:text=PTS=%{pts\\\\:hms}\
                                            ,drawtext=fontfile=${fontfile}:box=1:boxborderw=2:boxcolor=black@0.5:fontcolor=white:fontsize=16:x=4:y=20:text=size=%{eif\\\\:w\\\\:d}x%{eif\\\\:h\\\\:d}\
           ,cropdetect=reset_count=1:round=1,drawtext=fontfile=${fontfile}:box=1:boxborderw=2:boxcolor=black@0.5:fontcolor=white:fontsize=16:x=4:y=36:text=cropdetect wxh=%{metadata\\\\:lavfi.cropdetect.w}x%{metadata\\\\:lavfi.cropdetect.h} x\\,y=%{metadata\\\\:lavfi.cropdetect.x}\\,%{metadata\\\\:lavfi.cropdetect.y}[b2]\
           ;[t2][b2]vstack",
            // field=1, metadata=1, safe=1
            "format=yuv444p,scale,split[t][b],[t]field=top[t1];[b]field=bottom[b1]\
                                        ;[t1]drawtext=fontfile=${fontfile}:box=1:boxborderw=4:boxcolor=black@0.5:fontcolor=white:fontsize=16:x=4:y=4:text=PTS=%{pts\\\\:hms}\
                                            ,drawtext=fontfile=${fontfile}:box=1:boxborderw=2:boxcolor=black@0.5:fontcolor=white:fontsize=16:x=4:y=20:text=size=%{eif\\\\:w\\\\:d}x%{eif\\\\:h\\\\:d}\
           ,cropdetect=reset_count=1:round=1,drawtext=fontfile=${fontfile}:box=1:boxborderw=2:boxcolor=black@0.5:fontcolor=white:fontsize=16:x=4:y=36:text=cropdetect wxh=%{metadata\\\\:lavfi.cropdetect.w}x%{metadata\\\\:lavfi.cropdetect.h} x\\,y=%{metadata\\\\:lavfi.cropdetect.x}\\,%{metadata\\\\:lavfi.cropdetect.y},\
            drawbox=color=yellow:x='iw*(((100-93)/2)/100)':y='ih*(((100-93)/2)/100)':width='iw*(93/100)':height='ih*(93/100)':thickness=1,drawbox=color=green:x='iw*(((100-90)/2)/100)':y='ih*(((100-90)/2)/100)':width='iw*(90/100)':height='ih*(90/100)':thickness=1[t2]\
                                        ;[b1]drawtext=fontfile=${fontfile}:box=1:boxborderw=4:boxcolor=black@0.5:fontcolor=white:fontsize=16:x=4:y=4:text=PTS=%{pts\\\\:hms}\
                                            ,drawtext=fontfile=${fontfile}:box=1:boxborderw=2:boxcolor=black@0.5:fontcolor=white:fontsize=16:x=4:y=20:text=size=%{eif\\\\:w\\\\:d}x%{eif\\\\:h\\\\:d}\
           ,cropdetect=reset_count=1:round=1,drawtext=fontfile=${fontfile}:box=1:boxborderw=2:boxcolor=black@0.5:fontcolor=white:fontsize=16:x=4:y=36:text=cropdetect wxh=%{metadata\\\\:lavfi.cropdetect.w}x%{metadata\\\\:lavfi.cropdetect.h} x\\,y=%{metadata\\\\:lavfi.cropdetect.x}\\,%{metadata\\\\:lavfi.cropdetect.y},\
            drawbox=color=yellow:x='iw*(((100-93)/2)/100)':y='ih*(((100-93)/2)/100)':width='iw*(93/100)':height='ih*(93/100)':thickness=1,drawbox=color=green:x='iw*(((100-90)/2)/100)':y='ih*(((100-90)/2)/100)':width='iw*(90/100)':height='ih*(90/100)':thickness=1[b2]\
           ;[t2][b2]vstack",
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
            { Args_Type_YuvA,     3,   0,   0,   0, "Plane" },
            { Args_Type_LogLin,   0,   0,   0,   0, "Levels" },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
            // field N, all planes N
            "format=${pix_fmt},histogram=colors_mode=coloronwhite:level_height=${height}-12:components=${2}:levels_mode=${3}",
            // field N, all planes Y
            "format=${pix_fmt},histogram=colors_mode=coloronwhite:level_height=${height}:levels_mode=${3}",
            // field Y, all planes N
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]format=${pix_fmt},histogram=colors_mode=coloronwhite:components=${2}:levels_mode=${3}[a2];[b1]format=${pix_fmt},histogram=colors_mode=coloronwhite:components=${2}:levels_mode=${3}[b2];[a2][b2]vstack",
            // field Y, all planes Y
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]format=${pix_fmt},histogram=colors_mode=coloronwhite:levels_mode=${3}[a2];[b1]format=${pix_fmt},histogram=colors_mode=coloronwhite:levels_mode=${3}[b2];[a2][b2]hstack",
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
            "format=${pix_fmt:4},thistogram=components=${3}:levels_mode=${4}",
            // field N, overlay N, all planes Y
            "format=${pix_fmt:4},thistogram=levels_mode=${4}",
            // field N, overlay Y, all planes N
            "format=${pix_fmt:4},thistogram=display_mode=overlay:components=${3}:levels_mode=${4}",
            // field N, overlay Y, all planes Y
            "format=${pix_fmt:4},thistogram=display_mode=overlay:levels_mode=${4}",
            // field Y, overlay N, all planes N
            "format=${pix_fmt:4},split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]thistogram=components=${3}:levels_mode=${4}[a2];[b1]thistogram=components=${3}:levels_mode=${4}[b2];[a2][b2]vstack",
            // field Y, overlay N, all planes Y
            "format=${pix_fmt:4},split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]thistogram=levels_mode=${4}[a2];[b1]thistogram=levels_mode=${4}[b2];[a2][b2]hstack",
            // field Y, overlay Y, all planes N
            "format=${pix_fmt:4},split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]thistogram=display_mode=overlay:components=${3}:levels_mode=${4}[a2];[b1]thistogram=display_mode=overlay:components=${3}:levels_mode=${4}[b2];[a2][b2]vstack",
            // field Y, overlay Y, all planes Y
            "format=${pix_fmt:4},split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]thistogram=display_mode=overlay:levels_mode=${4}[a2];[b1]thistogram=display_mode=overlay:levels_mode=${4}[b2];[a2][b2]vstack",
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
            "format=${pix_fmt:4},waveform=intensity=${2}:mode=column:mirror=1:c=${3}:f=${5}:graticule=green:flags=numbers+dots:scale=${6}",
            // field N, all planes N, vertical Y
            "format=${pix_fmt:4},waveform=intensity=${2}:mode=row:   mirror=1:c=${3}:f=${5}:graticule=green:flags=numbers+dots:scale=${6}",
            // field N, all planes Y, vertical N
            "format=${pix_fmt:4},waveform=intensity=${2}:mode=column:mirror=1:c=${3}:f=${5}:graticule=green:flags=numbers+dots:scale=${6}:display=overlay",
            // field N, all planes Y, vertical Y
            "format=${pix_fmt:4},waveform=intensity=${2}:mode=row:   mirror=1:c=${3}:f=${5}:graticule=green:flags=numbers+dots:scale=${6}:display=overlay",
            // field Y, all planes N, vertical N
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]format=${pix_fmt:4},waveform=intensity=${2}:mode=column:mirror=1:c=${3}:f=${5}:graticule=green:flags=numbers+dots:scale=${6}[a2];[b1]format=${pix_fmt:4},waveform=intensity=${2}:mode=column:mirror=1:c=${3}:f=${5}:graticule=green:flags=numbers+dots:scale=${6}[b2];[a2][b2]vstack",
            // field Y, all planes N, vertical Y
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]format=${pix_fmt:4},waveform=intensity=${2}:mode=row:   mirror=1:c=${3}:f=${5}:graticule=green:flags=numbers+dots:scale=${6}[a2];[b1]format=${pix_fmt:4},waveform=intensity=${2}:mode=row:   mirror=1:c=${3}:f=${5}:graticule=green:flags=numbers+dots:scale=${6}[b2];[a2][b2]vstack",
            // field Y, all planes Y, vertical N
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]format=${pix_fmt:4},waveform=intensity=${2}:mode=column:mirror=1:c=${3}:f=${5}:graticule=green:flags=numbers+dots:scale=${6}:display=overlay[a2];[b1]format=${pix_fmt:4},waveform=intensity=${2}:mode=column:mirror=1:c=${3}:f=${5}:graticule=green:flags=numbers+dots:scale=${6}:display=overlay[b2];[a2][b2]vstack",
            // field Y, all planes Y, vertical Y
            "split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]format=${pix_fmt:4},waveform=intensity=${2}:mode=row:   mirror=1:c=${3}:f=${5}:graticule=green:flags=numbers+dots:scale=${6}:display=overlay[a2];[b1]format=${pix_fmt:4},waveform=intensity=${2}:mode=row:   mirror=1:c=${3}:f=${5}:graticule=green:flags=numbers+dots:scale=${6}:display=overlay[b2];[b2][a2]vstack",
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
            "               crop=iw:1:0:${1}:0:1,format=${pix_fmt:4},waveform=intensity=${2}:mode=column:mirror=1:components=7:display=overlay:graticule=green:flags=numbers+dots:scale=${5}",
            "split[a][b];[a]crop=iw:1:0:${1}:0:1,format=${pix_fmt:4},waveform=intensity=${2}:mode=column:mirror=1:components=7:display=overlay:graticule=green:flags=numbers+dots:scale=${5},scale=iw:${height},drawbox=y=${1}:w=iw:h=1:color=yellow,setsar=1/1[a1];[b]lutyuv=y=val/2,setsar=1/1[b1];[a1][b1]blend=addition",
            "               crop=1:ih:${1}:0:0:1,format=${pix_fmt:4},waveform=intensity=${2}:mode=row:   mirror=1:components=7:display=overlay:graticule=green:flags=numbers+dots:scale=${5}",
            "split[a][b];[a]crop=1:ih:${1}:0:0:1,format=${pix_fmt:4},waveform=intensity=${2}:mode=row:   mirror=1:components=7:display=overlay:graticule=green:flags=numbers+dots:scale=${5},scale=${width}:${height},drawbox=x=${1}:w=1:h=ih:color=yellow,setsar=1/1[a1];[b]lutyuv=y=val/2,setsar=1/1[b1];[a1][b1]blend=addition",
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
            "format=${pix_fmt:6},vectorscope=i=${2}:mode=${3}:envelope=${4}:colorspace=${5}:graticule=green:flags=name,pad=ih*${dar}:ih:(ow-iw)/2:(oh-ih)/2",
            "format=${pix_fmt:6},split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1]vectorscope=i=${2}:mode=${3}:envelope=${4}:colorspace=${5}:graticule=green:flags=name[a2];[b1]vectorscope=i=${2}:mode=${3}:envelope=${4}:colorspace=${5}:graticule=green:flags=name[b2];[a2][b2]vstack,pad=ih*${dar}:ih:(ow-iw)/2:(oh-ih)/2",
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
            "format=${pix_fmt:6},split[h][l];[l]vectorscope=i=${2}:mode=${3}:envelope=${4}:colorspace=${5}:graticule=green:flags=name:l=0:h=${6}[l1];[h]vectorscope=i=${2}:mode=${3}:envelope=${4}:colorspace=${5}:graticule=green:flags=name:l=${6}:h=1[h1];[l1][h1]hstack",
            "format=${pix_fmt:6},split[a][b];\
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
                       "extractplanes=${2},histeq=strength=${3}:intensity=${4}",
            "il=l=d:c=d,extractplanes=${2},histeq=strength=${3}:intensity=${4}",
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
            "format=${pix_fmt},\
            lut=\
                c0=if(eq(${2}\\,-1)\\,(maxval+minval)/2\\,if(eq(${2}\\,0)\\,val\\,if(eq(bitand(val\\,pow(2\\,${bitdepth}-${2}))\\,0)\\,minval\\,maxval))):\
                c1=if(eq(${3}\\,-1)\\,(maxval+minval)/2\\,if(eq(${3}\\,0)\\,val\\,if(eq(bitand(val\\,pow(2\\,${bitdepth}-${3}))\\,0)\\,minval\\,maxval))):\
                c2=if(eq(${4}\\,-1)\\,(maxval+minval)/2\\,if(eq(${4}\\,0)\\,val\\,if(eq(bitand(val\\,pow(2\\,${bitdepth}-${4}))\\,0)\\,minval\\,maxval)))",
            "il=l=d:c=d,format=${pix_fmt},\
            lut=\
                c0=if(eq(${2}\\,-1)\\,(maxval+minval)/2\\,if(eq(${2}\\,0)\\,val\\,if(eq(bitand(val\\,pow(2\\,${bitdepth}-${2}))\\,0)\\,minval\\,maxval))):\
                c1=if(eq(${3}\\,-1)\\,(maxval+minval)/2\\,if(eq(${3}\\,0)\\,val\\,if(eq(bitand(val\\,pow(2\\,${bitdepth}-${3}))\\,0)\\,minval\\,maxval))):\
                c2=if(eq(${4}\\,-1)\\,(maxval+minval)/2\\,if(eq(${4}\\,0)\\,val\\,if(eq(bitand(val\\,pow(2\\,${bitdepth}-${4}))\\,0)\\,minval\\,maxval)))",
        },
    },
    {
        "Bit Plane (10 slices)",
        0,
        {
            { Args_Type_Yuv,      0,   0,   0,   0, "Plane"},
            { Args_Type_Toggle,   0,   0,   0,   0, "Show 2" },
            { Args_Type_Toggle,   1,   0,   0,   0, "Slice" },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
          // Slice N Show 2 N
          "format=${pix_fmt},\
          split=10[b0][b1][b2][b3][b4][b5][b6][b7][b8][b9];\
          [b0]lut=c0=pow(2\\,${bitdepth}-1):c1=pow(2\\,${bitdepth}-1):c2=pow(2\\,${bitdepth}-1):${1}=bitand(val\\,pow(2\\,${bitdepth}-1))*pow(2\\,1)[b0c];\
          [b1]lut=c0=pow(2\\,${bitdepth}-1):c1=pow(2\\,${bitdepth}-1):c2=pow(2\\,${bitdepth}-1):${1}=bitand(val\\,pow(2\\,${bitdepth}-2))*pow(2\\,2)[b1c];\
          [b2]lut=c0=pow(2\\,${bitdepth}-1):c1=pow(2\\,${bitdepth}-1):c2=pow(2\\,${bitdepth}-1):${1}=bitand(val\\,pow(2\\,${bitdepth}-3))*pow(2\\,3)[b2c];\
          [b3]lut=c0=pow(2\\,${bitdepth}-1):c1=pow(2\\,${bitdepth}-1):c2=pow(2\\,${bitdepth}-1):${1}=bitand(val\\,pow(2\\,${bitdepth}-4))*pow(2\\,4)[b3c];\
          [b4]lut=c0=pow(2\\,${bitdepth}-1):c1=pow(2\\,${bitdepth}-1):c2=pow(2\\,${bitdepth}-1):${1}=bitand(val\\,pow(2\\,${bitdepth}-5))*pow(2\\,5)[b4c];\
          [b5]lut=c0=pow(2\\,${bitdepth}-1):c1=pow(2\\,${bitdepth}-1):c2=pow(2\\,${bitdepth}-1):${1}=bitand(val\\,pow(2\\,${bitdepth}-6))*pow(2\\,6)[b5c];\
          [b6]lut=c0=pow(2\\,${bitdepth}-1):c1=pow(2\\,${bitdepth}-1):c2=pow(2\\,${bitdepth}-1):${1}=bitand(val\\,pow(2\\,${bitdepth}-7))*pow(2\\,7)[b6c];\
          [b7]lut=c0=pow(2\\,${bitdepth}-1):c1=pow(2\\,${bitdepth}-1):c2=pow(2\\,${bitdepth}-1):${1}=bitand(val\\,pow(2\\,${bitdepth}-8))*pow(2\\,8)[b7c];\
          [b8]lut=c0=pow(2\\,${bitdepth}-1):c1=pow(2\\,${bitdepth}-1):c2=pow(2\\,${bitdepth}-1):${1}=bitand(val\\,pow(2\\,${bitdepth}-9))*pow(2\\,9)[b8c];\
          [b9]lut=c0=pow(2\\,${bitdepth}-1):c1=pow(2\\,${bitdepth}-1):c2=pow(2\\,${bitdepth}-1):${1}=bitand(val\\,pow(2\\,${bitdepth}-10))*pow(2\\,10)[b9c];\
          [b0c][b1c][b2c][b3c][b4c][b5c][b6c][b7c][b8c][b9c]hstack=10",
          // Slice Y Show 2 N
          "format=${pix_fmt},\
          split=10[b0][b1][b2][b3][b4][b5][b6][b7][b8][b9];\
          [b0]crop=iw/10:ih:(iw/10)*0:0,lut=c0=pow(2\\,${bitdepth}-1):c1=pow(2\\,${bitdepth}-1):c2=pow(2\\,${bitdepth}-1):${1}=bitand(val\\,pow(2\\,${bitdepth}-1))*pow(2\\,1)[b0c];\
          [b1]crop=iw/10:ih:(iw/10)*1:0,lut=c0=pow(2\\,${bitdepth}-1):c1=pow(2\\,${bitdepth}-1):c2=pow(2\\,${bitdepth}-1):${1}=bitand(val\\,pow(2\\,${bitdepth}-2))*pow(2\\,2)[b1c];\
          [b2]crop=iw/10:ih:(iw/10)*2:0,lut=c0=pow(2\\,${bitdepth}-1):c1=pow(2\\,${bitdepth}-1):c2=pow(2\\,${bitdepth}-1):${1}=bitand(val\\,pow(2\\,${bitdepth}-3))*pow(2\\,3)[b2c];\
          [b3]crop=iw/10:ih:(iw/10)*3:0,lut=c0=pow(2\\,${bitdepth}-1):c1=pow(2\\,${bitdepth}-1):c2=pow(2\\,${bitdepth}-1):${1}=bitand(val\\,pow(2\\,${bitdepth}-4))*pow(2\\,4)[b3c];\
          [b4]crop=iw/10:ih:(iw/10)*4:0,lut=c0=pow(2\\,${bitdepth}-1):c1=pow(2\\,${bitdepth}-1):c2=pow(2\\,${bitdepth}-1):${1}=bitand(val\\,pow(2\\,${bitdepth}-5))*pow(2\\,5)[b4c];\
          [b5]crop=iw/10:ih:(iw/10)*5:0,lut=c0=pow(2\\,${bitdepth}-1):c1=pow(2\\,${bitdepth}-1):c2=pow(2\\,${bitdepth}-1):${1}=bitand(val\\,pow(2\\,${bitdepth}-6))*pow(2\\,6)[b5c];\
          [b6]crop=iw/10:ih:(iw/10)*6:0,lut=c0=pow(2\\,${bitdepth}-1):c1=pow(2\\,${bitdepth}-1):c2=pow(2\\,${bitdepth}-1):${1}=bitand(val\\,pow(2\\,${bitdepth}-7))*pow(2\\,7)[b6c];\
          [b7]crop=iw/10:ih:(iw/10)*7:0,lut=c0=pow(2\\,${bitdepth}-1):c1=pow(2\\,${bitdepth}-1):c2=pow(2\\,${bitdepth}-1):${1}=bitand(val\\,pow(2\\,${bitdepth}-8))*pow(2\\,8)[b7c];\
          [b8]crop=iw/10:ih:(iw/10)*8:0,lut=c0=pow(2\\,${bitdepth}-1):c1=pow(2\\,${bitdepth}-1):c2=pow(2\\,${bitdepth}-1):${1}=bitand(val\\,pow(2\\,${bitdepth}-9))*pow(2\\,9)[b8c];\
          [b9]crop=iw/10:ih:(iw/10)*9:0,lut=c0=pow(2\\,${bitdepth}-1):c1=pow(2\\,${bitdepth}-1):c2=pow(2\\,${bitdepth}-1):${1}=bitand(val\\,pow(2\\,${bitdepth}-10))*pow(2\\,10)[b9c];\
          [b0c][b1c][b2c][b3c][b4c][b5c][b6c][b7c][b8c][b9c]hstack=10,format=yuv444p,drawgrid=w=iw/10:h=ih:t=2:c=green@0.5",
          // Slice N Show 2 Y
          "format=${pix_fmt},\
          split=10[b0][b1][b2][b3][b4][b5][b6][b7][b8][b9];\
          [b0]lut=c0=pow(2\\,${bitdepth}-1):c1=pow(2\\,${bitdepth}-1):c2=pow(2\\,${bitdepth}-1):${1}=bitand(val\\,pow(2\\,${bitdepth}-1)+pow(2\\,${bitdepth}-2))*pow(2\\,1)[b0c];\
          [b1]lut=c0=pow(2\\,${bitdepth}-1):c1=pow(2\\,${bitdepth}-1):c2=pow(2\\,${bitdepth}-1):${1}=bitand(val\\,pow(2\\,${bitdepth}-2)+pow(2\\,${bitdepth}-3))*pow(2\\,2)[b1c];\
          [b2]lut=c0=pow(2\\,${bitdepth}-1):c1=pow(2\\,${bitdepth}-1):c2=pow(2\\,${bitdepth}-1):${1}=bitand(val\\,pow(2\\,${bitdepth}-3)+pow(2\\,${bitdepth}-4))*pow(2\\,3)[b2c];\
          [b3]lut=c0=pow(2\\,${bitdepth}-1):c1=pow(2\\,${bitdepth}-1):c2=pow(2\\,${bitdepth}-1):${1}=bitand(val\\,pow(2\\,${bitdepth}-4)+pow(2\\,${bitdepth}-5))*pow(2\\,4)[b3c];\
          [b4]lut=c0=pow(2\\,${bitdepth}-1):c1=pow(2\\,${bitdepth}-1):c2=pow(2\\,${bitdepth}-1):${1}=bitand(val\\,pow(2\\,${bitdepth}-5)+pow(2\\,${bitdepth}-6))*pow(2\\,5)[b4c];\
          [b5]lut=c0=pow(2\\,${bitdepth}-1):c1=pow(2\\,${bitdepth}-1):c2=pow(2\\,${bitdepth}-1):${1}=bitand(val\\,pow(2\\,${bitdepth}-6)+pow(2\\,${bitdepth}-7))*pow(2\\,6)[b5c];\
          [b6]lut=c0=pow(2\\,${bitdepth}-1):c1=pow(2\\,${bitdepth}-1):c2=pow(2\\,${bitdepth}-1):${1}=bitand(val\\,pow(2\\,${bitdepth}-7)+pow(2\\,${bitdepth}-8))*pow(2\\,7)[b6c];\
          [b7]lut=c0=pow(2\\,${bitdepth}-1):c1=pow(2\\,${bitdepth}-1):c2=pow(2\\,${bitdepth}-1):${1}=bitand(val\\,pow(2\\,${bitdepth}-8)+pow(2\\,${bitdepth}-9))*pow(2\\,8)[b7c];\
          [b8]lut=c0=pow(2\\,${bitdepth}-1):c1=pow(2\\,${bitdepth}-1):c2=pow(2\\,${bitdepth}-1):${1}=bitand(val\\,pow(2\\,${bitdepth}-9)+pow(2\\,${bitdepth}-10))*pow(2\\,9)[b8c];\
          [b9]lut=c0=pow(2\\,${bitdepth}-1):c1=pow(2\\,${bitdepth}-1):c2=pow(2\\,${bitdepth}-1):${1}=bitand(val\\,pow(2\\,${bitdepth}-10))*pow(2\\,10)[b9c];\
          [b0c][b1c][b2c][b3c][b4c][b5c][b6c][b7c][b8c][b9c]hstack=10",
          // Slice Y Show 2 Y
          "format=${pix_fmt},\
          split=10[b0][b1][b2][b3][b4][b5][b6][b7][b8][b9];\
          [b0]crop=iw/10:ih:(iw/10)*0:0,lut=c0=pow(2\\,${bitdepth}-1):c1=pow(2\\,${bitdepth}-1):c2=pow(2\\,${bitdepth}-1):${1}=bitand(val\\,pow(2\\,${bitdepth}-1)+pow(2\\,${bitdepth}-2))*pow(2\\,1)[b0c];\
          [b1]crop=iw/10:ih:(iw/10)*1:0,lut=c0=pow(2\\,${bitdepth}-1):c1=pow(2\\,${bitdepth}-1):c2=pow(2\\,${bitdepth}-1):${1}=bitand(val\\,pow(2\\,${bitdepth}-2)+pow(2\\,${bitdepth}-3))*pow(2\\,2)[b1c];\
          [b2]crop=iw/10:ih:(iw/10)*2:0,lut=c0=pow(2\\,${bitdepth}-1):c1=pow(2\\,${bitdepth}-1):c2=pow(2\\,${bitdepth}-1):${1}=bitand(val\\,pow(2\\,${bitdepth}-3)+pow(2\\,${bitdepth}-4))*pow(2\\,3)[b2c];\
          [b3]crop=iw/10:ih:(iw/10)*3:0,lut=c0=pow(2\\,${bitdepth}-1):c1=pow(2\\,${bitdepth}-1):c2=pow(2\\,${bitdepth}-1):${1}=bitand(val\\,pow(2\\,${bitdepth}-4)+pow(2\\,${bitdepth}-5))*pow(2\\,4)[b3c];\
          [b4]crop=iw/10:ih:(iw/10)*4:0,lut=c0=pow(2\\,${bitdepth}-1):c1=pow(2\\,${bitdepth}-1):c2=pow(2\\,${bitdepth}-1):${1}=bitand(val\\,pow(2\\,${bitdepth}-5)+pow(2\\,${bitdepth}-6))*pow(2\\,5)[b4c];\
          [b5]crop=iw/10:ih:(iw/10)*5:0,lut=c0=pow(2\\,${bitdepth}-1):c1=pow(2\\,${bitdepth}-1):c2=pow(2\\,${bitdepth}-1):${1}=bitand(val\\,pow(2\\,${bitdepth}-6)+pow(2\\,${bitdepth}-7))*pow(2\\,6)[b5c];\
          [b6]crop=iw/10:ih:(iw/10)*6:0,lut=c0=pow(2\\,${bitdepth}-1):c1=pow(2\\,${bitdepth}-1):c2=pow(2\\,${bitdepth}-1):${1}=bitand(val\\,pow(2\\,${bitdepth}-7)+pow(2\\,${bitdepth}-8))*pow(2\\,7)[b6c];\
          [b7]crop=iw/10:ih:(iw/10)*7:0,lut=c0=pow(2\\,${bitdepth}-1):c1=pow(2\\,${bitdepth}-1):c2=pow(2\\,${bitdepth}-1):${1}=bitand(val\\,pow(2\\,${bitdepth}-8)+pow(2\\,${bitdepth}-9))*pow(2\\,8)[b7c];\
          [b8]crop=iw/10:ih:(iw/10)*8:0,lut=c0=pow(2\\,${bitdepth}-1):c1=pow(2\\,${bitdepth}-1):c2=pow(2\\,${bitdepth}-1):${1}=bitand(val\\,pow(2\\,${bitdepth}-9)+pow(2\\,${bitdepth}-10))*pow(2\\,9)[b8c];\
          [b9]crop=iw/10:ih:(iw/10)*9:0,lut=c0=pow(2\\,${bitdepth}-1):c1=pow(2\\,${bitdepth}-1):c2=pow(2\\,${bitdepth}-1):${1}=bitand(val\\,pow(2\\,${bitdepth}-10))*pow(2\\,10)[b9c];\
          [b0c][b1c][b2c][b3c][b4c][b5c][b6c][b7c][b8c][b9c]hstack=10,drawgrid=w=iw/10:h=ih:t=2:c=green@0.5",
        },
    },
    {
        "Bit Plane Noise",
        0,
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field" },
            { Args_Type_Slider,   1,   1,  16,   1, "Bit position" },
            { Args_Type_YuvA,     0,   0,   0,   0, "Plane"},
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
            "bitplanenoise=bitplane=${bitdepth}+1-${2}:filter=1,extractplanes=${3}",
            "bitplanenoise=bitplane=${bitdepth}+1-${2}:filter=1",
            "il=l=d:c=d,bitplanenoise=bitplane=${bitdepth}-${2}:filter=1,extractplanes=${3}",
            "il=l=d:c=d,bitplanenoise=bitplane=${bitdepth}-${2}:filter=1",

        },
    },
    {
        "Bit Plane Noise Graph",
        0,
        {
            { Args_Type_Yuv,      0,   0,   0,   0, "Plane"},
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
          "split=3[b1][b2][b3];\
          [b1]bitplanenoise=1,bitplanenoise=2,bitplanenoise=3,bitplanenoise=4,\
          drawgraph=mode=line:slide=scroll:size=612x${height}:min=0:max=1:m1=lavfi.bitplanenoise.${1}.1:m2=lavfi.bitplanenoise.${1}.2:m3=lavfi.bitplanenoise.${1}.3:m4=lavfi.bitplanenoise.${1}.4:bg=Wheat@1:\
          fg1=0xff0000ff:fg2=0xffff0000:fg3=0xffff00ff:fg4=0xff808000,\
          pad=iw+108:ih:108:0:color=white,format=rgba,\
          drawtext=fontfile=${fontfile}:fontcolor=black:fontsize=12:text='RANDOM':x=4:y=(h/12)*0+6,\
          drawtext=fontfile=${fontfile}:fontcolor=0xff0000ff:fontsize=12:text=bit1  %{metadata\\\\:lavfi.bitplanenoise.${1}.1}:x=4:y=(h/12)*1+6,\
          drawtext=fontfile=${fontfile}:fontcolor=0x0000ffff:fontsize=12:text=bit2  %{metadata\\\\:lavfi.bitplanenoise.${1}.2}:x=4:y=(h/12)*2+6,\
          drawtext=fontfile=${fontfile}:fontcolor=0xff00ffff:fontsize=12:text=bit3  %{metadata\\\\:lavfi.bitplanenoise.${1}.3}:x=4:y=(h/12)*3+6,\
          drawtext=fontfile=${fontfile}:fontcolor=0x008080ff:fontsize=12:text=bit4  %{metadata\\\\:lavfi.bitplanenoise.${1}.4}:x=4:y=(h/12)*4+6[b1o];\
          \
          [b2]bitplanenoise=5,bitplanenoise=6,bitplanenoise=7,bitplanenoise=8,\
          drawgraph=mode=line:slide=scroll:size=612x${height}:min=0:max=1:m1=lavfi.bitplanenoise.${1}.5:m2=lavfi.bitplanenoise.${1}.6:m3=lavfi.bitplanenoise.${1}.7:m4=lavfi.bitplanenoise.${1}.8:bg=white@0:\
          fg1=0xff008000:fg2=0xff00ff00:fg3=0xff000080:fg4=0xff800000,\
          pad=iw+108:ih:108:0:color=white@0,format=rgba,\
          drawtext=fontfile=${fontfile}:fontcolor=0x008000ff:fontsize=12:text=bit5  %{metadata\\\\:lavfi.bitplanenoise.${1}.5}:x=4:y=(h/12)*5+6,\
          drawtext=fontfile=${fontfile}:fontcolor=0x00ff00ff:fontsize=12:text=bit6  %{metadata\\\\:lavfi.bitplanenoise.${1}.6}:x=4:y=(h/12)*6+6,\
          drawtext=fontfile=${fontfile}:fontcolor=0x800000ff:fontsize=12:text=bit7  %{metadata\\\\:lavfi.bitplanenoise.${1}.7}:x=4:y=(h/12)*7+6,\
          drawtext=fontfile=${fontfile}:fontcolor=0x000080ff:fontsize=12:text=bit8  %{metadata\\\\:lavfi.bitplanenoise.${1}.8}:x=4:y=(h/12)*8+6[b2o];\
          \
          [b3]bitplanenoise=9,bitplanenoise=10,\
          drawgraph=mode=line:slide=scroll:size=612x${height}:min=0:max=1:m1=lavfi.bitplanenoise.${1}.9:m2=lavfi.bitplanenoise.${1}.10:bg=white@0:\
          fg1=0xff800080:fg2=0xff0000ff,\
          pad=iw+108:ih:108:0:color=white@0,format=rgba,\
          drawtext=fontfile=${fontfile}:fontcolor=0x800080ff:fontsize=12:text=bit9  %{metadata\\\\:lavfi.bitplanenoise.${1}.9}:x=4:y=(h/12)*9+6,\
          drawtext=fontfile=${fontfile}:fontcolor=0xff0000ff:fontsize=12:text=bit10 %{metadata\\\\:lavfi.bitplanenoise.${1}.10}:x=4:y=(h/12)*10+6,\
          drawtext=fontfile=${fontfile}:fontcolor=black:fontsize=12:text='NON-RANDOM':x=4:y=(h/12)*11+6,\
          drawtext=fontfile=${fontfile}:fontcolor=black:fontsize=12:text='bit1=LSB':x=4:y=h-12[b3o];\
          \
          [b1o][b2o]overlay[bpn1];[bpn1][b3o]overlay,\
          drawbox=x=100:y=(ih/10)*1:c=green@0.2:t=1:h=1:w=iw,\
          drawbox=x=100:y=(ih/10)*2:c=green@0.2:t=1:h=1:w=iw,\
          drawbox=x=100:y=(ih/10)*3:c=green@0.2:t=1:h=1:w=iw,\
          drawbox=x=100:y=(ih/10)*4:c=green@0.2:t=1:h=1:w=iw,\
          drawbox=x=100:y=(ih/10)*5:c=green@0.5:t=1:h=1:w=iw,\
          drawbox=x=100:y=(ih/10)*6:c=green@0.2:t=1:h=1:w=iw,\
          drawbox=x=100:y=(ih/10)*7:c=green@0.2:t=1:h=1:w=iw,\
          drawbox=x=100:y=(ih/10)*8:c=green@0.2:t=1:h=1:w=iw,\
          drawbox=x=100:y=(ih/10)*9:c=green@0.2:t=1:h=1:w=iw",
        },
    },
    {
        "Value Highlight",
        0,
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field" },
            { Args_Type_Yuv,      0,   0,   0,   0, "Plane" },
            { Args_Type_Slider, 235,   0, 255,   1, "Value"},
            { Args_Type_ClrPck, 0x40e0d0,   0,   0,   0, ""},
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
                       "split[vhmask][vhpic];color=color=${4}[vhcolor];[vhpic]extractplanes=${2}[vhpic1];[vhcolor][vhpic1]scale2ref[vhcolor1][vhpic1];[vhmask]format=${pix_fmt},extractplanes=${2},lut=c0=if(eq(val\\,${3})\\,minval\\,maxval),[vhpic1]alphamerge,[vhcolor1]overlay",
            "il=l=d:c=d,split[vhmask][vhpic];color=color=${4}[vhcolor];[vhpic]extractplanes=${2}[vhpic1];[vhcolor][vhpic1]scale2ref[vhcolor1][vhpic1];[vhmask]format=${pix_fmt},extractplanes=${2},lut=c0=if(eq(val\\,${3})\\,minval\\,maxval),[vhpic1]alphamerge,[vhcolor1]overlay",
        },
    },
    {
        "Value Highlight (Range)",
        0,
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field" },
            { Args_Type_Yuv,      0,   0,   0,   0, "Plane" },
            { Args_Type_Slider, 235,   0, 255,   1, "Min"},
            { Args_Type_Slider, 255,   0, 255,   1, "Max"},
            { Args_Type_ClrPck, 0x40e0d0,   0,   0,   0, ""},
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
                       "split[vhrmask][vhrpic];color=color=${5}[vhrcolor];[vhrpic]extractplanes=${2}[vhrpic1];[vhrcolor][vhrpic1]scale2ref[vhrcolor1][vhrpic1];[vhrmask]format=${pix_fmt},extractplanes=${2},lut=c0=if(between(val\\,${3}\\,${4})\\,minval\\,maxval),[vhrpic1]alphamerge,[vhrcolor1]overlay",
            "il=l=d:c=d,split[vhrmask][vhrpic];color=color=${5}[vhrcolor];[vhrpic]extractplanes=${2}[vhrpic1];[vhrcolor][vhrpic1]scale2ref[vhrcolor1][vhrpic1];[vhrmask]format=${pix_fmt},extractplanes=${2},lut=c0=if(between(val\\,${3}\\,${4})\\,minval\\,maxval),[vhrpic1]alphamerge,[vhrcolor1]overlay",
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
        "Adjust Signal",
        0,
        {
            { Args_Type_Slider,   0,-180, 180,   1, "Black"},
            { Args_Type_Slider, 100,   0, 400, 100, "Contrast"},
            { Args_Type_Slider,   0,-180, 180,   1, "Hue"},
            { Args_Type_Slider,  10,   0,  30,  10, "Sat"},
            { Args_Type_Slider,   0,-128, 128,   1, "Cb Shift"},
            { Args_Type_Slider,   0,-128, 128,   1, "Cr Shift"},
            { Args_Type_None,          0,   0,   0,   0, nullptr },
        },
        {
            "lutyuv=y=(val+${1})*${2}:u=val+${5}:v=val+${6},hue=h=${3}:s=${4}",
        },
    },
    {
        "Chroma Shift",
        0,
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field" },
            { Args_Type_Slider,   0,-128, 128,   1, "Cr Horiz"},
            { Args_Type_Slider,   0,-128, 128,   1, "Cr Vert"},
            { Args_Type_Slider,   0,-128, 128,   1, "Cb Horiz"},
            { Args_Type_Slider,   0,-128, 128,   1, "Cb Vert"},
            { Args_Type_None,          0,   0,   0,   0, nullptr },
            { Args_Type_None,          0,   0,   0,   0, nullptr },
        },
        {
            "format=yuv444p,chromashift=cbh=${2}:cbv=${3}:crh=${4}:crv=${5}",
            "il=l=d:c=d,format=yuv444p,chromashift=cbh=${2}:cbv=${3}:crh=${4}:crv=${5}",
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
                       "limiter=min=${3}:max=${4}:planes=${2},shuffleplanes=${2}-1,histeq=strength=${5},format=gray,format=yuv444p",
                       "limiter=min=${3}:max=${4}:planes=7,histeq=strength=${5},format=yuv444p",
            "il=l=d:c=d,limiter=min=${3}:max=${4}:planes=${2},shuffleplanes=${2}-1,histeq=strength=${5},format=gray,format=yuv444p",
            "il=l=d:c=d,limiter=min=${3}:max=${4}:planes=7,histeq=strength=${5},format=yuv444p",
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
            "geq=lum=lum(X\\,Y)-lum(X-${1}\\,Y-${2})+pow(2\\,${bitdepth}-1):cb=cb(X\\,Y)-cb(X-${3}\\,Y-${4})+pow(2\\,${bitdepth}-1):cr=cr(X\\,Y)-cr(X-${3}\\,Y-${4})+pow(2\\,${bitdepth}-1),histeq=strength=${5}",
        },
    },
    {
        "Broadcast Range Pixels",
        0,
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field" },
            { Args_Type_ClrPck, 0x40e0d0,   0,   0,   0, ""},
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
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
                       "lutyuv=y=if(gt(val\\,maxval)\\,((maxval-minval+1)/(pow(2\\,${bitdepth})-maxval+1))*(val-maxval+1)+minval\\,0):u=(maxval+minval)/2:v=(maxval+minval)/2",
                       "lutyuv=y=if(lt(val\\,minval)\\,((maxval-minval+1)/minval+1)*(val+1)+minval\\,0):u=(maxval+minval)/2:v=(maxval+minval)/2",
            "il=l=d:c=d,lutyuv=y=if(gt(val\\,maxval)\\,((maxval-minval+1)/(pow(2\\,${bitdepth})-maxval+1))*(val-maxval+1)+minval\\,0):u=(maxval+minval)/2:v=(maxval+minval)/2",
            "il=l=d:c=d,lutyuv=y=if(lt(val\\,minval)\\,((maxval-minval+1)/minval+1)*(val+1)+minval\\,0):u=(maxval+minval)/2:v=(maxval+minval)/2",
        },
    },
    {
        "Temporal Outlier Pixels",
        0,
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field" },
            { Args_Type_ClrPck, 0x40e0d0,   0,   0,   0, ""},
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
            "format=yuv444p,signalstats=out=tout:c=${2}",
            "format=yuv444p,il=l=d:c=d,signalstats=out=tout:c=${2}",
        },
    },
    {
        "Vertical Line Repetition",
        0,
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field" },
            { Args_Type_ClrPck, 0x40e0d0,   0,   0,   0, ""},
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
            "format=yuv444p,signalstats=out=vrep:c=${2}",
            "format=yuv444p,signalstats=out=vrep:c=${2},il=l=d:c=d",
        },
    },
    {
        "Filmstrip",
        0,
        {
            { Args_Type_Slider,   8,   1,  12,   1, "Columns"},
            { Args_Type_Slider,   4,   1,  12,   1, "Rows"},
            { Args_Type_Toggle,   0,   0,   0,   0, "Field" },
            { Args_Type_Toggle,   0,   0,   0,   0, "FullSize" },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
            "scale=iw/${1}:ih/${2}:force_original_aspect_ratio=decrease,tile=${1}x${2}:overlap=${1}*${2}-1:init_padding=${1}*${2}-1",
            "tile=${1}x${2}:overlap=${1}*${2}-1:init_padding=${1}*${2}-1",
            "split=2[fst][fsb];[fst]field=top,scale=iw/${1}:ih/${2}:force_original_aspect_ratio=decrease[fst1];[fsb]field=bottom,scale=iw/${1}:ih/${2}:force_original_aspect_ratio=decrease[fsb1];\
             [fst1]tile=${1}x${2}:overlap=${1}*${2}-1:init_padding=${1}*${2}-1[fst2];\
             [fsb1]tile=${1}x${2}:overlap=${1}*${2}-1:init_padding=${1}*${2}-1[fsb2];[fst2][fsb2]vstack",
            "split=2[fst][fsb];[fst]field=top[fst1];[fsb]field=bottom[fsb1];\
             [fst1]tile=${1}x${2}:overlap=${1}*${2}-1:init_padding=${1}*${2}-1[fst2];\
             [fsb1]tile=${1}x${2}:overlap=${1}*${2}-1:init_padding=${1}*${2}-1[fsb2];[fst2][fsb2]vstack",
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
            "format=rgb24|yuv444p,crop=iw:1:0:${1}:0:1,tile=1x${2}:overlap=${2}-1:init_padding=${2}-1",
            "format=rgb24|yuv444p,split[a][b];[a]crop=iw:1:0:${1}:0:1,tile=1x${2}:overlap=${2}-1:init_padding=${2}-1,pad=iw:ih+1[a1];[b]crop=iw:1:0:${1}+1:0:1,tile=1x${2}:overlap=${2}-1:init_padding=${2}-1[b1];[a1][b1]vstack"
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
            { Args_Type_Slider,  30,   1, 100,   1, "scan_max" },
            { Args_Type_Slider,  27,  10,  70, 100, "spw" },
            { Args_Type_Toggle,   0,   0,   0,   0, "parity" },
            { Args_Type_Toggle,   1,   0,   0,   0, "zoom" },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
            { Args_Type_None,     0,   0,   0,   0, nullptr },
        },
        {
            "readvitc=scan_max=${1},readeia608=scan_max=${1}:spw=${2},drawtext=fontfile=${fontfile}:fontcolor=white:fontsize=36:box=1:boxcolor=black@0.5:x=(w-tw)/2:y=h*3/4-ascent:text=Line %{metadata\\\\:lavfi.readeia608.0.line\\\\:-} %{metadata\\\\:lavfi.readeia608.0.cc\\\\:------} - Line %{metadata\\\\:lavfi.readeia608.1.line\\\\:-} %{metadata\\\\:lavfi.readeia608.1.cc\\\\:------},drawtext=fontfile=${fontfile}:fontcolor=white:fontsize=36:box=1:boxcolor=black@0.5:x=(w-tw)/2:y=h*3/4-ascent*3:text=VITC %{metadata\\\\:lavfi.readvitc.tc_str\\\\:-- -- -- --}",
            "readvitc=scan_max=${1},readeia608=scan_max=${1}:spw=${2},crop=iw:${1}:0:0,scale=${width}:${height}:flags=neighbor,drawtext=fontfile=${fontfile}:fontcolor=white:fontsize=36:box=1:boxcolor=black@0.5:x=(w-tw)/2:y=h*3/4-ascent:text=Line %{metadata\\\\:lavfi.readeia608.0.line\\\\:-} %{metadata\\\\:lavfi.readeia608.0.cc\\\\:------} - Line %{metadata\\\\:lavfi.readeia608.1.line\\\\:-} %{metadata\\\\:lavfi.readeia608.1.cc\\\\:------},drawtext=fontfile=${fontfile}:fontcolor=white:fontsize=36:box=1:boxcolor=black@0.5:x=(w-tw)/2:y=h*3/4-ascent*3:text=VITC %{metadata\\\\:lavfi.readvitc.tc_str\\\\:-- -- -- --}",
            "readvitc=scan_max=${1},readeia608=scan_max=${1}:spw=${2}:chp=1,drawtext=fontfile=${fontfile}:fontcolor=white:fontsize=36:box=1:boxcolor=black@0.5:x=(w-tw)/2:y=h*3/4-ascent:text=Line %{metadata\\\\:lavfi.readeia608.0.line\\\\:-} %{metadata\\\\:lavfi.readeia608.0.cc\\\\:------} - Line %{metadata\\\\:lavfi.readeia608.1.line\\\\:-} %{metadata\\\\:lavfi.readeia608.1.cc\\\\:------},drawtext=fontfile=${fontfile}:fontcolor=white:fontsize=36:box=1:boxcolor=black@0.5:x=(w-tw)/2:y=h*3/4-ascent*3:text=VITC %{metadata\\\\:lavfi.readvitc.tc_str\\\\:-- -- -- --}",
            "readvitc=scan_max=${1},readeia608=scan_max=${1}:spw=${2}:chp=1,crop=iw:${1}:0:0,scale=${width}:${height}:flags=neighbor,drawtext=fontfile=${fontfile}:fontcolor=white:fontsize=36:box=1:boxcolor=black@0.5:x=(w-tw)/2:y=h*3/4-ascent:text=Line %{metadata\\\\:lavfi.readeia608.0.line\\\\:-} %{metadata\\\\:lavfi.readeia608.0.cc\\\\:------} - Line %{metadata\\\\:lavfi.readeia608.1.line\\\\:-} %{metadata\\\\:lavfi.readeia608.1.cc\\\\:------},drawtext=fontfile=${fontfile}:fontcolor=white:fontsize=36:box=1:boxcolor=black@0.5:x=(w-tw)/2:y=h*3/4-ascent*3:text=VITC %{metadata\\\\:lavfi.readvitc.tc_str\\\\:-- -- -- --}",
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
        "Datascope",
        0,
        {
            { Args_Type_Toggle,   0,   0,   0,   0, "Field" },
            { Args_Type_Slider,   0,   0,   0,   1, "x" },
            { Args_Type_Slider,   0,   0,   0,   1, "y" },
            { Args_Type_Slider,   1,   0,   2,   1, "DataMode" },
            { Args_Type_Slider,   1,   0,   1,   1, "Axis"},
            { Args_Type_Slider,   0,   0,   1,   1, "Dec" },
            { Args_Type_Toggle,   0,   0,   1,   1, "Show" },
        },
        {
                       "datascope=x=${2}:y=${3}:mode=${4}:axis=${5}:format=${6}",
                       "format=yuv444p,drawbox=x=${2}:y=${3}:color=yellow:thickness=4:width=32:height=4,drawbox=x=${2}:y=${3}:color=yellow:thickness=4:width=4:height=32",
            "il=l=d:c=d,datascope=x=${2}:y=${3}:mode=${4}:axis=${5}:format=${6}",
            "il=l=d:c=d,format=yuv444p,drawbox=x=${2}:y=${3}:color=yellow:thickness=4:width=32:height=4,drawbox=x=${2}:y=${3}:color=yellow:thickness=4:width=4:height=32",
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
            "scale='max(640\\,iw)':'max(480\\,ih)',pixscope=x=${2}/${width}:y=${3}/${height}:w=${4}:h=${5}",
            "il=l=d:c=d,scale='max(640\\,iw)':'max(480\\,ih)',pixscope=x=${2}/${width}:y=${3}/${height}:w=${4}:h=${5}",
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
            "crop=${3}:${4}:${1}:${2},scale=${width}:${height},\
            format=${pix_fmt:4},waveform=intensity=0.8:mode=column:mirror=1:c=1:f=${5}:graticule=green:flags=numbers+dots:scale=${6},scale=${width}:${height},setsar=1/1",
            "split[a][b];\
            [a]format=yuv444p,lutyuv=y=val/4,drawbox=w=${3}:h=${4}:x=${1}:y=${2}:color=invert:thickness=1[a1];\
            [b]crop=${3}:${4}:${1}:${2},scale=${width}:${height},\
            format=${pix_fmt:4},waveform=intensity=0.8:mode=column:mirror=1:c=1:f=${5}:graticule=green:flags=numbers+dots:scale=${6}[b1];\
            [a1][b1]scale2ref[a2][b1];\
            [a2][b1]blend=addition",
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
            { Args_Type_Slider,   3,   0,   5,   1, "Mode" },
            { Args_Type_Slider,   0,   0,   3,   1, "Peak" },
            { Args_Type_Toggle,   1,   0,   0,   0, "Background"},
        },
        {
            "crop=${3}:${4}:${1}:${2},\
            format=${pix_fmt:6},vectorscope=i=0.1:mode=${5}:envelope=${6}:colorspace=601:graticule=green:flags=name,pad=ih*${dar}:ih:(ow-iw)/2:(oh-ih)/2",
            "split[a][b];\
            [a]lutyuv=y=val/4,scale=${width}:${height},setsar=1/1,format=${pix_fmt:6},drawbox=w=${3}:h=${4}:x=${1}:y=${2}:color=invert:thickness=1[a1];\
            [b]crop=${3}:${4}:${1}:${2},\
            format=${pix_fmt:6},vectorscope=i=0.1:mode=${5}:envelope=${6}:colorspace=601:graticule=green:flags=name,pad=ih*${dar}:ih:(ow-iw)/2:(oh-ih)/2,scale=${width}:${height},setsar=1/1[b1];\
            [a1][b1]blend=addition",
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
            "aformat=sample_fmts=flt|fltp:channel_layouts=stereo,showwaves=mode=${3}:n=${2}:s=${width}x${height}:split_channels=0,negate",
            "aformat=sample_fmts=flt|fltp:channel_layouts=stereo,showwaves=mode=${3}:n=${2}:s=${width}x${height}:split_channels=1,negate",
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
            "showcqt=s=${width}x${height}",
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
            "aphasemeter=s=160x${width}:mpc=red:video=1[out0][out1];[out0]anullsink;[out1]transpose=0",
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
            "showfreqs=s=${width}x160:cmode=separate",
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
            "showvolume=w=${width}:h=40:f=0.95:dm=1",
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
            "abitscope=s=160x${width},transpose=2",
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
            "aformat=sample_fmts=flt|fltp:channel_layouts=stereo,ebur128=video=1:meter=${1}[out0][out1];[out1]anullsink;[out0]copy",
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

extern int FiltersListDefault_Count;

#endif // FILTERS_H
