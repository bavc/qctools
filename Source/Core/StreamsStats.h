/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef StreamsStats_H
#define StreamsStats_H

#include <stdio.h>
#include <string>
#include <list>
#include <memory>

#include "Core/CommonStreamStats.h"

struct AVFormatContext;
class QXmlStreamWriter;
class CommonStreamStats;

class StreamsStats {

public:
    typedef std::unique_ptr<CommonStreamStats> CommonStreamStatsPtr;

    StreamsStats(AVFormatContext* context = NULL);
    ~StreamsStats();

    bool readFromXML(const char* data, size_t size);
    void writeToXML(QXmlStreamWriter* writer);

    int bitsPerRawSample() const;

private:
    std::list<CommonStreamStatsPtr> streams;
};

#endif // StreamsStats_H
