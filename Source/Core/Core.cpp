#include <Core/Core.h>

const char* Names[PlotName_Max]=
{
    //Y
    "YMIN",
    "YLOW",
    "YAVG",
    "YHIGH",
    "YMAX",
    //U
    "UMIN",
    "ULOW",
    "UAVG",
    "UHIGH",
    "UMAX",
    //V
    "VMIN",
    "VLOW",
    "VAVG",
    "VHIGH",
    "VMAX",
    //DIFF
    "YDIF",
    "UDIF",
    "VDIF",
    "YDIF1",
    "YDIF2",
    //Others
    "TOUT",
    "VREP",
    "RANG",
    "HEAD",
};

size_t StatsFile_Positions[PlotType_Max]=
{
     0,
     5,
    10,
    15,
    18,
    16,
    17,
    15,
    20,
    21,
    23,
    22,
};

size_t StatsFile_Counts[PlotType_Max]=
{
    5,
    5,
    5,
    1,
    2,
    1,
    1,
    3,
    1,
    1,
    1,
    1,
};