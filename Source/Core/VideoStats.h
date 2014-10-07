/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef VideoStats_H
#define VideoStats_H

#include "Core/Core.h"

#include <string>
#include <vector>
#include <stdint.h>
using namespace std;

struct AVFrame;

class VideoStats
{
public:
    // Constructor / Destructor
    VideoStats(size_t FrameCount=0, double Duration=0, size_t FrameCount_Max=(1<<18), double Frequency=0);
    ~VideoStats();

    // Data
    double**                    x;                          // Time information, per frame (0=frame number, 1=seconds, 2=minutes, 3=hours)
    double**                    y;                          // Data (PlotType_Max size)
    double*                     durations;                  // Duration of a frame, per frame
    bool*                       key_frames;                 // Key frame status, per frame
    size_t                      x_Current;                  // Data is filled up to
    size_t                      x_Current_Max;              // Data will be filled up to
    double                      x_Max[4];                   // Maximum x by plot
    double                      y_Max[PlotType_Max];        // Maximum y by plot

    // Status
    double                      State_Get();

    // Stats
    string                      Average_Get(PlotName Pos);
    string                      Average_Get(PlotName Pos, PlotName Pos2);
    string                      Count_Get(PlotName Pos);
    string                      Count2_Get(PlotName Pos);
    string                      Percent_Get(PlotName Pos);
    
    // External data
    void                        VideoStatsFromExternalData(const string &Data);
    void                        VideoStatsFromFrame(struct AVFrame* Frame, int Width, int Height);
    void                        TimeStampFromFrame(struct AVFrame* Frame, size_t FramePos);
    void                        VideoStatsFinish();
    string                      StatsToCSV();
    string                      StatsToXML(int Width, int Height);

private:
    // Status
    bool                        IsComplete;
    uint64_t                    VideoFirstTimeStamp;                      

    // VideoStats
    double                      Stats_Totals[PlotName_Max];
    uint64_t                    Stats_Counts[PlotName_Max];
    uint64_t                    Stats_Counts2[PlotName_Max];

    // Info
    double                      Frequency;

    // Memory management
    size_t                      Data_Reserved; // Count of frames reserved in memory;
    void                        Data_Reserve(size_t NewValue); // Increase Data_Reserved
};

#endif // Stats_H
