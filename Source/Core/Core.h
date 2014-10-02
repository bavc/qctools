/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef Core_H
#define Core_H

#include <new>

#ifndef UNICODE
    #define UNICODE
#endif //UNICODE

extern const char* Version;

const std::size_t PlotName_Begin=21;

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
    //Diffs
    PlotName_YDIF,
    PlotName_UDIF,
    PlotName_VDIF,
    //PlotName_YDIF1,
    //PlotName_YDIF2,
    //Sat
    PlotName_SATMIN,
    PlotName_SATLOW,
    PlotName_SATAVG,
    PlotName_SATHIGH,
    PlotName_SATMAX,
    //Hue
    //PlotName_HUEMOD,
    PlotName_HUEMED,
    PlotName_HUEAVG,
    //Other
    PlotName_TOUT,
    //PlotName_HEAD,
    PlotName_VREP,
    PlotName_BRNG,
    //Crop
    PlotName_Crop_x1,
    PlotName_Crop_x2,
    PlotName_Crop_y1,
    PlotName_Crop_y2,
    PlotName_Crop_w,
    PlotName_Crop_h,
    //MSEf
    PlotName_MSE_v,
    PlotName_MSE_u,
    PlotName_MSE_y,
    //PSNRf
    PlotName_PSNR_v,
    PlotName_PSNR_u,
    PlotName_PSNR_y,
    //Internal
    PlotName_Max
};

enum PlotType
{
    PlotType_Y,
    PlotType_U,
    PlotType_V,
    PlotType_YDiff,
    //PlotType_YDiffX,
    PlotType_UDiff,
    PlotType_VDiff,
    PlotType_Diffs,
    PlotType_Sat,
    PlotType_Hue,
    PlotType_TOUT,
    //PlotType_HEAD,
    PlotType_VREP,
    PlotType_BRNG,
    PlotType_CropW,
    PlotType_CropH,
    PlotType_CropF,
    PlotType_MSE,
    PlotType_PSNR,
    PlotType_Axis,
    PlotType_Max
};

struct per_plot_group
{
    const   PlotName    Start;
    const   std::size_t Count;
    const   double      Min;
    const   double      Max;
    const   double      StepsCount;
    const   char*       Name;
    const   bool        CheckedByDefault;
    const   char*       Description;
};
struct per_plot_item
{
    const   PlotType    Group1;
    const   PlotType    Group2;
    const   char*       Name;
    const   char*       FFmpeg_Name;
    const   char*       FFmpeg_Name_2_3;
    const   int         DigitsAfterComma;
    const   bool        NewLine;
    const   double      DefaultLimit;
    const   double      DefaultLimit2;
};
extern const struct per_plot_group  PerPlotGroup    [PlotType_Max];
extern const struct per_plot_item   PerPlotName     [PlotName_Max];

#endif // Core_H
