/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include <Core/VideoCore.h>
#include <cfloat>

struct per_group VideoPerGroup [Group_VideoMax]=
{
    //Y
    {
        Item_YMIN,      5,    "0",  "(function() { return two_pow_bitsPerRawSample_minus_one; })",  3,  "Y",  true,
        "For all samples of the Y plane, plot the MAXimum, MIVimum, AVeraGe,\n"
        "LOW (10th percentile), and HIGH (90th percentile) values for each frame.\n"
        "The Y plane provides information about video brightness or luminance.",
        ActiveFilter_Video_signalstats,
        "MinMaxOfThePlot",
    },
    //U
    {
        Item_UMIN,      5,    "0",  "(function() { return two_pow_bitsPerRawSample_minus_one; })",  3,  "U",  true,
        "For all samples of the U plane, plot the MAXimum, MIVimum, AVeraGe,\n"
        "LOW (10th percentile), and HIGH (90th percentile) values for each frame.\n"
        "The U plane (along with V) provides information about video color.",
        ActiveFilter_Video_signalstats,
        "Formula",
    },
    //V
    {
        Item_VMIN,      5,    "0",  "(function() { return two_pow_bitsPerRawSample_minus_one; })",  3,  "V",  true,
        "For all samples of the V plane, plot the MAXimum, MIVimum, AVeraGe,\n"
        "LOW (10th percentile), and HIGH (90th percentile) values for each frame.\n"
        "The V plane (along with U) provides information about video color.",
        ActiveFilter_Video_signalstats,
        "Custom;0;55"
    },
    //YDiff
    {
        Item_YDIF,      1,    "0",  nullptr,  3,  "Y Differences", false,
        "YDIF plots the amount of differences between the Y plane of the current\n"
        "frame and the preceding one. It indicates the extent of visual change\n"
        "from one frame to the next.",
        ActiveFilter_Video_signalstats,
    },
    //UDiff
    {
        Item_UDIF,      1,    "0",  nullptr,  3,  "U Differences", false,
        "UDIF plots the amount of differences between the U plane of the current\n"
        "frame and the preceding one. It indicates the extent of visual change\n"
        "from one frame to the next.",
        ActiveFilter_Video_signalstats,
    },
    //VDiff
    {
        Item_VDIF,      1,    "0",  nullptr,  3,  "V Differences", false,
        "VDIF plots the amount of differences between the V plane of the current\n"
        "frame and the preceding one. It indicates the extent of visual change\n"
        "from one frame to the next.",
        ActiveFilter_Video_signalstats,
    },
    //Diffs
    {
        Item_VDIF,      3,    "0",  nullptr,  3,  "Differences",  true,
        "Plots YDIF, UDIF, and VDIF all together.",
        ActiveFilter_Video_signalstats,
    },
    //Sat
    {
        Item_SATMIN,    5,    "0",  "(function() { return sqrt_pow_bitsPerRawSample_2; })",  4,  "Saturation", true,
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
        ActiveFilter_Video_signalstats,
    },
    //Hue
    {
        Item_HUEMED,    2,    "0",  "360",  4,  "Hue", false,
        "The hue filter expresses the average color hue in degrees from 0 to 360.\n"
        "Degrees are measured clockwise starting from 0 at the bottom side of a\n"
        "vectorscope. Skin tone is 147 degrees. Hue is plotted via median and average.",
        ActiveFilter_Video_signalstats,
    },
    //TOUT
    {
        Item_TOUT,      1,    "0",  nullptr,  4,  "Temporal Outliers (TOUT)", false,
        "Pixels are labeled as temporal outliers (TOUT) if they are unlike the corresponding\n"
        "pixels of the previous and subsequent frames. Peaks in TOUT can show areas with\n"
        "skew or tracking issues which cause white speckle in the video. It can also\n"
        "indicate very noisy recordings.",
        ActiveFilter_Video_signalstats,
    },
    //VREP
    {
        Item_VREP,      1,    "0",  nullptr,  4,  "Line Repetition (VREP)", false,
        "Vertical Line Repetitions, or the VREP filter, is useful in detecting\n"
        "the use of a dropout compensator in the digitization of analog video. VREP\n"
        "plots the number of repeated horizontal lines which is untypical for an analog\n"
        "recording to naturally produce. VREP would also plot clearn born digital \n"
        "which has many very similar lines of video in the image.",
        ActiveFilter_Video_signalstats,
    },
    //BRNG
    {
        Item_BRNG,      1,    "0",  nullptr,  4,  "Broadcast Range", false,
        "The BRNG filter is one that identifies the amount of pixels that fall\n"
        "outside the standard video broadcast range of 16-235 pixels for Y or\n"
        "16-240 for U and V.",
        ActiveFilter_Video_signalstats,
    },
    //Bit depths
    {
        Item_YBITS,     3,    "0",  "16",  4,  "Active Bit Depths", false,
        "These graphs show how many consecutive lower bits of used, per\n"
        "frame per plane. For instance, a 10-bit v210 video which plots at\n"
        "8 may reveal that some processing before encoding the v210 removed\n"
        "the least significant two bits.",
        ActiveFilter_Video_signalstats,
    },
    //CropW
    {
        Item_Crop_x1,   2,    "0",  nullptr,  4,  "Crop Width", false,
        "CropW plots the number of columns of pixels would could be safely removed\n"
        "from the left or right side of the image without removing any non-black\n"
        "pixels. It would detect video frames with pillarboxing.",
        ActiveFilter_Video_cropdetect,
    },
    //CropH
    {
        Item_Crop_y1,   2,    "0",  nullptr,  4,  "Crop Height", false,
        "CropW plots the number of row of pixels would could be safely removed\n"
        "from the top or bottom side of the image without removing any non-black\n"
        "pixels. It would detect video frames with letterboxing.",
        ActiveFilter_Video_cropdetect,
    },
    //CropF
    {
        Item_Crop_w,   2,    "0",  nullptr,  4,  "Crop Frame", false,
        "Presents the total number of rows (Crop Height) and columns (Crop Width)\n"
        "which could be removed from the edges to only remove black pixels.",
        ActiveFilter_Video_cropdetect,
    },
    //MSEf
    {
        Item_MSE_v,     3,    "0",  nullptr,  4,  "Fields compared via MSE", false,
        "Plots an assessment of visual difference of field 1 versus field 2 via\n"
        "Mean Square Error for each plane (Y, U, and V). Higher values may be\n"
        "indicative of differences between the images of field 1 and field 2 as\n"
        "may be caused by a head clog or playback error.",
        ActiveFilter_Video_Psnr,
    },
    //PSNRf
    {
        Item_PSNR_v,    3,    "0",  nullptr,  4,  "Fields compared via PSNR", false,
        "Plots an assessment of visual difference of field 1 versus field 2 via\n"
        "Peak Signal to Noise Ratio for each plane (Y, U, and V). Lower values may\n"
        "be indicative of differences between the images of field 1 and field 2 as\n"
        "may be caused by a head clog or playback error.",
        ActiveFilter_Video_Psnr,
    },
    //SSIMf
    {
        Item_SSIM_Y,    4,    "0",  nullptr,  4,  "Fields compared via SSIM", false,
        "Plots an assessment of visual difference of field 1 versus field 2 via\n"
        "SSIM (Structural SImilarity Metric) for each plane (Y, U, and V). Lower values may\n"
        "be indicative of differences between the images of field 1 and field 2 as\n"
        "may be caused by a head clog or playback error.",
        ActiveFilter_Video_Ssim,
    },
    //idet.single
    {
        Item_IDET_S_BFF,    4,    "0",    "2",  4,  "Interlacement Detection (single frame)", false,
        "Plots an interpretation of the interlacement pattern of the visual image.\n"
        "This version uses single frame detection which considers only immediately\n"
        "adjacent frames when classifying each frame. Each frame's classification is\n"
        "plotted with a half-life of 1.",
        ActiveFilter_Video_Idet,
    },
    //idet.multiple
    {
        Item_IDET_M_BFF,    4,    "0",    "2",  4,  "Interlacement Detection (multiple frame)", false,
        "Plots an interpretation of the interlacement pattern of the visual image.\n"
        "This version uses multiple frame detection which incorporates the classification\n"
        "history of previous frames. Each frame's classification is plotted with a\n"
        "half-life of 1.",
        ActiveFilter_Video_Idet,
    },
    //idet.repeat
    {
        Item_IDET_R_B,     3,    "0",    "2",  4,  "Interlacement Detection (repeating fields)", false,
        "Plots an interpretation of the interlacement pattern of the visual image.\n"
        "This plot shows fields that are repeated between adjacent frames (a sign\n"
        "of telecine). Each frame's classification is plotted with a half-life of 1.",
        ActiveFilter_Video_Idet,
    },
    //deflicker
    {
        Item_DEFL,     1,    "-1",    "1",  4,  "Flicker", false,
        "Plots a quantification of the temporal frame luminance variation by an\n"
        "arithmetric mean of sets of 5 frames. The plotted value shows the relative\n"
        "change in luminance that would be used by libavfilter's deflicker filter.",
        ActiveFilter_Video_Deflicker,
    },
    //entropy
    {
        Item_ENTR_Y,     3,    "0",    "1",  4,  "Entropy", true,
        "Entropy\n"
        "Plots the graylevel entropy of the histogram of the color channels.\n"
        "A color channel with only a single shade will have entropy of 0,\n"
        "while a channel using all shades will be 1.",
        ActiveFilter_Video_Entropy,
    },
    //entropy-diff
    {
        Item_ENTR_Y_D,   3,    "0",    "1",  4,  "Entropy frame-to-frame difference", true,
        "Entropy Difference\n"
        "Plots the frame-to-frame difference in the graylevel entropy\n"
        "of the histogram of the color channels. Incoherancy in plotted\n"
        "values may help indicate a damaged digital tape source (such as\n"
        "scratched D5 tape) or highlight digital manipulation of the image.\n"
        "The values should correspond to chaos or discontinuity in the\n"
        "histogram plot.",
        ActiveFilter_Video_EntropyDiff,
    },
    //Item_pkt_duration_time
    {
        Item_pkt_duration_time,     1,    "0",  nullptr,  4,  "Packet Duration", false,
        "Plots the duration in seconds of each frame. If the file is of constant\n"
        "frame rate than this should be a straight line.",
        ActiveFilter_Video_signalstats,
    },
    //Item_pkt_size
    {
        Item_pkt_size,     1,    "0",  nullptr,  4,  "Packet Size", false,
        "Plots the size in bytes of each frame. If the file is of uncompressed\n"
        "frame rate than this should be a straight line, but a lossless or\n"
        "lossy file should show the variety of frame sizes.",
        ActiveFilter_Video_signalstats,
    },
    //Item_blockdetect
    {
        Item_blockdetect,  1,    "0",  "50",  4,  "Blockiness", false,
        "Blockiness.",
        ActiveFilter_Video_blockdetect,
    },
    //Item_blurdetect
    {
        Item_blurdetect,   1,    "0",  "50",  4,  "Blurriness", false,
        "Blurriness",
        ActiveFilter_Video_blurdetect,
    },

