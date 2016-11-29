/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include "GUI/FileInformation.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "GUI/mainwindow.h"
#include "Core/FFmpeg_Glue.h"
#include "Core/BlackmagicDeckLink_Glue.h"
#include "Core/VideoStats.h"
#include "Core/AudioStats.h"

#include <QProcess>
#include <QFileInfo>
#include <QDesktopServices>
#include <QApplication>
#include <QDebug>
#include <QUrl>
#include <zlib.h>
#include <zconf.h>

#include <string>
#include <sstream>
#ifdef _WIN32
    #include <algorithm>
#endif

//***************************************************************************
// Simultaneous parsing
//***************************************************************************
static int ActiveParsing_Count=0;

void FileInformation::run()
{
    if (blackmagicDeckLink_Glue)
    {
        blackmagicDeckLink_Glue->Glue=Glue;
            
        for (;;)
        {
            switch (blackmagicDeckLink_Glue->Config_Out.Status)
            {
                case BlackmagicDeckLink_Glue::instancied:
                                                        blackmagicDeckLink_Glue->Start();
                                                        break;
                case BlackmagicDeckLink_Glue::finished: 
                                                        WantToStop=true;
                                                        break;
                default : ;
            }
                
            if (WantToStop)
                break;
            yieldCurrentThread();
        }

        delete blackmagicDeckLink_Glue; blackmagicDeckLink_Glue=NULL;
    }
    else
    {
        for (;;)
        {
            if (Glue)
            {
                if (!Glue->NextFrame())
                    break;
            }
            if (WantToStop)
                break;
            yieldCurrentThread();
        }
    }

    ActiveParsing_Count--;
}



