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
    Item_YBITS,
    Item_UBITS,
    Item_VBITS,
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
    //SSIMf
    Item_SSIM_Y,
    Item_SSIM_U,
    Item_SSIM_V,
    Item_SSIM_All,
    //idet.single
    Item_IDET_S_BFF,
    Item_IDET_S_TFF,
    Item_IDET_S_PROG,
    Item_IDET_S_UND,
    //idet.multiple
    Item_IDET_M_BFF,
    Item_IDET_M_TFF,
    Item_IDET_M_PROG,
    Item_IDET_M_UND,
    //idet.repeat
    Item_IDET_R_B,
    Item_IDET_R_T,
    Item_IDET_R_N,
    //deflicker.ratio
    Item_DEFL,
    //entropy
    Item_ENTR_Y,
    Item_ENTR_U,
    Item_ENTR_V,
    //entropy-diff
    Item_ENTR_Y_D,
    Item_ENTR_U_D,
    Item_ENTR_V_D,
    //pkt_duration_time & pkt_size
    Item_pkt_duration_time,
    Item_pkt_size,
    // block and blur
    Item_blockdetect,
    Item_blurdetect,
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
    Group_YUVB,
    Group_CropW,
    Group_CropH,
    Group_CropF,
    Group_MSE,
    Group_PSNR,
    Group_SSIM,
    Group_IDET_S,
    Group_IDET_M,
    Group_IDET_R,
    Group_DEFL,
    Group_ENTR,
    Group_ENTRD,
    Group_pkt_duration_time,
    Group_pkt_size,
    Group_blockdetect,
    Group_blurdetect,
    Group_VideoMax
};

extern struct per_group  VideoPerGroup    [Group_VideoMax];
extern const struct per_item   VideoPerItem     [Item_VideoMax];

#endif // Core_H
