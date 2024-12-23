/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef VideoStreamStats_H
#define VideoStreamStats_H

#include "Core/CommonStreamStats.h"

namespace tinyxml2 {
    class XMLElement;
}
class QAVStream;
class VideoStreamStats : public CommonStreamStats
{
public:
    VideoStreamStats(tinyxml2::XMLElement* streamElement);
    VideoStreamStats(QAVStream* stream);

    virtual void writeStreamInfoToXML(QXmlStreamWriter* writer);

    std::string getWidth() const;
    void setWidth(const std::string &value);

    std::string getHeight() const;
    void setHeight(const std::string &value);

    std::string getCoded_width() const;
    void setCoded_width(const std::string &value);

    std::string getCoded_height() const;
    void setCoded_height(const std::string &value);

    std::string getHas_b_frames() const;
    void setHas_b_frames(const std::string &value);

    std::string getSample_aspect_ratio() const;
    void setSample_aspect_ratio(const std::string &value);

    std::string getDisplay_aspect_ratio() const;
    void setDisplay_aspect_ratio(const std::string &value);

    std::string getPix_fmt() const;
    void setPix_fmt(const std::string &value);

    std::string getLevel() const;
    void setLevel(const std::string &value);

    std::string getField_order() const;
    void setField_order(const std::string &value);

    std::string getRefs() const;
    void setRefs(const std::string &value);

private:

    std::string width;
    std::string height;
    std::string coded_width;
    std::string coded_height;
    std::string has_b_frames;
    std::string sample_aspect_ratio;
    std::string display_aspect_ratio;
    std::string pix_fmt;
    std::string level;
    std::string field_order;
    std::string refs;
};

#endif // VideoStreamStats_H
