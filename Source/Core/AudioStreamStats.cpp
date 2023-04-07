/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include "Core/AudioStreamStats.h"
//---------------------------------------------------------------------------
#include <qavplayer.h>
#include <qavstream.h>
//---------------------------------------------------------------------------
extern "C"
{
#include <libavutil/frame.h>
#include <libavutil/pixdesc.h>
#include <libavutil/bprint.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
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

AudioStreamStats::AudioStreamStats(XMLElement *streamElement) : CommonStreamStats(streamElement),
    sample_rate(0),
    channels(0),
    bits_per_sample(0)
{
    codec_type = "audio";
    stream_type = AVMEDIA_TYPE_AUDIO;

    const char* sample_fmt_value = streamElement->Attribute("sample_fmt");
    if(sample_fmt_value) {
        sample_fmt = av_get_sample_fmt(sample_fmt_value);
        sample_fmt_string = sample_fmt_value;
    }

    const char* sample_rate_value = streamElement->Attribute("sample_rate");
    if(sample_rate_value)
        sample_rate = std::stoi(sample_rate_value);

    const char* channels_value = streamElement->Attribute("channels");
    if(channels_value)
        channels = std::stoi(channels_value);

    const char* channel_layout_value = streamElement->Attribute("channel_layout");
    if(channel_layout_value)
        channel_layout = channel_layout_value;

    const char* bits_per_sample_value = streamElement->Attribute("bits_per_sample");
    if(bits_per_sample_value)
        bits_per_sample = std::stoi(bits_per_sample_value);
}

AudioStreamStats::AudioStreamStats(QAVStream* stream, AVFormatContext *context) : CommonStreamStats(stream),
    sample_fmt_string(""),
    sample_rate(stream != NULL ? stream->stream()->codecpar->sample_rate : 0),
    channels(stream != NULL ? stream->stream()->codecpar->channels : 0),
    channel_layout(""),
    bits_per_sample(stream != NULL ? av_get_bits_per_sample(stream->stream()->codecpar->codec_id) : 0)
{
    Q_UNUSED(context);

    codec_type = "audio";
    stream_type = AVMEDIA_TYPE_AUDIO;

    if(stream)
    {
        sample_fmt = stream->stream()->codecpar->format;
        const char* s = av_get_sample_fmt_name((AVSampleFormat) sample_fmt);
        if (s)
            sample_fmt_string = s;
        else
            sample_fmt_string = "unknown";

        AVBPrint pbuf;
        av_bprint_init(&pbuf, 1, AV_BPRINT_SIZE_UNLIMITED);

        if (stream->stream()->codecpar->channel_layout) {
            av_bprint_clear(&pbuf);
            av_bprint_channel_layout(&pbuf, stream->stream()->codecpar->channels, stream->stream()->codecpar->channel_layout);
            channel_layout = pbuf.str;
        } else {
            channel_layout = "unknown";
        }

        av_bprint_finalize(&pbuf, NULL);
    }
}

void AudioStreamStats::writeStreamInfoToXML(QXmlStreamWriter *writer)
{
    CommonStreamStats::writeStreamInfoToXML(writer);

    assert(writer);

    writer->writeAttribute("sample_fmt", QString::fromStdString(getSample_fmt_string()));
    writer->writeAttribute("sample_rate", QString::number(getSample_rate()));
    writer->writeAttribute("channels", QString::number(getChannels()));
    writer->writeAttribute("channel_layout", QString::fromStdString(getChannel_layout()));
    writer->writeAttribute("bits_per_sample", QString::number(getBits_per_sample()));
    writer->writeAttribute("r_frame_rate", QString::fromStdString(getR_frame_rate()));
    writer->writeAttribute("avg_frame_rate", QString::fromStdString(getAvg_frame_rate()));
    writer->writeAttribute("time_base", QString::fromStdString(getTime_base()));
    writer->writeAttribute("start_pts", QString::fromStdString(getStart_pts()));
    writer->writeAttribute("start_time", QString::fromStdString(getStart_time()));

    CommonStreamStats::writeDispositionInfoToXML(writer);
    CommonStreamStats::writeMetadataToXML(writer);
}

int AudioStreamStats::getSample_fmt() const
{
    return sample_fmt;
}

std::string AudioStreamStats::getSample_fmt_string() const
{
    return sample_fmt_string;
}

void AudioStreamStats::setSample_fmt_string(const std::string &value)
{
    sample_fmt_string = value;
}

int AudioStreamStats::getSample_rate() const
{
    return sample_rate;
}

void AudioStreamStats::setSample_rate(const int &value)
{
    sample_rate = value;
}

int AudioStreamStats::getChannels() const
{
    return channels;
}

void AudioStreamStats::setChannels(int value)
{
    channels = value;
}

std::string AudioStreamStats::getChannel_layout() const
{
    return channel_layout;
}

void AudioStreamStats::setChannel_layout(const std::string &value)
{
    channel_layout = value;
}

int AudioStreamStats::getBits_per_sample() const
{
    return bits_per_sample;
}

void AudioStreamStats::setBits_per_sample(int value)
{
    bits_per_sample = value;
}
