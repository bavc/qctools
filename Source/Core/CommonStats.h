/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef CommonStats_H
#define CommonStats_H

#include <string>
#include <vector>
#include <stdint.h>
using namespace std;

struct AVFrame;
struct AVStream;
struct per_item;

class CommonStats
{
public:
    // Constructor / Destructor
    CommonStats(const struct per_item* PerItem, int Type, size_t CountOfGroups, size_t CountOfItems, size_t FrameCount=0, double Duration=0, AVStream* stream = NULL);
    virtual ~CommonStats();

    // Data
    double**                    x;                          // Time information, per frame (0=frame number, 1=seconds, 2=minutes, 3=hours)
    double**                    y;                          // Data (Group_xxxMax size)
    double*                     durations;                  // Duration of a frame, per frame
    int64_t*					pkt_pos;                    // Frame offsets
    int64_t*                    pkt_pts;                    // pkt_pts
    int*                        pkt_size;                   // Frame size
    int*                        pix_fmt;                    //
    char*                       pict_type_char;             //
    bool*                       key_frames;                 // Key frame status, per frame
    size_t                      x_Current;                  // Data is filled up to
    size_t                      x_Current_Max;              // Data will be filled up to
    double                      x_Max[4];                   // Maximum x by plot
    double*                     y_Min;                      // Minimum y by plot
    double*                     y_Max;                      // Maximum y by plot
    double                      FirstTimeStamp;             // Time stamp of the first frame

    // Status
    int                         Type_Get();
    double                      State_Get();

    // Stats
    string                      Average_Get(size_t Pos);
    string                      Average_Get(size_t Pos, size_t Pos2);
    string                      Count_Get(size_t Pos);
    string                      Count2_Get(size_t Pos);
    string                      Percent_Get(size_t Pos);

    // External data
    virtual void                StatsFromExternalData(const char* Data, size_t Size) = 0;
            void                StatsFromExternalData_Finish() {Frequency=1; StatsFinish();}
    virtual void                StatsFromFrame(struct AVFrame* Frame, int Width, int Height) = 0;
    virtual void                TimeStampFromFrame(struct AVFrame* Frame, size_t FramePos) = 0;
    virtual void                StatsFinish();
    virtual string              StatsToCSV() {return string();};
    virtual string              StatsToXML(int, int) {return string();};

protected:
    // Status
    bool                        IsComplete;

    // Counts
    double*                     Stats_Totals;
    uint64_t*                   Stats_Counts;
    uint64_t*                   Stats_Counts2;

    // Info
    double                      Frequency;
    int							streamIndex;

    // Memory management
    size_t                      Data_Reserved; // Count of frames reserved in memory;
    void                        Data_Reserve(size_t NewValue); // Increase Data_Reserved

    // Arrays
    int                         Type;
    const struct per_item*      PerItem;
    size_t                      CountOfGroups;
    size_t                      CountOfItems;
};

#endif // Stats_H
