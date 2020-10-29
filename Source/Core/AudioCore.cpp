/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include <Core/AudioCore.h>
#include <cfloat>

struct per_group AudioPerGroup [Group_AudioMax]=
{
    //R128
    {
        Item_R128M,             1,  "-70",    "0",  3,  "EBU R128",  false,
        "R 128 refers to a European Broadcasting Union (EBU) specification\n"
        "document governing several loudness parameters, including momentary,\n"
        "integrated, and short-term loudness. QCTools specifically examines momentary\n"
        "loudness, or sudden changes in volume over brief intervals of time (up to 400ms).\n"
        "This can be helpful in identifying areas where volume may exceed upper loudness\n"
        "tolerance levels as perceived by an audience.\n",
        ActiveFilter_Audio_EbuR128,
    },
    //aphasemeter
    {
        Item_aphasemeter,       1,  "-1",    "1",  3,  "Audio Phase",  false,
        "The audio phase value represents the mean phase of current audio frame. Value is\n"
        "in range [-1, 1]. The -1 means left and right channels are completely out of\n"
        "phase and 1 means channels are in phase.",
        ActiveFilter_Audio_aphasemeter,
    },
    //astats dc offset
    {
        Item_DC_offset,             1,   nullptr,    nullptr,  6,  "Audio DC Offset",  true,
        "For selected audio tracks this graph plots the DC offset (mean\n"
        "amplitude displacement from zero), minimal sample level, and \n"
        "maximum sample level. Note that this value is plotted per audio\n"
        "frame and not per audio sample.",
        ActiveFilter_Audio_astats,
    },
    //astats levels
    {
        Item_Min_level,             2,   "-1",    "1",  6,  "Audio Levels (Overall)",  true,
        "For selected audio tracks this graph plots the minimal sample level,\n"
        "and maximum sample level. Note that this value is plotted per audio\n"
        "frame and not per audio sample.",
        ActiveFilter_Audio_astats,
    },
    //astats levels
    {
        Item_Min_level1,            4,   "-1",    "1",  6,  "Audio Levels (Ch 1 vs 2)",  true,
        "For selected audio tracks this graph plots the minimal sample level,\n"
        "and maximum sample level of the first two channels separately. Note\n"
        "that this value is plotted per audio frame and not per audio sample.",
        ActiveFilter_Audio_astats,
    },
    //zero crossings
    {
        Item_Zero_Crossing1,         2,   "0",  "0.5",  6,  "Audio Zero Crossing Rate",  true,
        "For the first two channels of an audio track this graph plots the rate\n"
        "of zero crossings against the number of audio samples.",
        ActiveFilter_Audio_astats,
    },
    //astats diff
    {
        Item_Min_difference,        3,    "0",    "1",  3,  "Audio Sample-to-Sample Differences", false,
        "For selected audio tracks this graph plots the minimal difference\n"
        "between two consecutive samples, maximal difference between two\n"
        "consecutive samples. and the mean difference between two consecutive\n"
        "samples (the average of each difference between two consecutive samples)."
        "A sharp spike in the maximum difference between consecuritve samples\n"
        "may be indictative of an interstitial error. Note that this value\n"
        "is plotted per audio frame and not per audio sample.",
        ActiveFilter_Audio_astats,
    },
    //astats rms
    {
        Item_Peak_level,            3,  "-70",    "0",  3,  "Audio RMS",      false,
        "For selected audio tracks this graph plots the Standard peak and RMS\n"
        "level measured in dBFS and the Peak and trough values for RMS level\n"
        "measured over a short window.  Note that this value is plotted per\n"
        "audio frame and not per audio sample.",
        ActiveFilter_Audio_astats,
    },
    //LRA
    //{
    //    Item_LRAL,       3,    0,    0,  3,  "LRA",  true,
    //    "(TODO)\n",
    //},
};

