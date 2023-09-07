/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef CommonStats_H
#define CommonStats_H

#include <string>
#include <string.h>
#include <vector>
#include <map>
#include <stdint.h>
#include <algorithm>
#include <cctype>
#include <functional>
#include <Core/Core.h>
#include <QtAVPlayer/qavplayer.h>

struct AVFrame;
class QAVStream;
struct per_item;
class XMLElement;

namespace tinyxml2 {
    class XMLElement;
}

class QAVFrame;
class CommonStats
{
public:
    // Constructor / Destructor
    CommonStats(const struct per_item* PerItem, int Type, size_t CountOfGroups, size_t CountOfItems, size_t FrameCount=0, double Duration=0, QAVStream* stream = NULL);
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
    char**                      comments;                   // Comments per frame (utf-8)

    // Status
    int                         Type_Get();
    double                      State_Get();

    // Stats
    std::string                      Average_Get(size_t Pos);
    std::string                      Average_Get(size_t Pos, size_t Pos2);
    std::string                      Count_Get(size_t Pos);
    std::string                      Count2_Get(size_t Pos);
    std::string                      Percent_Get(size_t Pos);

    static void statsFromExternalData(const char* Data, size_t Size, const std::function<CommonStats*(int, int)>& statsGetter);

    virtual void parseFrame(tinyxml2::XMLElement* frame) = 0;

    // External data
            void                StatsFromExternalData_Finish() {Frequency=1; StatsFinish();}
    virtual void                StatsFromFrame(const QAVFrame& Frame, int Width, int Height) = 0;
    virtual void                TimeStampFromFrame(const QAVFrame& Frame, size_t FramePos) = 0;
    virtual void                StatsFinish();
    virtual std::string              StatsToXML(const activefilters& filters) = 0;

    struct StatsValueInfo {
        size_t index;
        enum Type {
            Int,
            Double,
            String
        } type;
        std::string initialValue;

        static bool endsWith(const std::string& value, const std::string& ending) {
            if(ending.size() > value.size())
                return false;

            return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
        }

        static bool is_number(const std::string& s)
        {
            int dotCount = 0;
            if (s.empty())
               return false;

            for (char c : s )
            {
               if ( !(std::isdigit(c) || c == '.' ) && dotCount > 1 )
               {
                  return false;
               }
               dotCount += (c == '.');
            }

            return true;
        }

        static Type typeFromKey(const std::string& key, const char* value) {
            static const char* IntEndings[] = {
                "MIN",
                "LOW",
                "HIGH",
                "MAX",
                "BITDEPTH",
                ".x1",
                ".x2",
                ".y1",
                ".y2",
                ".w",
                ".h",
                ".x",
                ".y"
            };

            for(auto ending : IntEndings) {
                if(endsWith(key, ending)) {
                    return Int;
                }
            }

            if(key.find(".idet") != std::string::npos) {
                return String;
            }

            // try to deduce type..
            if(is_number(value)) {
                if(strstr(value, ".") != nullptr) {
                    try {
                        std::stod(value);
                        return Double;
                    }
                    catch(const std::exception& ex) {

                    }
                } else {
                    try {
                        std::stoi(value);
                        return Int;
                    }
                    catch(const std::exception& ex) {

                    }
                }
            }

            return String;
        }
    };
    typedef std::string StringStatsKey;

    void initializeAdditionalStats();
    void updateAdditionalStats(StatsValueInfo::Type type, size_t oldSize, size_t size);
    void processAdditionalStats(const char* key, const char* value, bool statsMapInitialized);
    void writeAdditionalStats(std::stringstream& stream, size_t index);

protected:
    size_t lastStatsIndexByValueType[3];
    std::map<StringStatsKey, StatsValueInfo> statsValueInfoByKeys;
    std::map<int, StringStatsKey> statsKeysByIndexByValueType[3];

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

    int**                       additionalIntStats;
    double**                    additionalDoubleStats;
    char***                     additionalStringStats;
};

#endif // Stats_H
