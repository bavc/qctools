/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef AudioStreamStats_H
#define AudioStreamStats_H

#include "Core/CommonStreamStats.h"

#include <string>
#include <vector>
#include <stdint.h>

namespace tinyxml2 {
    class XMLElement;
}

class AudioStreamStats : public CommonStreamStats
{
public:
    AudioStreamStats(tinyxml2::XMLElement* streamElement);
    AudioStreamStats(AVStream* stream, AVFormatContext *context);

    virtual void writeStreamInfoToXML(QXmlStreamWriter* writer);

    int getSample_fmt() const;

    std::string getSample_fmt_string() const;
    void setSample_fmt_string(const std::string &value);

    int getSample_rate() const;
    void setSample_rate(const int &value);

    int getChannels() const;
    void setChannels(int value);

    std::string getChannel_layout() const;
    void setChannel_layout(const std::string &value);

    int getBits_per_sample() const;
    void setBits_per_sample(int value);

private:
    std::string sample_fmt_string;
    int sample_fmt;
    int sample_rate;
    int channels;
    std::string channel_layout;
    int bits_per_sample;
};

#endif // AudioStreamStats_H
