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


#ifdef _WIN32
    #include <string>
    #include <algorithm>
#endif
//---------------------------------------------------------------------------

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
}

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
FileInformation::FileInformation (MainWindow* Main_, const QString &FileName_) :
    FileName(FileName_),
    Main(Main_)
{
    // Running CSV feed
    if (FileName.indexOf(".qctools.csv")+12==FileName.length())
        FileName.resize(FileName.length()-12);
    QFileInfo FileInfo_CSV(FileName+".qctools.csv");
    string FileName_string=FileName.toUtf8().data();
    #ifdef _WIN32
        replace(FileName_string.begin(), FileName_string.end(), '/', '\\' );
    #endif
    if (FileInfo_CSV.exists())
    {
        string StatsFromExternalData;
        QFile F(FileName+".qctools.csv");
        if (F.open(QIODevice::ReadOnly))
        {
            QByteArray Data=F.readAll();
            StatsFromExternalData.assign(Data.data(), Data.size());
        }
        Glue= new FFmpeg_Glue(FileName_string.c_str(), 72, 72, FFmpeg_Glue::Output_JpegList, string(), string(), true, false, false, StatsFromExternalData);
    }
    else
        Glue= new FFmpeg_Glue(FileName_string.c_str(), 72, 72, FFmpeg_Glue::Output_JpegList, "values=stat=tout|vrep|rang|head", "", true, false, true);

    if (Glue->VideoFrameCount==0)
        return; // Problem

    Frames_Pos=0;
    WantToStop=false;
    start();
}

//---------------------------------------------------------------------------
FileInformation::~FileInformation ()
{
    WantToStop=true;
    wait();
    delete Glue; Glue=NULL;
}

//***************************************************************************
// Actions
//***************************************************************************

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
    if (Glue==NULL || Pos>=Glue->VideoFramePos)
    {
        Pixmap.load(":/icon/logo.jpg");
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
