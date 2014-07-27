/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include <Core/Core.h>

const struct per_plot_group PerPlotGroup [PlotType_Max]=
{
    //Y
    {
        PlotName_YMIN,      5,    0,  255,  3,  "Y",  true,
        "YUV refers to a particular a way of encoding color information in analog video where Y channels carry luma,\n"
            "or brightness information, and U and V channels carry information about color, or chrominance.\n"
            "QCTools can analyze the YUV Values of a particular encoded video file in order to provide information about the appearance of the video.\n"
            "These filters examine every pixel in a given channel and records the Maximum, Minimum,\n"
            "and then adds up and divides by the total number of pixels to calculate and provide the Average value.",
    },
    //U
    {
        PlotName_UMIN,      5,    0,  255,  3,  "U",  true,
        "U and V filters act to detect color abnormalities in video. It can be difficult to derive meaning from U or V values\n"
            "on their own but they provide supplementary information and can be good indicators of artifacts especially\n"
            "when occurring in tandem with similar Y Value readings. Black and white videos should present flat-lines (or no data) for UV channels.\n"
            "Activity in UV Channels for black and white video content, however, would certainly be an indication of chrominance noise.\n"
            "Alternatively, a color video which shows flat-lines for these channels would be an indicator of a color drop-out scenario.",
    },
    //V
    {
        PlotName_VMIN,      5,    0,  255,  3,  "V",  true,
        "U and V filters act to detect color abnormalities in video. It can be difficult to derive meaning from U or V values\n"
            "on their own but they provide supplementary information and can be good indicators of artifacts especially\n"
            "when occurring in tandem with similar Y Value readings. Black and white videos should present flat-lines (or no data) for UV channels.\n"
            "Activity in UV Channels for black and white video content, however, would certainly be an indication of chrominance noise.\n"
            "Alternatively, a color video which shows flat-lines for these channels would be an indicator of a color drop-out scenario.",
    },
    //YDiff
    {
        PlotName_YDIF,      1,    0,  255,  4,  "YDiff", false,
        "This QCTools filter selects two successive frames of video and subtracts the values of one from the other in order to find the change,\n"
            "or difference, between the two frames (measured in pixels). This information is meaningful in that it indicates the rapidity\n"
            "with which a video picture is changing from one frame to the next. Aside from scene-change scenarios,\n"
            "a video picture should not undergo dramatic changes in these values unless an artifact is present.\n"
            "A scene-change would present as a short but dramatic spike in the graph.\n"
            "Other YUV Difference spikes may present in cases where picture problems are visible.\n"
            "Often, head problems with corrupted frames will result in large YUV Difference values/ graph spikes.\n"
            "The YDIF1 and YDIF2 filters help detect artifacts caused by video deck head clogs, with each filter providing a reading per head.\n"
            "The filter is built on the assumption that one head is functioning (and as such, presents as a stable graph line),\n"
            "and one is corrupt (presenting as a variable line with spikes and troughs).\n"
            "A user, in reading the two results against one another, would see the areas where the two readings diverge as red portions on the graph.\n"
            "See graph below: between 17-19s the YDIF1 and YDIF2 readings vary dramatically, presenting as red portions in the graph.",
    },
    //YDiffX
    //{
    //    PlotName_YDIF1,     2,    0,  255,  4,  "YDiffX", false,
    //    "The YDIF1 and YDIF2 filters help detect artifacts caused by video deck head clogs, with each filter providing a reading per head.\n"
    //        "The filter is built on the assumption that one head is functioning (and as such, presents as a stable graph line),\n"
    //        "and one is corrupt (presenting as a variable line with spikes and troughs).\n"
    //        "A user, in reading the two results against one another, would see the areas where the two readings diverge as red portions on the graph.\n"
    //        "See graph below: between 17-19s the YDIF1 and YDIF2 readings vary dramatically, presenting as red portions in the graph.",
    //},
    //UDiff
    {
        PlotName_UDIF,      1,    0,  255,  4,  "UDiff", false,
        "The YDIF1 and YDIF2 filters help detect artifacts caused by video deck head clogs, with each filter providing a reading per head.\n"
            "The filter is built on the assumption that one head is functioning (and as such, presents as a stable graph line),\n"
            "and one is corrupt (presenting as a variable line with spikes and troughs).\n"
            "A user, in reading the two results against one another, would see the areas where the two readings diverge as red portions on the graph.\n"
            "See graph below: between 17-19s the YDIF1 and YDIF2 readings vary dramatically, presenting as red portions in the graph.",
    },
    //VDiff
    {
        PlotName_VDIF,      1,    0,  255,  4,  "VDiff", false,
        "The YDIF1 and YDIF2 filters help detect artifacts caused by video deck head clogs, with each filter providing a reading per head.\n"
            "The filter is built on the assumption that one head is functioning (and as such, presents as a stable graph line),\n"
            "and one is corrupt (presenting as a variable line with spikes and troughs).\n"
            "A user, in reading the two results against one another, would see the areas where the two readings diverge as red portions on the graph.\n"
            "See graph below: between 17-19s the YDIF1 and YDIF2 readings vary dramatically, presenting as red portions in the graph.",
    },
    //Diffs
    {
        PlotName_YDIF,      3,    0,  255,  4,  "Diffs",  true,
        "The YDIF1 and YDIF2 filters help detect artifacts caused by video deck head clogs, with each filter providing a reading per head.\n"
            "The filter is built on the assumption that one head is functioning (and as such, presents as a stable graph line),\n"
            "and one is corrupt (presenting as a variable line with spikes and troughs).\n"
            "A user, in reading the two results against one another, would see the areas where the two readings diverge as red portions on the graph.\n"
            "See graph below: between 17-19s the YDIF1 and YDIF2 readings vary dramatically, presenting as red portions in the graph.",
    },
    //Sat
    {
        PlotName_SATMIN,    5,    0,  180,  4,  "Sat", false,
        "This filter does the equivalent of plotting all pixels in a vectorscope and measuring the distance from that plotted point to the center of the vectorscope.\n"
        "The SATAVG will provide an overall idea of how much color saturation exists within a given frame.\n"
        "The plot for SATMAX will highlight a number of errors that cause color levels that exceed broadcast range or are mathematically illegal.\n"
        "Values form 0 to 88.7 are considered within the 75% chrominance broadcast range, while values from 88.7 to 118.2 fall in between the 75% and 100% chrominance broadcast ranges.\n"
        "Values from 118.2 to the maximum value of 181.02 represent illegal YUV color data that can not be converted back to RGB without producing negatives or overflows,\n"
        "such values a indicative errors such as time base corrector errors or Digital Betacam damage.",
    },
    //Hue
    {
        PlotName_HUEMED,    2,    0,  360,  4,  "Hue", false,
        "Hue (Graph Description TBD)",
    },
    //TOUT
    {
        PlotName_TOUT,      1,    0,  0.9,  4,  "TOUT", false,
        "This filter was created to detect white speckle noise in analog VHS and 8mm video.\n"
            "It works by analyzing the current pixel against the two above and below and calculates an average value.\n"
            "In cases where the filter detects a pixel value which is dramatically outside of this established average,\n"
            "the graph will show small spikes, or blips, which correspond to white speckling in the video.",
    },
    //HEAD
    //{
    //    PlotName_HEAD,      1,    0,    0,  4,  "HEAD", false,
    //    "Head Switching (Graph Description TBD)",
    //},
    //VREP
    {
        PlotName_VREP,      1,    0,    1,  4,  "VREP", false,
        "Vertical Line Repetitions, or the VREP filter, is useful in analyzing U-Matic tapes and detecting artifacts generated\n"
            "in the course of the digitization process. Specifically, VREP detects the repetition of lines in a video.\n"
            "The filter works by taking a given video line and comparing it against a video line that occurs 4 pixels earlier.\n"
            "If the difference in the two is less than 512, the filter reads them as being close enough to be deemed repetitious.\n"
            "Note that the VREP filter is still under development.",
    },
    //BRNG
    {
        PlotName_BRNG,      1,    0,    1,  4,  "BRNG", false,
        "The BRNG filter is one that identifies the number of pixels which fall outside the standard video broadcast range of 16-235 pixels.\n"
            "Normal, noise-free video would not trigger this filer, but noise ocurring outside of these parameters would read as spikes in the graph.\n"
            "Typically anything with a value over 0.01 will read as an artifact. While the BRNG filter is good at detecting the general presence of noise,\n"
            "it can be a bit non-specific in its identification of the causes.",
    },
    //CropW
    {
        PlotName_Crop_x1,   2,    0,    0,  4,  "CropW", false,
        "CropW (Graph Description TBD)",
    },
    //CropH
    {
        PlotName_Crop_y1,   2,    0,    0,  4,  "CropH", false,
        "CropH (Graph Description TBD)",
    },
    //CropF
    {
        PlotName_Crop_w,   2,    0,    0,  4,  "CropF", false,
        "CropH (Graph Description TBD)",
    },
    //MSEf
    {
        PlotName_MSE_v,     3,    0,    0,  4,  "MSEf", false,
        "Similar to PSNRf but reports on the Mean Square Error between field 1 and field 2. Higher values may be indicative of differences between the images of field 1 and field 2.",
    },
    //PSNRf
    {
        PlotName_PSNR_v,    3,    0,    0,  4,  "PSNRf", false,
        "Plot the Peak Signal to Noise ratio between the video in field 1 (odd lines) versus the video in field 2 (even lines).\n"
        "Lower values indicate that field 1 and field 2 are becoming more different as would happen during a playback error such as a head clog.\n"
        "See http://ffmpeg.org/ffmpeg-filters.html#psnr for more information.",
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
    { PlotType_Y,       PlotType_Max,       "YMIN",       "lavfi.values.YMIN",    "lavfi.signalstats.YMIN",    0,   false,  },
    { PlotType_Y,       PlotType_Max,       "YLOW",       "lavfi.values.YLOW",    "lavfi.signalstats.YLOW",    0,   false,  },
    { PlotType_Y,       PlotType_Max,       "YAVG",       "lavfi.values.YAVG",    "lavfi.signalstats.YAVG",    0,   false,  },
    { PlotType_Y,       PlotType_Max,       "YHIGH",      "lavfi.values.YHIGH",   "lavfi.signalstats.YHIGH",   0,   false,  },
    { PlotType_Y,       PlotType_Max,       "YMAX",       "lavfi.values.YMAX",    "lavfi.signalstats.YMAX",    0,   true,   },
    //U
    { PlotType_U,       PlotType_Max,       "UMIN",       "lavfi.values.UMIN",    "lavfi.signalstats.UMIN",    0,   false,  },
    { PlotType_U,       PlotType_Max,       "ULOW",       "lavfi.values.ULOW",    "lavfi.signalstats.ULOW",    0,   false,  },
    { PlotType_U,       PlotType_Max,       "UAVG",       "lavfi.values.UAVG",    "lavfi.signalstats.UAVG",    0,   false,  },
    { PlotType_U,       PlotType_Max,       "UHIGH",      "lavfi.values.UHIGH",   "lavfi.signalstats.UHIGH",   0,   false,  },
    { PlotType_U,       PlotType_Max,       "UMAX",       "lavfi.values.UMAX",    "lavfi.signalstats.UMAX",    0,   true,   },
    //V
    { PlotType_V,       PlotType_Max,       "VMIN",       "lavfi.values.VMIN",    "lavfi.signalstats.VMIN",    0,  false,  },
    { PlotType_V,       PlotType_Max,       "VLOW",       "lavfi.values.VLOW",    "lavfi.signalstats.VLOW",    0,  false,  },
    { PlotType_V,       PlotType_Max,       "VAVG",       "lavfi.values.VAVG",    "lavfi.signalstats.VAVG",    0,  false,  },
    { PlotType_V,       PlotType_Max,       "VHIGH",      "lavfi.values.VHIGH",   "lavfi.signalstats.VHIGH",   0,  false,  },
    { PlotType_V,       PlotType_Max,       "VMAX",       "lavfi.values.VMAX",    "lavfi.signalstats.VMAX",    0,  true,   },
    //Diffs
    { PlotType_YDiff,   PlotType_Diffs,     "YDIF",       "lavfi.values.YDIF",    "lavfi.signalstats.YDIF",    5,  false,  },
    { PlotType_UDiff,   PlotType_Diffs,     "UDIF",       "lavfi.values.UDIF",    "lavfi.signalstats.UDIF",    5,  false,  },
    { PlotType_VDiff,   PlotType_Diffs,     "VDIF",       "lavfi.values.VDIF",    "lavfi.signalstats.VDIF",    5,  false,  },
    //{ PlotType_YDiffX,  PlotType_Max,       "YDIF1",      "lavfi.values.YDIF1",   "lavfi.signalstats.YDIF1",   5,  false,  },
    //{ PlotType_YDiffX,  PlotType_Max,       "YDIF2",      "lavfi.values.YDIF2",   "lavfi.signalstats.YDIF2",   5,  true,   },
    //Sat
    { PlotType_Sat,     PlotType_Max,       "SATMIN",     "lavfi.values.SATMIN",  "lavfi.signalstats.SATMIN",  0,  false,  },
    { PlotType_Sat,     PlotType_Max,       "SATLOW",     "lavfi.values.SATLOW",  "lavfi.signalstats.SATLOW",  0,  false,  },
    { PlotType_Sat,     PlotType_Max,       "SATAVG",     "lavfi.values.SATAVG",  "lavfi.signalstats.SATAVG",  0,  false,  },
    { PlotType_Sat,     PlotType_Max,       "SATHIGH",    "lavfi.values.SATHIGH", "lavfi.signalstats.SATHIGH", 0,  false,  },
    { PlotType_Sat,     PlotType_Max,       "SATMAX",     "lavfi.values.SATMAX",  "lavfi.signalstats.SATMAX",  0,  true,   },
    //Hue
    //{ PlotType_Hue,     PlotType_Max,       "HUEMOD",     "lavfi.values.HUEMOD",  "lavfi.signalstats.HUEMOD",  0,  false,  },
    { PlotType_Hue,     PlotType_Max,       "HUEMED",     "lavfi.values.HUEMED",  "lavfi.signalstats.HUEMED",  0,  false,  },
    { PlotType_Hue,     PlotType_Max,       "HUEAVG",     "lavfi.values.HUEAVG",  "lavfi.signalstats.HUEAVG",  0,  true,   },
    //Others
    { PlotType_TOUT,    PlotType_Max,       "TOUT",       "lavfi.values.TOUT",    "lavfi.signalstatsTOUT.",    8,  false,  },
    //{ PlotType_HEAD,    PlotType_Max,       "HEAD",       "lavfi.values.HEAD",    "lavfi.signalstats.HEAD",    8,  false,  },
    { PlotType_VREP,    PlotType_Max,       "VREP",       "lavfi.values.VREP",    "lavfi.signalstats.VREP",    8,  false,  },
    { PlotType_BRNG,    PlotType_Max,       "BRNG",       "lavfi.values.RANG",    "lavfi.signalstats.BRNG",    8,  true,   },
    //Crop
    { PlotType_CropW,   PlotType_Max,       "Crop x1",    "",                     "lavfi.cropdetect.x1", 0,  false,  },
    { PlotType_CropW,   PlotType_Max,       "Crop x2",    "",                     "lavfi.cropdetect.x2", 0,  false,  },
    { PlotType_CropH,   PlotType_Max,       "Crop y1",    "",                     "lavfi.cropdetect.y1", 0,  false,  },
    { PlotType_CropH,   PlotType_Max,       "Crop y2",    "",                     "lavfi.cropdetect.y1", 0,  true,   },
    { PlotType_CropF,   PlotType_Max,       "Crop w",     "",                     "lavfi.cropdetect.w",  0,  false,  },
    { PlotType_CropF,   PlotType_Max,       "Crop h",     "",                     "lavfi.cropdetect.h",  0,  true,   },
    //MSEf
    { PlotType_MSE,     PlotType_Max,       "MSEf V",     "lavfi.psnr.mse.v",     "lavfi.psnr.mse.v",          2,  true,   },
    { PlotType_MSE,     PlotType_Max,       "MSEf U",     "lavfi.psnr.mse.u",     "lavfi.psnr.mse.u",          2,  false,  },
    { PlotType_MSE,     PlotType_Max,       "MSEf Y",     "lavfi.psnr.mse.y",     "lavfi.psnr.mse.y",          2,  false,  },
    //PSNRf
    { PlotType_PSNR,    PlotType_Max,       "PSNRf V",    "lavfi.psnr.psnr.v",    "lavfi.psnr.psnr.v",         2,  true,   },
    { PlotType_PSNR,    PlotType_Max,       "PSNRf U",    "lavfi.psnr.psnr.u",    "lavfi.psnr.psnr.u",         2,  false,  },
    { PlotType_PSNR,    PlotType_Max,       "PSNRf Y",    "lavfi.psnr.psnr.y",    "lavfi.psnr.psnr.y",         2,  false,  },
};
