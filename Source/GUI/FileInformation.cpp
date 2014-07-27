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


#ifdef _WIN32
    #include <string>
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
    std::string StatsFromExternalData;
    QString StatsFromExternalData_FileName;

    // Finding the right file names (both media file and stats file)
    if (FileName.indexOf(".qctools.xml.gz")==FileName.length()-15)
    {
        StatsFromExternalData_FileName=FileName;
        FileName.resize(FileName.length()-15);
    }
    else if (FileName.indexOf(".xml.gz")==FileName.length()-7)
    {
        StatsFromExternalData_FileName=FileName;
        FileName.resize(FileName.length()-7);
    }
    else if (FileName.indexOf(".qctools.xml")==FileName.length()-12)
    {
        StatsFromExternalData_FileName=FileName;
        FileName.resize(FileName.length()-12);
    }
    else if (FileName.indexOf(".xml")==FileName.length()-4)
    {
        StatsFromExternalData_FileName=FileName;
        FileName.resize(FileName.length()-4);
    }
    if (StatsFromExternalData_FileName.size()==0)
    {
        if (QFile::exists(StatsFromExternalData_FileName+".qctools.xml.gz"))
            StatsFromExternalData_FileName=StatsFromExternalData_FileName+".qctools.xml.gz";
        else if (QFile::exists(StatsFromExternalData_FileName+".qctools.xml"))
            StatsFromExternalData_FileName=StatsFromExternalData_FileName+".qctools.xml";
    }

    // External data optional input
    if (StatsFromExternalData.empty())
    {
        QFileInfo FileInfo_XmlGz(FileName+".qctools.xml.gz");
        if (FileInfo_XmlGz.exists())
        {
            QFile F(FileName+".qctools.xml.gz");
            if (F.open(QIODevice::ReadOnly))
            {
                QByteArray Compressed=F.readAll();
                uLongf Buffer_Size=0;
                Buffer_Size|=Compressed[Compressed.size()-1]<<24;
                Buffer_Size|=Compressed[Compressed.size()-2]<<16;
                Buffer_Size|=Compressed[Compressed.size()-3]<<8;
                Buffer_Size|=Compressed[Compressed.size()-4];
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
    }
    if (StatsFromExternalData.empty())
    {
        QFileInfo FileInfo_Xml(FileName+".qctools.xml");
        if (FileInfo_Xml.exists())
        {
            QFile F(FileName+".qctools.xml");
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
        Glue= new FFmpeg_Glue(FileName_string.c_str(), 72, 72, FFmpeg_Glue::Output_JpegList, "signalstats=stat=tout+vrep+brng,cropdetect,split[a][b];[a]field=top[a1];[b]field=bottom[b1],[a1][b1]psnr", "", true, false, true);

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
void FileInformation::Import_XmlGz (const QString &ExportFileName)
{
    //TODO
}

//---------------------------------------------------------------------------
void FileInformation::Export_XmlGz (const QString &ExportFileName)
{
    //TODO
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

