/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include "Core/VideoStreamStats.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
extern "C"
{
#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#endif

#include <libavutil/frame.h>
#include <libavutil/pixdesc.h>
#include <libavutil/rational.h>
#include <libavformat/avformat.h>
}

#include "tinyxml2.h"
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <cfloat>
#include <cassert>
#include <QString>
#include <QXmlStreamWriter>
using namespace tinyxml2;

std::string field_order_to_string(AVFieldOrder field_order) {
    if (field_order == AV_FIELD_PROGRESSIVE)
        return "progressive";
    else if (field_order == AV_FIELD_TT)
        return "tt";
    else if (field_order == AV_FIELD_BB)
        return "bb";
    else if (field_order == AV_FIELD_TB)
        return "tb";
    else if (field_order == AV_FIELD_BT)
        return "bt";
    else
        return "unknown";
}

static std::string rational_to_string(AVRational r, char sep) {
    return std::to_string(r.num).append(1, sep).append(std::to_string(r.den));
}

VideoStreamStats::VideoStreamStats(XMLElement *streamElement) : CommonStreamStats(streamElement)
{
    codec_type = "video";

    const char* width_value = streamElement->Attribute("width");
    if(width_value)
        width = width_value;

    const char* height_value = streamElement->Attribute("height");
    if(height_value)
        height = height_value;

    const char* coded_width_value = streamElement->Attribute("coded_width");
    if(coded_width_value)
        coded_width = coded_width_value;

    const char* coded_height_value = streamElement->Attribute("coded_height");
    if(coded_height_value)
        coded_height = coded_height_value;

    const char* has_b_frames_value = streamElement->Attribute("has_b_frames");
    if(has_b_frames_value)
        has_b_frames = has_b_frames_value;

    const char* sample_aspect_ratio_value = streamElement->Attribute("sample_aspect_ratio");
    if(sample_aspect_ratio_value)
        sample_aspect_ratio = sample_aspect_ratio_value;

    const char* display_aspect_ratio_value = streamElement->Attribute("display_aspect_ratio");
    if(display_aspect_ratio_value)
        display_aspect_ratio = display_aspect_ratio_value;

    const char* pix_fmt_value = streamElement->Attribute("pix_fmt");
    if(pix_fmt_value)
        pix_fmt = pix_fmt_value;

    const char* level_value = streamElement->Attribute("level");
    if(level_value)
        level = level_value;

    const char* field_order_value = streamElement->Attribute("field_order");
    if(field_order_value)
        field_order = field_order_value;

    const char* refs_value = streamElement->Attribute("refs");
    if(refs_value)
        refs = refs_value;
}

VideoStreamStats::VideoStreamStats(AVStream* stream, AVFormatContext *context) : CommonStreamStats(stream),
    width(stream != NULL ? std::to_string(stream->codecpar->width) : ""),
    height(stream != NULL ? std::to_string(stream->codecpar->height) : ""),
    coded_width(stream != NULL ? std::to_string(stream->codec->coded_width) : ""),
    coded_height(stream != NULL ? std::to_string(stream->codec->coded_height) : ""),
    has_b_frames(stream != NULL ? std::to_string(stream->codecpar->video_delay) : ""),
    sample_aspect_ratio(""),
    display_aspect_ratio(""),
    pix_fmt(""),
    level(stream != NULL ? std::to_string(stream->codecpar->level) : ""),
    field_order(stream != NULL ? field_order_to_string(stream->codecpar->field_order) : ""),
    refs(stream != NULL ? std::to_string(stream->codec->refs) : "")
{
    if(stream != NULL)
    {
        AVRational sar = av_guess_sample_aspect_ratio(context, stream, NULL);
        sample_aspect_ratio = rational_to_string(sar, ':');

        if(stream->codec->sample_aspect_ratio.den)
        {
            AVRational dar;

            av_reduce(&dar.num, &dar.den,
                      stream->codecpar->width  * sar.num,
                      stream->codecpar->height * sar.den,
                      1024*1024);

            display_aspect_ratio = rational_to_string(dar, ':');
        }

        const char* s = av_get_pix_fmt_name((AVPixelFormat) stream->codecpar->format);
        if(!s)
            s = "unknown";

        pix_fmt = s;
    }

    codec_type = "video";
}

