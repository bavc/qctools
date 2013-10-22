#include "GUI/PerFile.h"
#include "mainwindow.h"

#include <QProcess>
#include <QFileInfo>
#include <QDesktopServices>
#include <QUrl>

#include <ZenLib/Ztring.h>
#include <ZenLib/File.h>
using namespace ZenLib;

//---------------------------------------------------------------------------
PerFile::PerFile ()
{
    // Process handlers
    BasicInfo=NULL;
    Thumbnails=NULL;
    Stats=NULL;
}

//---------------------------------------------------------------------------
PerFile::~PerFile ()
{
    if (Stats)
        Stats->Stop();
    if (Thumbnails)
        Thumbnails->Stop();
    delete Stats;
    delete Thumbnails;
    delete BasicInfo;
}

//---------------------------------------------------------------------------
void PerFile::Export_CSV (const QString &ExportFileName)
{
    Stats->Export_CSV(ExportFileName);
}

//---------------------------------------------------------------------------
void PerFile::Launch (MainWindow* SourceClass_, const QString &FileName_)
{
    // Init - Infos
    FileName=FileName_;

    // Init - For callback
    SourceClass=SourceClass_;

    // BasicInfo creation
    delete BasicInfo; BasicInfo=new ffmpeg_BasicInfo();
    BasicInfo->Launch(this, FileName);
}

//***************************************************************************
// BasicInfo
//***************************************************************************

//---------------------------------------------------------------------------
void PerFile::BasicInfo_Finished ()
{
    // Thumbails creation
    delete Thumbnails; Thumbnails=new ffmpeg_Thumbnails();
    Thumbnails->Launch(this, FileName, BasicInfo->Frames_Total_Get());

    // Stats creation
    delete Stats; Stats=new ffmpeg_Stats();
    Stats->Launch(this, FileName, BasicInfo->Frames_Total_Get());

    // Source
    SourceClass->BasicInfo_Finished();
}

//***************************************************************************
// Thumbnails
//***************************************************************************

//---------------------------------------------------------------------------
void PerFile::Thumbnails_Updated ()
{
    // Source
    SourceClass->Thumbnails_Updated();
}

//---------------------------------------------------------------------------
void PerFile::Thumbnails_Finished ()
{
    // Source
    SourceClass->Thumbnails_Finished();
}

//***************************************************************************
// Stats
//***************************************************************************

//---------------------------------------------------------------------------
void PerFile::Stats_Updated ()
{
    // Source
    SourceClass->Stats_Updated();
}

//---------------------------------------------------------------------------
void PerFile::Stats_Finished ()
{
    // Source
    SourceClass->Stats_Finished();
}
