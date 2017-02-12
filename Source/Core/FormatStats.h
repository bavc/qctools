/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef FormatStats_H
#define FormatStats_H

#include <stdio.h>
#include <string>
#include <list>

struct AVFormatContext;
class QXmlStreamWriter;

class FormatStats {

public:
    FormatStats(AVFormatContext* context = NULL);

    void writeToXML(QXmlStreamWriter* writer);
    void writeMetadataToXML(QXmlStreamWriter* writer);

    std::string getFilename() const;
    void setFilename(const std::string &value);

    int getNb_streams() const;
    void setNb_streams(int value);

    int getNb_programs() const;
    void setNb_programs(int value);

    std::string getFormat_name() const;
    void setFormat_name(const std::string &value);

    std::string getFormat_long_name() const;
    void setFormat_long_name(const std::string &value);

    std::string getStart_time() const;
    void setStart_time(const std::string &value);

    std::string getDuration() const;
    void setDuration(const std::string &value);

    int getSize() const;
    void setSize(int value);

    int getBit_rate() const;
    void setBit_rate(int value);

    int getProbe_score() const;
    void setProbe_score(int value);

    typedef std::list<std::pair<std::string, std::string>> Metadata;
    const Metadata& getMetadata() const;
    void setMetadata(const Metadata &value);

    bool readFromXML (const char* Data, size_t Size);

private:
    std::string filename;
    int nb_streams;
    int nb_programs;
    std::string format_name;
    std::string format_long_name;
    std::string start_time;
    std::string duration;
    int size;
    int bit_rate;
    int probe_score;

    Metadata metadata;
};

#endif // 
