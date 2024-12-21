#include "Core/StreamsStats.h"
#include <qavstream.h>

extern "C"
{
#include <libavutil/pixdesc.h>
#include <libavutil/rational.h>
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>
}

#include "tinyxml2.h"
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <cfloat>
#include <cassert>
#include <string>
#include <list>
#include <QString>
#include <QDebug>
#include <QXmlStreamWriter>

#include "Core/VideoStreamStats.h"
#include "Core/AudioStreamStats.h"

using namespace tinyxml2;

StreamsStats::StreamsStats(QVector<QAVStream*> qavstreams)
{
    for (size_t pos = 0; pos < qavstreams.count(); ++pos)
    {
        auto qavstream = qavstreams[pos];
        switch (qavstream->stream()->codecpar->codec_type)
        {
            case AVMEDIA_TYPE_VIDEO:
                streams.push_back(std::unique_ptr<VideoStreamStats>(new VideoStreamStats(qavstream)));
                break;
            case AVMEDIA_TYPE_AUDIO:
                streams.push_back(std::unique_ptr<AudioStreamStats>(new AudioStreamStats(qavstream)));
                break;
            default:
                qDebug() << "only Audio / Video streams are supported for now.. skipping stream of index = " << pos << " and of type = " << qavstream->stream()->codecpar->codec_type;
        }
    }
}

StreamsStats::~StreamsStats()
{

}

bool StreamsStats::readFromXML(const char *data, size_t size)
{
    XMLDocument Document;
    if (Document.Parse(data, size))
       return false;

    XMLElement* rootElement = Document.FirstChildElement("ffprobe:ffprobe");
    if (rootElement)
    {
        XMLElement* streamsElement = rootElement->FirstChildElement("streams");
        if (streamsElement)
        {
            XMLElement* streamElement = streamsElement->FirstChildElement("stream");
            while(streamElement)
            {
                const char* codec_type = streamElement->Attribute("codec_type");
                if(codec_type)
                {
                    if(strcmp(codec_type, "video") == 0)
                        streams.push_back(std::unique_ptr<VideoStreamStats>(new VideoStreamStats(streamElement)));
                    else if(strcmp(codec_type, "audio") == 0)
                        streams.push_back(std::unique_ptr<AudioStreamStats>(new AudioStreamStats(streamElement)));
                }

                streamElement = streamElement->NextSiblingElement();
            }
        }
    }

    return true;
}

void StreamsStats::writeToXML(QXmlStreamWriter *writer)
{
    writer->writeStartElement("streams");
    for(std::list<CommonStreamStatsPtr>::const_iterator it = streams.cbegin(); it != streams.cend(); ++it)
    {
        (*it)->writeToXML(writer);
    }
    writer->writeEndElement();

}

int StreamsStats::bitsPerRawVideoSample() const
{
    for(auto& stream : streams) {
        if(stream->getType() == AVMEDIA_TYPE_VIDEO && stream->getBitsPerRawSample() != 0)
            return stream->getBitsPerRawSample();
    }

    return 0;
}

int StreamsStats::avSampleFormat() const
{
    for(auto& stream : streams) {
        if(stream->getType() == AVMEDIA_TYPE_AUDIO && (static_cast<AudioStreamStats*> (stream.get()))->getSample_fmt() != AV_SAMPLE_FMT_NONE)
            return (static_cast<AudioStreamStats*> (stream.get()))->getSample_fmt();
    }

    return AV_SAMPLE_FMT_NONE;
}
