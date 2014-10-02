/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include <Core/Core.h>
#include <cfloat>

const char* Version="0.5.0";
    
const struct per_plot_group PerPlotGroup [PlotType_Max]=
{
    //Y
    {
        PlotName_YMIN,      5,    0,  255,  3,  "Y",  true,
        "For all samples of the Y plane, plot the MAXimum, MIVimum, AVeraGe,\n"
        "LOW (10th percentile), and HIGH (90th percentile) values for each\n"
        "frame.\n"
        "The Y plane provides information about video brightness or luminance.",
    },
    //U
    {
        PlotName_UMIN,      5,    0,  255,  3,  "U",  true,
        "For all samples of the U plane, plot the MAXimum, MIVimum, AVeraGe,\n"
        "LOW (10th percentile), and HIGH (90th percentile) values for each\n"
        "frame.\n"
        "The U plane (along with V) provides information about video color.",
    },
    //V
    {
        PlotName_VMIN,      5,    0,  255,  3,  "V",  true,
        "For all samples of the V plane, plot the MAXimum, MIVimum, AVeraGe,\n"
        "LOW (10th percentile), and HIGH (90th percentile) values for each\n"
        "frame.\n"
        "The V plane (along with U) provides information about video color.",
    },
    //YDiff
    {
        PlotName_YDIF,      1,    0,  255,  3,  "YDiff", false,
        "YDIF plots the amount of differences between the Y plane of the current\n"
        "frame and the preceding one. It indicates the extent of visual change\n"
        "from one frame to the next.,"
    },
    //UDiff
    {
        PlotName_UDIF,      1,    0,  255,  3,  "UDiff", false,
        "UDIF plots the amount of differences between the U plane of the current\n"
        "frame and the preceding one. It indicates the extent of visual change\n"
        "from one frame to the next.,"
    },
    //VDiff
    {
        PlotName_VDIF,      1,    0,  255,  3,  "VDiff", false,
        "VDIF plots the amount of differences between the V plane of the current\n"
        "frame and the preceding one. It indicates the extent of visual change\n"
        "from one frame to the next.,"
    },
    //Diffs
    {
        PlotName_YDIF,      3,    0,  255,  3,  "Diffs",  true,
        "Plots YDIF, UDIF, and VDIF all together.",
    },
    //Sat
    {
        PlotName_SATMIN,    5,    0,  180,  4,  "Sat", true,
        "This filter does the equivalent of plotting all pixels in a vectorscope\n"
        "and measuring the distance from that plotted points to the center of the\n"
        "vectorscope. The MAXimum, MIVimum, AVeraGe, LOW (10th percentile), and\n"
        "HIGH (90th percentile) values for each frame are plotted.\n"
        "The SATAVG will provide an overall idea of how much color saturation exists within a given frame.\n"
        "The plot for SATMAX will highlight a number of errors that cause color levels that exceed broadcast range or are mathematically illegal.\n"
        "Values form 0 to 88.7 are considered within the 75% chrominance broadcast range.\n"
        "Values from 88.7 to 118.2 fall in between the 75% and 100% chrominance broadcast ranges.\n"
        "Values from 118.2 to the maximum value of 181.02 represent illegal YUV color data that\n"
        "can not be converted back to RGB without producing negatives or overflows,\n"
        "such values a indicative errors such as time base corrector errors or Digital Betacam damage.",
    },
    //Hue
    {
        PlotName_HUEMED,    2,    0,  360,  4,  "Hue", false,
        "The hue filter expresses the average color hue in degrees from 0 to 360.\n"
        "Degrees are measured clockwise starting from 0 at the bottom side of a\n"
        "vectorscope. Skin tone is 147 degrees. Hue is plotted via median and average.",
    },
    //TOUT
    {
        PlotName_TOUT,      1,    0,  0.9,  4,  "TOUT", false,
        "Pixels are labeled as temporal outliers (TOUT) if they are unlike the corresponding\n"
        "pixels of the previous and subsequent frames. Peaks in TOUT can show areas with\n"
        "skew or tracking issues which cause white speckle in the video. It can also\n"
        "indicate very noisy recordings.",
    },
    //VREP
    {
        PlotName_VREP,      1,    0,    1,  4,  "VREP", false,
        "Vertical Line Repetitions, or the VREP filter, is useful in detecting\n"
        "the use of a dropout compensator in the digitization of analog video. VREP\n"
        "plots the number of repeated horizontal lines which is untypical for an analog\n"
        "recording to naturally produce. VREP would also plot clearn born digital \n"
        "which has many very similar lines of video in the image.",
    },
    //BRNG
    {
        PlotName_BRNG,      1,    0,    1,  4,  "BRNG", false,
        "The BRNG filter is one that identifies the amount of pixels that fall\n"
        "outside the standard video broadcast range of 16-235 pixels for Y or\n"
        "16-240 for U and V.",
    },
    //CropW
    {
        PlotName_Crop_x1,   2,    0,    0,  4,  "CropW", false,
        "CropW plots the number of columns of pixels would could be safely removed\n"
        "from the left or right side of the image without removing any non-black\n"
        "pixels. It would detect video frames with pillarboxing.",
    },
    //CropH
    {
        PlotName_Crop_y1,   2,    0,    0,  4,  "CropH", false,
        "CropW plots the number of row of pixels would could be safely removed\n"
        "from the top or bottom side of the image without removing any non-black\n"
        "pixels. It would detect video frames with letterboxing.",
    },
    //CropF
    {
        PlotName_Crop_w,   2,    0,    0,  4,  "CropF", false,
        "Presents the total number of rows (Crop Height) and columns (Crop Width)\n"
        "which could be removed from the edges to only remove black pixels.",
    },
    //MSEf
    {
        PlotName_MSE_v,     3,    0,    0,  4,  "MSEf", false,
        "Plots an assessment of visual difference of field 1 versus field 2 via\n"
        "Mean Square Error for each plane (Y, U, and V). Higher values may be\n"
        "indicative of differences between the images of field 1 and field 2 as\n"
        "may be caused by a head clog or playback error.",
    },
    //PSNRf
    {
        PlotName_PSNR_v,    3,    0,    0,  4,  "PSNRf", false,
        "Plots an assessment of visual difference of field 1 versus field 2 via\n"
        "Peak Signal to Noise Ratio for each plane (Y, U, and V). Lower values may\n"
        "be indicative of differences between the images of field 1 and field 2 as\n"
        "may be caused by a head clog or playback error.",
    },
    //Internal
    {
        PlotName_Max,       0,    0,    0,  0,  "",
        "",
    },
};

