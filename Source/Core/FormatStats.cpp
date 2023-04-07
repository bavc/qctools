#include "Core/Core.h"
#include "Core/FormatStats.h"

extern "C"
{
#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#endif

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
#include <QXmlStreamWriter>
using namespace tinyxml2;

static FormatStats::Metadata extractMetadata(AVDictionary *tags)
{
    FormatStats::Metadata metadata;

    if (!tags)
        return metadata;

    AVDictionaryEntry *tag = NULL;

    while ((tag = av_dict_get(tags, "", tag, AV_DICT_IGNORE_SUFFIX))) {
        if(tag->key && tag->value)
        {
            metadata.push_back(std::pair<std::string, std::string>(tag->key, tag->value));
        } else
        {
            break;
        }
    }

    return metadata;
}

FormatStats::FormatStats(AVFormatContext *context) :
    filename(context != NULL ? context->url : ""),
    nb_streams(context != NULL ? context->nb_streams : 0),
    nb_programs(context != NULL ? context->nb_programs : 0),
    format_name(context != NULL ? context->iformat->name : ""),
    format_long_name(context != NULL ? context->iformat->long_name : ""),
    start_time(context != NULL ? std::to_string( (context->start_time * av_q2d( {1, AV_TIME_BASE} )) ) : ""),
    duration(context != NULL ? std::to_string( (context->duration * av_q2d( {1, AV_TIME_BASE} )) ) : ""),
    size(context != NULL ? (context->pb ? avio_size(context->pb) : -1) : -1),
    bit_rate(context != NULL ? (context->bit_rate > 0 ? context->bit_rate : -1) : -1),
    probe_score(context != NULL ? context->probe_score : 0),
    metadata(context != NULL ? (extractMetadata(context->metadata)) : Metadata())
{

}

void FormatStats::writeToXML(QXmlStreamWriter *writer)
{
    assert(writer);

    writer->writeStartElement("format");

    writer->writeAttribute("filename", QString::fromStdString(getFilename()));
    writer->writeAttribute("nb_streams", QString::number(getNb_streams()));
    writer->writeAttribute("nb_programs", QString::number(getNb_programs()));
    writer->writeAttribute("format_name", QString::fromStdString(getFormat_name()));
    writer->writeAttribute("format_long_name", QString::fromStdString(getFormat_long_name()));
    writer->writeAttribute("start_time", QString::fromStdString(getStart_time()));
    writer->writeAttribute("duration", QString::fromStdString(getDuration()));
    writer->writeAttribute("size", getSize() > 0 ? QString::number(getSize()) : NOT_AVAILABLE);
    writer->writeAttribute("bit_rate", getBit_rate() > 0 ? QString::number(getBit_rate()) : NOT_AVAILABLE);
    writer->writeAttribute("probe_score", QString::number(getProbe_score()));

    writeMetadataToXML(writer);

    writer->writeEndElement();
}

void FormatStats::writeMetadataToXML(QXmlStreamWriter *writer)
{
    assert(writer);

    Metadata::const_iterator it = metadata.cbegin();
    for(; it != metadata.cend(); ++it)
    {
        writer->writeStartElement("tag");
        writer->writeAttribute("key", QString::fromStdString(it->first) );
        writer->writeAttribute("value", QString::fromStdString(it->second) );
        writer->writeEndElement();
    }
}

std::string FormatStats::getFilename() const
{
    return filename;
}

void FormatStats::setFilename(const std::string &value)
{
    filename = value;
}

int FormatStats::getNb_streams() const
{
    return nb_streams;
}

void FormatStats::setNb_streams(int value)
{
    nb_streams = value;
}

int FormatStats::getNb_programs() const
{
    return nb_programs;
}

void FormatStats::setNb_programs(int value)
{
    nb_programs = value;
}

std::string FormatStats::getFormat_name() const
{
    return format_name;
}

void FormatStats::setFormat_name(const std::string &value)
{
    format_name = value;
}

std::string FormatStats::getFormat_long_name() const
{
    return format_long_name;
}

void FormatStats::setFormat_long_name(const std::string &value)
{
    format_long_name = value;
}

std::string FormatStats::getStart_time() const
{
    return start_time;
}

void FormatStats::setStart_time(const std::string &value)
{
    start_time = value;
}

std::string FormatStats::getDuration() const
{
    return duration;
}

int FormatStats::getSize() const
{
    return size;
}

int FormatStats::getBit_rate() const
{
    return bit_rate;
}

void FormatStats::setBit_rate(int value)
{
    bit_rate = value;
}

int FormatStats::getProbe_score() const
{
    return probe_score;
}

void FormatStats::setProbe_score(int value)
{
    probe_score = value;
}

bool FormatStats::readFromXML(const char *Data, size_t Size)
{
    // XML input
    XMLDocument Document;
    if (Document.Parse(Data, Size))
       return false;

    XMLElement* Root=Document.FirstChildElement("ffprobe:ffprobe");
    if (Root)
    {
        XMLElement* format=Root->FirstChildElement("format");
        if (format)
        {
            const char* fileName = format->Attribute("filename");
            if(fileName)
                setFilename(fileName);

            const char* nb_streams = format->Attribute("nb_streams");
            if(nb_streams)
                setNb_streams(std::stoi(nb_streams));

            const char* nb_programs = format->Attribute("nb_programs");
            if(nb_programs)
                setNb_programs(std::stoi(nb_programs));

            const char* format_name = format->Attribute("format_name");
            if(format_name)
                setFormat_name(format_name);

            const char* format_long_name = format->Attribute("format_long_name");
            if(format_long_name)
                setFormat_long_name(format_long_name);

            const char* duration = format->Attribute("duration");
            if(duration)
                setDuration(duration);

            const char* start_time = format->Attribute("start_time");
            if(start_time)
                setStart_time(start_time);

            const char* size = format->Attribute("size");
            if(size && !isNotAvailable(size))
                setSize(std::stoi(size));

            const char* bit_rate = format->Attribute("bit_rate");
            if(bit_rate && !isNotAvailable(bit_rate))
                setBit_rate(std::stoi(bit_rate));

            const char* probe_score = format->Attribute("probe_score");
            if(probe_score)
                setProbe_score(std::stoi(probe_score));

            XMLElement* Tag=format->FirstChildElement();
            while (Tag)
            {
                if (!strcmp(Tag->Value(), "tag"))
                {
                    const char* key=Tag->Attribute("key");
                    if (key)
                    {
                        const char* value = Tag->Attribute("value");
                        if(value) {
                            metadata.push_back(std::pair<std::string, std::string>(key, value));
                        }
                    }
                }

                Tag=Tag->NextSiblingElement();
            }
        }
    }

    return true;
}

void FormatStats::setSize(int value)
{
    size = value;
}

void FormatStats::setDuration(const std::string &value)
{
    duration = value;
}