//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
FileInformation::FileInformation (MainWindow* Main_, const QString &FileName_, activefilters ActiveFilters_, activealltracks ActiveAllTracks_, BlackmagicDeckLink_Glue* blackmagicDeckLink_Glue_, int FrameCount, const string &Encoding_FileName, const std::string &Encoding_Format) :
    FileName(FileName_),
    ActiveFilters(ActiveFilters_),
    ActiveAllTracks(ActiveAllTracks_),
    Main(Main_),
    blackmagicDeckLink_Glue(blackmagicDeckLink_Glue_)
{
    QString StatsFromExternalData_FileName;
    bool StatsFromExternalData_FileName_IsCompressed=false;

    // Finding the right file names (both media file and stats file)
    if (FileName.indexOf(".qctools.xml.gz")==FileName.length()-15)
    {
        StatsFromExternalData_FileName=FileName;
        FileName.resize(FileName.length()-15);
        StatsFromExternalData_FileName_IsCompressed=true;
    }
    else if (FileName.indexOf(".qctools.xml")==FileName.length()-12)
    {
        StatsFromExternalData_FileName=FileName;
        FileName.resize(FileName.length()-12);
    }
    else if (FileName.indexOf(".xml.gz")==FileName.length()-7)
    {
        StatsFromExternalData_FileName=FileName;
        FileName.resize(FileName.length()-7);
        StatsFromExternalData_FileName_IsCompressed=true;
    }
    else if (FileName.indexOf(".xml")==FileName.length()-4)
    {
        StatsFromExternalData_FileName=FileName;
        FileName.resize(FileName.length()-4);
    }
    if (StatsFromExternalData_FileName.size()==0)
    {
        if (QFile::exists(FileName+".qctools.xml.gz"))
        {
            StatsFromExternalData_FileName=FileName+".qctools.xml.gz";
            StatsFromExternalData_FileName_IsCompressed=true;
        }
        else if (QFile::exists(FileName+".qctools.xml"))
        {
            StatsFromExternalData_FileName=FileName+".qctools.xml";
        }
        else if (QFile::exists(FileName+".xml.gz"))
        {
            StatsFromExternalData_FileName=FileName+".xml.gz";
            StatsFromExternalData_FileName_IsCompressed=true;
        }
        else if (QFile::exists(FileName+".xml"))
        {
            StatsFromExternalData_FileName=FileName+".xml";
        }
    }

    // External data optional input
    string StatsFromExternalData;
    if (!StatsFromExternalData_FileName.size()==0)
    {
        if (StatsFromExternalData_FileName_IsCompressed)
        {
            QFile F(StatsFromExternalData_FileName);
            if (F.open(QIODevice::ReadOnly))
            {
                QByteArray Compressed=F.readAll();
                uLongf Buffer_Size=0;
                Buffer_Size|=((unsigned char)Compressed[Compressed.size()-1])<<24;
                Buffer_Size|=((unsigned char)Compressed[Compressed.size()-2])<<16;
                Buffer_Size|=((unsigned char)Compressed[Compressed.size()-3])<<8;
                Buffer_Size|=((unsigned char)Compressed[Compressed.size()-4]);
                char* Buffer=new char[Buffer_Size];
                z_stream strm;
                strm.next_in = (Bytef *) Compressed.data();
                strm.avail_in = Compressed.size() ;
                strm.next_out = (unsigned char*) Buffer;
                strm.avail_out = Buffer_Size;
                strm.total_out = 0;
                strm.zalloc = Z_NULL;
                strm.zfree = Z_NULL;
                if (inflateInit2(&strm, 15 + 16)>=0) // 15 + 16 are magic values for gzip
                {
                    if (inflate(&strm, Z_SYNC_FLUSH)>=0)
                    {
                        inflateEnd (&strm);
                        StatsFromExternalData.assign(Buffer, Buffer_Size);
                    }
                }
                delete[] Buffer;
            }
        }
        else
        {
            QFile F(StatsFromExternalData_FileName);
            if (F.open(QIODevice::ReadOnly))
            {
                QByteArray Data=F.readAll();

                StatsFromExternalData.assign(Data.data(), Data.size());
            }
        }
    }

    // Running FFmpeg
    string FileName_string=FileName.toUtf8().data();
    #ifdef _WIN32
        replace(FileName_string.begin(), FileName_string.end(), '/', '\\' );
    #endif
    string Filters[Type_Max];
    if (StatsFromExternalData.empty())
    {
        if (ActiveFilters[ActiveFilter_Video_signalstats])
            Filters[0]+=",signalstats=stat=tout+vrep+brng";
        if (ActiveFilters[ActiveFilter_Video_cropdetect])
            Filters[0]+=",cropdetect=reset=1:round=1";
        if (ActiveFilters[ActiveFilter_Video_Idet])
            Filters[0]+=",idet=half_life=1";
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
        if (ActiveFilters[ActiveFilter_Audio_EbuR128])
            Filters[1]+=",ebur128=metadata=1";
        if (ActiveFilters[ActiveFilter_Audio_astats])
            Filters[1]+=",astats=metadata=1:reset=1:length=0.4";
        Filters[1].erase(0, 1); // remove first comma
    }
    else
    {
        VideoStats* Video=new VideoStats();
        Stats.push_back(Video);
        Video->StatsFromExternalData(StatsFromExternalData);

        AudioStats* Audio=new AudioStats();
        Stats.push_back(Audio);
        Audio->StatsFromExternalData(StatsFromExternalData);
    }
    Glue=new FFmpeg_Glue(FileName_string.c_str(), ActiveAllTracks, &Stats, Stats.empty());
    if (FileName_string.empty())
    {
        Glue->AddInput_Video(FrameCount, 1001, 30000, 720, 486, blackmagicDeckLink_Glue->Config_In.VideoBitDepth, blackmagicDeckLink_Glue->Config_In.VideoCompression, blackmagicDeckLink_Glue->Config_In.TC_in);
        Glue->AddInput_Audio(FrameCount, 1001, 30000, 48000, blackmagicDeckLink_Glue->Config_In.AudioBitDepth, blackmagicDeckLink_Glue->Config_In.AudioTargetBitDepth, blackmagicDeckLink_Glue->Config_In.ChannelsCount);
    }
    else if (Glue->ContainerFormat_Get().empty())
    {
        delete Glue;
        Glue=NULL;
        for (size_t Pos=0; Pos<Stats.size(); Pos++)
            Stats[Pos]->StatsFinish();
    }

    if (Glue)
    {
        Glue->AddOutput(0, 72, 72, FFmpeg_Glue::Output_Jpeg);
        if (!Encoding_FileName.empty())
        {
            Glue->AddOutput(Encoding_FileName, Encoding_Format);
            FileName=QString::fromUtf8(Encoding_FileName.c_str());
        }
        Glue->AddOutput(1, 0, 0, FFmpeg_Glue::Output_Stats, 0, Filters[0]);
        Glue->AddOutput(0, 0, 0, FFmpeg_Glue::Output_Stats, 1, Filters[1]);
    }

    // Looking for the first video stream
    ReferenceStream_Pos=0;
    for (; ReferenceStream_Pos<Stats.size(); ReferenceStream_Pos++)
        if (Stats[ReferenceStream_Pos] && Stats[ReferenceStream_Pos]->Type_Get()==0)
            break;
    if (ReferenceStream_Pos>=Stats.size())
        ReferenceStream_Pos=0;

    Frames_Pos=0;
    WantToStop=false;
    Parse();
}

//---------------------------------------------------------------------------
FileInformation::~FileInformation ()
{
    WantToStop=true;
    wait();

    for (size_t Pos=0; Pos<Stats.size(); Pos++)
        delete Stats[Pos];
    delete Glue;
}

//***************************************************************************
// Parsing
//***************************************************************************

//---------------------------------------------------------------------------
void FileInformation::Parse ()
{
    int Max=QThread::idealThreadCount();
    if (Max>2)
        Max-=2;
    else
        Max=1;
    if (!isRunning() && ActiveParsing_Count<Max)
    {
        ActiveParsing_Count++;
        start();
    }
}

//***************************************************************************
// Actions
//***************************************************************************