void VideoStreamStats::writeStreamInfoToXML(QXmlStreamWriter *writer)
{
    CommonStreamStats::writeStreamInfoToXML(writer);

    assert(writer);

    writer->writeAttribute("width", QString::fromStdString(getWidth()));
    writer->writeAttribute("height", QString::fromStdString(getHeight()));
    writer->writeAttribute("coded_width", QString::fromStdString(getCoded_width()));
    writer->writeAttribute("coded_height", QString::fromStdString(getCoded_height()));
    writer->writeAttribute("has_b_frames", QString::fromStdString(getHas_b_frames()));
    writer->writeAttribute("sample_aspect_ratio", QString::fromStdString(getSample_aspect_ratio()));
    writer->writeAttribute("display_aspect_ratio", QString::fromStdString(getDisplay_aspect_ratio()));
    writer->writeAttribute("pix_fmt", QString::fromStdString(getPix_fmt()));
    writer->writeAttribute("level", QString::fromStdString(getLevel()));
    writer->writeAttribute("field_order", QString::fromStdString(getField_order()));
    writer->writeAttribute("refs", QString::fromStdString(getRefs()));
    writer->writeAttribute("r_frame_rate", QString::fromStdString(getR_frame_rate()));
    writer->writeAttribute("avg_frame_rate", QString::fromStdString(getAvg_frame_rate()));
    writer->writeAttribute("time_base", QString::fromStdString(getTime_base()));
    writer->writeAttribute("start_pts", QString::fromStdString(getStart_pts()));
    writer->writeAttribute("start_time", QString::fromStdString(getStart_time()));

    CommonStreamStats::writeDispositionInfoToXML(writer);
    CommonStreamStats::writeMetadataToXML(writer);
}

std::string VideoStreamStats::getWidth() const
{
    return width;
}

void VideoStreamStats::setWidth(const std::string &value)
{
    width = value;
}

std::string VideoStreamStats::getHeight() const
{
    return height;
}

void VideoStreamStats::setHeight(const std::string &value)
{
    height = value;
}

std::string VideoStreamStats::getCoded_width() const
{
    return coded_width;
}

void VideoStreamStats::setCoded_width(const std::string &value)
{
    coded_width = value;
}

std::string VideoStreamStats::getCoded_height() const
{
    return coded_height;
}

void VideoStreamStats::setCoded_height(const std::string &value)
{
    coded_height = value;
}

std::string VideoStreamStats::getHas_b_frames() const
{
    return has_b_frames;
}

void VideoStreamStats::setHas_b_frames(const std::string &value)
{
    has_b_frames = value;
}

std::string VideoStreamStats::getSample_aspect_ratio() const
{
    return sample_aspect_ratio;
}

void VideoStreamStats::setSample_aspect_ratio(const std::string &value)
{
    sample_aspect_ratio = value;
}

std::string VideoStreamStats::getDisplay_aspect_ratio() const
{
    return display_aspect_ratio;
}

void VideoStreamStats::setDisplay_aspect_ratio(const std::string &value)
{
    display_aspect_ratio = value;
}

std::string VideoStreamStats::getPix_fmt() const
{
    return pix_fmt;
}

void VideoStreamStats::setPix_fmt(const std::string &value)
{
    pix_fmt = value;
}

std::string VideoStreamStats::getLevel() const
{
    return level;
}

void VideoStreamStats::setLevel(const std::string &value)
{
    level = value;
}

std::string VideoStreamStats::getField_order() const
{
    return field_order;
}

void VideoStreamStats::setField_order(const std::string &value)
{
    field_order = value;
}

std::string VideoStreamStats::getRefs() const
{
    return refs;
}

void VideoStreamStats::setRefs(const std::string &value)
{
    refs = value;
}