const struct per_plot_item PerPlotName [PlotName_Max]=
{
    //Y
    { PlotType_Y,       PlotType_Max,       "YMIN",       "lavfi.values.YMIN",    "lavfi.signalstats.YMIN",    0,   false,  DBL_MAX, DBL_MAX },
    { PlotType_Y,       PlotType_Max,       "YLOW",       "lavfi.values.YLOW",    "lavfi.signalstats.YLOW",    0,   false,  DBL_MAX, DBL_MAX },
    { PlotType_Y,       PlotType_Max,       "YAVG",       "lavfi.values.YAVG",    "lavfi.signalstats.YAVG",    0,   false,  DBL_MAX, DBL_MAX },
    { PlotType_Y,       PlotType_Max,       "YHIGH",      "lavfi.values.YHIGH",   "lavfi.signalstats.YHIGH",   0,   false,  DBL_MAX, DBL_MAX },
    { PlotType_Y,       PlotType_Max,       "YMAX",       "lavfi.values.YMAX",    "lavfi.signalstats.YMAX",    0,   true,   DBL_MAX, DBL_MAX },
    //U
    { PlotType_U,       PlotType_Max,       "UMIN",       "lavfi.values.UMIN",    "lavfi.signalstats.UMIN",    0,   false,  DBL_MAX, DBL_MAX },
    { PlotType_U,       PlotType_Max,       "ULOW",       "lavfi.values.ULOW",    "lavfi.signalstats.ULOW",    0,   false,  DBL_MAX, DBL_MAX },
    { PlotType_U,       PlotType_Max,       "UAVG",       "lavfi.values.UAVG",    "lavfi.signalstats.UAVG",    0,   false,  DBL_MAX, DBL_MAX },
    { PlotType_U,       PlotType_Max,       "UHIGH",      "lavfi.values.UHIGH",   "lavfi.signalstats.UHIGH",   0,   false,  DBL_MAX, DBL_MAX },
    { PlotType_U,       PlotType_Max,       "UMAX",       "lavfi.values.UMAX",    "lavfi.signalstats.UMAX",    0,   true,   DBL_MAX, DBL_MAX },
    //V
    { PlotType_V,       PlotType_Max,       "VMIN",       "lavfi.values.VMIN",    "lavfi.signalstats.VMIN",    0,  false,  DBL_MAX, DBL_MAX },
    { PlotType_V,       PlotType_Max,       "VLOW",       "lavfi.values.VLOW",    "lavfi.signalstats.VLOW",    0,  false,  DBL_MAX, DBL_MAX },
    { PlotType_V,       PlotType_Max,       "VAVG",       "lavfi.values.VAVG",    "lavfi.signalstats.VAVG",    0,  false,  DBL_MAX, DBL_MAX },
    { PlotType_V,       PlotType_Max,       "VHIGH",      "lavfi.values.VHIGH",   "lavfi.signalstats.VHIGH",   0,  false,  DBL_MAX, DBL_MAX },
    { PlotType_V,       PlotType_Max,       "VMAX",       "lavfi.values.VMAX",    "lavfi.signalstats.VMAX",    0,  true,   DBL_MAX, DBL_MAX },
    //Diffs
    { PlotType_YDiff,   PlotType_Diffs,     "YDIF",       "lavfi.values.YDIF",    "lavfi.signalstats.YDIF",    5,  false,  DBL_MAX, DBL_MAX },
    { PlotType_UDiff,   PlotType_Diffs,     "UDIF",       "lavfi.values.UDIF",    "lavfi.signalstats.UDIF",    5,  false,  DBL_MAX, DBL_MAX },
    { PlotType_VDiff,   PlotType_Diffs,     "VDIF",       "lavfi.values.VDIF",    "lavfi.signalstats.VDIF",    5,  false,  DBL_MAX, DBL_MAX },
  //{ PlotType_YDiffX,  PlotType_Max,       "YDIF1",      "lavfi.values.YDIF1",   "lavfi.signalstats.YDIF1",   5,  false,  DBL_MAX, DBL_MAX },
  //{ PlotType_YDiffX,  PlotType_Max,       "YDIF2",      "lavfi.values.YDIF2",   "lavfi.signalstats.YDIF2",   5,  true,   DBL_MAX, DBL_MAX },
    //Sat
    { PlotType_Sat,     PlotType_Max,       "SATMIN",     "lavfi.values.SATMIN",  "lavfi.signalstats.SATMIN",  0,  false,  DBL_MAX, DBL_MAX },
    { PlotType_Sat,     PlotType_Max,       "SATLOW",     "lavfi.values.SATLOW",  "lavfi.signalstats.SATLOW",  0,  false,  DBL_MAX, DBL_MAX },
    { PlotType_Sat,     PlotType_Max,       "SATAVG",     "lavfi.values.SATAVG",  "lavfi.signalstats.SATAVG",  0,  false,  DBL_MAX, DBL_MAX },
    { PlotType_Sat,     PlotType_Max,       "SATHIGH",    "lavfi.values.SATHIGH", "lavfi.signalstats.SATHIGH", 0,  false,  DBL_MAX, DBL_MAX },
    { PlotType_Sat,     PlotType_Max,       "SATMAX",     "lavfi.values.SATMAX",  "lavfi.signalstats.SATMAX",  0,  true,      88.7,   118.2 },
    //Hue
  //{ PlotType_Hue,     PlotType_Max,       "HUEMOD",     "lavfi.values.HUEMOD",  "lavfi.signalstats.HUEMOD",  0,  false,  DBL_MAX, DBL_MAX },
    { PlotType_Hue,     PlotType_Max,       "HUEMED",     "lavfi.values.HUEMED",  "lavfi.signalstats.HUEMED",  0,  false,  DBL_MAX, DBL_MAX },
    { PlotType_Hue,     PlotType_Max,       "HUEAVG",     "lavfi.values.HUEAVG",  "lavfi.signalstats.HUEAVG",  0,  true,   DBL_MAX, DBL_MAX },
    //Others
    { PlotType_TOUT,    PlotType_Max,       "TOUT",       "lavfi.values.TOUT",    "lavfi.signalstats.TOUT",    8,  false,    0.005, DBL_MAX },
  //{ PlotType_HEAD,    PlotType_Max,       "HEAD",       "lavfi.values.HEAD",    "lavfi.signalstats.HEAD",    8,  false,  DBL_MAX, DBL_MAX },
    { PlotType_VREP,    PlotType_Max,       "VREP",       "lavfi.values.VREP",    "lavfi.signalstats.VREP",    8,  false,  DBL_MAX, DBL_MAX },
    { PlotType_BRNG,    PlotType_Max,       "BRNG",       "lavfi.values.RANG",    "lavfi.signalstats.BRNG",    8,  true,         0, DBL_MAX },
    //Crop
    { PlotType_CropW,   PlotType_Max,       "Crop Left",  "",                     "lavfi.cropdetect.x1",       0,  false,  DBL_MAX, DBL_MAX },
    { PlotType_CropW,   PlotType_Max,       "Crop Right", "",                     "lavfi.cropdetect.x2",       0,  false,  DBL_MAX, DBL_MAX },
    { PlotType_CropH,   PlotType_Max,       "Crop Top",   "",                     "lavfi.cropdetect.y1",       0,  false,  DBL_MAX, DBL_MAX },
    { PlotType_CropH,   PlotType_Max,       "Crop Bottom","",                     "lavfi.cropdetect.y2",       0,  true,   DBL_MAX, DBL_MAX },
    { PlotType_CropF,   PlotType_Max,       "Crop Width", "",                     "lavfi.cropdetect.w",        0,  false,  DBL_MAX, DBL_MAX },
    { PlotType_CropF,   PlotType_Max,       "Crop Height","",                     "lavfi.cropdetect.h",        0,  true,   DBL_MAX, DBL_MAX },
    //MSEf
    { PlotType_MSE,     PlotType_Max,       "MSEf Y",     "lavfi.psnr.mse.y",     "lavfi.psnr.mse.y",          2,  false,     1000, DBL_MAX },
    { PlotType_MSE,     PlotType_Max,       "MSEf U",     "lavfi.psnr.mse.u",     "lavfi.psnr.mse.u",          2,  false,  DBL_MAX, DBL_MAX },
    { PlotType_MSE,     PlotType_Max,       "MSEf V",     "lavfi.psnr.mse.v",     "lavfi.psnr.mse.v",          2,  true,   DBL_MAX, DBL_MAX },
    //PSNRf
    { PlotType_PSNR,    PlotType_Max,       "PSNRf Y",    "lavfi.psnr.psnr.y",    "lavfi.psnr.psnr.y",         2,  false,  DBL_MAX, DBL_MAX },
    { PlotType_PSNR,    PlotType_Max,       "PSNRf U",    "lavfi.psnr.psnr.u",    "lavfi.psnr.psnr.u",         2,  false,  DBL_MAX, DBL_MAX },
    { PlotType_PSNR,    PlotType_Max,       "PSNRf V",    "lavfi.psnr.psnr.v",    "lavfi.psnr.psnr.v",         2,  true,   DBL_MAX, DBL_MAX },
};
