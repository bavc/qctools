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
#include <QVector>

#include "Core/CommonStreamStats.h"

class QXmlStreamWriter;
class CommonStreamStats;
class QAVStream;
class StreamsStats {

public:
    typedef std::unique_ptr<CommonStreamStats> CommonStreamStatsPtr;

    StreamsStats(QVector<QAVStream*> streams = {});
    ~StreamsStats();

    bool readFromXML(const char* data, size_t size);
    void writeToXML(QXmlStreamWriter* writer);

    int bitsPerRawVideoSample() const;
    int avSampleFormat() const;

    const CommonStreamStatsPtr& getReferenceStream() const;
    const std::list<CommonStreamStatsPtr>& getStreams() const { return streams; }
private:
    std::list<CommonStreamStatsPtr> streams;
    CommonStreamStatsPtr nullCommonStreamStatsPtr;
};

#endif // StreamsStats_H
