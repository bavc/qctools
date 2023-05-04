/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include "Core/FileInformation.h"
#include "Core/SignalServer.h"
#include "Core/VideoStats.h"
#include "Core/AudioStats.h"
#include "Core/FormatStats.h"
#include "Core/StreamsStats.h"

#include "FFmpegVideoEncoder.h"

extern "C" {
#include <libavcodec/codec_id.h>
#include <libavformat/avformat.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>

#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>

#include <libavutil/pixfmt.h>
#include <libavutil/imgutils.h>
#include <libavutil/ffversion.h>

#ifndef WITH_SYSTEM_FFMPEG
#include <config.h>
#else
#include <ctime>
#endif
}

#include <QProcess>
#include <QFileInfo>
#include <QDebug>
#include <QTemporaryFile>
#include <QXmlStreamWriter>
#include <QUrl>
#include <QBuffer>
#include <QPair>
#include <QDir>
#include <QEventLoop>
#include <zlib.h>
#include <zconf.h>

#include <cmath>
#include <string>
#include <sstream>
#include <iostream>
#include <cassert>
#include <QEventLoop>
#include <algorithm>
#include <qavplayer.h>
#include <qavcodec_p.h>
#include <float.h>

// extracted from FFMpeg_Glue:

std::string FFmpeg_Version()
{
    return FFMPEG_VERSION;
}

//---------------------------------------------------------------------------
int FFmpeg_Year()
{
#ifdef WITH_SYSTEM_FFMPEG
    time_t t = std::time(0);
    struct tm * now = std::localtime(&t);
    return now->tm_year + 1900;
#else
    return CONFIG_THIS_YEAR;
#endif
}

//---------------------------------------------------------------------------
std::string FFmpeg_Compiler()
{
#ifdef WITH_SYSTEM_FFMPEG
    return "not available";
#else
    return CC_IDENT;
#endif
}

//---------------------------------------------------------------------------
std::string FFmpeg_Configuration()
{
#ifdef WITH_SYSTEM_FFMPEG
    return "not available";
#else
    return FFMPEG_CONFIGURATION;
#endif
}

void LibsVersion_Inject(std::stringstream &LibsVersion, const char* Name, int Value)
{
    LibsVersion<<' ' << Name << "=\"" << Value << '\"';
}
void LibsVersion_Inject(std::stringstream &LibsVersion, const char* Name, const char* Value)
{
    LibsVersion<<' ' << Name << "=\"" << Value << '\"';
}

