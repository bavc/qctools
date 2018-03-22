/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef CommonStreamStats_H
#define CommonStreamStats_H

#include <string>
#include <list>

class QXmlStreamWriter;

namespace tinyxml2 {
    class XMLElement;
}

struct AVFormatContext;
struct AVStream;

class CommonStreamStats
{
public:
    CommonStreamStats(tinyxml2::XMLElement* streamElement);
    CommonStreamStats(AVStream* stream);
    virtual ~CommonStreamStats();

    // stream info
    int get_StreamIndex() const;

    std::string getCodec_Name() const;
    std::string getCodec_Long_Name() const;
    std::string getCodec_Type() const;
    std::string getCodec_Time_Base() const;
    std::string getCodec_TagString() const;

    int getCodec_Tag() const;

    std::string getR_frame_rate() const;
    void setR_frame_rate(const std::string &value);

    std::string getAvg_frame_rate() const;
    void setAvg_frame_rate(const std::string &value);

    std::string getTime_base() const;
    void setTime_base(const std::string &value);

    std::string getStart_pts() const;
    void setStart_pts(const std::string &value);

    std::string getStart_time() const;
    void setStart_time(const std::string &value);

    int getDisposition() const;
    void setDisposition(int value);

    int getBitsPerRawSample() const;
    void setBitsPerRawSample(int value);

    typedef std::list<std::pair<std::string, std::string>> Metadata;
    const Metadata& getMetadata() const;
    void setMetadata(const Metadata &value);

    void writeToXML(QXmlStreamWriter* writer);

    virtual void writeStreamInfoToXML(QXmlStreamWriter* writer);
    virtual void writeDispositionInfoToXML(QXmlStreamWriter* writer);
    virtual void writeMetadataToXML(QXmlStreamWriter* writer);

protected:
    // stream info
    int	stream_index;
    std::string codec_name;
    std::string codec_long_name;
    std::string codec_type;
    std::string codec_time_base;
    int codec_tag;

    std::string r_frame_rate;
    std::string avg_frame_rate;
    std::string time_base;
    std::string start_pts;
    std::string start_time;

    int disposition;
    int bits_per_raw_sample;
    Metadata metadata;
};

#endif // CommonStreamStats_H
