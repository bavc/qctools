/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
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
    "BRNG",
    "HEAD",
};

const int PlotValues_DigitsAfterComma[PlotName_Max]=
{
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    5,
    5,
    5,
    5,
    5,
    8,
    8,
    8,
    8,
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
    0,  //Internal
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
    0,  //Internal
};

size_t StatsFile_CountPerLine[]=
{
    5,
    5,
    5,
    5,
    4,
};

const char* StatsFile_Description[PlotType_Max]=
{
    "YUV refers to a particular a way of encoding color information in analog video where Y channels carry luma,\n"
        "or brightness information, and U and V channels carry information about color, or chrominance.\n"
        "QCTools can analyze the YUV Values of a particular encoded video file in order to provide information about the appearance of the video.\n"
        "These filters examine every pixel in a given channel and records the Maximum, Minimum,\n"
        "and then adds up and divides by the total number of pixels to calculate and provide the Average value.",
    "U and V filters act to detect color abnormalities in video. It can be difficult to derive meaning from U or V values\n"
        "on their own but they provide supplementary information and can be good indicators of artifacts especially\n"
        "when occurring in tandem with similar Y Value readings. Black and white videos should present flat-lines (or no data) for UV channels.\n"
        "Activity in UV Channels for black and white video content, however, would certainly be an indication of chrominance noise.\n"
        "Alternatively, a color video which shows flat-lines for these channels would be an indicator of a color drop-out scenario.",
    "U and V filters act to detect color abnormalities in video. It can be difficult to derive meaning from U or V values\n"
        "on their own but they provide supplementary information and can be good indicators of artifacts especially\n"
        "when occurring in tandem with similar Y Value readings. Black and white videos should present flat-lines (or no data) for UV channels.\n"
        "Activity in UV Channels for black and white video content, however, would certainly be an indication of chrominance noise.\n"
        "Alternatively, a color video which shows flat-lines for these channels would be an indicator of a color drop-out scenario.",
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
    "The YDIF1 and YDIF2 filters help detect artifacts caused by video deck head clogs, with each filter providing a reading per head.\n"
        "The filter is built on the assumption that one head is functioning (and as such, presents as a stable graph line),\n"
        "and one is corrupt (presenting as a variable line with spikes and troughs).\n"
        "A user, in reading the two results against one another, would see the areas where the two readings diverge as red portions on the graph.\n"
        "See graph below: between 17-19s the YDIF1 and YDIF2 readings vary dramatically, presenting as red portions in the graph.",
    "The YDIF1 and YDIF2 filters help detect artifacts caused by video deck head clogs, with each filter providing a reading per head.\n"
        "The filter is built on the assumption that one head is functioning (and as such, presents as a stable graph line),\n"
        "and one is corrupt (presenting as a variable line with spikes and troughs).\n"
        "A user, in reading the two results against one another, would see the areas where the two readings diverge as red portions on the graph.\n"
        "See graph below: between 17-19s the YDIF1 and YDIF2 readings vary dramatically, presenting as red portions in the graph.",
    "The YDIF1 and YDIF2 filters help detect artifacts caused by video deck head clogs, with each filter providing a reading per head.\n"
        "The filter is built on the assumption that one head is functioning (and as such, presents as a stable graph line),\n"
        "and one is corrupt (presenting as a variable line with spikes and troughs).\n"
        "A user, in reading the two results against one another, would see the areas where the two readings diverge as red portions on the graph.\n"
        "See graph below: between 17-19s the YDIF1 and YDIF2 readings vary dramatically, presenting as red portions in the graph.",
    "The YDIF1 and YDIF2 filters help detect artifacts caused by video deck head clogs, with each filter providing a reading per head.\n"
        "The filter is built on the assumption that one head is functioning (and as such, presents as a stable graph line),\n"
        "and one is corrupt (presenting as a variable line with spikes and troughs).\n"
        "A user, in reading the two results against one another, would see the areas where the two readings diverge as red portions on the graph.\n"
        "See graph below: between 17-19s the YDIF1 and YDIF2 readings vary dramatically, presenting as red portions in the graph.",
    "This filter was created to detect white speckle noise in analog VHS and 8mm video.\n"
        "It works by analyzing the current pixel against the two above and below and calculates an average value.\n"
        "In cases where the filter detects a pixel value which is dramatically outside of this established average,\n"
        "the graph will show small spikes, or blips, which correspond to white speckling in the video.",
    "Head Switching (Filter Description TBD)",
    "Vertical Line Repetitions, or the VREP filter, is useful in analyzing U-Matic tapes and detecting artifacts generated\n"
        "in the course of the digitization process. Specifically, VREP detects the repetition of lines in a video.\n"
        "The filter works by taking a given video line and comparing it against a video line that occurs 4 pixels earlier.\n"
        "If the difference in the two is less than 512, the filter reads them as being close enough to be deemed repetitious.\n"
        "Note that the VREP filter is still under development.",
    "The BRNG filter is one that identifies the number of pixels which fall outside the standard video broadcast range of 16-235 pixels.\n"
        "Normal, noise-free video would not trigger this filer, but noise ocurring outside of these parameters would read as spikes in the graph.\n"
        "Typically anything with a value over 0.01 will read as an artifact. While the BRNG filter is good at detecting the general presence of noise,\n"
        "it can be a bit non-specific in its identification of the causes.",
    "",  //Internal
};

