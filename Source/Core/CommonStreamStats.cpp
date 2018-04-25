/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include "Core/CommonStreamStats.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
extern "C"
{
#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#endif

#include <libavutil/frame.h>
#include <libavformat/avformat.h>
}

#include "Core/Core.h"
#include "tinyxml2.h"
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <cfloat>
#include <cassert>
#include <QXmlStreamWriter>
using namespace tinyxml2;

static std::string rational_to_string(AVRational r, char sep) {
    return std::to_string(r.num).append(1, sep).append(std::to_string(r.den));
}

static CommonStreamStats::Metadata extractMetadata(AVDictionary *tags)
{
    CommonStreamStats::Metadata metadata;

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

CommonStreamStats::CommonStreamStats(XMLElement *streamElement) :
    stream_index(0),
    codec_tag(0),
    disposition(0),
    bits_per_raw_sample(0)
{
    const char* stream_index_value = streamElement->Attribute("index");
    if(stream_index_value)
        stream_index = std::stoi(stream_index_value);

    const char* codec_name_value = streamElement->Attribute("codec_name");
    if(codec_name_value)
        codec_name = codec_name_value;

    const char* codec_long_name_value = streamElement->Attribute("codec_long_name");
    if(codec_long_name_value)
        codec_long_name = codec_long_name_value;

    const char* codec_tag_value = streamElement->Attribute("codec_tag");
    if(codec_tag_value)
        codec_tag = std::stoi(codec_tag_value, 0, 16);

    const char* r_frame_rate_value = streamElement->Attribute("r_frame_rate");
    if(r_frame_rate_value)
        r_frame_rate = r_frame_rate_value;

    const char* avg_frame_rate_value = streamElement->Attribute("avg_frame_rate");
    if(avg_frame_rate_value)
        avg_frame_rate = avg_frame_rate_value;

    const char* time_base_value = streamElement->Attribute("time_base");
    if(time_base_value)
        time_base = time_base_value;

    const char* start_pts_value = streamElement->Attribute("start_pts");
    if(start_pts_value)
        start_pts = start_pts_value;

    const char* start_time_value = streamElement->Attribute("start_time");
    if(start_time_value)
        start_time = start_time_value;

    const char* codec_time_base_value = streamElement->Attribute("codec_time_base");
    if(codec_time_base_value)
        codec_time_base = codec_time_base_value;

    XMLElement* dispositionElement = streamElement->FirstChildElement("disposition");
    if(dispositionElement) {
        static const char* dispositionAttributes[] = {
            "default",
            "dub",
            "original",
            "comment",
            "lyrics",
            "karaoke",
            "forced",
            "hearing_impaired",
            "visual_impaired",
            "clean_effects",
            "attached_pic",
            "timed_thumbnails"
        };
        static const int dispositionFlags[] = {
            AV_DISPOSITION_DEFAULT,
            AV_DISPOSITION_DUB,
            AV_DISPOSITION_ORIGINAL,
            AV_DISPOSITION_COMMENT,
            AV_DISPOSITION_LYRICS,
            AV_DISPOSITION_KARAOKE,
            AV_DISPOSITION_FORCED,
            AV_DISPOSITION_HEARING_IMPAIRED,
            AV_DISPOSITION_VISUAL_IMPAIRED,
            AV_DISPOSITION_CLEAN_EFFECTS,
            AV_DISPOSITION_ATTACHED_PIC,
            AV_DISPOSITION_TIMED_THUMBNAILS
        };

        for(size_t i = 0; i < sizeof(dispositionAttributes) / sizeof(const char*); ++i)
        {
            const char* value = dispositionElement->Attribute(dispositionAttributes[i]);
            if(value) {
                disposition |= (std::stoi(value) == 1 ? dispositionFlags[i] : 0);
            }
        }
    }

    const char* bits_per_raw_sample_value = streamElement->Attribute("bits_per_raw_sample");
    if(bits_per_raw_sample_value) {
        bits_per_raw_sample = atoi(bits_per_raw_sample_value);
    }

    XMLElement* tag = streamElement->FirstChildElement("tag");

    while(tag != NULL)
    {
        const char* key = tag->Attribute("key");
        if(key) {
            const char* value = tag->Attribute("value");
            if(value) {
                metadata.push_back(std::make_pair(key, value));
            }
        }

        tag = tag->NextSiblingElement();
    }
}

CommonStreamStats::CommonStreamStats(AVStream* stream) :
    stream_index(stream->index),
    codec_name(stream ? stream->codec->codec->name : ""),
    codec_long_name(stream ? stream->codec->codec->long_name : ""),
    codec_tag(stream ? stream->codec->codec_tag : 0),
    r_frame_rate(stream != NULL ? rational_to_string(stream->r_frame_rate, '/') : ""),
    avg_frame_rate(stream != NULL ? rational_to_string(stream->avg_frame_rate, '/') : ""),
    time_base(stream != NULL ? rational_to_string(stream->time_base, '/') : ""),
    start_pts(stream != NULL ? std::to_string(stream->start_time) : ""),
    start_time(stream != NULL ? std::to_string(stream->start_time * av_q2d(stream->time_base)) : ""),
    codec_time_base(stream ? rational_to_string(stream->codec->time_base, '/') : ""),
    disposition(stream ? stream->disposition : 0),
    bits_per_raw_sample(stream ? stream->codec->bits_per_raw_sample : 0),
    metadata(stream ? extractMetadata(stream->metadata) : Metadata())
{

}

CommonStreamStats::~CommonStreamStats()
{

}

int CommonStreamStats::get_StreamIndex() const
{
    return stream_index;
}


std::string CommonStreamStats::getCodec_Name() const
{
    return codec_name;
}

std::string CommonStreamStats::getCodec_Long_Name() const
{
    return codec_long_name;
}

std::string CommonStreamStats::getCodec_Type() const
{
    return codec_type;
}

int CommonStreamStats::getType() const
{
    return stream_type;
}

std::string CommonStreamStats::getCodec_Time_Base() const
{
    return codec_time_base;
}

std::string CommonStreamStats::getCodec_TagString() const
{
    char val_str[128];
    av_get_codec_tag_string(val_str, sizeof(val_str), codec_tag);

    return val_str;
}

int CommonStreamStats::getCodec_Tag() const
{
    return codec_tag;
}

std::string CommonStreamStats::getR_frame_rate() const
{
    return r_frame_rate;
}

void CommonStreamStats::setR_frame_rate(const std::string &value)
{
    r_frame_rate = value;
}

std::string CommonStreamStats::getAvg_frame_rate() const
{
    return avg_frame_rate;
}

void CommonStreamStats::setAvg_frame_rate(const std::string &value)
{
    avg_frame_rate = value;
}

std::string CommonStreamStats::getTime_base() const
{
    return time_base;
}

void CommonStreamStats::setTime_base(const std::string &value)
{
    time_base = value;
}

std::string CommonStreamStats::getStart_pts() const
{
    return start_pts;
}

void CommonStreamStats::setStart_pts(const std::string &value)
{
    start_pts = value;
}

std::string CommonStreamStats::getStart_time() const
{
    return start_time;
}

void CommonStreamStats::setStart_time(const std::string &value)
{
    start_time = value;
}

const CommonStreamStats::Metadata& CommonStreamStats::getMetadata() const
{
    return metadata;
}

void CommonStreamStats::setMetadata(const Metadata &value)
{
    metadata = value;
}

void CommonStreamStats::writeToXML(QXmlStreamWriter *writer)
{
    writer->writeStartElement("stream");
    writeStreamInfoToXML(writer);
    writer->writeEndElement();
}

void CommonStreamStats::writeStreamInfoToXML(QXmlStreamWriter* writer)
{
    assert(writer);

    writer->writeAttribute("index", QString::number(get_StreamIndex()));
    writer->writeAttribute("codec_name", QString::fromStdString(getCodec_Name()));
    writer->writeAttribute("codec_long_name", QString::fromStdString(getCodec_Long_Name()));
    writer->writeAttribute("codec_type", QString::fromStdString(getCodec_Type()));
    writer->writeAttribute("codec_time_base", QString::fromStdString(getCodec_Time_Base()));
    writer->writeAttribute("codec_tag_string", QString::fromStdString(getCodec_TagString()));

    char buffer[32];
    snprintf(buffer, sizeof(buffer), "0x%04x", getCodec_Tag());
    writer->writeAttribute("codec_tag", QString::fromLatin1(buffer));

    writer->writeAttribute("bits_per_raw_sample", QString::number(getBitsPerRawSample()));
}

void CommonStreamStats::writeDispositionInfoToXML(QXmlStreamWriter *writer)
{
    writer->writeStartElement("disposition");

    writer->writeAttribute("default", QString::number(!!(disposition & AV_DISPOSITION_DEFAULT)));
    writer->writeAttribute("dub", QString::number(!!(disposition & AV_DISPOSITION_DUB)));
    writer->writeAttribute("original", QString::number(!!(disposition & AV_DISPOSITION_ORIGINAL)));
    writer->writeAttribute("comment", QString::number(!!(disposition & AV_DISPOSITION_COMMENT)));
    writer->writeAttribute("lyrics", QString::number(!!(disposition & AV_DISPOSITION_LYRICS)));
    writer->writeAttribute("karaoke", QString::number(!!(disposition & AV_DISPOSITION_KARAOKE)));
    writer->writeAttribute("forced", QString::number(!!(disposition & AV_DISPOSITION_FORCED)));
    writer->writeAttribute("hearing_impaired", QString::number(!!(disposition & AV_DISPOSITION_HEARING_IMPAIRED)));
    writer->writeAttribute("visual_impaired", QString::number(!!(disposition & AV_DISPOSITION_VISUAL_IMPAIRED)));
    writer->writeAttribute("clean_effects", QString::number(!!(disposition & AV_DISPOSITION_CLEAN_EFFECTS)));
    writer->writeAttribute("attached_pic", QString::number(!!(disposition & AV_DISPOSITION_ATTACHED_PIC)));
    writer->writeAttribute("timed_thumbnails", QString::number(!!(disposition & AV_DISPOSITION_TIMED_THUMBNAILS)));

    writer->writeEndElement();
}

void CommonStreamStats::writeMetadataToXML(QXmlStreamWriter *writer)
{
    Metadata::const_iterator it = metadata.cbegin();
    for(; it != metadata.cend(); ++it)
    {
        writer->writeStartElement("tag");
        writer->writeAttribute("key", QString::fromStdString(it->first) );
        writer->writeAttribute("value", QString::fromStdString(it->second) );
        writer->writeEndElement();
    }
}

int CommonStreamStats::getDisposition() const
{
    return disposition;
}

void CommonStreamStats::setDisposition(int value)
{
    disposition = value;
}

int CommonStreamStats::getBitsPerRawSample() const
{
    return bits_per_raw_sample;
}

void CommonStreamStats::setBitsPerRawSample(int value)
{
    bits_per_raw_sample = value;
}