    //const   std::size_t Start; //Item
    //const   std::size_t Count;
    //const   double      Min;
    //const   double      Max;
    //const   double      StepsCount;
    //const   char*       Name;
    //const   bool        CheckedByDefault;
    //const   char*       Description;
    //activefilter        ActiveFilterGroup;
};

const struct per_item VideoPerItem [Item_VideoMax]=
{
    //Y
    { Group_Y,       Group_VideoMax,       "Y MIN",         "lavfi.signalstats.YMIN",    0,   false,  DBL_MAX, DBL_MAX, ActiveFilter_Video_signalstats, "silver",     1 },
    { Group_Y,       Group_VideoMax,       "Y LOW",         "lavfi.signalstats.YLOW",    0,   false,  DBL_MAX, DBL_MAX, ActiveFilter_Video_signalstats, "darkgreen",  1, "Y MIN;teal;0.2" },
    { Group_Y,       Group_VideoMax,       "Y AVG",         "lavfi.signalstats.YAVG",    0,   false,  DBL_MAX, DBL_MAX, ActiveFilter_Video_signalstats, "black",      1, "Y LOW;teal;0.8" },
    { Group_Y,       Group_VideoMax,       "Y HIGH",        "lavfi.signalstats.YHIGH",   0,   false,  DBL_MAX, DBL_MAX, ActiveFilter_Video_signalstats, "lightgreen", 1, "Y AVG;teal;0.8" },
    { Group_Y,       Group_VideoMax,       "Y MAX",         "lavfi.signalstats.YMAX",    0,   true,   DBL_MAX, DBL_MAX, ActiveFilter_Video_signalstats, "gray",       1, "Y HIGH;teal;0.2" },
    //U
    { Group_U,       Group_VideoMax,       "U MIN",         "lavfi.signalstats.UMIN",    0,   false,  DBL_MAX, DBL_MAX, ActiveFilter_Video_signalstats, "silver",     1 },
    { Group_U,       Group_VideoMax,       "U LOW",         "lavfi.signalstats.ULOW",    0,   false,  DBL_MAX, DBL_MAX, ActiveFilter_Video_signalstats, "darkblue",   1, "U MIN;darkblue;0.2" },
    { Group_U,       Group_VideoMax,       "U AVG",         "lavfi.signalstats.UAVG",    0,   false,  DBL_MAX, DBL_MAX, ActiveFilter_Video_signalstats, "black",      1, "U LOW;darkblue;0.8" },
    { Group_U,       Group_VideoMax,       "U HIGH",        "lavfi.signalstats.UHIGH",   0,   false,  DBL_MAX, DBL_MAX, ActiveFilter_Video_signalstats, "lightblue",  1, "U AVG;darkblue;0.8" },
    { Group_U,       Group_VideoMax,       "U MAX",         "lavfi.signalstats.UMAX",    0,   true,   DBL_MAX, DBL_MAX, ActiveFilter_Video_signalstats, "gray",       1, "U HIGH;darkblue;0.2" },
    //V
    { Group_V,       Group_VideoMax,       "V MIN",         "lavfi.signalstats.VMIN",    0,   false,  DBL_MAX, DBL_MAX, ActiveFilter_Video_signalstats, "silver",     1 },
    { Group_V,       Group_VideoMax,       "V LOW",         "lavfi.signalstats.VLOW",    0,   false,  DBL_MAX, DBL_MAX, ActiveFilter_Video_signalstats, "darkred",    1, "V MIN;darkred;0.2" },
    { Group_V,       Group_VideoMax,       "V AVG",         "lavfi.signalstats.VAVG",    0,   false,  DBL_MAX, DBL_MAX, ActiveFilter_Video_signalstats, "black",      1, "V LOW;darkred;0.8" },
    { Group_V,       Group_VideoMax,       "V HIGH",        "lavfi.signalstats.VHIGH",   0,   false,  DBL_MAX, DBL_MAX, ActiveFilter_Video_signalstats, "pink",       1, "V AVG;darkred;0.8" },
    { Group_V,       Group_VideoMax,       "V MAX",         "lavfi.signalstats.VMAX",    0,   true,   DBL_MAX, DBL_MAX, ActiveFilter_Video_signalstats, "gray",       1, "V HIGH;darkred;0.2" },
    //Diffs
    { Group_VDiff,   Group_Diffs,          "V DIF",         "lavfi.signalstats.VDIF",    5,   false,  DBL_MAX, DBL_MAX, ActiveFilter_Video_signalstats, "darkred",    1, "0;darkred;0.4" },
    { Group_UDiff,   Group_Diffs,          "U DIF",         "lavfi.signalstats.UDIF",    5,   false,  DBL_MAX, DBL_MAX, ActiveFilter_Video_signalstats, "darkblue",   1, "0;darkblue;0.4" },
    { Group_YDiff,   Group_Diffs,          "Y DIF",         "lavfi.signalstats.YDIF",    5,   false,  DBL_MAX, DBL_MAX, ActiveFilter_Video_signalstats, "darkgreen",  1, "0;teal;0.4" },
    //Sat
    { Group_Sat,     Group_VideoMax,       "SAT MIN",       "lavfi.signalstats.SATMIN",  0,   false,  DBL_MAX, DBL_MAX, ActiveFilter_Video_signalstats, "silver",     1, "0;white;1" },
    { Group_Sat,     Group_VideoMax,       "SAT LOW",       "lavfi.signalstats.SATLOW",  0,   false,  DBL_MAX, DBL_MAX, ActiveFilter_Video_signalstats, "lightred",   1, "SAT MIN;red;0.2" },
    { Group_Sat,     Group_VideoMax,       "SAT AVG",       "lavfi.signalstats.SATAVG",  0,   false,  DBL_MAX, DBL_MAX, ActiveFilter_Video_signalstats, "black",      1, "SAT LOW;red;0.4" },
    { Group_Sat,     Group_VideoMax,       "SAT HIGH",      "lavfi.signalstats.SATHIGH", 0,   false,  DBL_MAX, DBL_MAX, ActiveFilter_Video_signalstats, "darkred",    1, "SAT AVG;red;0.6" },
    { Group_Sat,     Group_VideoMax,       "SAT MAX",       "lavfi.signalstats.SATMAX",  0,   true,      88.7,   118.2, ActiveFilter_Video_signalstats, "gray",       1, "SAT HIGH;red;0.8" },
    //Hue
    { Group_Hue,     Group_VideoMax,       "HUE MED",       "lavfi.signalstats.HUEMED",  0,  false,  DBL_MAX, DBL_MAX, ActiveFilter_Video_signalstats, "orange",     1 },
    { Group_Hue,     Group_VideoMax,       "HUE AVG",       "lavfi.signalstats.HUEAVG",  0,  true,   DBL_MAX, DBL_MAX, ActiveFilter_Video_signalstats, "purple",     1 },
    //Others
    { Group_TOUT,    Group_VideoMax,       "TOUT",          "lavfi.signalstats.TOUT",    8,  false,    0.005, DBL_MAX, ActiveFilter_Video_signalstats, "darkgray",   1, "0;whitesmoke;0.6" },
    { Group_VREP,    Group_VideoMax,       "VREP",          "lavfi.signalstats.VREP",    8,  false,  DBL_MAX, DBL_MAX, ActiveFilter_Video_signalstats, "purple",     1, "0;purple;0.6" },
    { Group_BRNG,    Group_VideoMax,       "BRNG",          "lavfi.signalstats.BRNG",    8,  true,      0.05, DBL_MAX, ActiveFilter_Video_signalstats, "black",      1, "0;yellow;0.6" },
    //Bitdepths
    { Group_YUVB,    Group_VideoMax,     "V Active Bits", "lavfi.signalstats.VBITDEPTH", 0,  true,   DBL_MAX, DBL_MAX, ActiveFilter_Video_signalstats, "darkred",    1, "0;darkred;0.4" },
    { Group_YUVB,    Group_VideoMax,     "U Active Bits", "lavfi.signalstats.UBITDEPTH", 0,  false,  DBL_MAX, DBL_MAX, ActiveFilter_Video_signalstats, "darkblue",   1, "0;darkblue;0.4" },
    { Group_YUVB,    Group_VideoMax,     "Y Active Bits", "lavfi.signalstats.YBITDEPTH", 0,  false,  DBL_MAX, DBL_MAX, ActiveFilter_Video_signalstats, "darkgreen",  1, "0;teal;0.4" },
    //Crop
    { Group_CropW,   Group_VideoMax,       "Crop Left",     "lavfi.cropdetect.x1",       0,  false,  DBL_MAX, DBL_MAX, ActiveFilter_Video_cropdetect, nullptr, -1  },
    { Group_CropW,   Group_VideoMax,       "Crop Right",    "lavfi.cropdetect.x2",       0,  false,  DBL_MAX, DBL_MAX, ActiveFilter_Video_cropdetect, nullptr, -1   },
    { Group_CropH,   Group_VideoMax,       "Crop Top",      "lavfi.cropdetect.y1",       0,  false,  DBL_MAX, DBL_MAX, ActiveFilter_Video_cropdetect, nullptr, -1   },
    { Group_CropH,   Group_VideoMax,       "Crop Bottom",   "lavfi.cropdetect.y2",       0,  true,   DBL_MAX, DBL_MAX, ActiveFilter_Video_cropdetect, nullptr, -1   },
    { Group_CropF,   Group_VideoMax,       "Crop Width",    "lavfi.cropdetect.w",        0,  false,  DBL_MAX, DBL_MAX, ActiveFilter_Video_cropdetect, nullptr, -1   },
    { Group_CropF,   Group_VideoMax,       "Crop Height",   "lavfi.cropdetect.h",        0,  true,   DBL_MAX, DBL_MAX, ActiveFilter_Video_cropdetect, nullptr, -1   },
    //MSEf
    { Group_MSE,     Group_VideoMax,       "MSEf V",        "lavfi.psnr.mse.v",          2,  true,   DBL_MAX, DBL_MAX, ActiveFilter_Video_Psnr, "darkred",    1 },
    { Group_MSE,     Group_VideoMax,       "MSEf U",        "lavfi.psnr.mse.u",          2,  false,  DBL_MAX, DBL_MAX, ActiveFilter_Video_Psnr, "darkblue",   1 },
    { Group_MSE,     Group_VideoMax,       "MSEf Y",        "lavfi.psnr.mse.y",          2,  false,     1000, DBL_MAX, ActiveFilter_Video_Psnr, "darkgreen",  1 },
    //PSNRf
    { Group_PSNR,    Group_VideoMax,       "PSNRf V",       "lavfi.psnr.psnr.v",         2,  true,   DBL_MAX, DBL_MAX, ActiveFilter_Video_Psnr, "darkred",    1 },
    { Group_PSNR,    Group_VideoMax,       "PSNRf U",       "lavfi.psnr.psnr.u",         2,  false,  DBL_MAX, DBL_MAX, ActiveFilter_Video_Psnr, "darkblue",   1 },
    { Group_PSNR,    Group_VideoMax,       "PSNRf Y",       "lavfi.psnr.psnr.y",         2,  false,  DBL_MAX, DBL_MAX, ActiveFilter_Video_Psnr, "darkgreen",  1 },
    //SSIMf
    { Group_SSIM,    Group_VideoMax,       "SSIMf All",     "lavfi.ssim.All",            2,  true,   DBL_MAX, DBL_MAX, ActiveFilter_Video_Ssim, "black",      1 },
    { Group_SSIM,    Group_VideoMax,       "SSIMf V",       "lavfi.ssim.V",              2,  false,  DBL_MAX, DBL_MAX, ActiveFilter_Video_Ssim, "darkred",    1 },
    { Group_SSIM,    Group_VideoMax,       "SSIMf U",       "lavfi.ssim.U",              2,  false,  DBL_MAX, DBL_MAX, ActiveFilter_Video_Ssim, "darkblue",   1 },
    { Group_SSIM,    Group_VideoMax,       "SSIMf Y",       "lavfi.ssim.Y",              2,  false,  DBL_MAX, DBL_MAX, ActiveFilter_Video_Ssim, "darkgreen",  1 },
    //IDET.single
    { Group_IDET_S,    Group_VideoMax,     "s.bff",         "lavfi.idet.single.bff",     2,  false,  DBL_MAX, DBL_MAX, ActiveFilter_Video_Idet, "royalblue",     2, "0;royalblue;1" },
    { Group_IDET_S,    Group_VideoMax,     "s.tff",         "lavfi.idet.single.tff",     2,  false,  DBL_MAX, DBL_MAX, ActiveFilter_Video_Idet, "gold",          2, "0;gold;1" },
    { Group_IDET_S,    Group_VideoMax,     "s.prog",    "lavfi.idet.single.progressive", 2,  false,  DBL_MAX, DBL_MAX, ActiveFilter_Video_Idet, "indigo",        2, "0;indigo;1" },
    { Group_IDET_S,    Group_VideoMax,     "s.und",     "lavfi.idet.single.undetermined",2,  false,  DBL_MAX, DBL_MAX, ActiveFilter_Video_Idet, "lightgray",     2, "0;lightgray;1" },
    //IDET.multiple
    { Group_IDET_M,    Group_VideoMax,     "m.bff",         "lavfi.idet.multiple.bff",   2,  false,   DBL_MAX, DBL_MAX, ActiveFilter_Video_Idet, "royalblue",     2, "0;royalblue;1" },
    { Group_IDET_M,    Group_VideoMax,     "m.tff",         "lavfi.idet.multiple.tff",   2,  false,   DBL_MAX, DBL_MAX, ActiveFilter_Video_Idet, "gold",          2, "0;gold;1" },
    { Group_IDET_M,    Group_VideoMax,     "m.prog",  "lavfi.idet.multiple.progressive", 2,  false,   DBL_MAX, DBL_MAX, ActiveFilter_Video_Idet, "indigo",        2, "0;indigo;1" },
    { Group_IDET_M,    Group_VideoMax,     "m.und",   "lavfi.idet.multiple.undetermined",2,  false,   DBL_MAX, DBL_MAX, ActiveFilter_Video_Idet, "lightgray",     2, "0;lightgray;1" },
    //IDET.repeat
    { Group_IDET_R,    Group_VideoMax,     "bottom",        "lavfi.idet.repeated.bottom",2,  false,   DBL_MAX, DBL_MAX, ActiveFilter_Video_Idet, "royalblue",     2, "0;royalblue;1" },
    { Group_IDET_R,    Group_VideoMax,     "top",           "lavfi.idet.repeated.top",   2,  false,   DBL_MAX, DBL_MAX, ActiveFilter_Video_Idet, "gold",          2, "0;gold;1" },
    { Group_IDET_R,    Group_VideoMax,     "neither",      "lavfi.idet.repeated.neither",2,  false,   DBL_MAX, DBL_MAX, ActiveFilter_Video_Idet, "lightgray",     2, "0;lightgray;1" },
    //DEFL
    { Group_DEFL,      Group_VideoMax,     "flicker", "lavfi.deflicker.relative_change", 5,  false,   DBL_MAX, DBL_MAX, ActiveFilter_Video_Deflicker, "olive",     1, "0;olive;0.6" },
    //ENTR
    { Group_ENTR,      Group_VideoMax,   "V ENT", "lavfi.entropy.normalized_entropy.normal.V",5,false,DBL_MAX, DBL_MAX, ActiveFilter_Video_Entropy,     "darkred",    1 },
    { Group_ENTR,      Group_VideoMax,   "U ENT", "lavfi.entropy.normalized_entropy.normal.U",5,false,DBL_MAX, DBL_MAX, ActiveFilter_Video_Entropy,     "darkblue",   1 },
    { Group_ENTR,      Group_VideoMax,   "Y ENT", "lavfi.entropy.normalized_entropy.normal.Y",5,false,DBL_MAX, DBL_MAX, ActiveFilter_Video_Entropy,     "darkgreen",  1 },
    //ENTR-DIFF
    { Group_ENTRD,     Group_VideoMax,"V ENT DIF","lavfi.entropy.normalized_entropy.diff.V",  5,  false,   DBL_MAX, DBL_MAX, ActiveFilter_Video_EntropyDiff, "darkred",    1 },
    { Group_ENTRD,     Group_VideoMax,"U ENT DIF","lavfi.entropy.normalized_entropy.diff.U",  5,  false,   DBL_MAX, DBL_MAX, ActiveFilter_Video_EntropyDiff, "darkblue",   1 },
    { Group_ENTRD,     Group_VideoMax,"Y ENT DIF","lavfi.entropy.normalized_entropy.diff.Y",  5,  false,   DBL_MAX, DBL_MAX, ActiveFilter_Video_EntropyDiff, "darkgreen",  1 },
    // pkt_duration_time & pkt_size
    { Group_pkt_duration_time, Group_VideoMax, "pkt_duration_time", "pkt_duration_time", 5,  false,   DBL_MAX, DBL_MAX, (activefilter) -1,              "black",     1, "0;lawngreen;0.6" },
    { Group_pkt_size,  Group_VideoMax,     "pkt_size",      "pkt_size",                  0,  false,   DBL_MAX, DBL_MAX, (activefilter) -1,              "black",     1, "0;maroon;0.6" },
    // block and blur detections
    { Group_blockdetect, Group_VideoMax,     "blockiness",     "lavfi.block",            6,  false,   DBL_MAX, DBL_MAX, ActiveFilter_Video_blockdetect, "black",     1, "0;plum;0.6" },
    { Group_blurdetect,  Group_VideoMax,     "blurriness",     "lavfi.blur",             6,  false,   DBL_MAX, DBL_MAX, ActiveFilter_Video_blurdetect,  "black",     1, "0;powderblue;0.6" },

    //    const   std::size_t Group1; //Group
    //    const   std::size_t Group2; //Group
    //    const   char*       Name;
    //    const   char*       FFmpeg_Name;
    //    const   int         DigitsAfterComma;
    //    const   bool        NewLine;
    //    const   double      DefaultLimit;
    //    const   double      DefaultLimit2;
};