const struct per_item AudioPerItem [Item_AudioMax]=
{
    //Y
    { Group_R128,         Group_AudioMax, "EBU R128 Momentary\nloudness","lavfi.r128.M",                        3,   false,  DBL_MAX, DBL_MAX, ActiveFilter_Audio_EbuR128 },
    { Group_aphasemeter,  Group_AudioMax, "Audio Phase",                "lavfi.aphasemeter.phase",              3,   false,  DBL_MAX, DBL_MAX, ActiveFilter_Audio_aphasemeter },
    { Group_astats_dc,    Group_AudioMax, "Audio DC Offset",            "lavfi.astats.Overall.DC_offset",       6,   false,  DBL_MAX, DBL_MAX, ActiveFilter_Audio_astats },
    { Group_astats_levels,Group_AudioMax, "Audio Min Level",            "lavfi.astats.Overall.Min_level",       6,   false,  DBL_MAX, DBL_MAX, ActiveFilter_Audio_astats },
    { Group_astats_levels,Group_AudioMax, "Audio Max Level",            "lavfi.astats.Overall.Max_level",       6,   false,  DBL_MAX, DBL_MAX, ActiveFilter_Audio_astats },
    { Group_astats_lvlchs,Group_AudioMax, "Audio Min Level (Ch2)",      "lavfi.astats.2.Min_level",             6,   false,  DBL_MAX, DBL_MAX, ActiveFilter_Audio_astats },
    { Group_astats_lvlchs,Group_AudioMax, "Audio Max Level (Ch2)",      "lavfi.astats.2.Max_level",             6,   false,  DBL_MAX, DBL_MAX, ActiveFilter_Audio_astats },
    { Group_astats_lvlchs,Group_AudioMax, "Audio Min Level (Ch1)",      "lavfi.astats.1.Min_level",             6,   false,  DBL_MAX, DBL_MAX, ActiveFilter_Audio_astats },
    { Group_astats_lvlchs,Group_AudioMax, "Audio Max Level (Ch1)",      "lavfi.astats.1.Max_level",             6,   false,  DBL_MAX, DBL_MAX, ActiveFilter_Audio_astats },
    { Group_astats_zeros, Group_AudioMax, "Audio Zero\nCrossing (ch 2)", "lavfi.astats.2.Zero_crossings_rate",  6,   false,  DBL_MAX, DBL_MAX, ActiveFilter_Audio_astats },
    { Group_astats_zeros, Group_AudioMax, "Audio Zero\nCrossing (ch 1)", "lavfi.astats.1.Zero_crossings_rate",  6,   false,  DBL_MAX, DBL_MAX, ActiveFilter_Audio_astats },
    { Group_adif,         Group_AudioMax, "Difference Min",             "lavfi.astats.Overall.Min_difference",  6,   false,  DBL_MAX, DBL_MAX, ActiveFilter_Audio_astats },
    { Group_adif,         Group_AudioMax, "Difference Max",             "lavfi.astats.Overall.Max_difference",  6,   false,  DBL_MAX, DBL_MAX, ActiveFilter_Audio_astats },
    { Group_adif,         Group_AudioMax, "Difference Mean",            "lavfi.astats.Overall.Mean_difference", 6,   false,  DBL_MAX, DBL_MAX, ActiveFilter_Audio_astats },
    { Group_astats_RMS,   Group_AudioMax, "Peak Level",                 "lavfi.astats.Overall.Peak_level",      6,   false,  DBL_MAX, DBL_MAX, ActiveFilter_Audio_astats },
    { Group_astats_RMS,   Group_AudioMax, "RMS Peak",                   "lavfi.astats.Overall.RMS_peak",        6,   false,  DBL_MAX, DBL_MAX, ActiveFilter_Audio_astats },
    { Group_astats_RMS,   Group_AudioMax, "RMS Trough",                 "lavfi.astats.Overall.RMS_trough",      6,   false,  DBL_MAX, DBL_MAX, ActiveFilter_Audio_astats },
    //{ Group_R128,   Group_AudioMax,       "R128.S",         "lavfi.r128.S",             0,   false,  DBL_MAX, DBL_MAX },
    //{ Group_R128,   Group_AudioMax,       "R128.I",         "lavfi.r128.I",             0,   true,   DBL_MAX, DBL_MAX },
    //U
    //{ Group_LRA,    Group_AudioMax,       "LRAL",           "lavfi.r128.LRA.low",       0,   false,  DBL_MAX, DBL_MAX },
    //{ Group_LRA,    Group_AudioMax,       "LRA",            "lavfi.r128.LRA",           0,   false,  DBL_MAX, DBL_MAX },
    //{ Group_LRA,    Group_AudioMax,       "LRAH",           "lavfi.r128.LRA.high",      0,   true,   DBL_MAX, DBL_MAX },
};