//---------------------------------------------------------------------------
void FileInformation::Export_XmlGz (const QString &ExportFileName)
{
    if (!Glue)
        return;

    stringstream Data;

    // Header
    Data<<"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    Data<<"<!-- Created by QCTools " << Version << " -->\n";
    Data<<"<ffprobe:ffprobe xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance' xmlns:ffprobe='http://www.ffmpeg.org/schema/ffprobe' xsi:schemaLocation='http://www.ffmpeg.org/schema/ffprobe ffprobe.xsd'>\n";
    Data<<"    <program_version version=\"" << Glue->FFmpeg_Version() << "\" copyright=\"Copyright (c) 2007-" << Glue->FFmpeg_Year() << " the FFmpeg developers\" build_date=\"" __DATE__ "\" build_time=\"" __TIME__ "\" compiler_ident=\"" << Glue->FFmpeg_Compiler() << "\" configuration=\"" << Glue->FFmpeg_Configuration() << "\"/>\n";
    Data<<"\n";
    Data<<"    <library_versions>\n";
    Data<<Glue->FFmpeg_LibsVersion();
    Data<<"    </library_versions>\n";
    Data<<"\n";
    Data<<"    <frames>\n";

    // From stats
    for (size_t Pos=0; Pos<Stats.size(); Pos++)
        if (Stats[Pos])
            Data<<Stats[Pos]->StatsToXML(Glue->Width_Get(), Glue->Height_Get());

    // Footer
    Data<<"    </frames>\n";
    Data<<"</ffprobe:ffprobe>";

    QFile F(ExportFileName);
    if (F.open(QIODevice::WriteOnly))
    {
        string DataS=Data.str();
        QByteArray Compressed=F.readAll();
        uLongf Buffer_Size=65536;
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
                F.write(Buffer, Buffer_Size-strm.avail_out);
            }
            while (strm.avail_out == 0);
            deflateEnd (&strm);
        }
        delete[] Buffer;
    }
}

//---------------------------------------------------------------------------
void FileInformation::Export_CSV (const QString &ExportFileName)
{
    if (ExportFileName.isEmpty())
        return;

    string StatsToExternalData=ReferenceStat()->StatsToCSV();

    QFile F(ExportFileName);
    F.open(QIODevice::WriteOnly|QIODevice::Truncate);
    F.write(StatsToExternalData.c_str(), StatsToExternalData.size());
}


//***************************************************************************
// Info
//***************************************************************************

//---------------------------------------------------------------------------
QPixmap FileInformation::Picture_Get (size_t Pos)
{
    QPixmap Pixmap;

    if (!Glue || Pos>=ReferenceStat()->x_Current || Pos>=Glue->Thumbnails_Size(0))
    {
        Pixmap.load(":/icon/logo.png");
        Pixmap=Pixmap.scaled(72, 72);
    }
    else
    {
        auto Thumbnail = Glue->Thumbnail_Get(0, Pos);
        if (!Thumbnail.isEmpty())
            Pixmap.loadFromData(Thumbnail);
        else
        {
            Pixmap.load(":/icon/logo.png");
            Pixmap=Pixmap.scaled(72, 72);
        }
    }
    return Pixmap;
}

//---------------------------------------------------------------------------
int FileInformation::Frames_Count_Get (size_t Stats_Pos)
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

                    frameType = FFmpeg_Glue::frameTypeToString(Stats[Stats_Pos]->pict_type[Pos]);
                    break;
                }
            }
        }
        else
            frameType = FFmpeg_Glue::frameTypeToString(Stats[Stats_Pos]->pict_type[Frames_Pos]);
    } else {
        if(frameIndex <= Stats[Stats_Pos]->x_Current_Max)
            frameType = FFmpeg_Glue::frameTypeToString(Stats[Stats_Pos]->pict_type[frameIndex]);
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

    if (Main)
        Main->Update();
}

//---------------------------------------------------------------------------
void FileInformation::Frames_Pos_Minus ()
{
    if (Frames_Pos==0)
        return;

    Frames_Pos--;

    if (Main)
        Main->Update();
}

//---------------------------------------------------------------------------
void FileInformation::Frames_Pos_Plus ()
{
    if (Frames_Pos+1>=ReferenceStat()->x_Current_Max)
        return;

    Frames_Pos++;

    if (Main)
        Main->Update();
}

//---------------------------------------------------------------------------
bool FileInformation::PlayBackFilters_Available ()
{
    return Glue;
}

qreal FileInformation::averageFrameRate() const
{
    if(!Glue)
        return 0;

    auto splitted = QString::fromStdString(Glue->AvgVideoFrameRate_Get()).split("/");
    if(splitted.length() == 1)
        return splitted[0].toDouble();

    return splitted[0].toDouble() / splitted[1].toDouble();
}

int FileInformation::BitsPerRawSample() const
{
    if(!Glue)
        return 0;

    return Glue->BitsPerRawSample_Get();
}

