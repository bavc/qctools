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

#include <QProcess>
#include <QFileInfo>
#include <QDesktopServices>
#include <QUrl>
#include <zlib.h>
#include <zconf.h>

#include <string>
#include <sstream>
#ifdef _WIN32
    #include <algorithm>
#endif
//---------------------------------------------------------------------------

//***************************************************************************
// Simultaneous parsing
//***************************************************************************
static int ActiveParsing_Count=0;

void FileInformation::run()
{
    for (;;)
    {
        Glue->NextFrame();
        if (Glue->VideoFramePos>=Glue->VideoFrameCount)
            break;
        if ((Glue->VideoFramePos%10)==0)
        {
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
FileInformation::FileInformation (MainWindow* Main_, const QString &FileName_) :
    FileName(FileName_),
    Main(Main_)
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
    if (!StatsFromExternalData.empty())
        Glue= new FFmpeg_Glue(FileName_string.c_str(), 72, 72, FFmpeg_Glue::Output_JpegList, string(), string(), true, false, false, StatsFromExternalData);
    else
        Glue= new FFmpeg_Glue(FileName_string.c_str(), 72, 72, FFmpeg_Glue::Output_JpegList, "signalstats=stat=tout+vrep+brng,cropdetect=reset=1:round=1,split[a][b];[a]field=top[a1];[b]field=bottom[b1],[a1][b1]psnr", "", true, false, true);

    if (Glue->VideoFrameCount==0 && StatsFromExternalData.empty())
        return; // Problem

    Frames_Pos=0;
    WantToStop=false;
    Parse();
}

//---------------------------------------------------------------------------
FileInformation::~FileInformation ()
{
    WantToStop=true;
    wait();
    delete Glue; Glue=NULL;
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
    stringstream Data;

    // Header
    Data<<"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    Data<<"<!-- Created by QCTools 0.5.0 -->\n";
    Data<<"<ffprobe:ffprobe xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance' xmlns:ffprobe='http://www.ffmpeg.org/schema/ffprobe' xsi:schemaLocation='http://www.ffmpeg.org/schema/ffprobe ffprobe.xsd'>\n";
    Data<<"    <program_version version=\"" << Glue->FFmpeg_Version() << "\" copyright=\"Copyright (c) 2007-" << Glue->FFmpeg_Year() << " the FFmpeg developers\" build_date=\""__DATE__"\" build_time=\""__TIME__"\" compiler_ident=\"" << Glue->FFmpeg_Compiler() << "\" configuration=\"" << Glue->FFmpeg_Configuration() << "\"/>\n";
    Data<<"\n";
    Data<<"    <library_versions>\n";
    Data<<Glue->FFmpeg_LibsVersion();
    Data<<"    </library_versions>\n";
    Data<<"\n";
    Data<<"    <frames>\n";

    // Per frame
    for (size_t x=0; x<Glue->VideoFrameCount_Max; ++x)
    {
        stringstream pkt_pts_time; pkt_pts_time<<Glue->x[1][x];
        stringstream pkt_duration_time; pkt_duration_time<<Glue->d[x];
        stringstream width; width<<Glue->Width_Get();
        stringstream height; height<<Glue->Height_Get();
        stringstream key_frame; key_frame<<Glue->KeyFrame_Get(x);
        Data<<"        <frame media_type=\"video\" key_frame=\"" << key_frame.str() << "\" pkt_pts_time=\"" << pkt_pts_time.str() << "\"";
        if (pkt_duration_time)
            Data<<" pkt_duration_time=\"" << pkt_duration_time.str() << "\"";
        Data<<" width=\"" << width.str() << "\" height=\"" << height.str() <<"\">\n";

        for (size_t Plot_Pos=0; Plot_Pos<PlotName_Max; Plot_Pos++)
        {
            string key=PerPlotName[Plot_Pos].FFmpeg_Name_2_3;

            stringstream value;
            switch (Plot_Pos)
            {
                case PlotName_Crop_x2 :
                case PlotName_Crop_w :
                                        // Special case, values are from width
                                        value<<Glue->Width_Get()-Glue->y[Plot_Pos][x];
                                        break;
                case PlotName_Crop_y2 :
                case PlotName_Crop_h :
                                        // Special case, values are from height
                                        value<<Glue->Height_Get()-Glue->y[Plot_Pos][x];
                                        break;
                default:
                                        value<<Glue->y[Plot_Pos][x];
            }

            Data<<"            <tag key=\""+key+"\" value=\""+value.str()+"\"/>\n";
        }

        Data<<"        </frame>\n";
    }

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

    string StatsToExternalData=Glue->StatsToExternalData();

    QFile F(ExportFileName);
    F.open(QIODevice::WriteOnly|QIODevice::Truncate);
    F.write(StatsToExternalData.c_str(), StatsToExternalData.size());
}


//***************************************************************************
// Info
//***************************************************************************

//---------------------------------------------------------------------------
QPixmap* FileInformation::Picture_Get (size_t Pos)
{
    if (Glue==NULL || Pos>=Glue->VideoFramePos || Pos>=Glue->JpegList.size())
    {
        Pixmap.load(":/icon/logo.png");
        Pixmap=Pixmap.scaled(72, 72);
    }
    else
        Pixmap.loadFromData(Glue->JpegList[Pos]->Data, Glue->JpegList[Pos]->Size);
    return &Pixmap;
}

//---------------------------------------------------------------------------
void FileInformation::Frames_Pos_Set (int Pos)
{
    if (Pos<0)
        Pos=0;
    if (Pos>=Glue->VideoFrameCount)
        Pos=Glue->VideoFrameCount-1;

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
    if (Frames_Pos+1>=Glue->VideoFrameCount)
        return;

    Frames_Pos++;

    if (Main)
        Main->Update();
}

//---------------------------------------------------------------------------
bool FileInformation::PlayBackFilters_Available ()
{
    return !Glue->ContainerFormat_Get().empty();
}

