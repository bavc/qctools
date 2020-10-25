/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef VideoStats_H
#define VideoStats_H

#include "Core/Core.h"
#include "Core/CommonStats.h"

#include <string>
#include <vector>
#include <stdint.h>
using namespace std;

struct AVFrame;
struct AVStream;
struct AVFormatContext;

namespace tinyxml2 {
    class XMLElement;
}

class VideoStats : public CommonStats
{
public:
    // Constructor / Destructor
    VideoStats(size_t FrameCount=0, double Duration=0, AVStream* stream = NULL);
    VideoStats(int streamIndex);
    ~VideoStats();

    // External data
    virtual void parseFrame(tinyxml2::XMLElement* frame);
    void                        StatsFromFrame(struct AVFrame* Frame, int Width, int Height);
    void                        TimeStampFromFrame(struct AVFrame* Frame, size_t FramePos);
    string                      StatsToXML(const activefilters& filters);

    int getWidth() const;
    void setWidth(int getWidth);

    int getHeight() const;
    void setHeight(int getHeight);

private:

    int width;
    int height;
};

#endif // Stats_H
