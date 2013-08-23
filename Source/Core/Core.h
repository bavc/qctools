#ifndef Core_H
#define Core_H

#include <new>

const size_t PlotName_Begin=21;

enum PlotName
{
    //Y
    PlotName_YMIN,
    PlotName_YLOW,
    PlotName_YAVG,
    PlotName_YHIGH,
    PlotName_YMAX,
    //U
    PlotName_UMIN,
    PlotName_ULOW,
    PlotName_UAVG,
    PlotName_UHIGH,
    PlotName_UMAX,
    //V
    PlotName_VMIN,
    PlotName_VLOW,
    PlotName_VAVG,
    PlotName_VHIGH,
    PlotName_VMAX,
    //DIF
    PlotName_YDIF,
    PlotName_UDIF,
    PlotName_VDIF,
    PlotName_YDIF1,
    PlotName_YDIF2,
    //Other
    PlotName_TOUT,
    PlotName_VREP,
    PlotName_RANG,
    PlotName_HEAD,
    PlotName_Max
};

extern const char* Names[PlotName_Max];

enum PlotType
{
    PlotType_Y,
    PlotType_U,
    PlotType_V,
    PlotType_YDiff,
    PlotType_YDiffX,
    PlotType_UDiff,
    PlotType_VDiff,
    PlotType_Diffs,
    PlotType_TOUT,
    PlotType_VREP,
    PlotType_HEAD,
    PlotType_RANG,
    PlotType_Max
};

extern size_t StatsFile_Positions[PlotType_Max];
extern size_t StatsFile_Counts[PlotType_Max];

#endif // Core_H
