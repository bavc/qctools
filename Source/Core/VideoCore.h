/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef VideoCore_H
#define VideoCore_H

#include <Core/Core.h>

const size_t VideoItem_Begin=21;

enum VideoItem
{
    //Y
    Item_YMIN,
    Item_YLOW,
    Item_YAVG,
    Item_YHIGH,
    Item_YMAX,
    //U
    Item_UMIN,
    Item_ULOW,
    Item_UAVG,
    Item_UHIGH,
    Item_UMAX,
    //V
    Item_VMIN,
    Item_VLOW,
    Item_VAVG,
    Item_VHIGH,
    Item_VMAX,
    //Diffs
    Item_VDIF,
    Item_UDIF,
    Item_YDIF,
    //Item_YDIF1,
    //Item_YDIF2,
    //Sat
    Item_SATMIN,
    Item_SATLOW,
    Item_SATAVG,
    Item_SATHIGH,
    Item_SATMAX,
    //Hue
    //Item_HUEMOD,
    Item_HUEMED,
    Item_HUEAVG,
    //Other
    Item_TOUT,
    //Item_HEAD,
    Item_VREP,
    Item_BRNG,
    //Crop
    Item_Crop_x1,
    Item_Crop_x2,
    Item_Crop_y1,
    Item_Crop_y2,
    Item_Crop_w,
    Item_Crop_h,
    //MSEf
    Item_MSE_v,
    Item_MSE_u,
    Item_MSE_y,
    //PSNRf
    Item_PSNR_v,
    Item_PSNR_u,
    Item_PSNR_y,
    //Internal
    Item_VideoMax
};

enum VideoGroup
{
    Group_Y,
    Group_U,
    Group_V,
    Group_YDiff,
    //Group_YDiffX,
    Group_UDiff,
    Group_VDiff,
    Group_Diffs,
    Group_Sat,
    Group_Hue,
    Group_TOUT,
    //Group_HEAD,
    Group_VREP,
    Group_BRNG,
    Group_CropW,
    Group_CropH,
    Group_CropF,
    Group_MSE,
    Group_PSNR,
    Group_VideoMax
};

extern const struct per_group  VideoPerGroup    [Group_VideoMax];
extern const struct per_item   VideoPerItem     [Item_VideoMax];

#endif // Core_H