#ifndef WITH_SYSTEM_FFMPEG
#define LIBSVERSION(libname, LIBNAME)                                               \
if (CONFIG_##LIBNAME)                                                           \
    {                                                                               \
            unsigned int version = libname##_version();                                 \
            LibsVersion<<"        <library_version";                                    \
            LibsVersion_Inject(LibsVersion, "name",    "lib" #libname);                 \
            LibsVersion_Inject(LibsVersion, "major",   LIB##LIBNAME##_VERSION_MAJOR);   \
            LibsVersion_Inject(LibsVersion, "minor",   LIB##LIBNAME##_VERSION_MINOR);   \
            LibsVersion_Inject(LibsVersion, "micro",   LIB##LIBNAME##_VERSION_MICRO);   \
            LibsVersion_Inject(LibsVersion, "version", version);                        \
            LibsVersion_Inject(LibsVersion, "ident",   LIB##LIBNAME##_IDENT);           \
            LibsVersion<<"/>\n";                                                        \
    }
#else
#define LIBSVERSION(libname, LIBNAME)                                               \
do {                                                                            \
        unsigned int version = libname##_version();                                 \
        LibsVersion<<"        <library_version";                                    \
        LibsVersion_Inject(LibsVersion, "name",    "lib" #libname);                 \
        LibsVersion_Inject(LibsVersion, "major",   LIB##LIBNAME##_VERSION_MAJOR);   \
        LibsVersion_Inject(LibsVersion, "minor",   LIB##LIBNAME##_VERSION_MINOR);   \
        LibsVersion_Inject(LibsVersion, "micro",   LIB##LIBNAME##_VERSION_MICRO);   \
        LibsVersion_Inject(LibsVersion, "version", version);                        \
        LibsVersion_Inject(LibsVersion, "ident",   LIB##LIBNAME##_IDENT);           \
        LibsVersion<<"/>\n";                                                        \
} while (0)
#endif

//---------------------------------------------------------------------------
std::string FFmpeg_LibsVersion()
{
    std::stringstream LibsVersion;
    LIBSVERSION(avutil,     AVUTIL);
    LIBSVERSION(avcodec,    AVCODEC);
    LIBSVERSION(avformat,   AVFORMAT);
    LIBSVERSION(avfilter,   AVFILTER);
    LIBSVERSION(swscale,    SWSCALE);
    return LibsVersion.str();
}

int guessBitsPerRawSampleFromFormat(int pixelFormat)
{
    switch(pixelFormat) {
    case AV_PIX_FMT_MONOWHITE:
    case AV_PIX_FMT_MONOBLACK:
        return 1;

    case AV_PIX_FMT_RGB444LE:
    case AV_PIX_FMT_RGB444BE:
    case AV_PIX_FMT_BGR444LE:
    case AV_PIX_FMT_BGR444BE:
        return 4;

    case AV_PIX_FMT_RGB565BE:
    case AV_PIX_FMT_RGB565LE:
    case AV_PIX_FMT_RGB555BE:
    case AV_PIX_FMT_RGB555LE:
    case AV_PIX_FMT_BGR565BE:
    case AV_PIX_FMT_BGR565LE:
    case AV_PIX_FMT_BGR555BE:
    case AV_PIX_FMT_BGR555LE:
        return 6;

    case AV_PIX_FMT_YUV420P9BE:
    case AV_PIX_FMT_YUV420P9LE:
    case AV_PIX_FMT_YUV444P9BE:
    case AV_PIX_FMT_YUV444P9LE:
    case AV_PIX_FMT_YUV422P9BE:
    case AV_PIX_FMT_YUV422P9LE:
    case AV_PIX_FMT_GBRP9BE:
    case AV_PIX_FMT_GBRP9LE:
    case AV_PIX_FMT_YUVA420P9BE:
    case AV_PIX_FMT_YUVA420P9LE:
    case AV_PIX_FMT_YUVA422P9BE:
    case AV_PIX_FMT_YUVA422P9LE:
    case AV_PIX_FMT_YUVA444P9BE:
    case AV_PIX_FMT_YUVA444P9LE:
    case AV_PIX_FMT_GRAY9BE:
    case AV_PIX_FMT_GRAY9LE:
        return 9;

    case AV_PIX_FMT_YUV420P10BE:
    case AV_PIX_FMT_YUV420P10LE:
    case AV_PIX_FMT_YUV422P10BE:
    case AV_PIX_FMT_YUV422P10LE:
    case AV_PIX_FMT_YUV444P10BE:
    case AV_PIX_FMT_YUV444P10LE:
    case AV_PIX_FMT_GBRP10BE:
    case AV_PIX_FMT_GBRP10LE:
    case AV_PIX_FMT_YUVA420P10BE:
    case AV_PIX_FMT_YUVA420P10LE:
    case AV_PIX_FMT_YUVA422P10BE:
    case AV_PIX_FMT_YUVA422P10LE:
    case AV_PIX_FMT_YUVA444P10BE:
    case AV_PIX_FMT_YUVA444P10LE:
    case AV_PIX_FMT_YUV440P10LE:
    case AV_PIX_FMT_YUV440P10BE:
    case AV_PIX_FMT_GBRAP10BE:
    case AV_PIX_FMT_GBRAP10LE:
    case AV_PIX_FMT_GRAY10BE:
    case AV_PIX_FMT_GRAY10LE:
        return 10;

    case AV_PIX_FMT_XYZ12LE:
    case AV_PIX_FMT_XYZ12BE:
    case AV_PIX_FMT_YUV420P12BE:
    case AV_PIX_FMT_YUV420P12LE:
    case AV_PIX_FMT_YUV422P12BE:
    case AV_PIX_FMT_YUV422P12LE:
    case AV_PIX_FMT_YUV444P12BE:
    case AV_PIX_FMT_YUV444P12LE:
    case AV_PIX_FMT_GBRP12BE:
    case AV_PIX_FMT_GBRP12LE:
    case AV_PIX_FMT_YUV440P12LE:
    case AV_PIX_FMT_YUV440P12BE:
    case AV_PIX_FMT_GBRAP12BE:
    case AV_PIX_FMT_GBRAP12LE:
    case AV_PIX_FMT_GRAY12BE:
    case AV_PIX_FMT_GRAY12LE:
        return 12;

    case AV_PIX_FMT_YUV420P14BE:
    case AV_PIX_FMT_YUV420P14LE:
    case AV_PIX_FMT_YUV422P14BE:
    case AV_PIX_FMT_YUV422P14LE:
    case AV_PIX_FMT_YUV444P14BE:
    case AV_PIX_FMT_YUV444P14LE:
    case AV_PIX_FMT_GBRP14BE:
    case AV_PIX_FMT_GBRP14LE:
        return 14;

    case AV_PIX_FMT_GRAY16BE:
    case AV_PIX_FMT_YUV420P16LE:
    case AV_PIX_FMT_YUV420P16BE:
    case AV_PIX_FMT_YUV422P16LE:
    case AV_PIX_FMT_YUV422P16BE:
    case AV_PIX_FMT_YUV444P16LE:
    case AV_PIX_FMT_YUV444P16BE:
    case AV_PIX_FMT_GRAY16LE:
    case AV_PIX_FMT_RGB48BE:
    case AV_PIX_FMT_RGB48LE:
    case AV_PIX_FMT_BGR48BE:
    case AV_PIX_FMT_BGR48LE:
    case AV_PIX_FMT_GBRP16BE:
    case AV_PIX_FMT_GBRP16LE:
    case AV_PIX_FMT_YUVA420P16BE:
    case AV_PIX_FMT_YUVA420P16LE:
    case AV_PIX_FMT_YUVA422P16BE:
    case AV_PIX_FMT_YUVA422P16LE:
    case AV_PIX_FMT_YUVA444P16BE:
    case AV_PIX_FMT_YUVA444P16LE:
    case AV_PIX_FMT_RGBA64BE:
    case AV_PIX_FMT_RGBA64LE:
    case AV_PIX_FMT_BGRA64BE:
    case AV_PIX_FMT_BGRA64LE:
    case AV_PIX_FMT_YA16BE:
    case AV_PIX_FMT_YA16LE:
    case AV_PIX_FMT_GBRAP16BE:
    case AV_PIX_FMT_GBRAP16LE:
    case AV_PIX_FMT_BAYER_BGGR16LE:
    case AV_PIX_FMT_BAYER_BGGR16BE:
    case AV_PIX_FMT_BAYER_RGGB16LE:
    case AV_PIX_FMT_BAYER_RGGB16BE:
    case AV_PIX_FMT_BAYER_GBRG16LE:
    case AV_PIX_FMT_BAYER_GBRG16BE:
    case AV_PIX_FMT_BAYER_GRBG16LE:
    case AV_PIX_FMT_BAYER_GRBG16BE:
    case AV_PIX_FMT_AYUV64LE:
        return 16;
    default:
        return 8;
    }
}

int bitsPerAudioSample(int audioFormat)
{
    return av_get_bytes_per_sample((AVSampleFormat) audioFormat);
}


bool isFloatAudioSampleFormat(int audioFormat)
{
    return audioFormat == AV_SAMPLE_FMT_FLT
           || audioFormat == AV_SAMPLE_FMT_FLTP
           || audioFormat == AV_SAMPLE_FMT_DBL
           || audioFormat == AV_SAMPLE_FMT_DBLP;
}

bool isSignedAudioSampleFormat(int audioFormat)
{
    return audioFormat == AV_SAMPLE_FMT_S16
           || audioFormat == AV_SAMPLE_FMT_S16P
           || audioFormat == AV_SAMPLE_FMT_S32
           || audioFormat == AV_SAMPLE_FMT_S32P
           || audioFormat == AV_SAMPLE_FMT_S64
           || audioFormat == AV_SAMPLE_FMT_S64P;
}

bool isUnsignedAudioSampleFormat(int audioFormat)
{
    return audioFormat == AV_SAMPLE_FMT_U8
           || audioFormat == AV_SAMPLE_FMT_U8P;
}

//***************************************************************************
// Simultaneous parsing
//***************************************************************************
static int ActiveParsing_Count=0;

void FileInformation::run()
{
    if(m_jobType == FileInformation::Exporting)
    {
        runExport();
    }
}

void FileInformation::runExport()
{
    Export_XmlGz(m_exportFileName, m_exportFilters);
}

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
void FileInformation::readStats(QIODevice& StatsFromExternalData_File, bool StatsFromExternalData_FileName_IsCompressed)
{
    m_hasStats = true;

    streamsStats = new StreamsStats();
    formatStats = new FormatStats();

    //XML init
    const char*  Xml_HeaderFooter="</frames></ffprobe:ffprobe><ffprobe:ffprobe><frames>";
    const size_t Xml_HeaderSize=25;
    const size_t Xml_FooterSize=27;
    const size_t Xml_MaxSize=0x1000000; //Blocks of 16 MiB, arbitrary chosen
    char* Xml=new char[Xml_MaxSize+Xml_HeaderSize+Xml_FooterSize];

    //Read init
    const size_t Compressed_MaxSize=0x100000; //Blocks of 1 MiB, arbitrary chosen
    char* Compressed=new char[Compressed_MaxSize];

    //Uncompress init
    z_stream strm;
    if (StatsFromExternalData_FileName_IsCompressed)
    {
        strm.next_in = NULL;
        strm.avail_in = 0;
        strm.next_out = NULL;
        strm.avail_out = 0;
        strm.total_out = 0;
        strm.zalloc = Z_NULL;
        strm.zfree = Z_NULL;
        inflateInit2(&strm, 15 + 16); // 15 + 16 are magic values for gzip
    }

    //Load and parse compressed data chunk by chunk
    char* Xml_Pointer=Xml;
    int inflate_Result;
    int TEMP = 0;

    for (;;)
    {
        //Load
        qint64 ReadSize;
        size_t avail_out=Xml_MaxSize-(Xml_Pointer-Xml);
        if (StatsFromExternalData_FileName_IsCompressed)
            ReadSize=StatsFromExternalData_File.read(Compressed, Compressed_MaxSize); //Load in an intermediate buffer for decompression
        else
            ReadSize=StatsFromExternalData_File.read(Xml_Pointer, avail_out); //Load directly in the XML buffer
        if (!ReadSize)
            break;
        TEMP += ReadSize;

        //Parse compressed data, with handling of the case the output buffer is not big enough
        strm.next_in=(Bytef*)Compressed;
        strm.avail_in=ReadSize;
        for (;;)
        {
            size_t Xml_Size=Xml_Pointer-Xml;
            if (StatsFromExternalData_FileName_IsCompressed)
            {
                //inflate
                strm.next_out=(Bytef*)Xml_Pointer;
                strm.avail_out= (avail_out - Xml_Size);
                if ((inflate_Result=inflate(&strm, Z_NO_FLUSH))<0)
                    break;
                Xml_Size+=(avail_out - Xml_Size) -strm.avail_out;
            }
            else
                Xml_Size+=ReadSize;

            //Cut the XML after the last XML frame footer, and keep the remaining data for the next turn
            size_t Xml_SizeForParsing=Xml_Size;
            //Look for XML frame footer
            while (Xml_SizeForParsing>Xml_HeaderSize &&
                    ( Xml[Xml_SizeForParsing-1]!='>' //"</frame>"
                || Xml[Xml_SizeForParsing-2]!='e'
                || Xml[Xml_SizeForParsing-3]!='m'
                || Xml[Xml_SizeForParsing-4]!='a'
                || Xml[Xml_SizeForParsing-5]!='r'
                || Xml[Xml_SizeForParsing-6]!='f'
                || Xml[Xml_SizeForParsing-7]!='/'
                || Xml[Xml_SizeForParsing-8]!='<'))
                Xml_SizeForParsing--;
            if (Xml_SizeForParsing<=Xml_HeaderSize)
            {
                //The block does not contain any complete frame, looping in order to get more data from the file
                Xml_Pointer=Xml+Xml_Size;
                break;
            }

            //Insert Xml_HeaderFooter inside the XML content
            memmove(Xml+Xml_SizeForParsing+Xml_HeaderSize+Xml_FooterSize, Xml+Xml_SizeForParsing, Xml_Size-Xml_SizeForParsing);
            memcpy(Xml+Xml_SizeForParsing, Xml_HeaderFooter, Xml_HeaderSize+Xml_FooterSize);
            Xml_Size+=Xml_HeaderSize+Xml_FooterSize;
            Xml_SizeForParsing+=Xml_FooterSize;

            CommonStats::statsFromExternalData(Xml, Xml_SizeForParsing, [&](int type, size_t index) -> CommonStats* {
                if(Stats.size() <= index)
                    Stats.resize(index + 1);

                if(!Stats[index])
                {
                    if(type == Type_Video)
                        Stats[index] = new VideoStats(index);
                    else if(type == Type_Audio)
                        Stats[index] = new AudioStats(index);
                }

                return Stats[index];
            });

            //Cut the parsed content
            memmove(Xml, Xml+Xml_SizeForParsing, Xml_Size-Xml_SizeForParsing);
            Xml_Pointer=Xml+Xml_Size-Xml_SizeForParsing;

            //Check if we need to stop
            if (!StatsFromExternalData_FileName_IsCompressed || strm.avail_out || inflate_Result == Z_STREAM_END)
                break;
        }

        //Coherency checks
        if (Xml_Pointer>=Xml+Xml_MaxSize+Xml_HeaderSize)
            break;
    }

    //Inform the parser that parsing is finished
    for(auto stats : Stats)
        if(stats)
            stats->StatsFromExternalData_Finish();

    //Parse formats
    formatStats->readFromXML(Xml, Xml_Pointer - Xml);

    //Parse streams
    streamsStats->readFromXML(Xml, Xml_Pointer - Xml);

    //Cleanup
    if (StatsFromExternalData_FileName_IsCompressed)
        inflateEnd(&strm);
    delete[] Compressed;
    delete[] Xml;
}

QSize FileInformation::panelSize() const
{
    return m_panelSize;
}

const QMap<std::string, QVector<int>> &FileInformation::panelOutputsByTitle() const
{
    return m_panelOutputsByTitle;
}

const std::map<std::string, std::string> &FileInformation::getPanelOutputMetadata(size_t index) const
{
    return m_panelMetadata[index];
}

size_t FileInformation::getPanelFramesCount(size_t index) const
{
    if(m_panelFrames.size() <= index)
        return 0;

    return m_panelFrames[index].size();
}

QAVVideoFrame FileInformation::getPanelFrame(size_t index, size_t panelFrameIndex) const
{
    return m_panelFrames[index][panelFrameIndex];
}

QByteArray getAttachment(const QString &fileName, QString& attachmentFileName)
{
    QByteArray attachment;

    // Open file
    AVFormatContext* formatContext = nullptr;
    auto fileNameString = fileName.toStdString();

    auto result = avformat_open_input(&formatContext, fileNameString.c_str(), NULL, NULL);
    if (result >= 0)
    {
        if (avformat_find_stream_info(formatContext, NULL)>=0)
        {
            for(auto i = 0; i < formatContext->nb_streams; ++i)
            {
                if(formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_ATTACHMENT) {
                    auto st = formatContext->streams[i];
                    if(st->codecpar->extradata_size != 0) {
                        attachment = QByteArray((const char*) st->codecpar->extradata, st->codecpar->extradata_size);
                        AVDictionaryEntry *e = av_dict_get(st->metadata, "filename", NULL, 0);
                        if(e) {
                            attachmentFileName = e->value;
                        }
                        break;
                    }
                }
            }
        }
    } else {
        char errbuf[255];
        qDebug() << "Could not open file: " << av_make_error_string(errbuf, sizeof errbuf, result) << "\n";
    }

    avformat_close_input(&formatContext);

    return attachment;
}

std::map<std::string, std::string> getStreamMetadata(AVStream* stream)
{
    std::map<std::string, std::string> metadata;

    auto tags = stream->metadata;
    if (!tags)
        return metadata;

    AVDictionaryEntry *tag = NULL;

    while ((tag = av_dict_get(tags, "", tag, AV_DICT_IGNORE_SUFFIX))) {
        if(tag->key && tag->value)
        {
            metadata[QString(tag->key).toLower().toStdString()] = tag->value;
        } else
        {
            break;
        }
    }

    return metadata;
}

FileInformation::FileInformation (SignalServer* signalServer, const QString &FileName_, activefilters ActiveFilters_, activealltracks ActiveAllTracks_,
                                  QMap<QString, std::tuple<QString, QString, QString, QString, int>> activePanels,
                                  const QString &QCvaultFileNamePrefix,
                                  int FrameCount) :
    FileName(FileName_),
    ActiveFilters(ActiveFilters_),
    ActiveAllTracks(ActiveAllTracks_),
    signalServer(signalServer),
    m_jobType(Parsing),
	streamsStats(NULL),
    formatStats(NULL),
    m_parsed(false),
    m_autoCheckFileUploaded(true),
    m_autoUpload(true),
    m_hasStats(false),
    m_commentsUpdated(false)
{
    static struct RegisterMetatypes {
        RegisterMetatypes() {
            qRegisterMetaType<SharedFile>("SharedFile");
        }
    } registerMetatypes;

    connect(this, SIGNAL(parsingCompleted(bool)), this, SLOT(parsingDone(bool)));

    QString StatsFromExternalData_FileName;
    bool StatsFromExternalData_FileName_IsCompressed=false;

    // Finding the right file names (both media file and stats file)

    static const QString dotQctoolsDotXmlDotGz = ".qctools.xml.gz";
    static const QString dotQctoolsDotXml = ".qctools.xml";
    static const QString dotXmlDotGz = ".xml.gz";
    static const QString dotQctoolsDotMkv = ".qctools.mkv";

    QByteArray attachment;
    std::string glueFileName = FileName.toUtf8().data();

    if (FileName.endsWith(dotQctoolsDotXmlDotGz))
    {
        StatsFromExternalData_FileName=FileName;
        FileName.resize(FileName.length() - dotQctoolsDotXmlDotGz.length());
        if(!QFile::exists(FileName)) {
            FileName = FileName + dotQctoolsDotXmlDotGz;
        }

        StatsFromExternalData_FileName_IsCompressed=true;
    }
    else if (FileName.endsWith(dotQctoolsDotXml))
    {
        StatsFromExternalData_FileName=FileName;
        FileName.resize(FileName.length() - dotQctoolsDotXml.length());

        if(!QFile::exists(FileName)) {
            FileName = FileName + dotQctoolsDotXml;
        }
    }
    else if (FileName.endsWith(dotXmlDotGz))
    {
        StatsFromExternalData_FileName=FileName;
        FileName.resize(FileName.length() - dotXmlDotGz.length());

        if(!QFile::exists(FileName)) {
            FileName = FileName + dotXmlDotGz;
        }

        StatsFromExternalData_FileName_IsCompressed=true;
    }
    else if (FileName.endsWith(dotQctoolsDotMkv))
    {        
        attachment = getAttachment(FileName, StatsFromExternalData_FileName);
        FileName.resize(FileName.length() - dotQctoolsDotMkv.length());

        if(!QFile::exists(FileName)) {
            FileName = FileName + dotQctoolsDotMkv;
        }
    }

    if (StatsFromExternalData_FileName.size()==0)
    {
        if (QFile::exists(FileName + dotQctoolsDotMkv))
        {
            attachment = getAttachment(FileName + dotQctoolsDotMkv, StatsFromExternalData_FileName);
            glueFileName = glueFileName + dotQctoolsDotMkv.toStdString();
        }
        else if (QFile::exists(FileName + dotQctoolsDotXmlDotGz))
        {
            StatsFromExternalData_FileName=FileName + dotQctoolsDotXmlDotGz;
            StatsFromExternalData_FileName_IsCompressed=true;
        }
        else if (QFile::exists(FileName + dotQctoolsDotXml))
        {
            StatsFromExternalData_FileName=FileName + dotQctoolsDotXml;
        }
        else if (QFile::exists(FileName + dotXmlDotGz))
        {
            StatsFromExternalData_FileName=FileName + dotXmlDotGz;
            StatsFromExternalData_FileName_IsCompressed=true;
        }
        else if (!QCvaultFileNamePrefix.isEmpty())
        {
            auto QCvaultFileName = QFileInfo(QCvaultFileNamePrefix).fileName();
            QDir QCvaultPath = QFileInfo(QCvaultFileNamePrefix).absolutePath();
            auto fileNameWithoutPath = QFileInfo(FileName).fileName();
            while (QCvaultFileName.size() >= fileNameWithoutPath.size())
            {
                // Is there compatible file names in QCvault path
                auto list = QCvaultPath.entryList(QStringList(QCvaultFileName + "*" + dotQctoolsDotMkv), QDir::Files, QDir::Name);
                if (list.size() == 1)
                {
                    attachment = getAttachment(QCvaultPath.absolutePath() + "/" + list[0], StatsFromExternalData_FileName);
                    glueFileName = (QCvaultPath.absolutePath() + "/" + list[0]).toStdString();
                    break;
                }
                list = QCvaultPath.entryList(QStringList(QCvaultFileName + "*" + dotQctoolsDotXmlDotGz), QDir::Files, QDir::Name);
                if (list.size() == 1)
                {
                    StatsFromExternalData_FileName = QCvaultPath.absolutePath() + "/" + list[0] + dotQctoolsDotXmlDotGz;
                    StatsFromExternalData_FileName_IsCompressed = true;
                    break;
                }

                if ( QCvaultFileName.size() == 1)
                    break;

                for (auto QCvaultFileNamePos = QCvaultFileName.size() - 1; QCvaultFileNamePos; QCvaultFileNamePos--)
                {
                    auto Character = QCvaultFileName.at(QCvaultFileNamePos);
                    if (Character == '.')
                    {
                        QCvaultFileName.resize(QCvaultFileNamePos);
                        break;
                    }
                }
            }
        }
    }

    QString shortFileName;
    std::unique_ptr<QIODevice> StatsFromExternalData_File;

    if(attachment.isEmpty()) {
        QFileInfo fileInfo(StatsFromExternalData_FileName);
        shortFileName = fileInfo.fileName();
        StatsFromExternalData_File.reset(new QFile(StatsFromExternalData_FileName));
    } else {
        shortFileName = StatsFromExternalData_FileName;
        StatsFromExternalData_File.reset(new QBuffer(&attachment));
        StatsFromExternalData_FileName_IsCompressed = true;
    }

    // External data optional input
    bool StatsFromExternalData_IsOpen=StatsFromExternalData_File->open(QIODevice::ReadOnly);

    // Running FFmpeg
    #ifdef _WIN32
        std::replace(glueFileName.begin(), glueFileName .end(), '/', '\\' );
    #endif
    std::string Filters[Type_Max];
    if (!StatsFromExternalData_IsOpen)
    {
        if (ActiveFilters[ActiveFilter_Video_signalstats])
            Filters[0]+=",signalstats=stat=tout+vrep+brng";
        if (ActiveFilters[ActiveFilter_Video_cropdetect])
            Filters[0]+=",cropdetect=reset=1:round=1";
        if (ActiveFilters[ActiveFilter_Video_Idet])
            Filters[0]+=",idet=half_life=1";
        if (ActiveFilters[ActiveFilter_Video_Deflicker])
            Filters[0]+=",deflicker=bypass=1";
        if (ActiveFilters[ActiveFilter_Video_Entropy])
            Filters[0]+=",entropy=mode=normal";
        if (ActiveFilters[ActiveFilter_Video_EntropyDiff])
            Filters[0]+=",entropy=mode=diff";
        if (ActiveFilters[ActiveFilter_Video_Psnr] && ActiveFilters[ActiveFilter_Video_Ssim])
        {
            Filters[0]+=",split[a][b];[a]field=top[a1];[b]field=bottom,split[b1][b2];[a1][b1]psnr[c1];[c1][b2]ssim";
        }
        else
        {
            if (ActiveFilters[ActiveFilter_Video_Psnr])
                Filters[0]+=",split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1][b1]psnr";
            if (ActiveFilters[ActiveFilter_Video_Ssim])
                Filters[0]+=",split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1][b1]ssim";
        }
        Filters[0].erase(0, 1); // remove first comma
        if (ActiveFilters[ActiveFilter_Audio_astats])
            Filters[1]+=",aformat=sample_fmts=flt|fltp,astats=metadata=1:reset=1:length=0.4";
        if (ActiveFilters[ActiveFilter_Audio_aphasemeter])
            Filters[1]+=",aphasemeter=video=0";
        if (ActiveFilters[ActiveFilter_Audio_EbuR128])
            Filters[1]+=",ebur128=metadata=1,aformat=sample_fmts=flt|fltp:sample_rates=${AUDIO_SAMPLE_RATE}";
        Filters[1].erase(0, 1); // remove first comma

        m_panelSize.setWidth(512);
    }
    else
    {
        readStats(*StatsFromExternalData_File, StatsFromExternalData_FileName_IsCompressed);

        if(signalServer->enabled() && m_autoCheckFileUploaded)
        {
            checkFileUploaded(shortFileName);
        }
    }

    if(glueFileName  == "-")
        glueFileName  = "pipe:0";

    m_mediaParser = new QAVPlayer();
    m_mediaParser->setSource(glueFileName.c_str());
    m_mediaParser->setSynced(false);

    QEventLoop loop;
    QMetaObject::Connection c;
    c = connect(m_mediaParser, &QAVPlayer::mediaStatusChanged, this, [&, this]() {
        loop.exit();
        QObject::disconnect(c);
    });
    loop.exec();

    if(!streamsStats && !formatStats)
    {
        auto streams = m_mediaParser->availableVideoStreams();
        streams.append(m_mediaParser->availableAudioStreams());
        streams.append(m_mediaParser->availableSubtitleStreams());

        AVFormatContext* FormatContext = nullptr;
        if (avformat_open_input(&FormatContext, glueFileName.c_str(), NULL, NULL)>=0)
        {
            QVector<QAVStream*> orderedStreams;
            if (avformat_find_stream_info(FormatContext, NULL)>=0) {

                containerFormat = FormatContext->iformat->long_name;
                streamCount = FormatContext->nb_streams;
                bitRate = FormatContext->bit_rate;

                for(auto i = 0; i < FormatContext->nb_streams; ++i) {
                    auto codec_type = FormatContext->streams[i]->codecpar->codec_type;
                    if(codec_type != AVMEDIA_TYPE_VIDEO && codec_type != AVMEDIA_TYPE_AUDIO)
                        continue;

                    auto streamIt = std::find_if(streams.begin(), streams.end(), [i](QAVStream& stream) {
                        return stream.stream()->index == i;
                    });

                    if(streamIt == streams.end()) {
                        qDebug() << "error: it should never happen";
                        assert(false);
                        continue;
                    }

                    if(streamIt->codec()->codec() == nullptr) {
                        qDebug() << "error: codec is null for stream" << i << "... skipping";
                        continue;
                    }
                    orderedStreams.append(&*streamIt);

                    auto Duration = 0;
                    auto FrameCount = streamIt->stream()->nb_frames;
                    if (streamIt->stream()->duration != AV_NOPTS_VALUE)
                        Duration= ((double)streamIt->stream()->duration)*streamIt->stream()->time_base.num/streamIt->stream()->time_base.den;

                    // If duration is not known, estimating it
                    if (Duration==0 && FormatContext->duration!=AV_NOPTS_VALUE)
                        Duration=((double)FormatContext->duration)/AV_TIME_BASE;

                    // If frame count is not known, estimating it
                    if (FrameCount==0 && streamIt->stream()->avg_frame_rate.num && streamIt->stream()->avg_frame_rate.den && Duration)
                        FrameCount=Duration*streamIt->stream()->avg_frame_rate.num/streamIt->stream()->avg_frame_rate.den;
                    if (FrameCount==0
                        && ((streamIt->stream()->time_base.num==1 && streamIt->stream()->time_base.den>=24 && streamIt->stream()->time_base.den<=60)
                            || (streamIt->stream()->time_base.num==1001 && streamIt->stream()->time_base.den>=24000 && streamIt->stream()->time_base.den<=60000)))
                        FrameCount=streamIt->stream()->duration;

                    if(streamIt->stream()->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                        auto Stat=new VideoStats(FrameCount, Duration, &*streamIt);
                        Stats.push_back(Stat);
                    } else if(streamIt->stream()->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                        auto Stat=new AudioStats(FrameCount, Duration, &*streamIt);
                        Stats.push_back(Stat);
                    }
                }

                streamsStats = new StreamsStats(orderedStreams, FormatContext);
                formatStats = new FormatStats(FormatContext);
            }

            avformat_close_input(&FormatContext);
        }
    }

    if(attachment.isEmpty()) {

        QList<QString> filters;
        filters.append("signalstats=stat=tout+vrep+brng,split[a][b];[a]field=top[a1];[b]field=bottom[b1];[a1][b1]psnr [stats]");
        if(!m_mediaParser->currentAudioStreams().empty())
            filters.append("aformat=sample_fmts=flt|fltp,astats=metadata=1:reset=1:length=0.4");
        filters.append("scale=72:72,format=rgb24 [thumbnails]");

        if(!m_mediaParser->currentVideoStreams().empty()) {
            auto codecHeight = m_mediaParser->currentVideoStreams()[0].stream()->codecpar->height;
            qDebug() << "codec height: " << codecHeight;
            m_panelSize.setHeight(codecHeight);
        }
        QSet<int> visitedStreamTypes;

        for(auto & streamStat : Stats)
        {
            auto streamType = streamStat->Type_Get();
            auto allTracks = ActiveAllTracks[streamType == AVMEDIA_TYPE_VIDEO ? Type_Video : Type_Audio];

            if(!allTracks && visitedStreamTypes.contains(streamType))
                continue;

            visitedStreamTypes.insert(streamType);

            for(auto panelTitle : activePanels.keys())
            {
                auto panelType = std::get<4>(activePanels[panelTitle]);
                if(streamType != panelType)
                    continue;

                auto sampleRate = 0;
                auto filter = std::get<0>(activePanels[panelTitle]);
                while(filter.indexOf(QString("${PANEL_WIDTH}")) != -1)
                    filter.replace(QString("${PANEL_WIDTH}"), QString::number(m_panelSize.width()));
                while(filter.indexOf(QString("${AUDIO_FRAME_RATE}")) != -1)
                    filter.replace(QString("${AUDIO_FRAME_RATE}"), QString::number(32));
                while(filter.indexOf(QString("${AUDIO_SAMPLE_RATE}")) != -1)
                    filter.replace(QString("${AUDIO_SAMPLE_RATE}"), QString::number(/* Glue->sampleRate(streamIndex)) */ sampleRate));
                while(filter.indexOf(QString("${DEFAULT_HEIGHT}")) != -1)
                    filter.replace(QString("${DEFAULT_HEIGHT}"), QString::number(360));

                auto version = std::get<1>(activePanels[panelTitle]);
                auto yaxis = std::get<2>(activePanels[panelTitle]);
                auto legend = std::get<3>(activePanels[panelTitle]);

                auto f = filter + QString(" [panel_%1]").arg(m_panelMetadata.size());
                qDebug() << "f: " << f;
                filters.append(f);

                std::map<std::string, std::string> metadata;
                metadata["filter"] = filter.toStdString();
                metadata["version"] = version.toStdString();
                metadata["yaxis"] = yaxis.toStdString();
                metadata["legend"] = legend.toStdString();
                metadata["panel_type"] = panelType == 0 ? "video" : "audio";

                m_panelMetadata.append(metadata);

                // qDebug() << "added output" << output << streamIndex << output->Title.c_str() << "streamType: " << streamType << "filter: " << filter;

                auto it = m_panelOutputsByTitle.find(panelTitle.toStdString());
                if(it != m_panelOutputsByTitle.end())
                {
                    it.value().append(m_panelMetadata.size() - 1);
                } else {
                    QVector<int> panelOutputs;
                    panelOutputs.append(m_panelMetadata.size() - 1);

                    m_panelOutputsByTitle[panelTitle.toStdString()] = panelOutputs;
                }
            }
        }

        m_mediaParser->setFilters(filters);

        QObject::connect(m_mediaParser, &QAVPlayer::audioFrame, m_mediaParser, [this](const QAVAudioFrame &frame) {
                qDebug() << "audio frame came from: " << frame.filterName();

                auto stat = Stats[1];

                stat->TimeStampFromFrame(frame.frame(), stat->x_Current);
                stat->StatsFromFrame(frame.frame(), 0, 0);

                ++AudioFrames_Pos;
                // nothing here
            },
            // Qt::QueuedConnection
            Qt::DirectConnection
            );

        QObject::connect(m_mediaParser, &QAVPlayer::videoFrame, m_mediaParser, [this](const QAVVideoFrame &frame) {
                qDebug() << "video frame came from: " << frame.filterName() << frame.stream() << "Frames_Pos = " << Frames_Pos;

                if(frame.filterName() == "stats") {
                    auto stat = Stats[0];

                    stat->TimeStampFromFrame(frame.frame(), stat->x_Current);
                    stat->StatsFromFrame(frame.frame(), frame.size().width(), frame.size().height());

                    ++Frames_Pos;
                } else if(frame.filterName().startsWith("panel")) {
                    auto index = frame.filterName().mid(5).toInt();
                    while(m_panelFrames.size() <= index)
                        m_panelFrames.append(QVector<QAVVideoFrame>());

                    m_panelFrames[index].append(frame);

                    qDebug() << "panel frame pts: " << frame.frame()->pts;
                    qDebug() << "m_panelFrames[index]: " << m_panelFrames[index].size() << index;
                }
                else
                {
                    m_thumbnails_frames.push_back(frame);
                }
            },
            //Qt::QueuedConnection
            Qt::DirectConnection
            );

        QObject::connect(m_mediaParser, &QAVPlayer::mediaStatusChanged, [this](QAVPlayer::MediaStatus status) {
            if(status == QAVPlayer::EndOfMedia) {
                m_parsed = true;
                Q_EMIT parsingCompleted(true);
            }
        });

        QObject::connect(m_mediaParser, &QAVPlayer::stateChanged, [this](QAVPlayer::State state) {
            if(state == QAVPlayer::PlayingState) {
                ++ActiveParsing_Count;
            } else if(state == QAVPlayer::StoppedState) {
                --ActiveParsing_Count;
            }
        });

    } else {
        auto availableVideoStreams = m_mediaParser->availableVideoStreams();
        m_mediaParser->setVideoStreams(availableVideoStreams);

#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
        QList<QString> filters;
        filters.append("format=rgb24");
        m_mediaParser->setFilters(filters);
#endif //

        for(auto i = 0; i < availableVideoStreams.count(); ++i) {
            if(i == 0)
                continue;

            auto metadata = getStreamMetadata(availableVideoStreams[i].stream());
            m_panelMetadata.append(metadata);

            auto title = metadata["title"];
            auto it = m_panelOutputsByTitle.find(title);
            if(it != m_panelOutputsByTitle.end())
            {
                it.value().append(i);
            } else {
                m_panelOutputsByTitle[title] = QVector<int> { i - 1 };
            }
        }

        QObject::connect(m_mediaParser, &QAVPlayer::videoFrame, m_mediaParser, [this](const QAVVideoFrame &frame) {
                qDebug() << "video frame came from: " << frame.filterName() << frame.stream() << "Frames_Pos = " << Frames_Pos;

                if(frame.stream().index() == 0) {
                    m_thumbnails_frames.push_back(frame);

                    ++Frames_Pos;
                } else {
                    int index = frame.stream().index();

                    while(m_panelFrames.size() <= index)
                        m_panelFrames.append(QVector<QAVVideoFrame>());

                    auto panelStreamIndex = index - 1;
                    m_panelFrames[panelStreamIndex].push_back(frame);
                }
            },
            //Qt::QueuedConnection
            Qt::DirectConnection
            );
    }

    // Looking for the reference stream (video or audio)
    ReferenceStream_Pos=0;
    for (; ReferenceStream_Pos<Stats.size(); ReferenceStream_Pos++)
        if (Stats[ReferenceStream_Pos] && Stats[ReferenceStream_Pos]->Type_Get()==0)
            break;
    if (ReferenceStream_Pos>=Stats.size())
    {
        ReferenceStream_Pos = 0;
        for (; ReferenceStream_Pos<Stats.size(); ReferenceStream_Pos++)
            if (Stats[ReferenceStream_Pos] && Stats[ReferenceStream_Pos]->Type_Get()==1) //Audio
                break;
        if (ReferenceStream_Pos>=Stats.size())
            Stats.clear(); //Removing all, as we can not sync with video or audio
    }

    if(!StatsFromExternalData_IsOpen || !attachment.isEmpty())
        startParse();
}

//---------------------------------------------------------------------------
FileInformation::~FileInformation ()
{
    if(m_mediaParser->state() == QAVPlayer::PlayingState)
        m_mediaParser->stop();

    bool result = wait();
    assert(result);

	delete streamsStats;
    delete formatStats;

    for (size_t Pos=0; Pos<Stats.size(); Pos++)
        delete Stats[Pos];
}

//***************************************************************************
// Parsing
//***************************************************************************

//---------------------------------------------------------------------------
void FileInformation::startParse ()
{
    m_jobType = Parsing;

    int Max=QThread::idealThreadCount();
    if (Max>2)
        Max-=2;
    else
        Max=1;
    if (ActiveParsing_Count<Max)
    {
        m_mediaParser->play();
    }
}

void FileInformation::startExport(const QString &exportFileName)
{
    m_jobType = Exporting;
    m_exportFileName = exportFileName;

    if (!isRunning())
    {
        start();
    }
}

//***************************************************************************
// Actions
//***************************************************************************

//---------------------------------------------------------------------------
void FileInformation::Export_XmlGz (const QString &ExportFileName, const activefilters& filters)
{
    std::stringstream Data;

    // Header
    Data<<"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    Data<<"<!-- Created by QCTools " << Version << " -->\n";
    Data<<"<ffprobe:ffprobe xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance' xmlns:ffprobe='http://www.ffmpeg.org/schema/ffprobe' xsi:schemaLocation='http://www.ffmpeg.org/schema/ffprobe ffprobe.xsd'>\n";
    Data<<"    <program_version version=\"" << FFmpeg_Version() << "\" copyright=\"Copyright (c) 2007-" << FFmpeg_Year() << " the FFmpeg developers\" build_date=\"" __DATE__ "\" build_time=\"" __TIME__ "\" compiler_ident=\"" << FFmpeg_Compiler() << "\" configuration=\"" << FFmpeg_Configuration() << "\"/>\n";
    Data<<"\n";
    Data<<"    <library_versions>\n";
    Data<<FFmpeg_LibsVersion();
    Data<<"    </library_versions>\n";

    Data<<"    <frames>\n";

    // From stats
    for (size_t Pos=0; Pos<Stats.size(); Pos++)
    {
        if (Stats[Pos])
        {
            if(Stats[Pos]->Type_Get() == Type_Video && !m_mediaParser->availableVideoStreams().empty())
            {
                auto videoStats = static_cast<VideoStats*>(Stats[Pos]);
                videoStats->setWidth(m_mediaParser->availableVideoStreams()[0].stream()->codecpar->width);
                videoStats->setHeight(m_mediaParser->availableVideoStreams()[0].stream()->codecpar->height);
            }
            Data<<Stats[Pos]->StatsToXML(filters);
        }
    }

    // Footer
    Data<<"    </frames>";

    QString streamsAndFormats;
    QXmlStreamWriter writer(&streamsAndFormats);
    writer.setAutoFormatting(true);
    writer.setAutoFormattingIndent(4);

    if(streamsStats)
        streamsStats->writeToXML(&writer);

    if(formatStats)
        formatStats->writeToXML(&writer);

    // add indentation
    QStringList splitted = streamsAndFormats.split("\n");
    for(size_t i = 0; i < splitted.length(); ++i)
        splitted[i] = QString(qAbs(writer.autoFormattingIndent()), writer.autoFormattingIndent() > 0 ? ' ' : '\t') + splitted[i];
    streamsAndFormats = splitted.join("\n");

    Data<<streamsAndFormats.toStdString() << "\n\n";

    Data<<"</ffprobe:ffprobe>";

    SharedFile file;
    QString name;

    if(ExportFileName.isEmpty())
    {
        file = SharedFile(new QTemporaryFile());
        QFileInfo info(fileName() + ".qctools.xml.gz");
        name = info.fileName();
    } else {
        file = SharedFile(new QFile(ExportFileName));
        QFileInfo info(ExportFileName);
        name = info.fileName();
    }

    std::string DataS=Data.str();
    uLongf Buffer_Size=65536;

    if(file->open(QIODevice::ReadWrite))
    {
        if(name.endsWith(".qctools.xml"))
        {
            auto bytesLeft = Data.str().size();
            auto writePtr = DataS.c_str();
            auto totalBytesWritten = 0;

            while(bytesLeft) {
                auto bytesToWrite = std::min(size_t(Buffer_Size), bytesLeft);
                auto bytesWritten = file->write(writePtr, bytesToWrite);
                totalBytesWritten += bytesWritten;

                Q_EMIT statsFileGenerationProgress(totalBytesWritten, DataS.size());

                writePtr += bytesToWrite;
                bytesLeft -= bytesWritten;

                if(bytesWritten != bytesToWrite)
                    break;
            }
        } else {
            char* Buffer=new char[Buffer_Size];
            z_stream strm;
            strm.next_in = (Bytef *) DataS.c_str();
            strm.avail_in = DataS.size() ;
            strm.total_out = 0;
            strm.zalloc = Z_NULL;
            strm.zfree = Z_NULL;
            if (deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY)>=0) // 15 + 16 are magic values for gzip
            {
                do
                {
                    strm.next_out = (unsigned char*) Buffer;
                    strm.avail_out = Buffer_Size;
                    if (deflate(&strm, Z_FINISH)<0)
                        break;
                    file->write(Buffer, Buffer_Size-strm.avail_out);

                    Q_EMIT statsFileGenerationProgress((char*) strm.next_in - DataS.c_str(), DataS.size());
                }
                while (strm.avail_out == 0);
                deflateEnd (&strm);
            }
            delete[] Buffer;
        }

        file->flush();
        file->seek(0);
    }

    Q_EMIT statsFileGenerated(file, name);
    m_commentsUpdated = false;
}

struct Output {
    struct AVPacketDeleter {
        void operator()(AVPacket* packet) {
            if (packet)
            {
                av_packet_unref(packet);
                av_packet_free(&packet);
            }
        }
    };

    typedef std::shared_ptr<AVFrame> AVFramePtr;
    typedef std::shared_ptr<AVPacket> AVPacketPtr;

    bool scaleBeforeEncoding = { false }; // used to change format before encoding
    SwsContext* ScaleContext = { nullptr };
    AVCodecContext* Output_CodecContext = { nullptr };
    AVFramePtr ScaledFrame;

    AVFilterGraph* FilterGraph = { nullptr };

    int Scale_OutputPixelFormat = { AV_PIX_FMT_YUVJ420P };
    int Output_PixelFormat = { AV_PIX_FMT_YUVJ420P };
    int Output_CodecID = { AV_CODEC_ID_MJPEG };
    int Width = { 0 };
    int Height = { 0 };
    int timeBaseNum = { 0 };
    int timeBaseDen = { 0 };

    ~Output() {
        if (Output_CodecContext)
            avcodec_free_context(&Output_CodecContext);

        // FFmpeg pointers - Scale
        if (ScaledFrame)
            ScaledFrame.reset();

        if (ScaleContext)
            sws_freeContext(ScaleContext);

        // FFmpeg pointers - Filter
        if (FilterGraph)
            avfilter_graph_free(&FilterGraph);
    }

    bool Scale_Init(AVFrame* frame) {
        if (!frame)
            return false;

        // Init
        ScaleContext = sws_getContext(frame->width, frame->height,
                                        (AVPixelFormat)frame->format,
                                        Width, Height,
                                        (AVPixelFormat) Scale_OutputPixelFormat,
                                        SWS_FAST_BILINEAR, NULL, NULL, NULL);

        // All is OK
        return true;
    }

    bool initEncoder(const QSize& size)
    {
        if(Output_CodecContext)
            return true;

        //
        auto *Output_Codec=avcodec_find_encoder((AVCodecID) Output_CodecID);
        if (!Output_Codec)
            return false;
        Output_CodecContext=avcodec_alloc_context3 (Output_Codec);
        if (!Output_CodecContext)
            return false;
        Output_CodecContext->qmin          = 8;
        Output_CodecContext->qmax          = 12;
        Output_CodecContext->width         = size.width();
        Output_CodecContext->height        = size.height();
        Output_CodecContext->pix_fmt       = (AVPixelFormat) Output_PixelFormat;
        Output_CodecContext->time_base.num = timeBaseNum;
        Output_CodecContext->time_base.den = timeBaseDen;

        qDebug() << "initEncoder: " << Output_CodecID << Output_CodecContext->width << Output_CodecContext->height <<
            Output_CodecContext->pix_fmt << Output_CodecContext->time_base.num << Output_CodecContext->time_base.den;

        if (avcodec_open2(Output_CodecContext, Output_Codec, NULL) < 0)
            return false;

        // All is OK
        return true;
    }

    std::unique_ptr<AVPacket, Output::AVPacketDeleter> encodeFrame(AVFrame* frame, bool* ok = nullptr)
    {
        if(ok)
            *ok = true;

        auto outPacket = std::unique_ptr<AVPacket, AVPacketDeleter>(av_packet_alloc(), AVPacketDeleter());
        av_init_packet (outPacket.get());

        outPacket->data = nullptr;
        outPacket->size = 0;

        struct NoDeleter {
            static void free(AVFrame* frame) {
                Q_UNUSED(frame)
            }
        };

        AVFramePtr Frame(frame, NoDeleter::free);

        if(scaleBeforeEncoding)
        {
            if (ScaleContext || Scale_Init(frame))
            {
                struct ScaledFrameDeleter {
                    static void free(AVFrame* frame) {
                        if(frame) {
                            av_freep(&frame->data[0]);
                            av_frame_free(&frame);
                        }
                    }
                };

                auto scaledFrame = AVFramePtr(av_frame_alloc(), ScaledFrameDeleter::free);
                av_frame_copy_props(scaledFrame.get(), Frame.get());

                scaledFrame->width = Width;
                scaledFrame->height= Height;
                scaledFrame->format=(AVPixelFormat)Scale_OutputPixelFormat;

                av_image_alloc(scaledFrame->data, scaledFrame->linesize, scaledFrame->width, scaledFrame->height, (AVPixelFormat) Scale_OutputPixelFormat, 1);
                if (sws_scale(ScaleContext, Frame->data, Frame->linesize, 0, Frame->height, scaledFrame->data, scaledFrame->linesize)<0)
                {
                    scaledFrame.reset();
                }
                else
                {
                    Frame = scaledFrame;
                }

            }
        }

        if (!Output_CodecContext && !initEncoder(QSize(Frame->width, Frame->height)))
        {
            if(ok)
                *ok = false;

            return outPacket;
        }

        int got_packet=0;
        int result = avcodec_send_frame(Output_CodecContext, Frame.get());
        if (result < 0)
        {
            char buffer[256];
            qDebug() << av_make_error_string(buffer, sizeof buffer, result);

            if(ok)
                *ok = false;

            return outPacket;
        }

        result = avcodec_receive_packet(Output_CodecContext, outPacket.get());
        if (result != 0)
        {
            char buffer[256];
            qDebug() << av_make_error_string(buffer, sizeof buffer, result);

            if(ok)
                *ok = false;

            return outPacket;
        }

        outPacket->duration = Frame->pkt_duration;
        return outPacket;
    }
};

void FileInformation::Export_QCTools_Mkv(const QString &ExportFileName, const activefilters &filters)
{
    QByteArray attachment;
    QString attachmentFileName;

    auto connection = connect(this, &FileInformation::statsFileGenerated, [&](SharedFile statsFile, const QString& name) {
        qDebug() << "fileName: " << statsFile.data()->fileName();
        attachment = statsFile->readAll();
        attachmentFileName = name;
    });

    Export_XmlGz(QString(), filters);

    makeMkvReport(ExportFileName, attachment, attachmentFileName);

    disconnect(connection);
}

void FileInformation::makeMkvReport(QString exportFileName, QByteArray attachment, QString attachmentFileName, const std::function<void(int, int)>& thumbnailsCallback, const std::function<void(int, int)>& panelsCallback)
{
    FFmpegVideoEncoder encoder;
    int thumbnailsCount = m_thumbnails_frames.size();
    int thumbnailIndex = 0;

    FFmpegVideoEncoder::Metadata metadata;
    metadata << FFmpegVideoEncoder::MetadataEntry(QString("title"), QString("QCTools Report for %1").arg(QFileInfo(fileName()).fileName()));
    metadata << FFmpegVideoEncoder::MetadataEntry(QString("creation_time"), QString("now"));

    encoder.setMetadata(metadata);

    FFmpegVideoEncoder::Source source;

    FFmpegVideoEncoder::Metadata streamMetadata;
    streamMetadata << FFmpegVideoEncoder::MetadataEntry(QString("title"), QString("Frame Thumbnails"));

    auto timeBase = QString::fromStdString(streamsStats->getStreams().begin()->get()->getTime_base());
    auto timeBaseSplitted = timeBase.split("/");
    int num = timeBaseSplitted[0].toInt();
    int den = timeBaseSplitted[1].toInt();

    auto codecTimeBase = QString::fromStdString(streamsStats->getStreams().begin()->get()->getCodec_Time_Base());
    auto codecTimeBaseSplitted = codecTimeBase.split("/");
    int codecNum = codecTimeBaseSplitted[0].toInt();
    int codecDen = codecTimeBaseSplitted[1].toInt();

    source.metadata = streamMetadata;
    source.width = m_thumbnails_frames.empty() ? 0 : m_thumbnails_frames[0].size().width();
    source.height = m_thumbnails_frames.empty() ? 0 : m_thumbnails_frames[0].size().height();

    source.num = num;
    source.den = den;

    QVector<std::shared_ptr<Output>> outputs;

    std::shared_ptr<Output> thumbnailsOutput = std::make_shared<Output>();
    thumbnailsOutput->scaleBeforeEncoding = true;
    outputs.push_back(thumbnailsOutput);

    source.getPacket = [&]() -> std::shared_ptr<AVPacket> {
        if(thumbnailsCallback)
            thumbnailsCallback(thumbnailIndex, thumbnailsCount);

        bool hasNext = thumbnailIndex < thumbnailsCount;

        if(!hasNext)
            return nullptr;

        auto frame = m_thumbnails_frames[thumbnailIndex];

        thumbnailsOutput->timeBaseDen = codecDen;
        thumbnailsOutput->timeBaseNum = codecNum;
        thumbnailsOutput->Width = frame.frame()->width;
        thumbnailsOutput->Height = frame.frame()->height;

        auto packet = thumbnailsOutput->encodeFrame(frame.frame());

        ++thumbnailIndex;

        return packet;
    };

    QVector<FFmpegVideoEncoder::Source> sources;
    sources.push_back(source);

    for(auto& panelTitle : panelOutputsByTitle().keys())
    {
        auto panelOutputIndexes = panelOutputsByTitle()[panelTitle];
        for(auto panelOutputIndex : panelOutputIndexes) {
            auto panelFramesCount = getPanelFramesCount(panelOutputIndex);
            if(panelFramesCount == 0)
                continue;

            auto panelsCount = getPanelFramesCount(panelOutputIndex);
            auto panelIndex = 0;

            FFmpegVideoEncoder::Metadata streamMetadata;
            streamMetadata << FFmpegVideoEncoder::MetadataEntry(QString("title"), QString::fromStdString(panelTitle));

            QString filterChain;
            QString filterOutputName = QString(" [panel_%1]").arg(panelOutputIndex);
            for(auto i = 0; i < m_mediaParser->filters().size(); ++i) {
                auto filter = m_mediaParser->filters().at(i);
                if(filter.endsWith(filterOutputName)) {
                    filterChain = filter.remove(filterOutputName);
                    break;
                }
            }
            streamMetadata << FFmpegVideoEncoder::MetadataEntry(QString("filterchain"), filterChain);

            auto outputMetadata = m_panelMetadata[panelOutputIndex];
            auto versionIt = outputMetadata.find("version");
            auto yaxisIt = outputMetadata.find("yaxis");
            auto legendIt = outputMetadata.find("legend");
            auto panelTypeIt = outputMetadata.find("panel_type");
            auto version = versionIt != outputMetadata.end() ? versionIt->second : "";
            auto yaxis = yaxisIt != outputMetadata.end() ? yaxisIt->second : "";
            auto legend = legendIt != outputMetadata.end() ? legendIt->second : "";
            auto panelType = panelTypeIt != outputMetadata.end() ? panelTypeIt->second : "video";
            auto isAudioPanel = panelType != "video";

            streamMetadata << FFmpegVideoEncoder::MetadataEntry(QString("version"), QString::fromStdString(version));
            streamMetadata << FFmpegVideoEncoder::MetadataEntry(QString("yaxis"), QString::fromStdString(yaxis));
            streamMetadata << FFmpegVideoEncoder::MetadataEntry(QString("legend"), QString::fromStdString(legend));
            streamMetadata << FFmpegVideoEncoder::MetadataEntry(QString("panel_type"), QString::fromStdString(panelType));

            FFmpegVideoEncoder::Source panelSource;
            panelSource.metadata = streamMetadata;
            panelSource.width = panelSize().width();
            panelSource.height = panelSize().height();

            // 2do: take related stream instead of first one
            auto timeBase = QString::fromStdString(streamsStats->getStreams().begin()->get()->getTime_base());
            auto timeBaseSplitted = timeBase.split("/");
            int num = timeBaseSplitted[0].toInt();
            int den = timeBaseSplitted[1].toInt();

            auto codecTimeBase = QString::fromStdString(streamsStats->getStreams().begin()->get()->getCodec_Time_Base());
            auto codecTimeBaseSplitted = codecTimeBase.split("/");
            int codecNum = codecTimeBaseSplitted[0].toInt();
            int codecDen = codecTimeBaseSplitted[1].toInt();

            std::shared_ptr<Output> output = std::make_shared<Output>();
            output->scaleBeforeEncoding = true;
            outputs.push_back(output);

            panelSource.num = num;
            panelSource.den = den;
            panelSource.getPacket = [output, codecNum, codecDen, panelIndex, panelsCount, panelOutputIndex, panelsCallback, this]() mutable -> std::shared_ptr<AVPacket> {
                if(panelsCallback)
                    panelsCallback(panelIndex, panelsCount);

                bool hasNext = panelIndex < panelsCount;

                if(!hasNext) {
                    return nullptr;
                }

                auto frame = getPanelFrame(panelOutputIndex, panelIndex);

                output->timeBaseDen = codecDen;
                output->timeBaseNum = codecNum;
                output->Width = frame.frame()->width;
                output->Height = frame.frame()->height;

                auto packet = output->encodeFrame(frame.frame());

                ++panelIndex;

                return packet;
            };

            sources.push_back(panelSource);
        }
    }

    encoder.makeVideo(exportFileName, sources, attachment, attachmentFileName);
}

size_t FileInformation::thumbnailsCount() {
    return m_thumbnails_frames.size();
}

//***************************************************************************
// Info
//***************************************************************************

//---------------------------------------------------------------------------

QAVVideoFrame FileInformation::getThumbnail(size_t pos)
{
    if (pos>=ReferenceStat()->x_Current || pos>= m_thumbnails_frames.size())
        return QAVVideoFrame();

    return m_thumbnails_frames[pos];
}

QString FileInformation::fileName() const
{
    return FileName;
}

int FileInformation::width() const
{
    auto videoStreams = m_mediaParser->availableVideoStreams();
    if(videoStreams.empty())
        return 0;

    return videoStreams[0].stream()->codecpar->width;
}

int FileInformation::height() const
{
    auto videoStreams = m_mediaParser->availableVideoStreams();
    if(videoStreams.empty())
        return 0;

    return videoStreams[0].stream()->codecpar->height;
}

int FileInformation::bitsPerRawSample() const
{
    auto videoStreams = m_mediaParser->availableVideoStreams();
    if(videoStreams.empty())
        return 0;

    return videoStreams[0].stream()->codecpar->bits_per_raw_sample;
}

double FileInformation::dar() const
{
    auto videoStreams = m_mediaParser->availableVideoStreams();
    if(videoStreams.empty())
        return 0;

    auto stream = videoStreams[0];
    double DAR;
    if (stream.stream()->codecpar->sample_aspect_ratio.num && stream.stream()->codecpar->sample_aspect_ratio.den)
        DAR=((double)stream.stream()->codecpar->width)/stream.stream()->codecpar->height*stream.stream()->codecpar->sample_aspect_ratio.num/stream.stream()->codecpar->sample_aspect_ratio.den;
    else
        DAR=((double)stream.stream()->codecpar->width)/stream.stream()->codecpar->height;
    return DAR;
}

std::string FileInformation::pixFormatName() const
{
    auto videoStreams = m_mediaParser->availableVideoStreams();
    if(videoStreams.empty())
        return std::string();

    auto stream = videoStreams[0];

    const AVPixFmtDescriptor* Desc=av_pix_fmt_desc_get((AVPixelFormat) stream.stream()->codecpar->format);
    if (!Desc)
        return std::string();
    return Desc->name;

}

int FileInformation::isRgbSet() const
{
    auto videoStreams = m_mediaParser->availableVideoStreams();
    if(videoStreams.empty())
        return 0;

    auto stream = videoStreams[0];

    const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get((AVPixelFormat) stream.stream()->codecpar->format);
    return (desc->flags & AV_PIX_FMT_FLAG_RGB);
}

double FileInformation::duration() const
{
    auto stat = std::find_if(Stats.begin(), Stats.end(), [](CommonStats* s) {
        return s->Type_Get() == AVMEDIA_TYPE_VIDEO;
    });

    if(stat == Stats.end())
        return 0;

    return (*stat)->x_Max[1];
}

std::string FileInformation::videoFormat() const
{
    auto videoStreams = m_mediaParser->availableVideoStreams();
    if(videoStreams.empty())
        return std::string();

    auto stream = videoStreams[0];

    if(stream.codec()->codec() == nullptr)
        return "unknown";

    if (stream.codec()->codec()->long_name == nullptr)
        return std::string();

    return stream.codec()->codec()->long_name;
}

std::string FileInformation::fieldOrder() const
{
    auto videoStreams = m_mediaParser->availableVideoStreams();
    if(videoStreams.empty())
        return std::string();

    auto stream = videoStreams[0];

    if (stream.stream()->codecpar == nullptr)
        return std::string();

    switch (stream.stream()->codecpar->field_order)
    {
        case AV_FIELD_UNKNOWN: return "unknown";
        case AV_FIELD_PROGRESSIVE: return "progressive";
        case AV_FIELD_TT:   return "TFF: top displayed first, top coded first";
        case AV_FIELD_BB:   return "BFF: bottom displayed first, bottom coded first";
        case AV_FIELD_TB:   return "TFF: top displayed first, coded interleaved";
        case AV_FIELD_BT:   return "BFF: bottom displayed first, coded interleaved";
        default: return std::string();
        }
}

std::string FileInformation::sar() const
{
    auto videoStreams = m_mediaParser->availableVideoStreams();
    if(videoStreams.empty())
        return 0;

    auto stream = videoStreams[0];

    if (stream.stream()->codecpar == nullptr)
        return std::string();

    if (stream.stream()->codecpar->sample_aspect_ratio.num && stream.stream()->codecpar->sample_aspect_ratio.num==0)
        return "Und";
    else if (stream.stream()->codecpar->sample_aspect_ratio.num)
    {
        std::ostringstream convert;
        convert << stream.stream()->codecpar->sample_aspect_ratio.num << "/" << stream.stream()->codecpar->sample_aspect_ratio.den;
        return convert.str();
    }
    else if (stream.stream()->codecpar->sample_aspect_ratio.num && stream.stream()->codecpar->sample_aspect_ratio.num==0)
        return "Und";
    else
    {
        std::ostringstream convert;
        convert << stream.stream()->codecpar->sample_aspect_ratio.num << "/" << stream.stream()->codecpar->sample_aspect_ratio.den;
        return convert.str();
    }
}

FileInformation::FrameRate FileInformation::getAvgVideoFrameRate() const
{
    auto videoStreams = m_mediaParser->availableVideoStreams();
    if(videoStreams.empty())
        return FrameRate(0, 0);

    auto stream = videoStreams[0];

    return FrameRate(stream.stream()->avg_frame_rate.num, stream.stream()->avg_frame_rate.den);
}

double FileInformation::framesDivDuration() const
{
    if(Frames_Count_Get() && duration())
        return (double) Frames_Count_Get() / duration();

    return 0;
}

std::string FileInformation::rvideoFrameRate() const
{
    auto videoStreams = m_mediaParser->availableVideoStreams();
    if(videoStreams.empty())
        return 0;

    auto stream = videoStreams[0];

    if(stream.codec()->codec() == nullptr)
        return "unknown";

    if (stream.codec()->codec()->long_name == NULL)
        return std::string();

    if (stream.stream()->r_frame_rate.num==0)
        return "Und";
    else
    {
        std::ostringstream convert;
        convert << stream.stream()->r_frame_rate.num << "/" << stream.stream()->r_frame_rate.den;
        return convert.str();
    }
}

std::string FileInformation::avgVideoFrameRate() const
{
    auto videoStreams = m_mediaParser->availableVideoStreams();
    if(videoStreams.empty())
        return 0;

    auto stream = videoStreams[0];

    if(stream.codec()->codec() == nullptr)
        return "unknown";

    if (stream.codec()->codec()->long_name == NULL)
        return std::string();

    std::ostringstream convert;
    if (stream.stream()->avg_frame_rate.num==0)
        return "Und";
    else
    {
        convert << stream.stream()->avg_frame_rate.num << "/" << stream.stream()->avg_frame_rate.den;
        return convert.str();
    }
}

std::string FileInformation::colorSpace() const
{
    auto videoStreams = m_mediaParser->availableVideoStreams();
    if(videoStreams.empty())
        return 0;

    auto stream = videoStreams[0];

    switch (stream.stream()->codecpar->color_space)
    {
    case AVCOL_SPC_RGB: return "RGB: order of coefficients is actually GBR, also IEC 61966-2-1 (sRGB)";
    case AVCOL_SPC_BT709: return "BT.709"; // full: "BT.709 / ITU-R BT1361 / IEC 61966-2-4 xvYCC709 / SMPTE RP177 Annex B"
    case AVCOL_SPC_UNSPECIFIED: return "Unspecified";
    case AVCOL_SPC_RESERVED: return "Reserved";
    case AVCOL_SPC_FCC: return "FCC Title 47 Code of Federal Regulations 73.682 (a)(20)";
    case AVCOL_SPC_BT470BG: return "BT.601 PAL";  // full: "BT.470bg / ITU-R BT601-6 625 / ITU-R BT1358 625 / ITU-R BT1700 625 PAL & SECAM / IEC 61966-2-4 xvYCC601"
    case AVCOL_SPC_SMPTE170M: return "BT.601 NTSC"; // full:"SMPTE 170m / ITU-R BT601-6 525 / ITU-R BT1358 525 / ITU-R BT1700 NTSC"
    case AVCOL_SPC_SMPTE240M: return "SMPTE 240m";
    case AVCOL_SPC_YCOCG: return "YCOCG: Used by Dirac / VC-2 and H.264 FRext, see ITU-T SG16";
    case AVCOL_SPC_BT2020_NCL: return "ITU-R BT2020 non-constant luminance system";
    case AVCOL_SPC_BT2020_CL: return "ITU-R BT2020 constant luminance system";
    case AVCOL_SPC_NB: return "Not part of ABI.";
    default: return std::string();
    }
}

std::string FileInformation::colorRange() const
{
    auto videoStreams = m_mediaParser->availableVideoStreams();
    if(videoStreams.empty())
        return 0;

    auto stream = videoStreams[0];

    switch (stream.stream()->codecpar->color_range)
    {
    case AVCOL_RANGE_UNSPECIFIED: return "Unspecified";
    case AVCOL_RANGE_MPEG: return "Broadcast Range"; // full: "Broadcast Range (219*2^n-1)"
    case AVCOL_RANGE_JPEG: return "Full Range"; // full: "Full Range (2^n-1)"
    case AVCOL_RANGE_NB: return "Not part of ABI";
    default: return std::string();
    }
}

std::string FileInformation::audioFormat() const
{
    auto audioStreams = m_mediaParser->availableAudioStreams();
    if(audioStreams.empty())
        return std::string();

    auto stream = audioStreams[0];

    if(stream.codec()->codec() == nullptr)
        return "unknown";

    if (stream.codec()->codec()->long_name == nullptr)
        return std::string();

    return stream.codec()->codec()->long_name;
}

std::string FileInformation::sampleFormat() const
{
    auto audioStreams = m_mediaParser->availableAudioStreams();
    if(audioStreams.empty())
        return std::string();

    auto stream = audioStreams[0];

    if(stream.codec()->codec() == nullptr)
        return "unknown";

    if (stream.codec()->codec()->long_name == nullptr)
        return std::string();

    switch (stream.stream()->codecpar->format)
    {
    case AV_SAMPLE_FMT_NONE: return "none";
    case AV_SAMPLE_FMT_U8: return "unsigned 8 bits";
    case AV_SAMPLE_FMT_S16: return "signed 16 bits";
    case AV_SAMPLE_FMT_S32: return "signed 32 bits";
    case AV_SAMPLE_FMT_FLT: return "float";
    case AV_SAMPLE_FMT_DBL: return "double";
    case AV_SAMPLE_FMT_U8P: return "unsigned 8 bits, planar";
    case AV_SAMPLE_FMT_S16P: return "signed 16 bits, planar";
    case AV_SAMPLE_FMT_S32P: return "signed 32 bits, planar";
    case AV_SAMPLE_FMT_FLTP: return "float, planar";
    case AV_SAMPLE_FMT_DBLP: return "double, planar";
    case AV_SAMPLE_FMT_NB: return "number of sample formats";
    default: return std::string();
    }
}

double FileInformation::samplingRate() const
{
    auto audioStreams = m_mediaParser->availableAudioStreams();
    if(audioStreams.empty())
        return 0;

    auto stream = audioStreams[0];

    return stream.stream()->codecpar->sample_rate;
}

std::string FileInformation::channelLayout() const
{
    auto audioStreams = m_mediaParser->availableAudioStreams();
    if(audioStreams.empty())
        return std::string();

    auto stream = audioStreams[0];

    if(stream.codec()->codec() == nullptr)
        return "unknown";

    if (stream.codec()->codec()->long_name == nullptr)
        return std::string();

    switch (stream.stream()->codecpar->channel_layout)
    {
    case AV_CH_LAYOUT_MONO: return "mono";
    case AV_CH_LAYOUT_STEREO: return "stereo";
    case AV_CH_LAYOUT_2POINT1: return "2.1";
    case AV_CH_LAYOUT_SURROUND: return "3.0";
    case AV_CH_LAYOUT_2_1: return "3.0(back)";
    case AV_CH_LAYOUT_4POINT0: return "4.0";
    case AV_CH_LAYOUT_QUAD: return "quad";
    case AV_CH_LAYOUT_2_2: return "quad(side)";
    case AV_CH_LAYOUT_3POINT1: return "3.1";
    case AV_CH_LAYOUT_5POINT0_BACK: return "5.0";
    case AV_CH_LAYOUT_5POINT0: return "5.0(side)";
    case AV_CH_LAYOUT_4POINT1: return "4.1";
    case AV_CH_LAYOUT_5POINT1_BACK:
    case AV_CH_LAYOUT_5POINT1: return "5.1(side)";
    case AV_CH_LAYOUT_6POINT0: return "6.0";
    case AV_CH_LAYOUT_6POINT0_FRONT: return "6.0(front)";
    case AV_CH_LAYOUT_HEXAGONAL: return "hexagonal";
    case AV_CH_LAYOUT_6POINT1: return "6.1";
    case AV_CH_LAYOUT_6POINT1_BACK: return "6.1";
    case AV_CH_LAYOUT_6POINT1_FRONT: return "6.1(front)";
    case AV_CH_LAYOUT_7POINT0: return "7.0";
    case AV_CH_LAYOUT_7POINT0_FRONT: return "7.0(front)";
    case AV_CH_LAYOUT_7POINT1: return "7.1";
    case AV_CH_LAYOUT_7POINT1_WIDE_BACK: return "7.1(wide)";
    case AV_CH_LAYOUT_7POINT1_WIDE: return "7.1(wide-side)";
    case AV_CH_LAYOUT_OCTAGONAL: return "octagonal";
    case AV_CH_LAYOUT_STEREO_DOWNMIX: return "downmix";
    default: return std::string();
    }
}

double FileInformation::abitDepth() const
{
    auto audioStreams = m_mediaParser->availableAudioStreams();
    if(audioStreams.empty())
        return 0;

    auto stream = audioStreams[0];

    return stream.stream()->codecpar->bits_per_coded_sample;
}

//---------------------------------------------------------------------------
int FileInformation::Frames_Count_Get (size_t Stats_Pos) const
{
    if (Stats_Pos==(size_t)-1)
        Stats_Pos=ReferenceStream_Pos_Get();
    
    if (Stats_Pos>=Stats.size())
        return -1;
    return Stats[Stats_Pos]->x_Max[0];
}

//---------------------------------------------------------------------------
int FileInformation::Frames_Pos_Get (size_t Stats_Pos)
{
    // Looking for the first video stream
    if (Stats_Pos==(size_t)-1)
        Stats_Pos=ReferenceStream_Pos_Get();
    if (Stats_Pos>=Stats.size())
        return -1;

    int Pos;

    if (Stats_Pos!=ReferenceStream_Pos_Get())
    {
        // Computing frame pos based on the first stream
        double TimeStamp=ReferenceStat()->x[1][Frames_Pos];
        Pos=0;
        for (; Pos<Stats[Stats_Pos]->x_Current_Max; Pos++)
        {
            if (Stats[Stats_Pos]->x[1][Pos]>=TimeStamp)
            {
                if (Pos && Stats[Stats_Pos]->x[1][Pos]!=TimeStamp)
                    Pos--;
                break;
            }
        }
    }
    else
        Pos=Frames_Pos;
    
    return Pos;
}

QString FileInformation::Frame_Type_Get(size_t Stats_Pos, size_t frameIndex) const
{
    // Looking for the first video stream
    if (Stats_Pos==(size_t)-1)
        Stats_Pos=ReferenceStream_Pos_Get();
    if (Stats_Pos>=Stats.size())
        return QString();

    QString frameType;

    if(frameIndex == -1)
    {
        if (Stats_Pos!=ReferenceStream_Pos_Get())
        {
            // Computing frame pos based on the first stream
            double TimeStamp=ReferenceStat()->x[1][Frames_Pos];
            int Pos=0;
            for (; Pos<Stats[Stats_Pos]->x_Current_Max; Pos++)
            {
                if (Stats[Stats_Pos]->x[1][Pos]>=TimeStamp)
                {
                    if (Pos && Stats[Stats_Pos]->x[1][Pos]!=TimeStamp)
                        Pos--;

                    frameType = QString("%1").arg(Stats[Stats_Pos]->pict_type_char[Pos]);
                    break;
                }
            }
        }
        else
            frameType = QString("%1").arg(Stats[Stats_Pos]->pict_type_char[Frames_Pos]);
    } else {
        if(frameIndex <= Stats[Stats_Pos]->x_Current_Max)
            frameType = QString("%1").arg(Stats[Stats_Pos]->pict_type_char[frameIndex]);
    }

    return frameType;
}

//---------------------------------------------------------------------------
void FileInformation::Frames_Pos_Set (int Pos, size_t Stats_Pos)
{
    // Looking for the first video stream
    if (Stats_Pos==(size_t)-1)
        Stats_Pos=ReferenceStream_Pos_Get();
    if (Stats_Pos>=Stats.size())
        return;

    if (Stats_Pos!=ReferenceStream_Pos_Get())
    {
        // Computing frame pos based on the first stream
        double TimeStamp=Stats[Stats_Pos]->x[1][Pos];
        Pos=0;
        for (; Pos<ReferenceStat()->x_Current_Max; Pos++)
            if (ReferenceStat()->x[1][Pos]>=TimeStamp)
            {
                if (Pos && ReferenceStat()->x[1][Pos]!=TimeStamp)
                    Pos--;
                break;
            }
    }
    
    if (Pos<0)
        Pos=0;
    if (Pos>=ReferenceStat()->x_Current_Max)
        Pos=ReferenceStat()->x_Current_Max-1;

    if (Frames_Pos==Pos)
        return;
    Frames_Pos=Pos;

    Q_EMIT positionChanged();
}

//---------------------------------------------------------------------------
void FileInformation::Frames_Pos_Minus ()
{
    if (Frames_Pos==0)
        return;

    Frames_Pos--;

    Q_EMIT positionChanged();
}

//---------------------------------------------------------------------------
bool FileInformation::Frames_Pos_Plus ()
{
    if (Frames_Pos+1>=ReferenceStat()->x_Current_Max)
        return false;;

    Frames_Pos++;

    Q_EMIT positionChanged();
    return true;
}

bool FileInformation::Frames_Pos_AtEnd()
{
    auto maxX = ReferenceStat()->x_Current_Max;
    bool atEnd = (Frames_Pos + 1) == maxX;
    return atEnd;
}

//---------------------------------------------------------------------------
bool FileInformation::PlayBackFilters_Available ()
{
    return true;
}

size_t FileInformation::VideoFrameCount_Get()
{
    for(auto i = 0; i < Stats.size(); ++i) {
        auto stat = Stats[i];
        if(stat->Type_Get() == Type_Video)
            return stat->x_Current_Max;
    }

    return 0;
}

bool FileInformation::IsRGB_Get()
{
    return true; // 2DO!!! streamsStats->IsRGB_Get();
}

qreal FileInformation::averageFrameRate() const
{
    /* 2DO !!!
    if(!Glue)
        return 0;

    auto splitted = QString::fromStdString(Glue->AvgVideoFrameRate_Get()).split("/");
    if(splitted.length() == 1)
        return splitted[0].toDouble();

    return splitted[0].toDouble() / splitted[1].toDouble();
    */

    return 25;
}

double FileInformation::TimeStampOfCurrentFrame() const
{
    for(auto i = 0; i < Stats.size(); ++i) {
        auto stat = Stats[i];
        if(stat->Type_Get() == Type_Video) {
            auto videoStat = static_cast<VideoStats*>(stat);
            return videoStat->FirstTimeStamp;
        }
    }

    return DBL_MAX;
}

bool FileInformation::isValid() const
{
    return true; // 2DO !!!
    // return Glue != 0;
}

int FileInformation::BitsPerRawSample(int streamType) const
{
    if(streamType == Type_Video) {
        int streamBitsPerRawSample = streamsStats ? streamsStats->bitsPerRawVideoSample() : 0;
        if(streamBitsPerRawSample != 0)
            return streamBitsPerRawSample;

        if(m_mediaParser && !m_mediaParser->availableVideoStreams().empty() && m_mediaParser->availableVideoStreams()[0].stream()->codecpar->bits_per_raw_sample != 0)
            return m_mediaParser->availableVideoStreams()[0].stream()->codecpar->bits_per_raw_sample;

        if(ReferenceStat()) {
            auto guessedBitsPerRawSample = guessBitsPerRawSampleFromFormat(*ReferenceStat()->pix_fmt);
            if(guessedBitsPerRawSample != 0)
                return guessedBitsPerRawSample;
        }

        return 8;
    } else if(streamType == Type_Audio) {

        int avSampleFormat = streamsStats ? streamsStats->avSampleFormat() : 0;
        if(avSampleFormat != -1)
            return av_get_bytes_per_sample((AVSampleFormat) avSampleFormat) * 8;

        if(m_mediaParser && !m_mediaParser->availableAudioStreams().empty() && m_mediaParser->availableAudioStreams()[0].stream()->codecpar->format != -1)
            return bitsPerAudioSample(m_mediaParser->availableAudioStreams()[0].stream()->codecpar->format) * 8;
    }

    return 0;
}

int FileInformation::audioSampleFormat() const
{
    int avSampleFormat = streamsStats ? streamsStats->avSampleFormat() : 0;
    if(avSampleFormat != -1)
        return avSampleFormat;

    if(m_mediaParser && !m_mediaParser->availableAudioStreams().empty() && m_mediaParser->availableAudioStreams()[0].stream()->codecpar->format != -1)
        return m_mediaParser->availableAudioStreams()[0].stream()->codecpar->format;

    return -1;
}

QPair<int, int> FileInformation::audioRanges() const
{
    auto sampleFormat = audioSampleFormat();
    if(isFloatAudioSampleFormat(sampleFormat)) {
        return QPair<int, int>(-1, 1);
    } else if(isSignedAudioSampleFormat(sampleFormat)) {
        auto bprs = BitsPerRawSample(Type_Audio);
        return QPair<int, int>(-pow(2, bprs - 1) - 1, pow(2, bprs - 1));
    } else if(isUnsignedAudioSampleFormat(sampleFormat)) {
        auto bprs = BitsPerRawSample(Type_Audio);
        return QPair<int, int>(0, pow(2, bprs));
    }

    return QPair<int, int>(0, 0);
}

FileInformation::SignalServerCheckUploadedStatus FileInformation::signalServerCheckUploadedStatus() const
{
    if(checkFileUploadedOperation)
    {
        switch(checkFileUploadedOperation->state())
        {
        case CheckFileUploadedOperation::Unknown:
            return SignalServerCheckUploadedStatus::Checking;
        case CheckFileUploadedOperation::Uploaded:
            return SignalServerCheckUploadedStatus::Uploaded;
        case CheckFileUploadedOperation::NotUploaded:
            return SignalServerCheckUploadedStatus::NotUploaded;
        case CheckFileUploadedOperation::Error:
            return SignalServerCheckUploadedStatus::CheckError;
        }
    }

    return NotChecked;
}

QString FileInformation::signalServerCheckUploadedStatusString() const
{
    switch(signalServerCheckUploadedStatus())
    {
    case NotChecked:
        return "Not Checked";
    case Checking:
        return "Checking";
    case Uploaded:
        return "Uploaded";
    case NotUploaded:
        return "Not Uploaded";
    case CheckError:
        return "Check Error";
    default:
        return "Unknown";
    }
}

QString FileInformation::signalServerCheckUploadedStatusErrorString() const
{
    return checkFileUploadedOperation ? checkFileUploadedOperation->errorString() : QString();
}

FileInformation::SignalServerUploadStatus FileInformation::signalServerUploadStatus() const
{
    if(uploadOperation)
    {
        switch(uploadOperation->state())
        {
        case UploadFileOperation::Uploading:
            return SignalServerUploadStatus::Uploading;
        case UploadFileOperation::Uploaded:
            return SignalServerUploadStatus::Done;
        case UploadFileOperation::Error:
            return SignalServerUploadStatus::UploadError;
        }
    }

    return Idle;
}

QString FileInformation::signalServerUploadStatusString() const
{
    switch(signalServerUploadStatus())
    {
    case Idle:
        return "Idle";
    case Uploading:
        return "Uploading";
    case Done:
        return "Upload Done";
    case UploadError:
        return "Upload Error";
    default:
        return "Unknown";
    }
}

QString FileInformation::signalServerUploadStatusErrorString() const
{
    return uploadOperation ? uploadOperation->errorString() : QString();
}

bool FileInformation::hasStats() const
{
    return m_hasStats;
}

void FileInformation::setAutoCheckFileUploaded(bool enable)
{
    m_autoCheckFileUploaded = enable;
}

void FileInformation::setAutoUpload(bool enable)
{
    m_autoUpload = enable;
}

int FileInformation::index() const
{
    return m_index;
}

void FileInformation::setIndex(int value)
{
    m_index = value;
}

void FileInformation::checkFileUploaded(const QString &fileName)
{
    if(thread() != QThread::currentThread())
    {
        QMetaObject::invokeMethod(this, "checkFileUploaded", Q_ARG(const QString&, fileName));
        return;
    }

    // on UI thread
    checkFileUploadedOperation = signalServer->checkFileUploaded(fileName);
    connect(checkFileUploadedOperation.data(), SIGNAL(finished()), this, SLOT(checkFileUploadedDone()));
}

void FileInformation::checkFileUploadedDone()
{
    if(checkFileUploadedOperation->state() == CheckFileUploadedOperation::NotUploaded)
    {
        qDebug() << "file not uploaded: " << checkFileUploadedOperation->fileName();

        if(signalServer->autoUpload() && m_parsed)
            handleAutoUpload();
    }
    else if(checkFileUploadedOperation->state() == CheckFileUploadedOperation::Uploaded)
    {
        qDebug() << "file is already uploaded: " << checkFileUploadedOperation->fileName();
    }
    else if(checkFileUploadedOperation->state() == CheckFileUploadedOperation::Error)
    {
        qDebug() << "file not uploaded: " << checkFileUploadedOperation->fileName() << ", error: " << checkFileUploadedOperation->errorString();
    }

    Q_EMIT signalServerCheckUploadedStatusChanged();
}

void FileInformation::upload(const QFileInfo& fileInfo)
{
    QString fullName = fileInfo.filePath();
    QSharedPointer<QFile> file = QSharedPointer<QFile>::create(fullName);
    if(file->open(QIODevice::ReadOnly))
    {
        upload(file, fileInfo.fileName());
    }
}

void FileInformation::upload(SharedFile file, const QString &fileName)
{
    uploadOperation = signalServer->uploadFile(fileName, file);
    connect(uploadOperation.data(), SIGNAL(finished()), this, SLOT(uploadDone()));
    connect(uploadOperation.data(), SIGNAL(uploadProgress(qint64, qint64)), this, SIGNAL(signalServerUploadProgressChanged(qint64, qint64)));

    Q_EMIT signalServerUploadStatusChanged();
}

void FileInformation::cancelUpload()
{
    if(uploadOperation)
        uploadOperation->cancel();
}

void FileInformation::setCommentsUpdated(CommonStats *stats)
{
    m_commentsUpdated = true;
    Q_EMIT commentsUpdated(stats);
}

void FileInformation::uploadDone()
{
    Q_EMIT signalServerUploadStatusChanged();
}

void FileInformation::parsingDone(bool success)
{
    if(m_autoUpload && signalServerCheckUploadedStatus() == SignalServerCheckUploadedStatus::NotUploaded)
    {
        qDebug() << "parsing done: " << success;

        if(signalServer->autoUpload() && m_parsed)
            handleAutoUpload();
    }
}

void FileInformation::handleAutoUpload()
{
    QString statsFileName = fileName() + ".qctools.xml.gz";
    QFileInfo fileInfo(statsFileName);

    if(!fileInfo.exists())
    {
        connect(this, SIGNAL(statsFileGenerated(SharedFile, QString)), this, SLOT(upload(SharedFile,QString)), Qt::UniqueConnection);
        startExport();
    } else {
        SharedFile file = SharedFile(new QFile(statsFileName));
        if(file->open(QFile::ReadOnly))
            upload(file, fileInfo.fileName());
    }
}

bool FileInformation::parsed() const
{
    return m_parsed;
}

void FileInformation::setExportFilters(const activefilters &exportFilters)
{
    m_exportFilters = exportFilters;
}

bool FileInformation::commentsUpdated() const
{
    return m_commentsUpdated;
}
